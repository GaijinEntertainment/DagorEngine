// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "framebufferLayout.h"
#include "renderPassObject.h"
#include "shlexterm.h"
#include "shsem.h"
#include "shTargetContext.h"
#include "shErrorReporting.h"
#include "shLog.h"
#include "commonUtils.h"
#include <generic/dag_sort.h>
#include <EASTL/string_view.h>
#include <EASTL/unique_ptr.h>
#include <drv/3d/dag_texFlags.h>

using namespace ShaderTerminal;
using namespace ShaderParser;

namespace
{
using FC = FramebufferFormatClass;

const FramebufferFormatInfo FORMAT_TOKENS[] = {
  // token         texfmt                     class     ch alpha depth
  {"argb8", TEXFMT_A8R8G8B8, FC::Float, 4, true, false},
  {"rgba8", TEXFMT_R8G8B8A8, FC::Float, 4, true, false},
  {"abgr8", TEXFMT_A8B8G8R8, FC::Float, 4, true, false},
  {"argb8_srgb", TEXFMT_A8R8G8B8 | TEXCF_SRGBWRITE | TEXCF_SRGBREAD, FC::Float, 4, true, false},
  {"rgba8_srgb", TEXFMT_R8G8B8A8 | TEXCF_SRGBWRITE | TEXCF_SRGBREAD, FC::Float, 4, true, false},
  {"abgr8_srgb", TEXFMT_A8B8G8R8 | TEXCF_SRGBWRITE | TEXCF_SRGBREAD, FC::Float, 4, true, false},
  {"a2rgb10", TEXFMT_A2R10G10B10, FC::Float, 4, true, false},
  {"a2bgr10", TEXFMT_A2B10G10R10, FC::Float, 4, true, false},
  {"r11g11b10f", TEXFMT_R11G11B10F, FC::Float, 3, false, false},
  {"argb16", TEXFMT_A16B16G16R16, FC::Float, 4, true, false},
  {"argb16s", TEXFMT_A16B16G16R16S, FC::Float, 4, true, false},
  {"argb16f", TEXFMT_A16B16G16R16F, FC::Float, 4, true, false},
  {"argb16ui", TEXFMT_A16B16G16R16UI, FC::Uint, 4, true, false},
  {"argb32f", TEXFMT_A32B32G32R32F, FC::Float, 4, true, false},
  {"argb32ui", TEXFMT_A32B32G32R32UI, FC::Uint, 4, true, false},
  {"a1rgb5", TEXFMT_A1R5G5B5, FC::Float, 4, true, false},
  {"a4rgb4", TEXFMT_A4R4G4B4, FC::Float, 4, true, false},
  {"rgb565", TEXFMT_R5G6B5, FC::Float, 3, false, false},
  {"rg8", TEXFMT_R8G8, FC::Float, 2, false, false},
  {"rg8s", TEXFMT_R8G8S, FC::Float, 2, false, false},
  {"rg16", TEXFMT_G16R16, FC::Float, 2, false, false},
  {"rg16f", TEXFMT_G16R16F, FC::Float, 2, false, false},
  {"rg32f", TEXFMT_G32R32F, FC::Float, 2, false, false},
  {"rg32ui", TEXFMT_R32G32UI, FC::Uint, 2, false, false},
  // A8's one channel is the alpha channel, and the component count rule is a lower bound, so a scalar field fits.
  {"a8", TEXFMT_A8, FC::Float, 1, true, false},
  {"l16", TEXFMT_L16, FC::Float, 1, false, false},
  {"r8", TEXFMT_R8, FC::Float, 1, false, false},
  {"r8ui", TEXFMT_R8UI, FC::Uint, 1, false, false},
  {"r16f", TEXFMT_R16F, FC::Float, 1, false, false},
  {"r16ui", TEXFMT_R16UI, FC::Uint, 1, false, false},
  {"r32f", TEXFMT_R32F, FC::Float, 1, false, false},
  {"r32ui", TEXFMT_R32UI, FC::Uint, 1, false, false},
  {"r32si", TEXFMT_R32SI, FC::Sint, 1, false, false},
  {"d16", TEXFMT_DEPTH16, FC::Float, 1, false, true},
  {"d24", TEXFMT_DEPTH24, FC::Float, 1, false, true},
  {"d32", TEXFMT_DEPTH32, FC::Float, 1, false, true},
  {"d32s8", TEXFMT_DEPTH32_S8, FC::Float, 1, false, true},
};

} // namespace

