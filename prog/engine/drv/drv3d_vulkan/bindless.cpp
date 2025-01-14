// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bindless.h"

#include "driver.h"
#include "image_resource.h"
#include "texture.h"

#include <drv/3d/dag_d3dResource.h>
#include <util/dag_globDef.h>
#include "free_list_utils.h"

#include <debug/dag_assert.h>
#include <vulkan_api.h>

#include "globals.h"
#include "dummy_resources.h"
#include "physical_device_set.h"
#include "backend.h"
#include "execution_context.h"
#include "device_context.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

namespace
{

struct BindlessSetConfig
{
  bool buffered;
  VkDescriptorType type;
};

BindlessSetConfig bindlessSetConfigs[spirv::bindless::MAX_SETS] = {
  {true, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, {false, VK_DESCRIPTOR_TYPE_SAMPLER}, {true, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}};

} // namespace

void BindlessManager::init()
{
  auto &bindlessManagerBackend = Backend::bindless;
  bindlessManagerBackend.init(Globals::VK::dev, Globals::cfg.bindlessSetLimits);
  if (!Globals::VK::phy.hasBindless)
    return;

  PipelineBindlessConfig::bindlessSetCount = bindlessManagerBackend.getActiveBindlessSetCount();
  bindlessManagerBackend.fillSetLayouts(PipelineBindlessConfig::bindlessSetLayouts);

  // default bindless sampler
  samplerTable.clear();
  samplerTable.push_back(SamplerState::make_default());
  const uint32_t defaultBindlessSamplerIndex = 0;
  Globals::ctx.updateBindlessSampler(defaultBindlessSamplerIndex, SamplerState::make_default());
}

uint32_t BindlessManager::allocateBindlessResourceRange(uint32_t resourceType, uint32_t count)
{
  auto range = free_list_allocate_smallest_fit<uint32_t>(freeSlotRanges[resTypeToSlotIdx(resourceType)], count);
  if (range.empty())
  {
    auto r = size[resTypeToSlotIdx(resourceType)];
    size[resTypeToSlotIdx(resourceType)] += count;
    return r;
  }
  else
  {
    return range.front();
  }
}

uint32_t BindlessManager::resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount)
{
  if (newCount == currentCount)
  {
    return index;
  }

  uint32_t slotIdx = resTypeToSlotIdx(resourceType);

  uint32_t rangeEnd = index + currentCount;
  if (rangeEnd == size[slotIdx])
  {
    // the range is in the end of the heap, so we just update the heap size
    size[slotIdx] = index + newCount;
    return index;
  }
  if (free_list_try_resize(freeSlotRanges[slotIdx], make_value_range(index, currentCount), newCount))
  {
    return index;
  }
  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the existing descriptors :/
  uint32_t newIndex = allocateBindlessResourceRange(resourceType, newCount);
  Globals::ctx.copyBindlessDescriptors(resourceType, index, newIndex, currentCount);
  freeBindlessResourceRange(resourceType, index, currentCount);
  return newIndex;
}

void BindlessManager::freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count)
{
  uint32_t slotIdx = resTypeToSlotIdx(resourceType);
  eastl::vector<ValueRange<uint32_t>> &freeSlotRangesRef = freeSlotRanges[slotIdx];
  uint32_t &sizeRef = size[slotIdx];

  G_ASSERTF_RETURN(index + count <= sizeRef, ,
    "Vulkan: freeBindlessResourceRange tried to free out of range slots, range %u - "
    "%u, bindless count %u",
    index, index + count, sizeRef);

  Globals::ctx.updateBindlessResourcesToNull(resourceType, index, count);

  if (index + count != sizeRef)
  {
    free_list_insert_and_coalesce(freeSlotRangesRef, make_value_range(index, count));
    return;
  }
  sizeRef = index;
  if (!freeSlotRangesRef.empty() && (freeSlotRangesRef.back().stop == sizeRef))
  {
    sizeRef = freeSlotRangesRef.back().start;
    freeSlotRangesRef.pop_back();
  }
}

