// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_queue.h"
#include "frame_info.h"
#include "vulkan_device.h"
#include "physical_device_set.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

namespace
{
bool can_display(VulkanInstance &instance, VulkanPhysicalDeviceHandle device, VulkanSurfaceKHRHandle display_surface, uint32_t family)
{
  VkBool32 support;
  if (VULKAN_CHECK_FAIL(instance.vkGetPhysicalDeviceSurfaceSupportKHR(device, family, display_surface, &support)))
  {
    support = VK_FALSE;
  }
  return (support == VK_TRUE);
}

const char *queue_type_to_name(DeviceQueueType type)
{
  switch (type)
  {
    case DeviceQueueType::GRAPHICS: return "graphics";
    case DeviceQueueType::COMPUTE: return "compute";
    case DeviceQueueType::TRANSFER_UPLOAD: return "transfer upload";
    case DeviceQueueType::TRANSFER_READBACK: return "transfer readback";
    case DeviceQueueType::ASYNC_GRAPHICS: return "async graphics";
    case DeviceQueueType::SPARSE_BINDING: return "sparse binding";
    default: break;
  }
  return "<error unreachable>";
}

struct FlatQueueInfo : DeviceQueueGroup::Info::Queue
{
  VkQueueFlags flags;
};

dag::Vector<FlatQueueInfo> flatten_queue_info(const dag::Vector<VkQueueFamilyProperties> &info)
{
  dag::Vector<FlatQueueInfo> result;
  FlatQueueInfo familyInfo;
  for (auto &&family : info)
  {
    familyInfo.family = &family - info.data();
    familyInfo.timestampBits = family.timestampValidBits;
    familyInfo.flags = family.queueFlags;
    if (family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
      familyInfo.flags |= VK_QUEUE_TRANSFER_BIT;
    for (familyInfo.index = 0; familyInfo.index < family.queueCount; ++familyInfo.index)
      result.push_back(familyInfo);
  }
  return result;
}

VkQueueFlags get_queue_flags_for_type(DeviceQueueType type)
{
  switch (type)
  {
    case DeviceQueueType::GRAPHICS:
    case DeviceQueueType::ASYNC_GRAPHICS: return VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    case DeviceQueueType::COMPUTE: return VK_QUEUE_COMPUTE_BIT;
    case DeviceQueueType::TRANSFER_READBACK:
    case DeviceQueueType::TRANSFER_UPLOAD: return VK_QUEUE_TRANSFER_BIT;
    case DeviceQueueType::SPARSE_BINDING: return VK_QUEUE_SPARSE_BINDING_BIT;
    default: break;
  }
  // unrechable, but make compiler happy
  return VK_QUEUE_GRAPHICS_BIT;
}

bool use_queue_for_display(DeviceQueueType type) { return type == DeviceQueueType::GRAPHICS; }

DeviceQueueType get_fallback_for(DeviceQueueType type)
{
  switch (type)
  {
    case DeviceQueueType::GRAPHICS: return DeviceQueueType::INVALID;
    case DeviceQueueType::ASYNC_GRAPHICS:
    case DeviceQueueType::SPARSE_BINDING:
    case DeviceQueueType::COMPUTE: return DeviceQueueType::GRAPHICS;
    case DeviceQueueType::TRANSFER_READBACK:
    case DeviceQueueType::TRANSFER_UPLOAD: return DeviceQueueType::COMPUTE;
    default: break;
  }
  // unrechable, but make comiler happy...
  return DeviceQueueType::INVALID;
}

dag::Vector<FlatQueueInfo>::iterator find_best_match(dag::Vector<FlatQueueInfo> &set, DeviceQueueType type, VulkanInstance &instance,
  VulkanPhysicalDeviceHandle device, VulkanSurfaceKHRHandle display_surface)
{
  const VkQueueFlags requiredFlags = get_queue_flags_for_type(type);
  const bool isUsedForDisplay = use_queue_for_display(type);
  uint32_t baseToManyBits = UINT32_MAX;
  auto base = end(set);
  for (auto at = begin(set); at != end(set); ++at)
  {
    if (requiredFlags != (at->flags & requiredFlags))
      continue;

    if (isUsedForDisplay)
      if (!can_display(instance, device, display_surface, at->family))
        continue;

    uint32_t toManyBits = count_bits(at->flags ^ requiredFlags);
    if (toManyBits < baseToManyBits)
    {
      baseToManyBits = toManyBits;
      base = at;
    }
  }
  return base;
}

bool select_queue(dag::Vector<FlatQueueInfo> &set, DeviceQueueType type, VulkanInstance &instance, VulkanPhysicalDeviceHandle device,
  VulkanSurfaceKHRHandle display_surface, FlatQueueInfo &selection)
{
  auto selected = find_best_match(set, type, instance, device, display_surface);
  if (selected != end(set))
  {
    selection = *selected;
    set.erase(selected);
    debug("vulkan: using %u-%u as %s queue", selection.family, selection.index, queue_type_to_name(type));
    return true;
  }
  debug("vulkan: no dedicated queue as %s queue found", queue_type_to_name(type));
  return false;
}
} // namespace

bool DeviceQueueGroup::Info::init(VulkanInstance &instance, VulkanPhysicalDeviceHandle device, VulkanSurfaceKHRHandle display_surface,
  const dag::Vector<VkQueueFamilyProperties> &info)
{
  auto flatQueueInfo = flatten_queue_info(info);
  carray<FlatQueueInfo, uint32_t(DeviceQueueType::COUNT)> selected;
  carray<bool, uint32_t(DeviceQueueType::COUNT)> isAvialable;
  for (uint32_t i = 0; i < uint32_t(DeviceQueueType::COUNT); ++i)
  {
    isAvialable[i] = select_queue(flatQueueInfo, static_cast<DeviceQueueType>(i), instance, device, display_surface, selected[i]);
  }

  supportsSparseBind = true;

  for (uint32_t i = 0; i < uint32_t(DeviceQueueType::COUNT); ++i)
  {
    if (isAvialable[i])
    {
      group[i].family = selected[i].family;
      group[i].index = selected[i].index;
      group[i].timestampBits = selected[i].timestampBits;
    }
    else
    {
      DeviceQueueType type = static_cast<DeviceQueueType>(i);
      for (;;)
      {
        type = get_fallback_for(type);
        if (DeviceQueueType::INVALID == type)
          return false;
        if (isAvialable[uint32_t(type)])
          break;
      }
      group[i] = group[uint32_t(type)];

      // special case for sparse fallback, because we don't want to restrict graphics queue with sparse,
      // check it once more and report
      if (type == DeviceQueueType::SPARSE_BINDING)
        supportsSparseBind = (selected[uint32_t(type)].flags & VK_QUEUE_SPARSE_BINDING_BIT) > 0;
      debug("vulkan: sharing %s queue as %s queue", queue_type_to_name(type), queue_type_to_name(static_cast<DeviceQueueType>(i)));
    }
  }

  computeCanPresent = can_display(instance, device, display_surface, group[uint32_t(DeviceQueueType::COMPUTE)].family);
  debug("vulkan: compute queue can display: %s", computeCanPresent ? "yes" : "no");

  return true;
}

void DeviceQueueGroup::tidy()
{
  for (int i = 0; i < group.size(); ++i)
  {
    if (group[i])
    {
      for (int j = i + 1; j < group.size(); ++j)
        if (group[i] == group[j])
          group[j] = nullptr;
      delete group[i];
      group[i] = nullptr;
    }
  }
}

DeviceQueueGroup::~DeviceQueueGroup() { tidy(); }

DeviceQueueGroup::DeviceQueueGroup(DeviceQueueGroup &&o) : computeCanPresent(o.computeCanPresent)
{
  mem_copy_from(group, o.group.data());
  mem_set_0(o.group);
}

DeviceQueueGroup &DeviceQueueGroup::operator=(DeviceQueueGroup &&o)
{
  tidy();
  mem_copy_from(group, o.group.data());
  mem_set_0(o.group);
  computeCanPresent = o.computeCanPresent;
  return *this;
}

DeviceQueueGroup::DeviceQueueGroup(const Info &info) : computeCanPresent(info.computeCanPresent)
{
  mem_set_0(group);
  for (int i = 0; i < info.group.size(); ++i)
  {
    if (!group[i])
    {
      VulkanQueueHandle queue;
      VULKAN_LOG_CALL(
        Globals::VK::dev.vkGetDeviceQueue(Globals::VK::dev.get(), info.group[i].family, info.group[i].index, ptr(queue)));
      group[i] = new DeviceQueue(queue, info.group[i].family, info.group[i].index, info.group[i].timestampBits);
      for (int j = i + 1; j < info.group.size(); ++j)
        if (info.group[i] == info.group[j])
          group[j] = group[i];
    }
  }
}

namespace drv3d_vulkan
{
StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> build_queue_create_info(const DeviceQueueGroup::Info &info,
  const float *prioset)
{
  StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> result;
  for (uint32_t i = 0; i < info.group.size(); ++i)
  {
    const DeviceQueueGroup::Info::Queue &q = info.group[i];
    uint32_t j = 0;
    for (; j < result.size(); ++j)
    {
      VkDeviceQueueCreateInfo &createInfo = result[j];
      if (createInfo.queueFamilyIndex == q.family)
      {
        if (createInfo.queueCount <= q.index)
          createInfo.queueCount = q.index + 1;
        break;
      }
    }
    if (j == result.size())
    {
      VkDeviceQueueCreateInfo &createInfo = result.push_back();
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      createInfo.pNext = nullptr;
      createInfo.flags = 0;
      createInfo.queueFamilyIndex = q.family;
      createInfo.queueCount = q.index + 1;
      createInfo.pQueuePriorities = prioset;
    }
  }
  return result;
}

DeviceQueue::~DeviceQueue()
{
  if (Globals::VK::phy.hasTimelineSemaphore)
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySemaphore(Globals::VK::dev.get(), timelineSemaphore, VKALLOC(semaphore)));
}

DeviceQueue::DeviceQueue(VulkanQueueHandle h, uint32_t fam, uint32_t idx, uint32_t timestamp_bits) :
  handle(h),
  family(fam),
  index(idx),
  timestampBits(timestamp_bits),
  timelineValue(0),
  lastFencedTimelineValue(0),
  prevLastFencedTimelineValue(0)
{
  if (Globals::VK::phy.hasTimelineSemaphore)
  {
    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphoreTypeCreateInfoKHR stci = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};
    stci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
    stci.initialValue = 0;
    chain_structs(sci, stci);

    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSemaphore(Globals::VK::dev.get(), &sci, VKALLOC(semaphore), ptr(timelineSemaphore)));
  }
}

