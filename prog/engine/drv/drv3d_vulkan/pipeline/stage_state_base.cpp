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
#include "backend/context.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

namespace
{

constexpr uint8_t comboSlotMask(uint8_t engine_type, uint8_t shader_type)
{
  return (engine_type << spirv::ResourceTypeMask::BIT_COUNT) | shader_type;
}

} // namespace

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
  for (uint32_t j = 0; j < spirv::T_REGISTER_INDEX_MAX; ++j)
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
    tBinds.set(j);
  }
}

void PipelineStageStateBase::setImmediateConsts(const uint32_t *data)
{
  memcpy(pushConstants, data, sizeof(uint32_t) * MAX_IMMEDIATE_CONST_WORDS);
  pushConstantsChanged = true;
}

void PipelineStageStateBase::setBempty(uint32_t unit) { bBinds.clear(unit); }

void PipelineStageStateBase::setTempty(uint32_t unit) { tBinds.clear(unit); }

void PipelineStageStateBase::setUempty(uint32_t unit) { uBinds.clear(unit); }

void PipelineStageStateBase::setSempty(uint32_t unit)
{
  sBinds.clear(unit);
  if (tBinds[unit].type == TRegister::TYPE_IMG)
    tBinds.set(unit);
}

void PipelineStageStateBase::applySamplersToCombinedImage(uint32_t unit, VkAnyDescriptorInfo &descriptor)
{
  if (sBinds[unit].si)
    descriptor += sBinds[unit].si->handle;
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    descriptor.image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
  }
}

void PipelineStageStateBase::setTinputAttachment(uint32_t flat_binding_index, Image *image, bool as_const_ds,
  VulkanImageViewHandle view)
{
  G_UNUSED(image);
  G_ASSERTF(image, "vulkan: obj must be valid!");

  VkAnyDescriptorInfo &regRef = dtab.TInputAttachment(flat_binding_index);
  if (regRef.image.imageView == view && regRef.type == VkAnyDescriptorInfo::TYPE_IMG)
    return;

  // caller code should reset state already, but better to be safe here
  lastDescriptorSet = VulkanNullHandle();

  regRef += VkDescriptorImageInfo{VK_NULL_HANDLE, view, getImgLayout(as_const_ds)};
}

void PipelineStageStateBase::setTtexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds)
{
  G_ASSERTF(image, "vulkan: binding empty tReg as image, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_IMG;
  reg.img.ptr = image;
  reg.img.view = view_state;
  isConstDepthStencil.set(unit, as_const_ds);
}

void PipelineStageStateBase::setUtexture(uint32_t unit, Image *image, ImageViewState view_state)
{
  G_ASSERTF(image, "vulkan: obj must be valid!");
  URegister &reg = uBinds.set(unit);
  reg.image = image;
  reg.imageView = view_state;
  reg.buffer = BufferRef{};
}

void PipelineStageStateBase::setTbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: binding empty tReg as buffer, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_BUF;
  reg.buf = buffer;
}

void PipelineStageStateBase::setUbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: obj must be valid!");
  URegister &reg = uBinds.set(unit);
  reg.buffer = buffer;
  reg.image = nullptr;
  reg.imageView = ImageViewState();
}

void PipelineStageStateBase::setBbuffer(uint32_t unit, BufferRef buffer)
{
  G_ASSERTF(buffer, "vulkan: obj must be valid!");
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
  if (tBinds[unit].type == TRegister::TYPE_IMG)
    tBinds.set(unit);
}

#if VULKAN_HAS_RAYTRACING
void PipelineStageStateBase::setTas(uint32_t unit, RaytraceAccelerationStructure *as)
{
  G_ASSERTF(as, "vulkan: binding empty tReg as accel struct, should be filtered at bind point!");
  TRegister &reg = tBinds.set(unit);
  reg.type = TRegister::TYPE_AS;
  reg.rtas = as;
}
#endif

