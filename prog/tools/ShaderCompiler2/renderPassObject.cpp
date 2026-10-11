// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderPassObject.h"
#include "framebufferLayout.h"
#include "shlexterm.h"
#include "shsem.h"
#include "shSemCode.h"
#include "shTargetContext.h"
#include "shErrorReporting.h"
#include "globalConfig.h"
#include "hwSemantic.h"
#include "boolExpr.h"
#include "shLog.h"
#include "const3d.h"
#include "commonUtils.h"
#include "deSerializationContext.h"
#include <generic/dag_sort.h>
#include <EASTL/unique_ptr.h>

using namespace ShaderTerminal;
using namespace ShaderParser;

const char *RenderPassNameAdapter::getName(int id) { return get_shaders_de_serialization_ctx()->renderPassNameMap().getName(id); }
int RenderPassNameAdapter::addName(const char *name) { return get_shaders_de_serialization_ctx()->renderPassNameMap().addVarId(name); }

int RenderPassObject::Subpass::readPos(int target_idx) const
{
  for (int i = 0; i < reads.size(); ++i)
    if (reads[i] == target_idx)
      return i;
  return -1;
}

int RenderPassObject::Subpass::colorPos(int target_idx) const
{
  for (int i = 0; i < colors.size(); ++i)
    if (colors[i] == target_idx)
      return i;
  return -1;
}

// Reading the subpass's own depth is how a read-only depth binding is spelled.
bool RenderPassObject::Subpass::depthReadOnly() const { return depthSampled || (depthTargetIdx >= 0 && readPos(depthTargetIdx) >= 0); }

bool RenderPassObject::Subpass::writes(int target_idx) const
{
  return colorPos(target_idx) >= 0 || (depthTargetIdx == target_idx && !depthReadOnly());
}

bool RenderPassObject::Subpass::binds(int target_idx) const
{
  return colorPos(target_idx) >= 0 || depthTargetIdx == target_idx || readPos(target_idx) >= 0 || vrsTargetIdx == target_idx;
}

bool RenderPassObject::isResolveDst(int target_idx) const
{
  for (const Resolve &r : resolves)
    if (r.dstTargetIdx == target_idx)
      return true;
  return false;
}

bool RenderPassObject::isResolveSrc(int target_idx) const
{
  for (const Resolve &r : resolves)
    if (r.srcTargetIdx == target_idx)
      return true;
  return false;
}

bool RenderPassObject::isVrsTarget(int target_idx) const
{
  for (const Subpass &sp : subpasses)
    if (sp.vrsTargetIdx == target_idx)
      return true;
  return false;
}

int RenderPassObject::targetIndex(int name_id) const
{
  for (int i = 0; i < targets.size(); ++i)
    if (targets[i].name == name_id)
      return i;
  return -1;
}

int RenderPassObject::subpassIndex(int name_id) const
{
  for (int i = 0; i < subpasses.size(); ++i)
    if (subpasses[i].name == name_id)
      return i;
  return -1;
}

eastl::string RenderPassObject::subpassList() const
{
  eastl::string s;
  for (const Subpass &sp : subpasses)
    s.append_sprintf("%s%s", s.empty() ? "" : ", ", str(sp.name));
  return s;
}

bool RenderPassObject::hasInlineFormats() const
{
  for (const Target &t : targets)
    if (t.formatSamples <= 0)
      return false;
  return !targets.empty();
}

eastl::string RenderPassObject::shapeDesc() const
{
  eastl::string s(eastl::string::CtorSprintf{}, "pass %s split=%d", nameStr(), allowSplit ? 1 : 0);
  for (const Target &t : targets)
    s.append_sprintf("|t:%s ms=%d fmt=0x%08X x%d", str(t.name), t.multisampled ? 1 : 0, t.format, t.formatSamples);
  for (int fb : framebuffers)
    s.append_sprintf("|fb:%s", str(fb));
  for (const Resolve &r : resolves)
    s.append_sprintf("|rs:%d>%d", r.srcTargetIdx, r.dstTargetIdx);
  for (const Subpass &sp : subpasses)
  {
    s.append_sprintf("|sp:%s depth=%d/%d/%d vrs=%d", str(sp.name), sp.depthTargetIdx, sp.depthReadOnly() ? 1 : 0,
      sp.depthSampled ? 1 : 0, sp.vrsTargetIdx);
    for (int c : sp.colors)
      s.append_sprintf(" c:%d", c);
    for (int r : sp.reads)
      s.append_sprintf(" r:%d", r);
  }
  return s;
}

const RenderPassGroup::ColorSlot *RenderPassGroup::findColor(int name_id) const
{
  for (const ColorSlot &c : colorSlots)
    if (c.name == name_id)
      return &c;
  return nullptr;
}

int RenderPassGroup::readPos(int name_id) const
{
  for (int i = 0; i < int(reads.size()); ++i)
    if (reads[i].name == name_id)
      return i;
  return -1;
}

eastl::string RenderPassGroup::shapeDesc() const { return describe(nullptr); }

eastl::string RenderPassGroup::describe(const RenderPassTable *passes) const
{
  eastl::string s(eastl::string::CtorSprintf{}, "group %s depth=%d/%d/%d reads=%d split=%d", nameStr(), depthPresent ? 1 : 0,
    depthReadOnly ? 1 : 0, depthSampled ? 1 : 0, readableCount, allowSplit ? 1 : 0);
  for (const ColorSlot &c : colorSlots)
    s.append_sprintf("|c:%s@%d/%d", str(c.name), c.slot, c.inAllMembers ? 1 : 0);
  for (const ReadSlot &r : reads)
    s.append_sprintf("|r:%s@%d ms=%d", str(r.name), r.targetIdx, r.multisampled ? 1 : 0);
  for (const Member &m : members)
  {
    const RenderPassObject *p = passes ? passes->byId(m.passIdx) : nullptr;
    if (p && m.subpassIdx >= 0 && m.subpassIdx < p->subpasses.size())
      s.append_sprintf("|m:%s.%s", p->nameStr(), p->subpassName(m.subpassIdx));
    else
      s.append_sprintf("|m:%d.%d", m.passIdx, m.subpassIdx);
  }
  return s;
}

const char *RenderPassNamed::kindName(Kind k)
{
  switch (k)
  {
    case Kind::Pass: return "render_pass";
    case Kind::Group: return "subpass group";
    case Kind::FramebufferLayout: return "framebuffer_layout";
    default: return "<none>";
  }
}

RenderPassNamed render_pass_namespace_find(shc::TargetContext &ctx, int name_id)
{
  return render_pass_namespace_find(ctx.renderPasses(), ctx.renderPassGroups(), name_id);
}

RenderPassNamed render_pass_namespace_find(const RenderPassTable &passes, const RenderPassGroupTable &groups, int name_id)
{
  if (RenderPassObject *p = passes.findByName(name_id))
    return {RenderPassNamed::Kind::Pass, p, nullptr, nullptr};
  if (RenderPassGroup *g = groups.findByName(name_id))
    return {RenderPassNamed::Kind::Group, nullptr, g, nullptr};
  return {};
}

void RenderPassTable::link(Tab<RenderPassObject *> &loaded_passes, const RenderPassGroupTable &groups, Tab<int> &lnktbl)
{
  bindNames(loaded_passes);
  lnktbl.resize(loaded_passes.size());
  render_pass_link_named<RenderPassObject>(
    loaded_passes, RenderPassNamed::Kind::Pass, "is declared with different shapes", "render passes",
    [&](int name_id) { return render_pass_namespace_find(*this, groups, name_id); },
    [](const RenderPassNamed &taken) { return taken.pass; },
    [&](int i, RenderPassObject *existing) { lnktbl[i] = existing->tableIdx; },
    [&](int i, RenderPassObject *p) {
      adopt(p);
      lnktbl[i] = p->tableIdx;
    },
    [](const RenderPassObject *p) { return p->shapeDesc(); });
}

void RenderPassGroupTable::link(Tab<RenderPassGroup *> &loaded_groups, const RenderPassTable &passes, dag::ConstSpan<int> pass_lnktbl,
  Tab<int> &lnktbl)
{
  bindNames(loaded_groups);
  for (RenderPassGroup *g : loaded_groups)
    for (RenderPassGroup::Member &m : g->members)
    {
      G_ASSERT(m.passIdx >= 0 && m.passIdx < pass_lnktbl.size());
      m.passIdx = pass_lnktbl[m.passIdx];
    }
  lnktbl.resize(loaded_groups.size());
  render_pass_link_named<RenderPassGroup>(
    loaded_groups, RenderPassNamed::Kind::Group, "resolves to different contracts", "groups",
    [&](int name_id) { return render_pass_namespace_find(passes, *this, name_id); },
    [](const RenderPassNamed &taken) { return taken.group; },
    [&](int i, RenderPassGroup *existing) { lnktbl[i] = existing->tableIdx; },
    [&](int i, RenderPassGroup *g) {
      adopt(g);
      lnktbl[i] = g->tableIdx;
    },
    [&](const RenderPassGroup *g) { return g->describe(&passes); });
}

bool render_pass_split_mode_declared(shc::TargetContext &ctx)
{
  return ctx.globVars().getIntervalList().intervalExists(ctx.intervalNameMap().getNameId(RENDER_PASS_SPLIT_MODE_INTERVAL));
}

int render_pass_split_mode_ordinal(const char *value_name, shc::TargetContext &ctx)
{
  const int intervalNameId = ctx.intervalNameMap().getNameId(RENDER_PASS_SPLIT_MODE_INTERVAL);
  const int valueNameId = ctx.intervalNameMap().getNameId(value_name);
  const auto intervalIndex = ctx.globVars().getIntervalList().getIntervalIndex(intervalNameId);
  if (intervalIndex == INTERVAL_NOT_INIT || valueNameId < 0)
    return -1;
  const Interval *interval = ctx.globVars().getIntervalList().getInterval(intervalIndex);
  if (!interval)
    return -1;
  for (int i = 0; i < interval->getValueCount(); ++i)
    if (interval->getValue(i).getNameId() == valueNameId)
      return i;
  return -1;
}

static const Terminal *leftmost_terminal(bool_expr &e)
{
  bool_expr *be = &e;
  while (!be->value)
    be = be->a;
  and_expr *ae = be->value;
  while (!ae->value)
    ae = ae->a;
  not_expr &ne = *ae->value;
  if (ne.is_not)
    return ne.is_not;
  bool_value &v = *ne.value;
  if (v.expr)
    return leftmost_terminal(*v.expr);
  for (const Terminal *t : {v.hw ? (const Terminal *)v.hw->var : nullptr, (const Terminal *)v.true_value,
         (const Terminal *)v.false_value, (const Terminal *)v.interval_ident, (const Terminal *)v.bool_var,
         (const Terminal *)v.texture_name, (const Terminal *)v.shader, (const Terminal *)v.maybe, (const Terminal *)v.maybe_bool_var,
         (const Terminal *)v.two_sided, (const Terminal *)v.real_two_sided})
    if (t)
      return t;
  return nullptr;
}

