// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "variantSemantic.h"
#include "shaderSemantic.h"
#include "hwSemantic.h"
#include "shExprParser.h"
#include <shaders/shUtils.h>
#include "defer.h"

namespace semantic
{

VarLookupRes lookup_state_var(const char *name, shc::VariantContext &ctx, bool allow_not_found, const Terminal *t)
{
  shc::TargetContext &targetCtx = ctx.tgtCtx();
  ShaderSemCode &semcode = ctx.parsedSemCode();

  VarLookupRes res = {};
  int varNameId = targetCtx.varNameMap().getVarId(name);

  res.id = semcode.find_var(varNameId);
  if (res.id < 0)
  {
    // else try to find global variable
    res.id = targetCtx.globVars().getVarInternalIndex(varNameId);
    if (res.id <= -1)
    {
      if (!allow_not_found)
        report_error(ctx.tgtCtx().sourceParseState().parser, t, "unknown variable '%s'", name);
      return {-2};
    }

    res.valType = targetCtx.globVars().getVar(res.id).type;
    res.isGlobal = true;

    return res;
  }
  semcode.vars[res.id].used = true;
  res.valType = semcode.vars[res.id].type;
  return res;
}

ShVarBool VariantBoolExprEvalCB::eval_expr(bool_expr &e)
{
  String evalExprErrStr{};
  String *savedStr = eastl::exchange(curEvalExprErrStr, &evalExprErrStr);

  ShVarBool result = ShaderParser::eval_shader_bool<true>(e, *this);
  curEvalExprErrStr = savedStr;

  if (!evalExprErrStr.empty() && (!result.isConst || result.value))
  {
    if (curEvalExprErrStr)
      curEvalExprErrStr->append(evalExprErrStr.c_str());
    else
    {
      String cond;
      ShaderParser::build_bool_expr_string(e, cond);
      report_error(ctx.tgtCtx().sourceParseState().parser, &e, "expr \"%s\" is not const.false and gives errors:\n%s", cond,
        evalExprErrStr.c_str());
    }
  }

  return result;
}

ShVarBool VariantBoolExprEvalCB::eval_bool_value(bool_value &val)
{
  const auto &variant = ctx.variant();
  auto &parser = ctx.tgtCtx().sourceParseState().parser;

  if (val.two_sided)
  {
    return ShVarBool(variant.getValue(ShaderVariant::VARTYPE_MODE, ShaderVariant::TWO_SIDED), true);
  }
  else if (val.real_two_sided)
  {
    return ShVarBool(variant.getValue(ShaderVariant::VARTYPE_MODE, ShaderVariant::REAL_TWO_SIDED), true);
  }
  else if (val.shader)
  {
    return ShVarBool(semantic::compare_shader(val, ctx.shCtx()), true);
  }
  else if (val.hw)
  {
    return ShVarBool(semantic::compare_hw_token(val, ctx.tgtCtx().compCtx()), true);
  }
  else if (val.interval_ident)
  {
    auto [varIndex, varType, isGlobal] = lookup_state_var(*val.interval_ident, ctx, true);
    bool has_corresponded_var = varIndex >= 0;

    G_ASSERT(val.cmpop);
    G_ASSERT(val.interval_value);

    Interval::BooleanExpr expr = Interval::EXPR_NOTINIT;
    switch (val.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: expr = Interval::EXPR_EQ; break;
      case SHADER_TOKENS::SHTOK_assign: expr = Interval::EXPR_EQ; break;
      case SHADER_TOKENS::SHTOK_greater: expr = Interval::EXPR_GREATER; break;
      case SHADER_TOKENS::SHTOK_greatereq: expr = Interval::EXPR_GREATER_EQ; break;
      case SHADER_TOKENS::SHTOK_smaller: expr = Interval::EXPR_SMALLER; break;
      case SHADER_TOKENS::SHTOK_smallereq: expr = Interval::EXPR_SMALLER_EQ; break;
      case SHADER_TOKENS::SHTOK_noteq: expr = Interval::EXPR_NOT_EQ; break;
      default: G_ASSERT(0);
    }

    if (expr == Interval::EXPR_NOTINIT)
      return ShVarBool(false, false);

    const IntervalList *intervalList = &variant.intervals;

    // search interval by name
    int e_interval_ident_id = ctx.tgtCtx().intervalNameMap().getNameId(val.interval_ident->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_interval_ident_id);
    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      intervalIndex = ctx.tgtCtx().globVars().getIntervalList().getIntervalIndex(e_interval_ident_id);

      if (intervalIndex == INTERVAL_NOT_INIT)
      {
        report_error(parser, val.interval_ident, "[ERROR] interval %s not found!", val.interval_ident->text);
        return ShVarBool(false, false);
      }

      intervalList = &ctx.tgtCtx().globVars().getIntervalList();

      if (!has_corresponded_var)
        isGlobal = true;
    }

    const Interval *interv = intervalList->getInterval(intervalIndex);
    G_ASSERT(interv);

    ShaderVariant::ValueType varValue = variant.getValue(interv->getVarType(), intervalIndex);
    if (varValue == -1)
    {
      if (auto valueMaybe = ctx.shCtx().assumes().getAssumedVal(val.interval_ident->text, isGlobal))
        varValue = interv->normalizeValue(*valueMaybe);
    }
    const Interval *interval = intervalList->getInterval(intervalIndex);
    if (!ShaderVariant::ValueRange(0, interval->getValueCount() - 1).isInRange(varValue))
    {
      report_bool_eval_error(
        String{0, "  illegal normalized value (%d) for this interval (%s)\n", varValue, get_interval_name(*interval, ctx.tgtCtx())});
      return ShVarBool(false, false);
    }

    String error_msg;
    bool bool_result = interval->checkExpression(varValue, expr, val.interval_value->text, error_msg, ctx.tgtCtx());
    if (!error_msg.empty())
      report_error(parser, val.interval_ident, error_msg);

    return ShVarBool(bool_result, true);
  }
  else if (val.texture_name)
  {
    auto [varIndex, varType, isGlobal] = lookup_state_var(*val.texture_name, ctx);
    if (varIndex < 0)
    {
      report_bool_eval_error(String{0, "  variable <%s> not defined!\n", val.texture_name});
      return ShVarBool(false, false);
    }

    const IntervalList *intervalList = &variant.intervals;

    // search interval by name
    int e_texture_ident_id = ctx.tgtCtx().intervalNameMap().getNameId(val.texture_name->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_texture_ident_id);
    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      intervalIndex = ctx.tgtCtx().globVars().getIntervalList().getIntervalIndex(e_texture_ident_id);

      if (intervalIndex == INTERVAL_NOT_INIT)
      {
        report_bool_eval_error(String{0, "  interval %s not found!\n", val.texture_name->text});
        return ShVarBool(false, false);
      }

      intervalList = &ctx.tgtCtx().globVars().getIntervalList();
    }

