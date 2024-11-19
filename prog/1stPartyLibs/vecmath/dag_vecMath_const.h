//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_vecMathDecl.h"

#ifdef __cplusplus
#include <cmath> // for INFINITY
#else
#include <math.h>
#endif


#if defined(_MSC_VER) && !defined(__clang__)
  #define DECL_VEC_CONST extern const __declspec(selectany) __declspec(align(16))
#else
  #define DECL_VEC_CONST alignas(16) static const
#endif

#define REPLICATE(v) v, v, v, v

#if _TARGET_SIMD_SSE
  DECL_VEC_CONST vec4f_const V_C_HALF = { REPLICATE(0.5f) };
  DECL_VEC_CONST vec4f_const V_C_HALF_MINUS_EPS = { REPLICATE(0.5f - 1.192092896e-07f * 32) };
  DECL_VEC_CONST vec4f_const V_C_ONE = { REPLICATE(1.0f) };
  DECL_VEC_CONST vec4f_const V_C_TWO = { REPLICATE(2.0f) };
  DECL_VEC_CONST vec4f_const V_C_PI = { REPLICATE(3.14159265f) };                  // pi
  DECL_VEC_CONST vec4f_const V_C_HALFPI = { REPLICATE(3.14159265f / 2.f) };        // pi/2
  DECL_VEC_CONST vec4f_const V_C_TWOPI = { REPLICATE(2.f * 3.14159265f) };         // pi*2
  DECL_VEC_CONST vec4f_const V_C_PI_DIV_4 = { REPLICATE(0.78539816f) };            // pi/4
  DECL_VEC_CONST vec4f_const V_C_2_DIV_PI = { REPLICATE(0.63661977f) };            // 2/pi
  DECL_VEC_CONST vec4f_const V_C_4_DIV_PI = { REPLICATE(1.27323954f) };            // 4/pi
  DECL_VEC_CONST vec4f_const V_C_MAX_VAL = { REPLICATE(1e32f) };
  DECL_VEC_CONST vec4f_const V_C_MIN_VAL = { REPLICATE(-1e32f) };
  DECL_VEC_CONST vec4f_const V_C_EPS_VAL = { REPLICATE(1.192092896e-07f) };
  DECL_VEC_CONST vec4f_const V_C_VERY_SMALL_VAL = { REPLICATE(4e-19f) };          // ~sqrt(FLT_MIN), safe threshold for passing argument to rcp(x)
#if defined(__clang__) || defined(__GNUC__)
  DECL_VEC_CONST vec4f_const V_C_INF = { REPLICATE(__builtin_inff()) };
#else
  DECL_VEC_CONST vec4f_const V_C_INF = { REPLICATE(INFINITY) };
