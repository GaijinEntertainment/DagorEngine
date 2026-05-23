// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "intervals.h"
#include "shLog.h"
#include "shsyn.h"

#include <generic/dag_span.h>
#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_enumerate.h>

#include <EASTL/optional.h>
#include <EASTL/array.h>

// @TODO(boolvar_opt):
// Further optimization opportunities:
// - SCJ folding pass -- if a short-circuiting evaluation leads to multiple scj-s, coalesce into one jump to the end of the chain
// - Boolvar inlining pass -- if a bool var is statically resolvable at compilation time, inline it's code.
// - Priority reordering pass -- shader checks are better done first, global boolvar as well, to exit earlier.
// - Per-shader caching of const results (broken by assumes atm) -- cache gatherVar results per-shader.

namespace shc
{
class TargetContext;
}

enum BoolExprEvalInstructionType : uint8_t
{
  BE_IT_PLACEHOLDER = 0,

  BE_IT_INTERVAL,
  BE_IT_TEXINTERVAL,
  BE_IT_BOOL_VAR,
  BE_IT_BOOL_VAR_MAYBE,
  BE_IT_TWOSIDED,
  BE_IT_REALTWOSIDED,
  BE_IT_SHADER,

  BE_IT_COUNT
};

inline constexpr eastl::array<const char *, BE_IT_COUNT> BE_IT_NAMES = {
  "PLACEHOLDER", "INTERVAL", "TEXINTERVAL", "BOOL_VAR", "BOOL_VAR_MAYBE", "TWOSIDED", "REALTWOSIDED", "SHADER"};

enum BoolExprEvalLinkType : uint8_t
{
  BE_ILT_PLACEHOLDER = 0,

  BE_ILT_AND,
  BE_ILT_OR,
  BE_ILT_EOE, // end of expression

  BE_ILT_COUNT
};

inline constexpr eastl::array<const char *, BE_ILT_COUNT> BE_ILT_NAMES = {"PLACEHOLDER", "AND", "OR", "EOE"};

enum ShortCircuitJumpLabel : uint16_t
{
  BE_SCJ_PLACEHOLDER = 0,
  BE_SCJ_EOE, // end of expression

  BE_SCJ_ENUMERATED_BASE // subtract from value
};

inline constexpr eastl::array<const char *, BE_SCJ_ENUMERATED_BASE> SCJ_NAMES = {"VACANT", "EOE"};

struct alignas(uint64_t) BoolExprEvalInstruction // aligned for type erasure to uint64_t
{
  BoolExprEvalInstructionType opcode = BE_IT_PLACEHOLDER;
  BoolExprEvalLinkType link : 4 = BE_ILT_PLACEHOLDER;
  Interval::BooleanExpr comp : 4 = Interval::EXPR_NOTINIT;
  uint16_t valueIsNegated : 1 = false;
  ShortCircuitJumpLabel shortCircuitJump : 15 = BE_SCJ_PLACEHOLDER;
  int16_t argumentId = -1;
  int16_t valueId = -1;
  // Place limit of 32k on interval/bool var/etc count, seems reasonable
};

inline constexpr uint32_t MAX_BE_SCJ_VALUE = (1 << 15) - 1;
inline constexpr int32_t MAX_BE_ARGID_VALUE = eastl::numeric_limits<int16_t>::max();
inline constexpr int32_t MAX_BE_VALID_VALUE = eastl::numeric_limits<int16_t>::max();

static_assert(sizeof(BoolExprEvalInstruction) == sizeof(uint64_t) && alignof(BoolExprEvalInstruction) == alignof(uint64_t));

struct BoolExprEvalProgram
{
  dag::ConstSpan<BoolExprEvalInstruction> code{};
  eastl::optional<bool> constvalue{};
};

inline constexpr const char *IBE_NAMES[Interval::EXPR_COUNT] = {
  "NOTINIT", "EQ", "GREATER", "GREATER_EQ", "SMALLER", "SMALLER_EQ", "NOT_EQ"};