bool render_pass_eval_build_condition(bool_expr &e, Parser &parser, shc::TargetContext &ctx, bool &out_value)
{
  // Not memoized to the node: a shape declaration is built once, so its conditions are never evaluated twice.
  eastl::vector<BoolExprEvalInstruction> bytecode;
  eastl::optional<bool> constValue;
  if (!compile_bool_expr_to_bytecode(bytecode, constValue, e, ctx) || !constValue)
  {
    const Terminal *at = leftmost_terminal(e);
    report_error(parser, at ? (const Symbol *)at : (const Symbol *)&e,
      "only hardware.* and true/false conditions are supported here; a render_pass or framebuffer_layout shape must be fully "
      "resolved at build time");
    return false;
  }
  out_value = *constValue;
  return true;
}

bool render_pass_target_has_native_subpasses()
{
#if _CROSS_TARGET_SPIRV
  return true;
#elif _CROSS_TARGET_METAL
  return shc::config().useIosToken; // one Metal binary serves tile iOS and emulated macOS
#else
  return false;
#endif
}

bool render_pass_target_is_metal()
{
#if _CROSS_TARGET_METAL
  return true; // not gated on -metalios: both Metal artifacts must stay shape-compatible
#else
  return false;
#endif
}

RenderPassGroupMemberRef render_pass_group_member(const RenderPassGroup::Member &m, const RenderPassTable &passes)
{
  const RenderPassObject *rp = passes.byId(m.passIdx);
  if (!rp || m.subpassIdx < 0 || m.subpassIdx >= rp->subpasses.size())
    return {};
  return {rp, &rp->subpasses[m.subpassIdx]};
}

int render_pass_color_slot(const RenderPassObject::Subpass &sp, int color_pos)
{
  return render_pass_backend_color_slot(render_pass_target_is_metal(), sp.colors[color_pos], color_pos);
}

int render_pass_scope_color_count(const RenderPassOptIn &scope, const RenderPassTable &passes)
{
  if (!scope.group)
    return scope.pass ? int(scope.pass->subpasses[scope.subpassIdx].colors.size()) : 0;
  int count = 0;
  for (const RenderPassGroup::Member &m : scope.group->members)
    if (const RenderPassGroupMemberRef mr = render_pass_group_member(m, passes))
      count = eastl::max(count, int(mr.subpass->colors.size()));
  return count;
}

bool render_pass_any_has_target(const RenderPassTable &passes, int target_name_id)
{
  for (const RenderPassObject *rp : passes.all())
    if (rp->hasLexicalTarget(target_name_id))
      return true;
  return false;
}

template <class Pred>
static bool render_pass_group_any_member(const RenderPassGroup &grp, const RenderPassTable &passes, Pred &&pred)
{
  for (const RenderPassGroup::Member &m : grp.members)
  {
    const RenderPassGroupMemberRef mr = render_pass_group_member(m, passes);
    if (mr && pred(*mr.pass))
      return true;
  }
  return false;
}

bool render_pass_group_target_is_resolve_dst(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes)
{
  return render_pass_group_any_member(grp, passes, [&](const RenderPassObject &p) {
    const int ti = p.targetIndex(target_name_id);
    return ti >= 0 && p.isResolveDst(ti);
  });
}

bool render_pass_group_target_is_vrs(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes)
{
  return render_pass_group_any_member(grp, passes, [&](const RenderPassObject &p) {
    const int ti = p.targetIndex(target_name_id);
    return ti >= 0 && p.isVrsTarget(ti);
  });
}

bool RenderPassOptIn::readsUnderAllowSplit() const
{
  G_ASSERT(*this);
  if (group)
    return group->allowSplit;
  return pass->allowSplit && !pass->subpasses[subpassIdx].reads.empty();
}

int RenderPassOptIn::readCount() const
{
  G_ASSERT(*this);
  return group ? int(group->reads.size()) : int(pass->subpasses[subpassIdx].reads.size());
}

int RenderPassOptIn::readWindowBase() const { return render_pass_read_window_base(readCount()); }

const char *RenderPassOptIn::targetAtRegister(int reg) const
{
  // The window fills top down: a read keeps its register however many reads follow it.
  if (!*this)
    return nullptr;
  if (group)
  {
    for (int i = 0; i < int(group->reads.size()); ++i)
      if (render_pass_read_register(i) == reg)
        return group->str(group->reads[i].name);
    return nullptr;
  }
  const RenderPassObject::Subpass &sp = pass->subpasses[subpassIdx];
  for (int i = 0; i < int(sp.reads.size()); ++i)
    if (render_pass_read_register(i) == reg)
      return pass->targetName(sp.reads[i]);
  return nullptr;
}

void render_pass_group_slot_divergence(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes,
  eastl::string &out_desc)
{
  const RenderPassObject *firstPass = nullptr;
  const RenderPassObject::Subpass *firstSp = nullptr;
  int firstSlot = -1;
  for (const RenderPassGroup::Member &m : grp.members)
  {
    const RenderPassGroupMemberRef mr = render_pass_group_member(m, passes);
    if (!mr)
      continue;
    const RenderPassObject &rp = *mr.pass;
    const RenderPassObject::Subpass &sp = *mr.subpass;
    for (int ci = 0; ci < sp.colors.size(); ++ci)
    {
      if (rp.targets[sp.colors[ci]].name != target_name_id)
        continue;
      const int slot = render_pass_color_slot(sp, ci);
      if (!firstPass)
      {
        firstPass = &rp;
        firstSp = &sp;
        firstSlot = slot;
      }
      else if (slot != firstSlot)
      {
        out_desc.append_sprintf("slot %d in '%s.%s' but slot %d in '%s.%s'", firstSlot, firstPass->nameStr(),
          firstPass->str(firstSp->name), slot, rp.nameStr(), rp.str(sp.name));
        return;
      }
    }
  }
}

namespace
{
struct RenderPassBuilder
{
  RenderPassObject &obj;
  Parser &parser;
  shc::TargetContext &ctx;
  bool &hadError;
  const Terminal *passAt = nullptr;

  bool simplifiedForm = true;
  RenderPassObject::Subpass implicitSubpass;
  const Terminal *implicitSubpassAt = nullptr;
  Tab<const Terminal *> formatAt;
  const Terminal *framebufferStatAt = nullptr;

  static constexpr char IMPLICIT_SUBPASS_NAME[] = "main";

  int intern(const char *name) { return ctx.renderPassNameMap().addVarId(name); }

  void reportErr(const BaseParNamespace::Symbol *t, const char *msg)
  {
    hadError = true;
    report_error(parser, t, "%s", msg);
  }

  template <typename... A>
  void reportErrf(const BaseParNamespace::Symbol *t, const char *fmt, const A &...a)
  {
    hadError = true;
    report_error(parser, t, fmt, a...);
  }

  int addTarget(const Terminal *at, const char *name, bool ms)
  {
    if (ShaderParser::render_pass_name_is_system_value(name))
      reportErrf(at,
        "render_pass '%s': target name '%s' is reserved: an output struct field semantic that begins with 'sv_' is a "
        "system value, not a target name",
        obj.nameStr(), name);
    if (streq(name, "optional") || streq(name, "src1"))
      reportErrf(at, "render_pass '%s': target name '%s' is reserved: it is an output struct field modifier", obj.nameStr(), name);
    if (obj.targets.size() >= RENDER_PASS_MAX_TARGETS)
      reportErrf(at, "render_pass '%s': target '%s' exceeds the %d targets a pass can bind", obj.nameStr(), name,
        RENDER_PASS_MAX_TARGETS);
    const int idx = obj.targets.size();
    obj.targets.push_back({});
    obj.targets.back().name = intern(name);
    obj.targets.back().multisampled = ms;
    formatAt.push_back(nullptr);
    return idx;
  }

  void applyFormat(int target_idx, SHTOK_ident *fmt_tok, SHTOK_ident *samples_tok, const Terminal *at)
  {
    if (formatAt[target_idx])
    {
      reportErrf(at, "render_pass '%s': target '%s' already has a format", obj.nameStr(), obj.targetName(target_idx));
      return;
    }
    formatAt[target_idx] = at;
    uint32_t fmt = 0;
    int samples = 1;
    if (!framebuffer_resolve_format_tokens("render_pass", obj.nameStr(), obj.targetName(target_idx), fmt_tok, samples_tok, parser, fmt,
          samples))
    {
      hadError = true;
      return;
    }
    obj.targets[target_idx].format = fmt;
    obj.targets[target_idx].formatSamples = samples;
  }

  struct PendingResolve
  {
    int src;
    SHTOK_ident *dstName;
    const Terminal *at;
  };
  Tab<PendingResolve> pendingResolves;

  void addResolve(int src, int dst, const Terminal *at)
  {
    if (dst == src)
    {
      reportErrf(at, "render_pass '%s': target '%s' resolves into itself", obj.nameStr(), obj.targetName(src));
      return;
    }
    if (obj.targets[dst].multisampled)
    {
      reportErrf(at, "render_pass '%s': resolve destination '%s' is declared multisampled; a resolve produces single-sampled contents",
        obj.nameStr(), obj.targetName(dst));
      return;
    }
    for (const RenderPassObject::Resolve &r : obj.resolves)
      if (r.srcTargetIdx == src || r.dstTargetIdx == dst)
      {
        reportErrf(at, "render_pass '%s': '%s' is already a resolve %s", obj.nameStr(),
          r.dstTargetIdx == dst ? obj.targetName(dst) : obj.targetName(src), r.dstTargetIdx == dst ? "destination" : "source");
        return;
      }
    obj.resolves.push_back({src, dst});
  }

  void applyResolve(int target_idx, target_resolve &tr, const Terminal *at, bool implicit)
  {
    if (!streq(tr.kw->text, "resolve"))
    {
      reportErrf(tr.kw, "render_pass '%s': target '%s': unknown '%s(...)' on its declaration (only resolve(TARGET) takes a target)",
        obj.nameStr(), obj.targetName(target_idx), tr.kw->text);
      return;
    }
    obj.targets[target_idx].multisampled = true;
    if (implicit)
    {
      int dst = obj.targetIndex(intern(tr.target_name->text));
      if (dst < 0)
        dst = addTarget(tr.target_name, tr.target_name->text, false);
      if (tr.format)
        applyFormat(dst, tr.format, tr.samples, tr.target_name);
      addResolve(target_idx, dst, at);
      return;
    }
    if (tr.format)
      reportErrf(tr.target_name,
        "render_pass '%s': resolve destination '%s' is declared by its own `target` statement; write its format there", obj.nameStr(),
        tr.target_name->text);
    pendingResolves.push_back({target_idx, tr.target_name, at});
  }

  void evalSubpassStat(subpass_stat &s, RenderPassObject::Subpass &sp)
  {
    if (s.if_stat)
    {
      evalSubpassIf(*s.if_stat, sp);
      return;
    }
    G_ASSERT(s.op);
    evalBindList(*s.op, s.bind, sp, false);
  }

