// © 2021 NVIDIA Corporation

/*
IMPORTANT:
- intrinsic related headers must not be included *AFTER* ML inclusion
- "ML_NAMESPACE" macro can be defined to wrap the entire ML into "ml" namespace
- sizeof(3-component vector) == sizeof(4-component vector) because of SSE
*/

#pragma once

#define ML_VERSION_MAJOR 1
#define ML_VERSION_MINOR 2
#define ML_VERSION_DATE "7 March 2025"

//======================================================================================================================
// Constants
//======================================================================================================================

// Intrinsic level
#define ML_INTRINSIC_SSE3 0 // +SSSE3
#define ML_INTRINSIC_SSE4 1 // 4.2 and below
#define ML_INTRINSIC_AVX1 2 // +FP16C
#define ML_INTRINSIC_AVX2 3 // +FMA3, +bit shift, +swizzle

//======================================================================================================================
// Settings
//======================================================================================================================

// Can be set to wrap the library into "ml" namespace
#ifndef ML_NAMESPACE
// #define ML_NAMESPACE
#endif

// Allowed HW intrinsics (emulated if not supported)
#ifndef ML_INTRINSIC_LEVEL // see above
#    if defined(__AVX2__)
#        define ML_INTRINSIC_LEVEL ML_INTRINSIC_AVX2
#    elif defined(__AVX__)
#        define ML_INTRINSIC_LEVEL ML_INTRINSIC_AVX1
#    elif defined(__SSE4_2__) || defined(__SSE4_1__)
#        define ML_INTRINSIC_LEVEL ML_INTRINSIC_SSE4
#    else
#        define ML_INTRINSIC_LEVEL ML_INTRINSIC_SSE3
#    endif
#endif

// SVML availability
#ifndef ML_SVML_AVAILABLE
#    if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM)
#        define ML_SVML_AVAILABLE 0
#    else
#        define ML_SVML_AVAILABLE (_MSC_VER >= 1920 && __clang__ == 0)
#    endif
#endif

// More precision (a little bit slower)
#ifndef ML_NEWTONRAPHSON_APROXIMATION
#    define ML_NEWTONRAPHSON_APROXIMATION 1
#endif

// Only for debugging (useful to debug issues in horizontal operations)
#ifndef ML_CHECK_W_IS_ZERO
#    define ML_CHECK_W_IS_ZERO 0
#endif

// Only for debugging (generate exeptions in rounding operations, only for SSE4)
#ifndef ML_EXEPTIONS
#    define ML_EXEPTIONS 0
#endif

// Reversed depth
#ifndef ML_DEPTH_REVERSED
#    define ML_DEPTH_REVERSED 1
#endif

// Can be handy for classic OpenGL
#ifndef ML_OGL
#    define ML_OGL 0
#endif

// Depth range
#ifndef ML_DEPTH_RANGE_NEAR
#    define ML_DEPTH_RANGE_NEAR 0.0f
#endif

#ifndef ML_ML_DEPTH_RANGE_FAR
#    define ML_DEPTH_RANGE_FAR 1.0f
#endif

// Inline preference
#ifndef ML_INLINE
#    if (defined(__GNUC__) || defined(__clang__))
#        define ML_INLINE __attribute__((always_inline)) inline
#    else
#        define ML_INLINE __forceinline
#    endif
#endif

//======================================================================================================================
// Macro stuff
//======================================================================================================================

// Compiler and environment

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstrict-aliasing"

#    define ML_ALIGN(alignment, x) x __attribute__((aligned(alignment)))
#elif defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wstrict-aliasing"

#    define ML_ALIGN(alignment, x) x __attribute__((aligned(alignment)))
#else
#    pragma warning(push)
#    pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union

#    define ML_ALIGN(alignment, x) __declspec(align(alignment)) x
#endif

// Headers

#include <cmath>   // overloaded floor, round, ceil, fmod, sin, cos, tan, asin, acos, atan, atan2, sqrt, pow, log, log2, exp, exp2
#include <cstdlib> // overloaded abs

#include <stdint.h>

#ifndef _WIN32
#    include <unistd.h> // TODO: needed?
#endif

#if (defined(__i386__) || defined(__x86_64__) || defined(__SCE__))
#    include <x86intrin.h>
#elif (defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM))
#    include "sse2neon.h"
#else
#    include <mmintrin.h>
#    if (ML_SVML_AVAILABLE || ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX1)
#        include <immintrin.h> // SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, FMA, SVML
#    elif (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_SSE4)
#        include <nmmintrin.h> // SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
#    else
#        include <tmmintrin.h> // SSE, SSE2, SSE3, SSSE3
#    endif
#endif

