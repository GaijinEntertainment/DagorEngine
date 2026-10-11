// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_relocatableFixedVector.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_barrier.h>
#include <drv/3d/dag_variableRateShading.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/array.h>
#include <EASTL/fixed_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <EASTL/span.h>
#include <ioSys/dag_dataBlock.h>
#include "validation.h"

#include "drv_assert_defs.h"
#include "drv_log_defs.h"
#include "renderPassValidation.h"

#include <drv/shadersMetaData/renderPassLowering.h>


namespace rp_impl
{
struct MSAAResolvePair
{
  int32_t src;
  int32_t dst;
  int32_t slot;
  RenderPassTargetAction srcAction;
};

inline dag::RelocatableFixedVector<MSAAResolvePair, 16> msaaResolves;
inline dag::RelocatableFixedVector<RenderPassTarget, 16> targets;
inline RenderPassArea activeRenderArea;
inline int32_t subpass = 0;
inline bool activeVRSTarget = false;

// Current RT/DS set for the active subpass. The generic path binds targets one slot per execute(), but the unified
// d3d::set_render_target rebinds the whole set atomically, so we accumulate the set here and re-issue it per bind.
inline eastl::array<RenderTarget, Driver3dRenderTarget::MAX_SIMRT> colorTargets{};
inline uint32_t colorTargetCount = 0;
inline RenderTarget depthTarget{};
inline DepthAccess depthAccess = DepthAccess::RW;

} // namespace rp_impl

namespace d3d _MULTI_INTERFACE
{
void clear_render_pass(const RenderPassTarget &target, const RenderPassArea &area, const RenderPassBind &bind);
} // namespace d3d _MULTI_INTERFACE

