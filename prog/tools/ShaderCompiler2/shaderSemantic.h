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

inline BoolVarLookupRes get_bool_maybe(ShaderTerminal::SHTOK_ident &ident, const auto &locvar_ctx, const shc::TargetContext &tgt_ctx)
{
  bool isGlobal = false;
  for (const BoolVarTable *tbl : {&locvar_ctx.localBoolVars(), &tgt_ctx.globBoolVars()})
  {
    if (auto found = tbl->tryGetExpr(ident))
      return {found.expr, isGlobal, found.hasBeenDeclaredMultipleTimes};
    isGlobal = true;
  }
  return {};
}

inline BoolVarLookupRes get_bool_expr(ShaderTerminal::SHTOK_ident &ident, Parser &parser, const auto &locvar_ctx,
  const shc::TargetContext &tgt_ctx)
{
  auto found = get_bool_maybe(ident, locvar_ctx, tgt_ctx);
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

bool parse_hlsl_source_to_blocks(const PerHlslStage<CodeSourceBlocks *> &dst, ShaderParser::ShaderBoolEvalCB &boolCb,
  shc::ShaderContext &ctx);

} // namespace semantic