bool framebuffer_format_by_token(const char *token, uint32_t &out_texfmt)
{
  for (const FramebufferFormatInfo &f : FORMAT_TOKENS)
    if (streq(f.token, token))
    {
      out_texfmt = f.texfmt;
      return true;
    }
  return false;
}

const FramebufferFormatInfo *framebuffer_format_info(uint32_t texfmt)
{
  for (const FramebufferFormatInfo &f : FORMAT_TOKENS)
    if (f.texfmt == texfmt)
      return &f;
  return nullptr;
}

const char *framebuffer_format_class_name(FramebufferFormatClass c)
{
  switch (c)
  {
    case FC::Uint: return "uint";
    case FC::Sint: return "sint";
    default: return "float";
  }
}

bool framebuffer_hlsl_type_class(const char *type, FramebufferFormatClass &out_cls, int &out_components)
{
  struct TypeBase
  {
    const char *base;
    FramebufferFormatClass cls;
  };
  // Precision is never part of the class: half and float are one class.
  static const TypeBase BASES[] = {{"min16float", FC::Float}, {"min16uint", FC::Uint}, {"min16int", FC::Sint}, {"float", FC::Float},
    {"half", FC::Float}, {"uint", FC::Uint}, {"int", FC::Sint}};
  for (const TypeBase &b : BASES)
  {
    const size_t l = strlen(b.base);
    if (strncmp(type, b.base, l) != 0)
      continue;
    if (type[l] == 0)
    {
      out_cls = b.cls;
      out_components = 1;
      return true;
    }
    if (type[l] >= '1' && type[l] <= '4' && type[l + 1] == 0)
    {
      out_cls = b.cls;
      out_components = type[l] - '0';
      return true;
    }
    return false;
  }
  return false;
}

const char *FramebufferLayoutRef::name() const { return layout ? layout->nameStr() : pass->nameStr(); }

bool FramebufferLayoutRef::targetFormat(int target_name_id, uint32_t &out_format, int &out_samples) const
{
  if (layout)
  {
    const FramebufferLayoutObject::Entry *e = layout->findEntry(target_name_id);
    if (!e)
      return false;
    out_format = e->format;
    out_samples = e->samples;
    return true;
  }
  const int ti = pass->targetIndex(target_name_id);
  if (ti < 0 || pass->targets[ti].formatSamples <= 0)
    return false;
  out_format = pass->targets[ti].format;
  out_samples = pass->targets[ti].formatSamples;
  return true;
}

const FramebufferFormatInfo *FramebufferLayoutRef::targetFormatInfo(int target_name_id) const
{
  uint32_t fmt = 0;
  int samples = 0;
  return targetFormat(target_name_id, fmt, samples) ? framebuffer_format_info(fmt) : nullptr;
}

static void append_layouts_of_pass(const RenderPassObject &rp, const FramebufferLayoutTable &layouts, Tab<FramebufferLayoutRef> &out)
{
  auto add = [&out](const FramebufferLayoutRef &r) {
    for (const FramebufferLayoutRef &prev : out)
      if (r.layout ? prev.layout == r.layout : (prev.pass == r.pass && !prev.layout))
        return;
    out.push_back(r);
  };
  if (rp.framebuffers.empty())
  {
    if (rp.hasInlineFormats())
      add({&rp, nullptr});
    return;
  }
  for (int fbName : rp.framebuffers)
    if (const FramebufferLayoutObject *l = layouts.findByName(fbName))
      add({&rp, l});
}

void framebuffer_layouts_of_pass(const RenderPassObject &rp, const FramebufferLayoutTable &layouts, Tab<FramebufferLayoutRef> &out)
{
  out.clear();
  append_layouts_of_pass(rp, layouts, out);
}

void framebuffer_layouts_of_group(const RenderPassGroup &grp, const RenderPassTable &passes, const FramebufferLayoutTable &layouts,
  Tab<FramebufferLayoutRef> &out)
{
  out.clear();
  for (const RenderPassGroup::Member &m : grp.members)
    if (const RenderPassGroupMemberRef mr = render_pass_group_member(m, passes))
      append_layouts_of_pass(*mr.pass, layouts, out);
}

