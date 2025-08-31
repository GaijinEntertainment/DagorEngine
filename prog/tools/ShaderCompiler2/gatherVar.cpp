// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gatherVar.h"
#include "shCompContext.h"
#include "hwSemantic.h"
#include "shaderSemantic.h"
#include "semUtils.h"
#include "shErrorReporting.h"
#include "globVarSem.h"
#include "globVar.h"
#include "globalConfig.h"
#include "boolVar.h"
#include "varMap.h"
#include <debug/dag_debug.h>
#include "assemblyShader.h"
#include "shLog.h"
#include "shExprParser.h"
#include "shCompiler.h"
#include "codeBlocks.h"
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include "hash.h"

#if _CROSS_TARGET_DX12
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#endif

using namespace ShaderParser;


static void validate_local_debug_mode_assume(const auto &stat, Parser &parser)
{
  if (strcmp(stat.interval->text, "debug_mode_enabled") == 0)
  {
    report_warning(parser, *stat.interval,
      "Assuming the debug_mode_enabled does not enable shader asserts and is not advised. To enable debug mode on a per-file basis, "
      "put the assume at global scope.");
  }
}

/*********************************
 *
 * class GatherVarShaderEvalCB
 *
 *********************************/
GatherVarShaderEvalCB::GatherVarShaderEvalCB(shc::ShaderContext &ctx) :
  ctx{ctx}, types{ctx.typeTables()}, parser{ctx.tgtCtx().sourceParseState().parser}, dynStack(tmpmem), dynCount(0), hasDynFlag(false)
{}

void GatherVarShaderEvalCB::eval_channel_decl(channel_decl &s, int str_id)
{
  // channels must be independed by dynamic variables
  if (dynCount > 0)
    report_error(parser, s.type->type, "cannot declare dynamic-depended channels (check 'if' condition(s) for dynamic variants)!");
}

// clang-format off
void GatherVarShaderEvalCB::eval_assume_stat(assume_stat &s)
{
  validate_local_debug_mode_assume(s, parser);
  add_shader_assume(s, parser, ctx);
}

void GatherVarShaderEvalCB::eval_assume_if_not_assumed_stat(assume_if_not_assumed_stat &s)
{
  validate_local_debug_mode_assume(s, parser);
  add_shader_assume_if_not_assumed(s, parser, ctx);
}
// clang-format on

void GatherVarShaderEvalCB::eval_bool_decl(bool_decl &decl)
{
  ctx.localBoolVars().add(decl, parser, true);

  String hlsl_bool_var(0, "##bool %s\n", decl.name->text);
  ctx.localHlslSrc().forEach([&](String &src) { src.append(hlsl_bool_var); });
}

void GatherVarShaderEvalCB::decl_bool_alias(const char *name, bool_expr &expr)
{
  bool_decl decl;
  SHTOK_ident ident;
  ident.file_start = 0;
  ident.line_start = 0;
  ident.col_start = 0;
  decl.name = &ident;
  decl.name->text = name;
  decl.expr = &expr;
  ctx.localBoolVars().add(decl, parser, true);
}

void GatherVarShaderEvalCB::eval_static(static_var_decl &s)
{
  // variables must be independed by dynamic variants
  if (dynCount > 0)
    report_error(parser, s.name, "cannot declare dynamic-depended variables (check 'if' condition(s) for dynamic variants)!");

  // register static variable name for future use
  if (s.mode && s.mode->mode->num == SHADER_TOKENS::SHTOK_dynamic)
    dynamicVars.addNameId(s.name->text);
  else
    staticVars.addNameId(s.name->text);
}


void GatherVarShaderEvalCB::eval_interval_decl(interval &interv)
{
  // intervals must be independed by dynamic variants
  if (dynCount > 0)
    report_error(parser, interv.name, "cannot declare dynamic-depended intervals (check 'if' condition(s) for dynamic variants)!");

  // check for global
  ShaderVariant::VarType type = ShaderVariant::VARTYPE_INTERVAL;

  if (ctx.tgtCtx().globVars().getVarInternalIndex(ctx.tgtCtx().varNameMap().getVarId(interv.name->text)) != -1)
  {
    type = ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL;
  }

  add_interval(ctx.intervals(), interv, type, parser, ctx.tgtCtx());
}

