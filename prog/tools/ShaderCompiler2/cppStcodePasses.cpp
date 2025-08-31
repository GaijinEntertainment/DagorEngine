// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cppStcodePasses.h"
#include "variantAssembly.h"
#include "shaderSemantic.h"


StcodeBranchedBuildEvalCB::StcodeBranchedBuildEvalCB(shc::VariantContext &a_ctx) :
  ctx{a_ctx}, parser{ctx.tgtCtx().sourceParseState().parser}
{
  for (auto &pt : ctx.parsedSemCode().passes)
    if (pt && pt->pass)
    {
      for (uintptr_t stmtNode : pt->pass->usedConstStatAstNodes)
        preshaderStatementsUsedInAnyVariant.insert((const state_block_stat *)stmtNode);
      for (auto [boolNode, evalRes] : pt->pass->boolAstNodesEvaluationResults)
      {
        auto [it, insterted] = boolEvalResults.emplace(boolNode, evalRes);
        if (!insterted && it->second.isConst)
        {
          G_ASSERT(evalRes.isConst);
          if (evalRes.value != it->second.value)
            it->second.isConst = false;
        }
      }
    }
}

void StcodeBranchedBuildEvalCB::eval_external_block(external_state_block &state_block)
{
  auto stageMaybe = semantic::parse_state_block_stage(state_block.scope->text);
  G_ASSERT(stageMaybe);

  ShaderStage stage = *stageMaybe;

  const auto evalIf = [this, stage](const state_block_if_stat &s, auto &&eval_if_stat) -> void {
    G_ASSERT(s.expr);

    const auto evalStats = [&, this](const auto &stats) {
      for (const auto &stat : stats)
      {
        if (stat->stblock_if_stat)
          eval_if_stat(*stat->stblock_if_stat, eval_if_stat);
        else
          evalExternalBlockStat(*stat, stage);
      }
    };

    int res = eval_if(*s.expr);
    if (res != ShaderEvalCB::IF_FALSE)
    {
      evalStats(s.true_stat);
    }
    eval_else(*s.expr);
    if (res != ShaderEvalCB::IF_TRUE)
    {
      evalStats(s.false_stat);
      if (s.else_if)
        eval_if_stat(*s.else_if, eval_if_stat);
    }
    eval_endif(*s.expr);
  };

  for (const auto stat : state_block.stblock_stat)
  {
    if (stat->stblock_if_stat)
      evalIf(*stat->stblock_if_stat, evalIf);
    else
      evalExternalBlockStat(*stat, stage);
  }
}

void StcodeBranchedBuildEvalCB::evalExternalBlockStat(const state_block_stat &state_block, ShaderStage stage)
{
  using semantic::VariableType;

  if (!preshaderStatementsUsedInAnyVariant.count(&state_block))
    return;

  ctx.cppStcode().cppStcode.reportStageUsage(stage);

  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);

  VariableType vt = semantic::parse_named_const_type(state_block);
  G_ASSERT(vt != VariableType::Unknown);

  const auto parsedDefMaybe = semantic::parse_named_const_definition(state_block, stage, vt, ctx, &rm_alloc);
  G_ASSERT(parsedDefMaybe);

  const semantic::NamedConstDefInfo &def = *parsedDefMaybe;
  constexpr int ID_PLACEHOLDER = 0;

  if (!def.isDynamic)
    return;

  bool ok = assembly::build_stcode_for_named_const<assembly::StcodeBuildFlagsBits::CPP>(def, ID_PLACEHOLDER, ctx, &rm_alloc, false);
  G_ASSERT(ok);

  if (def.pairSamplerTmpDecl && def.isDynamic && def.hardcodedRegister == -1)
  {
    auto [samplerVarId, _1, _2] = semantic::lookup_state_var(*def.pairSamplerTmpDecl->name, ctx);
    const String samplerConstName{0, "%s%s", def.mangledName.c_str(), def.pairSamplerBindSuffix};
    assembly::build_stcode_for_pair_sampler<assembly::StcodeBuildFlagsBits::CPP>(samplerConstName.c_str(), def.pairSamplerName.c_str(),
      ID_PLACEHOLDER, stage, samplerVarId, def.pairSamplerIsGlobal, ctx);
  }
}

void StcodeBranchedBuildEvalCB::eval_shader_locdecl(local_var_decl &s)
{
  // @TODO: cache between passes?
  auto parseResMaybe = semantic::parse_local_var_decl(s, ctx, false, true);
  if (!parseResMaybe)
    return;

  if (!parseResMaybe->var->isConst)
    assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::CPP>(parseResMaybe->var, parseResMaybe->expr.get(), s.name, ctx);
}

