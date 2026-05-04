// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// gpu work item aka frame info

#include <perfMon/dag_sleepPrecise.h>
#include "buffer_resource.h"
#include "driver.h"
#include "fence_manager.h"
#include "image_resource.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include "pipeline.h"
#include "query_pools.h"
#include "async_completion_state.h"
#include "cleanup_queue.h"
#include "queries.h"
#include "device_execution_tracker.h"
#include "device_queue.h"

namespace drv3d_vulkan
{

struct FrameInfo
{
  CleanupQueue cleanups;

  struct QueueCommandBuffers
  {
    DeviceQueueType queue;
    VulkanCommandPoolHandle commandPool;
    Tab<VulkanCommandBufferHandle> freeCommandBuffers;
    Tab<VulkanCommandBufferHandle> pendingCommandBuffers;
    int64_t nextMemoryReleaseTime;

    void init(DeviceQueueType target_queue);
    VulkanCommandBufferHandle allocateCommandBuffer();
    void shutdown();
    void finishCmdBuffers();
  };

  struct CommandBuffersGroup
  {
    carray<QueueCommandBuffers, uint32_t(DeviceQueueType::COUNT)> group;
    void init();
    void shutdown();
    void finishCmdBuffers();
    inline QueueCommandBuffers &operator[](DeviceQueueType type) { return group[static_cast<uint32_t>(type)]; }
  };
  CommandBuffersGroup commandBuffers = {};

  ThreadedFence *frameDone = nullptr;
  ThreadedFence *readbackDone = nullptr;
  Tab<VulkanSemaphoreHandle> readySemaphores;
  int pendingSemaphoresRingIdx = 0;
  Tab<VulkanSemaphoreHandle> pendingSemaphores[GPU_TIMELINE_HISTORY_SIZE];
  Tab<VulkanCommandBufferHandle> pendingDmaBuffers;
  QueryBlock *pendingTimestamps = nullptr;
  QueryBlock *pendingOcclusionQueries = nullptr;
  dag::Vector<eastl::unique_ptr<ShaderModule>> deletedShaderModules;
  DeviceExecutionTracker execTrackers[REPLAY_TIMELINE_HISTORY_SIZE];
  int execTrackerId = 0;
  DeviceExecutionTracker &execTracker() { return execTrackers[execTrackerId]; }
  const DeviceExecutionTracker &execTracker() const { return execTrackers[execTrackerId]; }
  Tab<VulkanEventHandle> gpuEvents;

  PreciseSleepContext preciseSleepContext = {};
  int64_t nextVsyncTimeRef = 0;
  // streamline + FIFO swapchain is broken either by presenting frames in bad order
  // or randomly freezing up for some time amount
  // but that only happens if we limit frame rate by natural waits in present or gpu wait
  // limiting frame rate to vsync refresh rate fixes this issue, so custom made frame limit
  // workaround is implemented in this method
  void nvStreamlineVsyncWait();

  void init();
  VulkanEventHandle allocateGpuEvent();
  VulkanCommandBufferHandle allocateCommandBuffer(DeviceQueueType queue);
  VulkanSemaphoreHandle allocSemaphore();
  void recycleSemaphore(VulkanSemaphoreHandle sem);
  void addPendingSemaphore(VulkanSemaphoreHandle sem);

  void finishShaderModules();
  void finishDescriptorSetReset();
  void finishSemaphores();
  void finishGpuWork();
  void finishCleanups();
  void finishGpuEvents();

  // internal state

  size_t replayId = -1;
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