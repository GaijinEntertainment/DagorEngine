// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include "const3d.h"
#include <drv/shadersMetaData/renderPassLowering.h>
#include <drv/3d/dag_decl.h>
#include "framebufferLayout.h"
#include "subpassRead.h"
#include "shLog.h"
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <dag/dag_vectorSet.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_localConv.h>
#include <string.h>

namespace shc
{
class TargetContext;
}

struct Parser;

namespace BaseParNamespace
{
struct Terminal;
}

namespace ShaderTerminal
{
struct bool_expr;
struct render_pass_decl;
struct render_pass_group_decl;
struct render_pass_use_stat;
} // namespace ShaderTerminal

inline constexpr int render_pass_read_register(int read_index) { return subpass_read_register(MAX_T_REGISTERS, read_index); }
inline constexpr int render_pass_read_window_base(int read_count) { return subpass_read_window_base(MAX_T_REGISTERS, read_count); }

class RenderPassObject
{
public:
  struct Target
  {
    RenderPassNameId name;
    bool multisampled = false;
    // formatSamples == 0: the pass names framebuffer layouts instead of inline formats.
    uint32_t format = 0;
    int formatSamples = 0;
  };

  struct Resolve
  {
    int srcTargetIdx = -1;
    int dstTargetIdx = -1;
  };

  struct Subpass
  { //-V802
    RenderPassNameId name;
    SerializableTab<int> colors;
    SerializableTab<int> reads;
    int depthTargetIdx = -1;
    int vrsTargetIdx = -1;
    bool depthSampled = false;

    int readPos(int target_idx) const;
    int colorPos(int target_idx) const;
    bool depthReadOnly() const;
    bool writes(int target_idx) const;
    bool binds(int target_idx) const;
  };

  RenderPassNameId name;
  int tableIdx = -1;
  SerializableTab<Target> targets;
  SerializableTab<Subpass> subpasses;
  SerializableTab<Resolve> resolves;
  bool allowSplit = false;
  SerializableTab<RenderPassNameId> framebuffers;

  BINDUMP_NON_SERIALIZABLE(
    // Untaken hardware branches included, so `optional` can tell a typo from a platform-only target.
    dag::VectorSet<int> lexicalTargetNames; const VarNameMap *names = nullptr;);

  static constexpr int MAX_SUBPASS_READS = 8;
  static constexpr int MAX_COLOR_SLOTS = Driver3dRenderTarget::MAX_SIMRT;
  static_assert(subpass_read_window_base(MAX_T_REGISTERS, MAX_SUBPASS_READS) >= 0);
  static_assert(MAX_T_REGISTERS == RENDER_PASS_T_REGISTER_COUNT);

  const char *str(int name_id) const { return names->getName(name_id); }
  const char *nameStr() const { return str(name); }
  int targetIndex(int name_id) const;
  const char *targetName(int i) const { return str(targets[i].name); }
  bool targetIsMs(int i) const { return targets[i].multisampled; }
  int subpassIndex(int name_id) const;
  eastl::string subpassList() const;
  const char *subpassName(int i) const { return str(subpasses[i].name); }
  bool isResolveDst(int target_idx) const;
  bool isResolveSrc(int target_idx) const;
  bool isVrsTarget(int target_idx) const;
  bool hasLexicalTarget(int name_id) const { return lexicalTargetNames.count(name_id) != 0; }
  bool hasInlineFormats() const;
  eastl::string shapeDesc() const;
};

class RenderPassTable;

class RenderPassGroup
{
public:
  struct Member
  {
    int passIdx = -1;
    int subpassIdx = -1;
  };

  struct ReadSlot
  { //-V802
    RenderPassNameId name;
    int targetIdx = -1; // Metal fetches an input attachment by it, not by ordinal
    bool multisampled = false;
  };

  // slot -1: the members disagree on the slot, so no group variant may write it.
  struct ColorSlot
  { //-V802
    RenderPassNameId name;
    int slot = -1;
    bool inAllMembers = false;
  };

  RenderPassNameId name;
  SerializableTab<Member> members;
  SerializableTab<ColorSlot> colorSlots;
  SerializableTab<ReadSlot> reads;
  int tableIdx = -1;
  int readableCount = 0; // common read prefix: only these reads are usable
  bool depthPresent = false;
  bool depthReadOnly = false;
  bool depthSampled = false;
  bool allowSplit = false;

  BINDUMP_NON_SERIALIZABLE(const VarNameMap *names = nullptr;);

  const char *str(int name_id) const { return names->getName(name_id); }
  const char *nameStr() const { return str(name); }
  const ColorSlot *findColor(int name_id) const;
  int readPos(int name_id) const;
  eastl::string shapeDesc() const;
  eastl::string describe(const RenderPassTable *passes) const;
};

class RenderPassGroupTable;

struct RenderPassGroupMemberRef
{
  const RenderPassObject *pass = nullptr;
  const RenderPassObject::Subpass *subpass = nullptr;

  explicit operator bool() const { return pass != nullptr; }
};

class RenderPassTable : public RenderPassNamedTable<RenderPassObject>
{
public:
  using RenderPassNamedTable::RenderPassNamedTable;

  void registerPass(RenderPassObject *p) { adopt(p); }
  void link(Tab<RenderPassObject *> &loaded_passes, const RenderPassGroupTable &groups, Tab<int> &lnktbl);
};

class RenderPassGroupTable : public RenderPassNamedTable<RenderPassGroup>
{
public:
  using RenderPassNamedTable::RenderPassNamedTable;