// Misc

#define ML_Unused(...) ((void)(__VA_ARGS__))
#define ML_Stringify_(token) #token
#define ML_Stringify(token) ML_Stringify_(token)

#if ML_EXEPTIONS
#    define ML_ROUNDING_EXEPTIONS_MASK _MM_FROUND_RAISE_EXC
#else
#    define ML_ROUNDING_EXEPTIONS_MASK _MM_FROUND_NO_EXC
#endif

// Debugging

#define ML_StaticAssertMsg(x, msg) static_assert(x, msg)

#ifdef _DEBUG
#    include <assert.h> // assert

#    define ML_Assert(x) assert(x)
#    define ML_AssertMsg(x, msg) assert(msg&& x)
#else
#    define ML_Assert(x) ((void)0)
#    define ML_AssertMsg(x, msg) ((void)0)
#endif

// Normalized device coordinates

#if ML_OGL // Depth range [-1; 1], origin "lower left"
#    define ML_NDC_NEAR_NO_REVERSE -1.0f
#    define ML_DEPTH_C0 (0.5f * (ML_DEPTH_RANGE_FAR - ML_DEPTH_RANGE_NEAR))
#    define ML_DEPTH_C1 (0.5f * (ML_DEPTH_RANGE_FAR + ML_DEPTH_RANGE_NEAR))

template <class T>
ML_INLINE T ML_ModifyProjZ(bool isReversed, T c2, T c3) {
    return isReversed ? -c2 : c2;
}

#else // Depth range [0; 1], origin "upper left"
#    define ML_NDC_NEAR_NO_REVERSE 0.0f
#    define ML_DEPTH_C0 (ML_DEPTH_RANGE_FAR - ML_DEPTH_RANGE_NEAR)
#    define ML_DEPTH_C1 ML_DEPTH_RANGE_NEAR

template <class T>
ML_INLINE T ML_ModifyProjZ(bool isReversed, T c2, T c3) {
    return T(0.5) * ((isReversed ? -c2 : c2) + c3);
}

#endif

#define ML_NDC_FAR_NO_REVERSE 1.0f

#if ML_DEPTH_REVERSED
#    define ML_NDC_NEAR ML_NDC_FAR_NO_REVERSE
#    define ML_NDC_FAR ML_NDC_NEAR_NO_REVERSE
#    define ML_DEPTH_EPS -1e-7f
#else
#    define ML_NDC_NEAR ML_NDC_NEAR_NO_REVERSE
#    define ML_NDC_FAR ML_NDC_FAR_NO_REVERSE
#    define ML_DEPTH_EPS 1e-7f
#endif

// TODO

/*
- add some missing HLSL-compatible math functionality
- find a way to improve emulation of intrinsics currently using "for (size_t i = 0;"
- minimize "#ifndef __cplusplus" usage in "ml.hlsli"
- GCC doesn't support "members with constructors in anonymous aggregates"
- search for TODO
*/

//======================================================================================================================
// MathLib
//======================================================================================================================

#ifdef ML_NAMESPACE
namespace ml {
#endif

//======================================================================================================================
// Forward declarations
//======================================================================================================================

struct bool2;
struct bool3;
struct bool4;

union int2;
union int3;
union int4;

typedef uint32_t uint;
union uint2;
union uint3;
union uint4;

union float2;
union float3;
union float4;
union float4x4;

union double2;
union double3;
union double4;
union double4x4;

//======================================================================================================================
// Enums
//======================================================================================================================

enum eStyle : uint8_t {
    STYLE_D3D,
    STYLE_OGL,
};

enum eClip : uint8_t {
    CLIP_OUT,
    CLIP_IN,
    CLIP_PARTIAL,
};

enum eCoordinate : uint32_t {
    COORD_X = 0,
    COORD_Y,
    COORD_Z,
    COORD_W,

    COORD_2D = 2,
    COORD_3D,
    COORD_4D,
};

enum ePlaneType : uint32_t {
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_BOTTOM,
    PLANE_TOP,
    PLANE_NEAR,
    PLANE_FAR,

    PLANES_NUM,
    PLANES_NO_NEAR_FAR = 4,
    PLANES_NO_FAR = 5,

    PLANE_MASK_L = 1 << PLANE_LEFT,
    PLANE_MASK_R = 1 << PLANE_RIGHT,
    PLANE_MASK_B = 1 << PLANE_BOTTOM,
    PLANE_MASK_T = 1 << PLANE_TOP,
    PLANE_MASK_N = 1 << PLANE_NEAR,
    PLANE_MASK_F = 1 << PLANE_FAR,

