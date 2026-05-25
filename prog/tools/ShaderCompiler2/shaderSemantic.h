// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shShaderContext.h"
#include "shVariantContext.h"
#include "shErrorReporting.h"
#include "commonUtils.h"
#include "codeBlocks.h"
#include "shsem.h"


namespace semantic
{

struct BoolVarLookupRes
{
  ShaderTerminal::bool_expr *expr = nullptr;
  bool isGlobal = false;
  bool hasMultipleDeclarations = false;

  operator bool() const { return expr != nullptr; }
};

inline BoolVarLookupRes get_bool_maybe_by_name_id(int name_id, const auto &locvar_ctx)
{
  if (name_id < 0)
    return {};
  bool isGlobal = false;
  for (const BoolVarTable *tbl : {&locvar_ctx.localBoolVars(), locvar_ctx.localBoolVars().getParent()})
  {
    if (auto found = tbl->tryGetExpr(name_id))
      return {found.expr, isGlobal, found.hasBeenDeclaredMultipleTimes};
    isGlobal = true;
  }
  return {};
}

inline BoolVarLookupRes get_bool_maybe(ShaderTerminal::SHTOK_ident &ident, const auto &locvar_ctx)
{
  return get_bool_maybe_by_name_id(locvar_ctx.localBoolVars().getNameMap()->getVarId(ident.text), locvar_ctx);
}

inline BoolVarLookupRes get_bool_expr(ShaderTerminal::SHTOK_ident &ident, Parser &parser, const auto &locvar_ctx)
{
  auto found = get_bool_maybe(ident, locvar_ctx);
  if (!found)
    report_error(parser, &ident, "Bool variable '%s' not found", ident.text);
  return found;
}

inline bool compare_shader(ShaderTerminal::bool_value &e, const shc::ShaderContext &ctx)
{
  G_ASSERT(e.cmpop);
  const char *shname = ctx.name();
  switch (e.cmpop->op->num)
  {
    case SHADER_TOKENS::SHTOK_eq:
    case SHADER_TOKENS::SHTOK_assign: return strcmp(shname, e.shader->text) == 0;
    case SHADER_TOKENS::SHTOK_noteq: return strcmp(shname, e.shader->text) != 0;
    default: G_ASSERT(0);
  }
  return false;
}

void initialize_debug_mode(shc::ShaderContext &ctx);
bool parse_hlsl_source_to_blocks(shc::ShaderContext &ctx, ShaderParser::ShaderBoolEvalCB &boolCb);

// Lookup and return (local and global intervals) or add a new interval (to the local or to the global list).
// Global interval list is aquired from the TargetContex, and localIntervals can be passed optionally.
// Interval is added to the local list if available, and to the the global otherwise.
eastl::pair<ShaderVariant::ExtType, int> get_or_add_resource_interval(const char *var_name, shc::TargetContext &tgtCtx,
  IntervalList *localIntervals = nullptr);

} // namespace semantic
