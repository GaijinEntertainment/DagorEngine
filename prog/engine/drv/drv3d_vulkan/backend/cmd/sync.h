// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/3d/dag_driver.h>
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
