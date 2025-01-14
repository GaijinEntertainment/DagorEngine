// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// collection of various temporal information that used inside single work item execution, but need memory backing

#include <drv/3d/dag_commands.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>

#include "vk_wrapped_handles.h"
#include "buffer_ref.h"
#include "device_queue.h"

namespace drv3d_vulkan
{

class Image;

struct ExecutionScratch
{
  eastl::vector<FrameEvents *> frameEventCallbacks;
  eastl::vector<Image *> imageResidenceRestores;

  struct DiscardNotify
  {
    BufferRef oldBuf;
    BufferRef newBuf;
    uint32_t flags;
  };
  eastl::vector<DiscardNotify> delayedDiscards;
  struct CommandBufferSubmit
  {
    DeviceQueueType queue;
    VulkanCommandBufferHandle handle;
    uint32_t signals;
    uint32_t waits;
  };
  eastl::vector<CommandBufferSubmit> cmdListsToSubmit;

  struct CommandBufferSubmitDeps
  {
    uint32_t from;
    uint32_t to;
  };
  eastl::vector<CommandBufferSubmitDeps> cmdListsSubmitDeps;

  struct QueueSubmitItem
  {
    Tab<VulkanCommandBufferHandle> cbs;
    Tab<VulkanSemaphoreHandle> signals;
    Tab<VulkanSemaphoreHandle> submitSemaphores;
    uint32_t signalsCount;
    DeviceQueueType queue;
    uint32_t originalSignalId;
    uint32_t originalWaitId;
    bool fenceWait;
  };
  Tab<QueueSubmitItem> submitGraph;

  struct UserQueueSignal
  {
    size_t bufferIdx;
    uint8_t waitedOnQueuesMask;
  };
  eastl::vector<UserQueueSignal> userQueueSignals;

  struct DebugEvent
  {
    uint32_t color;
    const char *name;
  };
  eastl::vector<DebugEvent> debugEventStack;
};

} // namespace drv3d_vulkan
