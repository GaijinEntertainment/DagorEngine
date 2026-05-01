//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <landClassEval/lcExprEval.h>
#include <math.h>

// Landclass expression node types
// Evaluation only requires lcExprEval.h (EvalNode virtual dispatch).
namespace lcexpr
{

#define LC_UNARY_NODE(Name, expr)                 \
  struct Name : EvalNode                          \
  {                                               \
    uint32_t n0;                                  \
    float eval(const EvalCtx &ctx) const override \
    {                                             \
      float s0 = ctx.at(n0)->eval(ctx);           \
      return (expr);                              \
    }                                             \
  }

#define LC_BINARY_NODE(Name, expr)                \
  struct Name : EvalNode                          \
  {                                               \
    uint32_t n0, n1;                              \
    float eval(const EvalCtx &ctx) const override \
    {                                             \
      float s0 = ctx.at(n0)->eval(ctx);           \
      float s1 = ctx.at(n1)->eval(ctx);           \
      return (expr);                              \
    }                                             \
  }

#define LC_TERNARY_NODE(Name, expr)               \
  struct Name : EvalNode                          \
  {                                               \
    uint32_t n0, n1, n2;                          \
    float eval(const EvalCtx &ctx) const override \
    {                                             \
      float s0 = ctx.at(n0)->eval(ctx);           \
      float s1 = ctx.at(n1)->eval(ctx);           \
      float s2 = ctx.at(n2)->eval(ctx);           \
      return (expr);                              \
    }                                             \
  }

// ---- Leaf nodes ----

struct ConstNode : EvalNode
{
  float value;
  float eval(const EvalCtx &) const override { return value; }
};

struct VarNode : EvalNode
{
  uint16_t varId;
  float eval(const EvalCtx &ctx) const override { return ctx.vars[varId]; }
};

// ---- Operator nodes ----

LC_UNARY_NODE(NegNode, -s0);
LC_UNARY_NODE(NotNode, s0 == 0.f ? 1.f : 0.f);

LC_BINARY_NODE(AddNode, s0 + s1);
LC_BINARY_NODE(SubNode, s0 - s1);
LC_BINARY_NODE(MulNode, s0 *s1);
LC_BINARY_NODE(DivNode, s0 / s1); // raw IEEE division; use evalSafe to clamp inf/nan

LC_BINARY_NODE(LtNode, s0 < s1 ? 1.f : 0.f);
LC_BINARY_NODE(GtNode, s0 > s1 ? 1.f : 0.f);
LC_BINARY_NODE(LeNode, s0 <= s1 ? 1.f : 0.f);
LC_BINARY_NODE(GeNode, s0 >= s1 ? 1.f : 0.f);
LC_BINARY_NODE(EqNode, fabsf(s0 - s1) < 1e-6f ? 1.f : 0.f);
LC_BINARY_NODE(NeNode, fabsf(s0 - s1) >= 1e-6f ? 1.f : 0.f);

// Short-circuit && and ||: only evaluate the right operand when needed.
struct AndNode : EvalNode
{
  uint32_t n0, n1;
  float eval(const EvalCtx &ctx) const override
  {
    if (ctx.at(n0)->eval(ctx) == 0.f)
      return 0.f;
    return ctx.at(n1)->eval(ctx) != 0.f ? 1.f : 0.f;
  }
};
struct OrNode : EvalNode
{
  uint32_t n0, n1;
  float eval(const EvalCtx &ctx) const override
  {
    if (ctx.at(n0)->eval(ctx) != 0.f)
      return 1.f;
    return ctx.at(n1)->eval(ctx) != 0.f ? 1.f : 0.f;
  }
};

// ---- Function nodes ----

LC_BINARY_NODE(MaxNode, s0 > s1 ? s0 : s1);
LC_BINARY_NODE(MinNode, s0 < s1 ? s0 : s1);
LC_BINARY_NODE(PowNode, powf(fabsf(s0), s1)); // safe: abs base to avoid NaN
LC_UNARY_NODE(SaturateNode, s0 < 0.f ? 0.f : (s0 > 1.f ? 1.f : s0));
LC_UNARY_NODE(SqrtNode, sqrtf(s0 > 0.f ? s0 : 0.f));
LC_UNARY_NODE(AbsNode, fabsf(s0));
LC_UNARY_NODE(FracNode, s0 - floorf(s0));
LC_TERNARY_NODE(ClampNode, s0 < s1 ? s1 : (s0 > s2 ? s2 : s0));
LC_TERNARY_NODE(LerpNode, s0 + (s1 - s0) * s2);

struct SmoothstepNode : EvalNode
{
  uint32_t n0;
  float eval(const EvalCtx &ctx) const override
  {
    float t = ctx.at(n0)->eval(ctx);
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    return t * t * (3.f - 2.f * t);
  }
};

struct RampNode : EvalNode
{
  uint32_t n0, n1, n2;
  float eval(const EvalCtx &ctx) const override
  {
    float val = ctx.at(n0)->eval(ctx), from = ctx.at(n1)->eval(ctx), to = ctx.at(n2)->eval(ctx);
    float dv = to - from;
    if (fabsf(dv) < 1e-10f)
      return val >= from ? 1.f : 0.f;
    float t = (val - from) / dv;
    return t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  }
};

struct SmoothRampNode : EvalNode
{
  uint32_t n0, n1, n2;
  float eval(const EvalCtx &ctx) const override
  {
    float val = ctx.at(n0)->eval(ctx), from = ctx.at(n1)->eval(ctx), to = ctx.at(n2)->eval(ctx);
    float dv = to - from;
    float t = fabsf(dv) < 1e-10f ? (val >= from ? 1.f : 0.f) : (val - from) / dv;
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    return t * t * (3.f - 2.f * t);
  }
};

// Optimized: ramp(vars[varId], from, to) -- no child dispatch, constants embedded
struct RampVarConstNode : EvalNode
{
  uint16_t varId;
  float from, to;
  float eval(const EvalCtx &ctx) const override
  {
    float dv = to - from;
    if (fabsf(dv) < 1e-10f)
      return ctx.vars[varId] >= from ? 1.f : 0.f;
    float t = (ctx.vars[varId] - from) / dv;
    return t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  }
};

// Optimized: smooth_ramp(vars[varId], from, to)
struct SmoothRampVarConstNode : EvalNode
{
  uint16_t varId;
  float from, to;
  float eval(const EvalCtx &ctx) const override
  {
    float dv = to - from;
    float t = fabsf(dv) < 1e-10f ? (ctx.vars[varId] >= from ? 1.f : 0.f) : (ctx.vars[varId] - from) / dv;
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    return t * t * (3.f - 2.f * t);
  }
};

struct SelectNode : EvalNode
{
  uint32_t n0, n1, n2;
  float eval(const EvalCtx &ctx) const override
  {
    return ctx.at(n0)->eval(ctx) != 0.f ? ctx.at(n1)->eval(ctx) : ctx.at(n2)->eval(ctx);
  }
};

// Assignment: writes vars[varId] = eval(n0) and returns that value. The parser only
// emits AssignNode when the target was previously declared via var/let, so this never
// writes into an external read-only slot.
struct AssignNode : EvalNode
{
  uint32_t n0;
  uint16_t varId;
  float eval(const EvalCtx &ctx) const override
  {
    float v = ctx.at(n0)->eval(ctx);
    ctx.vars[varId] = v;
    return v;
  }
};

// Sequence: evaluates n0 (for its side effects, typically an assignment), discards the
// result, then evaluates n1 and returns it. Both `,` and `;` parse to this node.
struct SeqNode : EvalNode
{
  uint32_t n0, n1;
  float eval(const EvalCtx &ctx) const override
  {
    ctx.at(n0)->eval(ctx);
    return ctx.at(n1)->eval(ctx);
  }
};

#undef LC_UNARY_NODE
#undef LC_BINARY_NODE
#undef LC_TERNARY_NODE

} // namespace lcexpr