void DeviceQueue::consumeWaitSemaphores(FrameInfo &gpu_frame)
{
  WinAutoLock lock{mutex};

  for (const int &i : waits.flags)
  {
    if (i == SubmitWaits::FLAG_NONE)
      gpu_frame.addPendingSemaphore(waits.semaphores[&i - waits.flags.begin()]);
  }

  waits.clear();
}

VkResult DeviceQueue::present(const TrimmedPresentInfo &presentInfo)
{
  WinAutoLock lock{mutex};

  VkResult ret = VK_SUCCESS;

  if (presentInfo.swapchainCount)
  {
    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pWaitSemaphores = ary(&presentInfo.frameReady);
    pi.waitSemaphoreCount = 1;

    pi.swapchainCount = presentInfo.swapchainCount;
    pi.pSwapchains = presentInfo.pSwapchains;
    pi.pImageIndices = presentInfo.pImageIndices;

    ret = VULKAN_LOG_CALL_R(Globals::VK::dev.vkQueuePresentKHR(handle, &pi));
  }
  else
  {
    G_ASSERT(is_null(presentInfo.frameReady));
  }

  return ret;
}

void DeviceQueue::submit(FrameInfo &gpu_frame, const TrimmedSubmitInfo &trimmed_info, VulkanFenceHandle fence)
{
  WinAutoLock lock{mutex};

  VkSubmitInfo si = {};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.pWaitSemaphores = ary(waits.semaphores.data());
  si.waitSemaphoreCount = waits.semaphores.size();
  si.pWaitDstStageMask = waits.locations.data();

  si.pCommandBuffers = trimmed_info.pCommandBuffers;
  si.commandBufferCount = trimmed_info.commandBufferCount;

  VkTimelineSemaphoreSubmitInfoKHR tssi = {};
  VkSemaphore comboSemaphores[2];
  uint64_t timelineValues[2];

  if (Globals::VK::phy.hasTimelineSemaphore)
  {
    ++timelineValue;
    timelineValues[0] = timelineValue;
    if (!is_null(fence))
    {
      prevLastFencedTimelineValue = lastFencedTimelineValue;
      lastFencedTimelineValue = timelineValue;
    }

    // only one non timeline semaphore is allowed - for swapchain logic
    G_ASSERTF(trimmed_info.signalSemaphoreCount < 2, "vulkan: too much signals (%u) on submit with timeline semaphores",
      trimmed_info.signalSemaphoreCount);

    comboSemaphores[0] = timelineSemaphore;
    if (trimmed_info.signalSemaphoreCount)
      comboSemaphores[1] = trimmed_info.pSignalSemaphores[0];
    si.pSignalSemaphores = &comboSemaphores[0];
    si.signalSemaphoreCount = trimmed_info.signalSemaphoreCount + 1;

    tssi.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
    tssi.waitSemaphoreValueCount = waits.semaphores.size();
    tssi.pWaitSemaphoreValues = waits.timelineValues.data();
    tssi.signalSemaphoreValueCount = si.signalSemaphoreCount;
    tssi.pSignalSemaphoreValues = timelineValues;
    chain_structs(si, tssi);
  }
  else
  {
    si.pSignalSemaphores = trimmed_info.pSignalSemaphores;
    si.signalSemaphoreCount = trimmed_info.signalSemaphoreCount;
  }


  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkQueueSubmit(handle, 1, &si, fence));
  consumeWaitSemaphores(gpu_frame);
}

