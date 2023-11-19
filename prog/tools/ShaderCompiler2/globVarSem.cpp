// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "globVarSem.h"
#include "shsyn.h"
#include "shExprParser.h"
#include "semUtils.h"
#include "shCompiler.h"

#include "globVar.h"
#include <shaders/dag_shaderCommon.h>
#include "varMap.h"
#include "intervals.h"

using namespace ShaderParser;
using namespace ShaderTerminal;

namespace ShaderParser
{
void error(const char *msg, Terminal *t, ShaderSyntaxParser &parser)
{
  if (t)
  {
    eastl::string str = msg;
    if (!t->macro_call_stack.empty())
      str.append("\nCall stack:\n");
    for (auto it = t->macro_call_stack.rbegin(); it != t->macro_call_stack.rend(); ++it)
      str.append_sprintf("  %s()\n    %s(%i)\n", it->name, parser.get_lex_parser().get_filename(it->file), it->line);
    parser.get_lex_parser().set_error(t->file_start, t->line_start, t->col_start, str.c_str());
  }
  else
    parser.get_lex_parser().set_error(msg);
}

static void warning(const char *msg, Terminal *t, ShaderSyntaxParser &parser)
{
  G_ASSERT(t);
  parser.get_lex_parser().set_warning(t->file_start, t->line_start, t->col_start, msg);
}

// add a new global variable to a global variable list
void add_global_var(global_var_decl *decl, ShaderTerminal::ShaderSyntaxParser &parser)
{
  if (!decl)
    return;

  int t;
  switch (decl->type->type->num)
  {
    case SHADER_TOKENS::SHTOK_float4: t = SHVT_COLOR4; break;
    case SHADER_TOKENS::SHTOK_float: t = SHVT_REAL; break;
    case SHADER_TOKENS::SHTOK_int: t = SHVT_INT; break;
    case SHADER_TOKENS::SHTOK_int4: t = SHVT_INT4; break;
    case SHADER_TOKENS::SHTOK_float4x4: t = SHVT_FLOAT4X4; break;
    case SHADER_TOKENS::SHTOK_texture: t = SHVT_TEXTURE; break;
    case SHADER_TOKENS::SHTOK_buffer: t = SHVT_BUFFER; break;
    case SHADER_TOKENS::SHTOK_const_buffer: t = SHVT_BUFFER; break;
    default: G_ASSERT(0);
  }
  int variableNameID = VarMap::addVarId(decl->name->text);

  int v = ShaderGlobal::get_var_internal_index(variableNameID);
  if (v >= 0)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "global variable '%s' already declared in ", decl->name->text);
    message += parser.get_lex_parser().get_symbol_location(variableNameID, SymbolType::GLOBAL_VARIABLE);
    error(message.c_str(), decl->name, parser);
    return;
  }

  parser.get_lex_parser().register_symbol(variableNameID, SymbolType::GLOBAL_VARIABLE, decl->name);

  bool is_array = decl->size != nullptr;
  if (is_array)
  {
    bool is_valid_syntax = !decl->expr && (t == SHVT_COLOR4 || t == SHVT_INT4);
    if (!is_valid_syntax)
    {
      error("Wrong array assignment", decl->name, parser);
      return;
    }
  }
  else
  {
    bool is_valid_syntax = !decl->arr0;
    if (!is_valid_syntax)
    {
      error("Wrong variable syntax", decl->name, parser);
      return;
    }
  }

  Tab<ShaderGlobal::Var> &variable_list = ShaderGlobal::getMutableVariableList();
  int size = is_array ? semutils::int_number(decl->size->text) : 1;

  if (is_array)
  {
    eastl::string array_size_getter_name(eastl::string::CtorSprintf{}, ARRAY_SIZE_GETTER_NAME, decl->name->text);

    v = append_items(variable_list, 1);
    variable_list[v].type = SHVT_INT;
    variable_list[v].nameId = VarMap::addVarId(array_size_getter_name.c_str());
    variable_list[v].isAlwaysReferenced = true;
    variable_list[v].value.i = size;
  }

  for (int i = 0; i < size; i++)
  {
    if (i > 0)
    {
      eastl::string name(eastl::string::CtorSprintf{}, "%s[%i]", decl->name->text, i);
      variableNameID = VarMap::addVarId(name.c_str());
    }

    v = append_items(variable_list, 1);

    variable_list[v].type = t;
    variable_list[v].nameId = variableNameID;
    variable_list[v].isAlwaysReferenced = decl->is_always_referenced ? true : false;
    variable_list[v].shouldIgnoreValue = decl->undefined ? true : false;
    variable_list[v].fileName = parser.get_lex_parser().get_filename(decl->name->file_start);
    variable_list[v].array_size = size;
    variable_list[v].index = i;

    Color4 val(0, 0, 0, 1);
    ShaderTerminal::arithmetic_expr *expr = nullptr;
    if (i == 0)
      expr = is_array ? decl->arr0 : decl->expr;
    else if (i - 1 < decl->arrN.size())
      expr = decl->arrN[i - 1];

    if (expr)
    {
      ExpressionParser::getStatic().setParser(&parser);
      if (!ExpressionParser::getStatic().parseConstExpression(*expr, val))
      {
        error("Wrong expression", decl->name, parser);
        return;
      }
    }
    switch (t)
    {
      case SHVT_COLOR4: variable_list[v].value.c4 = val; break;
      case SHVT_REAL: variable_list[v].value.r = val[0]; break;
      case SHVT_INT: variable_list[v].value.i = real2int(val[0]); break;
      case SHVT_INT4:
        variable_list[v].value.i4 = IPoint4(real2int(val[0]), real2int(val[1]), real2int(val[2]), real2int(val[3]));
        break;
      case SHVT_FLOAT4X4:
        // default value is not supported
        break;
      case SHVT_BUFFER:
        variable_list[v].value.buffer.id = BAD_D3DRESID;
        variable_list[v].value.buffer.data = NULL;
        break;
      case SHVT_TEXTURE:
        variable_list[v].value.texture.id = BAD_TEXTUREID;
        variable_list[v].value.texture.data = NULL;
        break;
      default: G_ASSERT(0);
    }
  }
}

