// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_device.h"

#include <drv/3d/dag_d3dResource.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/array.h>
#include <value_range.h>
#include "bindless_common.h"
#include "image_view_state.h"
#include "buffer_ref.h"

namespace drv3d_vulkan
{

struct BaseTex;
class ExecutionContext;
class SamplerResource;
class Buffer;

class BindlessManager
{
public:
  void init();

  uint32_t allocateBindlessResourceRange(uint32_t resourceType, uint32_t count);
  uint32_t resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount);
  void freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count);
  uint32_t registerBindlessSampler(BaseTex *texture);
  uint32_t registerBindlessSampler(SamplerResource *sampler);

private:
  uint32_t registerBindlessSampler(SamplerState sampler);
  uint32_t resTypeToSlotIdx(uint32_t resourceType) { return RES3D_SBUF != resourceType ? SLOTS_TEX : SLOTS_BUF; }


  enum
  {
    SLOTS_TEX = 0,
    SLOTS_BUF = 1,
    SLOTS_COUNT = 2
  };

  uint32_t size[SLOTS_COUNT] = {0, 0};
  eastl::vector<ValueRange<uint32_t>> freeSlotRanges[SLOTS_COUNT];
  eastl::vector<SamplerState> samplerTable;
};

class BindlessManagerBackend // -V730
{
private:
  static constexpr uint32_t BUFFERED_SET_COUNT = GPU_TIMELINE_HISTORY_SIZE;

public:
  void init(const VulkanDevice &vulkanDevice, BindlessSetLimits &limits);
  void shutdown(const VulkanDevice &vulkanDevice);

  void cleanupBindlessTexture(uint32_t index, Image *image);
  void updateBindlessTexture(uint32_t index, Image *image, const ImageViewState view);

  void cleanupBindlessBuffer(uint32_t index, Buffer *buf);
  void updateBindlessBuffer(uint32_t index, const BufferRef &buf);

  void updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo);
  void copyBindlessDescriptors(uint32_t resource_type, uint32_t src, uint32_t dst, uint32_t count);

  void fillSetLayouts(BindlessSetLayouts &tgt) const { tgt = layouts; }

  uint32_t getActiveBindlessSetCount() const;

  void advance();
  void bindSets(VkPipelineBindPoint bindPoint, VulkanPipelineLayoutHandle pipelineLayout);

private:
  void createBindlessLayout(const VulkanDevice &device, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VulkanDescriptorSetLayoutHandle &descriptorLayout);
  void allocateBindlessSet(const VulkanDevice &device, uint32_t descriptorCount, VulkanDescriptorSetLayoutHandle descriptorLayout,
    VulkanDescriptorSetHandle &descriptorSet);
  void copyDescriptors(uint32_t set_idx, uint32_t ring_src, uint32_t ring_dst, uint32_t src, uint32_t dst, uint32_t count);
  uint32_t resTypeToSetIdx(uint32_t res_type)
  {
    return RES3D_SBUF != res_type ? spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX
                                  : spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX;
  }

  VkWriteDescriptorSet initSetWrite(uint32_t set_idx, uint32_t index);
  void resetSets();
  void updateFlatSets();

  void markDirtyRange(uint32_t set_idx, uint32_t start, uint32_t size = 1)
  {
    auto &dirtyRange = bufferedSets[actualSetId][set_idx].dirtyRange;
    dirtyRange.start = eastl::min(dirtyRange.start, start);
    dirtyRange.stop = eastl::max(dirtyRange.stop, start + size);
  }

  VulkanDescriptorPoolHandle descriptorPool;
  BindlessSetLayouts layouts;

  struct BindlessSetWithDirty
  {
    VulkanDescriptorSetHandle set;
    ValueRange<uint32_t> dirtyRange;
  };

  uint32_t setLimits[spirv::bindless::MAX_SETS];
  eastl::array<BindlessSetWithDirty, spirv::bindless::MAX_SETS> bufferedSets[BUFFERED_SET_COUNT];

  uint32_t actualSetId = 0;

  eastl::hash_map<uint32_t, Image *> imageSlots;
  eastl::hash_map<uint32_t, Buffer *> bufferSlots;
  bool enabled = false;
};

} // namespace drv3d_vulkan
