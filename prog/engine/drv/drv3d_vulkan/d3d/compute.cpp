// Copyright (C) Gaijin Games KFT.  All rights reserved.

// compute dispatch implementation
#include <drv/3d/dag_dispatch.h>
#include "globals.h"
#include "frontend.h"
#include "global_lock.h"
#include "global_const_buffer.h"
#include "buffer.h"
#include "device_context.h"

using namespace drv3d_vulkan;

void checkComputeOutsideNativeRP()
{
  if (Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
    DAG_FATAL("vulkan: dispatch called inside native render pass %p:%s! fix application logic to avoid this!",
      Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(),
      Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>()
        ->getDebugName());
}

bool d3d::dispatch(uint32_t x, uint32_t y, uint32_t z, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  checkComputeOutsideNativeRP();

  Frontend::GCB.flushCompute(Globals::ctx);
  CmdDispatch cmd{x, y, z};
  Globals::ctx.dispatchPipeline(cmd, "dispatch");
  return true;
}

bool d3d::dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  G_ASSERTF(args != nullptr, "dispatch_indirect with nullptr buffer is invalid");
  G_ASSERTF(args->getFlags() & SBCF_BIND_UNORDERED, "dispatch_indirect buffer without SBCF_BIND_UNORDERED flag");
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "dispatch_indirect buffer is not usable as indirect buffer");
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  checkComputeOutsideNativeRP();

  Frontend::GCB.flushCompute(Globals::ctx);
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;
  CmdDispatchIndirect cmd{buffer->getBufferRef(), byte_offset};
  Globals::ctx.dispatchPipeline(cmd, "dispatchIndirect");
  return true;
}
