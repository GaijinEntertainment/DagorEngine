// Copyright (C) Gaijin Games KFT.  All rights reserved.

// user provided GPU sync logic implementation
#include <drv/3d/dag_driver.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"
#include "driver_config.h"
#include "command.h"
#include "physical_device_set.h"

using namespace drv3d_vulkan;

namespace
{

DeviceQueueType gpu_pipeline_to_device_queue_type(GpuPipeline pipe)
{
  if (pipe == GpuPipeline::ASYNC_COMPUTE)
    return DeviceQueueType::COMPUTE;
  else
    return DeviceQueueType::GRAPHICS;
}

// use alloc less approach for fence handle
// pack replay ID and local frame fence index
struct PrefixedFenceHandle
{
  // underlying type must fit to external
  static_assert(sizeof(GPUFENCEHANDLE) >= sizeof(uint32_t));
  static constexpr uint32_t fenceBits = 8;
  static constexpr uint32_t fenceMask = (1 << fenceBits) - 1;
  static constexpr uint32_t frameMask = ~fenceMask;

  GPUFENCEHANDLE val;
  GPUFENCEHANDLE toDriver() { return val; }
  uint32_t toIdx() { return val & fenceMask; }

  // +1 to avoid collision with BAD_GPUFENCEHANDLE
  GPUFENCEHANDLE getThisFramePrefix() { return (((Frontend::replay->id + 1) << fenceBits) & frameMask); }
  bool verify() { return getThisFramePrefix() == (val & frameMask); }

  PrefixedFenceHandle(uint32_t idx) : val(getThisFramePrefix() | idx)
  {
    G_ASSERTF(idx < fenceMask, "vulkan: too much user fences per frame, adjust masking");
  }
  PrefixedFenceHandle(GPUFENCEHANDLE user_fence) : val(user_fence) {}
};

} // anonymous namespace

// TODO: move implementation out of device context class

void d3d::resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  if (Globals::lock.isAcquired() && Globals::VK::phy.hasBindless)
    Globals::ctx.resourceBarrier(desc, gpu_pipeline);
}

void drv3d_vulkan::d3d_command_change_queue(GpuPipeline gpu_pipeline)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  Globals::ctx.dispatchCommand<CmdQueueSwitch>({(int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
}

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline gpu_pipeline)
{
  // ignore out of gpu lock fence inserts, because they point to unknown time (can interrupt pass or so)
  // and if they are used - usually for lock transfer tracking, which are handled by frame-frame global sync
  if (!Globals::lock.isAcquired())
    return BAD_GPUFENCEHANDLE;

  uint32_t rawSignalIdx = Frontend::replay->userSignalCount++;
  Globals::ctx.dispatchCommand<CmdQueueSignal>({rawSignalIdx, (int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
  return PrefixedFenceHandle{rawSignalIdx}.toDriver();
}

void d3d::insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline) //-V669
{
  if (fence == BAD_GPUFENCEHANDLE)
    return;

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  PrefixedFenceHandle userFence(fence);

  // old signal from different job!
  if (!userFence.verify())
    return;
  Globals::ctx.dispatchCommand<CmdQueueWait>({userFence.toIdx(), (int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
}