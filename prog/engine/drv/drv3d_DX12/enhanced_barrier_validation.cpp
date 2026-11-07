// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texFlags.h>
#include <drv_log_defs.h>
#include <util/dag_stringify.h>

#include "enhanced_barrier_validation.h"
#include "texture.h"


namespace
{
using namespace drv3d_dx12;

constexpr d3d::AccessFlags buffer_only_access_mask =
  d3d::AccessFlag::IndirectArgument | d3d::AccessFlag::VertexBuffer | d3d::AccessFlag::IndexBuffer | d3d::AccessFlag::ConstantBuffer;

constexpr d3d::AccessFlags texture_only_access_mask =
  d3d::AccessFlag::RenderTargetRead | d3d::AccessFlag::RenderTargetWrite | d3d::AccessFlag::DepthStencilWrite |
  d3d::AccessFlag::DepthStencilRead | d3d::AccessFlag::InputAttachment | d3d::AccessFlag::BlitRead | d3d::AccessFlag::BlitWrite |
  d3d::AccessFlag::ResolveRead | d3d::AccessFlag::ResolveWrite | d3d::AccessFlag::ClearTargetWrite | d3d::AccessFlag::ShadingRate;

constexpr d3d::AccessFlags shader_resource_access_mask =
  d3d::AccessFlag::ShaderResource | d3d::AccessFlag::InputAttachment | d3d::AccessFlag::BlitRead;

constexpr d3d::AccessFlags render_target_access_mask =
  d3d::AccessFlag::RenderTargetRead | d3d::AccessFlag::RenderTargetWrite | d3d::AccessFlag::BlitWrite;

constexpr d3d::AccessFlags depth_stencil_access_mask = d3d::AccessFlag::DepthStencilWrite | d3d::AccessFlag::DepthStencilRead;

constexpr d3d::AccessFlags copy_access_mask = d3d::AccessFlag::CopyRead | d3d::AccessFlag::CopyWrite;

constexpr d3d::AccessFlags resolve_access_mask = d3d::AccessFlag::ResolveRead | d3d::AccessFlag::ResolveWrite;

constexpr d3d::AccessFlags unordered_access_mask = d3d::AccessFlag::UnorderedAccess | d3d::AccessFlag::ClearWrite;

constexpr d3d::AccessFlags clear_access_mask = d3d::AccessFlag::ClearWrite | d3d::AccessFlag::ClearTargetWrite;

constexpr d3d::PipelineStageFlags vertex_shading_stage_mask =
  d3d::PipelineStageFlag::VertexAttributeInput | d3d::PipelineStageFlag::AllVertexShading;

constexpr d3d::PipelineStageFlags pixel_shading_stage_mask =
  d3d::PipelineStageFlag::PixelShading | d3d::PipelineStageFlag::Rasterization;

constexpr d3d::PipelineStageFlags depth_stencil_stage_mask =
  d3d::PipelineStageFlag::EarlyFragmentTests | d3d::PipelineStageFlag::LateFragmentTests;

constexpr d3d::PipelineStageFlags shading_stage_mask =
  d3d::PipelineStageFlag::All | vertex_shading_stage_mask | pixel_shading_stage_mask | d3d::PipelineStageFlag::ComputeShading;

constexpr uint32_t all_subresources_index = 0xffffffffu;

uint32_t write_access_type_count(d3d::AccessFlags access)
{
  uint32_t count = 0;
  count += (access & render_target_access_mask) ? 1 : 0;
  count += (access & d3d::AccessFlag::UnorderedAccess) ? 1 : 0;
  count += (access & d3d::AccessFlag::DepthStencilWrite) ? 1 : 0;
  count += (access & d3d::AccessFlag::CopyWrite) ? 1 : 0;
  count += (access & d3d::AccessFlag::ResolveWrite) ? 1 : 0;
  count += (access & d3d::AccessFlag::ClearWrite) ? 1 : 0;
  count += (access & d3d::AccessFlag::ClearTargetWrite) ? 1 : 0;
  return count;
}

d3d::AccessFlags accesses_allowed_in(d3d::TextureLayout layout, d3d::AccessFlags access, bool is_depth)
{
  switch (layout)
  {
    case d3d::TextureLayout::GenericRead:
      return shader_resource_access_mask | d3d::AccessFlag::CopyRead | d3d::AccessFlag::DepthStencilRead |
             d3d::AccessFlag::ResolveRead | d3d::AccessFlag::ShadingRate;
    case d3d::TextureLayout::RenderTarget:
    case d3d::TextureLayout::BlitDest: return render_target_access_mask;
    case d3d::TextureLayout::UnorderedAccess: return d3d::AccessFlag::UnorderedAccess;
    case d3d::TextureLayout::DepthRwStencilRw:
    case d3d::TextureLayout::DepthRwStencilRo:
    case d3d::TextureLayout::DepthRoStencilRw:
    case d3d::TextureLayout::DepthRw: return depth_stencil_access_mask;
    case d3d::TextureLayout::DepthRoStencilRo:
    case d3d::TextureLayout::DepthRo: return d3d::AccessFlag::DepthStencilRead;
    case d3d::TextureLayout::ShaderResource:
    case d3d::TextureLayout::BlitSource: return shader_resource_access_mask;
    case d3d::TextureLayout::CopySource: return d3d::AccessFlag::CopyRead;
    case d3d::TextureLayout::CopyDest: return d3d::AccessFlag::CopyWrite;
    case d3d::TextureLayout::ResolveSource: return d3d::AccessFlag::ResolveRead;
    case d3d::TextureLayout::ResolveDest: return d3d::AccessFlag::ResolveWrite;
    case d3d::TextureLayout::ShadingRateSource: return d3d::AccessFlag::ShadingRate;
    case d3d::TextureLayout::ClearDest:
      if (access & d3d::AccessFlag::ClearTargetWrite)
        return d3d::AccessFlag::ClearTargetWrite | (is_depth ? depth_stencil_access_mask : render_target_access_mask);
      return d3d::AccessFlag::ClearWrite | d3d::AccessFlag::UnorderedAccess;
    default: return {};
  }
}

d3d::AccessFlag lowest_access_flag(uint32_t bits) { return static_cast<d3d::AccessFlag>(bits & (0u - bits)); }

void validate_enhanced_barrier_access(d3d::PipelineStageFlags stages, d3d::AccessFlags access, bool is_depth, const char *res_name)
{
  if (write_access_type_count(access) > 1)
    D3D_CONTRACT_ERROR("DX12: enhanced barrier for <%s>: access mask 0x%08X sets more than one write access type", res_name,
      access.asInteger());

  const bool anyStage = bool(stages & d3d::PipelineStageFlag::All);
  const bool shadingStage = bool(stages & shading_stage_mask);
  const bool clearStage = anyStage || bool(stages & d3d::PipelineStageFlag::Clear);
  const bool clearViewStage = bool(stages & d3d::PipelineStageFlag::ClearView);
  const bool uavClear = clearStage && (access & d3d::AccessFlag::ClearWrite);
  const bool targetClear = clearStage && (access & d3d::AccessFlag::ClearTargetWrite);
  const bool blitReadStage = (stages & d3d::PipelineStageFlag::Blit) && (access & d3d::AccessFlag::BlitRead);
  const bool blitWriteStage = (stages & d3d::PipelineStageFlag::Blit) && (access & d3d::AccessFlag::BlitWrite);
  const bool renderTargetSynced = anyStage || bool(stages & d3d::PipelineStageFlag::OutputMerging) || blitWriteStage ||
                                  (!is_depth && (clearViewStage || targetClear));
  const bool unorderedSynced = shadingStage || uavClear;
  const bool depthStencilSynced = anyStage || bool(stages & depth_stencil_stage_mask) || (is_depth && (clearViewStage || targetClear));

  auto requireStage = [access, res_name](d3d::AccessFlags group, bool synced) {
    if (synced)
      return;
    if (const uint32_t unsynced = (access & group).asInteger())
      D3D_CONTRACT_ERROR("DX12: enhanced barrier for <%s>: access %s has no matching pipeline stage", res_name,
        to_string(lowest_access_flag(unsynced)));
  };
  requireStage(d3d::AccessFlag::IndirectArgument, anyStage || bool(stages & d3d::PipelineStageFlag::ExecuteIndirect));
  requireStage(d3d::AccessFlag::VertexBuffer, anyStage || bool(stages & vertex_shading_stage_mask));
  requireStage(d3d::AccessFlag::IndexBuffer, anyStage || bool(stages & d3d::PipelineStageFlag::IndexInput));
  requireStage(d3d::AccessFlag::ConstantBuffer, shadingStage);
  requireStage(shader_resource_access_mask, shadingStage || blitReadStage);
  requireStage(render_target_access_mask, renderTargetSynced);
  requireStage(d3d::AccessFlag::UnorderedAccess, unorderedSynced);
  requireStage(depth_stencil_access_mask, depthStencilSynced);
  requireStage(copy_access_mask, anyStage || bool(stages & d3d::PipelineStageFlag::Copy));
  requireStage(resolve_access_mask, anyStage || bool(stages & d3d::PipelineStageFlag::Resolve));
  requireStage(d3d::AccessFlag::ShadingRate, anyStage || bool(stages & pixel_shading_stage_mask));
  requireStage(clear_access_mask, clearStage);
}

void validate_enhanced_texture_layout(d3d::TextureLayout layout, d3d::AccessFlags access, BaseTex *btex)
{
  const char *texName = btex->getName();
  if (layout == d3d::TextureLayout::Undefined)
  {
    if (access != d3d::AccessFlags{})
      D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: Undefined layout requires a NoAccess access mask", texName);
    return;
  }

  const bool isDepth = btex->getFormat().isDepth();
  if (const uint32_t incompatible = access.asInteger() & ~accesses_allowed_in(layout, access, isDepth).asInteger())
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: access %s is not compatible with layout %s", texName,
      to_string(lowest_access_flag(incompatible)), to_string(layout));