  void evalBindList(SHTOK_ident &op_tok, dag::ConstSpan<subpass_bind *> binds, RenderPassObject::Subpass &sp, bool implicit)
  {
    const char *op = op_tok.text;
    const bool isColor = streq(op, "color");
    const bool isDepth = streq(op, "depth");
    const bool isSampledDepth = streq(op, "sampled_depth");
    const bool isRead = streq(op, "read");
    const bool isVrs = streq(op, "vrs");
    if (!isColor && !isDepth && !isSampledDepth && !isRead && !isVrs)
    {
      reportErr(&op_tok, implicit ? "unknown render_pass statement (expected color/depth/sampled_depth/vrs; a shape with target and "
                                    "subpass statements is declared with the 'multi_render_pass' keyword)"
                                  : "unknown subpass operation (expected color/depth/sampled_depth/read/vrs)");
      return;
    }
    if (implicit && isRead)
    {
      reportErr(&op_tok, "a simplified render_pass has no read(...) list; a shape that reads attachments is declared with the "
                         "'multi_render_pass' keyword and subpass blocks");
      return;
    }

    for (subpass_bind *bp : binds)
    {
      subpass_bind &b = *bp;
      int tIdx = obj.targetIndex(intern(b.target_name->text));
      if (tIdx < 0 && implicit)
        tIdx = addTarget(b.target_name, b.target_name->text, false);
      if (tIdx < 0)
      {
        reportErrf(b.target_name, "render_pass '%s': unknown target '%s' in subpass '%s' (declare it with `target %s;` first)",
          obj.nameStr(), b.target_name->text, obj.str(sp.name), b.target_name->text);
        continue;
      }
      if (b.tail && b.tail->format)
      {
        if (!implicit)
          reportErrf(b.target_name,
            "render_pass '%s': subpass '%s' declares roles only; the format of '%s' belongs on its `target` declaration",
            obj.nameStr(), obj.str(sp.name), b.target_name->text);
        else
          applyFormat(tIdx, b.tail->format, b.tail->samples, b.target_name);
      }
      if (b.qual)
      {
        if (!implicit)
          reportErrf(b.qual,
            "render_pass '%s': subpass '%s' declares roles only; the qualifier of '%s' belongs on its `target` declaration",
            obj.nameStr(), obj.str(sp.name), b.target_name->text);
        else
          reportErrf(b.qual,
            "render_pass '%s': target '%s' takes no qualifier here; a multisampled target is declared by its resolve, or in the "
            "full form",
            obj.nameStr(), b.target_name->text);
      }
      if (b.tail && b.tail->resolve)
      {
        if (!implicit)
          reportErrf(b.target_name,
            "render_pass '%s': subpass '%s' declares roles only; the resolve of '%s' belongs on its `target` declaration",
            obj.nameStr(), obj.str(sp.name), b.target_name->text);
        else if (isSampledDepth)
          reportErrf(b.target_name,
            "render_pass '%s': sampled_depth('%s') has nothing to resolve; a sampled depth is never written by the pass",
            obj.nameStr(), b.target_name->text);
        else if (isVrs)
          reportErrf(b.target_name,
            "render_pass '%s': vrs('%s') has nothing to resolve; nothing in the pass writes a shading-rate "
            "attachment",
            obj.nameStr(), b.target_name->text);
        else
          applyResolve(tIdx, *b.tail->resolve, b.target_name, true);
      }

      if (isColor)
      {
        if (sp.colorPos(tIdx) >= 0)
          reportErrf(b.target_name, "render_pass '%s': subpass '%s' binds '%s' as a color twice", obj.nameStr(), obj.str(sp.name),
            b.target_name->text);
        else
          sp.colors.push_back(tIdx);
      }
      else if (isDepth || isSampledDepth)
      {
        if (sp.depthTargetIdx >= 0)
          reportErrf(b.target_name, "render_pass '%s': subpass '%s' already has a depth attachment", obj.nameStr(), obj.str(sp.name));
        else
        {
          sp.depthTargetIdx = tIdx;
          sp.depthSampled = isSampledDepth;
        }
      }
      else if (isVrs)
      {
        if (sp.vrsTargetIdx >= 0)
          reportErrf(b.target_name, "render_pass '%s': subpass '%s' already has a vrs attachment", obj.nameStr(), obj.str(sp.name));
        else
          sp.vrsTargetIdx = tIdx;
      }
      else
      {
        if (sp.readPos(tIdx) >= 0)
          reportErrf(b.target_name, "render_pass '%s': subpass '%s' reads '%s' twice", obj.nameStr(), obj.str(sp.name),
            b.target_name->text);
        else
          sp.reads.push_back(tIdx);
      }
    }
  }

  void evalFramebufferStat(SHTOK_ident &op_tok, dag::ConstSpan<subpass_bind *> binds)
  {
    if (framebufferStatAt)
    {
      reportErrf(&op_tok, "render_pass '%s' already has a framebuffer statement; name every supported layout in one framebuffer(...)",
        obj.nameStr());
      return;
    }
    framebufferStatAt = &op_tok;
    Tab<int> flat;
    for (subpass_bind *bp : binds)
    {
      subpass_bind &b = *bp;
      if (b.tail || b.qual)
      {
        reportErrf(b.target_name,
          "render_pass '%s': framebuffer(...) names declared framebuffer layouts, so '%s' takes no format, no qualifier and no "
          "resolve",
          obj.nameStr(), b.target_name->text);
        continue;
      }
      if (streq(b.target_name->text, obj.nameStr()))
      {
        reportErrf(b.target_name, "render_pass '%s' names itself in framebuffer(...); a member is a declared framebuffer_layout",
          obj.nameStr());
        continue;
      }
      const FramebufferLayoutObject *layout = ctx.framebufferLayouts().findByName(b.target_name->text);
      if (!layout)
      {
        if (const RenderPassNamed other = render_pass_namespace_find(ctx, intern(b.target_name->text)))
          reportErrf(b.target_name, "render_pass '%s': '%s' is a %s; framebuffer(...) names framebuffer layouts", obj.nameStr(),
            b.target_name->text, other.kindName());
        else
          reportErrf(b.target_name,
            "render_pass '%s': unknown framebuffer_layout '%s'; a layout is known once its declaration "
            "precedes this render_pass",
            obj.nameStr(), b.target_name->text);
        continue;
      }
      framebuffer_layout_flatten(*layout, flat);
    }
    framebuffer_sort_names_by_text(flat, ctx.renderPassNameMap());
    obj.framebuffers.resize(flat.size());
    for (int i = 0; i < flat.size(); ++i)
      obj.framebuffers[i] = flat[i];
  }

  void evalSubpassIf(subpass_if &s, RenderPassObject::Subpass &sp)
  {
    render_pass_eval_if_chain(s, parser, ctx, hadError, [&](subpass_stat &st) { evalSubpassStat(st, sp); });
  }

  void evalStat(render_pass_stat &s)
  {
    if (s.if_stat)
    {
      evalIf(*s.if_stat);
      return;
    }
    if (s.op && streq(s.op->text, "framebuffer"))
    {
      evalFramebufferStat(*s.op, s.bind);
      return;
    }
    if (s.op)
    {
      if (!simplifiedForm)
      {
        reportErrf(s.op,
          "multi_render_pass '%s' binds its targets in `subpass NAME { ... }` blocks; a shape whose color()/depth() lists are its "
          "whole body is declared with the 'render_pass' keyword",
          obj.nameStr());
        return;
      }
      if (!implicitSubpassAt)
        implicitSubpassAt = s.op;
      implicitSubpass.name = intern(IMPLICIT_SUBPASS_NAME);
      evalBindList(*s.op, s.bind, implicitSubpass, true);
      return;
    }
    Terminal *kwTerm = s.sp_kw ? (Terminal *)s.sp_kw : (Terminal *)s.kw;
    G_ASSERT(kwTerm);
    const char *kw = kwTerm->text;
    if (!s.name)
    {
      if (streq(kw, "allow_split"))
        obj.allowSplit = true;
      else
        reportErr(s.kw, "unknown render_pass statement (expected allow_split)");
      return;
    }
    bool qualMs = false;
    if (s.qual)
    {
      if (!streq(kw, "target") || !streq(s.qual->text, "multisampled"))
      {
        reportErr(s.qual, "unknown target qualifier (expected '[multisampled] target NAME;')");
        return;
      }
      qualMs = true;
    }
    if (simplifiedForm)
    {
      reportErrf(kwTerm,
        "render_pass '%s' is the simplified form: its color()/depth() lists declare the targets, in order of first appearance. A "
        "shape with `target` and `subpass` statements is declared with the 'multi_render_pass' keyword",
        obj.nameStr());
      return;
    }
    if (streq(kw, "target"))
    {
      if (!s.sp_stat.empty())
      {
        reportErr(s.name, "target declaration cannot have a body");
        return;
      }
      if (obj.targetIndex(intern(s.name->text)) >= 0)
      {
        reportErrf(s.name, "render_pass '%s' already declares target '%s'", obj.nameStr(), s.name->text);
        return;
      }
      const int tIdx = addTarget(s.name, s.name->text, qualMs);
      if (s.tail && s.tail->format)
        applyFormat(tIdx, s.tail->format, s.tail->samples, s.name);
      if (s.tail && s.tail->resolve)
        applyResolve(tIdx, *s.tail->resolve, s.name, false);
    }
    else if (streq(kw, "subpass"))
    {
      if (obj.subpassIndex(intern(s.name->text)) >= 0)
      {
        reportErrf(s.name, "render_pass '%s' already declares subpass '%s'", obj.nameStr(), s.name->text);
        return;
      }
      if (s.sp_stat.empty())
      {
        reportErrf(s.name, "render_pass '%s': subpass '%s' must have a body", obj.nameStr(), s.name->text);
        return;
      }
      RenderPassObject::Subpass sp;
      sp.name = intern(s.name->text);
      for (subpass_stat *st : s.sp_stat)
        evalSubpassStat(*st, sp);
      validateSubpass(sp, s.name);
      obj.subpasses.push_back(eastl::move(sp));
    }
    else
      reportErr(s.kw, "unknown render_pass statement (expected target/subpass)");
  }

  void finish()
  {
    for (const PendingResolve &pr : pendingResolves)
    {
      const int dst = obj.targetIndex(intern(pr.dstName->text));
      if (dst < 0)
        reportErrf(pr.dstName, "render_pass '%s': resolve destination '%s' is not a declared target of this pass", obj.nameStr(),
          pr.dstName->text);
      else
        addResolve(pr.src, dst, pr.at);
    }
    if (implicitSubpassAt)
    {
      validateSubpass(implicitSubpass, implicitSubpassAt);
      obj.subpasses.push_back(eastl::move(implicitSubpass));
    }
    validatePass();
    validateFramebuffers();
    validateLayoutShapes();
    validateMetalShape();
  }

