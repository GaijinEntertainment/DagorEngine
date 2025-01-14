// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_relocatableFixedVector.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_commands.h>
#include <EASTL/fixed_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <EASTL/span.h>
#include <ioSys/dag_dataBlock.h>


namespace rp_impl
{
struct MSAAResolvePair
{
  int32_t src;
  int32_t dst;
};

inline dag::RelocatableFixedVector<MSAAResolvePair, 16> msaaResolves;
inline dag::RelocatableFixedVector<RenderPassTarget, 16> targets;
inline RenderPassArea activeRenderArea;
inline int32_t subpass = 0;

} // namespace rp_impl

namespace d3d
{
void clear_render_pass(const RenderPassTarget &target, const RenderPassArea &area, const RenderPassBind &bind);

namespace render_pass_generic
{
struct RenderPass
{
  dag::RelocatableFixedVector<RenderPassBind, 32> actions;
  dag::RelocatableFixedVector<uint32_t, 32> sequence;
  int32_t subpassCnt;
  int32_t targetCnt;
  int32_t bindingOffset;

  void addSubpassToList(const RenderPassDesc &rp_desc, int32_t subpass);
  void execute(uint32_t idx);
  void resolveMSAATargets();

  const char *getDebugName()
  {
#if DAGOR_DBGLEVEL > 0
    return dbgName.c_str();
#else
    return "<unknown>";
#endif
  }

#if DAGOR_DBGLEVEL > 0
  eastl::fixed_string<char, 128> dbgName;
#endif
};

inline RenderPass *activeRP = nullptr;

void RenderPass::addSubpassToList(const RenderPassDesc &rp_desc, int32_t subpass)
{
  for (auto &bind : eastl::span{rp_desc.binds, rp_desc.bindCount})
  {
    if (bind.subpass == subpass)
      actions.push_back(bind);
  }
  sequence.push_back(actions.size());
}

void RenderPass::execute(uint32_t idx)
{
  G_ASSERT(idx < actions.size());
  RenderPassBind &bind = actions[idx];
  RenderPassTarget &target = rp_impl::targets[bind.target];

  TextureInfo ti{};
  if (target.resource.tex)
    target.resource.tex->getinfo(ti);

  resource_barrier({target.resource.tex, bind.dependencyBarrier, target.resource.layer * ti.mipLevels + target.resource.mip_level, 1});

  // do not bind targets on end subpassor if we not gonna RW to them
  if ((bind.subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END) || (bind.action == RP_TA_NONE))
    return;

  if (bind.action & RP_TA_SUBPASS_READ)
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      if (target.resource.tex)
        target.resource.tex->texmiplevel(target.resource.mip_level, target.resource.mip_level);
      set_tex(STAGE_PS, activeRP->bindingOffset + bind.slot, target.resource.tex);
    }
    else
    {
      G_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      set_depth(target.resource.tex, target.resource.layer, DepthAccess::SampledRO);
    }
  }
  else if (bind.action & RP_TA_SUBPASS_WRITE)
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      set_render_target(bind.slot, target.resource.tex, target.resource.layer, target.resource.mip_level);
    else
    {
      G_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      set_depth(target.resource.tex, target.resource.layer, DepthAccess::RW);
    }

    // process clears
    if (bind.action & (RP_TA_LOAD_CLEAR | RP_TA_LOAD_NO_CARE))
    {
      clear_render_pass(target, rp_impl::activeRenderArea, bind);
    }
  }
  else if (bind.action & RP_TA_SUBPASS_RESOLVE)
  {
    uint32_t srcIndex = 0;
    for (const auto &srcBind : actions)
    {
      if ((srcBind.slot == bind.slot) && (srcBind.subpass == bind.subpass) && (srcIndex++ != idx))
        rp_impl::msaaResolves.push_back({srcBind.target, bind.target});
    }
  }
}

void RenderPass::resolveMSAATargets()
{
  if (rp_impl::msaaResolves.empty())
    return;

  for (auto i : rp_impl::msaaResolves)
  {
    RenderPassTarget &srcTgt = rp_impl::targets[i.src];
    RenderPassTarget &dstTgt = rp_impl::targets[i.dst];
    // TODO: layer & mip is not handled!!!
    dstTgt.resource.tex->update(srcTgt.resource.tex);
  }
  rp_impl::msaaResolves.clear();
}


static bool validate_renderpass_action(RenderPassTargetAction action, const char *what)
{
  G_UNUSED(action);
  G_UNUSED(what);

  bool noErrors = true;

  G_ASSERTF_AND_DO(__popcount(action & RP_TA_SUBPASS_MASK) <= 1, noErrors &= false,
    "'%s' action consists of multiple subpass operations", what);
  G_ASSERTF_AND_DO(__popcount(action & RP_TA_LOAD_MASK) <= 1, noErrors &= false, "'%s' action consists of multiple load operations",
    what);
  G_ASSERTF_AND_DO(__popcount(action & RP_TA_STORE_MASK) <= 1, noErrors &= false, "'%s' action consists of multiple store operations",
    what);

  return noErrors;
}

