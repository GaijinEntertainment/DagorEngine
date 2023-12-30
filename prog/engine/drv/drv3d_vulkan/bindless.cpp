#include "bindless.h"

#include "driver.h"
#include "device.h"
#include "image_resource.h"
#include "texture.h"
#include "device_context.h"

#include <3d/dag_d3dResource.h>
#include <util/dag_globDef.h>
#include "free_list_utils.h"

#include <debug/dag_assert.h>
#include <vulkan_api.h>

using namespace drv3d_vulkan;

uint32_t BindlessManager::allocateBindlessResourceRange(uint32_t resourceType, uint32_t count)
{
  G_ASSERTF(RES3D_SBUF != resourceType, "vulkan: allocating bindless resource resource range with type RES3D_SBUF is not supported");
  G_UNUSED(resourceType);

  auto range = free_list_allocate_smallest_fit<uint32_t>(freeSlotRanges, count);
  if (range.empty())
  {
    auto r = size;
    size += count;
    return r;
  }
  else
  {
    return range.front();
  }
}

uint32_t BindlessManager::resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount)
{
  G_ASSERTF(RES3D_SBUF != resourceType, "vulkan: resizing bindless resource resource range with type RES3D_SBUF is not supported");

  if (newCount == currentCount)
  {
    return index;
  }

  uint32_t rangeEnd = index + currentCount;
  if (rangeEnd == size)
  {
    // the range is in the end of the heap, so we just update the heap size
    size = index + newCount;
    return index;
  }
  if (free_list_try_resize(freeSlotRanges, make_value_range(index, currentCount), newCount))
  {
    return index;
  }
  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the existing descriptors :/
  uint32_t newIndex = allocateBindlessResourceRange(resourceType, newCount);
  get_device().getContext().copyBindlessTextureDescriptors(index, newIndex, currentCount);
  freeBindlessResourceRange(resourceType, index, currentCount);
  return newIndex;
}

void BindlessManager::freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count)
{
  G_ASSERTF(RES3D_SBUF != resourceType, "vulkan: resizing bindless resource resource range with type RES3D_SBUF is not supported");
  G_ASSERTF_RETURN(index + count <= size, ,
    "Vulkan: freeBindlessResourceRange tried to free out of range slots, range %u - "
    "%u, bindless count %u",
    index, index + count, size);

  get_device().getContext().updateBindlessResourcesToNull(resourceType, index, count);

  if (index + count != size)
  {
    free_list_insert_and_coalesce(freeSlotRanges, make_value_range(index, count));
    return;
  }
  size = index;
  if (!freeSlotRanges.empty() && (freeSlotRanges.back().stop == size))
  {
    size = freeSlotRanges.back().start;
    freeSlotRanges.pop_back();
  }
}

uint32_t BindlessManager::registerBindlessSampler(BaseTex *texture)
{
  SamplerState sampler = texture->samplerState;

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

  auto &device = get_device();
  device.getContext().updateBindlessSampler(newIndex, sampler);
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
  return get_device().getDeviceProperties().hasBindless ? spirv::bindless::MAX_SETS : 0;
}

void BindlessManagerBackend::init(const VulkanDevice &device, uint32_t _maxBindlessTextureCount, uint32_t _maxBindlessSamplerCount)
{
  maxBindlessTextureCount = _maxBindlessTextureCount;
  maxBindlessSamplerCount = _maxBindlessSamplerCount;

  G_ASSERTF(maxBindlessTextureCount <= get_device().getDeviceProperties().maxBindlessTextures,
    "vulkan: BindlessManagerBackend::init - too many bindless texture resource requested: %u, device limit: %u",
    maxBindlessTextureCount, get_device().getDeviceProperties().maxBindlessTextures);
  G_ASSERTF(maxBindlessSamplerCount <= get_device().getDeviceProperties().maxBindlessSamplers,
    "vulkan: BindlessManagerBackend::init - too many bindless sampler resource requested: %u, device limit: %u",
    maxBindlessSamplerCount, get_device().getDeviceProperties().maxBindlessSamplers);

  // create descriptor pool supports bindless
  eastl::array<VkDescriptorPoolSize, 2> bindlessPoolSizes = {
    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxBindlessTextureCount * BUFFERED_SET_COUNT},
    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, maxBindlessSamplerCount}};

  VkDescriptorPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr};
  poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
  poolCreateInfo.maxSets = BUFFERED_SET_COUNT + 1;
  poolCreateInfo.poolSizeCount = bindlessPoolSizes.size();
  poolCreateInfo.pPoolSizes = bindlessPoolSizes.data();
  VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorPool(device.get(), &poolCreateInfo, nullptr, ptr(descriptorPool)));

  createBindlessLayout(device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxBindlessTextureCount, textureDescriptorLayout);
  for (uint32_t i = 0; i < BUFFERED_SET_COUNT; i++)
  {
    allocateBindlessSet(device, maxBindlessTextureCount, textureDescriptorLayout, textureDescriptorSets[i].set);
  }

  createBindlessLayout(device, VK_DESCRIPTOR_TYPE_SAMPLER, maxBindlessSamplerCount, samplerDescriptorLayout);
  allocateBindlessSet(device, maxBindlessSamplerCount, samplerDescriptorLayout, samplerDescriptorSet);

  resetTextureSets();

  sets[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX] = textureDescriptorSets[actualSetId].set;
  sets[spirv::bindless::SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX] = samplerDescriptorSet;
  enabled = true;
}

