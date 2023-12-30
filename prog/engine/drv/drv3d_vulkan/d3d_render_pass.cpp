// all renderpass related stuff
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>

#include "device.h"
#include "driver.h"
#include "pipeline_state.h"
#include "render_pass_resource.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccessor
{
  PipelineState &pipeState;
  Device &drvDev;
  VulkanDevice &dev;
  DeviceContext &ctx;
  ResourceManager &resources;

  LocalAccessor() :
    pipeState(get_device().getContext().getFrontend().pipelineState),
    drvDev(get_device()),
    dev(get_device().getVkDevice()),
    ctx(get_device().getContext()),
    resources(get_device().resources)
  {}
};
} // namespace

d3d::RenderPass *d3d::create_render_pass(const RenderPassDesc &rp_desc)
{
  LocalAccessor la;
  SharedGuardedObjectAutoLock lock(la.resources);

  RenderPassResource *ret = la.resources.alloc<RenderPassResource>({&rp_desc});
  la.drvDev.setRenderPassName(ret, rp_desc.debugName);
  return (d3d::RenderPass *)ret;
}

void d3d::delete_render_pass(d3d::RenderPass *rp)
{
  // can happen from external thread by overall deletion rules, so must be processed in backend
  LocalAccessor la;
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

  la.ctx.nativeRenderPassChanged();
}
