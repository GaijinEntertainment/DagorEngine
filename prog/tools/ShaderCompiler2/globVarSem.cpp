// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globVarSem.h"
#include "shTargetContext.h"
#include "shsyn.h"
#include "shExprParser.h"
#include "shErrorReporting.h"
#include "semUtils.h"
#include "shCompiler.h"
#include "shCompContext.h"
#include "cppStcode.h"

#include "globVar.h"
#include "samplers.h"
#include <shaders/dag_shaderCommon.h>
#include <util/dag_bitwise_cast.h>
#include "varMap.h"
#include "intervals.h"

using namespace ShaderParser;
using namespace ShaderTerminal;

namespace ShaderParser
{
// add a new global variable to a global variable list
void add_global_var(global_var_decl *decl, Parser &parser, shc::TargetContext &ctx)
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
    case SHADER_TOKENS::SHTOK_tlas: t = SHVT_TLAS; break;
    case SHADER_TOKENS::SHTOK_const_buffer: t = SHVT_BUFFER; break;
    default: G_ASSERT(0);
  }
  int variableNameID = ctx.varNameMap().addVarId(decl->name->text);

  int v = ctx.globVars().getVarInternalIndex(variableNameID);
  if (v >= 0)
  {
    String message;

    String incStack = parser.get_lexer().build_current_include_stack();
    message.aprintf(0, "%s\nglobal variable '%s' already declared in %s", incStack.c_str(), decl->name->text,
      parser.get_lexer().get_symbol_location(variableNameID, SymbolType::GLOBAL_VARIABLE));

    report_error(parser, decl->name, message.c_str());
    return;
  }

  parser.get_lexer().register_symbol(variableNameID, SymbolType::GLOBAL_VARIABLE, decl->name);

  bool is_array = decl->size != nullptr;
  if (is_array)
  {
    bool is_valid_syntax = !decl->expr && (t == SHVT_COLOR4 || t == SHVT_INT4);
    if (!is_valid_syntax)
    {
      report_error(parser, decl->name, "Wrong array assignment");
      return;
    }
  }
  else
  {
    bool is_valid_syntax = !decl->arr0;
    if (!is_valid_syntax)
    {
      report_error(parser, decl->name, "Wrong variable syntax");
      return;
    }
  }

  Tab<ShaderGlobal::Var> &variable_list = ctx.globVars().getMutableVariableList();
  int size = is_array ? semutils::int_number(decl->size->text) : 1;

  if (is_array)
  {
    eastl::string array_size_getter_name(eastl::string::CtorSprintf{}, ARRAY_SIZE_GETTER_NAME, decl->name->text);

    v = append_items(variable_list, 1);
    variable_list[v].type = SHVT_INT;
    variable_list[v].nameId = ctx.varNameMap().addVarId(array_size_getter_name.c_str());
    variable_list[v].isAlwaysReferenced = true;
    variable_list[v].value.i = size;
  }

  for (int i = 0; i < size; i++)
  {
    if (i > 0)
    {
      eastl::string name(eastl::string::CtorSprintf{}, "%s[%i]", decl->name->text, i);
      variableNameID = ctx.varNameMap().addVarId(name.c_str());
    }

    v = append_items(variable_list, 1);

    variable_list[v].type = t;
    variable_list[v].nameId = variableNameID;
    variable_list[v].isAlwaysReferenced = decl->is_always_referenced ? true : false;
    variable_list[v].shouldIgnoreValue = decl->undefined ? true : false;
    variable_list[v].fileName = bindump::string(parser.get_lexer().get_filename(decl->name->file_start));
    variable_list[v].array_size = size;
    variable_list[v].index = i;

    const bool expectingInt = t == SHVT_INT || t == SHVT_INT4;
    Color4 val = expectingInt ? Color4{bitwise_cast<float>(0), bitwise_cast<float>(0), bitwise_cast<float>(0), bitwise_cast<float>(1)}
                              : Color4{0, 0, 0, 1};

    ShaderTerminal::arithmetic_expr *expr = nullptr;
    if (i == 0)
      expr = is_array ? decl->arr0 : decl->expr;
    else if (i - 1 < decl->arrN.size())
      expr = decl->arrN[i - 1];

    if (expr)
    {
      const ExpressionParser exprParser{parser};

      shexpr::ValueType expectedValType = shexpr::VT_UNDEFINED;
      if (t == SHVT_REAL || t == SHVT_INT)
        expectedValType = shexpr::VT_REAL;
      else if (t == SHVT_COLOR4 || t == SHVT_INT4)
        expectedValType = shexpr::VT_COLOR4;
      else if (t == SHVT_FLOAT4X4)
      {
        report_error(parser, decl->name, "float4x4 default value is not supported");
        return;
      }

      if (!exprParser.parseConstExpression(*expr, val, ExpressionParser::Context{expectedValType, expectingInt, decl->name}))
      {
        report_error(parser, decl->name, "Wrong expression");
        return;
      }
    }

    switch (t)
    {
      case SHVT_COLOR4: variable_list[v].value.c4.set(val); break;
      case SHVT_REAL: variable_list[v].value.r = val[0]; break;
      case SHVT_INT: variable_list[v].value.i = bitwise_cast<int>(val[0]); break;
      case SHVT_INT4:
        variable_list[v].value.i4.set(bitwise_cast<int>(val[0]), bitwise_cast<int>(val[1]), bitwise_cast<int>(val[2]),
          bitwise_cast<int>(val[3]));
        break;
      case SHVT_FLOAT4X4:
        // default value is not supported
        break;
      case SHVT_BUFFER:
        variable_list[v].value.bufId = unsigned(BAD_D3DRESID);
        variable_list[v].value.buf = NULL;
        break;
      case SHVT_TLAS: variable_list[v].value.tlas = NULL; break;
      case SHVT_TEXTURE:
        variable_list[v].value.texId = unsigned(BAD_TEXTUREID);
        variable_list[v].value.tex = NULL;
        break;
      default: G_ASSERT(0);
    }
  }

  ctx.cppStcode().globVars.setVar(ShaderVarType(t), decl->name->text);
}

