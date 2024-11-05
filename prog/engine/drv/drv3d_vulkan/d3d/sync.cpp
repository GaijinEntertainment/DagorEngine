// Copyright (C) Gaijin Games KFT.  All rights reserved.

// user provided GPU sync logic implementation
#include <drv/3d/dag_driver.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"

using namespace drv3d_vulkan;

// TODO: move implementation out of device context class

void d3d::resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  if (Globals::lock.isAcquired())
    Globals::ctx.resourceBarrier(desc, gpu_pipeline);
}