    PLANE_MASK_NONE = 0,
    PLANE_MASK_LRBT = PLANE_MASK_L | PLANE_MASK_R | PLANE_MASK_B | PLANE_MASK_T,
    PLANE_MASK_NF = PLANE_MASK_N | PLANE_MASK_F,
    PLANE_MASK_LRBTNF = PLANE_MASK_LRBT | PLANE_MASK_NF,
};

enum eProjectionData {
    PROJ_ZNEAR,
    PROJ_ZFAR,
    PROJ_ASPECT,
    PROJ_FOVX,
    PROJ_FOVY,
    PROJ_MINX,
    PROJ_MAXX,
    PROJ_MINY,
    PROJ_MAXY,
    PROJ_DIRX,
    PROJ_DIRY,
    PROJ_ANGLEMINX,
    PROJ_ANGLEMAXX,
    PROJ_ANGLEMINY,
    PROJ_ANGLEMAXY,

    PROJ_NUM,
};

enum eProjectionFlag {
    PROJ_ORTHO = 0x00000001,
    PROJ_REVERSED_Z = 0x00000002,
    PROJ_LEFT_HANDED = 0x00000004,
};

template <class T>
ML_INLINE void Swap(T& x, T& y) {
    T t = x;
    x = y;
    y = t;
}

//======================================================================================================================
// Intrinsic emulation
//======================================================================================================================

#include "Guts/emulation.h"

//======================================================================================================================
// Math
//======================================================================================================================

#include "Guts/math.h"

//======================================================================================================================
// Floating point tricks
//======================================================================================================================

union uFloat {
    float f;
    uint32_t i;

    ML_INLINE uFloat()
        : i(0) {
    }

    ML_INLINE uFloat(float x)
        : f(x) {
    }

    ML_INLINE uFloat(uint32_t x)
        : i(x) {
    }

    ML_INLINE void abs() {
        i &= ~(1 << 31);
    }

    ML_INLINE bool IsNegative() const {
        return (i >> 31) != 0;
    }

    ML_INLINE uint32_t Mantissa() const {
        return i & ((1 << 23) - 1);
    }

    ML_INLINE uint32_t Exponent() const {
        return (i >> 23) & 255;
    }

    ML_INLINE bool IsInf() const {
        return Exponent() == 255 && Mantissa() == 0;
    }

    ML_INLINE bool IsNan() const {
        return Exponent() == 255 && Mantissa() != 0;
    }

    static ML_INLINE float PrecisionGreater(float x) {
        uFloat y(x);
        y.i++;

        return y.f - x;
    }

    static ML_INLINE float PrecisionLess(float x) {
        uFloat y(x);
        y.i--;

        return y.f - x;
    }
};

union uDouble {
    double f;
    uint64_t i;

    ML_INLINE uDouble()
        : i(0) {
    }

    ML_INLINE uDouble(double x)
        : f(x) {
    }

    ML_INLINE uDouble(uint64_t x)
        : i(x) {
    }

    ML_INLINE bool IsNegative() const {
        return (i >> 63) != 0;
    }

    ML_INLINE void abs() {
        i &= ~(1ULL << 63);
    }

    ML_INLINE uint64_t Mantissa() const {
        return i & ((1ULL << 52) - 1);
    }

    ML_INLINE uint64_t Exponent() const {
        return (i >> 52) & 2047;
    }

    ML_INLINE bool IsInf() const {
        return Exponent() == 2047 && Mantissa() == 0;
    }

    ML_INLINE bool IsNan() const {
        return Exponent() == 2047 && Mantissa() != 0;
    }

    static ML_INLINE double PrecisionGreater(double x) {
        uDouble y(x);
        y.i++;

        return y.f - x;
    }

