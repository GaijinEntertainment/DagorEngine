// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <math/dag_lsbVisitor.h>

#include "descriptor_set.h"
#include "render_pass.h"
#include "buffer_resource.h"
#include "image_resource.h"
#include "state_field_resource_binds.h"
#include "util/backtrace.h"
#include "descriptor_table.h"
#include "dummy_resources.h"

namespace drv3d_vulkan
{

struct ReducedSRegister
{
  const SamplerInfo *si;
  void clear() { si = nullptr; }
};

struct PipelineStageStateBase
{
  template <typename Element, size_t Count>
  struct TrackedBindArray
  {
    Element regs[Count] = {};
    static_assert(Count <= 32);
    static constexpr uint32_t FULL_MASK = (uint32_t)((1ull << Count) - 1);
    uint32_t dirtyMask = FULL_MASK;

    void markDirty() { dirtyMask = FULL_MASK; }
    void resetDirtyBits(uint32_t bits) { dirtyMask &= ~bits; }

    void clear(uint32_t unit)
    {
      dirtyMask |= (1 << unit);
      regs[unit].clear();
    }

    Element &set(uint32_t unit)
    {
      dirtyMask |= (1 << unit);
      return regs[unit];
    }

    Element &operator[](uint32_t unit) { return regs[unit]; }
  };

  TrackedBindArray<BufferRef, spirv::B_REGISTER_INDEX_MAX> bBinds;
  TrackedBindArray<TRegister, spirv::T_REGISTER_INDEX_MAX> tBinds;
  TrackedBindArray<URegister, spirv::U_REGISTER_INDEX_MAX> uBinds;
  TrackedBindArray<ReducedSRegister, spirv::S_REGISTER_INDEX_MAX> sBinds;
  eastl::bitset<spirv::T_REGISTER_INDEX_MAX> isConstDepthStencil;
  uint32_t bOffsetDirtyMask = 0;

  // empty b0 bind is replaced with global const buffer;
  static constexpr uint32_t GCBSlot = 0; // will not work properly if we want non 0 slot!
  bool bGCBActive = true;

  DescriptorTable dtab;

  // resetted on pipeline layout changes
  VulkanDescriptorSetHandle lastDescriptorSet;

  bool pushConstantsChanged = false;
  uint32_t pushConstants[MAX_IMMEDIATE_CONST_WORDS] = {};

  PipelineStageStateBase();

  void setImmediateConsts(const uint32_t *data);

  void setUempty(uint32_t unit);
  void setUtexture(uint32_t unit, Image *image, ImageViewState view_state);
  void setUbuffer(uint32_t unit, BufferRef buffer);

  void setTempty(uint32_t unit);
  void setTtexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds);
  void setTinputAttachment(uint32_t flat_binding_index, Image *image, bool as_const_ds, VulkanImageViewHandle view);
  void setTbuffer(uint32_t unit, BufferRef buffer);
#if VULKAN_HAS_RAYTRACING
  void setTas(uint32_t unit, RaytraceAccelerationStructure *as);
#endif

  void setBempty(uint32_t unit);
  void setBbuffer(uint32_t unit, BufferRef buffer);

  void setSempty(uint32_t unit);
  void setSSampler(uint32_t unit, const SamplerInfo *sampler);
  void applySamplersToCombinedImage(uint32_t unit, VkAnyDescriptorInfo &descriptor);

  // set RO DS if matches and drop RO DS if not
  void syncDepthROStateInT(Image *image, uint32_t mip, uint32_t face, bool ro_ds);
  void syncAccessesForBOffsets(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage);
  void syncAccessesNoDiff(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage);

  void replaceAndTrackFallbackAccess(const spirv::ShaderHeader &hdr, uint8_t shader_reg, ExtendedShaderStage stage);
  void trackResAccess(spirv::EngineSlotMapping mapped_slot, ExtendedShaderStage stage);

