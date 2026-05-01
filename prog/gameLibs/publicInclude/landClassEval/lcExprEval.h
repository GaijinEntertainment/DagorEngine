//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_check_nan.h>
#include <math.h>

// Landclass expression evaluator -- base types and eval.
// This is all that's needed to evaluate a compiled expression.
//
// IMPORTANT: eval/evalSafe do NOT bounds-check variable reads.
// Caller must verify numVars > computeMaxVarId(ir, root) once per expression
// (see lcExprParser.h) before entering the per-pixel eval loop.
//
// vars is a mutable buffer because user-declared variables (var/let/=) write back into
// the same array. External (caller-registered) vars and user vars share the storage,
// distinguished only by varId range. Caller is responsible for filling external slots
// before each eval and for zero-initialising user slots if the expression assumes them
// undeclared on first read.
namespace lcexpr
{

typedef Tab<uint8_t> Arena;

struct EvalNode;

struct EvalCtx
{
  const uint8_t *arena;
  float *vars;
  int numVars;

  const EvalNode *at(uint32_t ofs) const { return reinterpret_cast<const EvalNode *>(arena + ofs); }
};

struct EvalNode
{
  virtual float eval(const EvalCtx &ctx) const = 0;
};

// Sentinel roots: encode a variable index directly, no arena node needed.
static constexpr uint32_t DIRECT_VAR_BASE = 0xFFFF0000u;
inline constexpr uint32_t DIRECT_VAR(uint16_t varId) { return DIRECT_VAR_BASE + varId; }

inline float eval(const Arena &arena, uint32_t root, float *vars, int numVars)
{
  if (root >= DIRECT_VAR_BASE)
    return vars[root - DIRECT_VAR_BASE];
  EvalCtx ctx = {arena.data(), vars, numVars};
  return ctx.at(root)->eval(ctx);
}

// Safe eval: clamp to [0,1], NaN/inf -> 0. Use when the caller wants a weight in [0,1].
inline float evalSafe(const Arena &arena, uint32_t root, float *vars, int numVars)
{
  float v = eval(arena, root, vars, numVars);
  if (!check_finite(v) || v < 0.f)
    return 0.f;
  return v > 1.f ? 1.f : v;
}

// Finite eval: NaN/inf -> 0, negatives -> 0, but no upper clamp. Use when the caller
// needs the raw magnitude of the result (for instance sum-mode weights that legitimately
// exceed 1.0, or an "importance" channel that scales directly with the returned value).
// The caller is responsible for clamping at the write site if it is feeding into a
// [0,1]-expected consumer.
inline float evalFinite(const Arena &arena, uint32_t root, float *vars, int numVars)
{
  float v = eval(arena, root, vars, numVars);
  if (!check_finite(v) || v < 0.f)
    return 0.f;
  return v;
}

} // namespace lcexpr
