#pragma once
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "descriptor_set.h"
#include "render_pass.h"
#include "buffer_resource.h"
#include "image_resource.h"
#include "state_field_resource_binds.h"
#include "util/backtrace.h"

namespace drv3d_vulkan
{

struct PipelineStageStateBase
{
  BufferRef bRegisters[spirv::B_REGISTER_INDEX_MAX] = {};
  TRegister tRegisters[spirv::T_REGISTER_INDEX_MAX] = {};
  URegister uRegisters[spirv::U_REGISTER_INDEX_MAX] = {};
  uint32_t bRegisterValidMask = 0;
  uint32_t tRegisterValidMask = 0;
  uint32_t uRegisterValidMask = 0;
  eastl::bitset<spirv::T_REGISTER_INDEX_MAX> isConstDepthStencil;
  VulkanBufferHandle constRegisterLastBuffer;
  uint32_t constRegisterLastBufferOffset = 0;
  uint32_t constRegisterLastBufferRange = 0;
  VulkanDescriptorSetHandle lastDescriptorSet;

  VkAnyDescriptorInfo registerTable[spirv::TOTAL_REGISTER_COUNT] = {};

  static constexpr VkDescriptorImageInfo emptyImageDescriptorInfo()
  {
    return {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  }

  inline VkAnyDescriptorInfo &getConstBufferRegister(uint32_t index) { return registerTable[index + spirv::B_CONST_BUFFER_OFFSET]; }
  inline VkAnyDescriptorInfo &getSampledImageRegister(uint32_t index) { return registerTable[index + spirv::T_SAMPLED_IMAGE_OFFSET]; }
  inline VkAnyDescriptorInfo &getInputAttachmentRegister(uint32_t index)
  {
    return registerTable[index + spirv::T_INPUT_ATTACHMENT_OFFSET];
  }
  inline VkAnyDescriptorInfo &getSampledImageWithCompareRegister(uint32_t index)
  {
    return registerTable[index + spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET];
  }
  inline VkAnyDescriptorInfo &getBufferAsSampledImageRegister(uint32_t index)
  {
    return registerTable[index + spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET];
  }
  inline VkAnyDescriptorInfo &getReadOnlyBufferRegister(uint32_t index) { return registerTable[index + spirv::T_BUFFER_OFFSET]; }
  inline VkAnyDescriptorInfo &getStorageImageRegister(uint32_t index) { return registerTable[index + spirv::U_IMAGE_OFFSET]; }
  inline VkAnyDescriptorInfo &getStorageBufferAsImageRegister(uint32_t index)
  {
    return registerTable[index + spirv::U_BUFFER_IMAGE_OFFSET];
  }
  inline VkAnyDescriptorInfo &getStorageBufferRegister(uint32_t index) { return registerTable[index + spirv::U_BUFFER_OFFSET]; }
  inline VkAnyDescriptorInfo &getStorageBufferWithCounterRegister(uint32_t index)
  {
    return registerTable[index + spirv::U_BUFFER_WITH_COUNTER_OFFSET];
  }
  inline VkAnyDescriptorInfo &getRaytraceAccelerationStructureRegister(uint32_t index)
  {
    return registerTable[index + spirv::T_ACCELERATION_STRUCTURE_OFFSET];
  }

  PipelineStageStateBase();

  void setUempty(uint32_t unit);
  void setUtexture(uint32_t unit, Image *image, ImageViewState view_state, VulkanImageViewHandle view);
  void setUbuffer(uint32_t unit, BufferRef buffer);

  void setTempty(uint32_t unit);
  void setTtexture(uint32_t unit, Image *image, ImageViewState view_state, SamplerInfo *sampler, bool as_const_ds,
    VulkanImageViewHandle view);
  void setTinputAttachment(uint32_t unit, Image *image, bool as_const_ds, VulkanImageViewHandle view);
  void setTbuffer(uint32_t unit, BufferRef buffer);
#if D3D_HAS_RAY_TRACING
  void setTas(uint32_t unit, RaytraceAccelerationStructure *as);
#endif

  void setBempty(uint32_t unit);
  void setBbuffer(uint32_t unit, BufferRef buffer);

  // set RO DS if matches and drop RO DS if not
  void syncDepthROStateInT(Image *image, uint32_t mip, uint32_t face, bool ro_ds);
  void validateBinds(const spirv::ShaderHeader &hdr, bool t_diff, bool u_diff, bool b_diff);
  // extensive checks only in dev builds
  // hard conflict = sure buggy binding recived from caller
  // dead resource = resource that was removed but still bound somehow
  // missing bind = shader used some resource at bind X but resource was not provided

  // TODO: promote this method do dev only! (need to invest time and cleanup many places)
  void checkForMissingBinds(const spirv::ShaderHeader &hdr, const ResourceDummySet &dummy_resource_table, ExecutionContext &ctx);
#if DAGOR_DBGLEVEL > 0
  void checkForHardConflicts(const spirv::ShaderHeader &hdr);
  void checkForDeadResources(const spirv::ShaderHeader &hdr, bool t_diff, bool u_diff, bool b_diff);
#else
  void checkForHardConflicts(const spirv::ShaderHeader &){};
  void checkForDeadResources(const spirv::ShaderHeader &, bool, bool, bool){};
#endif

  void invalidateState();
  template <typename T>
  void apply(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, uint32_t frame_index, DescriptorSet &registers,
    ExecutionContext &ctx, T bind);
};

// this needs to be here, or it will fail to compile because of incomplete types
template <typename T>
void PipelineStageStateBase::apply(VulkanDevice &device, const ResourceDummySet &dummy_resource_table, uint32_t frame_index,
  DescriptorSet &registers, ExecutionContext &ctx, T bind)
{
  const auto &header = registers.header;

  bool tDiff = (tRegisterValidMask & header.tRegisterUseMask) != header.tRegisterUseMask;
  bool uDiff = (uRegisterValidMask & header.uRegisterUseMask) != header.uRegisterUseMask;
  bool bDiff = (bRegisterValidMask & header.bRegisterUseMask) != header.bRegisterUseMask;
  if (tDiff || uDiff || bDiff || is_null(lastDescriptorSet))
  {
    validateBinds(header, tDiff, uDiff, bDiff);

    tRegisterValidMask = header.tRegisterUseMask;
    uRegisterValidMask = header.uRegisterUseMask;
    bRegisterValidMask = header.bRegisterUseMask;

    // empty b0 bind is replaced with global const buffer;
    // GCB == global const buffer
    const uint32_t GCBSlot = 0; // will not work properly if we want non 0 slot!
    bool useGCB = (header.bRegisterUseMask & (1 << GCBSlot)) && (bRegisters[GCBSlot].buffer == nullptr);
    uint32_t GCBshift = useGCB ? GCBSlot + 1 : GCBSlot;
    uint32_t dynamicOffsets[spirv::B_REGISTER_INDEX_MAX];
    uint32_t dynamicOffsetCount = GCBshift;

    if (useGCB)
    {
      VkAnyDescriptorInfo &GCBReg = getConstBufferRegister(GCBSlot);
      dynamicOffsets[GCBSlot] = constRegisterLastBufferOffset;
      GCBReg.buffer.buffer = constRegisterLastBuffer;
      GCBReg.buffer.range = constRegisterLastBufferRange;
      GCBReg.type = VkAnyDescriptorInfo::TYPE_BUF;
    }

    for (uint32_t i = GCBshift; i < spirv::B_REGISTER_INDEX_MAX; ++i)
      if (header.bRegisterUseMask & (1ul << i))
        dynamicOffsets[dynamicOffsetCount++] = bRegisters[i] ? bRegisters[i].dataOffset(0) : 0;

    checkForMissingBinds(header, dummy_resource_table, ctx);

    lastDescriptorSet = registers.getSet(device, frame_index, registerTable);
    bind(lastDescriptorSet, dynamicOffsets, dynamicOffsetCount);
  }
}

} // namespace drv3d_vulkan