void add_sampler(sampler_decl *decl, Parser &parser, shc::TargetContext &ctx) { ctx.samplers().add(*decl, parser); }

void add_interval(IntervalList &intervals, interval &interv, ShaderVariant::VarType type, Parser &parser, shc::TargetContext &ctx)
{
  SymbolType symbol_type = type == ShaderVariant::VARTYPE_GLOBAL_INTERVAL ? SymbolType::INTERVAL : SymbolType::LOCAL_INTERVAL;
  int intervalId = ctx.intervalNameMap().addNameId(interv.name->text);
  if (intervals.intervalExists(intervalId))
  {
    eastl::string message(eastl::string::CtorSprintf{}, "interval '%s' already declared in ", interv.name->text);
    message += parser.get_lexer().get_symbol_location(intervalId, symbol_type);
    report_warning(parser, *interv.name, message.c_str());
  }

  eastl::string file_name = parser.get_lexer().get_filename(interv.name->file_start);

  Interval newInterval(intervalId, type, eastl::move(file_name));
  parser.get_lexer().register_symbol(newInterval.getNameId(), symbol_type, interv.name);

  for (int i = 0; i < interv.var_decl.size(); i++)
  {
    interval_var *var = interv.var_decl[i];

    real v = semutils::real_number(var->val);

    int valueNameId = ctx.intervalNameMap().addNameId(var->name->text);
    if (!newInterval.addValue(valueNameId, v))
      report_error(parser, interv.name, "Duplicate value found in interval!");
  }

  int valueNameId = ctx.intervalNameMap().addNameId(interv.last_var_name->text);
  if (!newInterval.addValue(valueNameId, IntervalValue::VALUE_INFINITY))
    report_error(parser, interv.name, "Duplicate value found in interval!");

  if (!intervals.addInterval(newInterval))
    report_error(parser, interv.name, "Interval already exists and their type diffirent from new interval!");
}

