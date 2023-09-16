#pragma once
#ifndef __DRV_VULKAN_DRIVER_H__
#define __DRV_VULKAN_DRIVER_H__

#include "vulkan_api.h"
#include "driver_defs.h"

#if _TARGET_PC_WIN
#include <windows.h>
#else
#include <stdlib.h>
#include <pthread.h>
#include <util/dag_stdint.h>
inline intptr_t GetCurrentThreadId() { return (intptr_t)pthread_self(); }
#endif

#include <EASTL/vector.h>

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <3d/dag_drv3d.h>
#include <util/dag_stdint.h>
#include <spirv/compiled_meta_data.h>
#include <perfMon/dag_graphStat.h>
#include <util/dag_hash.h>

#include <atomic>

#include "util/bits.h"
#include "util/masked_slice.h"
#include "util/value_range.h"
#include "util/range_iterators.h"
#include "util/variant_vector.h"
#include "util/scoped_timer.h"
#include "extension_utils.h"
#include "shader_stage_traits.h"
#include "drv_log_defs.h"

#ifndef UINT64_MAX
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFull
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFFul
#endif

#if _TARGET_64BIT
#define PTR_LIKE_HEX_FMT "%016X"
#else
#define PTR_LIKE_HEX_FMT "%08X"
#endif

#define VERIFY_GLOBAL_LOCK_ACQUIRED() G_ASSERT(drv3d_vulkan::is_global_lock_acquired())

// There is no reason not to enable it. For all current games, there are Swappy libraries,
// and future games are expected to use newer API and NDK levels.
// It is disableable here for debugging purposes.
#if _TARGET_ANDROID
// disabled due to wrong limiting (possible conflicting loopers) and followup problems with ANRs
#define ENABLE_SWAPPY 0
#else
#define ENABLE_SWAPPY 0
#endif

namespace drv3d_vulkan
{
// global facade
class Device;
Device &get_device();
void on_front_flush();
bool is_global_lock_acquired();
} // namespace drv3d_vulkan

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

#define VK_LOADER_ENTRY_POINT_LIST                              \
  VK_DEFINE_ENTRY_POINT(vkCreateInstance)                       \
  VK_DEFINE_ENTRY_POINT(vkGetInstanceProcAddr)                  \
  VK_DEFINE_ENTRY_POINT(vkEnumerateInstanceExtensionProperties) \
  VK_DEFINE_ENTRY_POINT(vkEnumerateInstanceLayerProperties)     \
  VK_DEFINE_ENTRY_POINT(vkGetDeviceProcAddr)

#define VK_INSTANCE_ENTRY_POINT_LIST                              \
  VK_DEFINE_ENTRY_POINT(vkEnumeratePhysicalDevices)               \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceFeatures)              \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceFormatProperties)      \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceImageFormatProperties) \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceProperties)            \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceQueueFamilyProperties) \
  VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceMemoryProperties)      \
  VK_DEFINE_ENTRY_POINT(vkEnumerateDeviceExtensionProperties)     \
  VK_DEFINE_ENTRY_POINT(vkEnumerateDeviceLayerProperties)         \
  VK_DEFINE_ENTRY_POINT(vkCreateDevice)                           \
  VK_DEFINE_ENTRY_POINT(vkDestroyInstance)

/*
VK_DEFINE_ENTRY_POINT(vkGetPhysicalDeviceSparseImageFormatProperties) \
*/

