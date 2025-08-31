// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stage_state_base.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include <math/dag_lsbVisitor.h>
#include "globals.h"
#include "dummy_resources.h"
#include "backend.h"
#include "execution_state.h"
#include "execution_context.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

PipelineStageStateBase::PipelineStageStateBase() { dtab.clear(); }

void PipelineStageStateBase::invalidateState()
{
  bBinds.markDirty();
  tBinds.markDirty();
  uBinds.markDirty();
  sBinds.markDirty();
  bOffsetDirtyMask = 0;
  lastDescriptorSet = VulkanNullHandle();
  pushConstantsChanged = true;
}

void PipelineStageStateBase::syncDepthROStateInT(Image *image, uint32_t, uint32_t, bool ro_ds)
{
  for (uint32_t j : LsbVisitor{tBinds.validMask()})
  {
    TRegister &reg = tBinds[j];
    if (reg.type != TRegister::TYPE_IMG)
      continue;

    bool imageMatch = reg.img.ptr == image;
    bool dropConstFromNonCurrentTarget = !imageMatch && isConstDepthStencil.test(j);
    bool currentTargetConstMismatch = imageMatch && (isConstDepthStencil.test(j) != ro_ds);

    if (currentTargetConstMismatch && ro_ds)
      isConstDepthStencil.set(j);
    else if (dropConstFromNonCurrentTarget || (currentTargetConstMismatch && !ro_ds))
      isConstDepthStencil.reset(j);
    else
      continue;

    VkImageLayout targetLayout = getImgLayout(isConstDepthStencil.test(j));
    dtab.TSampledImage(j) += targetLayout;

    tBinds.set(j);
  }
}

void PipelineStageStateBase::setImmediateConsts(const uint32_t *data)
{
  memcpy(pushConstants, data, sizeof(uint32_t) * MAX_IMMEDIATE_CONST_WORDS);
  pushConstantsChanged = true;
}

void PipelineStageStateBase::setBempty(uint32_t unit)
{
  bBinds.clear(unit);
  dtab.BConstBuffer(unit).clear();
}

void PipelineStageStateBase::setTempty(uint32_t unit)
{
  tBinds.clear(unit);
  dtab.TBuffer(unit).clear();
  dtab.TSampledBuffer(unit).clear();
  dtab.TSampledImage(unit).clear();
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  dtab.TRaytraceAS(unit).clear();
#endif
}

void PipelineStageStateBase::setUempty(uint32_t unit)
{
  uBinds.clear(unit);
  dtab.UBuffer(unit).clear();
  dtab.USampledBuffer(unit).clear();
  dtab.UImage(unit).clear();
}

void PipelineStageStateBase::setSempty(uint32_t unit)
{
  sBinds.clear(unit);
  if (dtab.TSampledImage(unit).type == VkAnyDescriptorInfo::TYPE_IMG)
  {
    if (applySamplersToCombinedImage(unit))
      tBinds.set(unit);
  }
  applySamplers(unit);
}

void PipelineStageStateBase::applySamplers(uint32_t unit)
{
  const SamplerInfo *samplerInfo = sBinds[unit].si;
  if (samplerInfo)
  {
    dtab.TSampler(unit) += samplerInfo->handle;
    dtab.TSampler(unit).type = VkAnyDescriptorInfo::TYPE_SAMPLER;
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    dtab.TSampler(unit).type = VkAnyDescriptorInfo::TYPE_NULL;
    dtab.TSampler(unit).image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
  }
}

bool PipelineStageStateBase::applySamplersToCombinedImage(uint32_t unit)
{
  const SamplerInfo *samplerInfo = sBinds[unit].si;
  VkSampler oldSmp = dtab.TSampledImage(unit).image.sampler;
  if (samplerInfo)
  {
    dtab.TSampledImage(unit) += samplerInfo->handle;
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    dtab.TSampledImage(unit).image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
  }
  return oldSmp != dtab.TSampledImage(unit).image.sampler;
}

void PipelineStageStateBase::setTinputAttachment(uint32_t unit, Image *image, bool as_const_ds, VulkanImageViewHandle view)
{
  G_UNUSED(image);

  VkAnyDescriptorInfo &regRef = dtab.TInputAttachment(unit);
  if (regRef.image.imageView == view && regRef.type == VkAnyDescriptorInfo::TYPE_IMG)
    return;

  G_ASSERTF(image, "vulkan: obj must be valid!");
  lastDescriptorSet = VulkanNullHandle();

  regRef += VkDescriptorImageInfo{VK_NULL_HANDLE, view, getImgLayout(as_const_ds)};
}

