//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_math.h>
#include <float.h>
#include <math/dag_half.h>
#include <math/dag_declAlign.h>
#include <math/dag_bits.h>
#include <math/dag_rectInt.h>
#include <util/dag_bitwise_cast.h>
#include <util/dag_compilerDefs.h>
#include <math/dag_check_nan.h>
#if _TARGET_SIMD_SSE
// To consider: use pragma intrinsic instead of including xmmintrin.h (for msvc)
#include <xmmintrin.h>
#elif _TARGET_SIMD_NEON
#include <arm_neon.h>
#endif

/// @addtogroup math
/// @{

// forward declarations for base math classes
class Point2;
class Point3;
class Point4;
class DPoint3;
class IPoint2;
class IPoint3;

class BBox2;
class BBox3;
class BSphere3;
class IBBox2;
class IBBox3;

class Quat;
class Matrix3;
class TMatrix;
class TMatrix4;

struct Frustum;


/// Common real number type, defined as float.
#ifndef __GAIJIN__REAL_TYPE_DEFINED__
#define __GAIJIN__REAL_TYPE_DEFINED__
typedef float real;
#endif

/// useful math constants

/// maximum possible #real value
#define MAX_REAL FLT_MAX
/// minimum possible #real value
#define MIN_REAL (-FLT_MAX)
/// smallest possible positive #real value greater than zero
#define REAL_EPS FLT_EPSILON

/// PI
#define PI     ((real)3.1415926535)
/// 2*PI
#define TWOPI  ((real)6.283185307)
/// PI/2
#define HALFPI ((real)1.570796326794895)

/// ~sqrt(FLT_MIN)
#define VERY_SMALL_NUMBER ((real)4e-19)

/// It is suggested to use in game mechanics to represent "unattainable" or very large values.
/// This value can be accurately represented by the double, float and int32 types,
/// and also converted to a string with the '%g' format (without the need to specify accuracy)
/// without loss of accuracy.
#define VERY_BIG_NUMBER ((real)2147440000)

/// useful math functions (routed via macroses)
#define DEG_TO_RAD    (PI / (real)180.0)
#define RAD_TO_DEG    ((real)180.0 / PI)
/// converts degrees to radians
#define DegToRad(deg) ((real)(deg)*DEG_TO_RAD)
/// converts radians to degrees
#define RadToDeg(rad) ((real)(rad)*RAD_TO_DEG)

#define INLINE __forceinline


INLINE float rabs(float a) { return fabsf(a); }

INLINE float fsel(float a, float b, float c) { return (a >= 0.0f) ? b : c; }
INLINE double fsel(double a, double b, double c) { return (a >= 0.0) ? b : c; }

template <class T>
constexpr T sqr(T x)
{
  return x * x;
}


/// normalize angle to the range [0;2*PI)
INLINE real norm_ang(real a)
{
  if (a >= 0.f && a < TWOPI)
    return a;
  else
    return a >= 0.f ? fmodf(a, TWOPI) : TWOPI + fmodf(a, TWOPI);
}

/// normalize angle to the range [-PI;PI]
INLINE float norm_s_ang(float a)
{
  a = fmodf(a, 2 * PI);
  return a > PI ? a - 2 * PI : (a < -PI ? a + 2 * PI : a);
}

// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
INLINE bool is_relative_equal_float(float a, float b, float max_diff = 1e-5f, float max_rel_diff = FLT_EPSILON)
{
  const float diff = fabsf(a - b);
  if (diff <= max_diff)
    return true;
  a = fabsf(a);
  b = fabsf(b);
  return diff <= (a < b ? b : a) * max_rel_diff;
}

INLINE bool is_equal_float(float a, float b, float eps = 1e-5f) { return is_relative_equal_float(a, b, eps, FLT_EPSILON); }


INLINE bool are_approximately_equal(float a, float b, float epsilon = 8 * FLT_EPSILON) // Three bits precision by default.
{
  return is_relative_equal_float(a, b, epsilon, epsilon);
}


/// @name Fast floating-point routines
// @{

/// @cond