#define VK_DEVICE_ENTRY_POINT_LIST                        \
  VK_DEFINE_ENTRY_POINT(vkDestroyDevice)                  \
  VK_DEFINE_ENTRY_POINT(vkGetDeviceQueue)                 \
  VK_DEFINE_ENTRY_POINT(vkQueueSubmit)                    \
  VK_DEFINE_ENTRY_POINT(vkQueueWaitIdle)                  \
  VK_DEFINE_ENTRY_POINT(vkDeviceWaitIdle)                 \
  VK_DEFINE_ENTRY_POINT(vkAllocateMemory)                 \
  VK_DEFINE_ENTRY_POINT(vkFreeMemory)                     \
  VK_DEFINE_ENTRY_POINT(vkMapMemory)                      \
  VK_DEFINE_ENTRY_POINT(vkUnmapMemory)                    \
  VK_DEFINE_ENTRY_POINT(vkInvalidateMappedMemoryRanges)   \
  VK_DEFINE_ENTRY_POINT(vkFlushMappedMemoryRanges)        \
  VK_DEFINE_ENTRY_POINT(vkBindBufferMemory)               \
  VK_DEFINE_ENTRY_POINT(vkBindImageMemory)                \
  VK_DEFINE_ENTRY_POINT(vkGetBufferMemoryRequirements)    \
  VK_DEFINE_ENTRY_POINT(vkGetImageMemoryRequirements)     \
  VK_DEFINE_ENTRY_POINT(vkCreateFence)                    \
  VK_DEFINE_ENTRY_POINT(vkDestroyFence)                   \
  VK_DEFINE_ENTRY_POINT(vkResetFences)                    \
  VK_DEFINE_ENTRY_POINT(vkGetFenceStatus)                 \
  VK_DEFINE_ENTRY_POINT(vkWaitForFences)                  \
  VK_DEFINE_ENTRY_POINT(vkCreateSemaphore)                \
  VK_DEFINE_ENTRY_POINT(vkDestroySemaphore)               \
  VK_DEFINE_ENTRY_POINT(vkCreateQueryPool)                \
  VK_DEFINE_ENTRY_POINT(vkDestroyQueryPool)               \
  VK_DEFINE_ENTRY_POINT(vkGetQueryPoolResults)            \
  VK_DEFINE_ENTRY_POINT(vkCreateBuffer)                   \
  VK_DEFINE_ENTRY_POINT(vkDestroyBuffer)                  \
  VK_DEFINE_ENTRY_POINT(vkCreateImage)                    \
  VK_DEFINE_ENTRY_POINT(vkDestroyImage)                   \
  VK_DEFINE_ENTRY_POINT(vkCreateImageView)                \
  VK_DEFINE_ENTRY_POINT(vkDestroyImageView)               \
  VK_DEFINE_ENTRY_POINT(vkCreateShaderModule)             \
  VK_DEFINE_ENTRY_POINT(vkDestroyShaderModule)            \
  VK_DEFINE_ENTRY_POINT(vkCreatePipelineCache)            \
  VK_DEFINE_ENTRY_POINT(vkDestroyPipelineCache)           \
  VK_DEFINE_ENTRY_POINT(vkGetPipelineCacheData)           \
  VK_DEFINE_ENTRY_POINT(vkMergePipelineCaches)            \
  VK_DEFINE_ENTRY_POINT(vkCreateGraphicsPipelines)        \
  VK_DEFINE_ENTRY_POINT(vkDestroyPipeline)                \
  VK_DEFINE_ENTRY_POINT(vkCreatePipelineLayout)           \
  VK_DEFINE_ENTRY_POINT(vkDestroyPipelineLayout)          \
  VK_DEFINE_ENTRY_POINT(vkCreateSampler)                  \
  VK_DEFINE_ENTRY_POINT(vkDestroySampler)                 \
  VK_DEFINE_ENTRY_POINT(vkCreateDescriptorSetLayout)      \
  VK_DEFINE_ENTRY_POINT(vkDestroyDescriptorSetLayout)     \
  VK_DEFINE_ENTRY_POINT(vkCreateDescriptorPool)           \
  VK_DEFINE_ENTRY_POINT(vkDestroyDescriptorPool)          \
  VK_DEFINE_ENTRY_POINT(vkResetDescriptorPool)            \
  VK_DEFINE_ENTRY_POINT(vkAllocateDescriptorSets)         \
  VK_DEFINE_ENTRY_POINT(vkFreeDescriptorSets)             \
  VK_DEFINE_ENTRY_POINT(vkUpdateDescriptorSets)           \
  VK_DEFINE_ENTRY_POINT(vkCreateFramebuffer)              \
  VK_DEFINE_ENTRY_POINT(vkDestroyFramebuffer)             \
  VK_DEFINE_ENTRY_POINT(vkCreateRenderPass)               \
  VK_DEFINE_ENTRY_POINT(vkDestroyRenderPass)              \
  VK_DEFINE_ENTRY_POINT(vkCreateCommandPool)              \
  VK_DEFINE_ENTRY_POINT(vkDestroyCommandPool)             \
  VK_DEFINE_ENTRY_POINT(vkResetCommandPool)               \
  VK_DEFINE_ENTRY_POINT(vkAllocateCommandBuffers)         \
  VK_DEFINE_ENTRY_POINT(vkFreeCommandBuffers)             \
  VK_DEFINE_ENTRY_POINT(vkBeginCommandBuffer)             \
  VK_DEFINE_ENTRY_POINT(vkEndCommandBuffer)               \
  VK_DEFINE_ENTRY_POINT(vkResetCommandBuffer)             \
  VK_DEFINE_ENTRY_POINT(vkCmdBindPipeline)                \
  VK_DEFINE_ENTRY_POINT(vkCmdSetViewport)                 \
  VK_DEFINE_ENTRY_POINT(vkCmdSetScissor)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdSetLineWidth)                \
  VK_DEFINE_ENTRY_POINT(vkCmdSetDepthBias)                \
  VK_DEFINE_ENTRY_POINT(vkCmdSetBlendConstants)           \
  VK_DEFINE_ENTRY_POINT(vkCmdSetDepthBounds)              \
  VK_DEFINE_ENTRY_POINT(vkCmdSetStencilCompareMask)       \
  VK_DEFINE_ENTRY_POINT(vkCmdSetStencilWriteMask)         \
  VK_DEFINE_ENTRY_POINT(vkCmdSetStencilReference)         \
  VK_DEFINE_ENTRY_POINT(vkCmdBindDescriptorSets)          \
  VK_DEFINE_ENTRY_POINT(vkCmdBindIndexBuffer)             \
  VK_DEFINE_ENTRY_POINT(vkCmdBindVertexBuffers)           \
  VK_DEFINE_ENTRY_POINT(vkCmdDraw)                        \
  VK_DEFINE_ENTRY_POINT(vkCmdDrawIndexed)                 \
  VK_DEFINE_ENTRY_POINT(vkCmdCopyBuffer)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdCopyImage)                   \
  VK_DEFINE_ENTRY_POINT(vkCmdBlitImage)                   \
  VK_DEFINE_ENTRY_POINT(vkCmdCopyBufferToImage)           \
  VK_DEFINE_ENTRY_POINT(vkCmdCopyImageToBuffer)           \
  VK_DEFINE_ENTRY_POINT(vkCmdClearColorImage)             \
  VK_DEFINE_ENTRY_POINT(vkCmdClearDepthStencilImage)      \
  VK_DEFINE_ENTRY_POINT(vkCmdClearAttachments)            \
  VK_DEFINE_ENTRY_POINT(vkCmdPipelineBarrier)             \
  VK_DEFINE_ENTRY_POINT(vkCmdBeginQuery)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdEndQuery)                    \
  VK_DEFINE_ENTRY_POINT(vkCmdResetQueryPool)              \
  VK_DEFINE_ENTRY_POINT(vkCmdBeginRenderPass)             \
  VK_DEFINE_ENTRY_POINT(vkCmdEndRenderPass)               \
  VK_DEFINE_ENTRY_POINT(vkCreateEvent)                    \
  VK_DEFINE_ENTRY_POINT(vkResetEvent)                     \
  VK_DEFINE_ENTRY_POINT(vkSetEvent)                       \
  VK_DEFINE_ENTRY_POINT(vkGetEventStatus)                 \
  VK_DEFINE_ENTRY_POINT(vkDestroyEvent)                   \
  VK_DEFINE_ENTRY_POINT(vkCmdSetEvent)                    \
  VK_DEFINE_ENTRY_POINT(vkCmdResetEvent)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdWaitEvents)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdUpdateBuffer)                \
  VK_DEFINE_ENTRY_POINT(vkCmdDispatch)                    \
  VK_DEFINE_ENTRY_POINT(vkCreateComputePipelines)         \
  VK_DEFINE_ENTRY_POINT(vkCreateBufferView)               \
  VK_DEFINE_ENTRY_POINT(vkDestroyBufferView)              \
  VK_DEFINE_ENTRY_POINT(vkCmdDispatchIndirect)            \
  VK_DEFINE_ENTRY_POINT(vkCmdDrawIndirect)                \
  VK_DEFINE_ENTRY_POINT(vkCmdDrawIndexedIndirect)         \
  VK_DEFINE_ENTRY_POINT(vkCmdFillBuffer)                  \
  VK_DEFINE_ENTRY_POINT(vkCmdWriteTimestamp)              \
  VK_DEFINE_ENTRY_POINT(vkCmdCopyQueryPoolResults)        \
  VK_DEFINE_ENTRY_POINT(vkCmdResolveImage)                \
  VK_DEFINE_ENTRY_POINT(vkCmdNextSubpass)                 \
/*                                                        \
VK_DEFINE_ENTRY_POINT(vkQueueWaitIdle)                    \
VK_DEFINE_ENTRY_POINT(vkGetDeviceMemoryCommitment)        \
VK_DEFINE_ENTRY_POINT(vkGetImageSparseMemoryRequirements) \
VK_DEFINE_ENTRY_POINT(vkQueueBindSparse)                  \
VK_DEFINE_ENTRY_POINT(vkGetImageSubresourceLayout)        \
VK_DEFINE_ENTRY_POINT(vkGetRenderAreaGranularity)         \
VK_DEFINE_ENTRY_POINT(vkCmdPushConstants)                 \
VK_DEFINE_ENTRY_POINT(vkCmdExecuteCommands)               \
*/

// required by specification see 32.2.1. Limit Requirements
constexpr uint32_t VK_KHR_PUSH_DESCRIPTOR_MIN_DESCRIPTORS_PER_SET = 32;