ShVarBool GatherVarShaderEvalCB::addInterval(const char *intervalName, Terminal *terminal, bool_value *e)
{
  // try to find registered interval
  int intervalName_id = ctx.tgtCtx().intervalNameMap().getNameId(intervalName);
  ShaderVariant::ExtType intervalIndex = ctx.intervals().getIntervalIndex(intervalName_id);
  const Interval *interv = ctx.intervals().getInterval(intervalIndex);

  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    intervalIndex = ctx.tgtCtx().globVars().getIntervalList().getIntervalIndex(intervalName_id);
    interv = ctx.tgtCtx().globVars().getIntervalList().getInterval(intervalIndex);

    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      report_error(parser, terminal, "interval %s not found!", intervalName);
      return ShVarBool(false, true);
    }
  }

  bool is_static = staticVars.getNameId(intervalName) != -1;
  bool is_dynamic = dynamicVars.getNameId(intervalName) != -1;

  bool is_global = !is_static && !is_dynamic;

  if (auto valueMaybe = ctx.assumes().getAssumedVal(intervalName, is_global))
  {
    ShaderVariant::ValueType valtype = interv->normalizeValue(*valueMaybe);

    if (!e)
      return ShVarBool(true, false);

    Interval::BooleanExpr expr = Interval::EXPR_NOTINIT;
    switch (e->cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: expr = Interval::EXPR_EQ; break;
      case SHADER_TOKENS::SHTOK_greater: expr = Interval::EXPR_GREATER; break;
      case SHADER_TOKENS::SHTOK_greatereq: expr = Interval::EXPR_GREATER_EQ; break;
      case SHADER_TOKENS::SHTOK_smaller: expr = Interval::EXPR_SMALLER; break;
      case SHADER_TOKENS::SHTOK_smallereq: expr = Interval::EXPR_SMALLER_EQ; break;
      case SHADER_TOKENS::SHTOK_noteq: expr = Interval::EXPR_NOT_EQ; break;
      default: G_ASSERT(0);
    }

    auto &dst = ctx.compiledShader().assumedIntervals.emplace_back();
    dst.name = bindump::string{intervalName};
    dst.value = uint8_t(*valueMaybe);

    String error_msg;
    bool bool_result = interv->checkExpression(valtype, expr, e->interval_value->text, error_msg, ctx.tgtCtx());
    if (!error_msg.empty())
      report_error(parser, terminal, error_msg);

    return ShVarBool(bool_result, true);
  }

  if (!is_static && !is_dynamic && ctx.tgtCtx().globVars().getVarInternalIndex(ctx.tgtCtx().varNameMap().getVarId(intervalName)) == -1)
    report_error(parser, terminal, "Non-assumed interval '%s' doesn't have a corresponding var", intervalName);

  // ok, found, adding variant flag
  ShaderVariant::TypeTable *lst = NULL;
  if (interv->getVarType() != ShaderVariant::VARTYPE_INTERVAL || !is_static)
  {
    lst = &types.allDynamicTypes;
    hasDynFlag = true;
  }
  else
  {
    lst = &types.allStaticTypes;
  }
  lst->addType(interv->getVarType(), intervalIndex, true);
  return ShVarBool(true, false);
}

