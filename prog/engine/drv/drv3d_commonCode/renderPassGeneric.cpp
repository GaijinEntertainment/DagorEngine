#include <util/dag_string.h>
#include "renderPassGeneric.h"
#include <3d/dag_drv3d.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>

using namespace render_pass_generic;

namespace rp_impl
{
RenderPass *activeRP = nullptr;
RenderPassArea activeRenderArea;
int32_t subpass = 0;
eastl::vector<RenderPassTarget> targets;
}; // namespace rp_impl

void RenderPass::addSubpassToList(const RenderPassDesc &rp_desc, int32_t subpass)
{
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];
    if (bind.subpass == subpass)
      actions.push_back(bind);
  }
  sequence.push_back(actions.size());
}

const char *RenderPass::getDebugName()
{
#if DAGOR_DBGLEVEL > 0
  return dbgName;
#else
  return "<unknown>";
#endif
}

void RenderPass::execute(uint32_t idx, ClearAccumulator &clear_acm)
{
  G_ASSERT(idx < actions.size());
  RenderPassBind &bind = actions[idx];
  RenderPassTarget &target = rp_impl::targets[bind.target];

  TextureInfo ti;
  target.resource.tex->getinfo(ti);

  d3d::resource_barrier(
    {target.resource.tex, bind.dependencyBarrier, target.resource.layer * ti.mipLevels + target.resource.mip_level, 1});

  // do not bind targets on end subpass
  // or if we not gonna RW to them
  if ((bind.subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END) || (bind.action == RP_TA_NONE))
    return;

  if (bind.action & RP_TA_SUBPASS_READ)
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      target.resource.tex->texmiplevel(target.resource.mip_level, target.resource.mip_level);
      d3d::set_tex(STAGE_PS, rp_impl::activeRP->bindingOffset + bind.slot, target.resource.tex);
    }
    else
    {
      G_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      d3d::set_depth(target.resource.tex, target.resource.layer, DepthAccess::SampledRO);
    }
  }
  else
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      d3d::set_render_target(bind.slot, target.resource.tex, target.resource.layer, target.resource.mip_level);
    else
    {
      G_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      d3d::set_depth(target.resource.tex, target.resource.layer, DepthAccess::RW);
    }

    // process clears
    if (bind.action & (RP_TA_LOAD_CLEAR | RP_TA_LOAD_NO_CARE))
    {
      if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      {
        if (bind.action & RP_TA_LOAD_NO_CARE)
          clear_acm.mask |= CLEAR_DISCARD_TARGET;
        else
          clear_acm.mask |= CLEAR_TARGET;

        // first slot with clear
        if ((bind.target < clear_acm.colorTarget) || (clear_acm.colorTarget < 0))
          clear_acm.colorTarget = bind.target;

        // this approach will not work as it ignores render area
        // so any followup custom clear values are ignored
        // E3DCOLOR c(target.clearValue.asUint[0]);
        // d3d::clear_rwtexi(target.resource.tex, {c.a, c.r, c.g, c.b}, target.resource.layer,
        // target.resource.mip_level);
      }
      else
      {
        if (bind.action & RP_TA_LOAD_NO_CARE)
          clear_acm.mask |= CLEAR_DISCARD_STENCIL | CLEAR_DISCARD_ZBUFFER;
        else
          clear_acm.mask |= CLEAR_STENCIL | CLEAR_ZBUFFER;

        clear_acm.depthTarget = bind.target;
      }
    }
  }
}

RenderPass *render_pass_generic::create_render_pass(const RenderPassDesc &rp_desc)
{
  RenderPass *ret = new RenderPass();
#if DAGOR_DBGLEVEL > 0
  ret->dbgName = rp_desc.debugName;
#endif
  ret->actions.reserve(rp_desc.bindCount);
  ret->sequence.push_back(0);
  ret->subpassCnt = -1;
  ret->targetCnt = rp_desc.targetCount;
  ret->bindingOffset = rp_desc.subpassBindingOffset;

  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];
    if (bind.subpass > ret->subpassCnt)
      ret->subpassCnt = bind.subpass;
  }
  ++ret->subpassCnt;
  G_ASSERTF(ret->subpassCnt > 0, "render pass desctiption lacking subpassCnt");

  for (uint32_t i = 0; i < ret->subpassCnt; ++i)
    ret->addSubpassToList(rp_desc, i);
  ret->addSubpassToList(rp_desc, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END);

  return ret;
}

void render_pass_generic::delete_render_pass(RenderPass *rp)
{
  G_ASSERTF(rp_impl::activeRP != rp, "trying to delete active render pass %s", rp->getDebugName());
  delete rp;
}

void render_pass_generic::begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  G_ASSERTF(!rp_impl::activeRP, "render pass %s already started", rp_impl::activeRP->getDebugName());
  G_ASSERT(rp);

  rp_impl::activeRenderArea = area;
  rp_impl::activeRP = rp;
  for (uint32_t i = 0; i < rp->targetCnt; ++i)
    rp_impl::targets.push_back(targets[i]);

  next_subpass();
}

void render_pass_generic::next_subpass()
{
  G_ASSERT(rp_impl::activeRP);

  eastl::vector<uint32_t> &seq = rp_impl::activeRP->sequence;
  G_ASSERTF(rp_impl::subpass + 1 < seq.size(), "trying to run non existent subpass %u of rp %s", rp_impl::subpass,
    rp_impl::activeRP->getDebugName());

  // bind fully empty RT/DS set
  d3d::set_render_target();
  d3d::set_render_target(0, nullptr, 0);

  RenderPass::ClearAccumulator ca;
  for (int i = seq[rp_impl::subpass]; i < seq[rp_impl::subpass + 1]; ++i)
    rp_impl::activeRP->execute(i, ca);

  // reset viewport to render area on any subpass change
  d3d::setview(rp_impl::activeRenderArea.left, rp_impl::activeRenderArea.top, rp_impl::activeRenderArea.width,
    rp_impl::activeRenderArea.height, rp_impl::activeRenderArea.minZ, rp_impl::activeRenderArea.maxZ);

  if (ca.mask > 0)
  {
    E3DCOLOR colorClearValue(0);
    if (ca.colorTarget >= 0)
    {
      ResourceClearValue &rcv = rp_impl::targets[ca.colorTarget].clearValue;
      colorClearValue = E3DCOLOR(rcv.asFloat[0] * 255, rcv.asFloat[1] * 255, rcv.asFloat[2] * 255, rcv.asFloat[3] * 255);
    }

    d3d::clearview(ca.mask, colorClearValue, ca.depthTarget >= 0 ? rp_impl::targets[ca.depthTarget].clearValue.asDepth : 0.0f,
      ca.depthTarget >= 0 ? rp_impl::targets[ca.depthTarget].clearValue.asStencil : 0);
  }

  ++rp_impl::subpass;
}

void render_pass_generic::end_render_pass()
{
  G_ASSERT(rp_impl::activeRP);

  if (rp_impl::subpass + 1 < rp_impl::activeRP->sequence.size())
    next_subpass();

  rp_impl::targets.clear();
  rp_impl::activeRP = nullptr;
  rp_impl::subpass = 0;

  //! after render pass ends, render targets are reset to backbuffer
  d3d::set_render_target();
}