static bool validate_renderpass_desc(const RenderPassDesc &rp_desc)
{
  G_UNUSED(rp_desc);

  bool noErrors = true;

  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    noErrors &= validate_renderpass_action(rp_desc.binds[i].action, rp_desc.debugName);
    G_ASSERTF_AND_DO(rp_desc.binds[i].target < rp_desc.targetCount, noErrors &= false,
      "target index is out of bounds in bind %u of render pass '%s'", i, rp_desc.debugName);
  }

  return noErrors;
}

RenderPass *create_render_pass(const RenderPassDesc &rp_desc)
{
  G_ASSERTF_RETURN(rp_desc.bindCount > 0, nullptr, "render pass '%s' bindCount is 0", rp_desc.debugName);
  if (!validate_renderpass_desc(rp_desc))
    return nullptr;

  auto ret = new RenderPass{};

  ret->actions.reserve(rp_desc.bindCount);
  ret->sequence.push_back(0);
  ret->subpassCnt = -1;
  ret->targetCnt = rp_desc.targetCount;
  ret->bindingOffset = rp_desc.subpassBindingOffset;

  for (auto &bind : eastl::span{rp_desc.binds, rp_desc.bindCount})
  {
    if (bind.subpass > ret->subpassCnt)
      ret->subpassCnt = bind.subpass;
  }
  ++ret->subpassCnt;
  G_ASSERTF_AND_DO(
    ret->subpassCnt > 0,
    {
      delete ret;
      return nullptr;
    },
    "render pass '%s' description lacking subpassCnt", rp_desc.debugName);

  for (uint32_t i = 0; i < ret->subpassCnt; ++i)
    ret->addSubpassToList(rp_desc, i);
  ret->addSubpassToList(rp_desc, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END);

#if DAGOR_DBGLEVEL > 0
  ret->dbgName = rp_desc.debugName;
#endif

  return ret;
}

void delete_render_pass(RenderPass *rp)
{
  G_ASSERTF(activeRP != rp, "trying to delete active render pass %s", rp->getDebugName());
  delete rp;
}

void next_subpass()
{
  G_ASSERT(activeRP);

  const auto &seq = activeRP->sequence;
  G_ASSERTF(rp_impl::subpass + 1 < seq.size(), "trying to run non existent subpass %u of rp %s", rp_impl::subpass,
    activeRP->getDebugName());

  // bind fully empty RT/DS set
  set_render_target();
  set_render_target(0, nullptr, 0);

  activeRP->resolveMSAATargets();

  for (int i = seq[rp_impl::subpass], e = seq[rp_impl::subpass + 1]; i < e; ++i)
    activeRP->execute(i);

  // reset viewport to render area on any subpass change
  setview(rp_impl::activeRenderArea.left, rp_impl::activeRenderArea.top, rp_impl::activeRenderArea.width,
    rp_impl::activeRenderArea.height, rp_impl::activeRenderArea.minZ, rp_impl::activeRenderArea.maxZ);

  rp_impl::subpass++;
}

namespace
{
bool is_generic_render_pass_validation_enabled()
{
#if DAGOR_DBGLEVEL == 0
  return false;
#endif
  static bool isEnabled = dgs_get_settings()->getBlockByNameEx("video")->getBool("enableGenericRenderPassValidation", false);
  return isEnabled;
}
} // namespace

void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  G_ASSERTF(!activeRP, "render pass %s already started", activeRP->getDebugName());
  G_ASSERTF(nullptr != rp, "'rp' of begin_render_pass was nullptr");

  rp_impl::activeRenderArea = area;
  activeRP = rp;
  for (auto &target : dag::Span{targets, rp->targetCnt})
  {
    if (!target.resource.tex)
      logerr("begin_render_pass for %s received a nullptr texture!", activeRP->getDebugName());
    rp_impl::targets.push_back(target);
  }

  next_subpass();

  if (is_generic_render_pass_validation_enabled())
    d3d::driver_command(Drv3dCommand::BEGIN_GENERIC_RENDER_PASS_CHECKS, (void *)&area);
}

void end_render_pass()
{
  G_ASSERTF(activeRP, "render pass was not started");

  if (is_generic_render_pass_validation_enabled())
    d3d::driver_command(Drv3dCommand::END_GENERIC_RENDER_PASS_CHECKS);

  if (rp_impl::subpass + 1 < activeRP->sequence.size())
    next_subpass();

  rp_impl::targets.clear();
  rp_impl::subpass = 0;
  activeRP = nullptr;

  //! after render pass ends, render targets are reset to backbuffer
  set_render_target();
}
} // namespace render_pass_generic

RenderPass *create_render_pass(const RenderPassDesc &rp_desc)
{
  return reinterpret_cast<RenderPass *>(render_pass_generic::create_render_pass(rp_desc));
}
void delete_render_pass(RenderPass *rp)
{
  render_pass_generic::delete_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp));
}
void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  render_pass_generic::begin_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp), area, targets);
}
void next_subpass() { render_pass_generic::next_subpass(); }
void end_render_pass() { render_pass_generic::end_render_pass(); }

#if DAGOR_DBGLEVEL > 0
void allow_render_pass_target_load() {}
#endif

} // namespace d3d