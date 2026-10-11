// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderRenderPass.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_barrier.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_texFlags.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/shadersMetaData/renderPassLowering.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorMap.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_tab.h>
#include <util/dag_hash.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <debug/dag_assert.h>
#include <debug/dag_fatal.h>
#include <debug/dag_log.h>
#include "shadersBinaryData.h"
#include "shBindumpsPrivate.h"
#include "shaderRenderPassPrivate.h"

#include <drv/shadersMetaData/dx11/register_limits.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <drv/shadersMetaData/metal/register_limits.h>
#if _TARGET_C1

#elif _TARGET_C2

#endif

using namespace ShaderRenderPass;

using FramebufferLayout = bindump::Mapper<shader_layout::FramebufferLayout>;
using RenderPassGroup = bindump::Mapper<shader_layout::RenderPassGroup>;

struct ShaderRenderPass::Pass::Impl
{
  struct Resolve
  {
    uint16_t src = 0;
    uint16_t dst = 0;
  };
  struct Subpass
  {
    dag::Vector<uint16_t> colors;
    dag::Vector<uint16_t> reads;
    uint16_t depthTarget = 0xFFFF;
    uint16_t vrsTarget = 0xFFFF;
    bool depthReadOnly = false;
  };

  // Copies, never pointers into the dump: an instance outlives a dump reload of an unchanged shape.
  eastl::string name;
  uint32_t nameHash = 0;
  uint32_t shapeHash = 0;
  eastl::string layoutName;
  uint32_t layoutNameHash = 0;
  dag::Vector<eastl::string> targetNames;
  dag::Vector<uint32_t> targetFormats; // TEXFMT_*
  dag::Vector<uint8_t> targetSamples;
  dag::Vector<uint16_t> targetPlan; // PLAN_* bits
  dag::Vector<int> firstBindSubpass;
  dag::Vector<Subpass> subpasses;
  dag::Vector<Resolve> resolves;
  dag::Vector<uint8_t> targetIsVrs;
  dag::Vector<int> driverTarget;
  int driverTargetCount = 0;
  bool vrsEnabled = false;
  int maxReads = 0;
  bool split = false;
  int splitModeVarId = -1;
  uint32_t dumpGeneration = 0;
  uint32_t resetCounter = 0;
  d3d::RenderPass *rp = nullptr;
};

namespace
{
using namespace shaderbindump;
using Impl = ShaderRenderPass::Pass::Impl;

constexpr int MAX_T_REGISTERS = RENDER_PASS_T_REGISTER_COUNT;
static_assert(dx11::MAX_T_REGISTERS == MAX_T_REGISTERS);
static_assert(dxil::MAX_T_REGISTERS == MAX_T_REGISTERS);
static_assert(metal::MAX_T_REGISTERS == MAX_T_REGISTERS);
#if _TARGET_C1

#elif _TARGET_C2

#endif

const ScriptedShadersBinDumpV6 *main_dump() { return shBinDumpOwner().getDumpV6(); }

int target_index(const Impl &p, const char *name)
{
  if (!name)
    return -1;
  for (int ti = 0; ti < int(p.targetNames.size()); ++ti)
    if (p.targetNames[ti] == name)
      return ti;
  return -1;
}

void hash_add(uint32_t &h, uint32_t v) { h = fnv1a_step<32>(v, h); }
uint32_t shape_hash(const RenderPass &rp)
{
  uint32_t h = FNV1Params<32>::offset_basis;
  hash_add(h, rp.nameHash);
  hash_add(h, rp.flags);
  hash_add(h, rp.targets.size());
  for (const auto &t : rp.targets)
  {
    for (const char *c = t.name.c_str(); *c; ++c)
      hash_add(h, uint8_t(*c));
    hash_add(h, t.flags);
  }
  hash_add(h, rp.resolves.size());
  for (const auto &r : rp.resolves)
  {
    hash_add(h, r.srcTargetIndex);
    hash_add(h, r.dstTargetIndex);
  }
  hash_add(h, rp.subpasses.size());
  for (const auto &sp : rp.subpasses)
  {
    hash_add(h, sp.nameHash);
    hash_add(h, sp.colors.size());
    for (uint16_t c : sp.colors)
      hash_add(h, c);
    hash_add(h, sp.depthTargetIndex);
    hash_add(h, sp.flags);
    hash_add(h, sp.reads.size());
    for (uint16_t r : sp.reads)
      hash_add(h, r);
    hash_add(h, sp.vrsTargetIndex);
  }
  return h;
}

struct FoundPass
{
  const RenderPass *rp = nullptr;
  const ScriptedShadersBinDumpV6 *dump = nullptr;
};

FoundPass find_pass(Id id)
{
  FoundPass res;
  if (id == Id::Invalid)
    return res;
  const ScriptedShadersBinDumpV6 *d = main_dump();
  if (!d)
    return res;
  const uint32_t nameHash = uint32_t(id);
  for (const auto &rp : d->renderPasses)
    if (rp.nameHash == nameHash)
    {
      res.rp = &rp;
      res.dump = d;
      break;
    }
  return res;
}

const FramebufferLayout *find_layout(const ScriptedShadersBinDumpV6 &dump, uint32_t name_hash)
{
  for (const auto &fb : dump.framebufferLayouts)
    if (fb.nameHash == name_hash)
      return &fb;
  return nullptr;
}

enum : uint16_t
{
  PLAN_LOAD = 1 << 0,
  PLAN_CLEAR = 1 << 1,
  PLAN_LOAD_DONT_CARE = 1 << 2,
  PLAN_STORE = 1 << 3,
  PLAN_NO_STORE = 1 << 4,
};
constexpr uint16_t PLAN_LOAD_MASK = PLAN_LOAD | PLAN_CLEAR | PLAN_LOAD_DONT_CARE;
constexpr uint16_t PLAN_STORE_MASK = PLAN_STORE | PLAN_NO_STORE;

uint32_t load_flags(uint16_t a)
{
  if (a & PLAN_CLEAR)
    return RP_TA_LOAD_CLEAR;
  if (a & PLAN_LOAD)
    return RP_TA_LOAD_READ;
  if (a & PLAN_LOAD_DONT_CARE)
    return RP_TA_LOAD_NO_CARE;
  return 0;
}
uint32_t store_flags(uint16_t a)
{
  if (a & PLAN_STORE)
    return RP_TA_STORE_WRITE;
  if (a & PLAN_NO_STORE)
    return RP_TA_STORE_NONE;
  return 0;
}

uint16_t load_bit(RpLoad a)
{
  switch (a)
  {
    case RpLoad::Clear: return PLAN_CLEAR;
    case RpLoad::DontCare: return PLAN_LOAD_DONT_CARE;
    default: return PLAN_LOAD;
  }
}
uint16_t store_bit(RpStore a)
{
  switch (a)
  {
    case RpStore::DontCare: return 0;
    default: return PLAN_STORE;
  }
}

int color_slot(int color_pos, int target_index)
{
  return render_pass_backend_color_slot(d3d::get_driver_code().is(d3d::metal), target_index, color_pos);
}

struct SplitModeAssume
{
  int varId = -1;
  bool pinnedToNative = false;
  bool pinnedToSplit = false;
};

SplitModeAssume read_split_mode_assume()
{
  SplitModeAssume a;
  const int vid = VariableMap::getVariableId(RENDER_PASS_SPLIT_MODE_INTERVAL);
  const bool assumed = ShaderGlobal::is_var_assumed(vid);
  a.pinnedToNative = assumed && ShaderGlobal::get_interval_assumed_value(vid) == RENDER_PASS_SPLIT_MODE_NATIVE;
  a.pinnedToSplit = assumed && !a.pinnedToNative;
  if (VariableMap::isGlobVariablePresent(vid) && !assumed)
    a.varId = vid;
  return a;
}

bool banlisted_device()
{
  const DriverDesc &desc = d3d::get_driver_desc();
  return desc.caps.hasNativeRenderPassSubPasses && desc.issues.hasBrokenSubpasses;
}

struct ActiveState
{
  const Impl *pass = nullptr;
  uint32_t passNameHash = 0;
  uint32_t shapeHash = 0;
  int subpass = -1;
  const char *passName = nullptr;
  int prevSplitMode = 0;
  RenderPassTarget targets[MAX_TARGETS] = {};
  RenderPassArea area{};
};
ActiveState g_active;

void destroy_impl(Impl *p)
{
  if (!p)
    return;
  // The active pass is published by raw pointer, so destroying it leaves draws reading freed memory.
  if (g_active.pass == p)
  {
    logerr("ShaderRenderPass: render_pass '%s' is destroyed while it is still active; ending it", p->name.c_str());
    ShaderRenderPass::end();
  }
  if (p->rp)
    d3d::delete_render_pass(p->rp);
  delete p;
}

// Nothing scopes these binds to the pass, so a stale read stays bound as SRV while it is a target.
void unbind_split_reads(const Impl &p)
{
  for (int ri = 0; ri < p.maxReads; ++ri)
    d3d::set_tex(STAGE_PS, subpass_read_register(MAX_T_REGISTERS, ri), nullptr);
}

// A resolve source must not stay bound as a render target, so the caller drops the targets first.
void perform_split_resolves(const Impl &p)
{
  for (const Impl::Resolve &r : p.resolves)
  {
    BaseTexture *src = g_active.targets[r.src].resource.tex;
    BaseTexture *dst = g_active.targets[r.dst].resource.tex;
    if (src && dst)
      dst->update(src);
  }
}

void bind_split_subpass(const Impl &p, int subpass)
{
  const auto &sp = p.subpasses[subpass];

  unbind_split_reads(p);
  if (p.vrsEnabled)
    d3d::set_variable_rate_shading_texture(sp.vrsTarget == 0xFFFF ? nullptr : g_active.targets[sp.vrsTarget].resource.tex);

  // Slots need not be dense (on Metal they are target indexes), so the holes stay empty.
  int colorSlotCount = 0;
  RenderTarget colors[MAX_TARGETS] = {};
  for (auto [ci, target] : enumerate(sp.colors))
  {
    const int slot = color_slot(int(ci), target);
    colors[slot] = g_active.targets[target].resource;
    colorSlotCount = max(colorSlotCount, slot + 1);
  }

  RenderTarget depth{};
  DepthAccess depthAccess = DepthAccess::RW;
  if (sp.depthTarget != 0xFFFF)
  {
    depth = g_active.targets[sp.depthTarget].resource;
    depthAccess = sp.depthReadOnly ? DepthAccess::SampledRO : DepthAccess::RW;
  }
  auto loadsHere = [&p, subpass](int target) {
    return p.firstBindSubpass[target] == subpass && load_flags(p.targetPlan[target]) == RP_TA_LOAD_CLEAR;
  };
  auto clearScoped = [&p](int target) {
    const RenderPassTarget &t = g_active.targets[target];
    TextureInfo ti;
    t.resource.tex->getinfo(ti, t.resource.mip_level);
    const RenderPassArea &a = g_active.area;
    if (a.left == 0 && a.top == 0 && a.width == uint32_t(ti.w) && a.height == uint32_t(ti.h))
    {
      d3d::clear_rt(t.resource, t.clearValue);
      return;
    }
    if (get_tex_format_desc(p.targetFormats[target]).isDepth())
    {
      d3d::set_render_target(t.resource, DepthAccess::RW, {});
      d3d::setview(a.left, a.top, a.width, a.height, 0.f, 1.f);
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, t.clearValue.asDepth, t.clearValue.asStencil);
      return;
    }
    const ChannelDType channels = get_tex_format_desc(p.targetFormats[target]).mainChannelsType;
    const bool isInt = channels == ChannelDType::UINT || channels == ChannelDType::SINT;
    bool expressible = channels == ChannelDType::UNORM;
    if (!isInt)
      for (float v : t.clearValue.asFloat)
        expressible &= v >= 0.f && v <= 1.f;
    if (!expressible)
    {
      const char *what = "ShaderRenderPass: render_pass '%s' clears target '%s' inside a partial render area, which split execution "
                         "does through an 8-bit unorm clearview that cannot express this clear value; clear the whole target, or "
                         "load it and draw the value";
#if DAGOR_DBGLEVEL > 0
      DAG_FATAL(what, p.name.c_str(), p.targetNames[target].c_str());
#else
      static bool reported = false;
      if (!reported)
      {
        reported = true;
        logerr(what, p.name.c_str(), p.targetNames[target].c_str());
      }
#endif
    }
    auto to8 = [](float v) { return uint8_t(clamp(v, 0.f, 1.f) * 255.f + 0.5f); };
    auto int8 = [](uint32_t v) { return uint8_t(min(v, 255u)); };
    const ResourceClearValue &cv = t.clearValue;
    const E3DCOLOR color = isInt ? E3DCOLOR(int8(cv.asUint[0]), int8(cv.asUint[1]), int8(cv.asUint[2]), int8(cv.asUint[3]))
                                 : E3DCOLOR(to8(cv.asFloat[0]), to8(cv.asFloat[1]), to8(cv.asFloat[2]), to8(cv.asFloat[3]));
    RenderTarget one = t.resource;
    d3d::set_render_target({}, DepthAccess::RW, make_span_const(&one, 1));
    d3d::setview(a.left, a.top, a.width, a.height, 0.f, 1.f);
    d3d::clearview(CLEAR_TARGET, color, 0.f, 0);
  };
  for (int ci = 0; ci < int(sp.colors.size()); ++ci)
    if (loadsHere(sp.colors[ci]))
      clearScoped(sp.colors[ci]);
  if (sp.depthTarget != 0xFFFF && loadsHere(sp.depthTarget))
    clearScoped(sp.depthTarget);