    static ML_INLINE double PrecisionLess(double x) {
        uDouble y(x);
        y.i--;

        return y.f - x;
    }
};

//======================================================================================================================
// Data types
//======================================================================================================================

#define ML_COMPARE_UNOPT(B, C, op) \
    ML_INLINE B operator op(const C& v) const { \
        int32_t mask = x op v.x ? 0x1 : 0; \
        mask |= y op v.y ? 0x2 : 0; \
        return B(mask); \
    }

#define ML_COMPARE(B, C, op, f, movemask, reg) \
    ML_INLINE B operator op(const C& v) const { \
        return B(movemask(f(reg, v.reg))); \
    }

#define ML_OP_UNOPT(C, T, op, opeq) \
    ML_INLINE C operator op(const C& v) const { \
        return C(x op v.x, y op v.y); \
    } \
    ML_INLINE C operator op(T c) const { \
        return C(x op c, y op c); \
    } \
    ML_INLINE friend C operator op(T c, const C& v) { \
        return C(c op v.x, c op v.y); \
    } \
    ML_INLINE void operator opeq(const C& v) { \
        x opeq v.x; \
        y opeq v.y; \
    } \
    ML_INLINE void operator opeq(T c) { \
        x opeq c; \
        y opeq c; \
    }

#define ML_OP(C, T, op, opeq, f, broadcast, reg) \
    ML_INLINE C operator op(const C& v) const { \
        return f(reg, v.reg); \
    } \
    ML_INLINE C operator op(T c) const { \
        return f(reg, broadcast(c)); \
    } \
    ML_INLINE friend C operator op(T c, const C& v) { \
        return f(broadcast(c), v.reg); \
    } \
    ML_INLINE void operator opeq(const C& v) { \
        reg = f(reg, v.reg); \
    } \
    ML_INLINE void operator opeq(T c) { \
        reg = f(reg, broadcast(c)); \
    }

// Vector swizzling
#include "Guts/swizzle.h"

// Boolean (1 bit emulation)
#include "Guts/bool1.h"

// Integer
#include "Guts/i32.h"
#include "Guts/u32.h"

// Float
#include "Guts/f16.h"
#include "Guts/f32.h"
#include "Guts/f64.h"

// Conversion
#include "Guts/conversion.h"

#undef ML_COMPARE_UNOPT
#undef ML_COMPARE
#undef ML_OP_UNOPT
#undef ML_OP

#undef ML_SWIZZLE_2
#undef ML_SWIZZLE_3
#undef ML_SWIZZLE_4

#undef _X
#undef _Y
#undef _Z
#undef _W

//======================================================================================================================
// Misc
//======================================================================================================================

template <class T>
ML_INLINE T CurveSmooth(const T& x) {
    return x * x * (3.0 - 2.0 * x);
}

template <class T>
ML_INLINE T CurveSin(const T& x) {
    return x * (1.0 - x * x / 3.0);
}

template <class T>
ML_INLINE T WaveTriangle(const T& x) {
    return abs(frac(x + T(0.5)) * T(2.0) - T(1.0));
}

template <class T>
ML_INLINE T WaveTriangleSmooth(const T& x) {
    return CurveSmooth(WaveTriangle(x));
}

ML_INLINE float DoubleToGequal(double dValue) {
    float fValue = (float)dValue;
    float fError = (float)(dValue - fValue);

    int32_t exponent = 0;
    frexp(fValue, &exponent);
    exponent = max(exponent, 0);
    exponent = (int32_t)log10f(float(1 << exponent));

    float fStep = 1.0f / pow(10.0f, float(7 - exponent));

    while (fError > 0.0f) {
        fValue += fStep;

        float fCurrError = float(dValue - fValue);

        if (fCurrError == fError)
            fStep += fStep;
        else
            fError = fCurrError;
    }

    return fValue;
}

ML_INLINE float DoubleToLequal(double dValue) {
    float fValue = (float)dValue;
    float fError = (float)(dValue - fValue);

    int32_t exponent = 0;
    frexp(fValue, &exponent);
    exponent = max(exponent, 0);
    exponent = (int32_t)log10f(float(1 << exponent));

    float fStep = 1.0f / pow(10.0f, float(7 - exponent));

    while (fError < 0.0f) {
        fValue -= fStep;

        float fCurrError = float(dValue - fValue);

        if (fCurrError == fError)
            fStep += fStep;
        else
            fError = fCurrError;
    }

    return fValue;
}

//======================================================================================================================
// Rect
//======================================================================================================================

template <class T>
class ctRect {
public:
    union {
        struct {
            T vMin[COORD_2D];
        };

        struct {
            T minx;
            T miny;
        };
    };

    union {
        struct {
            T vMax[COORD_2D];
        };

        struct {
            T maxx;
            T maxy;
        };
    };

public:
    ML_INLINE ctRect() {
        Clear();
    }

    ML_INLINE void Clear() {
        minx = miny = T(1 << 30);
        maxx = maxy = T(-(1 << 30));
    }

    ML_INLINE bool IsValid() const {
        return maxx > minx && maxy > miny;
    }

    ML_INLINE void Add(T px, T py) {
        minx = min(minx, px);
        maxx = max(maxx, px);
        miny = min(miny, py);
        maxy = max(maxy, py);
    }

    ML_INLINE void Add(const T* pPoint2) {
        Add(pPoint2[0], pPoint2[1]);
    }

    ML_INLINE bool IsIntersectWith(const T* pMin, const T* pMax) const {
        ML_Assert(IsValid());

        if (maxx < pMin[0] || maxy < pMin[1] || minx > pMax[0] || miny > pMax[1])
            return false;

        return true;
    }

