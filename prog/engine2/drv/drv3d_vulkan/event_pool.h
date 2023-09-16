#pragma once

#include "vulkan_device.h"

namespace drv3d_vulkan
{
class EventPool
{
public:
  inline VulkanEventHandle allocate(VulkanDevice &device, bool set_state)
  {
    VulkanEventHandle event;
    if (freeEvents.empty())
    {
      VkEventCreateInfo eci;
      eci.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
      eci.pNext = NULL;
      eci.flags = 0;
      VULKAN_EXIT_ON_FAIL(device.vkCreateEvent(device.get(), &eci, NULL, ptr(event)));
    }
    else
    {
      event = freeEvents.back();
      freeEvents.pop_back();
    }

    if (set_state)
    {
      VULKAN_EXIT_ON_FAIL(device.vkSetEvent(device.get(), event));
    }
    else
    {
      VULKAN_EXIT_ON_FAIL(device.vkResetEvent(device.get(), event));
    }

    return event;
  }
  inline void free(VulkanEventHandle event) { freeEvents.push_back(event); }

  inline void resetAll(VulkanDevice &device)
  {
    for (int i = 0; i < freeEvents.size(); ++i)
      VULKAN_LOG_CALL(device.vkDestroyEvent(device.get(), freeEvents[i], NULL));
    clear_and_shrink(freeEvents);
  }

private:
  Tab<VulkanEventHandle> freeEvents;
};
} // namespace drv3d_vulkan