#define VULKAN_RESULT_CODE_LIST                        \
  VULKAN_OK_CODE(VK_SUCCESS)                           \
  VULKAN_OK_CODE(VK_NOT_READY)                         \
  VULKAN_OK_CODE(VK_TIMEOUT)                           \
  VULKAN_OK_CODE(VK_EVENT_SET)                         \
  VULKAN_OK_CODE(VK_EVENT_RESET)                       \
  VULKAN_OK_CODE(VK_INCOMPLETE)                        \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_HOST_MEMORY)       \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_DEVICE_MEMORY)     \
  VULKAN_ERROR_CODE(VK_ERROR_INITIALIZATION_FAILED)    \
  VULKAN_ERROR_CODE(VK_ERROR_DEVICE_LOST)              \
  VULKAN_ERROR_CODE(VK_ERROR_MEMORY_MAP_FAILED)        \
  VULKAN_ERROR_CODE(VK_ERROR_LAYER_NOT_PRESENT)        \
  VULKAN_ERROR_CODE(VK_ERROR_EXTENSION_NOT_PRESENT)    \
  VULKAN_ERROR_CODE(VK_ERROR_FEATURE_NOT_PRESENT)      \
  VULKAN_ERROR_CODE(VK_ERROR_INCOMPATIBLE_DRIVER)      \
  VULKAN_ERROR_CODE(VK_ERROR_TOO_MANY_OBJECTS)         \
  VULKAN_ERROR_CODE(VK_ERROR_FORMAT_NOT_SUPPORTED)     \
  VULKAN_ERROR_CODE(VK_ERROR_SURFACE_LOST_KHR)         \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_DATE_KHR)          \
  VULKAN_ERROR_CODE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR) \
  VULKAN_ERROR_CODE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) \
  VULKAN_ERROR_CODE(VK_ERROR_VALIDATION_FAILED_EXT)    \
  VULKAN_ERROR_CODE(VK_ERROR_FRAGMENTED_POOL)          \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_POOL_MEMORY_KHR)