namespace d3d::render_pass_generic _MULTI_INTERFACE
{
struct RenderPass
{
  dag::RelocatableFixedVector<RenderPassBind, 32> actions;
  dag::RelocatableFixedVector<uint32_t, 32> sequence;
  int32_t subpassCnt = 0;
  int32_t targetCnt = 0;

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

static void apply_render_targets()
{
  set_render_target(rp_impl::depthTarget, rp_impl::depthAccess,
    dag::ConstSpan<RenderTarget>(rp_impl::colorTargets.data(), rp_impl::colorTargetCount));
}

// Nothing else unbinds these SRVs. A target read in one subpass and written as a color in the next
// would otherwise be bound as SRV and render target at once.
static void unbind_subpass_reads()
{
  if (rp_impl::subpass == 0)
    return;
  const auto &seq = activeRP->sequence;
  for (int i = seq[rp_impl::subpass - 1], e = seq[rp_impl::subpass]; i < e; ++i)
  {
    const RenderPassBind &bind = activeRP->actions[i];
    if ((bind.action & RP_TA_SUBPASS_READ) && bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      set_tex(STAGE_PS, subpass_read_register(RP_GENERIC_MAX_T_REGISTERS, bind.slot), nullptr);
  }
}

static void reset_render_targets()
{
  for (auto &t : rp_impl::colorTargets)
    t = RenderTarget{};
  rp_impl::colorTargetCount = 0;
  rp_impl::depthTarget = RenderTarget{};
  rp_impl::depthAccess = DepthAccess::RW;
}

static void transition_untracked_resolve_source(BaseTexture *tex, int32_t slot, RenderPassTargetAction action)
{
  TextureInfo ti{};
  tex->getinfo(ti);
  if ((ti.cflg & TEXCF_NO_STATE_TRACKING) == 0)
    return;

  d3d::PipelineStageFlags stages = d3d::PipelineStageFlag::OutputMerging;
  d3d::AccessFlags access = d3d::AccessFlag::RenderTargetWrite;
  d3d::TextureLayout layout = d3d::TextureLayout::RenderTarget;
  if (slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
  {
    stages = d3d::PipelineStageFlag::EarlyFragmentTests | d3d::PipelineStageFlag::LateFragmentTests;
    access = d3d::AccessFlag::DepthStencilWrite;
    layout = d3d::TextureLayout::DepthRw;
    if (action & RP_TA_SUBPASS_READ)
    {
      stages |= d3d::PipelineStageFlag::AllVertexShading | d3d::PipelineStageFlag::PixelShading;
      access = d3d::AccessFlag::DepthStencilRead | d3d::AccessFlag::ShaderResource;
      layout = d3d::TextureLayout::GenericRead;
    }
  }
  enhanced_texture_barrier({{stages, d3d::PipelineStageFlag::Resolve}, {access, d3d::AccessFlag::ResolveRead},
                             {layout, d3d::TextureLayout::ResolveSource}, {{0xffffffffu, 0}, {0, 0}, {0, 0}}},
    tex);
}

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
  D3D_CONTRACT_ASSERT(idx < actions.size());
  RenderPassBind &bind = actions[idx];
  RenderPassTarget &target = rp_impl::targets[bind.target];

  TextureInfo ti{};
  if (target.resource.tex)
    target.resource.tex->getinfo(ti);

  if ((ti.cflg & TEXCF_NO_STATE_TRACKING) == 0)
    resource_barrier(
      {target.resource.tex, bind.dependencyBarrier, target.resource.layer * ti.mipLevels + target.resource.mip_level, 1});

  // do not bind targets on end subpassor if we not gonna RW to them
  if ((bind.subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END) || (bind.action == RP_TA_NONE))
    return;

  if (bind.action & RP_TA_SUBPASS_READ)
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      if (target.resource.tex)
        target.resource.tex->texmiplevel(target.resource.mip_level, target.resource.mip_level);
      set_tex(STAGE_PS, subpass_read_register(RP_GENERIC_MAX_T_REGISTERS, bind.slot), target.resource.tex);
    }
    else
    {
      D3D_CONTRACT_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      rp_impl::depthTarget = target.resource;
      rp_impl::depthAccess = DepthAccess::SampledRO;
    }
  }
  else if (bind.action & RP_TA_SUBPASS_WRITE)
  {
    if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      rp_impl::colorTargets[bind.slot] = target.resource;
      if (uint32_t(bind.slot) + 1 > rp_impl::colorTargetCount)
        rp_impl::colorTargetCount = bind.slot + 1;
    }
    else
    {
      D3D_CONTRACT_ASSERTF(target.resource.mip_level == 0, "using mip level for depth bind is not supported (rp %s)", getDebugName());
      rp_impl::depthTarget = target.resource;
      rp_impl::depthAccess = DepthAccess::RW;
    }
  }
  else if (bind.action & RP_TA_SUBPASS_RESOLVE)
  {
    // The resolve source is the attachment bound through the same slot in the same subpass: the one
    // written there, or the read-only depth on the depth-stencil slot. The first match is the source,
    // the way creation validates it: a second pair into one destination would overwrite the first.
    for (const auto &srcBind : actions)
      if (srcBind.slot == bind.slot && srcBind.subpass == bind.subpass &&
          (srcBind.action & render_pass_validation::resolve_source_actions(bind.slot)))
      {
        rp_impl::msaaResolves.push_back({srcBind.target, bind.target, bind.slot, srcBind.action});
        break;
      }
  }
  else if (bind.action & RP_TA_SUBPASS_VRS_READ)
  {
    G_ASSERTF(bind.slot == RenderPassExtraIndexes::RP_SLOT_VRS_TEXTURE,
      "RP: trying to bind target as VRS texture, yet slot %u != RP_SLOT_VRS_TEXTURE", bind.slot);
    d3d::set_variable_rate_shading_texture(target.resource.tex);
    rp_impl::activeVRSTarget = true;
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
    transition_untracked_resolve_source(srcTgt.resource.tex, i.slot, i.srcAction);
    // TODO: layer & mip is not handled!!!
    dstTgt.resource.tex->update(srcTgt.resource.tex);
  }
  rp_impl::msaaResolves.clear();
}


static bool validate_read_slots(const RenderPassDesc &rp_desc)
{
  const char *name = rp_desc.debugName ? rp_desc.debugName : "<unnamed>";
  G_UNUSED(name);
  bool noErrors = true;

  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];
    if ((bind.action & RP_TA_SUBPASS_READ) == 0 || bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      continue;
    D3D_CONTRACT_ASSERTF_AND_DO(subpass_read_fits_window(RP_GENERIC_MAX_T_REGISTERS, bind.slot), noErrors = false,
      "subpass read slot %d is out of the T register window in bind %u of render pass '%s'", bind.slot, i, name);
  }

  return noErrors;
}

RenderPass *create_render_pass(const RenderPassDesc &rp_desc)
{
  bool descOk = validate_render_pass_desc(rp_desc);
  descOk &= validate_read_slots(rp_desc);
  if (!descOk)
    return nullptr;

  auto ret = new RenderPass{};

  ret->actions.reserve(rp_desc.bindCount);
  ret->sequence.push_back(0);
  ret->subpassCnt = -1;
  ret->targetCnt = rp_desc.targetCount;

  for (auto &bind : eastl::span{rp_desc.binds, rp_desc.bindCount})
    if (bind.subpass > ret->subpassCnt)
      ret->subpassCnt = bind.subpass;
  ++ret->subpassCnt;

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
  D3D_CONTRACT_ASSERTF(activeRP != rp, "trying to delete active render pass %s", rp->getDebugName());
  delete rp;
}