    ML_INLINE bool IsIntersectWith(const ctRect<T>& rRect) const {
        return IsIntersectWith(rRect.vMin, rRect.vMax);
    }

    ML_INLINE eClip GetIntersectionStateWith(const T* pMin, const T* pMax) const {
        ML_Assert(IsValid());

        if (!IsIntersectWith(pMin, pMax))
            return CLIP_OUT;

        if (minx < pMin[0] && maxx > pMax[0] && miny < pMin[1] && maxy > pMax[1])
            return CLIP_IN;

        return CLIP_PARTIAL;
    }

    ML_INLINE eClip GetIntersectionStateWith(const ctRect<T>& rRect) const {
        return GetIntersectionStateWith(rRect.vMin, rRect.vMax);
    }
};

//======================================================================================================================
// Frustum
//======================================================================================================================

ML_INLINE bool MvpToPlanes(eStyle depthStyle, const float4x4& m, float4* pvPlane6) {
    const float eps = 1e-7f;

    float4x4 mt;
    m.TransposeTo(mt);

    float4 l = mt[3] + mt[0];
    float4 r = mt[3] - mt[0];
    float4 b = mt[3] + mt[1];
    float4 t = mt[3] - mt[1];
    float4 f = mt[3] - mt[2];
    float4 n = mt[2];

    if (depthStyle == STYLE_OGL)
        n += mt[3];

    // Side planes
    l *= rsqrt(dot(l.xyz, l.xyz));
    r *= rsqrt(dot(r.xyz, r.xyz));
    b *= rsqrt(dot(b.xyz, b.xyz));
    t *= rsqrt(dot(t.xyz, t.xyz));

    // Near & far planes
    n /= max(length(n.xyz), eps);
    f /= max(length(f.xyz), eps);

    // Handle reversed projection
    bool bReversed = abs(n.w) > abs(f.w);

    if (bReversed)
        Swap(n, f);

    // Handle infinite projection
    if (length(f.xyz) < eps)
        f = float4(-n.x, -n.y, -n.z, f.w);

    pvPlane6[PLANE_LEFT] = l;
    pvPlane6[PLANE_RIGHT] = r;
    pvPlane6[PLANE_BOTTOM] = b;
    pvPlane6[PLANE_TOP] = t;
    pvPlane6[PLANE_NEAR] = n;
    pvPlane6[PLANE_FAR] = f;

    return bReversed;
}

class cFrustum {
private:
    float4 m_vPlane[PLANES_NUM] = {};
    float4x4 m_mPlanesT = {};
    v4f m_vMask[PLANES_NUM] = {};

public:
    ML_INLINE void Setup(eStyle depthStyle, const float4x4& mMvp) {
        MvpToPlanes(depthStyle, mMvp, m_vPlane);

        m_mPlanesT[0] = m_vPlane[PLANE_LEFT];
        m_mPlanesT[1] = m_vPlane[PLANE_RIGHT];
        m_mPlanesT[2] = m_vPlane[PLANE_BOTTOM];
        m_mPlanesT[3] = m_vPlane[PLANE_TOP];
        m_mPlanesT.Transpose();

        for (uint32_t i = 0; i < PLANES_NUM; i++)
            m_vMask[i] = _mm_cmpgt_ps(m_vPlane[i].xmm, _mm_setzero_ps());
    }

    ML_INLINE void Translate(const float3& vPos) {
        // Update of m_vMask is not required, because only m_vMask.w can be changed, but this component doesn't affect results
        for (uint32_t i = 0; i < PLANES_NUM; i++)
            m_vPlane[i].w = Dot43(m_vPlane[i], vPos);
    }

    ML_INLINE bool CheckSphere(const float3& center, float fRadius, uint32_t planes = PLANES_NUM) const {
        v4f p1 = v4f_setw1(center.xmm);

        for (uint32_t i = 0; i < planes; i++) {
            float d = dot(m_vPlane[i], p1);

            if (d < -fRadius)
                return false;
        }

        return true;
    }

    ML_INLINE bool CheckAabb(const float3& minv, const float3& maxv, uint32_t planes) const {
        v4f min1 = v4f_setw1(minv.xmm);
        v4f max1 = v4f_setw1(maxv.xmm);

        for (uint32_t i = 0; i < planes; i++) {
            v4f v = _mm_blendv_ps(min1, max1, m_vMask[i]);
            v = v4f_dot44(m_vPlane[i].xmm, v);

            if (v4f_isnegative1_all(v))
                return false;
        }

        return true;
    }