#define VULKAN_FEATURESET_DISABLED_LIST                           \
  VULKAN_FEATURE_DISABLE(robustBufferAccess)                      \
  VULKAN_FEATURE_DISABLE(sampleRateShading)                       \
  VULKAN_FEATURE_DISABLE(dualSrcBlend)                            \
  VULKAN_FEATURE_DISABLE(logicOp)                                 \
  VULKAN_FEATURE_DISABLE(wideLines)                               \
  VULKAN_FEATURE_DISABLE(largePoints)                             \
  VULKAN_FEATURE_DISABLE(alphaToOne)                              \
  VULKAN_FEATURE_DISABLE(multiViewport)                           \
  VULKAN_FEATURE_DISABLE(textureCompressionETC2)                  \
  VULKAN_FEATURE_DISABLE(pipelineStatisticsQuery)                 \
  VULKAN_FEATURE_DISABLE(vertexPipelineStoresAndAtomics)          \
  VULKAN_FEATURE_DISABLE(shaderTessellationAndGeometryPointSize)  \
  VULKAN_FEATURE_DISABLE(shaderImageGatherExtended)               \
  VULKAN_FEATURE_DISABLE(shaderStorageImageExtendedFormats)       \
  VULKAN_FEATURE_DISABLE(shaderStorageImageMultisample)           \
  VULKAN_FEATURE_DISABLE(shaderStorageImageReadWithoutFormat)     \
  VULKAN_FEATURE_DISABLE(shaderSampledImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderStorageBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_DISABLE(shaderStorageImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderFloat64)                           \
  VULKAN_FEATURE_DISABLE(shaderInt64)                             \
  VULKAN_FEATURE_DISABLE(shaderInt16)                             \
  VULKAN_FEATURE_DISABLE(shaderResourceResidency)                 \
  VULKAN_FEATURE_DISABLE(shaderResourceMinLod)                    \
  VULKAN_FEATURE_DISABLE(sparseBinding)                           \
  VULKAN_FEATURE_DISABLE(sparseResidencyBuffer)                   \
  VULKAN_FEATURE_DISABLE(sparseResidencyImage2D)                  \
  VULKAN_FEATURE_DISABLE(sparseResidencyImage3D)                  \
  VULKAN_FEATURE_DISABLE(sparseResidency2Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency4Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency8Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency16Samples)                \
  VULKAN_FEATURE_DISABLE(sparseResidencyAliased)                  \
  VULKAN_FEATURE_DISABLE(variableMultisampleRate)                 \
  VULKAN_FEATURE_DISABLE(inheritedQueries)                        \
  VULKAN_FEATURE_DISABLE(depthBiasClamp)                          \
  VULKAN_FEATURE_DISABLE(occlusionQueryPrecise)                   \
  VULKAN_FEATURE_DISABLE(fullDrawIndexUint32)                     \
  VULKAN_FEATURE_DISABLE(shaderUniformBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_DISABLE(shaderCullDistance)                      \
  VULKAN_FEATURE_DISABLE(shaderClipDistance)                      \
  VULKAN_FEATURE_DISABLE(shaderStorageImageWriteWithoutFormat)

#define VULKAN_FEATURESET_OPTIONAL_LIST               \
  VULKAN_FEATURE_OPTIONAL(geometryShader)             \
  VULKAN_FEATURE_OPTIONAL(textureCompressionBC)       \
  VULKAN_FEATURE_OPTIONAL(textureCompressionASTC_LDR) \
  VULKAN_FEATURE_OPTIONAL(fragmentStoresAndAtomics)   \
  VULKAN_FEATURE_OPTIONAL(fillModeNonSolid)           \
  VULKAN_FEATURE_OPTIONAL(depthBounds)                \
  VULKAN_FEATURE_OPTIONAL(samplerAnisotropy)          \
  VULKAN_FEATURE_OPTIONAL(depthClamp)                 \
  VULKAN_FEATURE_OPTIONAL(imageCubeArray)             \
  VULKAN_FEATURE_OPTIONAL(multiDrawIndirect)          \
  VULKAN_FEATURE_OPTIONAL(tessellationShader)         \
  VULKAN_FEATURE_OPTIONAL(drawIndirectFirstInstance)

// independentBlend:
// - currently adreno 4xx chips do not support this
// - Required to have different color write mask per render target.
#define VULKAN_FEATURESET_REQUIRED_LIST VULKAN_FEATURE_REQUIRE(independentBlend)

namespace drv3d_vulkan
{
inline const char *vulkan_error_string(VkResult result)
{
  switch (result)
  {
    default:
      return result > 0 ? "unknown vulkan success code" : "unknown vulkan error code";
// report success codes
#define VULKAN_OK_CODE(name) \
  case name: return #name;
// report error codes
#define VULKAN_ERROR_CODE(name) \
  case name: return #name;
      VULKAN_RESULT_CODE_LIST
#undef VULKAN_OK_CODE
#undef VULKAN_ERROR_CODE
  }
}
void set_last_error(VkResult error);
inline VkResult vulkan_check_result(VkResult result, const char *DAGOR_HAS_LOGS(line), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(lno))
{
  set_last_error(result);
  switch (result)
  {
    default:
      debug("%s returned unknown return code %u, %s %u", line, result, file, lno);
      break;
// report success codes
#define VULKAN_OK_CODE(name) \
  case name: break;
// report error codes
#define VULKAN_ERROR_CODE(name) \
  case name: /*logerr*/ debug("%s returned %s, %s %u", line, #name, file, lno); break;
      VULKAN_RESULT_CODE_LIST
#undef VULKAN_OK_CODE
#undef VULKAN_ERROR_CODE
  }
  // G_ASSERT(result >= 0);
  return result;
}

void generateFaultReport();

#if 0
  //DAGOR_DBGLEVEL > 1
template <typename T> inline T log_and_return_internal(T c, const char *cstr)
{
  debug("%u %s %s:%u", GetCurrentThreadId(), cstr, __FILE__, __LINE__);
  return c;
}
#define VULKAN_LOG_CALL(c)                                              \
  do                                                                    \
  {                                                                     \
    debug("%u %s %s:%u", GetCurrentThreadId(), #c, __FILE__, __LINE__); \
    c;                                                                  \
  } while (0);
#define VULKAN_LOG_CALL_R(c)   log_and_return_internal(c, #c)
#define VULKAN_CHECK_RESULT(r) drv3d_vulkan::vulkan_check_result(VULKAN_LOG_CALL_R(r), #r, __FILE__, __LINE__)
#else
#define VULKAN_LOG_CALL(c)     c
#define VULKAN_LOG_CALL_R(c)   c
#define VULKAN_CHECK_RESULT(r) drv3d_vulkan::vulkan_check_result(VULKAN_LOG_CALL_R(r), #r, __FILE__, __LINE__)
#endif

#define VULKAN_OK(r)         ((r) >= 0)
#define VULKAN_FAIL(r)       ((r) < 0)
#define VULKAN_CHECK_OK(r)   VULKAN_OK(VULKAN_CHECK_RESULT(r))
#define VULKAN_CHECK_FAIL(r) VULKAN_FAIL(VULKAN_CHECK_RESULT(r))
#define VULKAN_EXIT_ON_FAIL(r) \
  if (VULKAN_CHECK_FAIL(r))    \
  {                            \
    generateFaultReport();     \
    fatal(#r " failed");       \
  }

// borrowed from ps4 backend
enum D3DFORMAT
{
  D3DFMT_UNKNOWN = 0,

  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_X8B8G8R8 = 33,
  D3DFMT_G16R16 = 34,
  D3DFMT_A2R10G10B10 = 35,
  D3DFMT_A16B16G16R16 = 36,

  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_L16 = 81,

  D3DFMT_V8U8 = 60,
  D3DFMT_V16U16 = 64,

  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,

  D3DFMT_DXT1 = MAKE4C('D', 'X', 'T', '1'),
  D3DFMT_DXT2 = MAKE4C('D', 'X', 'T', '2'),
  D3DFMT_DXT3 = MAKE4C('D', 'X', 'T', '3'),
  D3DFMT_DXT4 = MAKE4C('D', 'X', 'T', '4'),
  D3DFMT_DXT5 = MAKE4C('D', 'X', 'T', '5'),
};

inline VkSamplerAddressMode translate_texture_address_mode_to_vulkan(int mode) { return (VkSamplerAddressMode)(mode - 1); }

inline VkCompareOp translate_compare_func_to_vulkan(int mode) { return (VkCompareOp)(mode - 1); }

inline uint32_t translate_compare_func_from_vulkan(VkCompareOp op) { return ((uint32_t)op) + 1; }

inline VkStencilOp translate_stencil_op_to_vulkan(uint32_t so) { return (VkStencilOp)(so - 1); }

inline VkBlendFactor translate_rgb_blend_mode_to_vulkan(uint32_t bm)
{
  static const VkBlendFactor table[] = {
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_SRC_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_FACTOR_DST_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    VK_BLEND_FACTOR_DST_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    VK_BLEND_FACTOR_ZERO, // NOT DEFINED
    VK_BLEND_FACTOR_ZERO, // NOT DEFINED
    VK_BLEND_FACTOR_CONSTANT_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    VK_BLEND_FACTOR_SRC1_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
    VK_BLEND_FACTOR_SRC1_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
  };
  return table[bm];
}

inline VkBlendFactor translate_alpha_blend_mode_to_vulkan(uint32_t bm)
{
  static const VkBlendFactor table[] = {
    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_SRC_ALPHA,           // VK_BLEND_FACTOR_SRC_COLOR   = 3,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR   = 4,
    VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    VK_BLEND_FACTOR_DST_ALPHA,           // VK_BLEND_FACTOR_DST_COLOR  = 9,
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR  = 10,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    VK_BLEND_FACTOR_ZERO,                     // NOT DEFINED
    VK_BLEND_FACTOR_ZERO,                     // NOT DEFINED
    VK_BLEND_FACTOR_CONSTANT_ALPHA,           // VK_BLEND_FACTOR_CONSTANT_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR
    VK_BLEND_FACTOR_SRC1_ALPHA,               // VK_BLEND_FACTOR_SRC1_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,     // VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR ,
    VK_BLEND_FACTOR_SRC1_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA // 19
  };
  return table[bm];
}

inline VkBlendOp translate_blend_op_to_vulkan(uint32_t bo) { return (VkBlendOp)(bo - 1); }

inline VkPrimitiveTopology translate_primitive_topology_to_vulkan(uint32_t top)
{
  if (top == PRIM_4_CONTROL_POINTS)
    return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

  return (VkPrimitiveTopology)(top - 1);
}

inline unsigned translate_primitive_topology_mask_to_vulkan(unsigned mask) { return mask >> 1u; }
} // namespace drv3d_vulkan

namespace drv3d_vulkan
{
struct InputLayout;
struct ShaderInfo;
struct Program;

enum class ActiveExecutionStage
{
  FRAME_BEGIN,
  GRAPHICS,
  COMPUTE,
  CUSTOM,
#if D3D_HAS_RAY_TRACING
  RAYTRACE,
#endif
};

#if D3D_HAS_RAY_TRACING
#define STAGE_MAX_ACTIVE STAGE_MAX_EXT
#else
#define STAGE_MAX_ACTIVE STAGE_MAX
#endif

// stores formats and offers some utility members
BEGIN_BITFIELD_TYPE(FormatStore, uint8_t)
  enum
  {
    CREATE_FLAGS_FORMAT_SHIFT = 24,
    // one additional entry for the default
    MAX_FORMAT_COUNT = (TEXFMT_LAST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) - (TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT) + 1 + 1,
    BITS = BitsNeeded<MAX_FORMAT_COUNT>::VALUE
  };
  ADD_BITFIELD_MEMBER(isSrgb, BITS, 1)
  ADD_BITFIELD_MEMBER(linearFormat, 0, BITS)
  ADD_BITFIELD_MEMBER(index, 0, BITS + 1)

  BEGIN_BITFIELD_TYPE(CreateFlagFormat, uint32_t)
    ADD_BITFIELD_MEMBER(format, 24, 8)
    ADD_BITFIELD_MEMBER(srgbRead, 22, 1)
    ADD_BITFIELD_MEMBER(srgbWrite, 21, 1)
  END_BITFIELD_TYPE()

  FormatStore(VkFormat fmt) = delete;
  // only valid to call if fmt was tested with canBeStored
  static FormatStore fromVkFormat(VkFormat fmt);
  static FormatStore fromCreateFlags(uint32_t cflg)
  {
    FormatStore fmt;
    fmt.setFromFlagTexFlags(cflg);
    return fmt;
  }
  VkFormat asVkFormat() const;
  VkFormat getLinearFormat() const;
  VkFormat getSRGBFormat() const;
  bool isSrgbCapableFormatType() const;
  FormatStore getLinearVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 0;
    return r;
  }
  FormatStore getSRGBVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 1;
    return r;
  }
  void setFromFlagTexFlags(uint32_t flags)
  {
    CreateFlagFormat fmt(flags);
    linearFormat = fmt.format;
    isSrgb = fmt.srgbRead | fmt.srgbWrite;
  }
  uint32_t asTexFlags() const
  {
    CreateFlagFormat fmt;
    fmt.format = linearFormat;
    fmt.srgbRead = isSrgb;
    fmt.srgbWrite = isSrgb;
    return fmt.wrapper.value;
  }
  uint32_t getFormatFlags() { return linearFormat << CREATE_FLAGS_FORMAT_SHIFT; }
  VkImageAspectFlags getAspektFlags() const
  {
    uint8_t val = linearFormat;
    if (val == (TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT) || val == (TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT))
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr auto range = make_value_range(TEXFMT_FIRST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT,
      (TEXFMT_LAST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) - (TEXFMT_FIRST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) + 1);

    if (range.isInside(val))
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    return VK_IMAGE_ASPECT_COLOR_BIT;
  }
  uint32_t calculateRowPitch(int32_t w) const
  {
    uint32_t blockSizeX;
    uint32_t blockSize = getBytesPerPixelBlock(&blockSizeX);
    return blockSize * ((w + blockSizeX - 1) / blockSizeX);
  }
  uint32_t calculateSlicePich(int32_t w, int32_t h) const
  {
    uint32_t blockSizeX, blockSizeY;
    uint32_t blockSize = getBytesPerPixelBlock(&blockSizeX, &blockSizeY);
    return blockSize * ((h + blockSizeY - 1) / blockSizeY) * ((w + blockSizeX - 1) / blockSizeX);
  }
  uint32_t calculateImageSize(int32_t w, int32_t h, int32_t d, int32_t levels) const
  {
    uint32_t total = 0;
    uint32_t bx, by;
    uint32_t bpp = getBytesPerPixelBlock(&bx, &by);
    for (int32_t l = 0; l < levels; ++l)
    {
      uint32_t wx = (max((w >> l), 1) + bx - 1) / bx;
      uint32_t wy = (max((h >> l), 1) + by - 1) / by;
      uint32_t wz = max((d >> l), 1);

      total += bpp * wx * wy * wz;
    }
    return total;
  }
  bool isSampledAsFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_V16U16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A8L8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    }
    return false;
  }
  bool isFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
      default: return false;
    }
  }
  bool isBlockFormat() const;
  uint32_t getBytesPerPixelBlock(uint32_t *block_x = NULL, uint32_t *block_y = NULL) const;
  const char *getNameString() const;
  // returns true if the format can be represented with this storage
  static bool canBeStored(VkFormat fmt);
  static void setRemapDepth24BitsToDepth32Bits(bool enable);
  static bool needMutableFormatForCreateFlags(uint32_t cf)
  {
    const uint32_t srgbrw = TEXCF_SRGBWRITE | TEXCF_SRGBREAD;
    // only if some of 2 bits are set means that for r/w we need to use different view format
    uint32_t anyBit = cf & srgbrw;
    if (!anyBit)
      return false;
    return (anyBit ^ srgbrw) != 0;
  }