void reset_vrs_texture()
{
  if (DAGOR_LIKELY(!rp_impl::activeVRSTarget))
    return;

  d3d::set_variable_rate_shading_texture(nullptr);
  rp_impl::activeVRSTarget = false;
}

void next_subpass()
{
  D3D_CONTRACT_ASSERT(activeRP);

  const auto &seq = activeRP->sequence;
  D3D_CONTRACT_ASSERTF(rp_impl::subpass + 1 < seq.size(), "trying to run non existent subpass %u of rp %s", rp_impl::subpass,
    activeRP->getDebugName());

  unbind_subpass_reads();
  reset_render_targets();
  reset_vrs_texture();

  activeRP->resolveMSAATargets();

  // execute() accumulates per-slot binds into the rp_impl RT/DS state without rebinding; apply once
  // here so the unified d3d::set_render_target gets the full subpass set in one call.
  for (int i = seq[rp_impl::subpass], e = seq[rp_impl::subpass + 1]; i < e; ++i)
    activeRP->execute(i);
  apply_render_targets();
  for (int i = seq[rp_impl::subpass], e = seq[rp_impl::subpass + 1]; i < e; ++i)
    if (activeRP->actions[i].action & RP_TA_SUBPASS_WRITE)
      if (activeRP->actions[i].action & (RP_TA_LOAD_CLEAR | RP_TA_LOAD_NO_CARE | RP_TA_LOAD_STENCIL_CLEAR))
        clear_render_pass(rp_impl::targets[activeRP->actions[i].target], rp_impl::activeRenderArea, activeRP->actions[i]);

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

void begin_render_pass(RenderPass *rp, const RenderPassArea area, dag::ConstSpan<RenderPassTarget> targets)
{
  D3D_CONTRACT_ASSERTF(!activeRP, "render pass %s already started", activeRP->getDebugName());
  D3D_CONTRACT_ASSERTF(nullptr != rp, "'rp' of begin_render_pass was nullptr");

  rp_impl::activeRenderArea = area;
  activeRP = rp;
  D3D_CONTRACT_ASSERTF(rp->targetCnt == targets.size(), "missing/excessive targets for rp %s, expected %u got %u", rp->getDebugName(),
    rp->targetCnt, targets.size());
  for (auto &target : targets)
  {
    if (!target.resource.tex)
      D3D_CONTRACT_ERROR("begin_render_pass for %s received a nullptr texture!", activeRP->getDebugName());
    rp_impl::targets.push_back(target);
  }

  next_subpass();

  if (is_generic_render_pass_validation_enabled())
    d3d::driver_command(Drv3dCommand::BEGIN_GENERIC_RENDER_PASS_CHECKS, (void *)&area);
}

void end_render_pass()
{
  D3D_CONTRACT_ASSERTF(activeRP, "render pass was not started");

  if (is_generic_render_pass_validation_enabled())
    d3d::driver_command(Drv3dCommand::END_GENERIC_RENDER_PASS_CHECKS);

  if (rp_impl::subpass + 1 < activeRP->sequence.size())
    next_subpass();
  unbind_subpass_reads();

  rp_impl::targets.clear();
  rp_impl::subpass = 0;
  activeRP = nullptr;
  reset_vrs_texture();

  //! after render pass ends, render targets are reset to backbuffer
  set_render_target();
}
} // namespace d3d::render_pass_generic_MULTI_INTERFACE

namespace d3d _MULTI_INTERFACE
{
RenderPass *create_render_pass(const RenderPassDesc &rp_desc)
{
  return reinterpret_cast<RenderPass *>(render_pass_generic::create_render_pass(rp_desc));
}
void delete_render_pass(RenderPass *rp)
{
  render_pass_generic::delete_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp));
}
void begin_render_pass(RenderPass *rp, const RenderPassArea area, dag::ConstSpan<RenderPassTarget> targets)
{
  render_pass_generic::begin_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp), area, targets);
}
void next_subpass() { render_pass_generic::next_subpass(); }
void end_render_pass() { render_pass_generic::end_render_pass(); }

#if DAGOR_DBGLEVEL > 0
void allow_render_pass_target_load() {}
#endif

} // namespace d3d _MULTI_INTERFACE