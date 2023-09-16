#pragma once

#include "util/value_range.h"
#include "vulkan_device.h"

#include <3d/dag_d3dResource.h>
#include <spirv/compiled_meta_data.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/array.h>

namespace drv3d_vulkan
{

struct BaseTex;
class ExecutionContext;

class BindlessManager
{
public:
  uint32_t allocateBindlessResourceRange(uint32_t resourceType, uint32_t count);
  uint32_t resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount);
  void freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count);
  uint32_t registerBindlessSampler(BaseTex *texture);

private:
  uint32_t size = 0;
  eastl::vector<ValueRange<uint32_t>> freeSlotRanges;
  eastl::vector<SamplerState> samplerTable;
};


class BindlessManagerBackend
{
private:
  static constexpr uint32_t BUFFERED_SET_COUNT = FRAME_FRAME_BACKLOG_LENGTH;

public:
  void init(const VulkanDevice &vulkanDevice, uint32_t _maxBindlessTextureCount, uint32_t _maxBindlessSamplerCount);
  void shutdown(const VulkanDevice &vulkanDevice);

  void cleanupBindlessTexture(uint32_t index, Image *image);
  void updateBindlessTexture(uint32_t index, Image *image, const ImageViewState view);
  void updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo);
  void copyBindlessTextureDescriptors(uint32_t src, uint32_t dst, uint32_t count);

  const VulkanDescriptorSetHandle &getTextureDescriptorSet() const { return textureDescriptorSets[actualSetId].set; }
  const VulkanDescriptorSetLayoutHandle &getTextureDescriptorLayout() const { return textureDescriptorLayout; }

  const VulkanDescriptorSetHandle &getSamplerDescriptorSet() const { return samplerDescriptorSet; }
  const VulkanDescriptorSetLayoutHandle &getSamplerDescriptorLayout() const { return samplerDescriptorLayout; }

  uint32_t getActiveBindlessSetCount() const;

  void advance();
  void bindSets(ExecutionContext &target, VkPipelineBindPoint bindPoint, VulkanPipelineLayoutHandle pipelineLayout);

private:
  void createBindlessLayout(const VulkanDevice &device, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VulkanDescriptorSetLayoutHandle &descriptorLayout);
  void allocateBindlessSet(const VulkanDevice &device, uint32_t descriptorCount, VulkanDescriptorSetLayoutHandle descriptorLayout,
    VulkanDescriptorSetHandle &descriptorSet);

  void resetTextureSets();

  void markDirtyRange(uint32_t start, uint32_t size = 1)
  {
    auto &dirtyRange = textureDescriptorSets[actualSetId].dirtyRange;
    dirtyRange.start = eastl::min(dirtyRange.start, start);
    dirtyRange.stop = eastl::max(dirtyRange.stop, start + size);
  }

  VulkanDescriptorPoolHandle descriptorPool;

  struct TextureSetWithDirty
  {
    VulkanDescriptorSetHandle set;
    ValueRange<uint32_t> dirtyRange;
  };

  eastl::array<TextureSetWithDirty, BUFFERED_SET_COUNT> textureDescriptorSets;

  VulkanDescriptorSetLayoutHandle textureDescriptorLayout;

  // NOTE: buffering of sampler descriptor heap is not needed since the frontend register
  // sampler logic prevents to update any existing descriptor. However manually calling
  // updateBindlessSampler inside the driver on an existing index could cause problem
  VulkanDescriptorSetHandle samplerDescriptorSet;
  VulkanDescriptorSetLayoutHandle samplerDescriptorLayout;

  VulkanDescriptorSetHandle sets[spirv::bindless::MAX_SETS];

  uint32_t actualSetId = 0;

  uint32_t maxBindlessTextureCount = 0;
  uint32_t maxBindlessSamplerCount = 0;
  eastl::hash_map<uint32_t, Image *> imageSlots;
  bool enabled = false;
};

} // namespace drv3d_vulkan