  void validateLayoutShapes()
  {
    if (hadError)
      return;
    const Terminal *at = framebufferStatAt ? framebufferStatAt : (implicitSubpassAt ? implicitSubpassAt : passAt);
    Tab<FramebufferLayoutRef> fbs;
    framebuffer_layouts_of_pass(obj, ctx.framebufferLayouts(), fbs);
    for (const FramebufferLayoutRef &fb : fbs)
    {
      for (int ti = 0; ti < obj.targets.size(); ++ti)
      {
        uint32_t fmt = 0;
        int samples = 0;
        if (!fb.targetFormat(obj.targets[ti].name, fmt, samples))
          continue;
        if (obj.targets[ti].multisampled && samples <= 1)
          reportErrf(at,
            "render_pass '%s': target '%s' is declared multisampled but framebuffer layout '%s' gives it x%d; a multisampled "
            "target has more than one sample in every supported layout",
            obj.nameStr(), obj.targetName(ti), fb.name(), samples);
        else if (!obj.targets[ti].multisampled && samples != 1 && simplifiedForm)
          reportErrf(at,
            "render_pass '%s': target '%s' is not multisampled but framebuffer layout '%s' gives it x%d; the simplified form "
            "infers multisampled from resolve(), so give the entry a resolve(...) or drop the sample count from the layout",
            obj.nameStr(), obj.targetName(ti), fb.name(), samples);
        else if (!obj.targets[ti].multisampled && samples != 1)
          reportErrf(at,
            "render_pass '%s': target '%s' is not declared multisampled but framebuffer layout '%s' gives it x%d; write "
            "`multisampled target %s;` or drop the sample count from the layout",
            obj.nameStr(), obj.targetName(ti), fb.name(), samples, obj.targetName(ti));
      }

      for (const RenderPassObject::Subpass &sp : obj.subpasses)
      {
        int firstIdx = -1, firstSamples = 0;
        auto bound = [&](int ti) {
          uint32_t fmt = 0;
          int samples = 0;
          if (!fb.targetFormat(obj.targets[ti].name, fmt, samples))
            return;
          if (firstIdx < 0)
          {
            firstIdx = ti;
            firstSamples = samples;
          }
          else if (samples != firstSamples)
            reportErrf(at,
              "render_pass '%s': subpass '%s' binds '%s' at x%d and '%s' at x%d in framebuffer layout '%s'; the color and depth "
              "attachments of one subpass all have the same sample count",
              obj.nameStr(), obj.str(sp.name), obj.targetName(firstIdx), firstSamples, obj.targetName(ti), samples, fb.name());
        };
        for (int c : sp.colors)
          bound(c);
        if (sp.depthTargetIdx >= 0)
          bound(sp.depthTargetIdx);
      }

      for (const RenderPassObject::Subpass &sp : obj.subpasses)
      {
        for (int c : sp.colors)
        {
          const FramebufferFormatInfo *fi = fb.targetFormatInfo(obj.targets[c].name);
          if (fi && fi->isDepth)
            reportErrf(at,
              "render_pass '%s': subpass '%s' binds '%s' as color, but framebuffer layout '%s' gives it the depth format '%s'",
              obj.nameStr(), obj.str(sp.name), obj.targetName(c), fb.name(), fi->token);
        }
        if (sp.depthTargetIdx >= 0)
        {
          const FramebufferFormatInfo *fi = fb.targetFormatInfo(obj.targets[sp.depthTargetIdx].name);
          if (fi && !fi->isDepth)
            reportErrf(at,
              "render_pass '%s': subpass '%s' binds '%s' as depth, but framebuffer layout '%s' gives it the color format '%s'",
              obj.nameStr(), obj.str(sp.name), obj.targetName(sp.depthTargetIdx), fb.name(), fi->token);
        }
        if (sp.vrsTargetIdx >= 0)
        {
          const FramebufferFormatInfo *fi = fb.targetFormatInfo(obj.targets[sp.vrsTargetIdx].name);
          if (fi && !streq(fi->token, "r8ui"))
            reportErrf(at,
              "render_pass '%s': subpass '%s' binds '%s' as the shading rate attachment, but framebuffer layout '%s' gives it '%s'; a "
              "shading rate attachment is r8ui",
              obj.nameStr(), obj.str(sp.name), obj.targetName(sp.vrsTargetIdx), fb.name(), fi->token);
        }
      }

      for (const RenderPassObject::Resolve &r : obj.resolves)
      {
        const FramebufferFormatInfo *si = fb.targetFormatInfo(obj.targets[r.srcTargetIdx].name);
        const FramebufferFormatInfo *di = fb.targetFormatInfo(obj.targets[r.dstTargetIdx].name);
        if (!si || !di || si->texfmt == di->texfmt)
          continue;
        reportErrf(at,
          "render_pass '%s': target '%s' is %s but resolves into '%s', which is %s in framebuffer layout '%s'; a hardware resolve "
          "does not convert formats",
          obj.nameStr(), obj.targetName(r.srcTargetIdx), si->token, obj.targetName(r.dstTargetIdx), di->token, fb.name());
      }
    }
  }

  void validateMetalShape()
  {
    if (hadError || obj.subpasses.empty() || !render_pass_target_is_metal())
      return;
    const Terminal *at = implicitSubpassAt ? implicitSubpassAt : passAt; //-V779 the guard folds to 'return' off-Metal
    // One shape serves both Metal artifacts, so what the tile path cannot express must not compile.
    for (const RenderPassObject::Subpass &sp : obj.subpasses)
      for (int r : sp.reads)
        for (const RenderPassObject::Subpass &other : obj.subpasses)
          if (other.depthTargetIdx == r)
          {
            reportErrf(at,
              "render_pass '%s': subpass '%s' reads '%s', which subpass '%s' binds as depth; Metal fetches color attachments only. "
              "Write the depth VALUE into its own single-channel color target (an r32f declared under `if (hardware.metal)`, "
              "written by the depth-writing subpass) and read that instead",
              obj.nameStr(), obj.str(sp.name), obj.targetName(r), obj.str(other.name));
            return;
          }

    const int vrs0 = obj.subpasses[0].vrsTargetIdx;
    for (const RenderPassObject::Subpass &sp : obj.subpasses)
      if (sp.vrsTargetIdx != vrs0)
      {
        reportErrf(at,
          "render_pass '%s': subpass '%s' has vrs attachment '%s' but subpass '%s' has '%s'; Metal applies one shading-rate map per "
          "pass, so every subpass of a shape names the same vrs target, or none does",
          obj.nameStr(), obj.subpassName(0), vrs0 >= 0 ? obj.targetName(vrs0) : "<none>", obj.str(sp.name),
          sp.vrsTargetIdx >= 0 ? obj.targetName(sp.vrsTargetIdx) : "<none>");
        return;
      }

    // The runtime drops an inactive vrs target from the driver pass, and Metal binds a color at its pass
    // target index, so a vrs target before a color shifts that color.
    for (const RenderPassObject::Subpass &sp : obj.subpasses)
    {
      if (sp.vrsTargetIdx < 0)
        continue;
      for (const RenderPassObject::Subpass &other : obj.subpasses)
        for (int c : other.colors)
          if (c > sp.vrsTargetIdx)
          {
            reportErrf(at,
              "render_pass '%s': vrs target '%s' is declared before color target '%s'; Metal binds a color at its target index, so "
              "declare vrs targets after every color target",
              obj.nameStr(), obj.targetName(sp.vrsTargetIdx), obj.targetName(c));
            return;
          }
    }

    int firstDepth = -1, firstDepthSp = -1;
    for (int i = 0; i < obj.subpasses.size(); ++i)
    {
      const RenderPassObject::Subpass &sp = obj.subpasses[i];
      if (sp.depthTargetIdx < 0)
        continue;
      if (firstDepth < 0)
      {
        firstDepth = sp.depthTargetIdx;
        firstDepthSp = i;
      }
      else if (sp.depthTargetIdx != firstDepth)
      {
        reportErrf(at,
          "render_pass '%s' uses different depth targets in subpasses '%s' and '%s'; Metal cannot swap the "
          "depth attachment between subpasses (tile-based rendering)",
          obj.nameStr(), obj.subpassName(firstDepthSp), obj.str(sp.name));
        return;
      }
    }
  }

  void validateFramebuffers()
  {
    const Terminal *at = framebufferStatAt ? framebufferStatAt : (implicitSubpassAt ? implicitSubpassAt : passAt);
    int withFormat = -1, withoutFormat = -1;
    for (int ti = 0; ti < obj.targets.size(); ++ti)
      (formatAt[ti] ? withFormat : withoutFormat) = ti;

    if (framebufferStatAt)
    {
      if (withFormat >= 0)
      {
        reportErrf(formatAt[withFormat],
          "render_pass '%s': target '%s' carries a format and the pass also has a framebuffer(...) statement; formats live in the "
          "declared layouts or on the target declarations, never in both",
          obj.nameStr(), obj.targetName(withFormat));
        return;
      }
      Tab<FramebufferLayoutRef> fbs;
      framebuffer_layouts_of_pass(obj, ctx.framebufferLayouts(), fbs);
      for (const FramebufferLayoutRef &fb : fbs)
        for (const RenderPassObject::Target &t : obj.targets)
          if (!fb.targetFormatInfo(t.name))
            reportErrf(framebufferStatAt, "render_pass '%s': target '%s' is not an entry of framebuffer_layout '%s'", obj.nameStr(),
              obj.str(t.name), fb.name());
      return;
    }

    if (withFormat < 0)
    {
      reportErrf(at,
        "render_pass '%s' has no framebuffer information: write `framebuffer(LAYOUT, ...);`, or a format on every target declaration "
        "(as in `target %s = argb8;`)",
        obj.nameStr(), obj.targets.empty() ? "NAME" : obj.targetName(0));
      return;
    }
    if (withoutFormat >= 0)
      reportErrf(at,
        "render_pass '%s': target '%s' carries a format but target '%s' does not; the inline form needs one on every target "
        "declaration, or name declared layouts with framebuffer(...)",
        obj.nameStr(), obj.targetName(withFormat), obj.targetName(withoutFormat));
  }

