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

  Tab<VulkanSemaphoreHandle> submitSemaphores;
  Tab<VkPipelineStageFlags> submitSemaphoresLocation;

  void clearSubmitSemaphores()
  {
    submitSemaphores.clear();
    submitSemaphoresLocation.clear();
  }

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
    VulkanSemaphoreHandle presentSignal;
  };

  DeviceQueue() = delete;
  ~DeviceQueue() = default;

  DeviceQueue(const DeviceQueue &) = delete;
  DeviceQueue &operator=(const DeviceQueue &) = delete;

  DeviceQueue(DeviceQueue &&) = delete;
  DeviceQueue &operator=(DeviceQueue &&) = delete;

  inline DeviceQueue(VulkanQueueHandle h, uint32_t fam, uint32_t idx, uint32_t timestamp_bits) :
    handle(h), family(fam), index(idx), timestampBits(timestamp_bits)
  {}

  inline uint32_t getTimestampBits() const { return timestampBits; }

  inline uint32_t getFamily() const { return family; }

  inline uint32_t getIndex() const { return index; }

  void consumeWaitSemaphores(FrameInfo &gpu_frame);
  VkResult present(VulkanDevice &device, FrameInfo &gpu_frame, const TrimmedPresentInfo &presentInfo);
  void submit(VulkanDevice &device, FrameInfo &gpu_frame, const TrimmedSubmitInfo &trimmed_info,
    VulkanFenceHandle fence = VulkanFenceHandle());

  inline VulkanQueueHandle getHandle() const { return handle; }

  void addSubmitSemaphore(VulkanSemaphoreHandle sem_handle, VkPipelineStageFlags sem_location)
  {
    submitSemaphores.push_back(sem_handle);
    submitSemaphoresLocation.push_back(sem_location);
  }
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
    bool init(VulkanInstance &instance, VulkanPhysicalDeviceHandle device, VulkanSurfaceKHRHandle display_surface,
      const eastl::vector<VkQueueFamilyProperties> &info);
  };
  DeviceQueueGroup(VulkanDevice &device, const Info &info);
  inline DeviceQueue &operator[](DeviceQueueType type) { return *group[static_cast<uint32_t>(type)]; }
  inline const DeviceQueue &operator[](DeviceQueueType type) const { return *group[static_cast<uint32_t>(type)]; }
  void consumeWaitSemaphores(FrameInfo &gpu_frame)
  {
    for (DeviceQueue *i : group)
      i->consumeWaitSemaphores(gpu_frame);
  }
};

inline bool operator==(DeviceQueueGroup::Info::Queue l, DeviceQueueGroup::Info::Queue r)
{
  return l.family == r.family && l.index == r.index;
}

StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> build_queue_create_info(const DeviceQueueGroup::Info &info,
  const float *prioset);
} // namespace drv3d_vulkan