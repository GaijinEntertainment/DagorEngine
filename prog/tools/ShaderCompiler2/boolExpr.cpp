// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "boolExpr.h"
#include "shTargetContext.h"
#include "hwSemantic.h"

#include <generic/dag_relocatableFixedVector.h>

// @TODO(boolvar_opt): non-recursive if compilation is hot enough

// Utility for error reporting with varargs. Don't mind the allocation -- error path is the cold path.
template <class... TArgs>
static void report_compilation_error(bool_expr_compiler_error_cb_t cb, const char *fmt, TArgs &&...args)
{
  auto m = string_f(fmt, eastl::forward<TArgs>(args)...);
  m = "Bool compilation error: " + m;
  cb(m.c_str());
}

static bool flip(bool v, bool negated) { return negated ? !v : v; }

static bool compile_bool_expr_to_bytecode_impl(         //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::bool_expr &expr,                      //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb,               //
  bool negate_expr);

static bool compile_bool_expr_not_to_bytecode_impl(     //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::not_expr &expr,                       //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb,               //
  bool negate_expr)
{
  negate_expr = flip(expr.is_not != nullptr, negate_expr);

  if (expr.value->expr)
  {
    return compile_bool_expr_to_bytecode_impl(out_bytecode, out_constvalue, *expr.value->expr, ctx, error_cb, negate_expr);
  }
  else
  {
    auto &val = *expr.value;

    if (val.hw)
    {
      out_constvalue.emplace(flip(semantic::compare_hw_token(val.hw->var->num, ctx.compCtx()), negate_expr));
      return true;
    }
    else if (val.true_value)
    {
      out_constvalue.emplace(flip(true, negate_expr));
      return true;
    }
    else if (val.false_value)
    {
      out_constvalue.emplace(flip(false, negate_expr));
      return true;
    }

    auto &instr = out_bytecode.emplace_back();
    instr.link = BE_ILT_EOE;
    instr.valueIsNegated = negate_expr;

#define SET_ARG_ID(id_)                                                                                               \
  do                                                                                                                  \
  {                                                                                                                   \
    G_ASSERT((id_) >= 0);                                                                                             \
    if ((id_) > MAX_BE_ARGID_VALUE)                                                                                   \
    {                                                                                                                 \
      report_compilation_error(error_cb, "argument id %d > %d too large for max bitness", (id_), MAX_BE_ARGID_VALUE); \
      return false;                                                                                                   \
    }                                                                                                                 \
    instr.argumentId = int16_t(id_);                                                                                  \
  } while (0)
#define SET_VAL_ID(id_)                                                                                            \
  do                                                                                                               \
  {                                                                                                                \
    G_ASSERT((id_) >= 0);                                                                                          \
    if ((id_) > MAX_BE_VALID_VALUE)                                                                                \
    {                                                                                                              \
      report_compilation_error(error_cb, "value id %d > %d too large for max bitness", (id_), MAX_BE_VALID_VALUE); \
      return false;                                                                                                \
    }                                                                                                              \
    instr.valueId = int16_t(id_);                                                                                  \
  } while (0)

    if (val.shader)
    {
      instr.opcode = BE_IT_SHADER;
      SET_ARG_ID(ctx.shaderNameMap().addNameId(val.shader->text));
    }
    else if (val.two_sided)
    {
      instr.opcode = BE_IT_TWOSIDED;
    }
    else if (val.real_two_sided)
    {
      instr.opcode = BE_IT_REALTWOSIDED;
    }
    else if (val.interval_ident)
    {
      instr.opcode = BE_IT_INTERVAL;
      SET_ARG_ID(ctx.intervalNameMap().addNameId(val.interval_ident->text));
      SET_VAL_ID(ctx.intervalNameMap().addNameId(val.interval_value->text));
    }
    else if (val.texture_name)
    {
      instr.opcode = BE_IT_TEXINTERVAL;
      SET_ARG_ID(ctx.intervalNameMap().addNameId(val.texture_name->text));
    }
    else if (val.bool_var)
    {
      instr.opcode = BE_IT_BOOL_VAR;
      SET_ARG_ID(ctx.boolVarNameMap().addVarId(val.bool_var->text));
    }
    else if (val.maybe)
    {
      instr.opcode = BE_IT_BOOL_VAR_MAYBE;
      SET_ARG_ID(ctx.boolVarNameMap().addVarId(val.maybe_bool_var->text));
    }
    else
    {
      G_ASSERTF(0, "ICE: Unhandled bool_value node variation: syntax has been extended and the new value shoulb be handled");
    }

    if (val.cmpop)
    {
      switch (val.cmpop->op->num)
      {
        case SHADER_TOKENS::SHTOK_eq:
        case SHADER_TOKENS::SHTOK_assign: instr.comp = Interval::EXPR_EQ; break;
        case SHADER_TOKENS::SHTOK_noteq: instr.comp = Interval::EXPR_NOT_EQ; break;
        case SHADER_TOKENS::SHTOK_greater: instr.comp = Interval::EXPR_GREATER; break;
        case SHADER_TOKENS::SHTOK_greatereq: instr.comp = Interval::EXPR_GREATER_EQ; break;
        case SHADER_TOKENS::SHTOK_smaller: instr.comp = Interval::EXPR_SMALLER; break;
        case SHADER_TOKENS::SHTOK_smallereq: instr.comp = Interval::EXPR_SMALLER_EQ; break;
        default: G_ASSERT(0);
      }

      if (DAGOR_UNLIKELY(!item_is_in(instr.opcode, {BE_IT_SHADER, BE_IT_INTERVAL, BE_IT_TEXINTERVAL})))
      {
        report_compilation_error(error_cb, "can't use comparison op for expression of type %s", BE_IT_NAMES[instr.opcode]);
        return false;
      }
      if (DAGOR_UNLIKELY(instr.opcode != BE_IT_INTERVAL && !item_is_in(instr.comp, {Interval::EXPR_EQ, Interval::EXPR_NOT_EQ})))
      {
        report_compilation_error(error_cb, "can't use comparison op %s for expression of type %s", IBE_NAMES[instr.comp],
          BE_IT_NAMES[instr.opcode]);
        return false;
      }
    }

    return true;
  }
#undef SET_ARG_ID
#undef SET_VAL_ID
}