#endif

  #define DECL_VECFLOAT4(X,Y,Z,W) {X,Y,Z,W}
  #define DECL_VECUINT4(X,Y,Z,W)  {X,Y,Z,W}

  DECL_VEC_CONST vec4i_const V_CI_SIGN_MASK = { REPLICATE(0x80000000) };
  DECL_VEC_CONST vec4i_const V_CI_INV_SIGN_MASK = { REPLICATE(0x7FFFFFFF) };
  DECL_VEC_CONST vec4i_const V_CI_3FF = { REPLICATE(0x3FF) };
  DECL_VEC_CONST vec4i_const V_CI_070 = { REPLICATE(0x70) };
  DECL_VEC_CONST vec4i_const V_CI_0FF = { REPLICATE(0xFF) };
  DECL_VEC_CONST vec4i_const V_CI_01F = { REPLICATE(0x1F) };
  DECL_VEC_CONST vec4i_const V_CI_07FFFFF = { REPLICATE(0x7fffff) };
  DECL_VEC_CONST vec4i_const V_CI_0 = { REPLICATE(0) };
  DECL_VEC_CONST vec4i_const V_CI_1 = { REPLICATE(1) };
  DECL_VEC_CONST vec4i_const V_CI_2 = { REPLICATE(2) };
  DECL_VEC_CONST vec4i_const V_CI_3 = { REPLICATE(3) };
  DECL_VEC_CONST vec4i_const V_CI_4 = { REPLICATE(4) };

  DECL_VEC_CONST vec4f_const V_C_UNIT_1000 = {1.0f, 0.0f, 0.0f, 0.0f};
  DECL_VEC_CONST vec4f_const V_C_UNIT_0100 = {0.0f, 1.0f, 0.0f, 0.0f};
  DECL_VEC_CONST vec4f_const V_C_UNIT_0010 = {0.0f, 0.0f, 1.0f, 0.0f};
  DECL_VEC_CONST vec4f_const V_C_UNIT_0001 = {0.0f, 0.0f, 0.0f, 1.0f};
  DECL_VEC_CONST vec4f_const V_C_UNIT_1110 = {1.0f, 1.0f, 1.0f, 0.0f};
  DECL_VEC_CONST vec4f_const V_C_UNIT_0011 = {0.0f, 0.0f, 1.0f, 1.0f};

  DECL_VEC_CONST vec4i_const V_CI_MASK0000 = { 0, 0, 0, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK0001 = { 0, 0, 0, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK0010 = { 0, 0, 0xFFFFFFFF, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK0011 = { 0, 0, 0xFFFFFFFF, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK0100 = { 0, 0xFFFFFFFF, 0, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK0101 = { 0, 0xFFFFFFFF, 0, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK0110 = { 0, 0xFFFFFFFF, 0xFFFFFFFF, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK0111 = { 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK1000 = { 0xFFFFFFFF, 0, 0, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK1001 = { 0xFFFFFFFF, 0, 0, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK1010 = { 0xFFFFFFFF, 0, 0xFFFFFFFF, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK1011 = { 0xFFFFFFFF, 0, 0xFFFFFFFF, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK1100 = { 0xFFFFFFFF, 0xFFFFFFFF, 0, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK1101 = { 0xFFFFFFFF, 0xFFFFFFFF, 0, 0xFFFFFFFF };
  DECL_VEC_CONST vec4i_const V_CI_MASK1110 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 };
  DECL_VEC_CONST vec4i_const V_CI_MASK1111 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

#elif _TARGET_SIMD_NEON
  #define V_C_HALF            vdupq_n_f32(0.5f)
  #define V_C_HALF_MINUS_EPS  vdupq_n_f32(0.5f - 1.192092896e-07f * 32)
  #define V_C_ONE             vdupq_n_f32(1.0f)
  #define V_C_TWO             vdupq_n_f32(2.0f)
  #define V_C_PI              vdupq_n_f32(3.14159265f)           // pi
  #define V_C_HALFPI          vdupq_n_f32(3.14159265f / 2.f)     // pi/2
  #define V_C_TWOPI           vdupq_n_f32(2.f * 3.14159265f)     // pi*2
  #define V_C_PI_DIV_4        vdupq_n_f32(0.78539816f)           // pi/4
  #define V_C_2_DIV_PI        vdupq_n_f32(0.63661977f)           // 2/pi
  #define V_C_4_DIV_PI        vdupq_n_f32(1.27323954f)           // 4/pi
  #define V_C_MAX_VAL         vdupq_n_f32(1e32f)
  #define V_C_MIN_VAL         vdupq_n_f32(-1e32f)
  #define V_C_EPS_VAL         vdupq_n_f32(1.192092896e-07f)
  #define V_C_VERY_SMALL_VAL  vdupq_n_f32(4e-19f)              // ~sqrt(FLT_MIN), safe threshold for passing argument to rcp(x)

  #define V_CI_SIGN_MASK      vdupq_n_s32(0x80000000)
  #define V_CI_INV_SIGN_MASK  vdupq_n_s32(0x7FFFFFFF)
  #define V_CI_3FF            vdupq_n_s32(0x3FF)
  #define V_CI_070            vdupq_n_s32(0x70)
  #define V_CI_0FF            vdupq_n_s32(0xFF)
  #define V_CI_0FF            vdupq_n_s32(0xFF)
  #define V_CI_01F            vdupq_n_s32(0x1F)
  #define V_CI_07FFFFF        vdupq_n_s32(0x7fffff)
  #define V_CI_0              vdupq_n_s32(0)
  #define V_CI_1              vdupq_n_s32(1)
  #define V_CI_2              vdupq_n_s32(2)
  #define V_CI_3              vdupq_n_s32(3)
  #define V_CI_4              vdupq_n_s32(4)

#if defined(__clang__) || defined(__GNUC__)
  #define V_C_INF             vdupq_n_f32(__builtin_inff())

  #define DECL_VECFLOAT4(X,Y,Z,W) (float32x4_t){X,Y,Z,W}
  #define DECL_VECUINT4(X,Y,Z,W)  (int32x4_t){(int)X,(int)Y,(int)Z,(int)W}
#elif defined(_MSC_VER)
  #define V_C_INF             vdupq_n_f32(INFINITY)

  constexpr struct vec4f_const_hlp
  {
    constexpr vec4f_const_hlp(float x, float y, float z, float w)
    {
      v.n128_f32[0] = x;
      v.n128_f32[1] = y;
      v.n128_f32[2] = z;
      v.n128_f32[3] = w;
    }
    __n128 v;
  };
  constexpr struct vec4i_const_hlp
  {
    constexpr vec4i_const_hlp(int x, int y, int z, int w)
    {
      v.n128_i32[0] = x;
      v.n128_i32[1] = y;
      v.n128_i32[2] = z;
      v.n128_i32[3] = w;
    }
    __n128 v;
  };
  #define DECL_VECFLOAT4(X,Y,Z,W) vec4f_const_hlp(X,Y,Z,W).v
  #define DECL_VECUINT4(X,Y,Z,W)  vec4i_const_hlp((int)X,(int)Y,(int)Z,(int)W).v
#endif

  #define V_C_UNIT_1000       DECL_VECFLOAT4( 1.0f, 0.0f, 0.0f, 0.0f )
  #define V_C_UNIT_0100       DECL_VECFLOAT4( 0.0f, 1.0f, 0.0f, 0.0f )
  #define V_C_UNIT_0010       DECL_VECFLOAT4( 0.0f, 0.0f, 1.0f, 0.0f )
  #define V_C_UNIT_0001       DECL_VECFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f )
  #define V_C_UNIT_1110       DECL_VECFLOAT4( 1.0f, 1.0f, 1.0f, 0.0f )
  #define V_C_UNIT_0011       DECL_VECFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f )

  #define V_CI_MASK0000       vdupq_n_s32(0)
  #define V_CI_MASK0001       DECL_VECUINT4( 0, 0, 0, -1 )
  #define V_CI_MASK0010       DECL_VECUINT4( 0, 0, -1, 0 )
  #define V_CI_MASK0011       DECL_VECUINT4( 0, 0, -1, -1 )
  #define V_CI_MASK0100       DECL_VECUINT4( 0, -1, 0, 0 )
  #define V_CI_MASK0101       DECL_VECUINT4( 0, -1, 0, -1 )
  #define V_CI_MASK0110       DECL_VECUINT4( 0, -1, -1, 0 )
  #define V_CI_MASK0111       DECL_VECUINT4( 0, -1, -1, -1 )
  #define V_CI_MASK1000       DECL_VECUINT4( -1, 0, 0, 0 )
  #define V_CI_MASK1001       DECL_VECUINT4( -1, 0, 0, -1 )
  #define V_CI_MASK1010       DECL_VECUINT4( -1, 0, -1, 0 )
  #define V_CI_MASK1011       DECL_VECUINT4( -1, 0, -1, -1 )
  #define V_CI_MASK1100       DECL_VECUINT4( -1, -1, 0, 0 )
  #define V_CI_MASK1101       DECL_VECUINT4( -1, -1, 0, -1 )
  #define V_CI_MASK1110       DECL_VECUINT4( -1, -1, -1, 0 )
  #define V_CI_MASK1111       DECL_VECUINT4( -1, -1, -1, -1 )
#endif

#undef REPLICATE
#undef DECL_VEC_CONST
