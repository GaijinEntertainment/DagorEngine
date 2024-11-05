// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stage_state_base.h"
#if D3D_HAS_RAY_TRACING
#include "raytrace_as_resource.h"
#endif
#include <math/dag_lsbVisitor.h>
#include "globals.h"
#include "dummy_resources.h"
#include "backend.h"
#include "execution_sync.h"

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
    dtab.TSampledCompareImage(j) += targetLayout;

    tBinds.set(j);
  }
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
  dtab.TSampledCompareImage(unit).clear();
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
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
    applySamplers(unit);
}

void PipelineStageStateBase::applySamplers(uint32_t unit)
{
  const SamplerInfo *samplerInfo = sBinds[unit].si;
  if (samplerInfo)
  {
    dtab.TSampledImage(unit) += samplerInfo->colorSampler();
    dtab.TSampledCompareImage(unit) += samplerInfo->compareSampler();
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    dtab.TSampledImage(unit).image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
    dtab.TSampledCompareImage(unit).image.sampler =
      dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX].descriptor.image.sampler;
  }
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
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  dtab.TRaytraceAS(unit).clear();
#endif

  VkAnyDescriptorInfo::ReducedImageInfo imgInfo{view, getImgLayout(as_const_ds)};
  dtab.TSampledImage(unit) += imgInfo;
  dtab.TSampledCompareImage(unit) += imgInfo;
  applySamplers(unit);
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
  dtab.TSampledCompareImage(unit).clear();
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
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
    applySamplers(unit);
}

#if D3D_HAS_RAY_TRACING
void PipelineStageStateBase::setTas(uint32_t unit, RaytraceAccelerationStructure *as)
{
  G_ASSERTF(as, "vulkan: binding empty tReg as accel struct, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_AS;
  reg.rtas = as;

  dtab.TSampledImage(unit).clear();
  dtab.TSampledCompareImage(unit).clear();
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
#if D3D_HAS_RAY_TRACING
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
    default: break;
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
      // hdr.missingTableIndex[i], ctx.getCurrentCmdCaller());

      sync_dummy_resource_on_missing_bind(dummy_resource_table, hdr.missingTableIndex[i], stage);

      slot = dummy_resource_table[hdr.missingTableIndex[i]].descriptor;
    }
  }
}