void DeviceQueue::bindSparse(FrameInfo &gpu_frame, uint32_t opaq_img_bind_count,
  const VkSparseImageOpaqueMemoryBindInfo *opaq_img_binds, uint32_t img_bind_count, const VkSparseImageMemoryBindInfo *img_binds)
{
  WinAutoLock lock{mutex};

  VkBindSparseInfo bsi = {VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, nullptr};

  bsi.pWaitSemaphores = ary(waits.semaphores.data());
  bsi.waitSemaphoreCount = waits.semaphores.size();

  bsi.imageOpaqueBindCount = opaq_img_bind_count;
  bsi.pImageOpaqueBinds = opaq_img_binds;
  bsi.imageBindCount = img_bind_count;
  bsi.pImageBinds = img_binds;

  VkTimelineSemaphoreSubmitInfoKHR tssi = {};
  if (Globals::VK::phy.hasTimelineSemaphore)
  {
    ++timelineValue;
    bsi.pSignalSemaphores = ary(&timelineSemaphore);
    bsi.signalSemaphoreCount = 1;

    tssi.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
    tssi.waitSemaphoreValueCount = waits.semaphores.size();
    tssi.pWaitSemaphoreValues = waits.timelineValues.data();
    tssi.signalSemaphoreValueCount = 1;
    tssi.pSignalSemaphoreValues = &timelineValue;
    chain_structs(bsi, tssi);
  }

  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkQueueBindSparse(handle, 1, &bsi, VulkanFenceHandle{}));
  consumeWaitSemaphores(gpu_frame);
}

} // namespace drv3d_vulkan