#define FP_BITS(fp)        (bitwise_cast<unsigned int, float>(fp))
#define FP_ABS_BITS(fp)    (FP_BITS(fp) & 0x7FFFFFFF)
#define FP_SIGN_BIT(fp)    (FP_BITS(fp) & 0x80000000)
#define FP_ONE_BITS        0x3F800000
#define FP_SIGNIFICAND(fp) (FP_BITS(fp) & 0x007FFFFF)
#define FP_EXPONENT(fp)    (FP_BITS(fp) & 0x7F800000)

#if !_TARGET_PC_LINUX
INLINE int __signbitf(float x) { return FP_SIGN_BIT(x); }
INLINE int signbitf(float x) { return FP_SIGN_BIT(x); }
#endif

INLINE bool float_nonzero(float x) { return rabs(x) >= FLT_EPSILON; }
INLINE float flt_epsion_threshold(float v) { return fabsf(v) < FLT_EPSILON ? 0.f : v; }
INLINE double flt_epsion_threshold(double v) { return fabs(v) < FLT_EPSILON ? 0.0 : v; }

/// @endcond

/// @name Fast floating-point routines
// @{

/// returns exp(-p) (approximate)
INLINE float fastexp(float p)
{
  const float e = -1.44269504f * float(0x00800000) * p;
  const int _i = int(e) + 0x3F800000;
  return bitwise_cast<float, int>(_i);
}

/// converts p (range 0..1) to integer (range 0..255), clamps if out of range
INLINE unsigned real2uchar(float p)
{
  const float _n = p + 1.0f;
  unsigned i = FP_BITS(_n);
  if (i >= 0x40000000)
    i = 0xFF;
  else if (i <= 0x3F800000)
    i = 0;
  else
    i = (i >> 15) & 0xFF;
  return i;
}

template <typename T>
struct DisablePointersInMath
{};
template <typename T>
struct DisablePointersInMath<T *>;

template <typename T>
INLINE T clamp(T t, const T min_val, const T max_val)
{
  DisablePointersInMath<T>();
  if (t < min_val)
    t = min_val;
  if (t <= max_val)
    return t;
  return max_val;
}

// fast versions, |Relative Error| <= 1.5 * 2^(-12)
#if _TARGET_SIMD_SSE
INLINE float fastinvsqrt(float x) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); }
INLINE float fastinv(float x) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x))); }
INLINE float fastsqrt(float x)
{
  __m128 vx = _mm_set_ss(x);
  return _mm_cvtss_f32(_mm_mul_ss(vx, _mm_rsqrt_ss(vx)));
}
#elif _TARGET_SIMD_NEON
INLINE float fastinvsqrt(float x) { return vget_lane_f32(vrsqrte_f32(vset_lane_f32(x, vdup_n_f32(0.f), 0)), 0); }
INLINE float fastinv(float x) { return vget_lane_f32(vrecpe_f32(vset_lane_f32(x, vdup_n_f32(0.f), 0)), 0); }
INLINE float fastsqrt(float x)
{
  float32x2_t vx = vset_lane_f32(x, vdup_n_f32(0.f), 0);
  return vget_lane_f32(vmul_f32(vx, vrsqrte_f32(vx)), 0);
}
#else
INLINE float fastinvsqrt(float x) { return 1.f / sqrtf(x); }
INLINE float fastinv(float x) { return 1.f / x; }
INLINE float fastsqrt(float x) { return sqrtf(x); }
#endif


// safe versions
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4723) // potential division by zero
#endif
INLINE float safediv(float a, float b) { return b > VERY_SMALL_NUMBER ? a / b : (b < -VERY_SMALL_NUMBER ? a / b : 0.f); }
INLINE double safediv(double a, double b) { return b > VERY_SMALL_NUMBER ? a / b : (b < -VERY_SMALL_NUMBER ? a / b : 0.0); }

INLINE float safeinvsqrtfast(float x) { return x > 0.f ? fastinvsqrt(x) : 1.f; }
INLINE double safeinvsqrtfast(double x) { return x > 0.0 ? fastinvsqrt(x) : 1.0; }

INLINE float safe_asin(float x) { return ::asinf(clamp(x, -1.f, 1.f)); }
INLINE double safe_asin(double x) { return ::asin(clamp(x, -1.0, 1.0)); }