  void validatePass()
  {
    const Terminal *at = implicitSubpassAt ? implicitSubpassAt : passAt;
    for (int si = 0; si < obj.subpasses.size(); ++si)
    {
      const RenderPassObject::Subpass &sp = obj.subpasses[si];
      if (sp.vrsTargetIdx >= 0)
        for (const RenderPassObject::Subpass &other : obj.subpasses)
          if (other.colorPos(sp.vrsTargetIdx) >= 0 || other.depthTargetIdx == sp.vrsTargetIdx || other.readPos(sp.vrsTargetIdx) >= 0)
            reportErrf(at,
              "render_pass '%s': '%s' is the vrs attachment of subpass '%s' and also a color, depth or read target of subpass '%s'",
              obj.nameStr(), obj.targetName(sp.vrsTargetIdx), obj.str(sp.name), obj.str(other.name));
    }

    for (int ti = 0; ti < obj.targets.size(); ++ti)
    {
      bool anyRole = false, written = false, sampledDepth = false;
      for (const RenderPassObject::Subpass &sp : obj.subpasses)
      {
        anyRole |= sp.binds(ti);
        written |= sp.writes(ti);
        sampledDepth |= sp.depthSampled && sp.depthTargetIdx == ti;
      }
      if (sampledDepth && (written || obj.isResolveDst(ti)))
        reportErrf(at,
          "render_pass '%s': '%s' is a sampled_depth target, but the pass writes it; sampling reads memory, not tile memory - for "
          "in-pass data use read(...)",
          obj.nameStr(), obj.targetName(ti));
      if (!anyRole && !obj.isResolveDst(ti))
        reportErrf(at, "render_pass '%s': target '%s' has no role in any subpass", obj.nameStr(), obj.targetName(ti));
      if (obj.isResolveSrc(ti) && !written)
        reportErrf(at,
          "render_pass '%s': target '%s' resolves but is never written; a resolve source must be a color or a writable depth of "
          "some subpass",
          obj.nameStr(), obj.targetName(ti));
    }

    for (const RenderPassObject::Resolve &r : obj.resolves)
    {
      for (const RenderPassObject::Subpass &sp : obj.subpasses)
      {
        const char *role = sp.colorPos(r.dstTargetIdx) >= 0 ? "color" : (sp.depthTargetIdx == r.dstTargetIdx ? "depth" : nullptr);
        if (!role && sp.readPos(r.dstTargetIdx) >= 0)
          role = "read";
        if (!role && sp.vrsTargetIdx == r.dstTargetIdx)
          role = "vrs";
        if (role)
          reportErrf(at,
            "render_pass '%s': '%s' is the resolve destination of '%s' and also a %s target of subpass '%s'; a resolve runs at the "
            "end of the pass, so its destination can have no role inside the pass. Read the resolved data in a later pass",
            obj.nameStr(), obj.targetName(r.dstTargetIdx), obj.targetName(r.srcTargetIdx), role, obj.str(sp.name));
      }
    }
  }

  void validateSubpass(RenderPassObject::Subpass &sp, const Terminal *at)
  {
    if (sp.colors.empty() && sp.depthTargetIdx < 0 && sp.reads.empty())
      reportErrf(at, "render_pass '%s': subpass '%s' has no attachments", obj.nameStr(), obj.str(sp.name));
    const bool tooManyColors = sp.colors.size() > RenderPassObject::MAX_COLOR_SLOTS;
    if (tooManyColors)
      reportErrf(at, "render_pass '%s': subpass '%s' has %d color attachments, at most %d are supported", obj.nameStr(),
        obj.str(sp.name), int(sp.colors.size()), RenderPassObject::MAX_COLOR_SLOTS);
    // The read list is auto-bound into [MAX_T - count, MAX_T); a wider list makes the window base negative.
    if (sp.reads.size() > RenderPassObject::MAX_SUBPASS_READS)
      reportErrf(at, "render_pass '%s': subpass '%s' reads %d targets, at most %d are supported", obj.nameStr(), obj.str(sp.name),
        int(sp.reads.size()), RenderPassObject::MAX_SUBPASS_READS);
    for (int ci = 0; ci < sp.colors.size() && !tooManyColors; ++ci)
    {
      const int slot = render_pass_color_slot(sp, ci);
      if (slot >= RenderPassObject::MAX_COLOR_SLOTS)
        reportErrf(at,
          "render_pass '%s': color target '%s' of subpass '%s' resolves to slot %d, but only %d color slots exist (on Metal the slot "
          "is the pass target index, so declare color targets among the first %d targets)",
          obj.nameStr(), obj.targetName(sp.colors[ci]), obj.str(sp.name), slot, RenderPassObject::MAX_COLOR_SLOTS,
          RenderPassObject::MAX_COLOR_SLOTS);
    }
    for (int c : sp.colors)
      if (c == sp.depthTargetIdx)
        reportErrf(at, "render_pass '%s': subpass '%s' uses '%s' as both color and depth", obj.nameStr(), obj.str(sp.name),
          obj.targetName(c));
    for (int r : sp.reads)
    {
      if (sp.colorPos(r) >= 0)
        reportErrf(at, "render_pass '%s': subpass '%s' reads '%s', which it also writes as color", obj.nameStr(), obj.str(sp.name),
          obj.targetName(r));
      if (render_pass_target_is_metal() && r >= RenderPassObject::MAX_COLOR_SLOTS)
        reportErrf(at,
          "render_pass '%s': subpass '%s' reads '%s', which is pass target %d; on Metal an input attachment is fetched by the pass "
          "target index, which must be below %d, so declare read targets among the first %d targets",
          obj.nameStr(), obj.str(sp.name), obj.targetName(r), r, RenderPassObject::MAX_COLOR_SLOTS, RenderPassObject::MAX_COLOR_SLOTS);
    }
  }

  void evalIf(render_pass_if &s)
  {
    render_pass_eval_if_chain(s, parser, ctx, hadError, [&](render_pass_stat &st) { evalStat(st); });
  }
};

} // namespace

bool render_pass_claim_namespace_name(RenderPassNamed::Kind kind, Terminal *name, Parser &parser, shc::TargetContext &ctx)
{
  const int nameId = ctx.renderPassNameMap().addVarId(name->text);
  RenderPassNamed taken;
  if (kind == RenderPassNamed::Kind::FramebufferLayout)
  {
    if (FramebufferLayoutObject *l = ctx.framebufferLayouts().findByName(nameId))
      taken = {RenderPassNamed::Kind::FramebufferLayout, nullptr, nullptr, l};
  }
  else
    taken = render_pass_namespace_find(ctx, nameId);
  if (!taken)
    return true;
  if (taken.kind == kind)
    report_error(parser, name, "invalid redeclaration of %s '%s'", taken.kindName(), name->text);
  else
    report_error(parser, name, "'%s' is already declared as a %s; render passes and subpass groups share one namespace", name->text,
      taken.kindName());
  return false;
}

namespace
{
struct LexicalTargetCollector
{
  VarNameMap &names;
  dag::VectorSet<int> &out;

  void add(const char *n) { out.insert(names.addVarId(n)); }

  void resolve(target_resolve &tr)
  {
    if (tr.target_name)
      add(tr.target_name->text);
  }

  void binds(dag::ConstSpan<subpass_bind *> list)
  {
    for (subpass_bind *b : list)
      if (b && b->target_name)
      {
        add(b->target_name->text);
        if (b->tail && b->tail->resolve)
          resolve(*b->tail->resolve);
      }
  }

  void subpassStats(dag::ConstSpan<subpass_stat *> stats)
  {
    for (subpass_stat *s : stats)
    {
      if (!s)
        continue;
      if (s->if_stat)
        for (subpass_if *si = s->if_stat; si; si = si->else_if)
        {
          subpassStats(si->true_stat);
          subpassStats(si->false_stat);
        }
      else
        binds(s->bind);
    }
  }

  // Both sides of every `if` are walked: the roster must not depend on which hardware branch this build takes.
  void passStats(dag::ConstSpan<render_pass_stat *> stats)
  {
    for (render_pass_stat *s : stats)
    {
      if (!s)
        continue;
      if (s->if_stat)
      {
        for (render_pass_if *si = s->if_stat; si; si = si->else_if)
        {
          passStats(si->true_stat);
          passStats(si->false_stat);
        }
        continue;
      }
      if (s->op)
      {
        if (!streq(s->op->text, "framebuffer"))
          binds(s->bind);
        continue;
      }
      Terminal *kwTerm = s->sp_kw ? (Terminal *)s->sp_kw : (Terminal *)s->kw;
      if (!kwTerm || !s->name)
        continue;
      if (streq(kwTerm->text, "target"))
      {
        add(s->name->text);
        if (s->tail && s->tail->resolve)
          resolve(*s->tail->resolve);
      }
      else if (streq(kwTerm->text, "subpass"))
        subpassStats(s->sp_stat);
    }
  }
};
} // namespace

void ShaderParser::add_render_pass(render_pass_decl *rp, Parser &parser, shc::TargetContext &ctx)
{
  if (!rp)
    return;

  if (!render_pass_claim_namespace_name(RenderPassNamed::Kind::Pass, rp->name, parser, ctx))
    return;

  eastl::unique_ptr<RenderPassObject> obj(new RenderPassObject);
  obj->name = ctx.renderPassNameMap().addVarId(rp->name->text);
  obj->names = &ctx.renderPassNameMap();
  LexicalTargetCollector{ctx.renderPassNameMap(), obj->lexicalTargetNames}.passStats(rp->stat);

  bool hadError = false;
  RenderPassBuilder builder{*obj, parser, ctx, hadError, rp->name, rp->simple_kw != nullptr};
  for (render_pass_stat *st : rp->stat)
    builder.evalStat(*st);
  builder.finish();

  if (obj->subpasses.empty())
  {
    if (rp->simple_kw)
      report_error(parser, rp->name, "render_pass '%s' binds no attachments; write its color()/depth() lists in the body",
        obj->nameStr());
    else
      report_error(parser, rp->name,
        "multi_render_pass '%s' has no subpasses; declare `subpass NAME { ... }` blocks, or declare the one-subpass form with the "
        "'render_pass' keyword",
        obj->nameStr());
  }

  if (hadError || obj->subpasses.empty())
    return;

  ctx.renderPasses().registerPass(obj.get());
  obj.release();
}