    const Interval *interv = intervalList->getInterval(intervalIndex);
    G_ASSERT(interv);

    ShaderVariant::ValueType varValue = variant.getValue(interv->getVarType(), intervalIndex);
    if (varValue == -1)
    {
      if (auto valueMaybe = ctx.shCtx().assumes().getAssumedVal(val.texture_name->text, isGlobal))
        varValue = interv->normalizeValue(*valueMaybe);
    }

    bool result = false;
    G_ASSERT(val.cmpop);
    switch (val.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq:
      case SHADER_TOKENS::SHTOK_assign:
      {
        result = varValue != 1;
        break;
      }
      case SHADER_TOKENS::SHTOK_noteq:
      {
        result = varValue == 1;
        break;
      }
      case SHADER_TOKENS::SHTOK_greater:
      case SHADER_TOKENS::SHTOK_greatereq:
      case SHADER_TOKENS::SHTOK_smaller:
      case SHADER_TOKENS::SHTOK_smallereq:
        report_error(parser, val.cmpop->op, "[ERROR] operators '>', '>=', '<' and '<=' are not supported here!");
        break;
      default: G_ASSERT(0);
    }

    return ShVarBool(result, true);
  }
  else if (val.bool_var)
  {
    auto res = semantic::get_bool_expr(*val.bool_var, parser, ctx, ctx.tgtCtx());
    if (res)
      return eval_expr(*res.expr);
  }
  else if (val.maybe)
  {
    auto res = semantic::get_bool_maybe(*val.maybe_bool_var, ctx, ctx.tgtCtx());
    if (res)
      return eval_expr(*res.expr);
    else
      return ShVarBool(false, true);
  }
  else if (val.true_value)
  {
    return ShVarBool(true, true);
  }
  else if (val.false_value)
  {
    return ShVarBool(false, true);
  }
  else
  {
    G_ASSERT(0);
  }
  return ShVarBool(false, false);
}