INLINE float safe_acos(float x) { return ::acosf(clamp(x, -1.f, 1.f)); }
INLINE double safe_acos(double x) { return ::acos(clamp(x, -1.0, 1.0)); }

INLINE float unsafe_asin(float x) { return ::asinf(x); }
INLINE double unsafe_asin(double x) { return ::asin(x); }

INLINE float unsafe_acos(float x) { return ::acosf(x); }
INLINE double unsafe_acos(double x) { return ::acos(x); }

INLINE float safe_atan2(float y, float x) { return (x != 0.f || y != 0.f) ? ::atan2f(y, x) : 0.f; }
INLINE double safe_atan2(double y, double x) { return (x != 0.0 || y != 0.0) ? ::atan2(y, x) : 0.f; }

INLINE float unsafe_atan2(float y, float x) { return ::atan2f(y, x); }
INLINE double unsafe_atan2(double y, double x) { return ::atan2(y, x); }

INLINE float safe_sqrt(float x) { return x > 0.f ? ::sqrtf(x) : 0.f; }
INLINE double safe_sqrt(double x) { return x > 0.0 ? ::sqrt(x) : 0.0; }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/// converts float to nearest integer,
/// faster than C conversion that truncates float
INLINE int float2int_near(float x)
{
#if !_TARGET_PC_WIN
  return int(floorf(x + 0.5f));
#else
  const int magic = (150 << 23) | (1 << 22);
  x += bitwise_cast<float, int>(magic);
  return bitwise_cast<int, float>(x) - magic;
#endif
}


INLINE float safeinv(float x) { return safediv(1.0f, x); }
INLINE double safeinv(double x) { return safediv(1.0, x); }

INLINE int real2int(float f)
{
  return float2int_near(f); ///< fixme:probably int(f) is even faster on modern compilers
}


INLINE real qterm(real a) { return (a <= 0) ? 0 : sqrtf(a) * 0.5f; }

// Warning!: Does not perform clipping,if box.center.z<0 always false
bool is_pt_inscreen_box(class Point2 &p, class BBox3 &b, class TMatrix4 &gtm);

extern const float realQNaN;
extern const float realSNaN;

extern const double doubleQNaN;
extern const double doubleSNaN;

void sincos(float rad, float &s, float &c);

// @}

template <typename T>
INLINE T lerp(const T a, const T b, float t)
{
  DisablePointersInMath<T>();
  return (T)(a * (1.0f - t) + b * t);
}

// timestep independent approach function
//
// from - .
//        |.
//        | ..   exp(-dt/visc)
//        |   .
//        |    ..
//        |      ...
//        |         ........
// to ------|--|----------  ..........
//         vis vis*1.44
//
// viscosity specifies time to move from source to destination by 1/2.7 of distance
// Time to get to average between 'from' and 'to' equals to (viscosity*(1/ln(2)) or approximately vis*1.443
// on time = viscosity value equals to 0.37 distance between source and destination

template <typename T>
INLINE T approach(T from, T to, float dt, float viscosity)
{
  DisablePointersInMath<T>();
  if (viscosity < 1e-9f)
    return to;
  else
    return from + (1.0f - expf(-dt / viscosity)) * (to - from);
}

// use approach_vel() for faster convergence of noisy
// values that needed high viscosity for smoothing
template <typename T>
INLINE T approach_vel(T from, T to, float dt, float viscosity, T &vel, float vel_viscosity, float vel_factor)
{
  T newVal = approach(from, to, dt, viscosity);

  if (dt >= 1e-9f)
    vel = approach(vel, (newVal - from) * (1.f / dt), dt, vel_viscosity);

  return newVal + vel * vel_factor * dt;
}

void init_math();

extern const float DECLSPEC_ALIGN(16) math_float_zero[16] ATTRIBUTE_ALIGN(16);

template <typename T>
INLINE const T &ZERO()
{
  static_assert(sizeof(T) <= sizeof(math_float_zero), "T is too big");
  return *reinterpret_cast<const T *>(math_float_zero);
}
template <typename T>
INLINE const T *ZERO_PTR()
{
  return &ZERO<T>();
}

#undef INLINE

/// @}