  switch (layout)
  {
    case d3d::TextureLayout::RenderTarget:
    case d3d::TextureLayout::BlitDest:
      if (!btex->isRenderTarget() || isDepth)
        D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: %s layout requires a color texture created with TEXCF_RTARGET",
          texName, to_string(layout));
      break;
    case d3d::TextureLayout::UnorderedAccess:
      if (!btex->isUav())
        D3D_CONTRACT_ERROR(
          "DX12: enhanced_texture_barrier for <%s>: UnorderedAccess layout requires a texture created with TEXCF_UNORDERED", texName);
      break;
    case d3d::TextureLayout::DepthRwStencilRw:
    case d3d::TextureLayout::DepthRwStencilRo:
    case d3d::TextureLayout::DepthRoStencilRw:
    case d3d::TextureLayout::DepthRoStencilRo:
    case d3d::TextureLayout::DepthRw:
    case d3d::TextureLayout::DepthRo:
      if (!isDepth)
        D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: %s layout requires a depth-format texture", texName,
          to_string(layout));
      break;
    case d3d::TextureLayout::ClearDest:
      if (!(access & clear_access_mask))
        D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: ClearDest layout requires ClearWrite or ClearTargetWrite access",
          texName);
      if ((access & d3d::AccessFlag::ClearWrite) && !btex->isUav())
        D3D_CONTRACT_ERROR(
          "DX12: enhanced_texture_barrier for <%s>: ClearWrite access requires a texture created with TEXCF_UNORDERED", texName);
      if ((access & d3d::AccessFlag::ClearTargetWrite) && !btex->isRenderTarget())
        D3D_CONTRACT_ERROR(
          "DX12: enhanced_texture_barrier for <%s>: ClearTargetWrite access requires a texture created with TEXCF_RTARGET", texName);
      break;
    case d3d::TextureLayout::ShadingRateSource:
      if (0 == (btex->cflg & TEXCF_VARIABLE_RATE))
        D3D_CONTRACT_ERROR(
          "DX12: enhanced_texture_barrier for <%s>: ShadingRateSource layout requires a texture created with TEXCF_VARIABLE_RATE",
          texName);
      break;
    default: break;
  }
}