bool framebuffer_samples_by_token(const char *token, int &out_samples)
{
  if (token[0] != 'x' || strlen(token) != 2)
    return false;
  const int n = token[1] - '0';
  if (n != 1 && n != 2 && n != 4 && n != 8)
    return false;
  out_samples = n;
  return true;
}

eastl::string framebuffer_format_token_list()
{
  eastl::string s;
  for (const FramebufferFormatInfo &f : FORMAT_TOKENS)
    s.append_sprintf("%s%s", s.empty() ? "" : ", ", f.token);
  return s;
}

const FramebufferLayoutObject::Entry *FramebufferLayoutObject::findEntry(int name_id) const
{
  for (const Entry &e : entries)
    if (e.name == name_id)
      return &e;
  return nullptr;
}

eastl::string FramebufferLayoutObject::shapeDesc() const
{
  eastl::string s(eastl::string::CtorSprintf{}, "framebuffer_layout %s group=%d", nameStr(), isGroup() ? 1 : 0);
  Tab<const Entry *> byName;
  for (const Entry &e : entries)
    byName.push_back(&e);
  fast_sort(byName, [this](const Entry *a, const Entry *b) { return strcmp(str(a->name), str(b->name)) < 0; });
  for (const Entry *e : byName)
    s.append_sprintf("|e:%s fmt=0x%08X x%d", str(e->name), e->format, e->samples);
  for (int m : members)
    s.append_sprintf("|m:%s", str(m));
  return s;
}

void framebuffer_layout_flatten(const FramebufferLayoutObject &l, Tab<int> &out)
{
  auto add = [&out](int n) {
    for (int prev : out)
      if (prev == n)
        return;
    out.push_back(n);
  };
  if (!l.isGroup())
    add(l.name);
  else
    for (int m : l.members)
      add(m);
}

void framebuffer_sort_names_by_text(Tab<int> &name_ids, const VarNameMap &names)
{
  fast_sort(name_ids, [&names](int a, int b) { return strcmp(names.getName(a), names.getName(b)) < 0; });
}

void FramebufferLayoutTable::link(Tab<FramebufferLayoutObject *> &loaded_layouts)
{
  bindNames(loaded_layouts);
  render_pass_link_named<FramebufferLayoutObject>(
    loaded_layouts, RenderPassNamed::Kind::FramebufferLayout, "is declared with different structures", "framebuffer layouts",
    [&](int name_id) {
      FramebufferLayoutObject *l = findByName(name_id);
      return l ? RenderPassNamed{RenderPassNamed::Kind::FramebufferLayout, nullptr, nullptr, l} : RenderPassNamed{};
    },
    [](const RenderPassNamed &taken) { return taken.layout; }, [](int, FramebufferLayoutObject *) {},
    [&](int, FramebufferLayoutObject *l) { adopt(l); }, [](const FramebufferLayoutObject *l) { return l->shapeDesc(); });
}

namespace
{
struct ExplicitLayoutBuilder
{
  FramebufferLayoutObject &obj;
  Parser &parser;
  shc::TargetContext &ctx;
  bool ok = true;

  void evalStat(framebuffer_layout_stat &s)
  {
    if (s.if_stat)
    {
      evalIf(*s.if_stat);
      return;
    }
    const int targetNameId = ctx.renderPassNameMap().addVarId(s.target_name->text);
    if (obj.findEntry(targetNameId))
    {
      report_error(parser, s.target_name, "framebuffer_layout '%s': duplicate entry for target '%s'", obj.nameStr(),
        s.target_name->text);
      ok = false;
      return;
    }
    FramebufferLayoutObject::Entry entry;
    entry.name = targetNameId;
    if (!framebuffer_resolve_format_tokens("framebuffer_layout", obj.nameStr(), s.target_name->text, s.format, s.samples, parser,
          entry.format, entry.samples))
    {
      ok = false;
      return;
    }
    obj.entries.push_back(eastl::move(entry));
  }