static void sync_dummy_resource_on_missing_bind(uint8_t missing_index, ExtendedShaderStage stage)
{
  const auto &dummyResourceTable = Globals::dummyResources.getTable();
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
        (Image *)dummyResourceTable[missing_index].resource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 12});
      break;

    case spirv::MISSING_SAMPLED_IMAGE_3D_INDEX:
    case spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::T),
        (Image *)dummyResourceTable[missing_index].resource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
      break;

    case spirv::MISSING_STORAGE_IMAGE_2D_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX:
    case spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U),
        (Image *)dummyResourceTable[missing_index].resource, VK_IMAGE_LAYOUT_GENERAL, {0, 1, 0, 12});
      break;

    case spirv::MISSING_STORAGE_IMAGE_3D_INDEX:
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U),
        (Image *)dummyResourceTable[missing_index].resource, VK_IMAGE_LAYOUT_GENERAL, {0, 1, 0, 1});
      break;

    case spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX:
    case spirv::MISSING_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummyResourceTable[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::T), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX:
    case spirv::MISSING_STORAGE_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummyResourceTable[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::U), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_CONST_BUFFER_INDEX:
    {
      Buffer *dummyBuf = (Buffer *)dummyResourceTable[missing_index].resource;
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::B), dummyBuf,
        {dummyBuf->bufOffsetLoc(0), dummyBuf->getBlockSize()});
    }
    break;
    case spirv::MISSING_TLAS_INDEX:
    {
#if VULKAN_HAS_RAYTRACING
      RaytraceAccelerationStructure *dummyTLAS = (RaytraceAccelerationStructure *)dummyResourceTable[missing_index].resource;
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
  VkShaderStageFlags shader_stages, ExtendedShaderStage)
{
  // pure push constants need to be set on layout change or changed values
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
}

PipelineStageStateBase::ApplyStatus PipelineStageStateBase::applyTReg(const TRegister &reg, uint32_t idx,
  const spirv::ShaderHeader &hdr)
{
  uint8_t absReg = hdr.engineSlotToRegister(idx, spirv::EngineSlotMapping::REG_T);
  VkAnyDescriptorInfo &descriptor = dtab.arr[absReg];
  switch (comboSlotMask(reg.type, hdr.resTypeMask.forTSlot(idx)))
  {
    case comboSlotMask(TRegister::TYPE_IMG, spirv::ResourceTypeMask::IMG):
      descriptor +=
        VkAnyDescriptorInfo::ReducedImageInfo{reg.img.ptr->getImageView(reg.img.view), getImgLayout(isConstDepthStencil.test(idx))};
      applySamplersToCombinedImage(idx, descriptor);
      break;
    case comboSlotMask(TRegister::TYPE_BUF, spirv::ResourceTypeMask::BUF):
      descriptor += VkDescriptorBufferInfo{reg.buf.getHandle(), reg.buf.bufOffset(0), reg.buf.visibleDataSize};
      break;
    case comboSlotMask(TRegister::TYPE_BUF, spirv::ResourceTypeMask::BUF_VIEW): descriptor += reg.buf.getView(); break;
    case comboSlotMask(TRegister::TYPE_AS, spirv::ResourceTypeMask::AS):
#if VULKAN_HAS_RAYTRACING
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      descriptor += reg.rtas->getHandle();
#endif
#endif
      break;
    default: descriptor.clear(); break;
  }
  return descriptor.type != VkAnyDescriptorInfo::TYPE_NULL ? ApplyStatus::RESOURCE : ApplyStatus::DUMMY;
}