    ML_INLINE bool CheckCapsule(const float3& capsule_start, const float3& capsule_axis, float capsule_radius, uint32_t planes) const {
        // https://github.com/toxygen/STA/blob/master/celestia-src/celmath/frustum.cpp

        float r2 = capsule_radius * capsule_radius;
        float3 capsule_end = capsule_start + capsule_axis;

        for (uint32_t i = 0; i < planes; i++) {
            float signedDist0 = Dot43(m_vPlane[i], capsule_start);
            float signedDist1 = Dot43(m_vPlane[i], capsule_end);

            if (signedDist0 * signedDist1 > r2) {
                if (abs(signedDist0) <= abs(signedDist1)) {
                    if (signedDist0 < -capsule_radius)
                        return false;
                } else {
                    if (signedDist1 < -capsule_radius)
                        return false;
                }
            }
        }

        return true;
    }

    ML_INLINE bool CheckSphere_mask(const float3& center, float fRadius, uint32_t mask, uint32_t planes) const {
        v4f p1 = v4f_setw1(center.xmm);

        for (uint32_t i = 0; i < planes; i++) {
            if (!(mask & (1 << i))) {
                float d = dot(m_vPlane[i], p1);

                if (d < -fRadius)
                    return false;
            }
        }

        return true;
    }

    ML_INLINE bool CheckAabb_mask(const float3& minv, const float3& maxv, uint32_t mask, uint32_t planes) const {
        v4f min1 = v4f_setw1(minv.xmm);
        v4f max1 = v4f_setw1(maxv.xmm);

        for (uint32_t i = 0; i < planes; i++) {
            if (!(mask & (1 << i))) {
                v4f v = _mm_blendv_ps(min1, max1, m_vMask[i]);
                v = v4f_dot44(m_vPlane[i].xmm, v);

                if (v4f_isnegative1_all(v))
                    return false;
            }
        }

        return true;
    }

    ML_INLINE eClip CheckSphere_state(const float3& center, float fRadius, uint32_t planes) const {
        v4f p1 = v4f_setw1(center.xmm);

        eClip clip = CLIP_IN;

        for (uint32_t i = 0; i < planes; i++) {
            float d = dot(m_vPlane[i], p1);

            if (d < -fRadius)
                return CLIP_OUT;

            if (d < fRadius)
                clip = CLIP_PARTIAL;
        }

        return clip;
    }

    ML_INLINE eClip CheckAabb_state(const float3& minv, const float3& maxv, uint32_t planes) const {
        v4f min1 = v4f_setw1(minv.xmm);
        v4f max1 = v4f_setw1(maxv.xmm);

        eClip clip = CLIP_IN;

        for (uint32_t i = 0; i < planes; i++) {
            v4f v = _mm_blendv_ps(min1, max1, m_vMask[i]);
            v = v4f_dot44(m_vPlane[i].xmm, v);

            if (v4f_isnegative1_all(v))
                return CLIP_OUT;

            v = _mm_blendv_ps(max1, min1, m_vMask[i]);
            v = v4f_dot44(m_vPlane[i].xmm, v);

            if (v4f_isnegative1_all(v))
                clip = CLIP_PARTIAL;
        }

        return clip;
    }

    ML_INLINE eClip CheckCapsule_state(const float3& capsule_start, const float3& capsule_axis, float capsule_radius, uint32_t planes) const {
        float r2 = capsule_radius * capsule_radius;
        float3 capsule_end = capsule_start + capsule_axis;

        uint32_t intersections = 0;

        for (uint32_t i = 0; i < planes; i++) {
            float signedDist0 = Dot43(m_vPlane[i], capsule_start);
            float signedDist1 = Dot43(m_vPlane[i], capsule_end);

            if (signedDist0 * signedDist1 > r2) {
                // Endpoints of capsule are on same side of plane. Test closest endpoint to see if it lies closer to the plane than radius
                if (abs(signedDist0) <= abs(signedDist1)) {
                    if (signedDist0 < -capsule_radius)
                        return CLIP_OUT;
                    else if (signedDist0 < capsule_radius)
                        intersections |= (1 << i);
                } else {
                    if (signedDist1 < -capsule_radius)
                        return CLIP_OUT;
                    else if (signedDist1 < capsule_radius)
                        intersections |= (1 << i);
                }
            } else {
                // Capsule endpoints are on different sides of the plane, so we have an intersection
                intersections |= (1 << i);
            }
        }

        return !intersections ? CLIP_IN : CLIP_PARTIAL;
    }