  d3d::allow_render_pass_target_load();
  d3d::set_render_target(depth, depthAccess, dag::ConstSpan<RenderTarget>(colors, colorSlotCount));
  d3d::setview(g_active.area.left, g_active.area.top, g_active.area.width, g_active.area.height, g_active.area.minZ,
    g_active.area.maxZ);

  // Split execution binds the textures itself, so it has to state the render-target-to-SRV transition.
  for (int ri = 0; ri < int(sp.reads.size()); ++ri)
  {
    const RenderPassTarget &t = g_active.targets[sp.reads[ri]];
    BaseTexture *tex = t.resource.tex;
    d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    // The native path carries the mip into the render pass, so the split bind must read the same one.
    if (tex)
      tex->texmiplevel(t.resource.mip_level, t.resource.mip_level);
    d3d::set_tex(STAGE_PS, subpass_read_register(MAX_T_REGISTERS, ri), tex);
  }
}

struct ResolvePlacement
{
  int subpass = -1;
  int slot = 0;
};

void add_resolve_binds(Tab<RenderPassBind> &binds, const Impl &p, dag::ConstSpan<ResolvePlacement> placement, int subpass)
{
  for (int ri = 0; ri < int(p.resolves.size()); ++ri)
  {
    if (placement[ri].subpass != subpass)
      continue;
    RenderPassBind b{};
    b.target = uint32_t(p.driverTarget[p.resolves[ri].dst]);
    b.subpass = subpass;
    b.slot = placement[ri].slot;
    b.action = RenderPassTargetAction(RP_TA_SUBPASS_RESOLVE | RP_TA_LOAD_NO_CARE | RP_TA_STORE_WRITE);
    b.dependencyBarrier = RB_STAGE_PIXEL | RB_RW_RENDER_TARGET;
    binds.push_back(b);
  }
}

struct SubpassIntro
{
  dag::Vector<int> colors;
  dag::Vector<int> reads;
};
struct PassIntro
{
  dag::Vector<SubpassIntro> subpasses;
  dag::Vector<ResolvePair> resolves;
  dag::Vector<FramebufferId> framebuffers;
};
// Indirect: the returned spans point into an entry, which a later lookup must not move.
dag::VectorMap<uint32_t, eastl::unique_ptr<PassIntro>> g_intro;
// The dump address alone does not identify a load: a reload can reuse the same block.
uint32_t g_intro_generation = 0;
const void *g_intro_dump = nullptr;

const PassIntro *get_intro(Id id)
{
  const FoundPass f = find_pass(id);
  if (!f.rp)
    return nullptr;
  const uint32_t generation = shaderbindump::get_generation();
  const void *mainDump = shBinDumpOwner().getDumpV6();
  if (g_intro_dump != mainDump || g_intro_generation != generation)
  {
    g_intro.clear();
    g_intro_dump = mainDump;
    g_intro_generation = generation;
  }
  auto it = g_intro.find(uint32_t(id));
  if (it != g_intro.end())
    return it->second.get();

  auto intro = eastl::make_unique<PassIntro>();
  PassIntro &pi = *intro;
  pi.subpasses.resize(f.rp->subpasses.size());
  for (int si = 0; si < int(f.rp->subpasses.size()); ++si)
  {
    const auto &sp = f.rp->subpasses[si];
    SubpassIntro &out = pi.subpasses[si];
    out.colors.reserve(sp.colors.size());
    for (uint16_t c : sp.colors)
      out.colors.push_back(c);
    out.reads.reserve(sp.reads.size());
    for (uint16_t r : sp.reads)
      out.reads.push_back(r);
  }
  pi.resolves.reserve(f.rp->resolves.size());
  for (const auto &r : f.rp->resolves)
    pi.resolves.push_back(ResolvePair{int(r.srcTargetIndex), int(r.dstTargetIndex)});
  pi.framebuffers.reserve(f.rp->framebuffers.size());
  for (uint16_t li : f.rp->framebuffers)
    pi.framebuffers.push_back(FramebufferId(f.dump->framebufferLayouts[li].nameHash));

  return g_intro.emplace(uint32_t(id), eastl::move(intro)).first->second.get();
}

struct LayoutBinding
{
  const FramebufferLayout *layout = nullptr;
  int supportedIdx = -1;
};