ShVarBool GatherVarShaderEvalCB::addTextureInterval(const char *textureName, Terminal *terminal, bool_value &e)
{
  //  try to find registered interval
  bool isGlobal = ctx.tgtCtx().globVars().getVarInternalIndex(ctx.tgtCtx().varNameMap().getVarId(textureName)) >= 0;
  ShaderVariant::VarType type = isGlobal ? ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL : ShaderVariant::VARTYPE_INTERVAL;

  int textureName_id = ctx.tgtCtx().intervalNameMap().getNameId(textureName);
  int intervalIndex = ctx.intervals().getIntervalIndex(textureName_id);

  if (intervalIndex == INTERVAL_NOT_INIT)
    intervalIndex = ctx.tgtCtx().globVars().getIntervalList().getIntervalIndex(textureName_id);

  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    // interval not found - register new fake-interval
    int textureIntervalNameId = ctx.tgtCtx().intervalNameMap().addNameId(textureName);
    Interval newInterval(textureIntervalNameId, type);
    newInterval.addValue(ctx.tgtCtx().intervalNameMap().addNameId("NULL"), 1.0f);
    newInterval.addValue(ctx.tgtCtx().intervalNameMap().addNameId("EXISTS"), IntervalValue::VALUE_INFINITY);

    if (!ctx.intervals().addInterval(newInterval))
    {
      report_error(parser, terminal, "interval already exists and their type diffirent from new interval!");
      return ShVarBool(false, false);
    }
    intervalIndex = ctx.intervals().getIntervalIndex(newInterval.getNameId());
  }

  if (auto valueMaybe = ctx.assumes().getAssumedVal(textureName, isGlobal))
  {
    float value = *valueMaybe;
    switch (e.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: return ShVarBool(value < 1, true);
      case SHADER_TOKENS::SHTOK_noteq: return ShVarBool(value >= 1, true);
    }
    report_error(parser, terminal, "bad cmp op for texture interval: %d", e.cmpop->op->num);
  }

  bool is_static = staticVars.getNameId(textureName) != -1;

  if (is_static)
    types.allStaticTypes.addType(type, intervalIndex, true);
  else
  {
    hasDynFlag = true;
    types.allDynamicTypes.addType(type, intervalIndex, true);
  }
  return ShVarBool(true, false);
}

void GatherVarShaderEvalCB::eval_external_block(external_state_block &state_block)
{
  auto eval_if_stat = [this](state_block_if_stat &s, auto &&eval_if_stat) -> void {
    G_ASSERT(s.expr);

    auto eval_stats = [&](auto &stats) {
      for (auto &stat : stats)
        if (stat->stblock_if_stat)
          eval_if_stat(*stat->stblock_if_stat, eval_if_stat);
    };

    ShVarBool b = eval_expr(*s.expr);
    if (b.value || !b.isConst)
      eval_stats(s.true_stat);

    if (!b.value || !b.isConst)
    {
      eval_stats(s.false_stat);
      if (s.else_if)
        eval_if_stat(*s.else_if, eval_if_stat);
    }
  };

  for (auto stat : state_block.stblock_stat)
    if (stat->stblock_if_stat)
      eval_if_stat(*stat->stblock_if_stat, eval_if_stat);
}

ShVarBool GatherVarShaderEvalCB::eval_expr(bool_expr &e)
{
  int statStartIdx = types.allStaticTypes.getCount();
  int dynStartIdx = types.allDynamicTypes.getCount();
  ShVarBool v = ShaderParser::eval_shader_bool(e, *this);
  for (int i = statStartIdx; i < types.allStaticTypes.getCount(); i++)
    types.referencedTypes.addType(types.allStaticTypes.getType(i).type, types.allStaticTypes.getType(i).extType, true);
  if (v.isConst && !v.value && statStartIdx + dynStartIdx != types.allStaticTypes.getCount() + types.allDynamicTypes.getCount())
  {
    String cond;
    ShaderParser::build_bool_expr_string(e, cond);
    debug("reject gathered variant vars: %d;%d -> %d;%d due to const.false expr=\"%s\"", types.allStaticTypes.getCount(),
      types.allDynamicTypes.getCount(), statStartIdx, dynStartIdx, cond);
    types.allStaticTypes.shrinkTo(statStartIdx);
    types.allDynamicTypes.shrinkTo(dynStartIdx);
  }
  return v;
}
ShVarBool GatherVarShaderEvalCB::eval_bool_value(bool_value &e)
{
  if (e.two_sided)
  {
    types.allStaticTypes.addType(ShaderVariant::VARTYPE_MODE, ShaderVariant::TWO_SIDED, true);
    return ShVarBool(true, false);
  }
  else if (e.real_two_sided)
  {
    types.allStaticTypes.addType(ShaderVariant::VARTYPE_MODE, ShaderVariant::REAL_TWO_SIDED, true);
    return ShVarBool(true, false);
  }
  else if (e.shader)
  {
    return ShVarBool(semantic::compare_shader(e, ctx), true);
  }
  else if (e.hw)
  {
    return ShVarBool(semantic::compare_hw_token(e, ctx.tgtCtx().compCtx()), true);
  }
  else if (e.interval_ident)
  {
    return addInterval(e.interval_ident->text, e.interval_ident, &e);
  }
  else if (e.texture_name)
  {
    return addTextureInterval(e.texture_name->text, e.texture_name, e);
  }
  else if (e.bool_var)
  {
    auto [expr, isGlobal, hasMultipleDeclarations] = semantic::get_bool_expr(*e.bool_var, parser, ctx, ctx.tgtCtx());
    if (expr)
    {
      // Local bool var declarations at this point may depend on intervals, if there has been more than one
      ShVarBool evalRes = eval_expr(*expr);
      return ShVarBool(evalRes.value, evalRes.isConst && (isGlobal || !hasMultipleDeclarations));
    }
  }
  else if (e.maybe)
  {
    auto [expr, isGlobal, _] = semantic::get_bool_maybe(*e.maybe_bool_var, ctx, ctx.tgtCtx());
    if (expr)
    {
      // With maybe the branch eval result may depend on intervals even if there was only one decl up to this point -- cause it may not
      // be present in some variants.
      ShVarBool evalRes = eval_expr(*expr);
      return ShVarBool(evalRes.value, evalRes.isConst && isGlobal);
    }
    else
      return ShVarBool(false, true);
  }
  else if (e.true_value)
  {
    return ShVarBool(true, true);
  }
  else if (e.false_value)
  {
    return ShVarBool(false, true);
  }
  else
  {
    G_ASSERT(0);
  }
  return ShVarBool(false, false);
}
int GatherVarShaderEvalCB::eval_interval_value(const char *ival_name)
{
  addInterval(ival_name, NULL, NULL);
  return 0;
}