void StcodeBranchedBuildEvalCB::eval_bool_decl(bool_decl &d)
{
  auto valMaybe = evalBoolStatIfExists(*d.expr);
  if (!valMaybe)
    return;

  ShVarBool val = *valMaybe;
  if (val.isConst)
  {
    ctx.cppStcode().cppStcode.setVarValue(d.name->text, boolToStr(val.value));
  }
  else
  {
    StcodeBuilder builder{};
    buildConditionString(*d.expr, builder);
    auto expr = builder.release();
    ctx.cppStcode().cppStcode.setVarValue(d.name->text, expr.c_str());
  }

  ctx.localBoolVars().add(d, parser, true);
}

int StcodeBranchedBuildEvalCB::eval_if(bool_expr &e)
{
  ShVarBool val = evalBoolStat(e);
  if (val.isConst)
    return val.value ? IF_TRUE : IF_FALSE;

  StcodeBuilder builder{};
  buildConditionString(e, builder);
  auto cond = builder.release();
  ctx.cppStcode().cppStcode.addIfClause(cond.c_str());

  return IF_BOTH;
}

void StcodeBranchedBuildEvalCB::eval_else(bool_expr &e)
{
  auto val = evalBoolStat(e);
  if (!val.isConst)
    ctx.cppStcode().cppStcode.addElseClause();
}

void StcodeBranchedBuildEvalCB::eval_endif(bool_expr &e)
{
  auto val = evalBoolStat(e);
  if (!val.isConst)
    ctx.cppStcode().cppStcode.addBlockClosing();
}