    ML_INLINE eClip CheckSphere_mask_state(const float3& center, float fRadius, uint32_t& mask, uint32_t planes) const {
        v4f p1 = v4f_setw1(center.xmm);

        eClip clip = CLIP_IN;

        for (uint32_t i = 0; i < planes; i++) {
            if (!(mask & (1 << i))) {
                float d = dot(m_vPlane[i], p1);

                if (d < -fRadius)
                    return CLIP_OUT;

                if (d < fRadius)
                    clip = CLIP_PARTIAL;
                else
                    mask |= 1 << i;
            }
        }

        return clip;
    }

    ML_INLINE eClip CheckAabb_mask_state(const float3& minv, const float3& maxv, uint32_t& mask, uint32_t planes) const {
        v4f min1 = v4f_setw1(minv.xmm);
        v4f max1 = v4f_setw1(maxv.xmm);

        eClip result = CLIP_IN;

        for (uint32_t i = 0; i < planes; i++) {
            if (!(mask & (1 << i))) {
                v4f v = _mm_blendv_ps(min1, max1, m_vMask[i]);
                v = v4f_dot44(m_vPlane[i].xmm, v);

                if (v4f_isnegative1_all(v))
                    return CLIP_OUT;

                v = _mm_blendv_ps(max1, min1, m_vMask[i]);
                v = v4f_dot44(m_vPlane[i].xmm, v);

                if (v4f_isnegative1_all(v))
                    result = CLIP_PARTIAL;
                else
                    mask |= 1 << i;
            }
        }

        return result;
    }

    ML_INLINE void SetNearFar(float zNearNeg, float zFarNeg) {
        m_vPlane[PLANE_NEAR].w = zNearNeg;
        m_vPlane[PLANE_FAR].w = -zFarNeg;
    }

    ML_INLINE void SetFar(float zFarNeg) {
        m_vPlane[PLANE_FAR].w = -zFarNeg;
    }