LayoutBinding bind_layout(const ScriptedShadersBinDumpV6 &dump, const RenderPass &rp, FramebufferId fb)
{
  LayoutBinding res;
  if (fb == FramebufferId::Invalid)
    return res;
  for (int f = 0; f < int(rp.framebuffers.size()); ++f)
  {
    const FramebufferLayout &lay = dump.framebufferLayouts[rp.framebuffers[f]];
    if (lay.nameHash == uint32_t(fb))
    {
      res.layout = &lay;
      res.supportedIdx = f;
      break;
    }
  }
  return res;
}

TargetFormatInfo layout_target_format(const RenderPass &rp, const LayoutBinding &lb, int target)
{
  TargetFormatInfo info{0, 0};
  const int targetCount = int(rp.targets.size());
  const int entry = lb.supportedIdx * targetCount + target;
  if (!lb.layout || target < 0 || target >= targetCount || entry >= int(rp.framebufferTargetEntries.size()))
    return info;
  const uint16_t e = rp.framebufferTargetEntries[entry];
  if (e >= lb.layout->targets.size())
    return info;
  info.format = lb.layout->targets[e].format;
  info.samples = lb.layout->targets[e].samples;
  return info;
}

#if DAGOR_DBGLEVEL > 0
struct DumpShapes
{
  uint32_t generation = ~0u;
  dag::Vector<uint32_t> shapeHash;
};
dag::VectorMap<uint32_t, DumpShapes> g_dump_shapes;

const DumpShapes *get_dump_shapes(ShaderBindumpHandle dump)
{
  const ScriptedShadersBinDumpOwner &owner = get_shaders_dump_owner(dump);
  const ScriptedShadersBinDumpV6 *d = owner.getDumpV6();
  if (!d)
    return nullptr;
  DumpShapes &ds = g_dump_shapes[uint32_t(dump)];
  if (ds.generation == owner.generation && ds.shapeHash.size() == d->renderPasses.size())
    return &ds;
  ds.generation = owner.generation;
  ds.shapeHash.clear();
  ds.shapeHash.reserve(d->renderPasses.size());
  for (const auto &rp : d->renderPasses)
    ds.shapeHash.push_back(shape_hash(rp));
  return &ds;
}
#endif
} // namespace

bool check_render_passes_against_main_dump(const ScriptedShadersBinDumpOwner &owner)
{
  const ScriptedShadersBinDumpV6 *add = owner.getDumpV6();
  if (!add)
    return true;
  const bool declaresAnything = !add->renderPasses.empty() || !add->renderPassGroups.empty() || !add->framebufferLayouts.empty();
  const ScriptedShadersBinDumpV6 *mainD = main_dump();
  if (!mainD)
  {
    if (declaresAnything)
    {
      logerr("Failed to link additional dump, it declares render pass objects and the main dump has none.");
      return false;
    }
    return true;
  }

  for (const auto &lay : add->framebufferLayouts)
  {
    const FramebufferLayout *m = find_layout(*mainD, lay.nameHash);
    if (!m)
    {
      logerr("Failed to link additional dump, framebuffer_layout '%s' is not found in the main dump.", lay.name.c_str());
      return false;
    }
    bool same = m->targets.size() == lay.targets.size();
    for (int i = 0; same && i < int(lay.targets.size()); ++i)
      same = strcmp(m->targets[i].name.c_str(), lay.targets[i].name.c_str()) == 0 && m->targets[i].format == lay.targets[i].format &&
             m->targets[i].samples == lay.targets[i].samples;
    if (!same)
    {
      logerr("Failed to link additional dump, inconsistent framebuffer_layout '%s'.", lay.name.c_str());
      return false;
    }
  }

  for (const auto &rp : add->renderPasses)
  {
    const RenderPass *m = nullptr;
    for (const auto &c : mainD->renderPasses)
      if (c.nameHash == rp.nameHash)
      {
        m = &c;
        break;
      }
    if (!m)
    {
      logerr("Failed to link additional dump, render_pass '%s' is not found in the main dump.", rp.name.c_str());
      return false;
    }
    if (shape_hash(*m) != shape_hash(rp))
    {
      logerr("Failed to link additional dump, inconsistent shape of render_pass '%s'.", rp.name.c_str());
      return false;
    }
    bool sameFb = m->framebuffers.size() == rp.framebuffers.size();
    for (int i = 0; sameFb && i < int(rp.framebuffers.size()); ++i)
      sameFb = mainD->framebufferLayouts[m->framebuffers[i]].nameHash == add->framebufferLayouts[rp.framebuffers[i]].nameHash;
    if (!sameFb)
    {
      logerr("Failed to link additional dump, inconsistent supported framebuffer layouts of render_pass '%s'.", rp.name.c_str());
      return false;
    }
  }

  for (const auto &grp : add->renderPassGroups)
  {
    const RenderPassGroup *m = nullptr;
    for (const auto &c : mainD->renderPassGroups)
      if (c.nameHash == grp.nameHash)
      {
        m = &c;
        break;
      }
    if (!m)
    {
      logerr("Failed to link additional dump, subpass group '%s' is not found in the main dump.", grp.name.c_str());
      return false;
    }
    bool same = m->members.size() == grp.members.size();
    for (int i = 0; same && i < int(grp.members.size()); ++i)
      same = m->members[i].passNameHash == grp.members[i].passNameHash && m->members[i].subpassIdx == grp.members[i].subpassIdx;
    if (!same)
    {
      logerr("Failed to link additional dump, inconsistent members of subpass group '%s'.", grp.name.c_str());
      return false;
    }
  }

  return true;
}

Id ShaderRenderPass::find_id(const char *name)
{
  const Id id = Id(uint32_t(str_hash_fnv1a(name)));
  return find_pass(id).rp ? id : Id::Invalid;
}

FramebufferId ShaderRenderPass::find_framebuffer_id(const char *name)
{
  const uint32_t nameHash = uint32_t(str_hash_fnv1a(name));
  const ScriptedShadersBinDumpV6 *d = main_dump();
  if (d && find_layout(*d, nameHash))
    return FramebufferId(nameHash);
  return FramebufferId::Invalid;
}

int ShaderRenderPass::get_target_count(Id id)
{
  const RenderPass *rp = find_pass(id).rp;
  return rp ? int(rp->targets.size()) : 0;
}

const char *ShaderRenderPass::get_target_name(Id id, int i)
{
  const RenderPass *rp = find_pass(id).rp;
  return rp && i >= 0 && i < int(rp->targets.size()) ? rp->targets[i].name.c_str() : nullptr;
}

int ShaderRenderPass::get_target_slot(Id id, const char *name)
{
  const RenderPass *rp = find_pass(id).rp;
  if (!rp || !name)
    return -1;
  for (int i = 0; i < int(rp->targets.size()); ++i)
    if (strcmp(rp->targets[i].name.c_str(), name) == 0)
      return i;
  return -1;
}

bool ShaderRenderPass::is_target_multisampled(Id id, int i)
{
  const RenderPass *rp = find_pass(id).rp;
  return rp && i >= 0 && i < int(rp->targets.size()) && (rp->targets[i].flags & shader_layout::RP_TARGET_MULTISAMPLED);
}

dag::ConstSpan<ResolvePair> ShaderRenderPass::get_resolves(Id id)
{
  const PassIntro *pi = get_intro(id);
  if (!pi)
    return {};
  return make_span_const(pi->resolves.data(), pi->resolves.size());
}

int ShaderRenderPass::get_subpass_count(Id id)
{
  const RenderPass *rp = find_pass(id).rp;
  return rp ? int(rp->subpasses.size()) : 0;
}

SubpassInfo ShaderRenderPass::get_subpass_info(Id id, int subpass)
{
  SubpassInfo info;
  const RenderPass *rp = find_pass(id).rp;
  const PassIntro *pi = get_intro(id);
  if (!rp || !pi || subpass < 0 || subpass >= int(pi->subpasses.size()))
    return info;
  const SubpassIntro &si = pi->subpasses[subpass];
  const auto &sp = rp->subpasses[subpass];
  info.colors = make_span_const(si.colors.data(), si.colors.size());
  info.reads = make_span_const(si.reads.data(), si.reads.size());
  info.depth = sp.depthTargetIndex == 0xFFFF ? -1 : int(sp.depthTargetIndex);
  info.depthUse = (sp.flags & shader_layout::RP_SUBPASS_DEPTH_SAMPLED)     ? DepthUse::Sampled
                  : (sp.flags & shader_layout::RP_SUBPASS_DEPTH_READ_ONLY) ? DepthUse::Read
                                                                           : DepthUse::Write;
  info.vrs = sp.vrsTargetIndex == 0xFFFF ? -1 : int(sp.vrsTargetIndex);
  return info;
}

dag::ConstSpan<FramebufferId> ShaderRenderPass::get_pass_framebuffers(Id id)
{
  const PassIntro *pi = get_intro(id);
  if (!pi)
    return {};
  return make_span_const(pi->framebuffers.data(), pi->framebuffers.size());
}