void PipelineStageStateBase::setTtexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds,
  VulkanImageViewHandle view)
{
  G_ASSERTF(image, "vulkan: binding empty tReg as image, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_IMG;
  reg.img.ptr = image;
  reg.img.view = view_state;
  isConstDepthStencil.set(unit, as_const_ds);

  dtab.TBuffer(unit).clear();
  dtab.TSampledBuffer(unit).clear();
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  dtab.TRaytraceAS(unit).clear();
#endif

  VkAnyDescriptorInfo::ReducedImageInfo imgInfo{view, getImgLayout(as_const_ds)};
  dtab.TSampledImage(unit) += imgInfo;
  applySamplersToCombinedImage(unit);
}

void PipelineStageStateBase::setUtexture(uint32_t unit, Image *image, ImageViewState view_state, VulkanImageViewHandle view)
{
  G_ASSERTF(!is_null(view), "vulkan: obj must be valid!");
  URegister &reg = uBinds.set(unit);
  reg.image = image;
  reg.imageView = view_state;
  reg.buffer = BufferRef{};

  dtab.UBuffer(unit).clear();
  dtab.USampledBuffer(unit).clear();
  dtab.UImage(unit) += view;
}

void PipelineStageStateBase::setTbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: obj must be valid!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_BUF;
  reg.buf = buffer;

  dtab.TSampledImage(unit).clear();
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  dtab.TRaytraceAS(unit).clear();
#endif

  G_ASSERTF(buffer, "vulkan: binding empty tReg as buffer, should be filtered at bind point!");
  dtab.TSampledBuffer(unit) += buffer.getView();
  dtab.TBuffer(unit) += VkDescriptorBufferInfo{buffer.getHandle(), buffer.bufOffset(0), buffer.visibleDataSize};
}

void PipelineStageStateBase::setUbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: obj must be valid!");
  URegister &reg = uBinds.set(unit);
  reg.buffer = buffer;
  reg.image = nullptr;
  reg.imageView = ImageViewState();

  dtab.UImage(unit).clear();
  dtab.USampledBuffer(unit) += buffer.getView();
  dtab.UBuffer(unit) += VkDescriptorBufferInfo{buffer.getHandle(), buffer.bufOffset(0), buffer.visibleDataSize};
}

void PipelineStageStateBase::setBbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: obj must be valid!");
  dtab.BConstBuffer(unit) += VkDescriptorBufferInfo{buffer.getHandle(), 0, buffer.visibleDataSize};

  if (bBinds[unit].buffer == buffer.buffer && bBinds[unit].visibleDataSize == buffer.visibleDataSize)
  {
    bBinds[unit] = buffer;
    bOffsetDirtyMask |= (1 << unit);
  }
  else
    bBinds.set(unit) = buffer;
}

void PipelineStageStateBase::setSSampler(uint32_t unit, const SamplerInfo *sampler)
{
  G_ASSERTF(sampler, "vulkan: obj must be valid!");
  sBinds.set(unit).si = sampler;
  if (dtab.TSampledImage(unit).type == VkAnyDescriptorInfo::TYPE_IMG)
  {
    if (applySamplersToCombinedImage(unit))
      tBinds.set(unit);
  }
  applySamplers(unit);
}

#if VULKAN_HAS_RAYTRACING
void PipelineStageStateBase::setTas(uint32_t unit, RaytraceAccelerationStructure *as)
{
  G_ASSERTF(as, "vulkan: binding empty tReg as accel struct, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_AS;
  reg.rtas = as;

  dtab.TSampledImage(unit).clear();
  dtab.TSampledBuffer(unit).clear();
  dtab.TBuffer(unit).clear();

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  dtab.TRaytraceAS(unit) += as->getHandle();
#endif
}
#endif

#if VULKAN_TRACK_DEAD_RESOURCE_USAGE > 0

void PipelineStageStateBase::checkForDeadResources(const spirv::ShaderHeader &hdr)
{
  for (uint32_t i : LsbVisitor{tBinds.validDirtyMask() & hdr.tRegisterUseMask})
  {
    const auto &reg = tBinds[i];
    switch (reg.type)
    {
      case TRegister::TYPE_IMG: reg.img.ptr->checkDead(); break;
      case TRegister::TYPE_BUF: reg.buf.buffer->checkDead(); break;
#if VULKAN_HAS_RAYTRACING
      case TRegister::TYPE_AS: reg.rtas->checkDead(); break;
#endif
      default:;
    }
  }

  for (uint32_t i : LsbVisitor{uBinds.validDirtyMask() & hdr.uRegisterUseMask})
  {
    const auto &reg = uBinds[i];
    if (reg.image)
      reg.image->checkDead();
    else
      reg.buffer.buffer->checkDead();
  }

  for (uint32_t i : LsbVisitor{bBinds.validDirtyMask() & hdr.bRegisterUseMask})
    bBinds[i].buffer->checkDead();
}

#endif // VULKAN_TRACK_DEAD_RESOURCE_USAGE > 0

static void sync_dummy_resource_on_missing_bind(const ResourceDummySet &dummy_resource_table, uint8_t missing_index,
  ExtendedShaderStage stage)
{
  switch (missing_index)
  {
    case spirv::MISSING_SAMPLED_IMAGE_2D_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::T),
        (Image *)dummy_resource_table[missing_index].resource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 12});
      break;

    case spirv::MISSING_SAMPLED_IMAGE_3D_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::T),
        (Image *)dummy_resource_table[missing_index].resource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
      break;

    case spirv::MISSING_STORAGE_IMAGE_2D_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U),
        (Image *)dummy_resource_table[missing_index].resource, VK_IMAGE_LAYOUT_GENERAL, {0, 1, 0, 12});
      break;

    case spirv::MISSING_STORAGE_IMAGE_3D_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U),
        (Image *)dummy_resource_table[missing_index].resource, VK_IMAGE_LAYOUT_GENERAL, {0, 1, 0, 1});
      break;

    case spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX:
    case spirv::MISSING_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummy_resource_table[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::T), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX:
    case spirv::MISSING_STORAGE_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummy_resource_table[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::U), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_CONST_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummy_resource_table[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::B), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_TLAS_INDEX:
    {
#if VULKAN_HAS_RAYTRACING
      RaytraceAccelerationStructure *dummyTLAS = (RaytraceAccelerationStructure *)dummy_resource_table[missing_index].resource;
      if (dummyTLAS)
        Backend::sync.addAccelerationStructureAccess(LogicAddress::forAccelerationStructureOnExecStage(stage, RegisterType::T),
          dummyTLAS);
#endif
    }
    break;
    default: G_ASSERTF(0, "vulkan: unhandled sync for dummy fallback %u", missing_index); break;
  }
}