void validate_enhanced_texture_subresources(const d3d::TextureSubresourceRange &subresources, BaseTex *btex)
{
  if (all_subresources_index == subresources.mips.first && 0 == subresources.mips.count)
    return;

  const char *texName = btex->getName();

  if (0 == subresources.mips.count || 0 == subresources.layers.count || 0 == subresources.planes.count)
  {
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: subresource range has a zero count (mips %u, layers %u, planes %u)",
      texName, subresources.mips.count, subresources.layers.count, subresources.planes.count);
    return;
  }

  const uint32_t mipCount = static_cast<uint32_t>(btex->level_count());
  const uint32_t layerCount = btex->getArrayCount().count();
  const uint32_t planeCount = btex->getFormat().getPlanes().count();
  if (subresources.mips.first >= mipCount || subresources.mips.count > mipCount - subresources.mips.first)
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: mip range [%u, %u) is out of the %u mip levels of the texture",
      texName, subresources.mips.first, subresources.mips.first + subresources.mips.count, mipCount);
  if (subresources.layers.first >= layerCount || subresources.layers.count > layerCount - subresources.layers.first)
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: array range [%u, %u) is out of the %u array layers of the texture",
      texName, subresources.layers.first, subresources.layers.first + subresources.layers.count, layerCount);
  if (subresources.planes.first >= planeCount || subresources.planes.count > planeCount - subresources.planes.first)
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: plane range [%u, %u) is out of the %u format planes of the texture",
      texName, subresources.planes.first, subresources.planes.first + subresources.planes.count, planeCount);
}
} // namespace