FramebufferId ShaderRenderPass::get_only_framebuffer(Id id)
{
  const dag::ConstSpan<FramebufferId> fbs = get_pass_framebuffers(id);
  if (fbs.size() != 1)
  {
    DAG_FATAL("ShaderRenderPass::get_only_framebuffer: the render_pass with name hash 0x%08X supports %d framebuffer layouts, not one",
      uint32_t(id), int(fbs.size()));
    return FramebufferId::Invalid;
  }
  return fbs[0];
}

const char *ShaderRenderPass::get_framebuffer_name(FramebufferId fb)
{
  if (fb == FramebufferId::Invalid)
    return nullptr;
  const ScriptedShadersBinDumpV6 *d = main_dump();
  const FramebufferLayout *lay = d ? find_layout(*d, uint32_t(fb)) : nullptr;
  return lay ? lay->name.c_str() : nullptr;
}

TargetFormatInfo ShaderRenderPass::get_framebuffer_target_format(Id id, FramebufferId fb, int target)
{
  const FoundPass f = find_pass(id);
  if (!f.rp)
    return {0, 0};
  const LayoutBinding lb = bind_layout(*f.dump, *f.rp, fb);
  if (!lb.layout)
    return {0, 0};
  return layout_target_format(*f.rp, lb, target);
}

bool ShaderRenderPass::device_breaks_subpass_reads() { return banlisted_device(); }

ExecutionConstraint ShaderRenderPass::get_execution_constraint(Id id)
{
  const RenderPass *rp = find_pass(id).rp;
  if (!rp || !(rp->flags & shader_layout::RP_ALLOW_SPLIT))
    return ExecutionConstraint::NativeOnly;
  if (banlisted_device())
    return ExecutionConstraint::SplitOnly;
  // A dump pinned to split by an assume carries the SRV-lowered readers only.
  const SplitModeAssume assume = read_split_mode_assume();
  if (assume.pinnedToSplit && assume.varId < 0 && d3d::get_driver_desc().caps.hasNativeRenderPassSubPasses)
    return ExecutionConstraint::SplitOnly;
  return ExecutionConstraint::Any;
}

Targets::Entry *Targets::entryFor(const char *target)
{
  if (!target)
  {
    DAG_FATAL("ShaderRenderPass::Targets: null target name");
    return &entries[MAX_TARGETS - 1];
  }
  for (int i = 0; i < count; ++i)
    if (entries[i].name && strcmp(entries[i].name, target) == 0)
      return &entries[i];
  if (count >= MAX_TARGETS)
  {
    DAG_FATAL("ShaderRenderPass::Targets: more than %d targets bound, at '%s'", MAX_TARGETS, target);
    return &entries[MAX_TARGETS - 1];
  }
  entries[count].name = target;
  return &entries[count++];
}

Targets::Entry *Targets::entryFor(int slot)
{
  // -1 is the unbound slot of a name-addressed entry, so a negative slot must not take one over.
  if (slot < 0)
  {
    DAG_FATAL("ShaderRenderPass::Targets: negative target slot %d", slot);
    return &entries[MAX_TARGETS - 1];
  }
  for (int i = 0; i < count; ++i)
    if (entries[i].slot == slot)
      return &entries[i];
  if (count >= MAX_TARGETS)
  {
    DAG_FATAL("ShaderRenderPass::Targets: more than %d targets bound, at slot %d", MAX_TARGETS, slot);
    return &entries[MAX_TARGETS - 1];
  }
  entries[count].slot = slot;
  return &entries[count++];
}

Targets &Targets::tex(const char *target, BaseTexture *t, uint8_t mip, uint16_t layer)
{
  entryFor(target)->target.resource = RenderTarget{t, mip, layer};
  return *this;
}
Targets &Targets::clear(const char *target, ResourceClearValue v)
{
  Entry *e = entryFor(target);
  e->target.clearValue = v;
  e->hasClear = true;
  return *this;
}
Targets &Targets::tex(int slot, BaseTexture *t, uint8_t mip, uint16_t layer)
{
  entryFor(slot)->target.resource = RenderTarget{t, mip, layer};
  return *this;
}
Targets &Targets::clear(int slot, ResourceClearValue v)
{
  Entry *e = entryFor(slot);
  e->target.clearValue = v;
  e->hasClear = true;
  return *this;
}

Actions::Entry *Actions::entryFor(const char *target)
{
  if (!target)
  {
    DAG_FATAL("ShaderRenderPass::Actions: null target name");
    return &entries[MAX_TARGETS - 1];
  }
  for (int i = 0; i < count; ++i)
    if (strcmp(entries[i].name, target) == 0)
      return &entries[i];
  if (count >= MAX_TARGETS)
  {
    DAG_FATAL("ShaderRenderPass::Actions: more than %d targets planned, at '%s'", MAX_TARGETS, target);
    return &entries[MAX_TARGETS - 1];
  }
  entries[count].name = target;
  return &entries[count++];
}

Actions &Actions::load(const char *target, RpLoad a)
{
  Entry *e = entryFor(target);
  e->load = a;
  e->hasLoad = true;
  return *this;
}
Actions &Actions::store(const char *target, RpStore a)
{
  Entry *e = entryFor(target);
  e->store = a;
  e->hasStore = true;
  return *this;
}

bool Pass::resolveTargets(const Impl &p, const Targets &t, RenderPassTarget *out, bool *clear_set, const char **bad_name,
  int *bad_slot)
{
  const int n = int(p.targetNames.size());
  bool bound[MAX_TARGETS] = {};
  for (int i = 0; i < n; ++i)
  {
    out[i] = RenderPassTarget{};
    if (clear_set)
      clear_set[i] = false;
  }
  for (int i = 0; i < t.count; ++i)
  {
    const Targets::Entry &e = t.entries[i];
    int slot = e.slot;
    if (e.name)
    {
      slot = target_index(p, e.name);
      if (slot < 0)
      {
        *bad_name = e.name;
        return false;
      }
    }
    if (slot >= n)
    {
      *bad_name = nullptr;
      *bad_slot = slot;
      return false;
    }
    if (bound[slot])
    {
      *bad_name = p.targetNames[slot].c_str();
      return false;
    }
    out[slot] = e.target;
    if (clear_set)
      clear_set[slot] = e.hasClear;
    bound[slot] = true;
  }
  for (int i = 0; i < n; ++i)
    if (!bound[i] && !(p.targetIsVrs[i] && !p.vrsEnabled))
    {
      *bad_name = p.targetNames[i].c_str();
      return false;
    }
  return true;
}