int VariantBoolExprEvalCB::eval_interval_value(const char *ival_name)
{
  int varNameId = ctx.tgtCtx().varNameMap().getVarId(ival_name);
  int vi = ctx.parsedSemCode().find_var(varNameId);
  if (vi >= 0)
    ctx.parsedSemCode().vars[vi].used = true;

  bool isGlobal = false;
  const IntervalList *intervalList = &ctx.variant().intervals;

  // search interval by name
  int e_interval_ident_id = ctx.tgtCtx().intervalNameMap().getNameId(ival_name);
  int intervalIndex = ctx.variant().intervals.getIntervalIndex(e_interval_ident_id);
  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    intervalIndex = ctx.tgtCtx().globVars().getIntervalList().getIntervalIndex(e_interval_ident_id);

    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      sh_debug(SHLOG_ERROR, "interval %s not found!", ival_name);
      return 0;
    }

    intervalList = &ctx.tgtCtx().globVars().getIntervalList();
    isGlobal = true;
  }

  const Interval *interv = intervalList->getInterval(intervalIndex);
  G_ASSERT(interv);

  ShaderVariant::ValueType varValue = ctx.variant().getValue(interv->getVarType(), intervalIndex);
  if (varValue == -1)
  {
    if (auto valueMaybe = ctx.shCtx().assumes().getAssumedVal(ival_name, isGlobal))
      varValue = interv->normalizeValue(*valueMaybe);
  }
  return varValue;
}

eastl::optional<ShaderStage> parse_state_block_stage(const char *stage_str)
{
  static const char *profiles[STAGE_MAX] = {"cs", "ps", "vs"};

  if (streq(stage_str, "ms"))
    stage_str = "vs";

  for (int i = 0; i < STAGE_MAX; ++i)
  {
    if (streq(profiles[i], stage_str))
      return ShaderStage(i);
  }

  return eastl::nullopt;
}

