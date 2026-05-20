// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <landClassEval/lcExprParserImpl.h>

// HeightmapLand-local factory for registering custom landclass-expression
// functions without touching gameLibs/landClassEval.
//
// The node-declaration macros mirror LC_UNARY_NODE / LC_BINARY_NODE / LC_TERNARY_NODE
// in prog/gameLibs/publicInclude/landClassEval/lcExprNodes.h: each one declares an
// EvalNode subclass whose eval() reads its children into s0[/s1[/s2]] and returns
// the provided expression. The registration helpers then wire a name -> arity into
// the parse map and a name -> arena-emit into the emit map, using the existing
// lcexpr::emit{Unary,Binary,Ternary}Fn<T> helpers from the core.
#define HML_LC_USER_UNARY(Name, expr)                     \
  struct Name : lcexpr::EvalNode                          \
  {                                                       \
    uint32_t n0;                                          \
    float eval(const lcexpr::EvalCtx &ctx) const override \
    {                                                     \
      float s0 = ctx.at(n0)->eval(ctx);                   \
      return (expr);                                      \
    }                                                     \
  }

#define HML_LC_USER_BINARY(Name, expr)                    \
  struct Name : lcexpr::EvalNode                          \
  {                                                       \
    uint32_t n0, n1;                                      \
    float eval(const lcexpr::EvalCtx &ctx) const override \
    {                                                     \
      float s0 = ctx.at(n0)->eval(ctx);                   \
      float s1 = ctx.at(n1)->eval(ctx);                   \
      return (expr);                                      \
    }                                                     \
  }

#define HML_LC_USER_TERNARY(Name, expr)                   \
  struct Name : lcexpr::EvalNode                          \
  {                                                       \
    uint32_t n0, n1, n2;                                  \
    float eval(const lcexpr::EvalCtx &ctx) const override \
    {                                                     \
      float s0 = ctx.at(n0)->eval(ctx);                   \
      float s1 = ctx.at(n1)->eval(ctx);                   \
      float s2 = ctx.at(n2)->eval(ctx);                   \
      return (expr);                                      \
    }                                                     \
  }

namespace hml_lcexpr_factory
{

// HeightmapLand reserves the first user-type slot for Curve. CurveNode (defined
// in hmlGenColorMap.cpp) reads its first arg as a varId-tagged float that holds
// an integer slot index into the curves table passed via EvalCtx::userCtx.
inline constexpr lcexpr::ExprType TYPE_CURVE = lcexpr::TYPE_USER_BASE;

// Thin wrappers that keep the "parse map + emit map" pair in sync. Pass the
// lcexpr::emit{Unary,Binary,Ternary}Fn<YourNode> instantiation as `emitter`.
inline void register_unary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name, lcexpr::NodeEmitFn emitter)
{
  lcexpr::register_func_parse(parseMap, name, 1);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

inline void register_binary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name, lcexpr::NodeEmitFn emitter)
{
  lcexpr::register_func_parse(parseMap, name, 2);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

inline void register_ternary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name,
  lcexpr::NodeEmitFn emitter)
{
  lcexpr::register_func_parse(parseMap, name, 3);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

// Typed registrations: same shape as the untyped wrappers above but propagate
// per-arg / result types into FuncParseInfo. Use these for functions whose
// arguments must be a specific user type (e.g. CURVE) -- the parser then
// rejects calls that pass the wrong type instead of silently coercing.
inline void register_typed_unary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name,
  lcexpr::ExprType argType, lcexpr::ExprType resultType, lcexpr::NodeEmitFn emitter)
{
  lcexpr::ExprType args[1] = {argType};
  lcexpr::register_func_parse_typed(parseMap, name, 1, args, resultType);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

inline void register_typed_binary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name,
  lcexpr::ExprType arg0Type, lcexpr::ExprType arg1Type, lcexpr::ExprType resultType, lcexpr::NodeEmitFn emitter)
{
  lcexpr::ExprType args[2] = {arg0Type, arg1Type};
  lcexpr::register_func_parse_typed(parseMap, name, 2, args, resultType);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

inline void register_typed_ternary(lcexpr::FuncParseMap &parseMap, lcexpr::NodeEmitMap &emitMap, const char *name,
  lcexpr::ExprType arg0Type, lcexpr::ExprType arg1Type, lcexpr::ExprType arg2Type, lcexpr::ExprType resultType,
  lcexpr::NodeEmitFn emitter)
{
  lcexpr::ExprType args[3] = {arg0Type, arg1Type, arg2Type};
  lcexpr::register_func_parse_typed(parseMap, name, 3, args, resultType);
  emitMap.emplace(lcexpr::hash_name(name), emitter);
}

} // namespace hml_lcexpr_factory