END_BITFIELD_TYPE()
inline bool operator==(FormatStore l, FormatStore r) { return l.index == r.index; }
inline bool operator!=(FormatStore l, FormatStore r) { return l.index != r.index; }
// packs a complete sampler state into 64 bits
BEGIN_BITFIELD_TYPE(SamplerState, uint64_t)
  enum
  {
    BIAS_BITS = 32,
    BIAS_OFFSET = 0,
    MIP_BITS = 1,
    MIP_SHIFT = BIAS_OFFSET + BIAS_BITS,
    FILTER_BITS = 1,
    FILTER_SHIFT = MIP_SHIFT + MIP_BITS,
    ADDRESS_BITS = 3,
    ADDRESS_MASK = (1 << ADDRESS_BITS) - 1,
    COORD_COUNT = 3,
    COORD_BITS = 3,
    COORD_SHIFT = FILTER_SHIFT + FILTER_BITS,
    ANISO_BITS = 3,
    ANISO_SHIFT = COORD_SHIFT + COORD_BITS * COORD_COUNT,
    BORDER_BITS = 3,
    BORDER_SHIFT = ANISO_SHIFT + ANISO_BITS,
    IS_CMP_BITS = 1,
    IS_CMP_SHIFT = BORDER_BITS + BORDER_SHIFT,
    FLOAT_EXP_BASE = 127,
    FLOAT_EXP_SHIFT = 23,
  };
  ADD_BITFIELD_MEMBER(mipMapMode, MIP_SHIFT, MIP_BITS)
  ADD_BITFIELD_MEMBER(filterMode, FILTER_SHIFT, FILTER_BITS)
  ADD_BITFIELD_MEMBER(borderColorMode, BORDER_SHIFT, BORDER_BITS)
  ADD_BITFIELD_MEMBER(anisotropicValue, ANISO_SHIFT, ANISO_BITS)
  ADD_BITFIELD_ARRAY(coordMode, COORD_SHIFT, COORD_BITS, COORD_COUNT)
  ADD_BITFIELD_MEMBER(biasBits, BIAS_OFFSET, BIAS_BITS)
  ADD_BITFIELD_MEMBER(isCompare, IS_CMP_SHIFT, IS_CMP_BITS)

  BEGIN_BITFIELD_TYPE(iee754Float, uint32_t)
    float asFloat;
    uint32_t asUint;
    int32_t asInt;
    ADD_BITFIELD_MEMBER(mantissa, 0, 23)
    ADD_BITFIELD_MEMBER(exponent, 23, 8)
    ADD_BITFIELD_MEMBER(sign, 31, 1)
  END_BITFIELD_TYPE()

  inline void setMip(VkSamplerMipmapMode mip) { mipMapMode = (uint32_t)mip; }
  inline VkSamplerMipmapMode getMip() const { return (VkSamplerMipmapMode)(uint32_t)mipMapMode; }
  inline void setFilter(VkFilter filter) { filterMode = filter; }
  inline VkFilter getFilter() const { return (VkFilter)(uint32_t)filterMode; }
  inline void setIsCompare(const bool isCompFilter) { isCompare = isCompFilter ? 1 : 0; }
  inline VkCompareOp getCompareOp() const { return isCompare ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS; }
  inline void setU(VkSamplerAddressMode u) { coordMode[0] = u; }
  inline VkSamplerAddressMode getU() const { return (VkSamplerAddressMode)(uint32_t)coordMode[0]; }
  inline void setV(VkSamplerAddressMode v) { coordMode[1] = v; }
  inline VkSamplerAddressMode getV() const { return (VkSamplerAddressMode)(uint32_t)coordMode[1]; }
  inline void setW(VkSamplerAddressMode w) { coordMode[2] = w; }
  inline VkSamplerAddressMode getW() const { return (VkSamplerAddressMode)(uint32_t)coordMode[2]; }
  inline void setBias(float b)
  {
    iee754Float f;
    f.asFloat = b;
    biasBits = f.asUint;
  }
  inline float getBias() const
  {
    iee754Float f;
    f.asUint = biasBits;
    return f.asFloat;
  }
  inline void setAniso(float a)
  {
    // some float magic, falls flat on its face if it is not ieee-754
    // extracts exponent and subtracts the base
    // clamps the result into range from  0 to 4 which represents 1,2,4,8 and 16 as floats
    // negative values are treated as positive
    // values from range 0 - 1 are rounded up
    // everything else is rounded down
    iee754Float f;
    f.asFloat = a;
    int32_t value = f.exponent - FLOAT_EXP_BASE;
    // clamp from 1 to 16
    value = clamp(value, 0, 4);
    anisotropicValue = value;
  }
  inline float getAniso() const
  {
    iee754Float f;
    f.exponent = FLOAT_EXP_BASE + anisotropicValue;
    return f.asFloat;
  }
  inline void setBorder(VkBorderColor b) { borderColorMode = b; }
  inline VkBorderColor getBorder() const { return (VkBorderColor)(uint32_t)borderColorMode; }