uint32_t BindlessManager::registerBindlessSampler(BaseTex *texture)
{
  G_ASSERT_RETURN(nullptr != texture, 0);
  return registerBindlessSampler(texture->samplerState);
}

uint32_t BindlessManager::registerBindlessSampler(SamplerResource *sampler)
{
  G_ASSERT_RETURN(nullptr != sampler, 0);
  return registerBindlessSampler(sampler->getDescription().state);
}

uint32_t BindlessManager::registerBindlessSampler(SamplerState sampler)
{
  uint32_t newIndex;
  {
    auto ref = eastl::find(begin(samplerTable), end(samplerTable), sampler);
    if (ref != end(samplerTable))
    {
      return static_cast<uint32_t>(ref - begin(samplerTable));
    }

    newIndex = static_cast<uint32_t>(samplerTable.size());
    samplerTable.push_back(sampler);
  }

  Globals::ctx.updateBindlessSampler(newIndex, sampler);
  return newIndex;
}

void BindlessManagerBackend::createBindlessLayout(const VulkanDevice &device, VkDescriptorType descriptorType,
  uint32_t descriptorCount, VulkanDescriptorSetLayoutHandle &descriptorLayout)
{
  // Create bindless global descriptor layout.
  VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                                           VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
                                           VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

  VkDescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.descriptorType = descriptorType;
  layoutBinding.descriptorCount = descriptorCount;
  layoutBinding.binding = 0;

  layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
  layoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &layoutBinding;
  layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedLayoutInfo{
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr};
  extendedLayoutInfo.bindingCount = 1;
  extendedLayoutInfo.pBindingFlags = &bindlessFlags;

  layoutInfo.pNext = &extendedLayoutInfo;

  VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, ptr(descriptorLayout)));
}

void BindlessManagerBackend::allocateBindlessSet(const VulkanDevice &device, uint32_t descriptorCount,
  const VulkanDescriptorSetLayoutHandle descriptorLayout, VulkanDescriptorSetHandle &descriptorSet)
{
  VkDescriptorSetAllocateInfo setAllocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setAllocateInfo.descriptorPool = descriptorPool;
  setAllocateInfo.descriptorSetCount = 1;
  setAllocateInfo.pSetLayouts = ptr(descriptorLayout);

  VkDescriptorSetVariableDescriptorCountAllocateInfoEXT setAllocateCountInfo{
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT, nullptr};
  uint32_t max_binding = descriptorCount;
  setAllocateCountInfo.descriptorSetCount = 1;
  setAllocateCountInfo.pDescriptorCounts = &max_binding;
  setAllocateInfo.pNext = &setAllocateCountInfo;

  VULKAN_EXIT_ON_FAIL(device.vkAllocateDescriptorSets(device.get(), &setAllocateInfo, ptr(descriptorSet)));
}

uint32_t BindlessManagerBackend::getActiveBindlessSetCount() const
{
  return Globals::VK::phy.hasBindless ? spirv::bindless::MAX_SETS : 0;
}

void BindlessManagerBackend::init(const VulkanDevice &device, BindlessSetLimits &limits)
{
  if (!Globals::VK::phy.hasBindless)
  {
    enabled = false;
    return;
  }
  // create descriptor pool supports bindless
  carray<VkDescriptorPoolSize, spirv::bindless::MAX_SETS> bindlessPoolSizes;
  uint32_t maxSets = 0;
  for (int i = 0; i < spirv::bindless::MAX_SETS; ++i)
  {
    uint32_t setCount = bindlessSetConfigs[i].buffered ? BUFFERED_SET_COUNT : 1;
    bindlessPoolSizes[i] = VkDescriptorPoolSize{bindlessSetConfigs[i].type, limits[i].slots * setCount};
    setLimits[i] = limits[i].slots;
    maxSets += setCount;
  }

  VkDescriptorPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr};
  poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
  poolCreateInfo.maxSets = maxSets;
  poolCreateInfo.poolSizeCount = bindlessPoolSizes.size();
  poolCreateInfo.pPoolSizes = bindlessPoolSizes.data();
  VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorPool(device.get(), &poolCreateInfo, nullptr, ptr(descriptorPool)));

  for (int i = 0; i < spirv::bindless::MAX_SETS; ++i)
  {
    createBindlessLayout(device, bindlessSetConfigs[i].type, limits[i].slots, layouts[i]);
    for (uint32_t j = 0; j < BUFFERED_SET_COUNT; j++)
    {
      if (!bindlessSetConfigs[i].buffered && j > 0)
      {
        bufferedSets[j][i].set = bufferedSets[0][i].set;
        continue;
      }
      allocateBindlessSet(device, limits[i].slots, layouts[i], bufferedSets[j][i].set);
    }
  }

  resetSets();

  enabled = true;
}