void add_interval(IntervalList &intervals, interval &interv, ShaderVariant::VarType type, ShaderSyntaxParser &parser)
{
  SymbolType symbol_type = type == ShaderVariant::VARTYPE_GLOBAL_INTERVAL ? SymbolType::INTERVAL : SymbolType::LOCAL_INTERVAL;
  if (intervals.intervalExists(interv.name->text))
  {
    int intervalId = IntervalValue::getIntervalNameId(interv.name->text);
    eastl::string message(eastl::string::CtorSprintf{}, "interval '%s' already declared in ", interv.name->text);
    message += parser.get_lex_parser().get_symbol_location(intervalId, symbol_type);
    warning(message.c_str(), interv.name, parser);
  }

  eastl::string file_name = parser.get_lex_parser().get_filename(interv.name->file_start);

  Interval newInterval(interv.name->text, type, interv.is_optional ? true : false, eastl::move(file_name));
  parser.get_lex_parser().register_symbol(newInterval.getNameId(), symbol_type, interv.name);

  ExpressionParser &exprParser = ExpressionParser::getStatic();
  exprParser.setParser(&parser);

  for (int i = 0; i < interv.var_decl.size(); i++)
  {
    interval_var *var = interv.var_decl[i];

    real v = semutils::real_number(var->val);

    if (!newInterval.addValue(var->name->text, v))
    {
      error("Duplicate value found in interval!", interv.name, parser);
    }
  }

  if (!newInterval.addValue(interv.last_var_name->text, IntervalValue::VALUE_INFINITY))
  {
    error("Duplicate value found in interval!", interv.name, parser);
  }

  if (!intervals.addInterval(newInterval))
  {
    error("Interval already exists and their type diffirent from new interval!", interv.name, parser);
  }
}

// add a new global variable interval
void add_global_interval(ShaderTerminal::interval &interv, ShaderTerminal::ShaderSyntaxParser &parser)
{
  IntervalList &intervals = ShaderGlobal::getMutableIntervalList();
  add_interval(intervals, interv, ShaderVariant::VARTYPE_GLOBAL_INTERVAL, parser);
}

static DataBlock blk_storage_for_assumed_vars;
void add_assume_to_blk(const char *block_name, const IntervalList &intervals, assume_stat &s, ShaderSyntaxParser &parser)
{
  DataBlock *blk = shc::getAssumedVarsBlock();
  if (!blk)
    shc::setAssumedVarsBlock(blk = &blk_storage_for_assumed_vars);

  if (block_name)
    blk = blk->addBlock(block_name);

  int interval_nid = IntervalValue::getIntervalNameId(s.interval->text);
  int value_nid = IntervalValue::getIntervalNameId(s.value->text);
  if (interval_nid < 0)
  {
    error(String(0, "Undeclared interval '%s' (nameId=%d) is used in 'assume'", s.interval->text, interval_nid), s.interval, parser);
    return;
  }
  if (value_nid < 0)
  {
    error(String(0, "Bad value '%s' (nameId=%d) for interval '%s' is used in 'assume'", s.value->text, value_nid, s.interval->text),
      s.value, parser);
    return;
  }

  ShaderVariant::ExtType intervalIndex = intervals.getIntervalIndex(interval_nid);
  const Interval *interv = nullptr;
  if (intervalIndex != INTERVAL_NOT_INIT)
    interv = intervals.getInterval(intervalIndex);
  else
  {
    intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(interval_nid);
    interv = ShaderGlobal::getIntervalList().getInterval(intervalIndex);
  }
  if (!interv)
  {
    error(String(0, "Undeclared interval '%s' (nameId=%d) is used in 'assume'", s.interval->text, interval_nid), s.interval, parser);
    return;
  }

  const IntervalValue *interv_val = interv->getValue(value_nid);
  if (!interv_val)
  {
    error(String(0, "Bad value '%s' (nameId=%d) for interval '%s' is used in 'assume'", s.value->text, value_nid, s.interval->text),
      s.value, parser);
    return;
  }

  float val = (interv_val->getBounds().getMin() + interv_val->getBounds().getMax()) * 0.5;
  int p_idx = blk->findParam(s.interval->text);
  if (p_idx >= 0)
  {
    if (blk->getParamType(p_idx) != DataBlock::TYPE_REAL)
    {
      error(String(0, "cannot override %s{ %s:%s= } in *_assume_static with 'assume' statement (:r= is required!)",
              block_name ? block_name : "", s.interval->text, dblk::resolve_short_type(blk->getParamType(p_idx))),
        s.interval, parser);
      return;
    }
    if (blk->getReal(p_idx) != val)
      warning(String(0, "Overriding %s (for %s) old=%g -> new=%g in 'assume' statement", s.interval->text,
                block_name ? block_name : "", blk->getReal(p_idx), val),
        s.interval, parser);
  }
  blk->setReal(s.interval->text, val);
  debug("%s=%s -> %s %s=%g", s.interval->text, s.value->text, blk->getBlockName(), s.interval->text, val);
}

void add_global_assume(ShaderTerminal::assume_stat &assume, ShaderTerminal::ShaderSyntaxParser &parser)
{
  add_assume_to_blk(nullptr, {}, assume, parser);
}

} // namespace ShaderParser
