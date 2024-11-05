// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/memory.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/algorithm.h>
#include "vulkan_device.h"
#include "pipeline/descriptor_table.h"

namespace drv3d_vulkan
{
class Buffer;

class DescriptorSet
{
private:
#if _TARGET_ANDROID
  static constexpr uint32_t POOL_SIZE = 64;
#else
  static constexpr uint32_t POOL_SIZE = 256;
#endif
  struct PoolInfo
  {
    uint32_t frameIndex;
    eastl::fixed_vector<VulkanDescriptorSetHandle, POOL_SIZE, false> freeSets;
    VulkanDescriptorPoolHandle pool;

    PoolInfo(VulkanDevice &device, dag::ConstSpan<VkDescriptorPoolSize> comp, VulkanDescriptorSetLayoutHandle layout,
      uint32_t frame_index) :
      frameIndex(frame_index)
    {
      VkDescriptorPoolSize composition[spirv::SHADER_HEADER_DECRIPTOR_COUNT_SIZE];
      uint32_t compositionCount = 0;
      for (auto &&c : comp)
      {
        VkDescriptorPoolSize &t = composition[compositionCount++];
        t = c;
        // need to scale by POOL_SIZE
        t.descriptorCount *= POOL_SIZE;
      }
      G_ASSERTF(compositionCount, "vulkan: shader without binds trying to create descriptor sets");
      VkDescriptorPoolCreateInfo dpci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
      dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      dpci.maxSets = POOL_SIZE;
      dpci.poolSizeCount = compositionCount;
      dpci.pPoolSizes = composition;
      VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorPool(device.get(), &dpci, NULL, ptr(pool)));
      // right away alloc all instances and manage them localy
      // need to create an array for each allocation
      VulkanDescriptorSetLayoutHandle layoutList[POOL_SIZE];
      for (auto &&l : layoutList)
        l = layout;
      freeSets.resize(POOL_SIZE);
      VkDescriptorSetAllocateInfo dsai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      dsai.descriptorPool = pool;
      dsai.descriptorSetCount = POOL_SIZE;
      dsai.pSetLayouts = ary(layoutList);
      VULKAN_EXIT_ON_FAIL(device.vkAllocateDescriptorSets(device.get(), &dsai, ary(freeSets.data())));
    }
    VulkanDescriptorSetHandle allocate()
    {
      VulkanDescriptorSetHandle result = freeSets.back();
      freeSets.pop_back();
      return result;
    }
    void shutdown(VulkanDevice &device)
    {
      VULKAN_LOG_CALL(device.vkDestroyDescriptorPool(device.get(), pool, NULL));
      freeSets.clear();
      pool = VulkanNullHandle();
    }
  };
  struct SetStorageEntry
  {
    SetStorageEntry() = default;
    ~SetStorageEntry() = default;
    SetStorageEntry(const SetStorageEntry &) = default;
    SetStorageEntry &operator=(const SetStorageEntry &) = default;
    SetStorageEntry(SetStorageEntry &&) = default;
    SetStorageEntry &operator=(SetStorageEntry &&) = default;

    SetStorageEntry(VulkanDescriptorSetHandle ds, PoolInfo *pi) : set(ds), pool(pi) {}

    VulkanDescriptorSetHandle set;
    PoolInfo *pool = nullptr;
  };

#if VK_KHR_descriptor_update_template
  VulkanDescriptorUpdateTemplateKHRHandle updateTemplate;
#endif
  VulkanDescriptorSetLayoutHandle layout;
  eastl::vector<eastl::unique_ptr<PoolInfo>> pools;
  eastl::vector<SetStorageEntry> sets;
  eastl::vector<VulkanDescriptorSetHandle> setRings[GPU_TIMELINE_HISTORY_SIZE];
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  VulkanDescriptorSetHandle lastUnnamedSet;
#endif
  uint32_t ringOffset = 0;
  uint32_t ringIndex = 0;
  size_t lastAbsFrameIndex = 0;

public:
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  VulkanDescriptorSetHandle getSetForNaming()
  {
    VulkanDescriptorSetHandle ret = lastUnnamedSet;
    lastUnnamedSet = VulkanNullHandle();
    return ret;
  }
#else
  VulkanDescriptorSetHandle getSetForNaming() { return VulkanDescriptorSetHandle(); }
#endif

  inline VulkanDescriptorSetLayoutHandle getLayout() const { return layout; }

  static VulkanDescriptorSetHandle dummyHandle;

  void dumpLogGeneralInfo()
  {
    debug("    descriptorSet: layout %llu ring (ofs %u idx %u lastAbsFrame %u)", generalize(layout), ringOffset, ringIndex,
      lastAbsFrameIndex);
    debug("      %u sets %u pools", sets.size(), pools.size());
  }