PipelineStageStateBase::ApplyStatus PipelineStageStateBase::applyUReg(const URegister &reg, uint32_t idx,
  const spirv::ShaderHeader &hdr)
{
  uint8_t absReg = hdr.engineSlotToRegister(idx, spirv::EngineSlotMapping::REG_U);
  VkAnyDescriptorInfo &descriptor = dtab.arr[absReg];

  // branchless version of
  // uint8_t uRegTypeMask = reg.buffer ? URegister::TYPE_BUF : reg.image ? URegister::TYPE_IMG : URegister::TYPE_NULL;
  uint8_t uRegTypeMask = (reg.image != nullptr) | ((reg.buffer.buffer != nullptr) << 1);

  switch (comboSlotMask(uRegTypeMask, hdr.resTypeMask.forUSlot(idx)))
  {
    case comboSlotMask(URegister::TYPE_IMG, spirv::ResourceTypeMask::IMG):
      descriptor += VkAnyDescriptorInfo::ReducedImageInfo{reg.image->getImageView(reg.imageView), VK_IMAGE_LAYOUT_GENERAL};
      break;
    case comboSlotMask(URegister::TYPE_BUF, spirv::ResourceTypeMask::BUF):
      descriptor += VkDescriptorBufferInfo{reg.buffer.getHandle(), reg.buffer.bufOffset(0), reg.buffer.visibleDataSize};
      break;
    case comboSlotMask(URegister::TYPE_BUF, spirv::ResourceTypeMask::BUF_VIEW): descriptor += reg.buffer.getView(); break;
    default: descriptor.clear(); break;
  }
  return descriptor.type != VkAnyDescriptorInfo::TYPE_NULL ? ApplyStatus::RESOURCE : ApplyStatus::DUMMY;
}

PipelineStageStateBase::ApplyStatus PipelineStageStateBase::applyBReg(const BufferRef &reg, uint32_t idx,
  const spirv::ShaderHeader &hdr)
{
  uint8_t absReg = hdr.engineSlotToRegister(idx, spirv::EngineSlotMapping::REG_B);
  VkAnyDescriptorInfo &descriptor = dtab.arr[absReg];

  if (reg.buffer)
    descriptor += VkDescriptorBufferInfo{reg.getHandle(), 0, reg.visibleDataSize};
  else
    descriptor.clear();
  return descriptor.type != VkAnyDescriptorInfo::TYPE_NULL ? ApplyStatus::RESOURCE : ApplyStatus::DUMMY;
}

PipelineStageStateBase::ApplyStatus PipelineStageStateBase::applySReg(const ReducedSRegister &reg, uint32_t idx,
  const spirv::ShaderHeader &hdr)
{
  uint8_t absReg = hdr.engineSlotToRegister(idx, spirv::EngineSlotMapping::REG_S);
  VkAnyDescriptorInfo &descriptor = dtab.arr[absReg];
  if (reg.si)
  {
    descriptor.type = VkAnyDescriptorInfo::TYPE_SAMPLER;
    descriptor += reg.si->handle;
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    descriptor.type = VkAnyDescriptorInfo::TYPE_NULL;
    descriptor.image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
  }
  return descriptor.type != VkAnyDescriptorInfo::TYPE_NULL ? ApplyStatus::RESOURCE : ApplyStatus::DUMMY;
}

void PipelineStageStateBase::syncAccessesForBOffsets(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage)
{
  for (uint32_t i : LsbVisitor{hdr.bRegisterUseMask & bOffsetDirtyMask})
    if (bBinds[i].buffer)
      trackBResAccesses(i, stage);
}
void PipelineStageStateBase::updateDescriptorsNoTrack(const spirv::ShaderHeader &hdr)
{
  for (uint32_t i : LsbVisitor{hdr.tRegisterUseMask & tBinds.dirtyMask})
    applyTReg(tBinds[i], i, hdr);
  for (uint32_t i : LsbVisitor{hdr.uRegisterUseMask & uBinds.dirtyMask})
    applyUReg(uBinds[i], i, hdr);
  for (uint32_t i : LsbVisitor{hdr.bRegisterUseMask & bBinds.dirtyMask})
    applyBReg(bBinds[i], i, hdr);
  for (uint32_t i : LsbVisitor{hdr.sRegisterUseMask & sBinds.dirtyMask})
    applySReg(sBinds[i], i, hdr);
}