END_BITFIELD_TYPE()
BEGIN_BITFIELD_TYPE(ImageViewState, uint64_t)
  enum
  {
    WORD_SIZE = sizeof(uint64_t) * 8,
    SAMPLE_STENCIL_BITS = 1,
    SAMPLE_STENCIL_SHIFT = 0,
    IS_RENDER_TARGET_BITS = 1,
    IS_RENDER_TARGET_SHIFT = SAMPLE_STENCIL_BITS + SAMPLE_STENCIL_SHIFT,
    IS_CUBEMAP_BITS = 1,
    IS_CUBEMAP_SHIFT = IS_RENDER_TARGET_BITS + IS_RENDER_TARGET_SHIFT,
    IS_ARRAY_BITS = 1,
    IS_ARRAY_SHIFT = IS_CUBEMAP_BITS + IS_CUBEMAP_SHIFT,
    FORMAT_BITS = FormatStore::BITS + 1,
    FORMAT_SHIFT = IS_ARRAY_SHIFT + IS_ARRAY_BITS,
    MIPMAP_OFFSET_BITS = BitsNeeded<15>::VALUE,
    MIPMAP_OFFSET_SHIFT = FORMAT_SHIFT + FORMAT_BITS,
    MIPMAP_RANGE_OFFSET = 1,
    MIPMAP_RANGE_BITS = BitsNeeded<16 - MIPMAP_RANGE_OFFSET>::VALUE,
    MIPMAP_RANGE_SHIFT = MIPMAP_OFFSET_SHIFT + MIPMAP_OFFSET_BITS,
    // automatic assign left over space to array range def
    ARRAY_DATA_SIZE = WORD_SIZE - MIPMAP_RANGE_SHIFT - MIPMAP_RANGE_BITS,
    ARRAY_OFFSET_BITS = (ARRAY_DATA_SIZE / 2) + (ARRAY_DATA_SIZE % 2),
    ARRAY_OFFSET_SHIFT = MIPMAP_RANGE_SHIFT + MIPMAP_RANGE_BITS,
    ARRAY_RANGE_OFFSET = 1,
    ARRAY_RANGE_BITS = ARRAY_DATA_SIZE / 2,
    ARRAY_RANGE_SHIFT = (ARRAY_OFFSET_SHIFT + ARRAY_OFFSET_BITS)
  };
  ADD_BITFIELD_MEMBER(sampleStencil, SAMPLE_STENCIL_SHIFT, SAMPLE_STENCIL_BITS)
  ADD_BITFIELD_MEMBER(isRenderTarget, IS_RENDER_TARGET_SHIFT, IS_RENDER_TARGET_BITS)
  ADD_BITFIELD_MEMBER(isCubemap, IS_CUBEMAP_SHIFT, IS_CUBEMAP_BITS)
  ADD_BITFIELD_MEMBER(isArray, IS_ARRAY_SHIFT, IS_ARRAY_BITS)
  ADD_BITFIELD_MEMBER(format, FORMAT_SHIFT, FORMAT_BITS)
  ADD_BITFIELD_MEMBER(mipmapOffset, MIPMAP_OFFSET_SHIFT, MIPMAP_OFFSET_BITS);
  ADD_BITFIELD_MEMBER(mipmapRange, MIPMAP_RANGE_SHIFT, MIPMAP_RANGE_BITS)
  ADD_BITFIELD_MEMBER(arrayOffset, ARRAY_OFFSET_SHIFT, ARRAY_OFFSET_BITS)
  ADD_BITFIELD_MEMBER(arrayRange, ARRAY_RANGE_SHIFT, ARRAY_RANGE_BITS)

  inline VkImageViewType getViewType(VkImageType type) const
  {
    switch (type)
    {
      case VK_IMAGE_TYPE_1D:
        G_ASSERT(isCubemap == 0);
        G_ASSERT(isRenderTarget == 0);
        if (isArray)
          return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        G_ASSERT(getArrayCount() == 1);
        return VK_IMAGE_VIEW_TYPE_1D;
      case VK_IMAGE_TYPE_2D:
        if (isCubemap)
        {
          if (isArray)
          {
            G_ASSERT((getArrayCount() % 6) == 0);
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
          }
          else
          {
            G_ASSERT(getArrayCount() == 6);
            return VK_IMAGE_VIEW_TYPE_CUBE;
          }
        }
        if (isArray)
        {
          return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        G_ASSERT(getArrayCount() == 1);
        return VK_IMAGE_VIEW_TYPE_2D;
      case VK_IMAGE_TYPE_3D:
        G_ASSERT(isCubemap == 0);
        G_ASSERT(isRenderTarget || getArrayBase() == 0);
        if (isRenderTarget)
        {
          if (isArray)
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          else
          {
            G_ASSERT(getArrayCount() == 1);
            return VK_IMAGE_VIEW_TYPE_2D;
          }
        }
        else
        {
          G_ASSERT(getArrayCount() == 1);
          G_ASSERT(isArray == 0);
          return VK_IMAGE_VIEW_TYPE_3D;
        }
      default:
        fatal("Unexpected image type %u", type);
        return VK_IMAGE_VIEW_TYPE_2D;
        break;
    }
  }

  inline void setFormat(FormatStore fmt) { format = fmt.index; }
  inline FormatStore getFormat() const { return FormatStore(format); }
  inline void setMipBase(uint8_t u) { mipmapOffset = u; }
  inline uint8_t getMipBase() const { return mipmapOffset; }
  inline void setMipCount(uint8_t u) { mipmapRange = u - MIPMAP_RANGE_OFFSET; }
  inline uint8_t getMipCount() const { return MIPMAP_RANGE_OFFSET + mipmapRange; }
  ValueRange<uint8_t> getMipRange() const { return {getMipBase(), uint8_t(getMipBase() + getMipCount())}; }
  inline void setArrayBase(uint8_t u) { arrayOffset = u; }
  inline uint16_t getArrayBase() const { return arrayOffset; }
  inline void setArrayCount(uint16_t u) { arrayRange = u - ARRAY_RANGE_OFFSET; }
  inline uint16_t getArrayCount() const { return ARRAY_RANGE_OFFSET + (uint32_t)arrayRange; }
  ValueRange<uint16_t> getArrayRange() const { return {getArrayBase(), uint16_t(getArrayBase() + getArrayCount())}; }
END_BITFIELD_TYPE()
inline bool operator==(ImageViewState l, ImageViewState r) { return l.wrapper.value == r.wrapper.value; }
inline bool operator!=(ImageViewState l, ImageViewState r) { return l.wrapper.value != r.wrapper.value; }
inline bool operator<(ImageViewState l, ImageViewState r) { return l.wrapper.value < r.wrapper.value; }

inline uint32_t select_memory_type(const VkPhysicalDeviceMemoryProperties &mem_info, uint32_t mask, VkMemoryPropertyFlags wanted_props,
  VkMemoryPropertyFlags unwanted_props)
{
  for (uint32_t i = 0; mask && i < mem_info.memoryTypeCount; ++i)
  {
    if (!(mask & (1 << i)))
      continue;
    if ((mem_info.memoryTypes[i].propertyFlags & wanted_props) == wanted_props)
    {
      // if the caller does not want this flags to be set skip it
      if (mem_info.memoryTypes[i].propertyFlags & unwanted_props)
        continue;
      return i;
    }
  }
  // second phase where the unwanted_props is ignored in the hope to find a memory type
  // that could be used
  for (uint32_t i = 0; mask && i < mem_info.memoryTypeCount; ++i)
  {
    if (!(mask & (1 << i)))
      continue;
    if ((mem_info.memoryTypes[i].propertyFlags & wanted_props) == wanted_props)
      return i;
  }
  return UINT32_MAX;
}

template <typename H, H NullValue, typename T>
class TaggedHandle
{
  H h = H(NullValue);

public:
  inline H get() const { return h; }
  inline explicit TaggedHandle(H a) : h(a) {}
  inline TaggedHandle() {}
  inline bool operator!() const { return h == NullValue; }
  friend inline bool operator==(TaggedHandle l, TaggedHandle r) { return l.get() == r.get(); }
  friend inline bool operator!=(TaggedHandle l, TaggedHandle r) { return l.get() != r.get(); }
  friend inline H operator-(TaggedHandle l, TaggedHandle r) { return l.get() - r.get(); }
  static inline TaggedHandle Null() { return TaggedHandle(NullValue); }
};

struct ProgramIDTag
{};
struct InputLayoutIDTag
{};
struct ShaderIDTag
{};
struct BufferHandleTag
{};

typedef uint32_t ResourceMemoryId;
typedef int LinearStorageIndex;
typedef TaggedHandle<int, -1, ProgramIDTag> ProgramID;
typedef TaggedHandle<int, -1, InputLayoutIDTag> InputLayoutID;
typedef TaggedHandle<int, -1, ShaderIDTag> ShaderID;

struct StringIndexRefTag
{};
typedef TaggedHandle<size_t, ~size_t(0), StringIndexRefTag> StringIndexRef;

struct SamplerInfo
{
  // 0 normal sampler, 1 sampler with depth compare enabled
  VulkanSamplerHandle sampler[2];
  SamplerState state;

  inline VulkanSamplerHandle compareSampler() const { return sampler[1]; }
  inline VulkanSamplerHandle colorSampler() const { return sampler[0]; }
};

enum ImageState
{
  SN_INITIAL = 0,             // nothing
  SN_SAMPLED_FROM = 1,        // writes needed to be flushed
  SN_RENDER_TARGET = 2,       // writes needed to be flushed
  SN_CONST_RENDER_TARGET = 3, // writes needed to be flushed
  SN_COPY_FROM = 4,           // writes needed to be flushed
  SN_COPY_TO = 5,             // nothing
  SN_SHADER_WRITE = 6,
  SN_WRITE_CLEAR = 7,
  SN_COUNT = 8
};

class Image;

struct DriverRenderState
{
  LinearStorageIndex dynamicIdx = 0;
  LinearStorageIndex staticIdx = 0;

  bool equal(const DriverRenderState &v) const
  {
    if (v.dynamicIdx != dynamicIdx)
      return false;

    if (v.staticIdx != staticIdx)
      return false;

    return true;
  }
};
} // namespace drv3d_vulkan