void BindlessManagerBackend::resetSets()
{
  for (uint32_t i = 0; i < BUFFERED_SET_COUNT; i++)
  {
    actualSetId = i;
    for (uint32_t j = 0; j < setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX]; j++)
    {
      updateBindlessTexture(j, nullptr, {});
    }
    for (uint32_t j = 0; j < setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX]; j++)
    {
      updateBindlessBuffer(j, {});
    }
  }

  for (int i = 0; i < BUFFERED_SET_COUNT; ++i)
    for (int j = 0; j < spirv::bindless::MAX_SETS; ++j)
    {
      bufferedSets[i][j].dirtyRange.reset(eastl::numeric_limits<uint32_t>::max(), 0);
    }
  actualSetId = 0;
}

void BindlessManagerBackend::shutdown(const VulkanDevice &vulkanDevice)
{
  VULKAN_LOG_CALL(vulkanDevice.vkDestroyDescriptorPool(vulkanDevice.get(), descriptorPool, NULL));

  for (VulkanDescriptorSetLayoutHandle i : layouts)
    VULKAN_LOG_CALL(vulkanDevice.vkDestroyDescriptorSetLayout(vulkanDevice.get(), i, NULL));
  imageSlots.clear();
  bufferSlots.clear();
}

VkWriteDescriptorSet BindlessManagerBackend::initSetWrite(uint32_t set_idx, uint32_t index)
{
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.dstArrayElement = index;
  descriptorWrite.descriptorType = bindlessSetConfigs[set_idx].type;
  descriptorWrite.dstSet = bufferedSets[actualSetId][set_idx].set;
  descriptorWrite.dstBinding = 0;
  return descriptorWrite;
}

void BindlessManagerBackend::cleanupBindlessTexture(uint32_t index, Image *image)
{
  G_ASSERTF(index < setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX],
    "vulkan: updating bindless resource (%s) is out of range: id=%u, "
    "setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX]=%u",
    image->getDebugName(), index, setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX]);
  G_UNUSED(image);

  G_ASSERTF(imageSlots[index] == image, "vulkan: trying to cleanup bindless slot (%u) for image %p:%s that does not own it!", index,
    image, image->getDebugName());
  imageSlots[index] = nullptr;

  VkDescriptorImageInfo descriptorImageInfo{};
  const auto &dummyResourceTable = Globals::dummyResources.getTable();
  descriptorImageInfo.imageView = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageView;
  descriptorImageInfo.imageLayout = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageLayout;
  descriptorImageInfo.sampler = VK_NULL_HANDLE;

  VkWriteDescriptorSet descriptorWrite = initSetWrite(spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX, index);
  descriptorWrite.pImageInfo = &descriptorImageInfo;
  auto &vulkanDevice = Globals::VK::dev;
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);

  markDirtyRange(spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX, index);
}