void BindlessManagerBackend::resetTextureSets()
{
  for (uint32_t i = 0; i < BUFFERED_SET_COUNT; i++)
  {
    actualSetId = i;
    for (uint32_t j = 0; j < maxBindlessTextureCount; j++)
    {
      updateBindlessTexture(j, nullptr, {});
    }
  }

  actualSetId = 0;
  for (auto &textureSet : textureDescriptorSets)
  {
    textureSet.dirtyRange.reset(eastl::numeric_limits<uint32_t>::max(), 0);
  }
}

void BindlessManagerBackend::shutdown(const VulkanDevice &vulkanDevice)
{
  VULKAN_LOG_CALL(vulkanDevice.vkDestroyDescriptorPool(vulkanDevice.get(), descriptorPool, NULL));

  VULKAN_LOG_CALL(vulkanDevice.vkDestroyDescriptorSetLayout(vulkanDevice.get(), textureDescriptorLayout, NULL));
  VULKAN_LOG_CALL(vulkanDevice.vkDestroyDescriptorSetLayout(vulkanDevice.get(), samplerDescriptorLayout, NULL));
  imageSlots.clear();
}

void BindlessManagerBackend::cleanupBindlessTexture(uint32_t index, Image *image)
{
  G_ASSERTF(index < maxBindlessTextureCount,
    "vulkan: updating bindless resource (%s) is out of range: id=%u, maxBindlessTextureCount=%u", image->getDebugName(), index,
    maxBindlessTextureCount);
  G_UNUSED(image);

  G_ASSERTF(imageSlots[index] == image, "vulkan: trying to cleanup bindless slot (%u) for image %p:%s that does not own it!", index,
    image, image->getDebugName());

  imageSlots[index] = nullptr;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.dstArrayElement = index;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorWrite.dstSet = textureDescriptorSets[actualSetId].set;
  descriptorWrite.dstBinding = 0;

  auto &device = get_device();
  VkDescriptorImageInfo descriptorImageInfo{};
  const auto &dummyResourceTable = device.getDummyResourceTable();
  descriptorImageInfo.imageView = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageView;
  descriptorImageInfo.imageLayout = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageLayout;
  descriptorImageInfo.sampler = VK_NULL_HANDLE;

  markDirtyRange(index);

  descriptorWrite.pImageInfo = &descriptorImageInfo;
  auto &vulkanDevice = device.getVkDevice();
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);
}

void BindlessManagerBackend::updateBindlessTexture(uint32_t index, Image *image, const ImageViewState view)
{
  G_ASSERTF(index < maxBindlessTextureCount,
    "vulkan: updating bindless resource (%s) is out of range: id=%u, maxBindlessTextureCount=%u",
    image ? image->getDebugName() : "<nullptr>", index, maxBindlessTextureCount);

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.dstArrayElement = index;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorWrite.dstSet = textureDescriptorSets[actualSetId].set;
  descriptorWrite.dstBinding = 0;

  auto &device = get_device();
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
    descriptorImageInfo.imageView = device.getImageView(image, view);
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  else
  {
    const auto &dummyResourceTable = device.getDummyResourceTable();
    descriptorImageInfo.imageView = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageView;
    descriptorImageInfo.imageLayout = dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageLayout;
  }

  descriptorImageInfo.sampler = VK_NULL_HANDLE;
  descriptorWrite.pImageInfo = &descriptorImageInfo;

  markDirtyRange(index);

  auto &vulkanDevice = device.getVkDevice();
  vulkanDevice.vkUpdateDescriptorSets(vulkanDevice.get(), 1, &descriptorWrite, 0, nullptr);
}

