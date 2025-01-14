// Copyright (C) Gaijin Games KFT.  All rights reserved.

// all renderpass related stuff
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>

#include "driver.h"
#include "pipeline_state.h"
#include "render_pass_resource.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "debug_naming.h"
#include "device_context.h"
#include "frontend.h"
#include "global_lock.h"
#include "frontend_pod_state.h"
#include "timelines.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccessor
{
  PipelineState &pipeState;
  VulkanDevice &dev;
  DeviceContext &ctx;
  ResourceManager &resources;

  LocalAccessor() : pipeState(Frontend::State::pipe), dev(Globals::VK::dev), ctx(Globals::ctx), resources(Globals::Mem::res)
  {
    VERIFY_GLOBAL_LOCK_ACQUIRED();
  }
};

struct LocalAccessorWithoutState
{
  VulkanDevice &dev;
  DeviceContext &ctx;
  ResourceManager &resources;

  LocalAccessorWithoutState() : dev(Globals::VK::dev), ctx(Globals::ctx), resources(Globals::Mem::res) {}
};
} // namespace

d3d::RenderPass *d3d::create_render_pass(const RenderPassDesc &rp_desc)
{
  LocalAccessorWithoutState la;
  WinAutoLock lk(Globals::Mem::mutex);

  RenderPassResource *ret = Globals::Mem::res.alloc<RenderPassResource>({&rp_desc});
  Globals::Dbg::naming.setRenderPassName(ret, rp_desc.debugName);
  return (d3d::RenderPass *)ret;
}

void d3d::delete_render_pass(d3d::RenderPass *rp)
{
  // can happen from external thread by overall deletion rules, so must be processed in backend
  LocalAccessorWithoutState la;
  la.ctx.destroyRenderPassResource((RenderPassResource *)rp);
}

void d3d::begin_render_pass(d3d::RenderPass *drv_rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  G_ASSERTF(drv_rp, "vulkan: can't start null render pass");
  using Bind = StateFieldRenderPassTarget;
  LocalAccessor la;

  RenderPassResource *rp = (RenderPassResource *)drv_rp;
  for (uint32_t i = 0; i < rp->getTargetsCount(); ++i)
  {
    G_ASSERTF(targets[i].resource.tex, "vulkan: trying to start render pass %s with missing target %u", rp->getDebugName(), i);
    la.pipeState.set<StateFieldRenderPassTargets, Bind::IndexedRaw, FrontGraphicsState, FrontRenderPassState>({i, targets[i]});
  }

  la.pipeState.set<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(rp);
  la.pipeState.set<StateFieldRenderPassSubpassIdx, StateFieldRenderPassSubpassIdx::Zero, FrontGraphicsState, FrontRenderPassState>({});
  la.pipeState.set<StateFieldRenderPassArea, RenderPassArea, FrontGraphicsState, FrontRenderPassState>(area);
  la.pipeState.set<StateFieldGraphicsViewport, RenderPassArea, FrontGraphicsState>(area);

  {
    OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
    Frontend::State::pod.nativeRenderPassesCount = Frontend::replay->nativeRPDrawCounter.size();
    la.pipeState.set<StateFieldRenderPassIndex, int, FrontGraphicsState, FrontRenderPassState>(
      Frontend::State::pod.nativeRenderPassesCount);
    Frontend::replay->nativeRPDrawCounter.push_back(Frontend::State::pod.drawsCount);
  }

  la.ctx.nativeRenderPassChanged();
}

void d3d::next_subpass()
{
  LocalAccessor la;

  la.pipeState.set<StateFieldRenderPassSubpassIdx, StateFieldRenderPassSubpassIdx::Next, FrontGraphicsState, FrontRenderPassState>({});
  const RenderPassArea &area =
    la.pipeState.getRO<StateFieldRenderPassArea, RenderPassArea, FrontGraphicsState, FrontRenderPassState>();
  la.pipeState.set<StateFieldGraphicsViewport, RenderPassArea, FrontGraphicsState>(area);
  la.ctx.nativeRenderPassChanged();
}

void d3d::end_render_pass()
{
  LocalAccessor la;
  using Bind = StateFieldRenderPassTarget;

  RenderPassResource *rp =
    la.pipeState.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>();
  G_ASSERTF(rp, "vulkan: trying to end render pass when there is no active one");

  la.pipeState.set<StateFieldRenderPassSubpassIdx, StateFieldRenderPassSubpassIdx::Invalid, FrontGraphicsState, FrontRenderPassState>(
    {});
  la.pipeState.set<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(nullptr);
  for (uint32_t i = 0; i < rp->getTargetsCount(); ++i)
    la.pipeState.set<StateFieldRenderPassTargets, Bind::Indexed, FrontGraphicsState, FrontRenderPassState>({i, Bind::empty});

  {
    OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
    Frontend::replay->nativeRPDrawCounter.back() = Frontend::State::pod.drawsCount - Frontend::replay->nativeRPDrawCounter.back();
  }
  la.ctx.nativeRenderPassChanged();
}