int GatherVarShaderEvalCB::eval_if(bool_expr &e)
{
  String cond("##if ");
  ShaderParser::build_bool_expr_string(e, cond, false);
  cond.append("\n");
  ctx.localHlslSrc().forEach([&](String &src) { src.append(cond); });

  // debug("expr-----------");
  ShVarBool b = eval_expr(e);

  // count dynamic flag
  dynStack.push_back(hasDynFlag);
  if (hasDynFlag)
    dynCount++;
  hasDynFlag = false;

  // debug("expr ok--------");
  return b.isConst ? (b.value ? IF_TRUE : IF_FALSE) : IF_BOTH;
}

void GatherVarShaderEvalCB::eval_else(bool_expr &e)
{
  ctx.localHlslSrc().forEach([](String &src) { src.append("##else\n"); });
}

void GatherVarShaderEvalCB::eval_supports(supports_stat &s)
{
#if _CROSS_TARGET_DX12
  eastl::string hlsl;
  for (auto name : s.name)
  {
    if (name->num == SHADER_TOKENS::SHTOK_none)
      continue;

    using namespace std::string_view_literals;
    if (name->text == "__draw_id"sv)
    {
      uint32_t slot = dxil::SPECIAL_CONSTANTS_REGISTER_INDEX;
      uint32_t space = dxil::DRAW_ID_REGISTER_SPACE;
      hlsl.append_sprintf("cbuffer draw_id_const_buffer:register(b%d, space%d){ uint __draw_id; };\n", slot, space);
      hlsl.append("uint get_draw_id() {return __draw_id;}\n");
    }
  }

  hlsl_mask_t hlslTypes = HLSL_FLAGS_ALL & ~HLSL_FLAGS_CS;
  ctx.localHlslSrc().forEach(hlslTypes, [&, this](String &src) {
    addSourceCode(src, s.name[0], hlsl.c_str(), parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled());
  });
#endif
}