namespace
{
struct ResolvedMember
{
  const RenderPassObject *pass;
  int subpassIdx;
  Terminal *at;
};

bool resolve_group_members(const any_of_members &list, const char *decl_name, Parser &parser, shc::TargetContext &ctx,
  Tab<ResolvedMember> &out)
{
  const eastl::string where =
    decl_name ? eastl::string(eastl::string::CtorSprintf{}, "subpass group '%s'", decl_name) : eastl::string("inline subpass group");
  if (!streq(list.kw->text, "any_of"))
  {
    report_error(parser, list.kw, "expected 'any_of' in %s, got '%s'", where.c_str(), list.kw->text);
    return false;
  }

  auto add = [&out](const RenderPassObject *pass, int sp_idx, Terminal *at) {
    for (const ResolvedMember &prev : out)
      if (prev.pass == pass && prev.subpassIdx == sp_idx)
        return;
    out.push_back({pass, sp_idx, at});
  };

  VarNameMap &names = ctx.renderPassNameMap();
  for (subpass_ref *m : list.member)
  {
    const char *name = m->pass_name->text;
    const int nameId = names.addVarId(name);
    if (m->subpass_name)
    {
      const RenderPassObject *pass = ctx.renderPasses().findByName(nameId);
      if (!pass)
      {
        const RenderPassNamed other = render_pass_namespace_find(ctx, nameId);
        if (other)
          report_error(parser, m->pass_name, "'%s' is a %s in %s; NAME.SUBPASS names a render_pass subpass", name, other.kindName(),
            where.c_str());
        else
          report_error(parser, m->pass_name, "unknown render_pass '%s' in %s; a pass is known once its declaration precedes this one",
            name, where.c_str());
        return false;
      }
      const int spIdx = pass->subpassIndex(names.addVarId(m->subpass_name->text));
      if (spIdx < 0)
      {
        report_error(parser, m->subpass_name, "render_pass '%s' has no subpass '%s' (its subpasses: %s)", name, m->subpass_name->text,
          pass->subpassList().c_str());
        return false;
      }
      add(pass, spIdx, m->pass_name);
      continue;
    }
    if (decl_name && streq(name, decl_name))
    {
      report_error(parser, m->pass_name, "membership cycle %s -> %s: a subpass group cannot be its own member", name, name);
      return false;
    }
    const RenderPassNamed member = render_pass_namespace_find(ctx, nameId);
    if (member.group)
    {
      for (const RenderPassGroup::Member &gm : member.group->members)
        add(ctx.renderPasses().byId(gm.passIdx), gm.subpassIdx, m->pass_name);
      continue;
    }
    if (!member.pass)
    {
      if (member)
        report_error(parser, m->pass_name, "'%s' is a %s in %s; a member is a render_pass or a subpass group", name, member.kindName(),
          where.c_str());
      else
        report_error(parser, m->pass_name,
          "unknown render_pass or subpass group '%s' in %s; a name is known once its declaration precedes this one", name,
          where.c_str());
      return false;
    }
    if (member.pass->subpasses.size() != 1)
    {
      report_error(parser, m->pass_name, "render_pass '%s' has %d subpasses; name the one %s takes (%s.SUBPASS)", name,
        int(member.pass->subpasses.size()), where.c_str(), name);
      return false;
    }
    add(member.pass, 0, m->pass_name);
  }

  if (out.empty())
  {
    report_error(parser, list.kw, "%s has no members", where.c_str());
    return false;
  }
  fast_sort(out, [](const ResolvedMember &a, const ResolvedMember &b) {
    const int byPass = strcmp(a.pass->nameStr(), b.pass->nameStr());
    return byPass != 0 ? byPass < 0 : a.subpassIdx < b.subpassIdx;
  });
  return true;
}

eastl::unique_ptr<RenderPassGroup> build_group(const char *name, dag::ConstSpan<ResolvedMember> resolved, Parser &parser,
  shc::TargetContext &ctx)
{
  eastl::unique_ptr<RenderPassGroup> grp(new RenderPassGroup);
  grp->name = ctx.renderPassNameMap().addVarId(name);
  grp->names = &ctx.renderPassNameMap();

  // One artifact serves every member, so a target name and its platform slot must map 1:1 in all of them.
  struct SlotOwner
  {
    int name;
    const ResolvedMember *member;
  };
  Tab<SlotOwner> slotOwners;
  Tab<int> slotMembers;
  for (const ResolvedMember &rm : resolved)
  {
    const RenderPassObject::Subpass &sp = rm.pass->subpasses[rm.subpassIdx];
    for (int ci = 0; ci < sp.colors.size(); ++ci)
    {
      const int tName = rm.pass->targets[sp.colors[ci]].name;
      const int slot = render_pass_color_slot(sp, ci);

      if (slot >= slotOwners.size())
        slotOwners.resize(slot + 1, SlotOwner{-1, nullptr});
      if (slotOwners[slot].member && slotOwners[slot].name != tName)
      {
        const RenderPassObject &owner = *slotOwners[slot].member->pass;
        report_error(parser, rm.at,
          "subpass group '%s' is not layout-compatible: slot %d carries target '%s' in '%s.%s' but target '%s' in '%s.%s'",
          grp->nameStr(), slot, grp->str(slotOwners[slot].name), owner.nameStr(),
          owner.subpassName(slotOwners[slot].member->subpassIdx), grp->str(tName), rm.pass->nameStr(), rm.pass->str(sp.name));
        return nullptr;
      }
      slotOwners[slot] = {tName, &rm};

      int knownIdx = -1;
      for (int k = 0; k < grp->colorSlots.size(); ++k)
        if (grp->colorSlots[k].name == tName)
        {
          knownIdx = k;
          break;
        }
      if (knownIdx >= 0)
      {
        // A target the members place differently is dropped from the contract instead of killing the
        // group: only a shader that writes it needs an agreed slot.
        if (grp->colorSlots[knownIdx].slot != slot)
          grp->colorSlots[knownIdx].slot = -1;
        ++slotMembers[knownIdx];
      }
      else
      {
        RenderPassGroup::ColorSlot cs;
        cs.name = tName;
        cs.slot = slot;
        grp->colorSlots.push_back(eastl::move(cs));
        slotMembers.push_back(1);
      }
    }
  }

  for (int k = 0; k < grp->colorSlots.size(); ++k)
    grp->colorSlots[k].inAllMembers = slotMembers[k] == resolved.size();

  // One compiled variant declares one field type per slot, so the HLSL class of a target must agree
  // across every member's supported layouts.
  Tab<Tab<FramebufferLayoutRef>> memberLayouts;
  memberLayouts.resize(resolved.size());
  for (int mi = 0; mi < resolved.size(); ++mi)
    framebuffer_layouts_of_pass(*resolved[mi].pass, ctx.framebufferLayouts(), memberLayouts[mi]);
  for (const RenderPassGroup::ColorSlot &cs : grp->colorSlots)
  {
    const FramebufferFormatInfo *first = nullptr;
    const ResolvedMember *firstFrom = nullptr;
    const char *firstLayout = nullptr;
    for (int mi = 0; mi < resolved.size(); ++mi)
    {
      const ResolvedMember &rm = resolved[mi];
      for (const FramebufferLayoutRef &fb : memberLayouts[mi])
      {
        const FramebufferFormatInfo *fi = fb.targetFormatInfo(cs.name);
        if (!fi)
          continue;
        if (!first)
        {
          first = fi;
          firstFrom = &rm;
          firstLayout = fb.name();
          continue;
        }
        if (fi->cls != first->cls)
        {
          if (firstFrom->pass == rm.pass)
            report_error(parser, rm.at,
              "subpass group '%s' is not format-compatible: layouts '%s' and '%s' of member '%s' disagree on target '%s' - it is %s "
              "(%s class) in one and %s (%s class) in the other; one compiled field has one class",
              grp->nameStr(), firstLayout, fb.name(), rm.pass->nameStr(), grp->str(cs.name), first->token,
              framebuffer_format_class_name(first->cls), fi->token, framebuffer_format_class_name(fi->cls));
          else
            report_error(parser, rm.at,
              "subpass group '%s' is not format-compatible: target '%s' is %s (%s class) in framebuffer layout '%s' of '%s' but %s "
              "(%s class) in framebuffer layout '%s' of '%s'; one compiled field has one class",
              grp->nameStr(), grp->str(cs.name), first->token, framebuffer_format_class_name(first->cls), firstLayout,
              firstFrom->pass->nameStr(), fi->token, framebuffer_format_class_name(fi->cls), fb.name(), rm.pass->nameStr());
          return nullptr;
        }
      }
    }
  }

  {
    const RenderPassObject::Subpass &sp0 = resolved[0].pass->subpasses[resolved[0].subpassIdx];
    grp->depthPresent = sp0.depthTargetIdx >= 0;
    grp->depthReadOnly = sp0.depthReadOnly();
    grp->depthSampled = sp0.depthSampled;
    for (const ResolvedMember &rm : resolved)
    {
      const RenderPassObject::Subpass &sp = rm.pass->subpasses[rm.subpassIdx];
      const bool present = sp.depthTargetIdx >= 0;
      const bool ro = sp.depthReadOnly();
      const bool sampled = sp.depthSampled;
      if (present != grp->depthPresent || (present && (ro != grp->depthReadOnly || sampled != grp->depthSampled)))
      {
        auto roleName = [](bool present, bool ro, bool sampled) {
          return !present ? "no depth" : sampled ? "sampled_depth" : ro ? "read-only depth" : "writable depth";
        };
        report_error(parser, rm.at, "subpass group '%s' is not layout-compatible: '%s.%s' has %s but '%s.%s' has %s", grp->nameStr(),
          resolved[0].pass->nameStr(), resolved[0].pass->str(sp0.name),
          roleName(grp->depthPresent, grp->depthReadOnly, grp->depthSampled), rm.pass->nameStr(), rm.pass->str(sp.name),
          roleName(present, ro, sampled));
        return nullptr;
      }
    }
  }

  // A read bakes its input attachment ordinal into the artifact: the shorter read() list is a prefix of
  // the longer, by target name, pass target index and sample count. The readable set is the shortest list.
  {
    struct UnifiedRead
    {
      int name;
      int targetIdx;
      bool multisampled;
      const ResolvedMember *from;
    };
    Tab<UnifiedRead> unified;
    int minReads = INT_MAX;
    for (const ResolvedMember &rm : resolved)
      minReads = eastl::min(minReads, int(rm.pass->subpasses[rm.subpassIdx].reads.size()));

    for (const ResolvedMember &rm : resolved)
    {
      const RenderPassObject::Subpass &sp = rm.pass->subpasses[rm.subpassIdx];
      for (int ri = 0; ri < sp.reads.size(); ++ri)
      {
        const int tIdx = sp.reads[ri];
        const int tName = rm.pass->targets[tIdx].name;
        const bool tMs = rm.pass->targetIsMs(tIdx);
        if (ri >= unified.size())
        {
          unified.push_back(UnifiedRead{tName, tIdx, tMs, &rm});
          continue;
        }
        if (ri >= minReads)
          continue;
        const ResolvedMember &first = *unified[ri].from;
        const RenderPassObject::Subpass &firstSp = first.pass->subpasses[first.subpassIdx];
        if (unified[ri].name != tName || unified[ri].targetIdx != tIdx)
        {
          report_error(parser, rm.at,
            "subpass group '%s' is not read-compatible: read %d is target '%s' (pass target index %d) in '%s.%s' but target "
            "'%s' (pass target index %d) in '%s.%s'; members must read a common prefix so that one compiled read resolves to the "
            "same attachment in all of them",
            grp->nameStr(), ri, grp->str(unified[ri].name), unified[ri].targetIdx, first.pass->nameStr(),
            first.pass->str(firstSp.name), grp->str(tName), tIdx, rm.pass->nameStr(), rm.pass->str(sp.name));
          return nullptr;
        }
        if (unified[ri].multisampled != tMs)
        {
          report_error(parser, rm.at,
            "subpass group '%s' is not read-compatible: target '%s' is %smultisampled in '%s.%s' but %smultisampled in "
            "'%s.%s'; that read is SubpassInput in one and SubpassInputMS in the other, and a shader declares one type. Split "
            "the group by sample count and select between the two with the interval the readers already branch on",
            grp->nameStr(), grp->str(tName), unified[ri].multisampled ? "" : "not ", first.pass->nameStr(),
            first.pass->str(firstSp.name), tMs ? "" : "not ", rm.pass->nameStr(), rm.pass->str(sp.name));
          return nullptr;
        }
      }
    }

    grp->readableCount = unified.empty() ? 0 : minReads;
    for (const UnifiedRead &ur : unified)
    {
      RenderPassGroup::ReadSlot rs;
      rs.name = ur.name;
      rs.targetIdx = ur.targetIdx;
      rs.multisampled = ur.multisampled;
      grp->reads.push_back(eastl::move(rs));
    }

    if (!unified.empty())
    {
      grp->allowSplit = resolved[0].pass->allowSplit;
      for (const ResolvedMember &rm : resolved)
      {
        if (rm.pass->allowSplit != grp->allowSplit)
        {
          report_error(parser, rm.at,
            "subpass group '%s' is not read-compatible: '%s' is %sallow_split but '%s' is %sallow_split; a reading variant "
            "needs the SRV-lowered flavor in either all members or none",
            grp->nameStr(), resolved[0].pass->nameStr(), grp->allowSplit ? "" : "not ", rm.pass->nameStr(),
            rm.pass->allowSplit ? "" : "not ");
          return nullptr;
        }
      }
    }
  }

  grp->members.resize(resolved.size());
  for (int i = 0; i < resolved.size(); ++i)
  {
    grp->members[i].passIdx = resolved[i].pass->tableIdx;
    grp->members[i].subpassIdx = resolved[i].subpassIdx;
  }

  return grp;
}
} // namespace

