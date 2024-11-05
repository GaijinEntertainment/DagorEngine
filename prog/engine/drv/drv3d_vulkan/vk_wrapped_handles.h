// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"

// meta type, represents VK_NULL_HANDLE
struct VulkanNullHandle
{};
// typedef to able casting handles to general type
typedef uint64_t VulkanHandle;

template <typename ValueType, typename TagType>
struct VulkanWrappedHandle
{
  ValueType value;

  inline operator ValueType() const { return value; }
  inline VulkanWrappedHandle() : value(VK_NULL_HANDLE) {}
  inline VulkanWrappedHandle(VulkanHandle v) : value((ValueType)v) {}
  inline VulkanWrappedHandle &operator=(VulkanNullHandle)
  {
    value = VK_NULL_HANDLE;
    return *this;
  }
  friend inline bool operator==(VulkanWrappedHandle l, VulkanWrappedHandle r) { return l.value == r.value; }
  friend inline bool operator!=(VulkanWrappedHandle l, VulkanWrappedHandle r) { return l.value != r.value; }
  friend inline bool operator==(ValueType l, VulkanWrappedHandle r) { return l == r.value; }
  friend inline bool operator!=(ValueType l, VulkanWrappedHandle r) { return l != r.value; }
  friend inline bool operator==(VulkanWrappedHandle l, ValueType r) { return l.value == r; }
  friend inline bool operator!=(VulkanWrappedHandle l, ValueType r) { return l.value != r; }
  VulkanWrappedHandle(const VulkanWrappedHandle &) = default;
  VulkanWrappedHandle &operator=(const VulkanWrappedHandle &) = default;

  // we use this to avoid mixing handles, but it will break
  // generalized -> typed handle conversions on 32bit builds
#if _TARGET_64BIT
  VulkanWrappedHandle(ValueType v) = delete; // default construct and assign
#endif
  VulkanWrappedHandle(VulkanNullHandle) = delete; // use default constructor
  bool operator!() const = delete;                // use is_null()

  friend void swap(VulkanWrappedHandle &l, VulkanWrappedHandle &r)
  {
    ValueType t = l.value;
    l.value = r.value;
    r.value = t;
  }
};

// needed this way, as overloading the & operator breaks container (may update them to use addressof?)
template <typename V, typename T>
inline V *ptr(VulkanWrappedHandle<V, T> &w)
{
  return &w.value;
}
template <typename V, typename T>
inline const V *ptr(const VulkanWrappedHandle<V, T> &w)
{
  return &w.value;
}
template <typename V, typename T>
inline V *ary(VulkanWrappedHandle<V, T> *w)
{
  return &w->value;
}
template <typename V, typename T>
inline const V *ary(const VulkanWrappedHandle<V, T> *w)
{
  return &w->value;
}
// less typing...
template <typename V, typename T>
inline bool is_null(VulkanWrappedHandle<V, T> w)
{
  return w.value == VK_NULL_HANDLE;
}
// all handles can be cast to uint64_t, needed for debug layer
template <typename V, typename T>
inline VulkanHandle generalize(VulkanWrappedHandle<V, T> w)
{
  return (VulkanHandle)w.value;
}


// this crap is needed, as in 32bit build all DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE will be
// uint64_t and will break overloading, because then there are multiple uint64_t variants,
// on 64 bit build this will just work, as they are pointers to opaque structs.
#define DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(Name, vk_name) \
  struct Name##Tag                                           \
  {};                                                        \
  typedef VulkanWrappedHandle<vk_name, Name##Tag> Name

#define DEFINE_VULKAN_HANDLE(Name, vk_name) \
  struct Name##Tag                          \
  {};                                       \
  typedef VulkanWrappedHandle<vk_name, Name##Tag> Name

DEFINE_VULKAN_HANDLE(VulkanInstanceHandle, VkInstance);
DEFINE_VULKAN_HANDLE(VulkanPhysicalDeviceHandle, VkPhysicalDevice);
DEFINE_VULKAN_HANDLE(VulkanDeviceHandle, VkDevice);
DEFINE_VULKAN_HANDLE(VulkanQueueHandle, VkQueue);
DEFINE_VULKAN_HANDLE(VulkanCommandBufferHandle, VkCommandBuffer);

DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanSemaphoreHandle, VkSemaphore);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanFenceHandle, VkFence);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDeviceMemoryHandle, VkDeviceMemory);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanBufferHandle, VkBuffer);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanImageHandle, VkImage);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanEventHandle, VkEvent);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanQueryPoolHandle, VkQueryPool);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanBufferViewHandle, VkBufferView);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanImageViewHandle, VkImageView);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanShaderModuleHandle, VkShaderModule);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanPipelineCacheHandle, VkPipelineCache);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanPipelineLayoutHandle, VkPipelineLayout);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanRenderPassHandle, VkRenderPass);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanPipelineHandle, VkPipeline);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDescriptorSetLayoutHandle, VkDescriptorSetLayout);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanSamplerHandle, VkSampler);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDescriptorPoolHandle, VkDescriptorPool);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDescriptorSetHandle, VkDescriptorSet);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanFramebufferHandle, VkFramebuffer);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanCommandPoolHandle, VkCommandPool);