eastl::optional<NamedConstDefInfo> parse_named_const_definition(const state_block_stat &state_block, ShaderStage stage,
  VariableType vt, shc::VariantContext &ctx, IMemAlloc *tmp_memory)
{
#define TMPMEM_ALLOC(t_)           (tmp_memory ? new (tmp_memory) t_{} : new t_{})
#define TMPMEM_ALLOC_N(t_, count_) (tmp_memory ? new (tmp_memory) t_[count_]{} : new t_[count_]{})

  Parser &parser = ctx.tgtCtx().sourceParseState().parser;

  NamedConstDefInfo def{.type = vt, .stage = stage};

  // Parse declaration nodes into fields of def
  def.isArray = state_block.arr || state_block.reg_arr || (state_block.var && state_block.var->par);

  G_ASSERT(state_block.var || state_block.arr || state_block.reg || state_block.reg_arr);
  if (state_block.var)
  {
    def.varTerm = state_block.var->var->name;
    def.nameSpaceTerm = state_block.var->var->nameSpace;
    def.hlsl = state_block.var->hlsl_var_text;
    if (state_block.var->val->expr)
      def.shaderVarTerm = state_block.var->val->expr->lhs->lhs->var_name;
    def.builtinVarTerm = state_block.var->val->builtin_var;
  }
  else if (state_block.arr)
  {
    def.varTerm = state_block.arr->var->name;
    def.nameSpaceTerm = state_block.arr->var->nameSpace;
    def.hlsl = state_block.arr->hlsl_var_text;
  }
  else if (state_block.reg)
  {
    def.varTerm = state_block.reg->var->name;
    def.nameSpaceTerm = state_block.reg->var->nameSpace;
    def.hlsl = state_block.reg->hlsl_var_text;
    def.shaderVarTerm = state_block.reg->shader_var;
  }
  else if (state_block.reg_arr)
  {
    def.varTerm = state_block.reg_arr->var->name;
    def.nameSpaceTerm = state_block.reg_arr->var->nameSpace;
    def.hlsl = state_block.reg_arr->hlsl_var_text;
    def.shaderVarTerm = state_block.reg_arr->shader_var;
    def.sizeVarTerm = state_block.reg_arr->size_shader_var;
  }
  else
    G_ASSERTF(0, "Invalid/unsupported state_block_stat type");

  def.baseName = def.varTerm->text;
  def.mangledName = String{def.baseName};

  if (!validate_hardcoded_regs_in_hlsl_block(def.hlsl))
    return eastl::nullopt;

  // Check for same-name redeclaration @TODO: this does not validate same expressions for static cbuf stage overlap names. Should be
  // validated somewhere
  G_ASSERT(def.varTerm);
  int conflict_blk_num = 0;
  for (int i = 0; i < ctx.namedConstTable().suppBlk.size(); i++)
  {
    if (def.stage == STAGE_VS)
    {
      if (ctx.namedConstTable().suppBlk[i]->getVsNameId(def.baseName) != -1)
        conflict_blk_num++;
    }
    else
    {
      if (ctx.namedConstTable().suppBlk[i]->getPsNameId(def.baseName) != -1)
        conflict_blk_num++;
    }
  }
  if (conflict_blk_num && conflict_blk_num == ctx.namedConstTable().suppBlk.size())
  {
    report_error(parser, def.varTerm, "named const <%s%s> conflicts with one in <%s> block", def.baseName, def.nameSpaceTerm->text,
      ctx.namedConstTable().suppBlk[0]->name.c_str());
    return eastl::nullopt;
  }
  else if (conflict_blk_num)
    report_warning(parser, *def.varTerm, "named const <%s%s> hides one in supported non-compatible blocks", def.baseName,
      def.nameSpaceTerm->text);

  if (ctx.namedConstTable().globConstBlk)
  {
    bool conflict_blk = false;
    if (def.stage == STAGE_VS)
    {
      conflict_blk = ctx.namedConstTable().globConstBlk->getVsNameId(def.baseName) != -1;
    }
    else
    {
      conflict_blk = ctx.namedConstTable().globConstBlk->getPsNameId(def.baseName) != -1;
    }
    if (conflict_blk)
      report_error(parser, def.varTerm, "named const <%s%s> conflicts with global const block <%s>", def.baseName,
        def.nameSpaceTerm->text, ctx.namedConstTable().globConstBlk->name.c_str());
  }

  if (def.type == VariableType::Unknown)
  {
    report_error(parser, def.nameSpaceTerm, "named const <%s> has unknown type <%s> block", def.baseName, def.nameSpaceTerm->text);
    return eastl::nullopt;
  }

  // validate hlsl existence
  if ((def.type == VariableType::buf || def.type == VariableType::cbuf || def.type == VariableType::tex ||
        def.type == VariableType::smp) &&
      !def.hlsl)
  {
    report_error(parser, def.nameSpaceTerm, "named const <%s> has type of <%s> and so requires hlsl", def.baseName,
      def.nameSpaceTerm->text);
  }

  if (def.type != VariableType::f44 && state_block.arr && !state_block.arr->par)
    report_error(parser, def.varTerm, "Array initializer for single named constants is only allowed for @f44 type");

  // @TODO: enable and test for other types if need be
  if (def.type != VariableType::f4 && def.type != VariableType::i4 && def.type != VariableType::u4 && def.type != VariableType::f44 &&
      def.type != VariableType::uav && ((state_block.arr && state_block.arr->par) || state_block.reg_arr))
  {
    report_error(parser, def.varTerm, "variable <%s> is of type <%s> and not @f4, @f44, @i4, @u4, @uav and so can't be array",
      def.baseName, def.nameSpaceTerm->text);
  }

  def.isBindless = vt_is_static_texture(def.type) && shc::config().enableBindless;
  if (def.isBindless)
  {
    if (stage == STAGE_CS)
    {
      report_error(parser, def.varTerm, "variable <%s> of type <%s> is not permitted in a compute shader", def.baseName,
        def.nameSpaceTerm->text);
    }
    def.mangledName = String{0, "%s_bindless_id", def.baseName};
  }

  bool needPairSampler = false;

  switch (def.type)
  {
    case VariableType::f1:
    case VariableType::f2:
    case VariableType::f3:
    case VariableType::f4:
    case VariableType::f44:
      def.shvarType = SHVT_COLOR4;
      def.regSpace = HLSL_RSPACE_C;
      break;
    case VariableType::i1:
    case VariableType::i2:
    case VariableType::i3:
    case VariableType::i4:
    case VariableType::u1:
    case VariableType::u2:
    case VariableType::u3:
    case VariableType::u4:
      def.shvarType = SHVT_INT4;
      def.regSpace = HLSL_RSPACE_C;
      break;
    case VariableType::tex:
    case VariableType::tex2d:
    case VariableType::tex3d:
    case VariableType::texArray:
    case VariableType::texCube:
    case VariableType::texCubeArray:
      def.shvarType = SHVT_TEXTURE;
      def.regSpace = HLSL_RSPACE_T;
      break;

    case VariableType::smp:
    case VariableType::smp2d:
    case VariableType::smp3d:
    case VariableType::smpArray:
    case VariableType::smpCube:
    case VariableType::smpCubeArray:
    case VariableType::shd:
    case VariableType::shdArray:
      def.shvarType = SHVT_TEXTURE;
      def.regSpace = HLSL_RSPACE_T;
      needPairSampler = true;
      def.pairSamplerIsShadow = def.type == VariableType::shd || def.type == VariableType::shdArray;
      break;
    case VariableType::staticSampler:
    case VariableType::staticCube:
    case VariableType::staticCubeArray:
    case VariableType::staticTexArray:
    case VariableType::staticTex3D:
      def.shvarType = SHVT_TEXTURE;
      def.regSpace = def.isBindless ? HLSL_RSPACE_C : HLSL_RSPACE_T;
      needPairSampler = !def.isBindless;
      switch (def.type)
      {
        case VariableType::staticSampler: def.shvarTexType = ShaderVarTextureType::SHVT_TEX_2D; break;
        case VariableType::staticTex3D: def.shvarTexType = ShaderVarTextureType::SHVT_TEX_3D; break;
        case VariableType::staticCube: def.shvarTexType = ShaderVarTextureType::SHVT_TEX_CUBE; break;
        case VariableType::staticTexArray: def.shvarTexType = ShaderVarTextureType::SHVT_TEX_2D_ARRAY; break;
        case VariableType::staticCubeArray: def.shvarTexType = ShaderVarTextureType::SHVT_TEX_CUBE_ARRAY; break;
        default: break;
      }
      break;
    case VariableType::sampler:
      if (def.stage != STAGE_PS && def.stage != STAGE_CS)
        report_error(parser, def.varTerm, "@sampler is supported only in ps/cs stage");
      def.shvarType = SHVT_SAMPLER;
      def.regSpace = HLSL_RSPACE_S;
      break;
    case VariableType::cbuf:
      def.shvarType = SHVT_BUFFER;
      def.regSpace = HLSL_RSPACE_B;
      break;
    case VariableType::buf:
      def.shvarType = SHVT_BUFFER;
      def.regSpace = HLSL_RSPACE_T;
      break;
    case VariableType::tlas:
      def.shvarType = SHVT_TLAS;
      def.regSpace = HLSL_RSPACE_T;
      break;
    case VariableType::uav:
    {
      // shvarType will be inferred from initializer later, as it can be either buffer, or texture
      def.regSpace = HLSL_RSPACE_U;
      break;
    }
    default: G_ASSERTF(0, "unhandled type <%s>", def.nameSpaceTerm->text);
  }

  // @HACK produces a fake sampler declaration if a pair sampler var needs to be implicitly emitted
  // (this takes place for @smp... vars). This feature should be inevitably removed in favor of explicit
  // @tex... + @sampler combinations
  auto addPairSampler = [&] {
    def.pairSamplerBindSuffix = def.pairSamplerIsShadow ? "_cmpSampler" : "_samplerstate";
    def.pairSamplerName = String{0, "%s_samplerstate", def.shaderVarTerm->text};
    def.pairSamplerTmpDecl = TMPMEM_ALLOC(sampler_decl);
    def.pairSamplerTmpDecl->name = TMPMEM_ALLOC(SHTOK_ident);
    def.pairSamplerTmpDecl->name->text = def.pairSamplerName.c_str();
    def.pairSamplerTmpDecl->is_always_referenced = nullptr;
  };

  // For hardcoded regs, collect register values
  if (state_block.reg || state_block.reg_arr)
  {
    def.isDynamic = true;

    auto getConstIntFromShadervar = [&](Terminal *t) -> int {
      auto [vi, reg_type, is_global] = lookup_state_var(*t, ctx);
      G_ASSERT(vi >= 0);
      if (reg_type != SHVT_INT)
      {
        report_error(parser, t, "Hardcoded register %s must be of type `int`, but found %s", t->text,
          ShUtils::shader_var_type_name(reg_type));
      }
      if (is_global)
      {
        ctx.tgtCtx().globVars().getVar(vi).isImplicitlyReferenced = true;
        return ctx.tgtCtx().globVars().getVar(vi).value.i;
      }
      else
        report_error(parser, t, "Only global variables for hardcoded registers are supported");
      return -1;
    };

    def.hardcodedRegister = getConstIntFromShadervar(def.shaderVarTerm);
    def.registerSize = state_block.reg ? 1 : getConstIntFromShadervar(def.sizeVarTerm);
    def.arrayElemCount = def.registerSize;
    if (state_block.reg_arr && state_block.reg_arr->hlsl_var_text)
    {
      if (def.type == VariableType::f4)
      {
        // @TODO: enable once implemented
        report_error(parser, state_block.reg_arr->var,
          "Hardcoded register arrays with custom hlsl for type @f4 (arrays of structs) are not implemented yet");
        return eastl::nullopt;
      }
      else if (def.type != VariableType::uav)
      {
        // @TODO: enable and test for other types if need be
        report_error(parser, state_block.reg_arr->var,
          "Hardcoded register arrays with custom hlsl not supported for type %s. For arrays of structs, use @f4 (not implemented "
          "yet)",
          state_block.reg_arr->var->nameSpace->text);
        return eastl::nullopt;
      }
    }

    if (needPairSampler)
      addPairSampler();

    return def;
  }

  // For non-hardcoded consts parse out initializer
  G_ASSERT(state_block.arr || state_block.var);

  int synInitializerElemCount;
  if (state_block.arr)
    synInitializerElemCount = 1 + state_block.arr->arrN.size();
  else
    synInitializerElemCount = 1;

  def.initializer.reserve(synInitializerElemCount);

  const bool isMatrixFromVectors = def.type == VariableType::f44 && state_block.arr && !state_block.arr->par;
  if (isMatrixFromVectors && synInitializerElemCount != 4)
  {
    report_error(parser, def.varTerm, "@f44 array initializer must consist of 4 vector expressions");
    return eastl::nullopt;
  }

  // @NOTE: this is the only current syntax where array initializes a singular object
  if (isMatrixFromVectors)
    def.isArray = false;

  if (def.type == VariableType::f44 && !isMatrixFromVectors)
    def.shvarType = SHVT_FLOAT4X4;

  if (state_block.arr && !state_block.arr->par && !isMatrixFromVectors && synInitializerElemCount > 1)
  {
    report_error(parser, def.varTerm, "variable <%s> is of type <%s> requires [] for initialization with %d vectors", def.baseName,
      def.nameSpaceTerm->text, def.initializer.size());
  }

  auto registerInitShvarType = [&](ShaderVarType vt) {
    if (isMatrixFromVectors)
    {
      if (vt != SHVT_COLOR4)
      {
        report_error(parser, def.varTerm, "@f44 array initializer must contain only float4 expressions");
        return false;
      }
    }
    else if (def.type == VariableType::uav) // For uavs, type is inferred from initializer vars
    {
      if (vt != SHVT_TEXTURE && vt != SHVT_BUFFER)
      {
        report_error(parser, def.varTerm, "@uav should be declared with texture or buffer only");
        return false;
      }

      if (def.shvarType == SHVT_UNKNOWN)
        def.shvarType = vt;
      else if (def.shvarType != vt)
      {
        report_error(parser, def.varTerm, "@uav array can not be declared with different element types");
        return false;
      }
    }

    if (def.shvarType != vt)
    {
      report_error(parser, def.varTerm, "%s shadervar can't be used for initialization of a %s named const",
        ShUtils::shader_var_type_name(vt), def.nameSpaceTerm->text);
      return false;
    }

    return true;
  };

  const int elementRegisterSize = (def.type == VariableType::f44 && !isMatrixFromVectors) ? 4 : 1;
  const bool allowsArithmeticExpressions = (vt_is_numeric(def.type) && def.type != VariableType::f44) || isMatrixFromVectors;

  // All arithmetic consts are forced to be dynamic for blocks -- all stcode is run as if it were dynamic anyways, and until we can
  // bake pure constexpr expressions directly into hlsl (@TODO), we need to force them to be considered dynamic
  if (allowsArithmeticExpressions && ctx.shCtx().blockLevel() != ShaderBlockLevel::SHADER)
    def.isDynamic = true;

  bool hadStaticElements = false;

  auto parseInitializerElem = [&](external_var_value_single &val,
                                bool is_numeric_expr) -> eastl::optional<NamedConstInitializerElement> {
    using namespace ShaderParser;
    using Elem = NamedConstInitializerElement;

    // This updates status of static-dynamic mixing on per-initializer element level
    DEFER([&] {
      if (def.isDynamic && hadStaticElements && !def.hasDynStcodeRelyingOnMaterialParams)
      {
        def.hasDynStcodeRelyingOnMaterialParams = true;
        def.exprWithDynamicAndMaterialTerms = &val;
      }
    });

    if (val.builtin_var)
    {
      def.isDynamic = true;
      return Elem{Elem::BuiltinVar{val.builtin_var->num}, val.builtin_var};
    }

    auto *expr = val.expr;

    if (is_numeric_expr)
    {
      G_ASSERT(vt_is_numeric(def.type));
      G_ASSERT(def.shvarType == SHVT_COLOR4 || def.shvarType == SHVT_INT4);
      const ExpressionParser exprParser{ctx};

      if (!registerInitShvarType(def.shvarType))
        return eastl::nullopt;

      Elem::ArithmeticExpr colorExpr{expr, shexpr::VT_COLOR4};

      if (!exprParser.parseExpression(*expr, &colorExpr,
            ExpressionParser::Context{shexpr::VT_COLOR4, def.shvarType == SHVT_INT4, def.varTerm}))
      {
        report_error(parser, &val, "Invalid preshader expression");
        return eastl::nullopt;
      }

      // This updates status of static-dynamic mixing based on info inside a given elem
      if (Symbol *s = colorExpr.hasDynamicAndMaterialTermsAt())
      {
        def.hasDynStcodeRelyingOnMaterialParams = true;
        def.exprWithDynamicAndMaterialTerms = s;
      }

      if (!colorExpr.collapseNumbers(parser)) // @TODO: make this void, it can't fail at this point
        return eastl::nullopt;

      Color4 v;
      if (colorExpr.evaluate(v, parser))
        return Elem{v, expr};

      def.isDynamic |= colorExpr.isDynamic();
      hadStaticElements |= !colorExpr.isDynamic();
      return Elem{eastl::move(colorExpr), expr};
    }
    else
    {
      if (!val.expr->lhs->lhs->var_name)
      {
        report_error(parser, &val, "Arithmetic expressions are not supported for matrix. Check: %s@f44", def.baseName);
        return eastl::nullopt;
      }
      auto [vi, vt, isGlobal] = semantic::lookup_state_var(*val.expr->lhs->lhs->var_name, ctx);
      if (vi == -1)
      {
        report_error(parser, &val, "Use of undeclared shadervar '%s' in preshader initializer", val.expr->lhs->lhs->var_name);
        return eastl::nullopt;
      }
      else if (!registerInitShvarType(ShaderVarType(vt))) // registration reports it's own errors
        return eastl::nullopt;
      else if (isGlobal)
      {
        def.isDynamic = true;
        def.pairSamplerIsGlobal |= needPairSampler;
        return Elem{Elem::GlobalVar{vi, ShaderVarType(vt), val.expr->lhs->lhs->var_name->text}, val.expr->lhs->lhs->var_name};
      }
      else
      {
        def.isDynamic |= ctx.parsedSemCode().vars[vi].dynamic;
        hadStaticElements |= !ctx.parsedSemCode().vars[vi].dynamic;
        return Elem{Elem::MaterialVar{vi, ShaderVarType(vt), val.expr->lhs->lhs->var_name->text}, val.expr->lhs->lhs->var_name};
      }
    }
  };

  auto processInitializerElem = [&](external_var_value_single &val) {
    auto elemMaybe = parseInitializerElem(val, allowsArithmeticExpressions);
    if (elemMaybe)
      def.initializer.emplace_back(eastl::move(*elemMaybe));
    return elemMaybe.has_value();
  };

  if (state_block.arr)
  {
    processInitializerElem(*state_block.arr->arr0);
    for (auto *elem : state_block.arr->arrN)
      processInitializerElem(*elem);
  }
  else if (state_block.var && state_block.var->par)
  {
    if (!def.shaderVarTerm)
      report_error(parser, def.varTerm, "Wrong named const array syntax. And array must be assigned from a single array shadervar.");
    auto [vi, vt, isGlobal] = lookup_state_var(*def.shaderVarTerm, ctx);
    if (!registerInitShvarType(ShaderVarType(vt)))
      return eastl::nullopt;
    if (!isGlobal)
    {
      G_ASSERTF(0, "ICE: Array shadervars are not supposed to be in material params, may be a bug in static var evaluation");
      return eastl::nullopt;
    }
    const ShaderGlobal::Var &v = ctx.tgtCtx().globVars().getVar(vi);
    def.initializer.reserve(v.array_size);
    for (int i = 0; i < v.array_size; i++)
    {
      external_var_value_single *value = TMPMEM_ALLOC(external_var_value_single);
      arithmetic_expr *e = TMPMEM_ALLOC(arithmetic_expr);
      arithmetic_expr_md *emd = TMPMEM_ALLOC(arithmetic_expr_md);
      arithmetic_operand *op = TMPMEM_ALLOC(arithmetic_operand);
      SHTOK_ident *varNameIdent = TMPMEM_ALLOC(SHTOK_ident);
      value->builtin_var = nullptr;
      value->expr = e;
      value->expr->lhs = emd;
      value->expr->lhs->lhs = op;
      value->expr->lhs->lhs->unary_op = nullptr;
      value->expr->lhs->lhs->real_value = nullptr;
      value->expr->lhs->lhs->color_value = nullptr;
      value->expr->lhs->lhs->func = nullptr;
      value->expr->lhs->lhs->expr = nullptr;
      value->expr->lhs->lhs->cmask = nullptr;
      value->expr->lhs->lhs->var_name = varNameIdent;
      if (i > 0)
      {
        auto name = string_f("%s[%i]", def.shaderVarTerm->text, i);
        char *nameStor = TMPMEM_ALLOC_N(char, name.length() + 1);
        memcpy(nameStor, name.data(), name.length());
        nameStor[name.length()] = '\0';
        varNameIdent->text = nameStor;
        processInitializerElem(*value);
      }
      else
      {
        varNameIdent->text = def.shaderVarTerm->text;
        processInitializerElem(*value);
      }
    }
  }
  else
    processInitializerElem(*state_block.var->val);

  if (def.initializer.empty())
    return eastl::nullopt;

  def.registerSize = def.initializer.size() * elementRegisterSize;
  def.arrayElemCount = def.type == VariableType::f44 ? def.registerSize / 4 : def.registerSize;

  // Validate semantic constraints on dynamicity and use of global/material vars combinations

  if (vt_is_static_texture(def.type))
  {
    if (def.initializer.size() != 1)
      report_error(parser, def.nameSpaceTerm, "@static textures with [] syntax are not supported", def.baseName);
    else if (def.isDynamic)
    {
      if (def.initializer[0].isGlobalVar())
        report_error(parser, def.nameSpaceTerm, "@static texture %s can't be global", def.baseName);
      else
        report_error(parser, def.nameSpaceTerm, "@static texture %s can't be dynamic", def.baseName);
    }

    G_ASSERT(def.initializer[0].isMaterialVar());
    def.bindlessVarId = def.initializer[0].materialVarId();
  }
  else if (vt_is_sampled_texture(def.type))
  {
    G_ASSERT(def.initializer.size() == 1);
    if (!def.isDynamic)
      report_error(parser, def.varTerm, "Texture %s in material should use @static.", def.shaderVarTerm->text);
  }

  if (needPairSampler)
    addPairSampler();

  return def;

#undef TMPMEM_ALLOC
}