void drv3d_dx12::validate_enhanced_buffer_barrier(const d3d::BufferBarrier &barrier, Sbuffer *buffer)
{
  const char *bufName = buffer->getBufName();
  if (const uint32_t textureOnly = (barrier.memorySync.src & texture_only_access_mask).asInteger())
    D3D_CONTRACT_ERROR("DX12: enhanced_buffer_barrier for <%s>: source access mask contains texture-only flags (%s)", bufName,
      to_string(lowest_access_flag(textureOnly)));
  if (const uint32_t textureOnly = (barrier.memorySync.dst & texture_only_access_mask).asInteger())
    D3D_CONTRACT_ERROR("DX12: enhanced_buffer_barrier for <%s>: destination access mask contains texture-only flags (%s)", bufName,
      to_string(lowest_access_flag(textureOnly)));

  const uint32_t needsUav = ((barrier.memorySync.src | barrier.memorySync.dst) & unordered_access_mask).asInteger();
  if (0 != needsUav && 0 == (buffer->getFlags() & SBCF_BIND_UNORDERED))
    D3D_CONTRACT_ERROR("DX12: enhanced_buffer_barrier for <%s>: access %s requires a buffer created with SBCF_BIND_UNORDERED", bufName,
      to_string(lowest_access_flag(needsUav)));

  validate_enhanced_barrier_access(barrier.pipelineSync.src, barrier.memorySync.src, false, bufName);
  validate_enhanced_barrier_access(barrier.pipelineSync.dst, barrier.memorySync.dst, false, bufName);
}

void drv3d_dx12::validate_enhanced_texture_barrier(const d3d::TextureBarrier &barrier, BaseTex *btex)
{
  const char *texName = btex->getName();
  if (const uint32_t bufferOnly = (barrier.memorySync.src & buffer_only_access_mask).asInteger())
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: source access mask contains buffer-only flags (%s)", texName,
      to_string(lowest_access_flag(bufferOnly)));
  if (const uint32_t bufferOnly = (barrier.memorySync.dst & buffer_only_access_mask).asInteger())
    D3D_CONTRACT_ERROR("DX12: enhanced_texture_barrier for <%s>: destination access mask contains buffer-only flags (%s)", texName,
      to_string(lowest_access_flag(bufferOnly)));

  const bool isDepth = btex->getFormat().isDepth();
  validate_enhanced_barrier_access(barrier.pipelineSync.src, barrier.memorySync.src, isDepth, texName);
  validate_enhanced_barrier_access(barrier.pipelineSync.dst, barrier.memorySync.dst, isDepth, texName);
  validate_enhanced_texture_layout(barrier.layoutTransition.src, barrier.memorySync.src, btex);
  validate_enhanced_texture_layout(barrier.layoutTransition.dst, barrier.memorySync.dst, btex);
  validate_enhanced_texture_subresources(barrier.subresources, btex);
}
