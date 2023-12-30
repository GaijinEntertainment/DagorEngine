#pragma once
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include "buffer_resource.h"
#include "image_resource.h"
#include "state_field_resource_binds.h"
#include "util/backtrace.h"
#include <math/dag_lsbVisitor.h>
#include "descriptor_table.h"

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
    uint32_t emptyMask = FULL_MASK;

    void markDirty() { dirtyMask = FULL_MASK; }
    uint32_t validMask() { return FULL_MASK & ~emptyMask; }
    uint32_t validDirtyMask() { return dirtyMask & ~emptyMask; }
    void resetDirtyBits(uint32_t bits) { dirtyMask &= ~bits; }

    void clear(uint32_t unit)
    {
      dirtyMask |= (1 << unit);
      emptyMask |= (1 << unit);
      regs[unit].clear();
    }

    Element &set(uint32_t unit)
    {
      dirtyMask |= (1 << unit);
      emptyMask &= ~(1 << unit);
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

  VulkanDescriptorSetHandle lastDescriptorSet;

  PipelineStageStateBase();

  void setUempty(uint32_t unit);
  void setUtexture(uint32_t unit, Image *image, ImageViewState view_state, VulkanImageViewHandle view);
  void setUbuffer(uint32_t unit, BufferRef buffer);

  void setTempty(uint32_t unit);
  void setTtexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds, VulkanImageViewHandle view);
  void setTinputAttachment(uint32_t unit, Image *image, bool as_const_ds, VulkanImageViewHandle view);
  void setTbuffer(uint32_t unit, BufferRef buffer);
#if D3D_HAS_RAY_TRACING
  void setTas(uint32_t unit, RaytraceAccelerationStructure *as);
#endif

  void setBempty(uint32_t unit);
  void setBbuffer(uint32_t unit, BufferRef buffer);

  void setSempty(uint32_t unit);
  void setSSampler(uint32_t unit, const SamplerInfo *sampler);
  void applySamplers(uint32_t unit);

  // set RO DS if matches and drop RO DS if not
  void syncDepthROStateInT(Image *image, uint32_t mip, uint32_t face, bool ro_ds);
  // extensive checks only in dev builds
  // hard conflict = sure buggy binding recived from caller
  // dead resource = resource that was removed but still bound somehow
  // missing bind = shader used some resource at bind X but resource was not provided

  // TODO: promote this method do dev only! (need to invest time and cleanup many places)
  void checkForMissingBinds(const spirv::ShaderHeader &hdr, const ResourceDummySet &dummy_resource_table, ExecutionContext &ctx,
    ShaderStage stage);

#if VULKAN_TRACK_DEAD_RESOURCE_USAGE > 0
  void checkForDeadResources(const spirv::ShaderHeader &hdr);
#else
  void checkForDeadResources(const spirv::ShaderHeader &){};
#endif

  void clearDirty(const spirv::ShaderHeader &hdr)
  {
    tBinds.resetDirtyBits(hdr.tRegisterUseMask);
    sBinds.resetDirtyBits(hdr.tRegisterUseMask);
    uBinds.resetDirtyBits(hdr.uRegisterUseMask);
    bBinds.resetDirtyBits(hdr.bRegisterUseMask);
    bOffsetDirtyMask &= ~hdr.bRegisterUseMask;
  }

  VkImageLayout getImgLayout(bool as_const_ds)
  {
    return as_const_ds ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  void invalidateState();
  template <typename T>
  void apply(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, size_t frame_index, DescriptorSet &registers,
    ExecutionContext &ctx, ShaderStage target_stage, T bind);

  template <typename T>
  void applyNoDiff(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, size_t frame_index, DescriptorSet &registers,
    ExecutionContext &ctx, ShaderStage target_stage, T bind);


  template <typename T>
  void bindDescriptor(const spirv::ShaderHeader &hdr, VulkanDescriptorSetHandle ds_handle, T cb)
  {
    uint32_t dynamicOffsets[spirv::B_REGISTER_INDEX_MAX];
    uint32_t dynamicOffsetCount = 0;
    for (uint32_t i : LsbVisitor{hdr.bRegisterUseMask})
      dynamicOffsets[dynamicOffsetCount++] = bBinds[i].buffer ? bBinds[i].bufOffset(0) : 0;
    cb(ds_handle, dynamicOffsets, dynamicOffsetCount);
  }
};

// this needs to be here, or it will fail to compile because of incomplete types
template <typename T>
void PipelineStageStateBase::apply(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, size_t frame_index,
  DescriptorSet &registers, ExecutionContext &ctx, ShaderStage target_stage, T bind)
{
  const auto &header = registers.header;

  // frequency sorted jump outs
  for (;;)
  {
    if (bBinds.dirtyMask & header.bRegisterUseMask)
      break;
    if (is_null(lastDescriptorSet))
      break;
    if (tBinds.dirtyMask & header.tRegisterUseMask)
      break;
    if (sBinds.dirtyMask & header.tRegisterUseMask)
      break;
    if (uBinds.dirtyMask & header.uRegisterUseMask)
      break;
    if (bOffsetDirtyMask & header.bRegisterUseMask)
    {
      bindDescriptor(header, lastDescriptorSet, bind);
      bOffsetDirtyMask &= ~header.bRegisterUseMask;
    }
    return;
  }
  checkForDeadResources(header);

  if ((header.tRegisterUseMask & tBinds.emptyMask) || (header.uRegisterUseMask & uBinds.emptyMask) ||
      (header.bRegisterUseMask & bBinds.emptyMask))
    checkForMissingBinds(header, dummy_resource_table, ctx, target_stage);

  lastDescriptorSet = registers.getSet(device, frame_index, &dtab.arr[0]);
  clearDirty(header);
  bindDescriptor(header, lastDescriptorSet, bind);
}

template <typename T>
void PipelineStageStateBase::applyNoDiff(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, size_t frame_index,
  DescriptorSet &registers, ExecutionContext &ctx, ShaderStage target_stage, T bind)
{
  const auto &header = registers.header;
  checkForDeadResources(header);

  if ((header.tRegisterUseMask & tBinds.emptyMask) || (header.uRegisterUseMask & uBinds.emptyMask) ||
      (header.bRegisterUseMask & bBinds.emptyMask))
    checkForMissingBinds(header, dummy_resource_table, ctx, target_stage);

  bindDescriptor(header, registers.getSet(device, frame_index, &dtab.arr[0]), bind);
}

} // namespace drv3d_vulkan