void StcodeBranchedBuildEvalCB::buildConditionBoolValue(const bool_value &val, StcodeBuilder &out)
{
  ShVarBool res = evalBoolStat(val);
  if (res.isConst)
  {
    out.emplaceBack(boolToStr(res.value));
    return;
  }

  if (val.bool_var)
  {
    auto [e, isGlobal, hasMultipleDeclarations] = semantic::get_bool_expr(*val.bool_var, parser, ctx, ctx.tgtCtx());
    if (isGlobal || !hasMultipleDeclarations)
      buildConditionString(*e, out);
    else
      out.emplaceBack(val.bool_var->text);
  }
  else if (val.maybe)
  {
    auto [e, isGlobal, _] = semantic::get_bool_maybe(*val.maybe_bool_var, ctx, ctx.tgtCtx());
    if (e)
    {
      if (isGlobal)
        buildConditionString(*e, out);
      else
        out.emplaceBack(val.maybe_bool_var->text);
    }
    else
      out.emplaceBack(boolToStr(false));
  }
  else if (val.interval_ident)
  {
    // @TODO: do I have this lookup part pulled out somewhere?
    auto [vi, vt, isGlobal] = semantic::lookup_state_var(*val.interval_ident, ctx, true);

    const IntervalList *intervalList = &ctx.variant().intervals;
    const shc::TargetContext &tgtCtx = ctx.tgtCtx();

    int e_interval_ident_id = tgtCtx.intervalNameMap().getNameId(val.interval_ident->text);
    int intervalIndex = intervalList->getIntervalIndex(e_interval_ident_id);
    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      intervalIndex = tgtCtx.globVars().getIntervalList().getIntervalIndex(e_interval_ident_id);
      G_ASSERT(intervalIndex != INTERVAL_NOT_INIT);
      intervalList = &tgtCtx.globVars().getIntervalList();

      if (vi < 0)
        isGlobal = true;
    }
    const Interval *interv = intervalList->getInterval(intervalIndex);

    int valId = tgtCtx.intervalNameMap().getNameId(val.interval_value->text);
    G_ASSERT(valId != -1);
    const auto &valueBounds = interv->getValueByNameId(valId)->getBounds();

    G_ASSERT(valueBounds.getMin() != IntervalValue::VALUE_INFINITY);
    G_ASSERT(valueBounds.getMax() != IntervalValue::VALUE_NEG_INFINITY);
    G_ASSERT(valueBounds.getMin() != IntervalValue::VALUE_NEG_INFINITY || valueBounds.getMax() != IntervalValue::VALUE_INFINITY);

    eastl::string deref =
      isGlobal ? string_f("*%s", val.interval_ident->text)
               : string_f("VAR(%s, %s)", val.interval_ident->text, stcode::shadervar_type_to_stcode_type(ShaderVarType(vt)));

    auto makeG = [&] { return string_f("((float)%s >= %.8ff)", deref.c_str(), valueBounds.getMax()); };
    auto makeL = [&] { return string_f("((float)%s < %.8ff)", deref.c_str(), valueBounds.getMin()); };
    auto makeGe = [&] { return string_f("((float)%s >= %.8ff)", deref.c_str(), valueBounds.getMin()); };
    auto makeLe = [&] { return string_f("((float)%s < %.8ff)", deref.c_str(), valueBounds.getMax()); };
    auto makeEqNeq = [&](bool eq) {
      if (valueBounds.getMin() == IntervalValue::VALUE_NEG_INFINITY)
        return string_f("%s((float)%s < %.8ff)", eq ? "" : "!", deref.c_str(), valueBounds.getMax());
      else if (valueBounds.getMax() == IntervalValue::VALUE_INFINITY)
        return string_f("%s((float)%s >= %.8ff)", eq ? "" : "!", deref.c_str(), valueBounds.getMin());
      else
      {
        return string_f("%s((float)%s >= %.8ff && (float)%s < %.8ff)", eq ? "" : "!", deref.c_str(), valueBounds.getMin(),
          deref.c_str(), valueBounds.getMax());
      }
    };

    switch (val.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: out.emplaceBack(makeEqNeq(true).c_str()); break;
      case SHADER_TOKENS::SHTOK_assign: out.emplaceBack(makeEqNeq(true).c_str()); break;
      case SHADER_TOKENS::SHTOK_greater: out.emplaceBack(makeG().c_str()); break;
      case SHADER_TOKENS::SHTOK_greatereq: out.emplaceBack(makeGe().c_str()); break;
      case SHADER_TOKENS::SHTOK_smaller: out.emplaceBack(makeL().c_str()); break;
      case SHADER_TOKENS::SHTOK_smallereq: out.emplaceBack(makeLe().c_str()); break;
      case SHADER_TOKENS::SHTOK_noteq: out.emplaceBack(makeEqNeq(false).c_str()); break;
      default: G_ASSERT(0);
    }
  }
  else if (val.texture_name)
  {
    auto [vi, vt, isGlobal] = semantic::lookup_state_var(*val.texture_name, ctx);
    G_ASSERT(vi >= 0);

    const char *op;
    switch (val.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq:
      case SHADER_TOKENS::SHTOK_assign: op = "=="; break;
      case SHADER_TOKENS::SHTOK_noteq: op = "!="; break;
      default: G_ASSERT(0);
    }

    if (isGlobal)
    {
      out.emplaceBack("*");
      out.emplaceBack(val.texture_name->text);
    }
    else
    {
      out.emplaceBack("VAR(");
      out.emplaceBack(val.texture_name->text);
      out.emplaceBack(", uint)");
    }
    out.emplaceBack(op);
    out.emplaceBack("0u");
  }
  else
  {
    G_ASSERT(0);
  }
}

void StcodeBranchedBuildEvalCB::buildConditionBoolNot(const not_expr &e, StcodeBuilder &out)
{
  if (e.is_not)
    out.emplaceBack("!");
  if (e.value->expr)
  {
    out.emplaceBack("(");
    buildConditionString(*e.value->expr, out);
    out.emplaceBack(")");
  }
  else
    buildConditionBoolValue(*e.value, out);
}

void StcodeBranchedBuildEvalCB::buildConditionBoolAnd(const and_expr &e, StcodeBuilder &out)
{
  if (e.value)
  {
    buildConditionBoolNot(*e.value, out);
    return;
  }

  out.emplaceBack("(");
  buildConditionBoolAnd(*e.a, out);
  out.emplaceBack(" && ");
  buildConditionBoolNot(*e.b, out);
  out.emplaceBack(")");
}

void StcodeBranchedBuildEvalCB::buildConditionString(const bool_expr &e, StcodeBuilder &out)
{
  ShVarBool res = evalBoolStat(e);
  if (res.isConst)
  {
    out.emplaceBack(boolToStr(res.value));
    return;
  }

  if (e.value)
  {
    buildConditionBoolAnd(*e.value, out);
    return;
  }

  out.emplaceBack("(");
  buildConditionString(*e.a, out);
  out.emplaceBack(" || ");
  buildConditionBoolAnd(*e.b, out);
  out.emplaceBack(")");
}