Pass::Pass(Id id, FramebufferId fb, const char *debug_name, const Actions *plan, Execution execution)
{
  const bool force_split = execution == Execution::ForceSplit;
  const uint32_t nameHash = uint32_t(id);
  const FoundPass found = find_pass(id);
  if (!found.rp)
  {
    DAG_FATAL("ShaderRenderPass::Pass: the main shader dump declares no render_pass with name hash 0x%08X", nameHash);
    return;
  }
  const RenderPass &rp = *found.rp;
  const ScriptedShadersBinDumpV6 *rpDump = found.dump;
  const uint32_t shapeHash = shape_hash(rp);
  if (int(rp.targets.size()) > MAX_TARGETS)
  {
    DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' has %u targets, more than the %d a Targets object holds", rp.name.c_str(),
      rp.targets.size(), MAX_TARGETS);
    return;
  }

  const LayoutBinding lb = bind_layout(*rpDump, rp, fb);
  if (!lb.layout)
  {
    String supported;
    for (uint16_t li : rp.framebuffers)
      supported.aprintf(64, "%s%s", supported.empty() ? "" : ", ", rpDump->framebufferLayouts[li].name.c_str());
    DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' does not support the framebuffer layout with name hash 0x%08X; it supports %s",
      rp.name.c_str(), uint32_t(fb), supported.c_str());
    return;
  }

  // DAG_FATAL returns in dev builds, so every failure below must still leave the instance invalid.
  bool failed = false;
  eastl::unique_ptr<Impl> p(new Impl);
  p->name = rp.name.c_str();
  p->nameHash = rp.nameHash;
  p->shapeHash = shapeHash;
  p->layoutName = lb.layout->name.c_str();
  p->layoutNameHash = lb.layout->nameHash;
  p->targetNames.reserve(rp.targets.size());
  for (const auto &t : rp.targets)
    p->targetNames.push_back(eastl::string(t.name.c_str()));

  p->targetIsVrs.assign(rp.targets.size(), 0);
  p->targetPlan.assign(rp.targets.size(), 0);
  p->firstBindSubpass.assign(rp.targets.size(), -1);
  p->subpasses.resize(rp.subpasses.size());
  for (int si = 0; si < int(rp.subpasses.size()); ++si)
  {
    const auto &sp = rp.subpasses[si];
    Impl::Subpass &out = p->subpasses[si];
    out.colors.reserve(sp.colors.size());
    for (uint16_t c : sp.colors)
      out.colors.push_back(c);
    out.reads.reserve(sp.reads.size());
    for (uint16_t r : sp.reads)
      out.reads.push_back(r);
    out.depthTarget = sp.depthTargetIndex;
    out.depthReadOnly = (sp.flags & shader_layout::RP_SUBPASS_DEPTH_READ_ONLY) != 0;
    out.vrsTarget = sp.vrsTargetIndex;
    if (out.vrsTarget != 0xFFFF)
      p->targetIsVrs[out.vrsTarget] = 1;
    if (int(out.reads.size()) > p->maxReads)
      p->maxReads = int(out.reads.size());
    auto markFirst = [&p, si](uint16_t t) {
      if (t != 0xFFFF && p->firstBindSubpass[t] < 0)
        p->firstBindSubpass[t] = si;
    };
    for (uint16_t c : out.colors)
      markFirst(c);
    for (uint16_t r : out.reads)
      markFirst(r);
    markFirst(out.depthTarget);
    markFirst(out.vrsTarget);
  }
  p->resolves.reserve(rp.resolves.size());
  for (const auto &r : rp.resolves)
    p->resolves.push_back(Impl::Resolve{r.srcTargetIndex, r.dstTargetIndex});

  const int targetCount = int(p->targetNames.size());
  auto targetName = [&p](int ti) { return p->targetNames[ti].c_str(); };

  Tab<uint8_t> written(framemem_ptr()), observed(framemem_ptr());
  Tab<int> firstWriteSubpass(framemem_ptr()), firstObserveSubpass(framemem_ptr());
  written.resize(targetCount);
  observed.resize(targetCount);
  mem_set_0(written);
  mem_set_0(observed);
  firstWriteSubpass.resize(targetCount);
  firstObserveSubpass.resize(targetCount);
  for (int ti = 0; ti < targetCount; ++ti)
    firstWriteSubpass[ti] = firstObserveSubpass[ti] = -1;
  for (int si = 0; si < int(p->subpasses.size()); ++si)
  {
    const Impl::Subpass &s = p->subpasses[si];
    auto markWrite = [&](int ti) {
      written[ti] = 1;
      if (firstWriteSubpass[ti] < 0)
        firstWriteSubpass[ti] = si;
    };
    auto markObserve = [&](int ti) {
      observed[ti] = 1;
      if (firstObserveSubpass[ti] < 0)
        firstObserveSubpass[ti] = si;
    };
    for (uint16_t c : s.colors)
      markWrite(c);
    if (s.depthTarget != 0xFFFF)
    {
      if (s.depthReadOnly)
        markObserve(s.depthTarget);
      else
        markWrite(s.depthTarget);
    }
    for (uint16_t r : s.reads)
      markObserve(r);
  }

  Tab<uint8_t> isResolveSrc(framemem_ptr()), isResolveDst(framemem_ptr());
  isResolveSrc.resize(targetCount);
  isResolveDst.resize(targetCount);
  mem_set_0(isResolveSrc);
  mem_set_0(isResolveDst);
  for (const Impl::Resolve &r : p->resolves)
  {
    isResolveSrc[r.src] = 1;
    isResolveDst[r.dst] = 1;
  }

  for (int ti = 0; ti < targetCount; ++ti)
  {
    if (isResolveDst[ti])
      p->targetPlan[ti] = PLAN_LOAD_DONT_CARE | PLAN_STORE;
    else if (isResolveSrc[ti])
      p->targetPlan[ti] = PLAN_LOAD;
    else if (!written[ti])
      p->targetPlan[ti] = PLAN_LOAD | PLAN_NO_STORE;
    else
      p->targetPlan[ti] = PLAN_LOAD | PLAN_STORE;
  }

  if (plan)
    for (int oi = 0; oi < plan->count; ++oi)
    {
      const Actions::Entry &e = plan->entries[oi];
      const int slot = target_index(*p, e.name);
      if (slot < 0)
      {
        DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' has no target '%s' named in the boundary plan", p->name.c_str(), e.name);
        failed = true;
        continue;
      }
      if (isResolveDst[slot] || !written[slot])
      {
        failed = true;
        DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' target '%s' is %s, whose boundary behavior is fixed; the plan takes no "
                  "entry for it",
          p->name.c_str(), targetName(slot),
          isResolveDst[slot]     ? "a resolve destination"
          : p->targetIsVrs[slot] ? "a vrs attachment"
                                 : "a target the pass never writes");
      }
      uint16_t &a = p->targetPlan[slot];
      if (e.hasLoad)
        a = (a & ~PLAN_LOAD_MASK) | load_bit(e.load);
      if (e.hasStore)
        a = (a & ~PLAN_STORE_MASK) | store_bit(e.store);
    }

  for (int ti = 0; ti < targetCount; ++ti)
  {
    const uint16_t a = p->targetPlan[ti];
    if ((a & PLAN_LOAD_DONT_CARE) && observed[ti] && (firstWriteSubpass[ti] < 0 || firstWriteSubpass[ti] >= firstObserveSubpass[ti]))
    {
      failed = true;
      DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' observes target '%s' in subpass %d, no earlier subpass writes it, and the "
                "plan's load is DontCare, so the read is undefined",
        p->name.c_str(), targetName(ti), firstObserveSubpass[ti]);
    }
  }

  for (int ti = 0; ti < targetCount; ++ti)
  {
    if ((p->targetPlan[ti] & PLAN_CLEAR) && observed[ti] &&
        (firstWriteSubpass[ti] < 0 || firstWriteSubpass[ti] > firstObserveSubpass[ti]))
    {
      failed = true;
      DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' observes target '%s' in subpass %d before any subpass writes it, so the "
                "load runs on a read-only attachment and the plan cannot clear it",
        p->name.c_str(), targetName(ti), firstObserveSubpass[ti]);
    }
  }

  p->targetFormats.resize(targetCount);
  p->targetSamples.resize(targetCount);
  for (int ti = 0; ti < targetCount; ++ti)
  {
    const TargetFormatInfo tf = layout_target_format(rp, lb, ti);
    p->targetFormats[ti] = tf.format;
    p->targetSamples[ti] = tf.samples;
  }

  // The readers baked SubpassInput or SubpassInputMS from the shape, so a stale dump must not pass.
  for (int ti = 0; ti < int(rp.targets.size()) && ti < targetCount; ++ti)
  {
    const bool ms = (rp.targets[ti].flags & shader_layout::RP_TARGET_MULTISAMPLED) != 0;
    if (ms ? p->targetSamples[ti] <= 1 : p->targetSamples[ti] != 1)
    {
      failed = true;
      DAG_FATAL(
        "ShaderRenderPass::Pass: render_pass '%s' target '%s' is declared %smultisampled, but framebuffer layout '%s' gives it "
        "%d sample(s)",
        p->name.c_str(), targetName(ti), ms ? "" : "not ", p->layoutName.c_str(), int(p->targetSamples[ti]));
    }
  }

  for (const Impl::Resolve &r : p->resolves)
    if (p->targetFormats[r.src] != p->targetFormats[r.dst])
    {
      failed = true;
      DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' resolves '%s' into '%s', but framebuffer layout '%s' gives them different "
                "formats (0x%08X vs 0x%08X)",
        p->name.c_str(), targetName(r.src), targetName(r.dst), p->layoutName.c_str(), p->targetFormats[r.src],
        p->targetFormats[r.dst]);
    }

  {
    bool anyVrs = false;
    for (int ti = 0; ti < targetCount; ++ti)
      anyVrs |= p->targetIsVrs[ti] != 0;
    const bool devSupports = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
    p->vrsEnabled = anyVrs && devSupports;
    if (anyVrs && !p->vrsEnabled)
    {
      static bool reported = false;
      if (!reported)
      {
        reported = true;
        debug("ShaderRenderPass: render_pass '%s' declares a vrs attachment, but this device has no VRS attachment support; it is "
              "ignored and the pass renders at full rate",
          p->name.c_str());
      }
    }
  }
  p->driverTarget.resize(targetCount);
  for (int ti = 0; ti < targetCount; ++ti)
    p->driverTarget[ti] = (p->targetIsVrs[ti] && !p->vrsEnabled) ? -1 : p->driverTargetCount++;

  bool anyReads = false;
  for (const Impl::Subpass &s : p->subpasses)
    anyReads |= !s.reads.empty();

  const ExecutionConstraint constraint = get_execution_constraint(id);
  if (force_split && constraint == ExecutionConstraint::NativeOnly)
  {
    failed = true;
    DAG_FATAL("ShaderRenderPass::Pass: force_split on render_pass '%s', which does not declare allow_split", p->name.c_str());
  }
  if (constraint == ExecutionConstraint::NativeOnly && anyReads && banlisted_device())
  {
    failed = true;
    DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' reads inside the pass and does not declare allow_split, but this driver is "
              "banlisted for native subpass reads; gate this pass with the device caps and render your own fallback path instead",
      p->name.c_str());
  }
  p->split = constraint == ExecutionConstraint::SplitOnly || (force_split && constraint != ExecutionConstraint::NativeOnly);

  const SplitModeAssume assume = read_split_mode_assume();
  p->splitModeVarId = assume.varId;
  const bool bakedLowering = assume.varId < 0 && d3d::get_driver_desc().caps.hasNativeRenderPassSubPasses;
  if (bakedLowering && !p->split && assume.pinnedToSplit)
  {
    // The readers carry the SRV-lowered flavor only, so native execution would sample unfilled registers.
    failed = true;
    DAG_FATAL("ShaderRenderPass::Pass: the dump assumes render_pass_split_mode to split, but render_pass '%s' does not declare "
              "allow_split; recompile the dump with the assume deleted",
      p->name.c_str());
  }
  else if (bakedLowering && p->split)
  {
    if (assume.pinnedToNative)
    {
#if DAGOR_DBGLEVEL > 0
      failed = true;
      DAG_FATAL("ShaderRenderPass::Pass: render_pass '%s' runs split, but the dump assumes render_pass_split_mode to native; "
                "recompile the dump with the assume deleted",
        p->name.c_str());
#else
      static bool reported = false;
      if (!reported)
      {
        reported = true;
        logerr("ShaderRenderPass::Pass: render_pass '%s' runs split, but the dump assumes render_pass_split_mode to native; "
               "falling back to native execution. Recompile the dump with the assume deleted",
          p->name.c_str());
      }
      p->split = false;
#endif
    }
  }

  if (failed)
    return;

  if (!p->split)
  {
    Tab<RenderPassTargetDesc> descs(framemem_ptr());
    descs.resize(p->driverTargetCount);
    for (int ti = 0; ti < targetCount; ++ti)
      if (p->driverTarget[ti] >= 0)
      {
        const unsigned texcf = p->targetFormats[ti] | TEXCF_RTARGET | make_sample_count_flag(p->targetSamples[ti]);
        descs[p->driverTarget[ti]] = RenderPassTargetDesc{nullptr, texcf, false};
      }

    auto drvTarget = [&p](int target) { return uint32_t(p->driverTarget[target]); };

    struct StoreInfo
    {
      int slot = 0;
      int subpass = -1;
      bool used = false;
    };
    Tab<StoreInfo> stores(framemem_ptr());
    stores.resize(targetCount);
    Tab<uint8_t> vrsLoaded(framemem_ptr());
    vrsLoaded.resize(targetCount);
    mem_set_0(vrsLoaded);

    for (int si = 0; si < int(p->subpasses.size()); ++si)
    {
      const Impl::Subpass &s = p->subpasses[si];
      for (int ci = 0; ci < int(s.colors.size()); ++ci)
        stores[s.colors[ci]] = {ci, si, true};
      if (s.depthTarget != 0xFFFF && !s.depthReadOnly)
        stores[s.depthTarget] = {RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL, si, true};
    }
    Tab<ResolvePlacement> resolvePlacement(framemem_ptr());
    resolvePlacement.resize(p->resolves.size());
    for (int ri = 0; ri < int(p->resolves.size()); ++ri)
      if (stores[p->resolves[ri].src].used)
        resolvePlacement[ri] = {stores[p->resolves[ri].src].subpass, stores[p->resolves[ri].src].slot};

    for (int si = 0; si < int(p->subpasses.size()); ++si)
    {
      const Impl::Subpass &s = p->subpasses[si];
      if (s.depthTarget != 0xFFFF && s.depthReadOnly && !stores[s.depthTarget].used)
        stores[s.depthTarget] = {RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL, si, true};
    }
    for (int si = 0; si < int(p->subpasses.size()); ++si)
    {
      const Impl::Subpass &s = p->subpasses[si];
      for (int ri = 0; ri < int(s.reads.size()); ++ri)
        if (!stores[s.reads[ri]].used)
          stores[s.reads[ri]] = {ri, si, true};
      if (s.vrsTarget != 0xFFFF && p->vrsEnabled && !stores[s.vrsTarget].used)
        stores[s.vrsTarget] = {RenderPassExtraIndexes::RP_SLOT_VRS_TEXTURE, si, true};
    }

    // The load rides the first bind of the target and only that one, or the driver loads it twice. One
    // subpass can bind the same attachment twice: read-only depth is also in its own read list.
    Tab<uint8_t> loadEmitted(framemem_ptr());
    loadEmitted.resize(targetCount);
    mem_set_0(loadEmitted);
    auto loadAt = [&p, &loadEmitted](int target, int subpass) -> uint32_t {
      if (p->firstBindSubpass[target] != subpass || loadEmitted[target])
        return 0u;
      loadEmitted[target] = 1;
      return load_flags(p->targetPlan[target]);
    };

    Tab<RenderPassBind> binds(framemem_ptr());
    for (int si = 0; si < int(p->subpasses.size()); ++si)
    {
      const Impl::Subpass &s = p->subpasses[si];

      for (int ci = 0; ci < int(s.colors.size()); ++ci)
      {
        RenderPassBind b{};
        b.target = drvTarget(s.colors[ci]);
        b.subpass = si;
        b.slot = ci;
        b.action = RenderPassTargetAction(RP_TA_SUBPASS_WRITE | loadAt(s.colors[ci], si));
        b.dependencyBarrier = RB_STAGE_PIXEL | RB_RW_RENDER_TARGET;
        binds.push_back(b);
      }

      if (s.depthTarget != 0xFFFF)
      {
        RenderPassBind b{};
        b.target = drvTarget(s.depthTarget);
        b.subpass = si;
        b.slot = RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;
        // Read-only depth is SUBPASS_READ on the DS slot; KEEP would become a preserve attachment.
        b.action = RenderPassTargetAction((s.depthReadOnly ? RP_TA_SUBPASS_READ : RP_TA_SUBPASS_WRITE) | loadAt(s.depthTarget, si));
        // Read-only depth combines the DS state with an SRV read, so the barrier needs a shader stage.
        b.dependencyBarrier = RB_STAGE_PIXEL | (s.depthReadOnly ? RB_RO_CONSTANT_DEPTH_STENCIL_TARGET : RB_RW_DEPTH_STENCIL_TARGET);
        binds.push_back(b);
      }

      add_resolve_binds(binds, *p, make_span_const(resolvePlacement.data(), resolvePlacement.size()), si);

      if (s.vrsTarget != 0xFFFF && p->vrsEnabled)
      {
        RenderPassBind b{};
        b.target = drvTarget(s.vrsTarget);
        b.subpass = si;
        b.slot = RenderPassExtraIndexes::RP_SLOT_VRS_TEXTURE;
        b.action = RenderPassTargetAction(RP_TA_SUBPASS_VRS_READ | (vrsLoaded[s.vrsTarget] ? 0u : uint32_t(RP_TA_LOAD_READ)));
        b.dependencyBarrier = RB_RO_SRV | RB_STAGE_PIXEL;
        binds.push_back(b);
        vrsLoaded[s.vrsTarget] = 1;
      }

      for (int ri = 0; ri < int(s.reads.size()); ++ri)
      {
        RenderPassBind b{};
        b.target = drvTarget(s.reads[ri]);
        b.subpass = si;
        b.slot = ri;
        b.action = RenderPassTargetAction(RP_TA_SUBPASS_READ | loadAt(s.reads[ri], si));
        b.dependencyBarrier = RB_RO_SRV | RB_STAGE_PIXEL;
        binds.push_back(b);
      }
    }

    for (int ti = 0; ti < int(stores.size()); ++ti)
      if (stores[ti].used)
      {
        const uint32_t flags = store_flags(p->targetPlan[ti]);
        RenderPassBind b{};
        b.target = drvTarget(ti);
        b.subpass = RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END;
        b.slot = stores[ti].slot;
        b.action = RenderPassTargetAction(flags ? flags : uint32_t(RP_TA_STORE_NO_CARE));
        b.dependencyBarrier = RB_NONE;
        binds.push_back(b);
      }

    RenderPassDesc desc{};
    desc.debugName = debug_name ? debug_name : p->name.c_str();
    desc.targetCount = p->driverTargetCount;
    desc.bindCount = binds.size();
    desc.targetsDesc = descs.data();
    desc.binds = binds.data();

    p->rp = d3d::create_render_pass(desc);
    if (!p->rp)
    {
      failed = true;
      DAG_FATAL("ShaderRenderPass::Pass: the driver refused the render pass built from render_pass '%s'", p->name.c_str());
    }
  }
  if (failed)
    return;

  p->dumpGeneration = shaderbindump::get_generation();
  p->resetCounter = get_d3d_full_reset_counter();

  String formats;
  for (int ti = 0; ti < targetCount; ++ti)
    formats.aprintf(64, "%s%s=0x%08X x%d", ti ? ", " : "", targetName(ti), p->targetFormats[ti], int(p->targetSamples[ti]));
  debug("ShaderRenderPass: created '%s'%s%s on framebuffer layout '%s', %s execution, targets: %s", p->name.c_str(),
    debug_name ? " as " : "", debug_name ? debug_name : "", p->layoutName.c_str(), p->split ? "split" : "native", formats.c_str());

  impl = p.release();
}