void PipelineStageStateBase::updateDescriptors(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage)
{
  for (uint32_t i : LsbVisitor{hdr.tRegisterUseMask & tBinds.dirtyMask})
  {
    if (applyTReg(tBinds[i], i, hdr) == ApplyStatus::RESOURCE)
      trackTResAccesses(i, stage);
    else
      replaceAndTrackFallbackAccess(hdr, hdr.engineSlotToRegister(i, spirv::EngineSlotMapping::REG_T), stage);
  }

  for (uint32_t i : LsbVisitor{hdr.uRegisterUseMask & uBinds.dirtyMask})
  {
    if (applyUReg(uBinds[i], i, hdr) == ApplyStatus::RESOURCE)
      trackUResAccesses(i, stage);
    else
      replaceAndTrackFallbackAccess(hdr, hdr.engineSlotToRegister(i, spirv::EngineSlotMapping::REG_U), stage);
  }

  for (uint32_t i : LsbVisitor{hdr.bRegisterUseMask & bBinds.dirtyMask})
  {
    if (applyBReg(bBinds[i], i, hdr) == ApplyStatus::RESOURCE)
      trackBResAccesses(i, stage);
    else
      replaceAndTrackFallbackAccess(hdr, hdr.engineSlotToRegister(i, spirv::EngineSlotMapping::REG_B), stage);
  }

  for (uint32_t i : LsbVisitor{hdr.sRegisterUseMask & sBinds.dirtyMask})
  {
    if (applySReg(sBinds[i], i, hdr) == ApplyStatus::DUMMY)
      replaceAndTrackFallbackAccess(hdr, hdr.engineSlotToRegister(i, spirv::EngineSlotMapping::REG_S), stage);
  }
}

void PipelineStageStateBase::syncAccessesNoDiff(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage)
{
  for (uint32_t i = 0; i < hdr.registerCount; ++i)
  {
    if (dtab.arr[i].type != VkAnyDescriptorInfo::TYPE_NULL)
      trackResAccess(hdr.registerToSlotMapping[i], stage);
    else
      replaceAndTrackFallbackAccess(hdr, i, stage);
  }
}

void PipelineStageStateBase::replaceAndTrackFallbackAccess(const spirv::ShaderHeader &hdr, uint8_t shader_reg,
  ExtendedShaderStage stage)
{
  // if there was no resource fallback to dummy and track sync for it too

  // enable this to find who forget to bind resources
  // D3D_ERROR("vulkan: empty binding for used register %u expected %u restype callsite %s", shader_reg,
  //  hdr.missingTableIndex[shader_reg], Backend::ctx.getCurrentCmdCaller());

  const auto &dummyResourceTable = Globals::dummyResources.getTable();
  VkAnyDescriptorInfo &descriptor = dtab.arr[shader_reg];
  switch (hdr.descriptorTypes[shader_reg].value)
  {
    // input attachments handled as part of render pass logic
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: break;
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      descriptor.image.sampler = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler;
      break;
    case VK_COMPRESSED_DESCRIPTOR_TYPE_AS:
    {
      sync_dummy_resource_on_missing_bind(spirv::MISSING_TLAS_INDEX, stage);
      descriptor = dummyResourceTable[spirv::MISSING_TLAS_INDEX].descriptor;
    }
    break;
    default:
    {
      G_ASSERTF(hdr.missingTableIndex[shader_reg] != spirv::MISSING_IS_FATAL_INDEX,
        "vulkan: resource not bound for register %u of type %u at callsite %s!", shader_reg, hdr.descriptorTypes[shader_reg].get(),
        Backend::ctx.getCurrentCmdCaller());

      sync_dummy_resource_on_missing_bind(hdr.missingTableIndex[shader_reg], stage);
      descriptor = dummyResourceTable[hdr.missingTableIndex[shader_reg]].descriptor;
    }
    break;
  }
}

void PipelineStageStateBase::trackResAccess(spirv::EngineSlotMapping mapped_slot, ExtendedShaderStage stage)
{
  switch (mapped_slot.type)
  {
    case spirv::EngineSlotMapping::REG_T: trackTResAccesses(mapped_slot.slot, stage); break;
    case spirv::EngineSlotMapping::REG_U: trackUResAccesses(mapped_slot.slot, stage); break;
    case spirv::EngineSlotMapping::REG_B: trackBResAccesses(mapped_slot.slot, stage); break;
    case spirv::EngineSlotMapping::REG_S: break;
    default: G_ASSERTF(0, "vulkan: unknown regMap.type = %u", mapped_slot.type); break;
  }
}

