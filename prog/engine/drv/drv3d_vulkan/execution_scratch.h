// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// collection of various temporal information that used inside single work item execution, but need memory backing

#include <drv/3d/dag_commands.h>
#include <util/dag_stdint.h>

#include "vk_wrapped_handles.h"
#include "buffer_ref.h"
#include "device_queue.h"

namespace drv3d_vulkan
{

class Image;
class Buffer;

struct ExecutionScratch
{
  dag::Vector<FrameEvents *> frameEventCallbacks;
  dag::Vector<Image *> imageResidenceRestores;
  dag::Vector<Buffer *> bufferResidenceRestores;

  struct DiscardNotify
  {
    BufferRef oldBuf;
    BufferRef newBuf;
    uint32_t flags;
  };
  dag::Vector<DiscardNotify> delayedDiscards;
  struct CommandBufferSubmit
  {
    DeviceQueueType queue;
    VulkanCommandBufferHandle handle;
    uint32_t signals;
    uint32_t waits;
  };
  dag::Vector<CommandBufferSubmit> cmdListsToSubmit;

  struct CommandBufferSubmitDeps
  {
    uint32_t from;
    uint32_t to;
  };
  dag::Vector<CommandBufferSubmitDeps> cmdListsSubmitDeps;

  struct QueueSubmitItem
  {
    Tab<VulkanCommandBufferHandle> cbs;
    Tab<VulkanSemaphoreHandle> signals;
    Tab<VulkanSemaphoreHandle> waitSemaphores;
    Tab<DeviceQueue::TimelineInfo> waitTimelines;
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
  dag::Vector<UserQueueSignal> userQueueSignals;

  struct DebugEvent
  {
    uint32_t color;
    const char *name;
  };
  dag::Vector<DebugEvent> debugEventStack;
};

} // namespace drv3d_vulkan
