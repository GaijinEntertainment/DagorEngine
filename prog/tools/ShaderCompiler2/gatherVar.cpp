// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gatherVar.h"
#include "semUtils.h"
#include "globVarSem.h"
#include "globVar.h"
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

extern SCFastNameMap glob_string_table;
extern bool isDebugModeEnabled;

/*********************************
 *
 * class GatherVarShaderEvalCB
 *
 *********************************/
GatherVarShaderEvalCB::GatherVarShaderEvalCB(ShaderSyntaxParser &_parser, const ShHardwareOptions &_opt, Terminal *shname,
  const char *hlsl_vs, const char *hlsl_hs, const char *hlsl_ds, const char *hlsl_gs, const char *hlsl_ps, const char *hlsl_cs,
  const char *hlsl_ms, const char *hlsl_as) :
  parser(_parser),
  hasHWFlag(false),
  hwStack(tmpmem),
  hwCount(0),
  opt(_opt),
  dynStack(tmpmem),
  dynCount(0),
  hasDynFlag(false),
  shname_token(shname)
{
  varTypes.setIntervalList(&intervals);
  dynVarTypes.setIntervalList(&intervals);
  allRefStaticVars.setIntervalList(&intervals);

  hlslVs = hlsl_vs;
  hlslPs = hlsl_ps;
  hlslHs = hlsl_hs;
  hlslDs = hlsl_ds;
  hlslGs = hlsl_gs;
  hlslCs = hlsl_cs;
  hlslMs = hlsl_ms;
  hlslAs = hlsl_as;

  if (isDebugModeEnabled)
  {
    DataBlock *blk = shc::getAssumedVarsBlock();
    if (blk)
    {
      blk = blk->addBlock(shname_token->text);
      blk->setReal(debug_mode_enabled_interval, 1.0f);
    }
  }
}

void GatherVarShaderEvalCB::eval_channel_decl(channel_decl &s, int str_id)
{
  // channels must be independed by hardware
  if (hwCount > 0)
  {
    error("cannot declare hardware-depended channels (check 'if' condition(s) for hw-depended flags)!", s.type->type);
  }

  // channels must be independed by dynamic variables
  if (dynCount > 0)
  {
    error("cannot declare dynamic-depended channels (check 'if' condition(s) for dynamic variants)!", s.type->type);
  }
}

// clang-format off
void GatherVarShaderEvalCB::eval_assume_stat(assume_stat &s)
{
  add_assume_to_blk(shname_token->text, intervals, s, parser);
}
// clang-format on

void GatherVarShaderEvalCB::eval_bool_decl(bool_decl &decl)
{
  BoolVar::add(true, decl, parser, true);

  String hlsl_bool_var(0, "##bool %s\n", decl.name->text);
  hlslVs.append(hlsl_bool_var);
  hlslHs.append(hlsl_bool_var);
  hlslDs.append(hlsl_bool_var);
  hlslGs.append(hlsl_bool_var);
  hlslPs.append(hlsl_bool_var);
  hlslCs.append(hlsl_bool_var);
  hlslMs.append(hlsl_bool_var);
  hlslAs.append(hlsl_bool_var);
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
  BoolVar::add(true, decl, parser, true);
}

int GatherVarShaderEvalCB::add_message(const char *message, bool file_name)
{
  float value = 0.0f;
  if (shc::getAssumedValue(debug_mode_enabled_interval, shname_token->text, true, value))
    if (value > 0.0f)
    {
      int id = messages.addNameId(message);
      if (!file_name)
        nonFilenameMessages.set(id, true);
      return id;
    }

  return -1;
}

void GatherVarShaderEvalCB::eval_static(static_var_decl &s)
{
  // variables must be independed by hardware
  if (hwCount > 0)
  {
    error("cannot declare hardware-depended variables (check 'if' condition for hw-depended flags)!", s.name);
    return;
  }

  // variables must be independed by dynamic variants
  if (dynCount > 0)
  {
    error("cannot declare dynamic-depended variables (check 'if' condition(s) for dynamic variants)!", s.name);
  }

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
  {
    error("cannot declare dynamic-depended intervals (check 'if' condition(s) for dynamic variants)!", interv.name);
  }

  // check for global
  ShaderVariant::VarType type = ShaderVariant::VARTYPE_INTERVAL;

  if (ShaderGlobal::get_var_internal_index(VarMap::getVarId(interv.name->text)) != -1)
  {
    type = ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL;
  }

  add_interval(intervals, interv, type, parser);
}