  DescriptorSet() = default;
  ~DescriptorSet() = default;
  DescriptorSet(const DescriptorSet &) = delete;
  DescriptorSet &operator=(const DescriptorSet &) = default;
  DescriptorSet(DescriptorSet &&) = delete;
  DescriptorSet &operator=(DescriptorSet &&) = default;
  struct SetWriteDataStore
  {
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
    VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo[spirv::T_REGISTER_INDEX_MAX]; //-V730_NOINIT
#endif
    VkWriteDescriptorSet writeInfoSet[spirv::REGISTER_ENTRIES]; //-V730_NOINIT
  };
  spirv::ShaderHeader header = {};
  spirv::HashValue hash = {};
  bool matches(const ShaderModuleHeader *shModule) const
  {
    if (shModule == nullptr)
      return (hash == spirv::HashValue{});

    if (shModule->hash != hash)
      return false;

    return memcmp(&header, &shModule->header, sizeof(header)) == 0;
  }
  inline dag::ConstSpan<VkDescriptorPoolSize> getPoolDescriptorSizes() const
  {
    return make_span_const(header.descriptorCounts, header.descriptorCountsCount);
  }
  inline void writeSet(SetWriteDataStore &output, const VkAnyDescriptorInfo *desc_array, VulkanDescriptorSetHandle set) const
  {
    // this way avoids warning about not initialized store
    // and its fine, even if header.registerCount is 0
    uint32_t i = 0;
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
    uint32_t asExtra = 0;
#endif
    do
    {
      uint32_t absReg = header.registerIndex[i];
      const VkAnyDescriptorInfo &dsInfo = desc_array[absReg];
      VkWriteDescriptorSet &wdi = output.writeInfoSet[i];

      wdi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      wdi.dstSet = set;
      wdi.dstBinding = i;
      wdi.dstArrayElement = 0;
      wdi.descriptorCount = 1;
      wdi.descriptorType = header.descriptorTypes[i];
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      if (wdi.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
      {
        auto &asInfo = output.accelerationStructureInfo[asExtra++];
        asInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        asInfo.pNext = nullptr;
        asInfo.accelerationStructureCount = 1;
        asInfo.pAccelerationStructures = &dsInfo.raytraceAccelerationStructure;

        wdi.pNext = &asInfo;
        wdi.pImageInfo = nullptr;
        wdi.pBufferInfo = nullptr;
        wdi.pTexelBufferView = nullptr;
      }
      else
#endif
      {
        wdi.pImageInfo = &dsInfo.image;
        wdi.pBufferInfo = &dsInfo.buffer;
        wdi.pTexelBufferView = &dsInfo.texelBuffer;
        wdi.pNext = nullptr;
      }
    } while (++i < header.registerCount);
  }
  void init(VulkanDevice &device, const spirv::HashValue &hsh, const spirv::ShaderHeader &cfg, VkShaderStageFlags stage_bits)
  {
    hash = hsh;
    header = cfg;
    VkDescriptorSetLayoutBinding bindingDefs[spirv::REGISTER_ENTRIES];
    for (uint32_t i = 0; i < header.registerCount; ++i)
    {
      VkDescriptorSetLayoutBinding &target = bindingDefs[i];
      target.descriptorType = header.descriptorTypes[i];

      target.binding = i;
      target.descriptorCount = 1;
      target.stageFlags = stage_bits;
      target.pImmutableSamplers = nullptr;
    }
    VkDescriptorSetLayoutCreateInfo dslci = //
      {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0};
    dslci.bindingCount = header.registerCount;
    dslci.pBindings = bindingDefs;
    VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorSetLayout(device.get(), &dslci, NULL, ptr(layout)));
  }