void BindlessManagerBackend::updateBindlessTexture(uint32_t index, Image *image, const ImageViewState view)
{
  G_ASSERTF(index < setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX],
    "vulkan: updating bindless resource (%s) is out of range: id=%u, "
    "setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX]=%u",
    image ? image->getDebugName() : "<nullptr>", index, setLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX]);

  VkDescriptorImageInfo descriptorImageInfo{};

  auto iter = imageSlots.find(index);
  Image *oldImage = iter != imageSlots.end() ? iter->second : nullptr;
  if (oldImage != image)
  {
    if (oldImage)
      oldImage->removeBindlessSlot(index);
    if (image)
      image->addBindlessSlot(index);
    imageSlots[index] = image;
  }

  if (image)
  {
    // NOTE: for content textures VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL should be ensured by the driver
    // for other resources the correct layout must be ensured by explicit barriers.
    // The questionable state is VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, which
    // enables using the texture as a readonly depth attachment and as shader resource in the same time.
    // To support this the driver should be informed about it and it requires extending the d3d interface.
    descriptorImageInfo.imageView = image->getImageView(view);
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    descriptorImageInfo.imageView = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageView;
    descriptorImageInfo.imageLayout = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageLayout;
  }
  descriptorImageInfo.sampler = VK_NULL_HANDLE;

  VkWriteDescriptorSet descriptorWrite = initSetWrite(spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX, index);
  descriptorWrite.pImageInfo = &descriptorImageInfo;
  auto &vulkanDevice = Globals::VK::dev;
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);

  markDirtyRange(spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX, index);
}

void BindlessManagerBackend::cleanupBindlessBuffer(uint32_t index, Buffer *buf)
{
  G_ASSERTF(index < setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX],
    "vulkan: updating bindless resource (%s) is out of range: id=%u, "
    "setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX]=%u",
    buf->getDebugName(), index, setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX]);
  G_UNUSED(buf);

  G_ASSERTF(bufferSlots[index] == buf, "vulkan: trying to cleanup bindless slot (%u) for buffer %p:%s that does not own it!", index,
    buf, buf->getDebugName());
  bufferSlots[index] = nullptr;

  const auto &dummyResourceTable = Globals::dummyResources.getTable();
  VkDescriptorBufferInfo descriptorBufferInfo = dummyResourceTable[spirv::MISSING_BUFFER_INDEX].descriptor.buffer;

  VkWriteDescriptorSet descriptorWrite = initSetWrite(spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX, index);
  descriptorWrite.pBufferInfo = &descriptorBufferInfo;
  auto &vulkanDevice = Globals::VK::dev;
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);

  markDirtyRange(spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX, index);
}

void BindlessManagerBackend::updateBindlessBuffer(uint32_t index, const BufferRef &buf)
{
  G_ASSERTF(index < setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX],
    "vulkan: updating bindless resource (%s) is out of range: id=%u, "
    "setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX]=%u",
    buf ? buf.buffer->getDebugName() : "<nullptr>", index, setLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX]);

  VkDescriptorBufferInfo descriptorBufferInfo{};

  auto iter = bufferSlots.find(index);
  Buffer *oldBuf = iter != bufferSlots.end() ? iter->second : nullptr;
  if (oldBuf != buf.buffer)
  {
    if (oldBuf)
      oldBuf->removeBindlessSlot(index);
    if (buf)
      buf.buffer->addBindlessSlot(index);
    bufferSlots[index] = buf.buffer;
  }

  if (buf)
  {
    descriptorBufferInfo.buffer = buf.buffer->getHandle();
    descriptorBufferInfo.offset = buf.bufOffset(0);
    descriptorBufferInfo.range = buf.visibleDataSize;
  }
  else
  {
    const auto &dummyResourceTable = Globals::dummyResources.getTable();
    descriptorBufferInfo = dummyResourceTable[spirv::MISSING_BUFFER_INDEX].descriptor.buffer;
  }

  VkWriteDescriptorSet descriptorWrite = initSetWrite(spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX, index);
  descriptorWrite.pBufferInfo = &descriptorBufferInfo;
  auto &vulkanDevice = Globals::VK::dev;
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);

  markDirtyRange(spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX, index);
}