template <typename C>
inline bool is_equal(const C &l, const C &r)
{
  if (l.size() == r.size())
  {
    for (uint32_t i = 0; i < l.size(); ++i)
      if (l[i] != r[i])
        return false;
    return true;
  }
  return false;
}
template <typename C>
inline bool is_equal_mem(const C &l, const C &r)
{
  if (l.size() == r.size())
    return 0 == memcmp(&l[0], &r[0], data_size(l));
  return false;
}

template <typename T>
inline typename T::reference back(T &t)
{
  return t.back();
}

inline const VkOffset3D &toOffset(const VkExtent3D &ext)
{
  // sanity checks
  G_STATIC_ASSERT(offsetof(VkExtent3D, width) == offsetof(VkOffset3D, x));
  G_STATIC_ASSERT(offsetof(VkExtent3D, height) == offsetof(VkOffset3D, y));
  G_STATIC_ASSERT(offsetof(VkExtent3D, depth) == offsetof(VkOffset3D, z));
  return reinterpret_cast<const VkOffset3D &>(ext);
}

inline bool operator==(VkExtent2D l, VkExtent2D r) { return l.height == r.height && l.width == r.width; }

inline bool operator!=(VkExtent2D l, VkExtent2D r) { return !(r == l); }

inline bool operator==(VkOffset2D l, VkOffset2D r) { return l.x == r.x && l.y == r.y; }

inline bool operator!=(VkOffset2D l, VkOffset2D r) { return !(r == l); }

inline bool operator==(const VkRect2D &l, const VkRect2D &r) { return l.extent == r.extent && l.offset == r.offset; }

inline bool operator!=(const VkRect2D &l, const VkRect2D &r) { return !(r == l); }

inline bool operator==(const VkViewport &l, const VkViewport &r)
{
  return l.height == r.height && l.maxDepth == r.maxDepth && l.minDepth == r.minDepth && l.width == r.width && l.x == r.x &&
         l.y == r.y;
}

inline bool operator!=(const VkViewport &l, const VkViewport &r) { return !(r == l); }

// chains b to a and preserves prevous next of a
#define VULKAN_CHAIN_STRUCTS(a, b) \
  do                               \
  {                                \
    b.pNext = a.pNext;             \
    a.pNext = &b;                  \
  } while (0)

inline VkImageSubresourceRange makeImageSubresourceRange(VkImageAspectFlags aspect, uint32_t base_mip, uint32_t mip_count,
  uint32_t array_index, uint32_t array_range)
{
  VkImageSubresourceRange r = {aspect, base_mip, mip_count, array_index, array_range};
  return r;
}

inline VkImageSubresourceLayers makeImageSubresourceLayers(VkImageAspectFlags aspect, uint32_t base_mip, uint32_t array_index,
  uint32_t array_range)
{
  VkImageSubresourceLayers r = {aspect, base_mip, array_index, array_range};
  return r;
}