static bool compile_bool_expr_and_to_bytecode_impl(     //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::and_expr &expr,                       //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb,               //
  bool negate_expr)
{
  if (expr.value)
    return compile_bool_expr_not_to_bytecode_impl(out_bytecode, out_constvalue, *expr.value, ctx, error_cb, negate_expr);

  eastl::optional<bool> constvalueA{};
  uint32_t startA = out_bytecode.size();
  if (!compile_bool_expr_and_to_bytecode_impl(out_bytecode, constvalueA, *expr.a, ctx, error_cb, negate_expr))
    return false;
  if (constvalueA)
  {
    out_bytecode.resize(startA); // clear out redundant bytecode
    if (negate_expr == constvalueA.value())
    {
      out_constvalue = constvalueA;
      return true;
    }
    else
    {
      return compile_bool_expr_not_to_bytecode_impl(out_bytecode, out_constvalue, *expr.b, ctx, error_cb, negate_expr);
    }
  }

  uint32_t lastAInstrId = out_bytecode.size() - 1;
  out_bytecode[lastAInstrId].link = negate_expr ? BE_ILT_OR : BE_ILT_AND;
  out_bytecode[lastAInstrId].shortCircuitJump = BE_SCJ_PLACEHOLDER;
  eastl::optional<bool> constvalueB{};
  uint32_t startB = out_bytecode.size();
  if (!compile_bool_expr_not_to_bytecode_impl(out_bytecode, constvalueB, *expr.b, ctx, error_cb, negate_expr))
    return false;
  if (constvalueB)
  {
    out_bytecode.resize(startB); // clear out redundant bytecode
    if (negate_expr == constvalueB.value())
    {
      out_bytecode.resize(startA); // A is also not needed
      out_constvalue = constvalueB;
    }
    else
    {
      // otherwise, the value of A is what we use
      out_bytecode[lastAInstrId].link = BE_ILT_EOE;
      out_constvalue = {};
    }
    return true;
  }

  uint32_t lastBInstrId = out_bytecode.size() - 1;
  uint32_t scj = lastBInstrId - lastAInstrId + BE_SCJ_ENUMERATED_BASE - 1;
  if (DAGOR_UNLIKELY(scj > MAX_BE_SCJ_VALUE))
  {
    report_compilation_error(error_cb, "bool expr bytecode label %u exceeds %u (max for bitwidth)", scj, MAX_BE_SCJ_VALUE);
    return false;
  }
  out_bytecode[lastAInstrId].shortCircuitJump = ShortCircuitJumpLabel(scj);
  out_bytecode[lastBInstrId].shortCircuitJump = BE_SCJ_EOE;
  return true;
}