Pass::Pass(Pass &&o) noexcept : impl(o.impl) { o.impl = nullptr; }

Pass &Pass::operator=(Pass &&o) noexcept
{
  if (this != &o)
  {
    destroy_impl(impl);
    impl = o.impl;
    o.impl = nullptr;
  }
  return *this;
}

Pass::~Pass()
{
  destroy_impl(impl);
  impl = nullptr;
}

int Pass::targetSlot(const char *name) const { return impl ? target_index(*impl, name) : -1; }

bool Pass::runsSplit() const { return impl && impl->split; }

bool Pass::vrsEnabled() const { return impl && impl->vrsEnabled; }

namespace
{
void revalidate_baked_plan(Impl &pi)
{
  const FoundPass live = find_pass(Id(pi.nameHash));
  if (!live.rp)
  {
    DAG_FATAL("ShaderRenderPass::begin: no loaded dump declares render_pass '%s' any more; recreate the pass instance",
      pi.name.c_str());
    return;
  }
  if (shape_hash(*live.rp) != pi.shapeHash)
  {
    DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' has a different shape in the loaded dump; recreate the pass instance",
      pi.name.c_str());
    return;
  }
  const LayoutBinding lb = bind_layout(*live.dump, *live.rp, FramebufferId(pi.layoutNameHash));
  if (!lb.layout)
  {
    DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' no longer supports framebuffer layout '%s'; recreate the pass instance",
      pi.name.c_str(), pi.layoutName.c_str());
    return;
  }
  for (int ti = 0; ti < int(pi.targetFormats.size()); ++ti)
  {
    const TargetFormatInfo tf = layout_target_format(*live.rp, lb, ti);
    if (tf.format != pi.targetFormats[ti] || tf.samples != pi.targetSamples[ti])
    {
      DAG_FATAL("ShaderRenderPass::begin: framebuffer layout '%s' now gives render_pass '%s' target '%s' format 0x%08X x%d instead of "
                "0x%08X x%d; recreate the pass instance",
        pi.layoutName.c_str(), pi.name.c_str(), pi.targetNames[ti].c_str(), tf.format, int(tf.samples), pi.targetFormats[ti],
        int(pi.targetSamples[ti]));
      return;
    }
  }
  if (get_execution_constraint(Id(pi.nameHash)) == ExecutionConstraint::SplitOnly && !pi.split)
  {
    DAG_FATAL("ShaderRenderPass::begin: this device now needs split execution for render_pass '%s', but the instance was built for "
              "native; recreate the pass instance",
      pi.name.c_str());
    return;
  }
  // A reload can add, drop or pin the interval. Where it pins, the readers carry one lowering only, so an
  // instance running the other mode has no reader left and the caller must build a new one.
  const SplitModeAssume assume = read_split_mode_assume();
  if (assume.varId < 0 && d3d::get_driver_desc().caps.hasNativeRenderPassSubPasses &&
      (pi.split ? assume.pinnedToNative : assume.pinnedToSplit))
  {
    DAG_FATAL("ShaderRenderPass::begin: the reloaded dump assumes render_pass_split_mode to %s, but render_pass '%s' runs %s; "
              "recreate the pass instance",
      assume.pinnedToNative ? "native" : "split", pi.name.c_str(), pi.split ? "split" : "native");
    return;
  }
  pi.splitModeVarId = assume.varId;
  pi.dumpGeneration = shaderbindump::get_generation();
  pi.resetCounter = get_d3d_full_reset_counter();
}