inline VkOffset3D makeOffset3D(int32_t x, int32_t y, int32_t z)
{
  VkOffset3D r = {x, y, z};
  return r;
}

inline VkExtent2D makeExtent2D(uint32_t width, uint32_t height)
{
  VkExtent2D r = {width, height};
  return r;
}

inline VkExtent3D makeExtent3D(VkExtent2D wh, uint32_t depth)
{
  VkExtent3D r = {wh.width, wh.height, depth};
  return r;
}

inline VkExtent3D makeExtent3D(uint32_t width, uint32_t height, uint32_t depth)
{
  VkExtent3D r = {width, height, depth};
  return r;
}

inline uint32_t nextPowerOfTwo(uint32_t u)
{
  --u;
  u |= u >> 1;
  u |= u >> 2;
  u |= u >> 4;
  u |= u >> 8;
  u |= u >> 16;
  return ++u;
}

inline uint64_t nextPowerOfTwo(uint64_t u)
{
  --u;
  u |= u >> 1;
  u |= u >> 2;
  u |= u >> 4;
  u |= u >> 8;
  u |= u >> 16;
  u |= u >> 32;
  return ++u;
}

struct VkAnyDescriptorInfo
{
  union
  {
    VkDescriptorImageInfo image;
    VkDescriptorBufferInfo buffer;
    VkBufferView texelBuffer;
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
    VkAccelerationStructureKHR raytraceAccelerationStructure;
#endif
  };

  enum
  {
    TYPE_NULL = 0,
    TYPE_IMG = 1,
    TYPE_BUF = 2,
    TYPE_BUF_VIEW = 3,
    TYPE_AS = 4
  };

  uint8_t type : 3;

  VkAnyDescriptorInfo &operator=(const VkDescriptorImageInfo &i)
  {
    image = i;
    type = TYPE_IMG;
    return *this;
  };
  VkAnyDescriptorInfo &operator=(const VkDescriptorBufferInfo &i)
  {
    buffer = i;
    type = TYPE_BUF;
    return *this;
  };
  VkAnyDescriptorInfo &operator=(VulkanBufferViewHandle i)
  {
    texelBuffer = i;
    type = TYPE_BUF_VIEW;
    return *this;
  };
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  VkAnyDescriptorInfo &operator=(VkAccelerationStructureKHR as)
  {
    raytraceAccelerationStructure = as;
    type = TYPE_AS;
    return *this;
  }
#endif

  void clear() { type = TYPE_NULL; }
};

typedef VkAnyDescriptorInfo ResourceDummySet[spirv::MISSING_IS_FATAL_INDEX + 1];

static constexpr VkStencilFaceFlags VK_STENCIL_FACE_BOTH_BIT = VK_STENCIL_FACE_FRONT_BIT | VK_STENCIL_FACE_BACK_BIT;

constexpr VkColorComponentFlags VK_COLOR_COMPONENT_RGBA_BIT =
  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

class DataBlock;
namespace drv3d_vulkan
{
template <typename T, typename U>
void chain_structs(T &t, U &u)
{
  G_ASSERT(u.pNext == nullptr);
  u.pNext = (void *)t.pNext;
  t.pNext = &u;
}

inline String vulkan_version_to_string(uint32_t version)
{
  return String(64, "%d.%d.%d", version >> 22, (version >> 12) & ((1 << 10) - 1), version & ((1 << 12) - 1));
}

bool check_and_update_device_features(VkPhysicalDeviceFeatures &set);
#if VK_EXT_debug_report || VK_EXT_debug_utils
// compiles 2 ways to define a message code supression mechanism
// ignoreMessageCodes:t=<comma seperated list>
// ignoreMessageCode:i=<id>
// ignoreMessageCode can have multiple entries (currently they are merged away by other code
// so the work around with ignoreMessageCodes
Tab<int32_t> compile_supressed_message_set(const DataBlock *config);
#endif
VkApplicationInfo create_app_info(const DataBlock *cfg);
void set_app_info(const char *name, uint32_t ver);
uint32_t get_app_ver();
VkPresentModeKHR get_requested_present_mode(bool use_vsync, bool use_adaptive_vsync);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
struct ShaderDebugInfo
{
  String name;
  String spirvDisAsm;
  String dxbcDisAsm;
  String sourceGLSL;
  String reconstructedHLSL;
  String reconstructedHLSLAndSourceHLSLXDif;
  String sourceHLSL;
  // name from engine with additional usefull info
  String debugName;
};
#endif

struct ShaderModuleBlob
{
  eastl::vector<uint32_t> blob;
  spirv::HashValue hash;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String name;
#endif
  size_t getBlobSize() const { return blob.size() * sizeof(uint32_t); }

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
};

struct ShaderModuleHeader
{
  spirv::ShaderHeader header;
  spirv::HashValue hash;
  VkShaderStageFlags stage;

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
};

struct ShaderModuleUse
{
  ShaderModuleHeader header;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ShaderDebugInfo dbg;
#endif
  uint32_t blobId;
};

struct ShaderModule
{
  VulkanShaderModuleHandle module;
  spirv::HashValue hash;
  uint32_t size;
  uint32_t id;

  ShaderModule() = default;
  ~ShaderModule() = default;

  ShaderModule(const ShaderModule &) = default;
  ShaderModule &operator=(const ShaderModule &) = default;

  ShaderModule(ShaderModule &&) = default;
  ShaderModule &operator=(ShaderModule &&) = default;

  ShaderModule(VulkanShaderModuleHandle m, uint32_t i, const spirv::HashValue &hv, uint32_t sz) : module(m), id(i), hash(hv), size(sz)
  {}
};

VkBufferImageCopy make_copy_info(FormatStore format, uint32_t mip, uint32_t base_layer, uint32_t layers, VkExtent3D original_size,
  VkDeviceSize src_offset);

String formatImageUsageFlags(VkImageUsageFlags flags);
String formatPipelineStageFlags(VkPipelineStageFlags flags);
String formatMemoryAccessFlags(VkAccessFlags flags);
const char *formatImageLayout(VkImageLayout layout);
String formatMemoryTypeFlags(VkMemoryPropertyFlags propertyFlags);
const char *formatActiveExecutionStage(ActiveExecutionStage stage);
const char *formatPrimitiveTopology(VkPrimitiveTopology top);
const char *formatShaderStage(ShaderStage stage);
const char *formatObjectType(VkObjectType obj_type);
const char *formatAttachmentLoadOp(const VkAttachmentLoadOp load_op);
const char *formatAttachmentStoreOp(const VkAttachmentStoreOp store_op);

template <typename T>
static uint32_t get_binary_size_unit_prefix_index(T size)
{
  uint32_t power = 0;
  const uint32_t shift = 10;
  for (;; ++power)
    if (0 == (size / (1ull << ((power + 1) * shift))))
      break;
  return power;
}

template <typename T>
static String byte_size_unit(T size)
{
  static const char *suffix[] = {"", "Ki", "Mi", "Gi"};
  uint32_t power = get_binary_size_unit_prefix_index(size);
  return String(128, "%f %sBytes", double(size) / (1ull << (power * 10)), suffix[power]);
}

} // namespace drv3d_vulkan

#endif // __DRV_VULKAN_DRIVER_H__