ShVarBool GatherVarShaderEvalCB::addInterval(const char *intervalName, Terminal *terminal, bool_value *e)
{
  // try to find registered interval
  int intervalName_id = IntervalValue::getIntervalNameId(intervalName);
  ShaderVariant::ExtType intervalIndex = intervals.getIntervalIndex(intervalName_id);
  const Interval *interv = intervals.getInterval(intervalIndex);

  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(intervalName_id);
    interv = ShaderGlobal::getIntervalList().getInterval(intervalIndex);

    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      error(String(256, "interval %s not found!", intervalName), terminal);
      return ShVarBool(false, true);
    }
  }

  bool is_static = staticVars.getNameId(intervalName) != -1;
  bool is_dynamic = dynamicVars.getNameId(intervalName) != -1;

  if (shc::getAssumedVarsBlock())
  {
    bool is_global = !is_static && !is_dynamic;
    float value = 0;

    if (shc::getAssumedValue(intervalName, shname_token->text, is_global, value))
    {
      ShaderVariant::ValueType valtype = interv->normalizeValue(value);
      // debug ( "use assume %.3f -> %d", value, valtype );

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

      assumedIntervals.emplace_back(intervalName, (uint8_t)value);

      String error_msg;
      bool bool_result = interv->checkExpression(valtype, expr, e->interval_value->text, error_msg);
      if (!error_msg.empty())
        error(error_msg, terminal);

      return ShVarBool(bool_result, true);
    }
  }

  if (!is_static && !is_dynamic && ShaderGlobal::get_var_internal_index(VarMap::getVarId(intervalName)) == -1)
  {
    error(String(0, "Non-assumed interval '%s' doesn't have a corresponding var", intervalName), terminal);
  }

  // ok, found, adding variant flag
  ShaderVariant::TypeTable *lst = NULL;
  if (interv->getVarType() != ShaderVariant::VARTYPE_INTERVAL || !is_static)
  {
    lst = &dynVarTypes;
    hasDynFlag = true;
  }
  else
  {
    lst = &varTypes;
  }
  lst->addType(interv->getVarType(), intervalIndex, true);
  return ShVarBool(true, false);
}

bool shc::getAssumedValue(const char *varname, const char *shader, bool global, float &out_value)
{
  DataBlock *ablk = getAssumedVarsBlock();
  if (!ablk)
    return false;

  if (shader)
  {
    // debug ( "get local assumes for shader %s (var=%s)", shader, varname );
    if (!global)
      ablk = ablk->getBlockByName(shader);
    else
    {
      DataBlock *b = ablk->getBlockByName(shader);
      if (b && b->findParam(varname) != -1)
        ablk = b;
    }
  }
  else
    ; // debug ( "get global assumes (var=%s)", varname );

  if (!ablk)
    return false;

  int idx = ablk->findParam(varname);

  if (idx == -1)
    return false;

  switch (ablk->getParamType(idx))
  {
    case DataBlock::TYPE_INT: out_value = (real)ablk->getInt(idx); return true;
    case DataBlock::TYPE_REAL: out_value = ablk->getReal(idx); return true;
    default: sh_debug(SHLOG_ERROR, "Assume variables: type in \"%s\" is neither \"real\" nor \"int\"", varname);
  }
  return false;
}