#if DAGOR_DBGLEVEL > 0
unsigned tex_cflg(BaseTexture *t)
{
  if (!t)
    return 0;
  TextureInfo info;
  t->getinfo(info);
  return info.cflg;
}

void verify_bound_textures(const Impl &pi, const bool *clear_set)
{
  for (int ti = 0; ti < int(pi.targetNames.size()); ++ti)
  {
    BaseTexture *tex = g_active.targets[ti].resource.tex;
    if (!tex)
    {
      if (!pi.targetIsVrs[ti])
        DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' target '%s' is bound to no texture", pi.name.c_str(),
          pi.targetNames[ti].c_str());
      continue;
    }
    const unsigned cflg = tex_cflg(tex);
    constexpr unsigned FORMAT_BITS = TEXFMT_MASK | TEXCF_SRGBWRITE | TEXCF_SRGBREAD;
    if ((cflg & FORMAT_BITS) != (pi.targetFormats[ti] & FORMAT_BITS))
      DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' target '%s' is 0x%08X in framebuffer layout '%s', but the bound texture is "
                "0x%08X",
        pi.name.c_str(), pi.targetNames[ti].c_str(), pi.targetFormats[ti] & FORMAT_BITS, pi.layoutName.c_str(), cflg & FORMAT_BITS);
    if (get_sample_count(cflg) != int(pi.targetSamples[ti]))
      DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' target '%s' has %d sample(s) in framebuffer layout '%s', but the bound "
                "texture has %d",
        pi.name.c_str(), pi.targetNames[ti].c_str(), int(pi.targetSamples[ti]), pi.layoutName.c_str(), get_sample_count(cflg));
    // A memoryless texture has no memory to read or to write, and split execution carries the contents
    // between the subpasses through memory.
    if (cflg & TEXCF_TRANSIENT)
    {
      const char *conflict = pi.split                                              ? "the pass runs split"
                             : load_flags(pi.targetPlan[ti]) == RP_TA_LOAD_READ    ? "the plan loads it"
                             : store_flags(pi.targetPlan[ti]) == RP_TA_STORE_WRITE ? "the plan stores it"
                                                                                   : nullptr;
      if (conflict)
        DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' target '%s' is bound to a TEXCF_TRANSIENT texture, but %s",
          pi.name.c_str(), pi.targetNames[ti].c_str(), conflict);
    }
    const bool plansClear = load_flags(pi.targetPlan[ti]) == RP_TA_LOAD_CLEAR;
    if (plansClear && !clear_set[ti])
      DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' clears target '%s', but no clear value was supplied for it",
        pi.name.c_str(), pi.targetNames[ti].c_str());
    if (!plansClear && clear_set[ti])
      DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' does not clear target '%s', but a clear value was supplied for it",
        pi.name.c_str(), pi.targetNames[ti].c_str());
  }
  if (!pi.split)
    return;
  for (const Impl::Resolve &r : pi.resolves)
    for (const int ti : {int(r.src), int(r.dst)})
    {
      const RenderTarget &rt = g_active.targets[ti].resource;
      if (rt.mip_level != 0 || rt.layer != 0)
        DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' runs split and resolves target '%s' through a whole-texture copy, so "
                  "it cannot be bound at mip %d layer %d",
          pi.name.c_str(), pi.targetNames[ti].c_str(), int(rt.mip_level), int(rt.layer));
    }
}
#endif
} // namespace

void ShaderRenderPass::begin(const Pass &p, const Targets &t, const RenderPassArea *area)
{
  G_ASSERTF_RETURN(!g_active.pass, , "ShaderRenderPass::begin: render_pass '%s' is still active", g_active.passName);
  g_active = {};
  G_ASSERTF_RETURN(p.valid(), , "ShaderRenderPass::begin: the pass instance was never created");
  Impl &pi = *p.impl;

  // A bindump reload and a device reset are release events, so this check cannot be dev-only.
  if (pi.dumpGeneration != shaderbindump::get_generation() || pi.resetCounter != get_d3d_full_reset_counter())
    revalidate_baked_plan(pi);

  const char *badName = nullptr;
#if DAGOR_DBGLEVEL > 0
  bool clearSet[MAX_TARGETS] = {};
#else
  bool *const clearSet = nullptr;
#endif
  int badSlot = -1;
  if (!Pass::resolveTargets(pi, t, g_active.targets, clearSet, &badName, &badSlot))
  {
    if (!badName)
      DAG_FATAL("ShaderRenderPass::begin: render_pass '%s' has no target slot %d; it declares %d targets", pi.name.c_str(), badSlot,
        int(pi.targetNames.size()));
    else
      DAG_FATAL(
        "ShaderRenderPass::begin: render_pass '%s' target '%s' is not declared by the shape, was left unbound, or was bound twice",
        pi.name.c_str(), badName);
    return;
  }

#if DAGOR_DBGLEVEL > 0
  verify_bound_textures(pi, clearSet);
#endif

  if (area)
    g_active.area = *area;
  else
  {
    // A VRS attachment is tile-granular, so it is not the pass extent; take the first target that is.
    TextureInfo info;
    info.w = info.h = 0;
    for (int ti = 0; ti < int(pi.targetNames.size()); ++ti)
      if (!pi.targetIsVrs[ti] && g_active.targets[ti].resource.tex)
      {
        g_active.targets[ti].resource.tex->getinfo(info, g_active.targets[ti].resource.mip_level);
        break;
      }
    g_active.area = RenderPassArea{0, 0, uint32_t(info.w), uint32_t(info.h), 0.f, 1.f};
  }

  g_active.pass = &pi;
  g_active.passNameHash = pi.nameHash;
  g_active.shapeHash = pi.shapeHash;
  g_active.subpass = 0;
  g_active.passName = pi.name.c_str();

  if (pi.splitModeVarId >= 0)
  {
    g_active.prevSplitMode = ShaderGlobal::get_int(pi.splitModeVarId);
    ShaderGlobal::set_int(pi.splitModeVarId, pi.split ? RENDER_PASS_SPLIT_MODE_SPLIT : RENDER_PASS_SPLIT_MODE_NATIVE);
  }

  if (pi.split)
    bind_split_subpass(pi, 0);
  else
  {
    RenderPassTarget drv[MAX_TARGETS];
    for (int ti = 0; ti < int(pi.targetNames.size()); ++ti)
      if (pi.driverTarget[ti] >= 0)
        drv[pi.driverTarget[ti]] = g_active.targets[ti];
    d3d::begin_render_pass(pi.rp, g_active.area, make_span_const(drv, pi.driverTargetCount));
  }
}