  void registerGroup(RenderPassGroup *g) { adopt(g); }
  void link(Tab<RenderPassGroup *> &loaded_groups, const RenderPassTable &passes, dag::ConstSpan<int> pass_lnktbl, Tab<int> &lnktbl);
};

struct RenderPassNamed
{
  enum class Kind
  {
    None,
    Pass,
    Group,
    FramebufferLayout,
  };

  Kind kind = Kind::None;
  RenderPassObject *pass = nullptr;
  RenderPassGroup *group = nullptr;
  FramebufferLayoutObject *layout = nullptr;

  explicit operator bool() const { return kind != Kind::None; }
  static const char *kindName(Kind k);
  const char *kindName() const { return kindName(kind); }
};

RenderPassNamed render_pass_namespace_find(shc::TargetContext &ctx, int name_id);
RenderPassNamed render_pass_namespace_find(const RenderPassTable &passes, const RenderPassGroupTable &groups, int name_id);

template <class T, class Find, class ExistingOf, class OnExisting, class Adopt, class Describe>
void render_pass_link_named(Tab<T *> &loaded, RenderPassNamed::Kind kind, const char *mismatch_words, const char *plural_words,
  Find &&find_taken, ExistingOf &&existing_of, OnExisting &&on_existing, Adopt &&adopt, Describe &&describe)
{
  for (int i = 0; i < loaded.size(); ++i)
  {
    T *obj = loaded[i];
    const RenderPassNamed taken = find_taken(int(obj->name));
    if (taken && taken.kind != kind)
      sh_debug(SHLOG_FATAL,
        "%s '%s' collides with a %s of the same name declared in another compilation unit; render passes and subpass groups "
        "share one namespace",
        RenderPassNamed::kindName(kind), obj->nameStr(), taken.kindName());
    if (T *existing = existing_of(taken))
    {
      if (existing->shapeDesc() != obj->shapeDesc())
        sh_debug(SHLOG_FATAL,
          "%s '%s' %s across compilation units; declare all %s once in a shared include (this unit: %s; the "
          "other: %s)",
          RenderPassNamed::kindName(kind), obj->nameStr(), mismatch_words, plural_words, describe(obj).c_str(),
          describe(existing).c_str());
      on_existing(i, existing);
      delete obj;
    }
    else
      adopt(i, obj);
  }
  loaded.clear();
}

bool render_pass_claim_namespace_name(RenderPassNamed::Kind kind, BaseParNamespace::Terminal *name, Parser &parser,
  shc::TargetContext &ctx);

bool render_pass_eval_build_condition(ShaderTerminal::bool_expr &e, Parser &parser, shc::TargetContext &ctx, bool &out_value);

template <class IfNode, class EvalStat>
void render_pass_eval_if_chain(IfNode &s, Parser &parser, shc::TargetContext &ctx, bool &had_error, EvalStat &&eval_stat)
{
  bool value = false;
  if (!render_pass_eval_build_condition(*s.expr, parser, ctx, value))
  {
    had_error = true;
    return;
  }
  for (auto *st : value ? s.true_stat : s.false_stat)
    eval_stat(*st);
  if (!value && s.else_if)
    render_pass_eval_if_chain(*s.else_if, parser, ctx, had_error, eval_stat);
}

class SemanticShaderPass;

bool render_pass_split_mode_declared(shc::TargetContext &ctx);
int render_pass_split_mode_ordinal(const char *value_name, shc::TargetContext &ctx);

bool render_pass_target_has_native_subpasses();

bool render_pass_target_is_metal();

RenderPassGroupMemberRef render_pass_group_member(const RenderPassGroup::Member &m, const RenderPassTable &passes);

int render_pass_color_slot(const RenderPassObject::Subpass &sp, int color_pos);

int render_pass_scope_color_count(const RenderPassOptIn &scope, const RenderPassTable &passes);

bool render_pass_any_has_target(const RenderPassTable &passes, int target_name_id);

bool render_pass_group_target_is_resolve_dst(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes);

bool render_pass_group_target_is_vrs(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes);

void render_pass_group_slot_divergence(const RenderPassGroup &grp, int target_name_id, const RenderPassTable &passes,
  eastl::string &out_desc);

namespace ShaderParser
{
inline bool render_pass_name_is_system_value(const char *name) { return dd_strnicmp(name, "sv_", 3) == 0; }

void add_render_pass(ShaderTerminal::render_pass_decl *rp, Parser &parser, shc::TargetContext &ctx);
void add_render_pass_group(ShaderTerminal::render_pass_group_decl *rpg, Parser &parser, shc::TargetContext &ctx);

enum class RenderPassOptInMode
{
  Declare,
  Reuse
};

RenderPassOptIn resolve_render_pass_opt_in(ShaderTerminal::render_pass_use_stat &s, Parser &parser, shc::TargetContext &ctx,
  RenderPassOptInMode mode);
BaseParNamespace::Terminal *render_pass_opt_in_term(ShaderTerminal::render_pass_use_stat &s);

void validate_render_pass_writes(SemanticShaderPass &pass, shc::TargetContext &ctx, Parser &parser);

bool blend_reads_dst_alpha(const SemanticShaderPass &pass, int slot);

eastl::string render_pass_scope_desc(const RenderPassOptIn &scope, bool *out_depth_present = nullptr,
  bool *out_depth_read_only = nullptr);

void render_pass_scope_layouts(const RenderPassOptIn &scope, shc::TargetContext &ctx, Tab<FramebufferLayoutRef> &out);

bool render_pass_scope_target_reads_as_float(dag::ConstSpan<FramebufferLayoutRef> layouts, int target_name_id,
  eastl::string &out_offender);

} // namespace ShaderParser
