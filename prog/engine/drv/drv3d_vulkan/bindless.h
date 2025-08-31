// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_device.h"

#include <drv/3d/dag_d3dResource.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <EASTL/hash_map.h>
#include <EASTL/array.h>
#include <value_range.h>
#include <osApiWrappers/dag_critSec.h>
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
  void afterBackendFlush();
  void onDeviceReset();
  void afterDeviceReset();

  uint32_t allocateBindlessResourceRange(D3DResourceType type, uint32_t count);
  uint32_t resizeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t currentCount, uint32_t newCount);
  void queueFreeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count);
  uint32_t registerBindlessSampler(const SamplerResource *sampler);

private:
  // unsafe to call out of front lock
  void freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count);
  uint32_t resTypeToSlotIdx(D3DResourceType type) { return D3DResourceType::SBUF != type ? SLOTS_TEX : SLOTS_BUF; }

  enum
  {
    SLOTS_TEX = 0,
    SLOTS_BUF = 1,
    SLOTS_COUNT = 2
  };

  // d3d:: calls that allocate/free bindless ranges don't have external sync requirement, so do sync internally
  uint32_t size[SLOTS_COUNT] = {0, 0};
  WinCritSec rangeMutexes[SLOTS_COUNT];
  dag::Vector<ValueRange<uint32_t>> freeSlotRanges[SLOTS_COUNT];
  dag::Vector<ValueRange<uint32_t>> pendingCleanups[SLOTS_COUNT];

  WinCritSec samplerTableMutex;
  dag::Vector<const SamplerResource *> samplerTable;
};

struct BindlessImageSlot
{
  Image *img;
  ImageViewState viewState;
  bool stub;
};

class BindlessManagerBackend // -V730
{
private:
  static constexpr uint32_t BUFFERED_SET_COUNT = GPU_TIMELINE_HISTORY_SIZE;

public:
  void init(BindlessSetLimits &limits);
  void shutdown();
  void destroyVkObjects();

  void restoreBindlessTexture(uint32_t index, Image *image);
  void evictBindlessTexture(uint32_t index, Image *image);
  void cleanupBindlessTexture(uint32_t index, Image *image);
  // true - slot was updated
  bool updateBindlessTexture(uint32_t index, Image *image, const ImageViewState view, bool stub, bool stub_swap);
  void setBindlessTexture(uint32_t index, Image *image, const ImageViewState view, bool stub);

  void restoreBindlessBuffer(uint32_t index, Buffer *buf);
  void evictBindlessBuffer(uint32_t index, Buffer *buf);
  void cleanupBindlessBuffer(uint32_t index, Buffer *buf);
  void updateBindlessBuffer(uint32_t index, const BufferRef &buf);

  void updateBindlessSampler(uint32_t index, const SamplerInfo *samplerInfo);
  void copyBindlessDescriptors(D3DResourceType type, uint32_t src, uint32_t dst, uint32_t count);

  void fillSetLayouts(BindlessSetLayouts &tgt) const { tgt = layouts; }

  uint32_t getActiveBindlessSetCount() const;

  void advance();
  void bindSets(VkPipelineBindPoint bindPoint, VulkanPipelineLayoutHandle pipelineLayout);
  void resetSets();

private:
  void createBindlessLayout(VkDescriptorType descriptorType, uint32_t descriptorCount,
    VulkanDescriptorSetLayoutHandle &descriptorLayout);
  void allocateBindlessSet(uint32_t descriptorCount, VulkanDescriptorSetLayoutHandle descriptorLayout,
    VulkanDescriptorSetHandle &descriptorSet);
  void copyDescriptors(uint32_t set_idx, uint32_t ring_src, uint32_t ring_dst, uint32_t src, uint32_t dst, uint32_t count);
  uint32_t resTypeToSetIdx(D3DResourceType res_type)
  {
    return D3DResourceType::SBUF != res_type ? spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX
                                             : spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX;
  }

  VkWriteDescriptorSet initSetWrite(uint32_t set_idx, uint32_t index);
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

  eastl::hash_map<uint32_t, BindlessImageSlot> imageSlots;
  eastl::hash_map<uint32_t, Buffer *> bufferSlots;
  bool enabled = false;
};

} // namespace drv3d_vulkan