void ShaderParser::add_render_pass_group(render_pass_group_decl *rpg, Parser &parser, shc::TargetContext &ctx)
{
  if (!rpg)
    return;

  if (rpg->old_kw)
  {
    report_error(parser, rpg->old_kw,
      "a subpass group is declared with the 'subpass' keyword: `subpass %s = any_of(...);`. 'render_pass' and "
      "'multi_render_pass' declare pass shapes",
      rpg->name->text);
    return;
  }
  if (!render_pass_claim_namespace_name(RenderPassNamed::Kind::Group, rpg->name, parser, ctx))
    return;

  Tab<ResolvedMember> resolved;
  if (!resolve_group_members(*rpg->list, rpg->name->text, parser, ctx, resolved))
    return;

  eastl::unique_ptr<RenderPassGroup> grp = build_group(rpg->name->text, resolved, parser, ctx);
  if (!grp)
    return;

  ctx.renderPassGroups().registerGroup(grp.get());
  grp.release();
}

Terminal *ShaderParser::render_pass_opt_in_term(render_pass_use_stat &s) { return s.pass_name ? s.pass_name : s.list->kw; }

RenderPassOptIn ShaderParser::resolve_render_pass_opt_in(render_pass_use_stat &s, Parser &parser, shc::TargetContext &ctx,
  RenderPassOptInMode mode)
{
  const bool report = mode == RenderPassOptInMode::Declare;
  if (s.old_kw)
  {
    if (report)
      report_error(parser, s.old_kw, "a shader opts in with the 'pass' keyword: `pass %s%s%s;`", s.pass_name->text,
        s.subpass_name ? "." : "", s.subpass_name ? s.subpass_name->text : "");
    return {};
  }

  if (s.list)
  {
    if (const RenderPassGroup *known = ctx.renderPassGroups().byId(s.inlineGroupNid))
      return RenderPassOptIn::ofGroup(known);
    if (!report)
      return {};

    Tab<ResolvedMember> resolved;
    if (!resolve_group_members(*s.list, nullptr, parser, ctx, resolved))
      return {};
    eastl::string name("any_of(");
    for (int i = 0; i < resolved.size(); ++i)
      name.append_sprintf("%s%s.%s", i ? "," : "", resolved[i].pass->nameStr(), resolved[i].pass->subpassName(resolved[i].subpassIdx));
    name.append(")");

    if (const RenderPassGroup *existing = ctx.renderPassGroups().findByName(name.c_str()))
    {
      s.inlineGroupNid = existing->tableIdx;
      return RenderPassOptIn::ofGroup(existing);
    }

    eastl::unique_ptr<RenderPassGroup> grp = build_group(name.c_str(), resolved, parser, ctx);
    if (!grp)
      return {};
    ctx.renderPassGroups().registerGroup(grp.get());
    const RenderPassGroup *out = grp.release();
    s.inlineGroupNid = out->tableIdx;
    return RenderPassOptIn::ofGroup(out);
  }

  const RenderPassNamed named = render_pass_namespace_find(ctx, ctx.renderPassNameMap().addVarId(s.pass_name->text));
  if (!named)
  {
    if (report && ctx.framebufferLayouts().findByName(s.pass_name->text))
      report_error(parser, s.pass_name, "'%s' is a framebuffer layout; a shader opts into a render_pass subpass or a subpass group",
        s.pass_name->text);
    else if (report)
      report_error(parser, s.pass_name, "unknown render_pass or subpass group '%s'", s.pass_name->text);
    return {};
  }
  if (!s.subpass_name)
  {
    if (!named.pass)
      return RenderPassOptIn::ofGroup(named.group);
    if (named.pass->subpasses.size() != 1)
    {
      if (report)
        report_error(parser, s.pass_name, "render_pass '%s' has %d subpasses; name the one to opt into (pass %s.SUBPASS;)",
          s.pass_name->text, int(named.pass->subpasses.size()), s.pass_name->text);
      return {};
    }
    return RenderPassOptIn::ofPass(named.pass, 0);
  }
  if (!named.pass)
  {
    if (report)
      report_error(parser, s.pass_name, "'%s' is a subpass group; opt in without a subpass (pass %s;)", s.pass_name->text,
        s.pass_name->text);
    return {};
  }
  const int spIdx = named.pass->subpassIndex(ctx.renderPassNameMap().addVarId(s.subpass_name->text));
  if (spIdx < 0)
  {
    if (report)
      report_error(parser, s.subpass_name, "render_pass '%s' has no subpass '%s' (its subpasses: %s)", s.pass_name->text,
        s.subpass_name->text, named.pass->subpassList().c_str());
    return {};
  }
  return RenderPassOptIn::ofPass(named.pass, spIdx);
}

bool ShaderParser::blend_reads_dst_alpha(const SemanticShaderPass &pass, int slot)
{
  // Without independent_blending the first parameter set drives every slot.
  const int bi = pass.independent_blending ? slot : 0;
  if (bi >= int(shaders::RenderState::NumIndependentBlendParameters))
    return false;
  // sasat is min(src.a, 1 - dst.a), so it reads destination alpha like the two explicit factors.
  auto isDstAlpha = [](int f) {
    return f == DRV3DC::BLEND_DESTALPHA || f == DRV3DC::BLEND_INVDESTALPHA || f == DRV3DC::BLEND_SRCALPHASAT;
  };
  if (pass.blend_src[bi] < 0)
    return false;
  if (isDstAlpha(pass.blend_src[bi]) || isDstAlpha(pass.blend_dst[bi]))
    return true;
  return !pass.force_noablend && (isDstAlpha(pass.blend_asrc[bi]) || isDstAlpha(pass.blend_adst[bi]));
}

eastl::string ShaderParser::render_pass_scope_desc(const RenderPassOptIn &scope, bool *out_depth_present, bool *out_depth_read_only)
{
  eastl::string desc;
  if (scope.group)
  {
    desc.sprintf("subpass group '%s'", scope.group->nameStr());
    if (out_depth_present)
      *out_depth_present = scope.group->depthPresent;
    if (out_depth_read_only)
      *out_depth_read_only = scope.group->depthReadOnly;
    return desc;
  }
  const RenderPassObject::Subpass &sp = scope.pass->subpasses[scope.subpassIdx];
  desc.sprintf("subpass '%s.%s'", scope.pass->nameStr(), scope.pass->str(sp.name));
  if (out_depth_present)
    *out_depth_present = sp.depthTargetIdx >= 0;
  if (out_depth_read_only)
    *out_depth_read_only = sp.depthReadOnly();
  return desc;
}

void ShaderParser::render_pass_scope_layouts(const RenderPassOptIn &scope, shc::TargetContext &ctx, Tab<FramebufferLayoutRef> &out)
{
  out.clear();
  if (scope.group)
    framebuffer_layouts_of_group(*scope.group, ctx.renderPasses(), ctx.framebufferLayouts(), out);
  else if (scope.pass)
    framebuffer_layouts_of_pass(*scope.pass, ctx.framebufferLayouts(), out);
}

bool ShaderParser::render_pass_scope_target_reads_as_float(dag::ConstSpan<FramebufferLayoutRef> layouts, int target_name_id,
  eastl::string &out_offender)
{
  for (const FramebufferLayoutRef &fb : layouts)
  {
    const FramebufferFormatInfo *fi = fb.targetFormatInfo(target_name_id);
    if (fi && fi->cls != FramebufferFormatClass::Float)
    {
      out_offender.sprintf("target '%s' is %s (%s) in framebuffer layout '%s'", fb.pass->str(target_name_id), fi->token,
        framebuffer_format_class_name(fi->cls), fb.name());
      return false;
    }
  }
  return true;
}

// The depth target counts: a multisampled depth-only pass rasterizes multisampled too.
static bool scope_is_multisampled(const RenderPassOptIn &scope, shc::TargetContext &ctx)
{
  auto subpassIsMs = [](const RenderPassObject &p, const RenderPassObject::Subpass &sp) {
    for (int c : sp.colors)
      if (p.targetIsMs(c))
        return true;
    return sp.depthTargetIdx >= 0 && p.targetIsMs(sp.depthTargetIdx);
  };
  if (!scope.group)
    return subpassIsMs(*scope.pass, scope.pass->subpasses[scope.subpassIdx]);
  const RenderPassTable &passes = ctx.renderPasses();
  for (const RenderPassGroup::Member &m : scope.group->members)
  {
    const RenderPassGroupMemberRef mr = render_pass_group_member(m, passes);
    if (!mr)
      continue;
    if (subpassIsMs(*mr.pass, *mr.subpass))
      return true;
  }
  return false;
}