    ML_INLINE const float4& GetPlane(uint32_t plane) {
        ML_Assert(plane < PLANES_NUM);

        return m_vPlane[plane];
    }
};

ML_INLINE void DecomposeProjection(eStyle originStyle, eStyle depthStyle, const float4x4& proj, uint32_t* puiFlags, float* pfSettings15, float* pfUnproject2, float* pfFrustum4,
    float* pfProject3, float* pfSafeNearZ) {
    float4 vPlane[PLANES_NUM];
    bool bReversedZ = MvpToPlanes(depthStyle, proj, vPlane);

    bool bIsOrtho = proj.a33 == 1.0f ? true : false;

    float fNearZ = -vPlane[PLANE_NEAR].w;
    float fFarZ = vPlane[PLANE_FAR].w;

    float x0, x1, y0, y1;
    if (bIsOrtho) {
        x0 = -vPlane[PLANE_LEFT].w;
        x1 = vPlane[PLANE_RIGHT].w;
        y0 = -vPlane[PLANE_BOTTOM].w;
        y1 = vPlane[PLANE_TOP].w;

        if (proj.a11 < 0.0f)
            Swap(y0, y1);
    } else {
        x0 = vPlane[PLANE_LEFT].z / vPlane[PLANE_LEFT].x;
        x1 = vPlane[PLANE_RIGHT].z / vPlane[PLANE_RIGHT].x;
        y0 = vPlane[PLANE_BOTTOM].z / vPlane[PLANE_BOTTOM].y;
        y1 = vPlane[PLANE_TOP].z / vPlane[PLANE_TOP].y;
    }

    // const float3& col2 = bReversedZ ? proj.col3 : proj.col2;
    float4 clip = proj * float4(0.0f, 0.0f, fNearZ, 1.0f);
    float3 col2 = bIsOrtho ? float3(proj.col2) * (bReversedZ ? -1.0f : 1.0f) : float3(0.0f, 0.0f, clip.w > 0.0f ? 1.0f : -1.0f);
    bool cmp = dot(cross(float3(proj.col0), float3(proj.col1)), col2.xyz) > 0.0f;
    bool bLeftHanded = proj.a11 > 0.0f ? cmp : !cmp;

    if (puiFlags) {
        *puiFlags = bIsOrtho ? PROJ_ORTHO : 0;
        *puiFlags |= bReversedZ ? PROJ_REVERSED_Z : 0;
        *puiFlags |= bLeftHanded ? PROJ_LEFT_HANDED : 0;
    }

    if (pfUnproject2) {
        // z = u0 / (depth + u1)

        pfUnproject2[0] = ML_DEPTH_C0 * proj.a23 / proj.a32;
        pfUnproject2[1] = -(ML_DEPTH_C0 * proj.a22 / proj.a32 + ML_DEPTH_C1);

        // z = 1 / (depth * u0 + u1);

        // pfUnproject2[0] = proj.a32 / (ML_DEPTH_C0 * proj.a23);
        // pfUnproject2[1] = -(proj.a22 / proj.a23 + ML_DEPTH_C1 / pfUnproject2[0]);
    }

    if (pfSafeNearZ) {
        *pfSafeNearZ = fNearZ - ML_DEPTH_EPS;

        if (!bIsOrtho) {
            float maxx = max(abs(x0), abs(x1));
            float maxy = max(abs(y0), abs(y1));

            *pfSafeNearZ *= sqrt(maxx * maxx + maxy * maxy + 1.0f);
        }
    }

    if (pfProject3) {
        // IMPORTANT: Rg - geometry radius, Rp - projected radius, Rn - projected normalized radius
        //      keep in mind:
        //          zp = -(mView * p).z
        //          zp_fix = mix(zp, 1.0, bIsOrtho), or
        //          zp_fix = (mViewProj * p).w
        //      project:
        //          Rn.x = Rg * pfProject3[0] / zp_fix
        //          Rn.y = Rg * pfProject3[1] / zp_fix
        //          Rp = 0.5 * viewport.w * Rn.x, or
        //          Rp = 0.5 * viewport.h * Rn.y, or
        //          Rp = Rg * K / zp_fix
        //      unproject:
        //          Rn.x = 2.0 * Rp / viewport.w
        //          Rn.y = 2.0 * Rp / viewport.h
        //          Rg = Rn.x * zp_fix / pfProject3[0], or
        //          Rg = Rn.y * zp_fix / pfProject3[1], or
        //          Rg = Rp * zp_fix / K
        //      K = 0.5 * viewport.w * pfProject3[0] = 0.5 * viewport.h * pfProject3[1]

        float fProjectx = 2.0f / (x1 - x0);
        float fProjecty = 2.0f / (y1 - y0);

        pfProject3[0] = abs(fProjectx);
        pfProject3[1] = abs(fProjecty);
        pfProject3[2] = bIsOrtho ? 1.0f : 0.0f;
    }

    if (pfFrustum4) {
        // IMPORTANT: view space position from screen space uv [0, 1]
        //          ray.xy = (pfFrustum4.zw * uv + pfFrustum4.xy) * mix(zDistanceNeg, -1.0, bIsOrtho)
        //          ray.z = 1.0 * zDistanceNeg

        pfFrustum4[0] = -x0;
        pfFrustum4[2] = x0 - x1;

        if (originStyle == STYLE_D3D) {
            pfFrustum4[1] = -y1;
            pfFrustum4[3] = y1 - y0;
        } else {
            pfFrustum4[1] = -y0;
            pfFrustum4[3] = y0 - y1;
        }
    }

    if (pfSettings15) {
        // Swap is possible, because it is the last pass...
        if (x0 > x1)
            Swap(x0, x1);

        if (y0 > y1)
            Swap(y0, y1);

        float fAngleY0 = atan(bIsOrtho ? 0.0f : y0);
        float fAngleY1 = atan(bIsOrtho ? 0.0f : y1);
        float fAngleX0 = atan(bIsOrtho ? 0.0f : x0);
        float fAngleX1 = atan(bIsOrtho ? 0.0f : x1);

        float fAspect = (x1 - x0) / (y1 - y0);

        pfSettings15[PROJ_ZNEAR] = fNearZ;
        pfSettings15[PROJ_ZFAR] = fFarZ;
        pfSettings15[PROJ_ASPECT] = fAspect;
        pfSettings15[PROJ_FOVX] = fAngleX1 - fAngleX0;
        pfSettings15[PROJ_FOVY] = fAngleY1 - fAngleY0;
        pfSettings15[PROJ_MINX] = x0 * fNearZ;
        pfSettings15[PROJ_MAXX] = x1 * fNearZ;
        pfSettings15[PROJ_MINY] = y0 * fNearZ;
        pfSettings15[PROJ_MAXY] = y1 * fNearZ;
        pfSettings15[PROJ_ANGLEMINX] = fAngleX0;
        pfSettings15[PROJ_ANGLEMAXX] = fAngleX1;
        pfSettings15[PROJ_ANGLEMINY] = fAngleY0;
        pfSettings15[PROJ_ANGLEMAXY] = fAngleY1;
        pfSettings15[PROJ_DIRX] = (fAngleX0 + fAngleX1) * 0.5f;
        pfSettings15[PROJ_DIRY] = (fAngleY0 + fAngleY1) * 0.5f;
    }
}

#include "Guts/other.h"
#include "Guts/packing.h"
#include "Guts/sorting.h"

#ifdef ML_NAMESPACE
}
#endif

//======================================================================================================================
// End
//======================================================================================================================

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#else
#    pragma warning(pop)
#endif