inline void dump_compiled_bool_expr_bytecode(dag::ConstSpan<BoolExprEvalInstruction> code, const char *tag, ShLogMode shlog_level)
{
  eastl::string dump;
  dump.append_sprintf("=== begin dump (%s) ===\n", tag);
  for (const auto [i, instr] : enumerate(code))
  {
    dump.append_sprintf("[%d]: %s link<%s> comp<%s>%s", i, BE_IT_NAMES[instr.opcode], BE_ILT_NAMES[instr.link], IBE_NAMES[instr.comp],
      instr.valueIsNegated ? " !val" : "");
    if (instr.shortCircuitJump >= BE_SCJ_ENUMERATED_BASE)
    {
      // An additional +1 for readability of the dump --
      // actual labels is calculated inside () and jumps into the middle of instr to use it's link.
      // The initial +1 is done so that we don't frame around '++ip' in vm code (code does ip += shortCircuitJump and then ++ip).
      // With this additional +1 we display the index of the actual next fully evaluated instruction.
      int tgt = (i + 1 + instr.shortCircuitJump - BE_SCJ_ENUMERATED_BASE) + 1;
      if (tgt == code.size())
        dump.append(" scj->EOE");
      else
        dump.append_sprintf(" scj->[%d]", tgt);
    }
    else if (instr.shortCircuitJump == BE_SCJ_EOE)
      dump.append(" scj->EOE");
    if (instr.argumentId != -1)
      dump.append_sprintf(" argid=%d", instr.argumentId);
    if (instr.valueId != -1)
      dump.append_sprintf(" valid=%d", instr.valueId);
    dump.append("\n");
  }
  dump.append("=== end dump ===");
  sh_debug(shlog_level, "%s", dump.c_str());
}

struct BoolExprEvalCbResult
{
  bool res = false;
  bool erred = false;
};

struct BoolExprFetchCbResult
{
  BoolExprEvalProgram program = {};
  bool erred = false;
};

inline eastl::optional<bool> evaluate_compiled_bool_expr(const BoolExprEvalProgram &program, //
  auto &&eval_interval,                                                                      //
  auto &&eval_texinterval,                                                                   //
  auto &&fetch_boolvar_prog,                                                                 //
  auto &&fetch_boolvar_prog_maybe,                                                           //
  auto &&eval_twosided,                                                                      //
  auto &&eval_realtwosided,                                                                  //
  auto &&eval_shader)
{
  if (program.constvalue)
    return program.constvalue.value();
  G_ASSERT(!program.code.empty());

  struct StackFrame
  {
    dag::ConstSpan<BoolExprEvalInstruction> code{};
    uint32_t ip = 0;
    bool lastEvalResult = false;
    bool jump = false;
  };
  dag::RelocatableFixedVector<StackFrame, 16> stack{};

  BoolExprEvalProgram fetchedProgram{};
  bool err = false;

  stack.emplace_back(program.code, 0);

  auto frame = stack.back();

  for (;;)
  {
    auto instr = frame.code[frame.ip];

    // Because c++ is too dumb to do CTAD over assignment, I can't use tie to assign from structs.
#define CALL_EVAL_CB(cb_, ...)          \
  do                                    \
  {                                     \
    auto [r_, e_] = (cb_)(__VA_ARGS__); \
    frame.lastEvalResult = r_;          \
    err = e_;                           \
  } while (0)
#define CALL_FETCH_CB(cb_, ...)         \
  do                                    \
  {                                     \
    auto [p_, e_] = (cb_)(__VA_ARGS__); \
    fetchedProgram = p_;                \
    err = e_;                           \
  } while (0)

    if (!frame.jump)
    {
      switch (instr.opcode)
      {
        case BE_IT_INTERVAL:
        {
          CALL_EVAL_CB(eval_interval, instr.argumentId, instr.valueId, instr.comp);
          break;
        }

        case BE_IT_TEXINTERVAL:
        {
          CALL_EVAL_CB(eval_texinterval, instr.argumentId, instr.comp);
          break;
        }

        case BE_IT_BOOL_VAR:
        {
          CALL_FETCH_CB(fetch_boolvar_prog, instr.argumentId);
          if (fetchedProgram.constvalue)
          {
            frame.lastEvalResult = fetchedProgram.constvalue.value();
            break;
          }
          else
          {
            // Need to process error here, because we need to go back to loop start
            if (DAGOR_UNLIKELY(err))
              return eastl::nullopt;

            stack.back() = frame;
            frame = stack.emplace_back(fetchedProgram.code, 0, false, false);

            // Back to loop start -- start processing pushed frame
            continue;
          }
        }

        case BE_IT_BOOL_VAR_MAYBE:
        {
          CALL_FETCH_CB(fetch_boolvar_prog_maybe, instr.argumentId);
          if (fetchedProgram.constvalue)
          {
            frame.lastEvalResult = fetchedProgram.constvalue.value();
            break;
          }
          else
          {
            // Need to process error here, because we need to go back to loop start
            if (DAGOR_UNLIKELY(err))
              return eastl::nullopt;

            stack.back() = frame;
            frame = stack.emplace_back(fetchedProgram.code, 0, false, false);

            // Back to loop start -- start processing pushed frame
            continue;
          }
        }

        case BE_IT_TWOSIDED:
        {
          CALL_EVAL_CB(eval_twosided);
          break;
        }

        case BE_IT_REALTWOSIDED:
        {
          CALL_EVAL_CB(eval_realtwosided);
          break;
        }

        case BE_IT_SHADER:
        {
          CALL_EVAL_CB(eval_shader, instr.argumentId, instr.comp);
          break;
        }

        default:
        {
          G_ASSERTF_RETURN(0, false, "Invalid instruction opc=%x at ip=%d in bool stream", uint32_t(instr.opcode), frame.ip);
          break;
        }
      }

      if (DAGOR_UNLIKELY(err))
        return eastl::nullopt;

      if (instr.valueIsNegated)
        frame.lastEvalResult = !frame.lastEvalResult;
    }

#undef CALL_EVAL_CB
#undef CALL_FETCH_CB

    switch (instr.link)
    {
      case BE_ILT_AND:
      {
        frame.jump = !frame.lastEvalResult;
        break;
      }

      case BE_ILT_OR:
      {
        frame.jump = frame.lastEvalResult;
        break;
      }

      case BE_ILT_EOE:
      {
        frame.jump = true;
        instr.shortCircuitJump = BE_SCJ_EOE;
        break;
      }

      default:
      {
        G_ASSERTF_RETURN(0, false, "Invalid instruction link opc=%x at ip=%d in bool stream", uint32_t(instr.opcode), frame.ip);
        break;
      }
    }

    if (frame.jump)
    {
      switch (instr.shortCircuitJump)
      {
        case BE_SCJ_EOE:
        {
          frame.ip = frame.code.size() - 1;
          break;
        }

        case BE_SCJ_PLACEHOLDER:
        {
          G_ASSERTF_RETURN(0, false, "Invalid instruction short circuit jump opc=%x at ip=%d in bool stream", uint32_t(instr.opcode),
            frame.ip);
          break;
        }

        default:
        {
          frame.ip += instr.shortCircuitJump - BE_SCJ_ENUMERATED_BASE; // Not mutually exclusive with ++ip -- min jump of 0 is
                                                                       // to-next-instr
          break;
        }
      }
    }

    ++frame.ip;

    if (frame.ip >= frame.code.size())
    {
      if (stack.size() == 1)
      {
        break;
      }
      else
      {
        stack.pop_back();
        stack.back().lastEvalResult = frame.lastEvalResult;
        frame = stack.back();

        // Having calculated the bool var res, apply negation from parent frame stop point
        if (frame.code[frame.ip].valueIsNegated)
          frame.lastEvalResult = !frame.lastEvalResult;
        // And force jump without advancing ip -- we still need to use the link from the stop point
        frame.jump = true;
      }
    }
  }

  return frame.lastEvalResult;
}