  void evalIf(framebuffer_layout_if &s)
  {
    bool failed = false;
    render_pass_eval_if_chain(s, parser, ctx, failed, [&](framebuffer_layout_stat &st) { evalStat(st); });
    if (failed)
      ok = false;
  }
};

bool build_explicit_layout(FramebufferLayoutObject &obj, framebuffer_layout_decl &fl, Parser &parser, shc::TargetContext &ctx)
{
  ExplicitLayoutBuilder builder{obj, parser, ctx};
  for (framebuffer_layout_stat *st : fl.stat)
    builder.evalStat(*st);
  if (obj.entries.empty() && builder.ok)
  {
    report_error(parser, fl.name, "framebuffer_layout '%s' has no entries; a layout assigns a format to at least one target",
      obj.nameStr());
    return false;
  }
  return builder.ok;
}

bool build_layout_group(FramebufferLayoutObject &obj, framebuffer_layout_decl &fl, Parser &parser, shc::TargetContext &ctx)
{
  const framebuffer_layout_any_of &list = *fl.list;
  if (!streq(list.kw->text, "any_of"))
  {
    report_error(parser, list.kw, "expected 'any_of' in framebuffer_layout '%s', got '%s'", obj.nameStr(), list.kw->text);
    return false;
  }
  Tab<int> flat;
  for (SHTOK_ident *m : list.member)
  {
    const char *name = m->text;
    if (streq(name, obj.nameStr()))
    {
      report_error(parser, m, "membership cycle %s -> %s: a framebuffer layout cannot be its own member", name, name);
      return false;
    }
    const FramebufferLayoutObject *member = ctx.framebufferLayouts().findByName(name);
    if (!member)
    {
      if (const RenderPassNamed other = render_pass_namespace_find(ctx, ctx.renderPassNameMap().getVarId(name)))
        report_error(parser, m, "'%s' is a %s; a framebuffer_layout group takes framebuffer layouts", name, other.kindName());
      else
        report_error(parser, m, "unknown framebuffer layout '%s' in framebuffer_layout '%s'", name, obj.nameStr());
      return false;
    }
    framebuffer_layout_flatten(*member, flat);
  }
  if (flat.empty())
  {
    report_error(parser, list.kw, "framebuffer_layout '%s' has no members", obj.nameStr());
    return false;
  }
  framebuffer_sort_names_by_text(flat, ctx.renderPassNameMap());
  obj.members.resize(flat.size());
  for (int i = 0; i < flat.size(); ++i)
    obj.members[i] = flat[i];
  return true;
}
} // namespace

bool framebuffer_resolve_format_tokens(const char *owner_kind, const char *owner_name, const char *target_name,
  const BaseParNamespace::Terminal *fmt_tok, const BaseParNamespace::Terminal *samples_tok, Parser &parser, uint32_t &out_format,
  int &out_samples)
{
  if (!framebuffer_format_by_token(fmt_tok->text, out_format))
  {
    report_error(parser, fmt_tok, "%s '%s': unknown format '%s' for target '%s'; known formats: %s", owner_kind, owner_name,
      fmt_tok->text, target_name, framebuffer_format_token_list().c_str());
    return false;
  }
  out_samples = 1;
  if (samples_tok && !framebuffer_samples_by_token(samples_tok->text, out_samples))
  {
    report_error(parser, samples_tok, "%s '%s': unknown sample count '%s' for target '%s'; write x1, x2, x4 or x8", owner_kind,
      owner_name, samples_tok->text, target_name);
    return false;
  }
  return true;
}

void ShaderParser::add_framebuffer_layout(framebuffer_layout_decl *fl, Parser &parser, shc::TargetContext &ctx)
{
  if (!fl)
    return;
  if (!render_pass_claim_namespace_name(RenderPassNamed::Kind::FramebufferLayout, fl->name, parser, ctx))
    return;

  eastl::unique_ptr<FramebufferLayoutObject> obj(new FramebufferLayoutObject);
  obj->name = ctx.renderPassNameMap().addVarId(fl->name->text);
  obj->names = &ctx.renderPassNameMap();

  if (!(fl->list ? build_layout_group(*obj, *fl, parser, ctx) : build_explicit_layout(*obj, *fl, parser, ctx)))
    return;

  ctx.framebufferLayouts().registerLayout(obj.get());
  obj.release();
}