  void initEmpty(VulkanDevice &device)
  {
    header.maxConstantCount = 0;
    header.bonesConstantsUsed = 0;
    header.tRegisterUseMask = 0;
    header.uRegisterUseMask = 0;
    header.bRegisterUseMask = 0;
    header.inputMask = 0;
    header.imageCount = 0;
    header.bufferCount = 0;
    header.bufferViewCount = 0;
    header.constBufferCount = 0;
    header.accelerationStructursCount = 0;
    header.descriptorCountsCount = 0;
    header.registerCount = 0;

    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0};
    dslci.bindingCount = 0;
    dslci.pBindings = nullptr;
    VULKAN_EXIT_ON_FAIL(device.vkCreateDescriptorSetLayout(device.get(), &dslci, NULL, ptr(layout)));
  }

  void initUpdateTemplate(VulkanDevice &device, VkPipelineBindPoint bind_point, VkPipelineLayout pipe_layout, uint32_t set_index)
  {
#if VK_KHR_descriptor_update_template
    if (device.hasExtension<DescriptorUpdateTemplateKHR>() && (header.registerCount > 0))
    {
      VkDescriptorUpdateTemplateEntryKHR entries[spirv::REGISTER_ENTRIES];
      VkDescriptorUpdateTemplateCreateInfoKHR dutci = //
        {VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO_KHR, nullptr, 0};
      dutci.descriptorUpdateEntryCount = header.registerCount;
      dutci.pDescriptorUpdateEntries = entries;
      dutci.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET_KHR;
      dutci.descriptorSetLayout = layout;
      dutci.pipelineBindPoint = bind_point;
      dutci.pipelineLayout = pipe_layout;
      dutci.set = set_index;

      for (uint32_t i = 0; i < header.registerCount; ++i)
      {
        VkDescriptorUpdateTemplateEntryKHR &target = entries[i];
        target.dstBinding = i;
        target.dstArrayElement = 0;
        target.descriptorCount = 1;
        target.descriptorType = header.descriptorTypes[i];
        target.offset = header.registerIndex[i] * sizeof(VkAnyDescriptorInfo);
        target.stride = 0; // ignored, only used if descriptorCount > 1
      }

      if (VULKAN_CHECK_FAIL(device.vkCreateDescriptorUpdateTemplateKHR(device.get(), &dutci, nullptr, ptr(updateTemplate))))
        // not fatal to fail here, just use default update method as fallback
        updateTemplate = VulkanNullHandle();
    }
#else
    G_UNUSED(device);
    G_UNUSED(bind_point);
    G_UNUSED(pipe_layout);
    G_UNUSED(pipe_layout);
    G_UNUSED(set_index);
#endif
  }
  void shutdown(VulkanDevice &device)
  {
    // no need to delete them individually, if the pool is
    // erased all sets are erased too
    sets.clear();
    for (auto &&pool : pools)
      pool->shutdown(device);
    pools.clear();
    if (!is_null(layout))
    {
      VULKAN_LOG_CALL(device.vkDestroyDescriptorSetLayout(device.get(), layout, NULL));
      layout = VulkanNullHandle();
    }
#if VK_KHR_descriptor_update_template
    if (!is_null(updateTemplate))
    {
      VULKAN_LOG_CALL(device.vkDestroyDescriptorUpdateTemplateKHR(device.get(), updateTemplate, nullptr));
      updateTemplate = VulkanNullHandle();
    }
#endif
    for (auto &i : setRings)
      i.clear();
  }
  void resetAll()
  {
    for (auto &&set : sets)
      set.pool->freeSets.push_back(set.set);
    sets.clear();
    ringOffset = 0;
    ringIndex = 0;
    lastAbsFrameIndex = 0;
    for (auto &i : setRings)
      i.clear();
  }

  void updateSet(VulkanDevice &device, const VkAnyDescriptorInfo *desc_array, VulkanDescriptorSetHandle set)
  {
#if VK_KHR_descriptor_update_template
    if (!is_null(updateTemplate))
      VULKAN_LOG_CALL(device.vkUpdateDescriptorSetWithTemplateKHR(device.get(), set, updateTemplate, desc_array));
    else
#endif
    {
      if (header.registerCount > 0)
      {
        SetWriteDataStore setWriteStore;
        writeSet(setWriteStore, desc_array, set);
        VULKAN_LOG_CALL(device.vkUpdateDescriptorSets(device.get(), header.registerCount, setWriteStore.writeInfoSet, 0, NULL));
      }
    }
  }

  VulkanDescriptorSetHandle getNewSet(VulkanDevice &device, uint32_t frame_index)
  {
    PoolInfo *pool = nullptr;
    for (auto &&pp : pools)
    {
      if (!pp->freeSets.size())
        continue;

      if (pp->frameIndex == frame_index)
      {
        pool = pp.get();
        break;
      }
    }

    if (!pool)
    {
      pool = new PoolInfo(device, getPoolDescriptorSizes(), layout, frame_index);
      pools.emplace_back(pool);
    }

    auto set = pool->allocate();
    sets.emplace_back(set, pool);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    lastUnnamedSet = set;
#endif
    return set;
  }

  VulkanDescriptorSetHandle getSet(VulkanDevice &device, size_t abs_frame_index, const VkAnyDescriptorInfo *desc_array)
  {
    G_ASSERTF(!is_null(layout), "vulkan: used uninitialized descriptor set");

    // if shader is not using any binds, return dummy set and filter it out later on
    // we need to return some dummy set for tracking to work properly
    if (header.descriptorCountsCount == 0)
      return dummyHandle;

    // change ring every new abs frame index to avoid ring growth
    if (lastAbsFrameIndex != abs_frame_index)
    {
      ringOffset = 0;
      ringIndex = (ringIndex + 1) % GPU_TIMELINE_HISTORY_SIZE;
      lastAbsFrameIndex = abs_frame_index;
    }

    VulkanDescriptorSetHandle ret;
    auto &setRing = setRings[ringIndex];

    if (ringOffset < setRing.size())
      ret = setRing[ringOffset];
    else
    {
      ret = getNewSet(device, ringIndex);
      setRing.push_back(ret);
    }
    ++ringOffset;

    updateSet(device, desc_array, ret);
    return ret;
  }
};
} // namespace drv3d_vulkan