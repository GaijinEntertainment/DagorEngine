// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_critSec.h>

#include "vulkan_device.h"
#include "driver.h"

namespace drv3d_vulkan
{

struct FrameInfo;

class DeviceQueue
{
  WinCritSec mutex;
  VulkanQueueHandle handle;
  uint32_t family = -1;
  uint32_t index = -1;
  uint32_t timestampBits = 0;

  // SOA for vulkan API convinience
  struct SubmitWaits
  {
    enum
    {
      FLAG_NONE = 0,
      FLAG_TIMELINE = 1,
      FLAG_EXTERN = 2
    };

    Tab<VkPipelineStageFlags> locations;
    Tab<VulkanSemaphoreHandle> semaphores;
    Tab<int> flags;
    Tab<uint64_t> timelineValues;

    void clear()
    {
      locations.clear();
      semaphores.clear();
      flags.clear();
      timelineValues.clear();
    }
  } waits;

  VulkanSemaphoreHandle timelineSemaphore;
  uint64_t timelineValue;
  // updated when submit happens with valid fence
  // used to control overlap of workload on other queues compared to one that is fenced
  // because fenced submit controls total "frame" of work on multiple queues
  uint64_t lastFencedTimelineValue;
  uint64_t prevLastFencedTimelineValue;


public:
  struct TrimmedSubmitInfo
  {
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore *pSignalSemaphores;
  };

  struct TrimmedPresentInfo
  {
    uint32_t swapchainCount;
    const VkSwapchainKHR *pSwapchains;
    const uint32_t *pImageIndices;
    VulkanSemaphoreHandle frameReady;
  };

  struct TimelineInfo
  {
    VulkanSemaphoreHandle sem;
    uint64_t value;
  };

  DeviceQueue() = delete;
  ~DeviceQueue();

  DeviceQueue(const DeviceQueue &) = delete;
  DeviceQueue &operator=(const DeviceQueue &) = delete;

  DeviceQueue(DeviceQueue &&) = delete;
  DeviceQueue &operator=(DeviceQueue &&) = delete;

  DeviceQueue(VulkanQueueHandle h, uint32_t fam, uint32_t idx, uint32_t timestamp_bits);

  inline uint32_t getTimestampBits() const { return timestampBits; }

  inline uint32_t getFamily() const { return family; }

  inline uint32_t getIndex() const { return index; }

  void consumeWaitSemaphores(FrameInfo &gpu_frame);
  VkResult present(const TrimmedPresentInfo &presentInfo);
  void submit(FrameInfo &gpu_frame, const TrimmedSubmitInfo &trimmed_info, VulkanFenceHandle fence = VulkanFenceHandle());

  inline VulkanQueueHandle getHandle() const { return handle; }

  TimelineInfo getTimeline() { return {timelineSemaphore, timelineValue}; }
  TimelineInfo getLastFencedTimeline() { return {timelineSemaphore, lastFencedTimelineValue}; }
  TimelineInfo getPrevLastFencedTimeline() { return {timelineSemaphore, prevLastFencedTimelineValue}; }

  void waitExternSemaphore(VulkanSemaphoreHandle sem_handle, VkPipelineStageFlags sem_location)
  {
    waits.flags.push_back(SubmitWaits::FLAG_EXTERN);
    waits.timelineValues.push_back(0);
    waits.semaphores.push_back(sem_handle);
    waits.locations.push_back(sem_location);
  }

  void waitSemaphore(VulkanSemaphoreHandle sem_handle, VkPipelineStageFlags sem_location)
  {
    waits.flags.push_back(SubmitWaits::FLAG_NONE);
    waits.timelineValues.push_back(0);
    waits.semaphores.push_back(sem_handle);
    waits.locations.push_back(sem_location);
  }

  void waitTimeline(TimelineInfo tl, VkPipelineStageFlags sem_location)
  {
    waits.flags.push_back(SubmitWaits::FLAG_TIMELINE);
    waits.timelineValues.push_back(tl.value);
    waits.semaphores.push_back(tl.sem);
    waits.locations.push_back(sem_location);
  }

  void bindSparse(FrameInfo &gpu_frame, uint32_t opaq_img_bind_count, const VkSparseImageOpaqueMemoryBindInfo *opaq_img_binds,
    uint32_t img_bind_count, const VkSparseImageMemoryBindInfo *img_binds);
};

inline bool needs_sync(const DeviceQueue &l, const DeviceQueue &r) { return &l != &r; }

inline bool needs_transfer(const DeviceQueue &l, const DeviceQueue &r) { return l.getFamily() != r.getFamily(); }

class DeviceQueueGroup
{
  carray<DeviceQueue *, uint32_t(DeviceQueueType::COUNT)> group;
  bool computeCanPresent;

  void tidy();

public:
  DeviceQueueGroup() = default;
  ~DeviceQueueGroup();

  DeviceQueueGroup(const DeviceQueueGroup &) = delete;
  DeviceQueueGroup &operator=(const DeviceQueueGroup &) = delete;

  DeviceQueueGroup(DeviceQueueGroup &&o);
  DeviceQueueGroup &operator=(DeviceQueueGroup &&o);

  struct Info
  {
    struct Queue
    {
      uint32_t family;
      uint32_t index;
      uint32_t timestampBits;
    };
    carray<Queue, uint32_t(DeviceQueueType::COUNT)> group;
    bool computeCanPresent;
    bool supportsSparseBind;
    bool init(VulkanInstance &instance, VulkanPhysicalDeviceHandle device, VulkanSurfaceKHRHandle display_surface,
      const dag::Vector<VkQueueFamilyProperties> &info);
  };
  DeviceQueueGroup(const Info &info);
  inline DeviceQueue &operator[](DeviceQueueType type) { return *group[static_cast<uint32_t>(type)]; }
  inline const DeviceQueue &operator[](DeviceQueueType type) const { return *group[static_cast<uint32_t>(type)]; }
  void consumeWaitSemaphores(FrameInfo &gpu_frame)
  {
    for (DeviceQueue *i : group)
      i->consumeWaitSemaphores(gpu_frame);
  }
  void shutdown() { tidy(); }

  bool isOverlapToPrevGPUWorkAllowed(DeviceQueueType type) { return type == DeviceQueueType::TRANSFER_UPLOAD; }
  // can be also named isOverlapToNextGPUWorkAllowed, but logic supports only one queue for now,
  // so use specific name to not confuse anyone
  bool isAsyncReadback(DeviceQueueType type) { return type == DeviceQueueType::TRANSFER_READBACK; }
};

inline bool operator==(DeviceQueueGroup::Info::Queue l, DeviceQueueGroup::Info::Queue r)
{
  return l.family == r.family && l.index == r.index;
}

StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> build_queue_create_info(const DeviceQueueGroup::Info &info,
  const float *prioset);
} // namespace drv3d_vulkan