void BindlessManagerBackend::updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo)
{
  VkWriteDescriptorSet descriptorWrite = initSetWrite(spirv::bindless::SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX, index);

  VkDescriptorImageInfo descriptorImageInfo;
  descriptorImageInfo.sampler = samplerInfo->colorSampler();
  descriptorImageInfo.imageView = VK_NULL_HANDLE;
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  descriptorWrite.pImageInfo = &descriptorImageInfo;

  auto &vulkan_device = Globals::VK::dev;
  vulkan_device.vkUpdateDescriptorSets(vulkan_device.get(), 1, &descriptorWrite, 0, nullptr);
}

void BindlessManagerBackend::copyDescriptors(uint32_t set_idx, uint32_t ring_src, uint32_t ring_dst, uint32_t src, uint32_t dst,
  uint32_t count)
{
  VkCopyDescriptorSet copyDescriptorSet{};
  copyDescriptorSet.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
  copyDescriptorSet.pNext = nullptr;
  copyDescriptorSet.srcSet = bufferedSets[ring_src][set_idx].set;
  copyDescriptorSet.srcBinding = 0;
  copyDescriptorSet.srcArrayElement = src;
  copyDescriptorSet.dstSet = bufferedSets[ring_dst][set_idx].set;
  copyDescriptorSet.dstBinding = 0;
  copyDescriptorSet.dstArrayElement = dst;
  copyDescriptorSet.descriptorCount = count;

  auto &vulkan_device = Globals::VK::dev;
  vulkan_device.vkUpdateDescriptorSets(vulkan_device.get(), 0, nullptr, 1, &copyDescriptorSet);
}

void BindlessManagerBackend::copyBindlessDescriptors(uint32_t resource_type, uint32_t src, uint32_t dst, uint32_t count)
{
  uint32_t setIdx = resTypeToSetIdx(resource_type);
  copyDescriptors(setIdx, actualSetId, actualSetId, src, dst, count);
  markDirtyRange(setIdx, dst, count);
}

void BindlessManagerBackend::bindSets(VkPipelineBindPoint bindPoint, VulkanPipelineLayoutHandle pipelineLayout)
{
  if (!enabled)
    return;

  VulkanDescriptorSetHandle sets[spirv::bindless::MAX_SETS];
  for (int i = 0; i < spirv::bindless::MAX_SETS; ++i)
    sets[i] = bufferedSets[actualSetId][i].set;

  Backend::cb.wCmdBindDescriptorSets(bindPoint, pipelineLayout, spirv::bindless::FIRST_DESCRIPTOR_SET_ACTUAL_INDEX,
    spirv::bindless::MAX_SETS, ary(sets), 0, nullptr);
}

void BindlessManagerBackend::advance()
{
  if (!enabled)
    return;

  uint32_t nextSetId = (actualSetId + 1) % BUFFERED_SET_COUNT;

  for (int i = 0; i < spirv::bindless::MAX_SETS; ++i)
  {
    if (!bindlessSetConfigs[i].buffered)
      continue;

    // The dirty descriptors could be copied from the appropriate sets
    // However due to the high amount of update descriptor calls from rendercode
    // the dirty ranges mostly overlaps, so aggregating them and copying the
    // the descriptor from the last set is more effective right now.
    // It has to contains the required descriptors, since it is updated with them
    // in the last advance
    ValueRange<uint32_t> copyRange(eastl::numeric_limits<uint32_t>::max(), 0);
    for (uint32_t j = nextSetId + 1; j < nextSetId + BUFFERED_SET_COUNT; j++)
    {
      uint32_t setId = j % BUFFERED_SET_COUNT;
      G_ASSERTF(setId != nextSetId, "Next set shouldn't be processed here!");
      const auto &dirtyRange = bufferedSets[setId][i].dirtyRange;
      copyRange.start = eastl::min(copyRange.start, dirtyRange.start);
      copyRange.stop = eastl::max(copyRange.stop, dirtyRange.stop);
    }

    if (copyRange.start < copyRange.stop)
      copyDescriptors(i, actualSetId, nextSetId, copyRange.start, copyRange.start, copyRange.size());
    bufferedSets[nextSetId][i].dirtyRange.reset(eastl::numeric_limits<uint32_t>::max(), 0);
  }

  actualSetId = nextSetId;
}
