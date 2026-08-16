// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_enhanced_barrier.h>
#include "fence_manager.h"
#include "image_resource.h"
#include "buffer_ref.h"

namespace drv3d_vulkan
{

struct CmdFlushDraws
{};

struct CmdFlushAndWait
{
  ThreadedFence *userFence;
};

struct CmdImageBarrier
{
  Image *img;
  ResourceBarrier state;
  uint32_t res_index;
  uint32_t res_range;
};

struct CmdBufferBarrier
{
  BufferRef bRef;
  ResourceBarrier state;
};

struct EnhancedImageBarrier
{
  Image *img;
  d3d::TextureBarrier barrier;
};

struct EnhancedBufferBarrier
{
  BufferRef bRef;
  d3d::BufferBarrier barrier;
};

// barriers live in Frontend::replay->enhancedImage/BufferBarriers; this references a range of them
struct CmdEnhancedBarrierBatch
{
  uint32_t imageIndex;
  uint32_t imageCount;
  uint32_t bufferIndex;
  uint32_t bufferCount;
};

struct CmdDelaySyncCompletion
{
  bool enable;
};

struct CmdQueueSwitch
{
  int queue;
};

struct CmdQueueSignal
{
  uint32_t signalIdx;
  int queue;
};

struct CmdQueueWait
{
  uint32_t signalIdx;
  int queue;
};

struct CmdCompleteSync
{};

} // namespace drv3d_vulkan