// add a new global variable interval
void add_global_interval(ShaderTerminal::interval &interv, Parser &parser, shc::TargetContext &ctx)
{
  IntervalList &intervals = ctx.globVars().getMutableIntervalList();
  add_interval(intervals, interv, ShaderVariant::VARTYPE_GLOBAL_INTERVAL, parser, ctx);
}

static void add_assume_impl(ShaderAssumesTable &table, const IntervalList &intervals, auto &stat, Parser &parser,
  shc::TargetContext &ctx)
{
  using stat_t = eastl::remove_cvref_t<decltype(stat)>;
  static_assert(eastl::is_same_v<stat_t, assume_stat> || eastl::is_same_v<stat_t, assume_if_not_assumed_stat>);
  constexpr bool OVERRIDE = eastl::is_same_v<stat_t, assume_stat>;

  int interval_nid = ctx.intervalNameMap().getNameId(stat.interval->text);
  int value_nid = ctx.intervalNameMap().getNameId(stat.value->text);
  if (interval_nid < 0)
  {
    report_error(parser, stat.interval, "Undeclared interval '%s' (nameId=%d) is used in 'assume'", stat.interval->text, interval_nid);
    return;
  }
  if (value_nid < 0)
  {
    report_error(parser, stat.value, "Bad value '%s' (nameId=%d) for interval '%s' is used in 'assume'", stat.value->text, value_nid,
      stat.interval->text);
    return;
  }

  ShaderVariant::ExtType intervalIndex = intervals.getIntervalIndex(interval_nid);
  const Interval *interv = nullptr;
  if (intervalIndex != INTERVAL_NOT_INIT)
    interv = intervals.getInterval(intervalIndex);
  else
  {
    intervalIndex = ctx.globVars().getIntervalList().getIntervalIndex(interval_nid);
    interv = ctx.globVars().getIntervalList().getInterval(intervalIndex);
  }
  if (!interv)
  {
    report_error(parser, stat.interval, "Undeclared interval '%s' (nameId=%d) is used in 'assume'", stat.interval->text, interval_nid);
    return;
  }

  const IntervalValue *interv_val = interv->getValueByNameId(value_nid);
  if (!interv_val)
  {
    report_error(parser, stat.value, "Bad value '%s' (nameId=%d) for interval '%s' is used in 'assume'", stat.value->text, value_nid,
      stat.interval->text);
    return;
  }

  float newval = table.addIntervalAssume(stat.interval->text, *interv_val, !OVERRIDE, parser, stat.interval);
  debug("%s=%s -> %s %s=%g", stat.interval->text, stat.value->text, table.getDebugName(), stat.interval->text, newval);
}

void add_shader_assume(assume_stat &s, Parser &parser, shc::ShaderContext &ctx)
{
  add_assume_impl(ctx.assumes(), ctx.intervals(), s, parser, ctx.tgtCtx());
}

void add_shader_assume_if_not_assumed(ShaderTerminal::assume_if_not_assumed_stat &s, Parser &parser, shc::ShaderContext &ctx)
{
  add_assume_impl(ctx.assumes(), ctx.intervals(), s, parser, ctx.tgtCtx());
}

void add_global_assume(ShaderTerminal::assume_stat &assume, Parser &parser, shc::TargetContext &ctx)
{
  add_assume_impl(ctx.globAssumes(), {}, assume, parser, ctx);
}

void add_global_assume_if_not_assumed(ShaderTerminal::assume_if_not_assumed_stat &assume, Parser &parser, shc::TargetContext &ctx)
{
  add_assume_impl(ctx.globAssumes(), {}, assume, parser, ctx);
}

} // namespace ShaderParser
