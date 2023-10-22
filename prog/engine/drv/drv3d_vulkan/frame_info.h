// gpu work item aka frame info
#pragma once
#include "buffer_resource.h"
#include "driver.h"
#include "fence_manager.h"
#include "image_resource.h"
#if D3D_HAS_RAY_TRACING
#include "raytrace_as_resource.h"
#endif
#include "pipeline.h"
#include "query_pools.h"
#include "async_completion_state.h"
#include "cleanup_queue.h"

namespace drv3d_vulkan
{

struct FrameInfo
{
  Tab<CleanupQueue *> cleanupsRefs;
  CleanupQueue cleanups;

  VulkanCommandPoolHandle commandPool;
  Tab<VulkanCommandBufferHandle> freeCommandBuffers;
  Tab<VulkanCommandBufferHandle> pendingCommandBuffers;
  ThreadedFence *frameDone = nullptr;
  Tab<VulkanSemaphoreHandle> readySemaphores;
  Tab<VulkanSemaphoreHandle> pendingSemaphores;
  Tab<VulkanCommandBufferHandle> pendingDmaBuffers;
  eastl::vector<TimestampQueryRef> pendingTimestamps;
  eastl::vector<AsyncCompletionState *> signalRecivers;
  eastl::vector<eastl::unique_ptr<ShaderModule>> deletedShaderModules;

  void sendSignal();
  void addSignalReciver(AsyncCompletionState &reciver);

  void init();
  VulkanCommandBufferHandle allocateCommandBuffer(VulkanDevice &device);
  VulkanSemaphoreHandle allocSemaphore(VulkanDevice &device, bool auto_track);

  void finishShaderModules();
  void finishTimeStamps();
  void finishDescriptorSetReset();
  void finishSemaphores();
  void finishGpuWork();
  void finishCmdBuffers();
  void finishCleanups();

  // internal state

  size_t index = -1;
  bool initialized = false;

  // timeline methods

  void submit();
  void acquire(size_t timeline_abs_idx);
  void wait();
  void cleanup();
  void process();
  void shutdown();
};

} // namespace drv3d_vulkan