using bool_expr_compiler_error_cb_t = void (*)(const char *);
inline const bool_expr_compiler_error_cb_t default_bool_expr_compiler_error_cb =
  +[](const char *msg) { sh_debug(SHLOG_ERROR, "%s", msg); };

bool compile_bool_expr_to_bytecode(                     //
  eastl::vector<BoolExprEvalInstruction> &out_bytecode, //
  eastl::optional<bool> &out_constvalue,                //
  ShaderTerminal::bool_expr &expr,                      //
  shc::TargetContext &ctx,                              //
  bool_expr_compiler_error_cb_t error_cb = default_bool_expr_compiler_error_cb);

// Type erased for usage with generated code

inline void dump_compiled_bool_expr_bytecode(dag::ConstSpan<uint64_t> code, const char *tag, ShLogMode shlog_level)
{
  static_assert(sizeof(uint64_t) == sizeof(BoolExprEvalInstruction));
  dump_compiled_bool_expr_bytecode(reinterpret_cast<dag::ConstSpan<BoolExprEvalInstruction> &>(code), tag, shlog_level);
}
inline bool compile_bool_expr_to_bytecode( //
  eastl::vector<uint64_t> &out_bytecode,   //
  eastl::optional<bool> &out_constvalue,   //
  ShaderTerminal::bool_expr &expr,         //
  shc::TargetContext &ctx,                 //
  bool_expr_compiler_error_cb_t error_cb = default_bool_expr_compiler_error_cb)
{
  static_assert(sizeof(uint64_t) == sizeof(BoolExprEvalInstruction));
  return compile_bool_expr_to_bytecode(reinterpret_cast<eastl::vector<BoolExprEvalInstruction> &>(out_bytecode), out_constvalue, expr,
    ctx, error_cb);
}

inline bool compile_bool_expr_cached(ShaderTerminal::bool_expr &node, shc::TargetContext &ctx)
{
  if (node.compiled)
    return true;
  if (!compile_bool_expr_to_bytecode(node.bytecode, node.constvalue, node, ctx))
    return false;
  node.compiled = true;
  return true;
}

inline BoolExprEvalProgram bytecode_from_expr(ShaderTerminal::bool_expr &node)
{
  G_ASSERT(node.compiled);
  return BoolExprEvalProgram{
    dag::ConstSpan<BoolExprEvalInstruction>{(const BoolExprEvalInstruction *)node.bytecode.data(), intptr_t(node.bytecode.size())},
    node.constvalue};
}