  void trackTResAccesses(uint32_t slot, ExtendedShaderStage stage);
  void trackUResAccesses(uint32_t slot, ExtendedShaderStage stage);
  void trackBResAccesses(uint32_t slot, ExtendedShaderStage stage);

  void clearDirty(const spirv::ShaderHeader &hdr)
  {
    tBinds.resetDirtyBits(hdr.tRegisterUseMask);
    sBinds.resetDirtyBits(hdr.sRegisterUseMask);
    uBinds.resetDirtyBits(hdr.uRegisterUseMask);
    bBinds.resetDirtyBits(hdr.bRegisterUseMask);
    bOffsetDirtyMask &= ~hdr.bRegisterUseMask;
    pushConstantsChanged = false;
  }

  VkImageLayout getImgLayout(bool as_const_ds)
  {
    return as_const_ds ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  void invalidateState();
  template <typename T>
  void setResources(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage, T bind);

  template <typename T>
  void setResourcesWithSyncStep(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage, T bind);

  template <typename T>
  void setResourcesNoDiff(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage, T bind);


  bool updateOnDemand(DescriptorSet &registers, ExtendedShaderStage stage, size_t frame_index);
  bool updateOnDemandWithSyncStep(DescriptorSet &registers, ExtendedShaderStage stage, size_t frame_index);


  template <typename T>
  void bindDescriptor(const spirv::ShaderHeader &hdr, VulkanDescriptorSetHandle ds_handle, T cb)
  {
    if (ds_handle == DescriptorSet::dummyHandle)
      return;
    uint32_t dynamicOffsets[spirv::B_REGISTER_INDEX_MAX];
    uint32_t dynamicOffsetCount = 0;
    for (uint32_t i : LsbVisitor{hdr.bRegisterUseMask})
      dynamicOffsets[dynamicOffsetCount++] = bBinds[i].buffer ? bBinds[i].bufOffset(0) : 0;
    cb(ds_handle, dynamicOffsets, dynamicOffsetCount);
  }

  void applyPushConstants(DescriptorSet &registers, VulkanPipelineLayoutHandle layout, VkShaderStageFlags shader_stages,
    ExtendedShaderStage target_stage);

  void updateDescriptors(const spirv::ShaderHeader &hdr, ExtendedShaderStage stage);
  void updateDescriptorsNoTrack(const spirv::ShaderHeader &hdr);

  enum ApplyStatus
  {
    RESOURCE,
    DUMMY
  };


  ApplyStatus applyTReg(const TRegister &reg, uint32_t idx, const spirv::ShaderHeader &hdr);
  ApplyStatus applyUReg(const URegister &reg, uint32_t idx, const spirv::ShaderHeader &hdr);
  ApplyStatus applyBReg(const BufferRef &reg, uint32_t idx, const spirv::ShaderHeader &hdr);
  ApplyStatus applySReg(const ReducedSRegister &reg, uint32_t idx, const spirv::ShaderHeader &hdr);
};

// this needs to be here, or it will fail to compile because of incomplete types
template <typename T>
void PipelineStageStateBase::setResources(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage, T bind)
{
  if (updateOnDemand(registers, target_stage, frame_index))
    bindDescriptor(registers.header, lastDescriptorSet, bind);
}

template <typename T>
void PipelineStageStateBase::setResourcesWithSyncStep(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage,
  T bind)
{
  if (updateOnDemandWithSyncStep(registers, target_stage, frame_index))
    bindDescriptor(registers.header, lastDescriptorSet, bind);
}

template <typename T>
void PipelineStageStateBase::setResourcesNoDiff(size_t frame_index, DescriptorSet &registers, ExtendedShaderStage target_stage, T bind)
{
  // assume that state is invalidated before call and check it lightly
  G_ASSERT(is_null(lastDescriptorSet));
  G_ASSERT(tBinds.dirtyMask == tBinds.FULL_MASK);
  const auto &header = registers.header;
  updateDescriptors(header, target_stage);
  bindDescriptor(header, registers.getSet(frame_index, &dtab.arr[0]), bind);
}

} // namespace drv3d_vulkan