void GatherVarShaderEvalCB::eval(immediate_const_block &s)
{
  int words = semutils::int_number(s.count->text);
  if (words <= 0)
    return;
  if (words > 4)
    report_error(parser, s.count, "maximum immediate words is 4");

  HlslCompilationStage stage = profile_to_hlsl_stage(s.scope->text);

  String hlsl;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2






#elif _CROSS_TARGET_DX12
  // On DX12 we use the space index to indicate how many dwords the buffer actually uses.
  // DXC rounds const buffer size to next vec4 size in reflection data, so we need to pass
  // along somehow how many dwords are actually used. With the space index this is a simple
  // way of letting DXC do the work for us.
  uint32_t slot = dxil::ROOT_CONSTANT_BUFFER_REGISTER_INDEX;
  uint32_t space = dxil::ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET + words;
  hlsl.aprintf(128, "cbuffer immediate_const_buffer:register(b%d, space%d){ uint", slot, space);
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "%s immediate_dword_%d", i != 0 ? "," : "", i);
  hlsl.aprintf(32, ";};\n");
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "uint get_immediate_dword_%d() {return immediate_dword_%d;}\n", i, i);
#else
  uint32_t slot = 8;
#if _CROSS_TARGET_SPIRV
  hlsl.aprintf(128, "#if SPIRV_DISALLOW_PUSH_CONSTANTS\n");
  slot = 7;
#endif
  hlsl.aprintf(128, "#define NUM_IMMEDIATE_DWORDS %u\n", words);
  hlsl.aprintf(128, "cbuffer immediate_const_buffer:register(b%d){ uint", slot);
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "%s immediate_dword_%d", i != 0 ? "," : "", i);
  hlsl.aprintf(32, ";};\n");
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "uint get_immediate_dword_%d() {return immediate_dword_%d;}\n", i, i);
#if _CROSS_TARGET_SPIRV
  hlsl.aprintf(128, "#else\n");
  const uint32_t MAX_IMMEDIATE_CONST_WORDS = 4;
  hlsl.aprintf(128,
    "struct ImmDwords { [[vk::offset(%u)]] uint data[%d]; };\n"
    "[[vk::push_constant]] ImmDwords imm_dwords;\n",
    (stage == HLSL_PS) ? MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t) : 0, words);
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "uint get_immediate_dword_%d() {return imm_dwords.data[%d];}\n", i, i);
  hlsl.aprintf(128, "#endif\n");
#endif
#endif

  if (!item_is_in(stage, {HLSL_VS, HLSL_PS, HLSL_CS, HLSL_MS, HLSL_AS}))
  {
    report_error(parser, s.scope, "unknown hlsl block");
    return;
  }

  auto &hlsls = ctx.localHlslSrc();

  addSourceCode(hlsls.all[stage], s.scope, hlsl.c_str(), parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled());
  if (stage == HLSL_VS)
  {
    // hs, ds and gs stage also see vs sources so they need this also
    addSourceCode(hlsls.fields.gs, s.scope, hlsl.c_str(), parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled());
    addSourceCode(hlsls.fields.hs, s.scope, hlsl.c_str(), parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled());
    addSourceCode(hlsls.fields.ds, s.scope, hlsl.c_str(), parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled());
  }
}


void GatherVarShaderEvalCB::eval_hlsl_decl(hlsl_local_decl_class &sh)
{
  if (!semantic::validate_hardcoded_regs_in_hlsl_block(sh.text))
    return;

  hlsl_mask_t hlsl_types = HLSL_FLAGS_ALL;
  if (sh.ident)
  {
    HlslCompilationStage stage = profile_to_hlsl_stage(sh.ident->text);

    if (stage == HLSL_INVALID)
      report_error(parser, sh.ident, "Unexpected scope %s", sh.ident->text);

    hlsl_types = HLSL_ALL_FLAGS_LIST[stage];
  }

  if (hlsl_types & HLSL_FLAGS_VS)
    hlsl_types |= HLSL_FLAGS_DS | HLSL_FLAGS_GS | HLSL_FLAGS_HS;

  if (hlsl_types & HLSL_FLAGS_AS)
    hlsl_types |= HLSL_FLAGS_MS;

  ctx.localHlslSrc().forEach(hlsl_types,
    [&, this](String &src) { addSourceCode(src, sh.text, parser, ctx.tgtCtx().globMessages(), ctx.isDebugModeEnabled()); });
}

void GatherVarShaderEvalCB::eval_endif(bool_expr &e)
{
  G_ASSERT(dynStack.size() > 0);
  bool v = dynStack.back();
  dynStack.pop_back();

  if (v)
  {
    G_ASSERT(dynCount > 0);
    dynCount--;
  }
  ctx.localHlslSrc().forEach([&](String &src) { src.append("##endif\n"); });
}
