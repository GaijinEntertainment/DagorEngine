// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// resource readbacks storage and reordering logic

#include "copy_info.h"
#include "vulkan_api.h"
#include "driver.h"

namespace drv3d_vulkan
{

struct BufferReadbacks
{
  dag::Vector<BufferCopyInfo> info;
  dag::Vector<VkBufferCopy> copies;
};

struct ImageReadbacks
{
  dag::Vector<ImageCopyInfo> info;
  dag::Vector<VkBufferImageCopy> copies;
};

struct BatchedReadbacks
{
  ImageReadbacks images;
  BufferReadbacks buffers;
  dag::Vector<BufferFlushInfo> bufferFlushes;
  void clear();
  bool empty();

  void transferOwnership(DeviceQueueType src, DeviceQueueType dst);
};

struct ResourceReadbacks
{
  // min is 3, because we must record current, old and next elements, due to QFOT requirements
  static constexpr size_t RING_SIZE = max<uint32_t>(3, REPLAY_TIMELINE_HISTORY_SIZE);
  BatchedReadbacks storage[RING_SIZE];
  size_t currentWriteIndex = 0;
  size_t currentProcessIndex = 0;
  size_t uploadDelay = 0;

  BatchedReadbacks &getForWrite() { return storage[currentWriteIndex]; }
  // in case of async readback, we process previous batched element, making readbacks overlapped on next frame
  BatchedReadbacks *getForProcess() { return &storage[currentProcessIndex]; }
  void init();
  void shutdown();
  void advance();
  size_t getLatency() { return uploadDelay; }
};

} // namespace drv3d_vulkan