void PipelineStageStateBase::trackTResAccesses(uint32_t slot, ExtendedShaderStage stage)
{
  const TRegister &reg = tBinds[slot];
  switch (reg.type)
  {
    case TRegister::TYPE_IMG:
    {
      bool roDepth = isConstDepthStencil.test(slot);
      VkImageLayout srvLayout = roDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::T), reg.img.ptr, srvLayout,
        {reg.img.view.getMipBase(), reg.img.view.getMipCount(), reg.img.view.getArrayBase(), reg.img.view.getArrayCount()});
    }
    break;
    case TRegister::TYPE_BUF:
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::T), reg.buf.buffer,
        {reg.buf.bufOffset(0), reg.buf.visibleDataSize});
      break;
#if VULKAN_HAS_RAYTRACING
    case TRegister::TYPE_AS:
      Backend::sync.addAccelerationStructureAccess(LogicAddress::forAccelerationStructureOnExecStage(stage, RegisterType::T),
        reg.rtas);
      break;
#endif
    default: break;
  }
}

void PipelineStageStateBase::trackUResAccesses(uint32_t slot, ExtendedShaderStage stage)
{
  const URegister &reg = uBinds[slot];
  if (reg.image)
  {
    Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U), reg.image, VK_IMAGE_LAYOUT_GENERAL,
      {reg.imageView.getMipBase(), reg.imageView.getMipCount(), reg.imageView.getArrayBase(), reg.imageView.getArrayCount()});
  }
  else
  {
    Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::U), reg.buffer.buffer,
      {reg.buffer.bufOffset(0), reg.buffer.visibleDataSize});
  }
}

void PipelineStageStateBase::trackBResAccesses(uint32_t slot, ExtendedShaderStage stage)
{
  const BufferRef &reg = bBinds[slot];
  if (slot == GCBSlot && bGCBActive)
    return;
  Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::B), reg.buffer,
    {reg.bufOffset(0), reg.visibleDataSize});
}

bool PipelineStageStateBase::updateOnDemand(DescriptorSet &registers, ExtendedShaderStage stage, size_t frame_index)
{
  const spirv::ShaderHeader &hdr = registers.header;
  // frequency sorted jump outs
  for (;;)
  {
    if (bBinds.dirtyMask & hdr.bRegisterUseMask)
      break;
    if (is_null(lastDescriptorSet))
      break;
    if (tBinds.dirtyMask & hdr.tRegisterUseMask)
      break;
    if (sBinds.dirtyMask & hdr.sRegisterUseMask)
      break;
    if (uBinds.dirtyMask & hdr.uRegisterUseMask)
      break;
    if (bOffsetDirtyMask & hdr.bRegisterUseMask)
    {
      syncAccessesForBOffsets(hdr, stage);
      bOffsetDirtyMask &= ~hdr.bRegisterUseMask;
      return true;
    }
    return false;
  }

  updateDescriptors(hdr, stage);

  lastDescriptorSet = registers.getSet(frame_index, &dtab.arr[0]);
  clearDirty(hdr);
  return true;
}

bool PipelineStageStateBase::updateOnDemandWithSyncStep(DescriptorSet &registers, ExtendedShaderStage stage, size_t frame_index)
{
  const spirv::ShaderHeader &hdr = registers.header;
  // frequency sorted jump outs
  for (;;)
  {
    if (bBinds.dirtyMask & hdr.bRegisterUseMask)
      break;
    if (is_null(lastDescriptorSet))
      break;
    if (tBinds.dirtyMask & hdr.tRegisterUseMask)
      break;
    if (sBinds.dirtyMask & hdr.sRegisterUseMask)
      break;
    if (uBinds.dirtyMask & hdr.uRegisterUseMask)
      break;
    if (bOffsetDirtyMask & hdr.bRegisterUseMask)
    {
      syncAccessesNoDiff(hdr, stage);
      bOffsetDirtyMask &= ~hdr.bRegisterUseMask;
      return true;
    }
    return false;
  }

  updateDescriptorsNoTrack(hdr);
  syncAccessesNoDiff(hdr, stage);

  lastDescriptorSet = registers.getSet(frame_index, &dtab.arr[0]);
  clearDirty(hdr);
  return true;
}