ShVarBool GatherVarShaderEvalCB::addTextureInterval(const char *textureName, Terminal *terminal, bool_value &e)
{
  // debug("texture found '%s' (id=%d)", textureName, VarMap::getVarId(textureName));
  //  try to find registered interval
  bool isGlobal = ShaderGlobal::get_var_internal_index(VarMap::getVarId(textureName)) >= 0;
  ShaderVariant::VarType type = isGlobal ? ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL : ShaderVariant::VARTYPE_INTERVAL;

  int textureName_id = IntervalValue::getIntervalNameId(textureName);
  int intervalIndex = intervals.getIntervalIndex(textureName_id);

  if (intervalIndex == INTERVAL_NOT_INIT)
    intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(textureName_id);

  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    // interval not found - register new fake-interval
    // debug("register new interval with name '%s'", (char*)textureName);

    Interval newInterval(textureName, type);
    newInterval.addValue("NULL", 1.0f);
    newInterval.addValue("EXISTS", IntervalValue::VALUE_INFINITY);

    if (!intervals.addInterval(newInterval))
    {
      error("interval already exists and their type diffirent from new interval!", terminal);
      return ShVarBool(false, false);
    }
    intervalIndex = intervals.getIntervalIndex(newInterval.getNameId());
  }

  if (shc::getAssumedVarsBlock())
  {
    float value = 0;
    if (shc::getAssumedValue(textureName, shname_token->text, isGlobal, value))
    {
      switch (e.cmpop->op->num)
      {
        case SHADER_TOKENS::SHTOK_eq: return ShVarBool(value < 1, true);
        case SHADER_TOKENS::SHTOK_noteq: return ShVarBool(value >= 1, true);
      }
      error(String(128, "bad cmp op for texture interval: %d", e.cmpop->op->num), terminal);
    }
  }

  bool is_static = staticVars.getNameId(textureName) != -1;

  if (is_static)
    varTypes.addType(type, intervalIndex, true);
  else
  {
    hasDynFlag = true;
    dynVarTypes.addType(type, intervalIndex, true);
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
  int varTypesStartIdx = varTypes.getCount();
  int dynVarTypesStartIdx = dynVarTypes.getCount();
  ShVarBool v = ShaderParser::eval_shader_bool(e, *this);
  for (int i = varTypesStartIdx; i < varTypes.getCount(); i++)
    allRefStaticVars.addType(varTypes.getType(i).type, varTypes.getType(i).extType, true);
  if (v.isConst && !v.value && varTypesStartIdx + dynVarTypesStartIdx != varTypes.getCount() + dynVarTypes.getCount())
  {
    String cond;
    ShaderParser::build_bool_expr_string(e, cond);
    debug("reject gathered variant vars: %d;%d -> %d;%d due to const.false expr=\"%s\"", varTypes.getCount(), dynVarTypes.getCount(),
      varTypesStartIdx, dynVarTypesStartIdx, cond);
    varTypes.shrinkTo(varTypesStartIdx);
    dynVarTypes.shrinkTo(dynVarTypesStartIdx);
  }
  return v;
}
ShVarBool GatherVarShaderEvalCB::eval_bool_value(bool_value &e)
{
  if (e.two_sided)
  {
    varTypes.addType(ShaderVariant::VARTYPE_MODE, ShaderVariant::TWO_SIDED, true);
    return ShVarBool(true, false);
  }
  else if (e.real_two_sided)
  {
    varTypes.addType(ShaderVariant::VARTYPE_MODE, ShaderVariant::REAL_TWO_SIDED, true);
    return ShVarBool(true, false);
  }
  else if (e.shader)
  {
    ShVarBool b = ShVarBool(AssembleShaderEvalCB::compareShader(shname_token, e), true);
    //    debug("check shader '%s' '%s': c=%d v=%d", shname_token->text, e.shader->text, b.isConst, b.value);
    return b;
  }
  else if (e.hw)
  {
    // hasHWFlag = true;//was only set true on older hw
    return ShVarBool(AssembleShaderEvalCB::compareHW(e, opt), true);
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
    auto expr = BoolVar::get_expr(*e.bool_var, parser);
    if (expr)
      return ShVarBool(eval_expr(*expr).value, false);
  }
  else if (e.maybe)
  {
    auto expr = BoolVar::maybe(*e.maybe_bool_var);
    if (expr)
      return ShVarBool(eval_expr(*expr).value, false);
    else
      return ShVarBool(true, false);
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

bool GatherVarShaderEvalCB::end_eval(CodeSourceBlocks &vs_blk, CodeSourceBlocks &hs_blk, CodeSourceBlocks &ds_blk,
  CodeSourceBlocks &gs_blk, CodeSourceBlocks &ps_blk, CodeSourceBlocks &cs_blk, CodeSourceBlocks &ms_blk, CodeSourceBlocks &as_blk)
{
  G_ASSERT(messages.nameCount() == 0);
  messages = glob_string_table;

  if (!vs_blk.parseSourceCode("vs", hlslVs, *this))
    return false;
  if (!hs_blk.parseSourceCode("hs", hlslHs, *this))
    return false;
  if (!ds_blk.parseSourceCode("ds", hlslDs, *this))
    return false;
  if (!gs_blk.parseSourceCode("gs", hlslGs, *this))
    return false;
  if (!ps_blk.parseSourceCode("ps", hlslPs, *this))
    return false;
  if (!cs_blk.parseSourceCode("cs", hlslCs, *this))
    return false;
  if (!ms_blk.parseSourceCode("ms", hlslMs, *this))
    return false;
  if (!as_blk.parseSourceCode("as", hlslAs, *this))
    return false;
  return true;
}

void GatherVarShaderEvalCB::error(const char *msg, Symbol *s)
{
  if (s)
  {
    eastl::string str = msg;
    if (!s->macro_call_stack.empty())
      str.append("\nCall stack:\n");
    for (auto it = s->macro_call_stack.rbegin(); it != s->macro_call_stack.rend(); ++it)
      str.append_sprintf("  %s()\n    %s(%i)\n", it->name, parser.get_lex_parser().get_filename(it->file), it->line);
    parser.get_lex_parser().set_error(s->file_start, s->line_start, s->col_start, str.c_str());
  }
  else
    parser.get_lex_parser().set_error(msg);
}


void GatherVarShaderEvalCB::warning(const char *msg, Symbol *s)
{
  G_ASSERT(s);
  parser.get_lex_parser().set_warning(s->file_start, s->line_start, s->col_start, msg);
}

int GatherVarShaderEvalCB::eval_if(bool_expr &e)
{
  String cond("##if ");
  ShaderParser::build_bool_expr_string(e, cond, false);
  cond.append("\n");
  hlslVs.append(cond);
  hlslHs.append(cond);
  hlslDs.append(cond);
  hlslGs.append(cond);
  hlslPs.append(cond);
  hlslCs.append(cond);
  hlslMs.append(cond);
  hlslAs.append(cond);

  // debug("expr-----------");
  ShVarBool b = eval_expr(e);

  // count hardware flag
  hwStack.push_back(hasHWFlag);
  if (hasHWFlag)
    hwCount++;
  hasHWFlag = false;

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
  hlslVs.append("##else\n");
  hlslHs.append("##else\n");
  hlslDs.append("##else\n");
  hlslGs.append("##else\n");
  hlslPs.append("##else\n");
  hlslCs.append("##else\n");
  hlslMs.append("##else\n");
  hlslAs.append("##else\n");
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

  uint32_t hlsl_types = 0;
  addSourceCode(hlslPs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslVs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslHs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslDs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslGs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslMs, &s, hlsl.c_str(), parser);
  addSourceCode(hlslAs, &s, hlsl.c_str(), parser);
#endif
}

void GatherVarShaderEvalCB::eval(immediate_const_block &s)
{
  int words = semutils::int_number(s.count->text);
  if (words <= 0)
    return;
  if (words > 4)
    error("maximum immediate words is 4", s.count);
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
  slot = 7;
#endif
  hlsl.printf(128, "#define NUM_IMMEDIATE_DWORDS %u\n", words);
  hlsl.aprintf(128, "cbuffer immediate_const_buffer:register(b%d){ uint", slot);
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "%s immediate_dword_%d", i != 0 ? "," : "", i);
  hlsl.aprintf(32, ";};\n");
  for (int i = 0; i < words; ++i)
    hlsl.aprintf(32, "uint get_immediate_dword_%d() {return immediate_dword_%d;}\n", i, i);
#endif

  uint32_t hlsl_types = 0;
  if (strstr(s.scope->text, "ps") != 0)
    addSourceCode(hlslPs, s.scope, hlsl.c_str(), parser);
  else if (strstr(s.scope->text, "vs") != 0)
  {
    addSourceCode(hlslVs, s.scope, hlsl.c_str(), parser);
    // hs, ds and gs stage also see vs sources so they need this also
    addSourceCode(hlslGs, s.scope, hlsl.c_str(), parser);
    addSourceCode(hlslHs, s.scope, hlsl.c_str(), parser);
    addSourceCode(hlslDs, s.scope, hlsl.c_str(), parser);
  }
  else if (strstr(s.scope->text, "cs") != 0)
    addSourceCode(hlslCs, s.scope, hlsl.c_str(), parser);
  else if (strstr(s.scope->text, "ms") != 0)
  {
    addSourceCode(hlslMs, s.scope, hlsl.c_str(), parser);
  }
  else if (strstr(s.scope->text, "as") != 0)
  {
    addSourceCode(hlslAs, s.scope, hlsl.c_str(), parser);
  }
  else
    error("unknown hlsl block", s.scope);
}


void GatherVarShaderEvalCB::eval_hlsl_decl(hlsl_local_decl_class &sh)
{
  if (!ShaderParser::validate_hardcoded_regs_in_hlsl_block(sh.text))
    return;

  uint32_t hlsl_types = HLSL_ALL;
  if (sh.ident)
  {
    hlsl_types = 0;
    if (strcmp(sh.ident->text, "ps") == 0)
      hlsl_types |= HLSL_PS;
    else if (strcmp(sh.ident->text, "vs") == 0)
      hlsl_types |= HLSL_VS;
    else if (strcmp(sh.ident->text, "cs") == 0)
      hlsl_types |= HLSL_CS;
    else if (strcmp(sh.ident->text, "ds") == 0)
      hlsl_types |= HLSL_DS;
    else if (strcmp(sh.ident->text, "hs") == 0)
      hlsl_types |= HLSL_HS;
    else if (strcmp(sh.ident->text, "gs") == 0)
      hlsl_types |= HLSL_GS;
    else if (strcmp(sh.ident->text, "ms") == 0)
      hlsl_types |= HLSL_MS;
    else if (strcmp(sh.ident->text, "as") == 0)
      hlsl_types |= HLSL_AS;
    else
      error(String(0, "Unexpected scope %s", sh.ident->text), sh.ident);
  }

  if (hlsl_types & HLSL_VS)
    hlsl_types |= HLSL_DS | HLSL_GS | HLSL_HS;

  if (hlsl_types & HLSL_AS)
    hlsl_types |= HLSL_MS;

  if (hlsl_types & HLSL_PS)
    addSourceCode(hlslPs, sh.text, parser);
  if (hlsl_types & HLSL_VS)
    addSourceCode(hlslVs, sh.text, parser);
  if (hlsl_types & HLSL_CS)
    addSourceCode(hlslCs, sh.text, parser);
  if (hlsl_types & HLSL_GS)
    addSourceCode(hlslGs, sh.text, parser);
  if (hlsl_types & HLSL_DS)
    addSourceCode(hlslDs, sh.text, parser);
  if (hlsl_types & HLSL_HS)
    addSourceCode(hlslHs, sh.text, parser);
  if (hlsl_types & HLSL_MS)
    addSourceCode(hlslMs, sh.text, parser);
  if (hlsl_types & HLSL_AS)
    addSourceCode(hlslAs, sh.text, parser);
}

void GatherVarShaderEvalCB::eval_endif(bool_expr &e)
{
  G_ASSERT(hwStack.size() > 0);
  bool v = hwStack.back();
  hwStack.pop_back();

  if (v)
  {
    G_ASSERT(hwCount > 0);
    hwCount--;
  }

  G_ASSERT(dynStack.size() > 0);
  v = dynStack.back();
  dynStack.pop_back();

  if (v)
  {
    G_ASSERT(dynCount > 0);
    dynCount--;
  }
  hlslVs.append("##endif\n");
  hlslHs.append("##endif\n");
  hlslDs.append("##endif\n");
  hlslGs.append("##endif\n");
  hlslPs.append("##endif\n");
  hlslCs.append("##endif\n");
  hlslMs.append("##endif\n");
  hlslAs.append("##endif\n");
}
