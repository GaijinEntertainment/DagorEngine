//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <landClassEval/lcExprParserImpl.h>

// Landclass expression fusion engine.
// Pattern-matches IR trees and replaces with fused single-eval nodes.
// EvalNode is NOT modified -- fusion operates entirely on the IR.
namespace lcexpr
{

// ============================================================================
// Fusion rule: pattern tree + patch rules + node factory
// ============================================================================

struct PatternNode
{
  uint64_t tag; // 0 = wildcard
  int c[3];     // child pattern indices, -1 = none
};

struct PatchOp
{
  enum Kind : uint8_t
  {
    VARID,
    CONST_VAL
  };
  uint8_t srcPatIdx;
  Kind kind;
  uint16_t dstOffset; // byte offset in fused node
};

typedef void (*NodeInitFn)(void *);

struct FusionRule
{
  // Size 16 accommodates the korea rule (max(var, ramp*ramp*smooth_ramp), 16 nodes).
  // Patches[12] accommodates its 10 capture points.
  PatternNode pattern[16];
  uint8_t patternSize;
  PatchOp patches[12];
  uint8_t numPatches;
  uint32_t nodeSize, nodeAlign;
  NodeInitFn initFn;
};

// ============================================================================
// Pattern matching
// ============================================================================

inline bool matchIR(const Tab<IRNode> &ir, int irIdx, const PatternNode *pat, int patIdx, int *matchMap)
{
  if (irIdx < 0)
    return false;
  const IRNode &n = ir[irIdx];
  const PatternNode &p = pat[patIdx];
  if (p.tag != 0 && n.tag != p.tag)
    return false;
  matchMap[patIdx] = irIdx;
  for (int i = 0; i < 3; i++)
  {
    if (p.c[i] < 0)
      break;
    if (i >= n.nc)
      return false;
    if (!matchIR(ir, n.c[i], pat, p.c[i], matchMap))
      return false;
  }
  return true;
}

inline uint32_t tryFuseRule(const Tab<IRNode> &ir, int irRoot, Arena &arena, const FusionRule &rule)
{
  int matchMap[16] = {};
  if (!matchIR(ir, irRoot, rule.pattern, 0, matchMap))
    return PARSE_ERROR;
  size_t ofs = arena.size();
  size_t aligned = (ofs + rule.nodeAlign - 1) & ~(rule.nodeAlign - 1);
  if (size_t pad = aligned - ofs)
    arena.append_default(pad);
  uint8_t *p = arena.append_default(rule.nodeSize);
  rule.initFn(p);
  for (int i = 0; i < rule.numPatches; i++)
  {
    const PatchOp &op = rule.patches[i];
    G_ASSERT(op.srcPatIdx < rule.patternSize);
    const IRNode &src = ir[matchMap[op.srcPatIdx]];
    uint8_t *dst = p + op.dstOffset;
    if (op.kind == PatchOp::VARID)
      *(uint16_t *)dst = src.varId;
    else
      *(float *)dst = src.constVal;
  }
  return (uint32_t)aligned;
}

inline uint32_t tryFuse(const Tab<IRNode> &ir, int irRoot, Arena &arena, const FusionRule *rules, int numRules)
{
  for (int i = 0; i < numRules; i++)
  {
    uint32_t r = tryFuseRule(ir, irRoot, arena, rules[i]);
    if (r != PARSE_ERROR)
      return r;
  }
  return PARSE_ERROR;
}

// ============================================================================
// Fused node types
// ============================================================================

// Helper: inline ramp and smooth_ramp
inline float fRamp(float val, float from, float to)
{
  float dv = to - from;
  if (fabsf(dv) < 1e-10f)
    return val >= from ? 1.f : 0.f;
  float t = (val - from) / dv;
  return t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
}
inline float fSRamp(float val, float from, float to)
{
  float t = fRamp(val, from, to);
  return t * t * (3.f - 2.f * t);
}

// #6/#10: ramp(v,C,C) and smooth_ramp(v,C,C) are now RampVarConstNode/SmoothRampVarConstNode
// in lcExprNodes.h -- emitted automatically by the compiler, no fusion rule needed.

// #7: ramp(v,C,C) * smooth_ramp(v,C,C) -- terrain product, no mask
struct FusedRampMulSmoothRamp : EvalNode
{
  uint16_t htId, angId;
  float htFrom, htTo, angFrom, angTo;
  float eval(const EvalCtx &ctx) const override
  {
    return fRamp(ctx.vars[htId], htFrom, htTo) * fSRamp(ctx.vars[angId], angFrom, angTo);
  }
};

// #8: max(v, ramp(v,C,C))
struct FusedMaxVarRamp : EvalNode
{
  uint16_t maskId, varId;
  float from, to;
  float eval(const EvalCtx &ctx) const override
  {
    float r = fRamp(ctx.vars[varId], from, to);
    return ctx.vars[maskId] > r ? ctx.vars[maskId] : r;
  }
};

// #9: max(v, smooth_ramp(v,C,C))
struct FusedMaxVarSmoothRamp : EvalNode
{
  uint16_t maskId, varId;
  float from, to;
  float eval(const EvalCtx &ctx) const override
  {
    float r = fSRamp(ctx.vars[varId], from, to);
    return ctx.vars[maskId] > r ? ctx.vars[maskId] : r;
  }
};

// #3 (original): max(v, ramp(v,C,C) * smooth_ramp(v,C,C))
struct FusedMaxMaskRampSmoothRamp : EvalNode
{
  uint16_t maskId, htId, angId;
  float htFrom, htTo, angFrom, angTo;
  float eval(const EvalCtx &ctx) const override
  {
    float terrain = fRamp(ctx.vars[htId], htFrom, htTo) * fSRamp(ctx.vars[angId], angFrom, angTo);
    return ctx.vars[maskId] > terrain ? ctx.vars[maskId] : terrain;
  }
};

// #4: v + ramp(v,C,C)
struct FusedSumMaskRamp : EvalNode
{
  uint16_t maskId, htId;
  float htFrom, htTo;
  float eval(const EvalCtx &ctx) const override { return ctx.vars[maskId] + fRamp(ctx.vars[htId], htFrom, htTo); }
};

// #5: v * smooth_ramp(v,C,C)
struct FusedMaskMulSmoothRamp : EvalNode
{
  uint16_t maskId, angId;
  float angFrom, angTo;
  float eval(const EvalCtx &ctx) const override { return ctx.vars[maskId] * fSRamp(ctx.vars[angId], angFrom, angTo); }
};

// #12: v * ramp(v,C,C)
struct FusedMaskMulRamp : EvalNode
{
  uint16_t maskId, varId;
  float from, to;
  float eval(const EvalCtx &ctx) const override { return ctx.vars[maskId] * fRamp(ctx.vars[varId], from, to); }
};

// korea heightLC: max(var, ramp(v,C,C) * ramp(v,C,C) * smooth_ramp(v,C,C)).
// Collapses 15+ virtual dispatches (generic + subtree specialization) into one.
struct FusedMaxMaskRampRampSmoothRamp : EvalNode
{
  uint16_t maskId, ht1Id, ht2Id, angId;
  float ht1From, ht1To, ht2From, ht2To, angFrom, angTo;
  float eval(const EvalCtx &ctx) const override
  {
    float terrain =
      fRamp(ctx.vars[ht1Id], ht1From, ht1To) * fRamp(ctx.vars[ht2Id], ht2From, ht2To) * fSRamp(ctx.vars[angId], angFrom, angTo);
    float m = ctx.vars[maskId];
    return m > terrain ? m : terrain;
  }
};

// zhengzhou: max(var, one - pow(base, max(threshold - var, floor))).
// Exponential attenuation from a height threshold. PowNode drops sign of base, so
// we mirror that with fabsf(base) to keep behavior identical to the unfused path.
struct FusedMaxVarExpAtten : EvalNode
{
  uint16_t maskId, htId;
  float one, base, threshold, floorV;
  float eval(const EvalCtx &ctx) const override
  {
    float diff = threshold - ctx.vars[htId];
    float m0 = diff > floorV ? diff : floorV;
    float r = one - powf(fabsf(base), m0);
    float m = ctx.vars[maskId];
    return m > r ? m : r;
  }
};

// ============================================================================
// Rule builder
// ============================================================================

template <typename T>
void initFusedNode(void *p)
{
  new (p) T();
}

#define P_NODE(idx, tag_str, c0, c1, c2) \
  {                                      \
    hash_name(tag_str), { c0, c1, c2 }   \
  }
#define P_LEAF(idx, tag_str)           \
  {                                    \
    hash_name(tag_str), { -1, -1, -1 } \
  }
#define PATCH_VAR(patIdx, field)   {patIdx, PatchOp::VARID, (uint16_t)offsetof(FusedT, field)}
#define PATCH_CONST(patIdx, field) {patIdx, PatchOp::CONST_VAL, (uint16_t)offsetof(FusedT, field)}

#define BEGIN_RULE(FusedType)      \
  if (n < maxRules)                \
  {                                \
    using FusedT = FusedType;      \
    FusionRule &r = out[n++];      \
    r = {};                        \
    r.nodeSize = sizeof(FusedT);   \
    r.nodeAlign = alignof(FusedT); \
    r.initFn = initFusedNode<FusedT>;

#define SET_PATTERN(...)                                 \
  {                                                      \
    const PatternNode _p[] = {__VA_ARGS__};              \
    r.patternSize = (uint8_t)(sizeof(_p) / sizeof(*_p)); \
    for (int _i = 0; _i < r.patternSize; _i++)           \
      r.pattern[_i] = _p[_i];                            \
  }
#define SET_PATCHES(...)                                \
  {                                                     \
    const PatchOp _q[] = {__VA_ARGS__};                 \
    r.numPatches = (uint8_t)(sizeof(_q) / sizeof(*_q)); \
    for (int _i = 0; _i < r.numPatches; _i++)           \
      r.patches[_i] = _q[_i];                           \
  }
#define END_RULE() }

inline int make_default_fusion_rules(FusionRule *out, int maxRules)
{
  int n = 0;

  // korea heightLC: max(var, ramp(var,c,c) * ramp(var,c,c) * smooth_ramp(var,c,c)).
  // Tree shape: max(0) -> var(1), mul(2). mul(2) -> mul(3), smooth_ramp(4).
  // mul(3) -> ramp(5), ramp(6). Each ramp has var + 2 const children.
  BEGIN_RULE(FusedMaxMaskRampRampSmoothRamp)
  SET_PATTERN(P_NODE(0, "max", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "mul", 3, 4, -1), P_NODE(3, "mul", 5, 6, -1),
    P_NODE(4, "smooth_ramp", 7, 8, 9), P_NODE(5, "ramp", 10, 11, 12), P_NODE(6, "ramp", 13, 14, 15), P_LEAF(7, "var"),
    P_LEAF(8, "const"), P_LEAF(9, "const"), P_LEAF(10, "var"), P_LEAF(11, "const"), P_LEAF(12, "const"), P_LEAF(13, "var"),
    P_LEAF(14, "const"), P_LEAF(15, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(10, ht1Id), PATCH_CONST(11, ht1From), PATCH_CONST(12, ht1To), PATCH_VAR(13, ht2Id),
    PATCH_CONST(14, ht2From), PATCH_CONST(15, ht2To), PATCH_VAR(7, angId), PATCH_CONST(8, angFrom), PATCH_CONST(9, angTo))
  END_RULE()

  // zhengzhou: max(var, 1 - pow(c, max(c - var, 0))).
  // Shape: max(0) -> var(1), sub(2). sub(2) -> const(3), pow(4).
  // pow(4) -> const(5), max(6). max(6) -> sub(7), const(10). sub(7) -> const(8), var(9).
  BEGIN_RULE(FusedMaxVarExpAtten)
  SET_PATTERN(P_NODE(0, "max", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "sub", 3, 4, -1), P_LEAF(3, "const"), P_NODE(4, "pow", 5, 6, -1),
    P_LEAF(5, "const"), P_NODE(6, "max", 7, 10, -1), P_NODE(7, "sub", 8, 9, -1), P_LEAF(8, "const"), P_LEAF(9, "var"),
    P_LEAF(10, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_CONST(3, one), PATCH_CONST(5, base), PATCH_CONST(8, threshold), PATCH_VAR(9, htId),
    PATCH_CONST(10, floorV))
  END_RULE()

  // max(var, ramp(var,c,c) * smooth_ramp(var,c,c))
  BEGIN_RULE(FusedMaxMaskRampSmoothRamp)
  SET_PATTERN(P_NODE(0, "max", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "mul", 3, 7, -1), P_NODE(3, "ramp", 4, 5, 6), P_LEAF(4, "var"),
    P_LEAF(5, "const"), P_LEAF(6, "const"), P_NODE(7, "smooth_ramp", 8, 9, 10), P_LEAF(8, "var"), P_LEAF(9, "const"),
    P_LEAF(10, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(4, htId), PATCH_CONST(5, htFrom), PATCH_CONST(6, htTo), PATCH_VAR(8, angId),
    PATCH_CONST(9, angFrom), PATCH_CONST(10, angTo))
  END_RULE()

  // var + ramp(var, c, c)
  BEGIN_RULE(FusedSumMaskRamp)
  SET_PATTERN(P_NODE(0, "add", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "ramp", 3, 4, 5), P_LEAF(3, "var"), P_LEAF(4, "const"),
    P_LEAF(5, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(3, htId), PATCH_CONST(4, htFrom), PATCH_CONST(5, htTo))
  END_RULE()

  // var * smooth_ramp(var, c, c)
  BEGIN_RULE(FusedMaskMulSmoothRamp)
  SET_PATTERN(P_NODE(0, "mul", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "smooth_ramp", 3, 4, 5), P_LEAF(3, "var"), P_LEAF(4, "const"),
    P_LEAF(5, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(3, angId), PATCH_CONST(4, angFrom), PATCH_CONST(5, angTo))
  END_RULE()

  // var * ramp(var, c, c)
  BEGIN_RULE(FusedMaskMulRamp)
  SET_PATTERN(P_NODE(0, "mul", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "ramp", 3, 4, 5), P_LEAF(3, "var"), P_LEAF(4, "const"),
    P_LEAF(5, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(3, varId), PATCH_CONST(4, from), PATCH_CONST(5, to))
  END_RULE()

  // ramp(var, c, c) * smooth_ramp(var, c, c) -- no mask
  BEGIN_RULE(FusedRampMulSmoothRamp)
  SET_PATTERN(P_NODE(0, "mul", 1, 5, -1), P_NODE(1, "ramp", 2, 3, 4), P_LEAF(2, "var"), P_LEAF(3, "const"), P_LEAF(4, "const"),
    P_NODE(5, "smooth_ramp", 6, 7, 8), P_LEAF(6, "var"), P_LEAF(7, "const"), P_LEAF(8, "const"))
  SET_PATCHES(PATCH_VAR(2, htId), PATCH_CONST(3, htFrom), PATCH_CONST(4, htTo), PATCH_VAR(6, angId), PATCH_CONST(7, angFrom),
    PATCH_CONST(8, angTo))
  END_RULE()

  // max(var, ramp(var, c, c))
  BEGIN_RULE(FusedMaxVarRamp)
  SET_PATTERN(P_NODE(0, "max", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "ramp", 3, 4, 5), P_LEAF(3, "var"), P_LEAF(4, "const"),
    P_LEAF(5, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(3, varId), PATCH_CONST(4, from), PATCH_CONST(5, to))
  END_RULE()

  // max(var, smooth_ramp(var, c, c))
  BEGIN_RULE(FusedMaxVarSmoothRamp)
  SET_PATTERN(P_NODE(0, "max", 1, 2, -1), P_LEAF(1, "var"), P_NODE(2, "smooth_ramp", 3, 4, 5), P_LEAF(3, "var"), P_LEAF(4, "const"),
    P_LEAF(5, "const"))
  SET_PATCHES(PATCH_VAR(1, maskId), PATCH_VAR(3, varId), PATCH_CONST(4, from), PATCH_CONST(5, to))
  END_RULE()

  // ramp(var,c,c) and smooth_ramp(var,c,c) are handled by the compiler directly
  // as RampVarConstNode/SmoothRampVarConstNode -- no fusion rule needed.

  return n;
}

#undef P_NODE
#undef P_LEAF
#undef PATCH_VAR
#undef PATCH_CONST
#undef BEGIN_RULE
#undef SET_PATTERN
#undef SET_PATCHES
#undef END_RULE

// ============================================================================
// Convenience: parse with fusion
// ============================================================================

// outMaxVarId (optional): highest varId referenced by the expression, or -1 if none.
inline uint32_t parseFused(const char *text, Arena &arena, const FuncParseMap &parseMap, const NodeEmitMap &emitMap,
  const FusionRule *rules, int numRules, NameMap &varMap, uint16_t &nextVarId, eastl::string &error, int *outMaxVarId = nullptr)
{
  // Snapshot the name state so a post-parseToIR compile failure rolls back atomically.
  // parseToIR itself already restores varMap / nextVarId on its own failure, but once
  // parseToIR has succeeded any new variables introduced by the text must also be
  // reverted if the downstream compile step subsequently fails.
  uint32_t startOfs = (uint32_t)arena.size();
  NameMap savedVarMap(varMap);
  uint16_t savedNextVarId = nextVarId;
  Tab<IRNode> ir;
  int root = parseToIR(text, ir, parseMap, emitMap, varMap, nextVarId, error);
  if (root < 0)
  {
    arena.resize(startOfs);
    return PARSE_ERROR;
  }
  uint32_t result = tryFuse(ir, root, arena, rules, numRules);
  if (result == PARSE_ERROR)
  {
    result = compile(ir, root, arena, emitMap);
    if (result == PARSE_ERROR)
    {
      arena.resize(startOfs);
      varMap = savedVarMap;
      nextVarId = savedNextVarId;
      error = "compile failed: IR tag missing from emit map";
      return PARSE_ERROR;
    }
  }
  if (outMaxVarId)
    *outMaxVarId = computeMaxVarId(ir, root);
  return result;
}

} // namespace lcexpr