eastl::optional<LocalVarDefInfo> parse_local_var_decl(local_var_decl &decl, shc::VariantContext &ctx,
  bool ignore_color_dimension_mismatch, bool allow_override)
{
  using namespace ShaderParser;

  const ExpressionParser exprParser{ctx};
  Parser &parser = ctx.tgtCtx().sourceParseState().parser;

  shexpr::ValueType valueType;
  bool isInteger = false;
  switch (decl.type->type->number())
  {
    case SHADER_TOKENS::SHTOK_float: valueType = shexpr::VT_REAL; break;
    case SHADER_TOKENS::SHTOK_float4: valueType = shexpr::VT_COLOR4; break;
    case SHADER_TOKENS::SHTOK_int4:
      valueType = shexpr::VT_COLOR4;
      isInteger = true;
      break;
    default: G_ASSERT(0);
  }

  int varNameId = ctx.tgtCtx().varNameMap().addVarId(decl.name->text);

  // check for variable names
  if (ctx.tgtCtx().globVars().getVarInternalIndex(varNameId) >= 0)
  {
    report_error(parser, decl.name, "variable with name '%s' already declared as global variable!", decl.name->text);
    return eastl::nullopt;
  }

  if (ctx.parsedSemCode().find_var(varNameId) >= 0)
  {
    report_error(parser, decl.name, "variable with name '%s' already declared!", decl.name->text);
    return eastl::nullopt;
  }

  auto &locVarTable = ctx.localStcodeVars();

  int varId = locVarTable.getVariableId(varNameId);
  if (varId != -1 && !allow_override)
  {
    report_error(parser, decl.name, "local variable with name '%s' already declared!", decl.name->text);
    return eastl::nullopt;
  }
  else if (varId == -1)
    varId = locVarTable.addVariable(LocalVar(varNameId, valueType, isInteger));

  LocalVar *var = locVarTable.getVariableById(varId);
  G_ASSERT(var);

  // initialize variable
  auto rootExpr = eastl::make_unique<ComplexExpression>(decl.expr, valueType);

  if (!exprParser.parseExpression(*decl.expr, rootExpr.get(), ExpressionParser::Context{valueType, isInteger, decl.name}))
    return eastl::nullopt;

  if (
    !ignore_color_dimension_mismatch && valueType == shexpr::VT_COLOR4 && rootExpr->getChannels() != 1 && rootExpr->getChannels() != 4)
  {
    report_error(parser, decl.name, "Invalid number of channels (%d) for float4 local variable with name '%s' (must be 1 or 4)",
      rootExpr->getChannels(), decl.name->text);
    return eastl::nullopt;
  }
  if (valueType == shexpr::VT_REAL && rootExpr->getChannels() != 1)
  {
    report_error(parser, decl.name, "Invalid number of channels (%d) for float local variable with name '%s' (must be 1)",
      rootExpr->getChannels(), decl.name->text);
    return eastl::nullopt;
  }

  if (!rootExpr->collapseNumbers(parser))
    return eastl::nullopt;

  var->isDynamic = rootExpr->isDynamic();
  var->dependsOnDynVarsAndMaterialParams = rootExpr->hasDynamicAndMaterialTermsAt() != nullptr;

  // try to get constant value
  Color4 v;
  if (valueType == shexpr::VT_REAL)
  {
    var->isConst = rootExpr->evaluate(v.r, parser);
  }
  else if (valueType == shexpr::VT_COLOR4)
  {
    var->isConst = rootExpr->evaluate(v, parser);
  }
  if (var->isConst)
  {
    var->cv.c.set(v);
    return LocalVarDefInfo{var, eastl::move(rootExpr)};
  }

  // assign as dynamic variable
  if (!rootExpr->canConvert(valueType))
  {
    report_error(parser, decl.name, "local variable '%s' must be initialized with %s expression, not %s (operands = %d, %d)!",
      decl.name->text, Expression::__getName(valueType), Expression::__getName(rootExpr->getValueType()), rootExpr->getOperandCount(),
      rootExpr->getType());
  }

  return LocalVarDefInfo{var, eastl::move(rootExpr)};
}

} // namespace semantic