void PipelineStageStateBase::applyPushConstants(DescriptorSet &registers, VulkanPipelineLayoutHandle layout,
  VkShaderStageFlags shader_stages, ExtendedShaderStage target_stage)
{
  // pure push constants need to be set on layout change or changed values
  // yet emulated must be binded once per change, so a bit different logic used here to fit both variants
  const spirv::ShaderHeader &hdr = registers.header;
  if (hdr.pushConstantsCount)
  {
    const VkShaderStageFlags mirroredExtraStagesMask =
      VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    uint32_t pushConstantSize =
      sizeof(uint32_t) * (shader_stages & mirroredExtraStagesMask ? MAX_IMMEDIATE_CONST_WORDS : hdr.pushConstantsCount);
    if (pushConstantsChanged || is_null(lastDescriptorSet))
    {
      Backend::cb.wCmdPushConstants(layout, shader_stages,
        shader_stages & VK_SHADER_STAGE_FRAGMENT_BIT ? MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t) : 0, pushConstantSize,
        pushConstants);
      pushConstantsChanged = false;
    }
  }
  else
  {
    if (pushConstantsChanged)
    {
      setBbuffer(IMMEDAITE_CB_REGISTER_NO, Backend::immediateConstBuffers[target_stage].push(&pushConstants[0]));
      pushConstantsChanged = false;
    }
  }
}

void PipelineStageStateBase::checkForMissingBinds(const spirv::ShaderHeader &hdr, const ResourceDummySet &dummy_resource_table,
  ExtendedShaderStage stage)
{
  // fill out the missing slots
  for (uint32_t i = 0; i < hdr.registerCount; ++i)
  {
    uint32_t absReg = hdr.registerIndex[i];
    VkAnyDescriptorInfo &slot = dtab.arr[absReg];
    if (slot.type == VkAnyDescriptorInfo::TYPE_NULL)
    {
      // enable this to find who forget to bind resources
      // D3D_ERROR("vulkan: empty binding for used register %u(%u) expected %u restype callsite %s", i, absReg,
      //  hdr.missingTableIndex[i], Backend::State::exec.getExecutionContext().getCurrentCmdCaller());
      switch (hdr.descriptorTypes[i])
      {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
          slot.image.sampler = dummy_resource_table[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
          break;
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
          sync_dummy_resource_on_missing_bind(dummy_resource_table, spirv::MISSING_TLAS_INDEX, stage);
          slot = dummy_resource_table[spirv::MISSING_TLAS_INDEX].descriptor;
        }
        break;
        default:
        {
          G_ASSERTF(hdr.missingTableIndex[i] != spirv::MISSING_IS_FATAL_INDEX,
            "vulkan: resource not bound for register %u(%u) of type %u at callsite %s!", i, absReg, hdr.descriptorTypes[i],
            Backend::State::exec.getExecutionContext().getCurrentCmdCaller());

          sync_dummy_resource_on_missing_bind(dummy_resource_table, hdr.missingTableIndex[i], stage);
          slot = dummy_resource_table[hdr.missingTableIndex[i]].descriptor;
        }
        break;
      }
    }
  }
}
