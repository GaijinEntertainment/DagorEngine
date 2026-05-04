// Copyright (C) Gaijin Games KFT.  All rights reserved.

// compute dispatch implementation
#include <drv/3d/dag_dispatch.h>
#include "globals.h"
#include "frontend.h"
#include "global_lock.h"
#include "global_const_buffer.h"
#include "buffer.h"
#include "physical_device_set.h"
#include "device_context.h"
#include "backend/cmd/draw_dispatch.h"

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
  D3D_CONTRACT_ASSERT(x <= Globals::VK::phy.properties.limits.maxComputeWorkGroupCount[0]);
  D3D_CONTRACT_ASSERT(y <= Globals::VK::phy.properties.limits.maxComputeWorkGroupCount[1]);
  D3D_CONTRACT_ASSERT(z <= Globals::VK::phy.properties.limits.maxComputeWorkGroupCount[2]);
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  checkComputeOutsideNativeRP();

  Frontend::GCB.flushCompute(Globals::ctx);
  Globals::ctx.dispatchPipeline<CmdDispatch>({x, y, z}, "dispatch");
  return true;
}

bool d3d::dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  D3D_CONTRACT_ASSERTF(args != nullptr, "dispatch_indirect with nullptr buffer is invalid");
  D3D_CONTRACT_ASSERTF(args->getFlags() & SBCF_BIND_UNORDERED, "dispatch_indirect buffer without SBCF_BIND_UNORDERED flag");
  D3D_CONTRACT_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "dispatch_indirect buffer is not usable as indirect buffer");
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  checkComputeOutsideNativeRP();

  Frontend::GCB.flushCompute(Globals::ctx);
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;
  Globals::ctx.dispatchPipeline<CmdDispatchIndirect>({buffer->getBufferRef(), byte_offset}, "dispatchIndirect");
  return true;
}