static bool compile_bool_expr_to_bytecode_impl(         //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::bool_expr &expr,                      //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb,               //
  bool negate_expr)
{
  if (expr.value)
    return compile_bool_expr_and_to_bytecode_impl(out_bytecode, out_constvalue, *expr.value, ctx, error_cb, negate_expr);

  eastl::optional<bool> constvalueA{};
  uint32_t startA = out_bytecode.size();
  if (!compile_bool_expr_to_bytecode_impl(out_bytecode, constvalueA, *expr.a, ctx, error_cb, negate_expr))
    return false;
  if (constvalueA)
  {
    out_bytecode.resize(startA); // clear out redundant bytecode
    if (negate_expr != constvalueA.value())
    {
      out_constvalue = constvalueA;
      return true;
    }
    else
    {
      return compile_bool_expr_and_to_bytecode_impl(out_bytecode, out_constvalue, *expr.b, ctx, error_cb, negate_expr);
    }
  }

  uint32_t lastAInstrId = out_bytecode.size() - 1;
  out_bytecode[lastAInstrId].link = negate_expr ? BE_ILT_AND : BE_ILT_OR;
  out_bytecode[lastAInstrId].shortCircuitJump = BE_SCJ_PLACEHOLDER;
  eastl::optional<bool> constvalueB{};
  uint32_t startB = out_bytecode.size();
  if (!compile_bool_expr_and_to_bytecode_impl(out_bytecode, constvalueB, *expr.b, ctx, error_cb, negate_expr))
    return false;
  if (constvalueB)
  {
    out_bytecode.resize(startB); // clear out redundant bytecode
    if (negate_expr != constvalueB.value())
    {
      out_bytecode.resize(startA); // A is also not needed
      out_constvalue = constvalueB;
    }
    else
    {
      // otherwise, the value of A is what we use
      out_bytecode[lastAInstrId].link = BE_ILT_EOE;
      out_constvalue = {};
    }
    return true;
  }

  uint32_t lastBInstrId = out_bytecode.size() - 1;
  uint32_t scj = lastBInstrId - lastAInstrId + BE_SCJ_ENUMERATED_BASE - 1;
  if (DAGOR_UNLIKELY(scj > MAX_BE_SCJ_VALUE))
  {
    report_compilation_error(error_cb, "bool expr bytecode label %u exceeds %u (max for bitwidth)", scj, MAX_BE_SCJ_VALUE);
    return false;
  }
  out_bytecode[lastAInstrId].shortCircuitJump = ShortCircuitJumpLabel(scj);
  out_bytecode[lastBInstrId].shortCircuitJump = BE_SCJ_EOE;
  return true;
}

bool compile_bool_expr_to_bytecode(                     //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::bool_expr &expr,                      //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb)
{
  out_bytecode.clear();
  out_constvalue.reset();
  return compile_bool_expr_to_bytecode_impl(out_bytecode, out_constvalue, expr, ctx, error_cb, false);
}
