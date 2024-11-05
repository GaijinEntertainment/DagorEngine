// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// List of Vulkan functions loaded by us

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