void BindlessManagerBackend::updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo)
{
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.dstArrayElement = index;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorWrite.dstSet = samplerDescriptorSet;
  descriptorWrite.dstBinding = 0;

  VkDescriptorImageInfo descriptorImageInfo;
  descriptorImageInfo.sampler = samplerInfo->colorSampler();
  descriptorImageInfo.imageView = VK_NULL_HANDLE;
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  descriptorWrite.pImageInfo = &descriptorImageInfo;

  auto &vulkan_device = get_device().getVkDevice();
  vulkan_device.vkUpdateDescriptorSets(vulkan_device.get(), 1, &descriptorWrite, 0, nullptr);
}

void BindlessManagerBackend::copyBindlessTextureDescriptors(uint32_t src, uint32_t dst, uint32_t count)
{
  VkCopyDescriptorSet copyDescriptorSet{};
  copyDescriptorSet.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
  copyDescriptorSet.pNext = nullptr;
  copyDescriptorSet.srcSet = textureDescriptorSets[actualSetId].set;
  copyDescriptorSet.srcBinding = 0;
  copyDescriptorSet.srcArrayElement = src;
  copyDescriptorSet.dstSet = textureDescriptorSets[actualSetId].set;
  copyDescriptorSet.dstBinding = 0;
  copyDescriptorSet.dstArrayElement = dst;
  copyDescriptorSet.descriptorCount = count;

  markDirtyRange(dst, count);

  auto &vulkan_device = get_device().getVkDevice();
  vulkan_device.vkUpdateDescriptorSets(vulkan_device.get(), 0, nullptr, 1, &copyDescriptorSet);
}

void BindlessManagerBackend::bindSets(ExecutionContext &target, VkPipelineBindPoint bindPoint,
  VulkanPipelineLayoutHandle pipelineLayout)
{
  if (!enabled)
    return;

  target.vkDev.vkCmdBindDescriptorSets(target.frameCore, bindPoint, pipelineLayout, spirv::bindless::FIRST_DESCRIPTOR_SET_ACTUAL_INDEX,
    spirv::bindless::MAX_SETS, ary(sets), 0, nullptr);
}

void BindlessManagerBackend::advance()
{
  if (!enabled)
    return;

  uint32_t nextSetId = (actualSetId + 1) % textureDescriptorSets.size();

  // The dirty descriptors could be copied from the appropriate texture sets
  // However due to the high amount of update descriptor calls from rendercode
  // the dirty ranges mostly overlaps, so aggregating them and copying the
  // the descriptor from the last set is more effective right now.
  // It has to contains the required descriptors, since it is updated with them
  // in the last advance
  ValueRange<uint32_t> copyRange(eastl::numeric_limits<uint32_t>::max(), 0);
  for (uint32_t i = nextSetId + 1; i < nextSetId + BUFFERED_SET_COUNT; i++)
  {
    uint32_t setId = i % BUFFERED_SET_COUNT;
    G_ASSERTF(setId != nextSetId, "Next set shouldn't be processed here!");
    const auto &dirtyRange = textureDescriptorSets[setId].dirtyRange;
    copyRange.start = eastl::min(copyRange.start, dirtyRange.start);
    copyRange.stop = eastl::max(copyRange.stop, dirtyRange.stop);
  }

  if (copyRange.start < copyRange.stop)
  {
    VkCopyDescriptorSet copyDescriptorSet{};

    copyDescriptorSet.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copyDescriptorSet.pNext = nullptr;
    copyDescriptorSet.srcSet = textureDescriptorSets[actualSetId].set;
    copyDescriptorSet.srcBinding = 0;
    copyDescriptorSet.srcArrayElement = copyRange.start;
    copyDescriptorSet.dstSet = textureDescriptorSets[nextSetId].set;
    copyDescriptorSet.dstBinding = 0;
    copyDescriptorSet.dstArrayElement = copyRange.start;
    copyDescriptorSet.descriptorCount = copyRange.size();

    auto &vulkan_device = get_device().getVkDevice();
    vulkan_device.vkUpdateDescriptorSets(vulkan_device.get(), 0, nullptr, 1, &copyDescriptorSet);
  }

  actualSetId = nextSetId;
  textureDescriptorSets[actualSetId].dirtyRange.reset(eastl::numeric_limits<uint32_t>::max(), 0);

  sets[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX] = textureDescriptorSets[actualSetId].set;
  sets[spirv::bindless::SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX] = samplerDescriptorSet;
}