void ShaderRenderPass::next_subpass()
{
  G_ASSERTF_RETURN(g_active.pass, , "ShaderRenderPass::next_subpass: no pass is active");
  G_ASSERTF_RETURN(g_active.subpass + 1 < int(g_active.pass->subpasses.size()), ,
    "ShaderRenderPass::next_subpass: render_pass '%s' has no subpass %d", g_active.passName, g_active.subpass + 1);
  if (g_active.pass->split)
    bind_split_subpass(*g_active.pass, g_active.subpass + 1);
  else
    d3d::next_subpass();
  ++g_active.subpass;
}

void ShaderRenderPass::end()
{
  G_ASSERTF_RETURN(g_active.pass, , "ShaderRenderPass::end: no pass is active");
  const Impl &pi = *g_active.pass;
  if (pi.split)
  {
    d3d::set_render_target({}, DepthAccess::RW, {});
    perform_split_resolves(pi);
    unbind_split_reads(pi);
    if (pi.vrsEnabled)
      d3d::set_variable_rate_shading_texture(nullptr);
  }
  else
    d3d::end_render_pass();
  if (pi.splitModeVarId >= 0)
    ShaderGlobal::set_int(pi.splitModeVarId, g_active.prevSplitMode);
  g_active = {};
}

Scope::Scope(const Pass &p, const Targets &t, const RenderPassArea *area)
{
  const Impl *outer = g_active.pass;
  ShaderRenderPass::begin(p, t, area);
  subpassCount = (g_active.pass && g_active.pass != outer) ? int(g_active.pass->subpasses.size()) : 0;
}

void Scope::nextSubpass()
{
  if (subpassCount > 0)
    ShaderRenderPass::next_subpass();
}

Scope::~Scope()
{
  if (subpassCount <= 0)
    return;
  G_ASSERTF(g_active.subpass + 1 >= subpassCount, "ShaderRenderPass::Scope: only %d of %d subpasses of render_pass '%s' were stepped",
    g_active.subpass + 1, subpassCount, g_active.passName ? g_active.passName : "<none>");
  ShaderRenderPass::end();
}

#if DAGOR_DBGLEVEL > 0
Marker::Marker(Id id, int subpass)
{
  G_ASSERTF(!g_active.pass, "ShaderRenderPass::Marker: render_pass '%s' is active; a marker only names plain render targets",
    g_active.passName);
  prevNameHash = g_active.passNameHash;
  prevShapeHash = g_active.shapeHash;
  prevSubpass = g_active.subpass;
  prevName = g_active.passName;
  const FoundPass f = find_pass(id);
  if (!f.rp)
  {
    DAG_FATAL("ShaderRenderPass::Marker: no loaded shader dump declares a render_pass with name hash 0x%08X", uint32_t(id));
    return;
  }
  for (const auto &sp : f.rp->subpasses)
    if (!sp.reads.empty())
    {
      DAG_FATAL("ShaderRenderPass::Marker: render_pass '%s' reads inside the pass, which plain render targets cannot deliver; drive "
                "it with begin/end instead of marking it",
        f.rp->name.c_str());
      return;
    }
  if (subpass < 0 || subpass >= int(f.rp->subpasses.size()))
  {
    DAG_FATAL("ShaderRenderPass::Marker: render_pass '%s' has no subpass %d (it has %d)", f.rp->name.c_str(), subpass,
      int(f.rp->subpasses.size()));
    return;
  }
  g_active.passNameHash = f.rp->nameHash;
  g_active.shapeHash = shape_hash(*f.rp);
  g_active.subpass = subpass;
  g_active.passName = f.rp->name.c_str();
}

Marker::~Marker()
{
  g_active.passNameHash = prevNameHash;
  g_active.shapeHash = prevShapeHash;
  g_active.subpass = prevSubpass;
  g_active.passName = prevName;
}
#endif

ActivePass ShaderRenderPass::get_active_pass()
{
  ActivePass a;
  a.passNameHash = g_active.passName ? Id(g_active.passNameHash) : Id::Invalid;
  a.shapeHash = g_active.shapeHash;
  a.subpass = g_active.passName ? g_active.subpass : -1;
  a.passName = g_active.passName;
  return a;
}

#if DAGOR_DBGLEVEL > 0
void ShaderRenderPass::verify_active_pass(ShaderBindumpHandle dump, int pass_index, int expected_subpass, const char *shader_name)
{
  const DumpShapes *ds = get_dump_shapes(dump);
  const ScriptedShadersBinDumpV6 *d = get_shaders_dump_owner(dump).getDumpV6();
  if (!ds || !d || pass_index < 0 || pass_index >= int(d->renderPasses.size()))
    return;
  const auto &exp = d->renderPasses[pass_index];
  const uint32_t expShapeHash = ds->shapeHash[pass_index];
  if (g_active.passNameHash == exp.nameHash && g_active.shapeHash == expShapeHash && g_active.subpass == expected_subpass)
    return;
  // A one-subpass shape without reads is legal with no pass active: it writes plain render targets.
  const bool acceptsNoPass = exp.subpasses.size() == 1 && exp.subpasses[0].reads.empty();
  if (acceptsNoPass && !g_active.passName)
    return;
  const char *expSubName = expected_subpass < int(exp.subpasses.size()) ? exp.subpasses[expected_subpass].name.c_str() : "?";
  if (g_active.passNameHash == exp.nameHash && g_active.shapeHash != expShapeHash)
    DAG_FATAL("shader '%s': variant expects render_pass %s.%s (subpass %d), and the active pass has that name but a DIFFERENT shape - "
              "a dump reload reshaped it under the active pass.\n"
              "Recreate pass instances after a reload.",
      shader_name, exp.name.c_str(), expSubName, expected_subpass);
  else
    DAG_FATAL("shader '%s': variant expects render_pass %s.%s (subpass %d), but the active pass is %s (subpass %d).\n"
              "Draw opted-in shaders inside ShaderRenderPass::begin/end for that pass.",
      shader_name, exp.name.c_str(), expSubName, expected_subpass, g_active.passName ? g_active.passName : "<none>", g_active.subpass);
}

void ShaderRenderPass::verify_active_pass_group(ShaderBindumpHandle dump, int group_index, const char *shader_name)
{
  const DumpShapes *ds = get_dump_shapes(dump);
  const ScriptedShadersBinDumpV6 *d = get_shaders_dump_owner(dump).getDumpV6();
  if (!ds || !d || group_index < 0 || group_index >= int(d->renderPassGroups.size()))
    return;
  if (g_active.subpass < 0 || !g_active.passName)
    return;
  const auto &grp = d->renderPassGroups[group_index];
  auto memberPass = [d](uint32_t name_hash) -> int {
    for (int i = 0; i < int(d->renderPasses.size()); ++i)
      if (d->renderPasses[i].nameHash == name_hash)
        return i;
    return -1;
  };
  for (const auto &m : grp.members)
  {
    const int pi = memberPass(m.passNameHash);
    if (pi >= 0 && m.passNameHash == g_active.passNameHash && ds->shapeHash[pi] == g_active.shapeHash &&
        int(m.subpassIdx) == g_active.subpass)
      return;
  }

  String memberList;
  for (const auto &m : grp.members)
  {
    const int pi = memberPass(m.passNameHash);
    memberList.aprintf(64, "%s%s[%u]", memberList.empty() ? "" : ", ", pi >= 0 ? d->renderPasses[pi].name.c_str() : "?",
      unsigned(m.subpassIdx));
  }
  DAG_FATAL("shader '%s': variant is valid in subpass group %s (members: %s), but the active pass is %s (subpass %d).\n"
            "Add that pass.subpass to the group, or draw this shader in one of its members.",
    shader_name, grp.name.c_str(), memberList.c_str(), g_active.passName, g_active.subpass);
}
#endif