void ShaderParser::validate_render_pass_writes(SemanticShaderPass &pass, shc::TargetContext &ctx, Parser &parser)
{
  // The reflected signature arrives with the bytecode, so checking before the cache has settled would
  // silently skip every check.
  G_ASSERT(!ctx.bytecodeCache().hasPendingPasses());
  const RenderPassOptInRef &optIn = pass.renderPassOptIn;
  if (!optIn.hasOptIn() || !pass.cs.empty())
    return;
  const RenderPassGroup *grp = optIn.isGroup() ? ctx.renderPassGroups().byId(optIn.groupId) : nullptr;
  const RenderPassObject *rp = nullptr;
  const RenderPassObject::Subpass *spp = nullptr;
  RenderPassOptIn optInScope = RenderPassOptIn::ofGroup(grp);
  Terminal *at = pass.renderPassDeclTerm;
  const bool writesZState = pass.z_write != 0;
  if (!grp)
  {
    rp = ctx.renderPasses().byId(optIn.passId);
    G_ASSERT_RETURN(rp, );
    spp = &rp->subpasses[optIn.subpassIdx];
    optInScope = RenderPassOptIn::ofPass(rp, optIn.subpassIdx);
  }
  bool depthPresent = false;
  bool depthReadOnly = false;
  eastl::string scope = render_pass_scope_desc(optInScope, &depthPresent, &depthReadOnly);

#if _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
  const bool bytecodeCompiled = shc::config().dxcContext != nullptr;
#elif _CROSS_TARGET_EMPTY
  const bool bytecodeCompiled = false;
#else
  const bool bytecodeCompiled = true;
#endif
  if (bytecodeCompiled && !pass.fsh.empty() && !pass.psOutputInfo.valid)
    report_error(parser, at,
      "the pixel shader output signature of this variant was not reflected, so the render_pass write checks of %s cannot run",
      scope.c_str());

  if (pass.psOutputInfo.valid && !pass.psOutput.declared())
  {
    if (pass.psOutputInfo.colorTargetMask)
      report_error(parser, at,
        "the pixel shader writes a color output, but this variant declares no (ps) output struct; a variant opted into %s names "
        "the render_pass targets it writes (`NAME@ps_output { float4 field : target; }`)",
        scope.c_str());
  }
  else if (pass.psOutputInfo.valid)
  {
    if (pass.psOutputInfo.colorTargetMask != pass.psOutput.structMask)
    {
      eastl::string expected, compiled;
      for (uint32_t m = pass.psOutput.structMask, i = 0; m; m >>= 1, ++i)
        if (m & 1)
        {
          const char *field = i < pass.psOutput.fieldBySlot.size() ? pass.psOutput.fieldBySlot[i].name.c_str() : "";
          expected.append_sprintf("%sSV_Target%d%s%s%s", expected.empty() ? "" : ", ", i, *field ? " (field '" : "", field,
            *field ? "')" : "");
        }
      for (uint32_t m = pass.psOutputInfo.colorTargetMask, i = 0; m; m >>= 1, ++i)
        if (m & 1)
          compiled.append_sprintf("%sSV_Target%d", compiled.empty() ? "" : ", ", i);
      report_error(parser, pass.psOutput.structTerm ? pass.psOutput.structTerm : at,
        "pixel shader outputs do not match the declared (ps) output struct for %s; is the struct the entry's return "
        "type?\n  declared: %s\n  compiled: %s",
        scope.c_str(), expected.empty() ? "<none>" : expected.c_str(), compiled.empty() ? "<none>" : compiled.c_str());
    }
    if (pass.color_write != -1)
      for (int i = 0; i < int(pass.psOutput.fieldBySlot.size()); ++i)
        if (!pass.psOutput.fieldBySlot[i].name.empty() && ((uint32_t(pass.color_write) >> (i * 4)) & 0xF) == 0)
          report_warning(parser, *(pass.psOutput.structTerm ? pass.psOutput.structTerm : at),
            "(ps) output struct field '%s' writes SV_Target%d but color_write masks that slot entirely",
            pass.psOutput.fieldBySlot[i].name.c_str(), i);
  }

  const char *depthRole = !depthPresent ? "no depth attachment" : "read-only depth";
  if (pass.psOutputInfo.valid && pass.psOutputInfo.writesDepth && (!depthPresent || depthReadOnly))
    report_error(parser, at, "%s has %s but the pixel shader writes SV_Depth", scope.c_str(), depthRole);
  // The converse is not checked: a writable depth attachment permits writes, it does not require them.

  // z_write unset defaults to on, so with no attachment to write this is dead state like z_test, not an
  // authoring mistake. A read-only binding is the real conflict: the attachment is bound for reading.
  if (writesZState && depthReadOnly)
    report_error(parser, at, "%s has read-only depth but the shader has z_write=true; set z_write=false", scope.c_str());
  else if (writesZState && !depthPresent)
    report_warning(parser, *at, "%s has no depth attachment, but the shader has z_write on (set or defaulted); set z_write=false",
      scope.c_str());

  // Dead state only warns, except dual-source blending, which no API honors with more than one target.
  {
    // The slot mask is the union over a group's members; the dual-source limit counts the widest single member.
    uint32_t scopeSlotMask = 0;
    const int scopeColorCount = render_pass_scope_color_count(optInScope, ctx.renderPasses());
    if (grp)
    {
      for (const RenderPassGroup::ColorSlot &cs : grp->colorSlots)
        if (cs.slot >= 0 && cs.slot < 32)
          scopeSlotMask |= 1u << cs.slot;
    }
    else if (rp && spp)
    {
      for (int ci = 0; ci < spp->colors.size(); ++ci)
        scopeSlotMask |= 1u << render_pass_color_slot(*spp, ci);
    }

    if (pass.dual_source_blending && scopeColorCount > 1)
      report_error(parser, at,
        "%s binds up to %d color targets in one draw, but the shader uses dual source blending (sc1/isc1/sa1/isa1 factors), which "
        "supports exactly one render target on every API",
        scope.c_str(), scopeColorCount);
    if (pass.dual_source_blending && pass.psOutput.declared() && !pass.psOutput.hasSrc1)
      report_error(parser, at,
        "%s: the blend state reads source 1 (sc1/isc1/sa1/isa1), but the (ps) output struct declares no 'src1' field to write it",
        scope.c_str());
    if (pass.psOutput.hasSrc1 && !pass.dual_source_blending)
      report_warning(parser, *at,
        "%s: the (ps) output struct declares a 'src1' field, but no blend factor of this variant reads "
        "source 1",
        scope.c_str());

    const int blendSlots = pass.independent_blending ? int(shaders::RenderState::NumIndependentBlendParameters) : 1;
    for (int i = 0; i < blendSlots; ++i)
    {
      if (pass.blend_src[i] < 0 || pass.blend_dst[i] < 0)
        continue;
      if (pass.independent_blending)
      {
        if (!(scopeSlotMask & (1u << i)))
          report_warning(parser, *at, "%s does not fill color slot %d, but the shader sets blend state for it", scope.c_str(), i);
      }
      else if (!scopeSlotMask)
        report_warning(parser, *at, "%s has no color target, but the shader sets blend state", scope.c_str());
    }

    if (pass.alpha_to_coverage > 0 && !scope_is_multisampled(optInScope, ctx))
      report_warning(parser, *at,
        "%s has no multisampled attachment, but the shader sets alpha_to_coverage=true; it needs more than one sample per pixel",
        scope.c_str());

    if (pass.psOutput.hasCoverage && pass.alpha_to_coverage > 0)
      report_error(parser, at,
        "%s: the (ps) output struct writes SV_Coverage and the shader sets alpha_to_coverage=true; D3D then drops "
        "alpha_to_coverage while Vulkan and Metal combine both masks, so the pair has no portable meaning. Use one of them",
        scope.c_str());
    if (pass.psOutput.hasCoverage && !scope_is_multisampled(optInScope, ctx))
      report_warning(parser, *at,
        "%s has no multisampled attachment, but the (ps) output struct writes SV_Coverage; with one sample per pixel it is a "
        "one-bit mask, an unusual spelling of discard",
        scope.c_str());

    if (pass.z_test != 0 && !depthPresent)
      report_warning(parser, *at, "%s has no depth attachment, but the shader has z_test on (set or defaulted); set z_test=false",
        scope.c_str());
    if (pass.stencil != 0 && !depthPresent)
      report_warning(parser, *at, "%s has no depth attachment, but the shader enables stencil", scope.c_str());
    if (pass.stencil != 0 && depthPresent && depthReadOnly &&
        (pass.stencil_pass != DRV3DC::STNCLOP_KEEP || pass.stencil_fail != DRV3DC::STNCLOP_KEEP ||
          pass.stencil_zfail != DRV3DC::STNCLOP_KEEP))
      report_warning(parser, *at,
        "%s has read-only depth, but the shader has a stencil write op; keep stencil_pass, stencil_fail and "
        "stencil_zfail at 'keep'",
        scope.c_str());
  }

  Tab<FramebufferLayoutRef> fbs;
  render_pass_scope_layouts(optInScope, ctx, fbs);
  const VarNameMap &names = ctx.renderPassNameMap();
  Terminal *structAt = pass.psOutput.structTerm ? pass.psOutput.structTerm : at;
  for (int slot = 0; slot < int(pass.psOutput.fieldBySlot.size()); ++slot)
  {
    const SemanticShaderPass::PsOutputField &f = pass.psOutput.fieldBySlot[slot];
    if (f.name.empty())
      continue;
    const FramebufferFormatClass fieldCls = f.cls;
    const int fieldComponents = f.components;
    const bool readsDstAlpha = blend_reads_dst_alpha(pass, slot);
    bool classReported = false;
    eastl::string noAlphaLayouts;
    int widestChannels = 0;
    const char *widestLayout = nullptr, *widestToken = nullptr;
    for (const FramebufferLayoutRef &fb : fbs)
    {
      const FramebufferFormatInfo *fi = fb.targetFormatInfo(names.getVarId(f.target.c_str()));
      if (!fi)
        continue;
      if (fi->cls != fieldCls && !classReported)
      {
        classReported = true;
        report_error(parser, structAt,
          "output field '%s' of %s is %s (%s class) but its target '%s' is %s (%s class) in framebuffer layout '%s'; a class "
          "mismatch reinterprets the bits",
          f.name.c_str(), scope.c_str(), f.type.c_str(), framebuffer_format_class_name(fieldCls), f.target.c_str(), fi->token,
          framebuffer_format_class_name(fi->cls), fb.name());
      }
      if (fi->channels > widestChannels)
      {
        widestChannels = fi->channels;
        widestLayout = fb.name();
        widestToken = fi->token;
      }
      if (readsDstAlpha && !fi->hasAlpha)
        noAlphaLayouts.append_sprintf("%s'%s' (%s)", noAlphaLayouts.empty() ? "" : ", ", fb.name(), fi->token);
    }
    if (!noAlphaLayouts.empty())
      report_warning(parser, *structAt,
        "%s blends target '%s' with a destination-alpha factor, but the target has no alpha channel in framebuffer layout %s, so "
        "the factor reads a constant",
        scope.c_str(), f.target.c_str(), noAlphaLayouts.c_str());
    // More components than channels is legal - the extra ones are dropped, as everywhere in HLSL.
    if (widestChannels > fieldComponents)
      report_error(parser, structAt,
        "output field '%s' of %s is %s but its target '%s' is %s in framebuffer layout '%s', which has %d channels; the channels "
        "the field does not write would be undefined",
        f.name.c_str(), scope.c_str(), f.type.c_str(), f.target.c_str(), widestToken, widestLayout, widestChannels);
  }
}
