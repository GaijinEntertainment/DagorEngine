// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "scalarTypes.h"
#include <math/dag_mathBase.h>

#ifndef __GENERATED_STCODE_FILE
#error This file can only be included in generated stcode
#endif

// These will be used further down the includes
#include <shaders/commonStcodeFunctions.h>

namespace stcode::cpp
{

inline float vsin(float x) { return sinf(x); }
inline float vcos(float x) { return cosf(x); }
inline float vsqrt(float x) { return sqrtf(x); }

inline float vpow(float x, float p) { return powf(x, p); }

inline float vmax(float a, float b) { return max(a, b); }
inline float vmin(float a, float b) { return min(a, b); }
inline float vfsel(float a, float b, float c) { return fsel(a, b, c); }

#define DECLARE_ONE_ARG_VEC_FUNC(_func) \
  inline float4 _func(float4 v) { return float4(_func(v.r), _func(v.g), _func(v.b), _func(v.a)); }

#define DECLARE_TWO_ARG_VEC_FUNC(_func) \
  inline float4 _func(float4 v, float4 u) { return float4(_func(v.r, u.r), _func(v.g, u.g), _func(v.b, u.b), _func(v.a, u.a)); }

#define DECLARE_THREE_ARG_VEC_FUNC(_func)                                                                  \
  inline float4 _func(float4 v, float4 u, float4 w)                                                        \
  {                                                                                                        \
    return float4(_func(v.r, u.r, w.r), _func(v.g, u.g, w.g), _func(v.b, u.b, w.b), _func(v.a, u.a, w.a)); \
  }

DECLARE_ONE_ARG_VEC_FUNC(vsin)
DECLARE_ONE_ARG_VEC_FUNC(vcos)
DECLARE_ONE_ARG_VEC_FUNC(vsqrt)

DECLARE_TWO_ARG_VEC_FUNC(vpow)

DECLARE_TWO_ARG_VEC_FUNC(vmin)
DECLARE_TWO_ARG_VEC_FUNC(vmax)

DECLARE_THREE_ARG_VEC_FUNC(vfsel)


#undef DECLARE_ONE_ARG_VEC_FUNC
#undef DECLARE_TWO_ARG_VEC_FUNC
#undef DECLARE_THREE_ARG_VEC_FUNC

} // namespace stcode::cpp
