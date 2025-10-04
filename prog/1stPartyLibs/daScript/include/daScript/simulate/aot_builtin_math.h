#pragma once

#include <dag_noise/dag_uint_noise.h>
#if (__clang_major__ < 12 || (__clang_major__ >= 17 && __clang_major__ <= 18)) && defined(__clang__) && defined(__FAST_MATH__)
#include <cstring> // memcpy
#include <cmath> // INFINITY
#endif

namespace das {
    __forceinline unsigned int uint_noise2D_int2(das::int2 pos, unsigned int seed)
    {
        return uint_noise2D(pos.x, pos.y, seed);
    }
    __forceinline unsigned int uint_noise3D_int3(das::int3 pos, unsigned int seed)
    {
        return uint_noise3D(pos.x, pos.y, pos.z, seed);
    }

    __forceinline float length2(vec4f a){return v_extract_x(v_length2_x(a));}
    __forceinline float length3(vec4f a){return v_extract_x(v_length3_x(a));}
    __forceinline float length4(vec4f a){return v_extract_x(v_length4_x(a));}
    __forceinline float lengthSq2(vec4f a){return v_extract_x(v_length2_sq_x(a));}
    __forceinline float lengthSq3(vec4f a){return v_extract_x(v_length3_sq_x(a));}
    __forceinline float lengthSq4(vec4f a){return v_extract_x(v_length4_sq_x(a));}

#if defined(_MSC_VER) && !defined(__clang__) && (_MSC_VER==1923)
    // workaround for internal compiler error with MSVC 2019
    __forceinline float invlength2(vec4f a) { return 1.0f / length2(a); }
    __forceinline float invlength3(vec4f a) { return 1.0f / length3(a); }
    __forceinline float invlength4(vec4f a) { return 1.0f / length4(a); }
#else
    __forceinline float invlength2(vec4f a){return v_extract_x(v_rsqrt_x(v_length2_sq_x(a)));}
    __forceinline float invlength3(vec4f a){return v_extract_x(v_rsqrt_x(v_length3_sq_x(a)));}
    __forceinline float invlength4(vec4f a){return v_extract_x(v_rsqrt_x(v_length4_sq_x(a)));}
#endif

    __forceinline float invlengthSq2(vec4f a){return v_extract_x(v_rcp_x(v_length3_sq_x(a)));}
    __forceinline float invlengthSq3(vec4f a){return v_extract_x(v_rcp_x(v_length3_sq_x(a)));}
    __forceinline float invlengthSq4(vec4f a){return v_extract_x(v_rcp_x(v_length4_sq_x(a)));}

    __forceinline float distance2     (vec4f a, vec4f b){return length2(v_sub(a,b));}
    __forceinline float invdistance2  (vec4f a, vec4f b){return invlength2(v_sub(a,b));}
    __forceinline float distanceSq2   (vec4f a, vec4f b){return lengthSq2(v_sub(a,b));}
    __forceinline float invdistanceSq2(vec4f a, vec4f b){return invlengthSq2(v_sub(a,b));}

    __forceinline float distance3     (vec4f a, vec4f b){return v_extract_x(v_length3_x(v_sub(a, b)));}
    __forceinline float invdistance3  (vec4f a, vec4f b){return v_extract_x(v_rcp_x(v_length3_x(v_sub(a, b))));}
    __forceinline float distanceSq3   (vec4f a, vec4f b){return v_extract_x(v_length3_sq_x(v_sub(a, b)));}
    __forceinline float invdistanceSq3(vec4f a, vec4f b){return v_extract_x(v_rcp_x(v_length3_sq_x(v_sub(a,b))));}

    __forceinline float distance4     (vec4f a, vec4f b){return v_extract_x(v_length4_x(v_sub(a, b)));}
    __forceinline float invdistance4  (vec4f a, vec4f b){return v_extract_x(v_rcp_x(v_length4_x(v_sub(a, b))));}
    __forceinline float distanceSq4   (vec4f a, vec4f b){return v_extract_x(v_length4_sq_x(v_sub(a, b)));}
    __forceinline float invdistanceSq4(vec4f a, vec4f b){return v_extract_x(v_rcp_x(v_length4_sq_x(v_sub(a,b))));}

    __forceinline float dot2(vec4f a, vec4f b){return v_extract_x(v_dot2_x(a, b));}
    __forceinline float dot3(vec4f a, vec4f b){return v_extract_x(v_dot3_x(a, b));}
    __forceinline float dot4(vec4f a, vec4f b){return v_extract_x(v_dot4_x(a, b));}

    __forceinline vec4f normalize2(vec4f a){return v_norm2(a); }
    __forceinline vec4f normalize3(vec4f a){return v_norm3(a); }
    __forceinline vec4f normalize4(vec4f a){return v_norm4(a); }
    __forceinline vec4f safe_normalize2(vec4f a){return v_norm2_safe(a, v_zero());}
    __forceinline vec4f safe_normalize3(vec4f a){return v_norm3_safe(a, v_zero());}
    __forceinline vec4f safe_normalize4(vec4f a){return v_norm4_safe(a, v_zero());}

    __forceinline vec4f cross3(vec4f a, vec4f b){vec4f v = v_cross3(a,b); return v;}

    __forceinline vec4f lerp_vec_float(vec4f a, vec4f b, float t) { return v_madd(v_sub(b, a), v_splats(t), a); }

#if defined(__GNUC__) || defined(__clang__)
#if !defined(__clang__)
#define DAS_FINITE_MATH __attribute__((optimize("no-finite-math-only")))
#else
#define DAS_FINITE_MATH
#endif
#if defined(__clang__) && !defined(__arm64__) && !defined(__e2k__) && !defined(_TARGET_C3)
#pragma float_control(push)
#pragma float_control(precise, on)
#endif
#if (__clang_major__ < 12 || (__clang_major__ >= 17 && __clang_major__ <= 18)) && defined(__clang__) && defined(__FAST_MATH__)
    //unfortunately older clang versions do not work with float_control, and in clang 17-18.1 it's broken
    __forceinline DAS_FINITE_MATH bool   fisnan(float  a) { volatile float b = a; return b != a; }
    __forceinline DAS_FINITE_MATH bool   disnan(double  a) { volatile double b = a; return b != a; }
    __forceinline DAS_FINITE_MATH bool   fisfinite(float a)
    {
      uint32_t i;
      memcpy(&i, &a, sizeof(a));
      i &= ~(1 << 31);
      memcpy(&a, &i, sizeof(a));
      static volatile float inf = INFINITY;
      return a != inf;
    }
    __forceinline DAS_FINITE_MATH bool   disfinite(double a)
    {
      uint64_t i;
      memcpy(&i, &a, sizeof(a));
      i &= ~(1ull << 63);
      memcpy(&a, &i, sizeof(a));
      static volatile double inf = INFINITY;
      return a != inf;
    }
#else
    ___noinline DAS_FINITE_MATH inline bool   fisnan(float  a) { return __builtin_isnan(a); }
    ___noinline DAS_FINITE_MATH inline bool   disnan(double  a) { return __builtin_isnan(a); }
    ___noinline DAS_FINITE_MATH inline bool   fisfinite(float  a) { return __builtin_isfinite(a); }
    ___noinline DAS_FINITE_MATH inline bool   disfinite(double  a) { return __builtin_isfinite(a); }
#endif
#undef DAS_FINITE_MATH
#if defined(__clang__) && !defined(__arm64__) && !defined(__e2k__) && !defined(_TARGET_C3)
#pragma float_control(pop)
#endif
#else
    //msvc just does not optimize fast math
    __forceinline bool   fisfinite(float  a) { return isfinite(a); }
    __forceinline bool   fisnan(float  a) { return isnan(a); }
    __forceinline bool   disfinite(double  a) { return isfinite(a); }
    __forceinline bool   disnan(double  a) { return isnan(a); }
#endif


    __forceinline double dsign (double a){return (a == 0) ? 0 : (a > 0) ? 1 : -1;}
    __forceinline double dabs  (double a){return fabs(a);}
    __forceinline double dsqrt (double a){return sqrt(a);}
    __forceinline double dexp  (double a){return exp(a);}
    __forceinline double drcp  (double a){return 1.0 / a;}
    __forceinline double dlog  (double a){return log(a);}
    __forceinline double dpow  (double a, double b){return pow(a,b);}
    __forceinline double dexp2 (double a){return exp2(a);}
    __forceinline double dlog2 (double a){return log2(a);}
    __forceinline double dsin  (double a){return sin(a);}
    __forceinline double dcos  (double a){return cos(a);}
    __forceinline double dasin (double a){return asin(a);}
    __forceinline double dacos (double a){return acos(a);}
    __forceinline double dtan  (double a){return tan(a);}
    __forceinline double datan (double a){return atan(a);}
    __forceinline double datan2(double a,double b){return atan2(a,b);}

    __forceinline float fasin (float a){return asin(a);}
    __forceinline float facos (float a){return acos(a);}
    __forceinline float fatan (float a){return atan(a);}
    __forceinline float fatan_est (float a){return v_extract_x(v_atan_est_x(v_set_x(a)));}
    __forceinline float fatan2(float a,float b){return atan2(a,b);}
    __forceinline float fatan2_est(float a,float b){return v_extract_x(v_atan2_est_x(v_set_x(a), v_set_x(b)));}

    __forceinline vec4f vasin(vec4f a){return v_asin(a);}
    __forceinline vec4f vacos(vec4f a){return v_acos(a);}
    __forceinline vec4f vatan(vec4f a){return v_atan(a);}
    __forceinline vec4f vatan_est(vec4f a){return v_atan_est(a);}
    __forceinline vec4f vatan2(vec4f a, vec4f b){return v_atan2(a,b);}
    __forceinline vec4f vatan2_est(vec4f a, vec4f b){return v_atan2_est(a,b);}

    __forceinline void sincosF ( float a, float & sv, float & cv ) {
        vec4f s,c;
        v_sincos_x(v_set_x(a), s, c);
        sv = v_extract_x(s);
        cv = v_extract_x(c);
    }

    __forceinline void sincosD ( double a, double & sv, double & cv ) {
        sv = sin(a);
        cv = cos(a);
    }

    // def reflect(v,n:float3)
    //  return v - float(2.) * dot(v, n) * n
    __forceinline vec4f reflect ( vec4f v, vec4f n ) {
        vec4f t = v_mul(v_dot3(v,n),n);
        return v_sub(v, v_add(t,t));
    }

    __forceinline vec4f reflect2 ( vec4f v, vec4f n ) {
        vec4f t = v_mul(v_dot2(v,n),n);
        return v_sub(v, v_add(t,t));
    }


    // def refract(v,n:float3;nint:float;outRefracted:float3&)
    // let dt = dot(v,n)
    // let discr = 1. - nint*nint*(1.-dt*dt)
    // if discr > 0.
    //     outRefracted = nint*(v-n*dt)-n*sqrt(discr)
    //     return true
    // return false
    __forceinline vec4f refract(vec4f v, vec4f n, float nint) {
        vec4f dtv = v_dot3(v, n);
        float dt = v_extract_x(dtv);
        float discr = 1.0f - nint*nint*(1.0f - dt*dt);
        if (discr >= 0.0f) {
            vec4f nintv = v_splats(nint);
            vec4f sqrt_discr = v_perm_xxxx(v_sqrt_x(v_set_x(discr)));
            return v_sub(v_mul(nintv, v_sub(v, v_mul(n, dtv))), v_mul(n, sqrt_discr));
        } else {
            return v_zero();
        }
    }

    __forceinline vec4f refract2(vec4f v, vec4f n, float nint) {
        vec4f dtv = v_dot2(v, n);
        float dt = v_extract_x(dtv);
        float discr = 1.0f - nint*nint*(1.0f - dt*dt);
        if (discr >= 0.0f) {
            vec4f nintv = v_splats(nint);
            vec4f sqrt_discr = v_perm_xxxx(v_sqrt_x(v_set_x(discr)));
            return v_sub(v_mul(nintv, v_sub(v, v_mul(n, dtv))), v_mul(n, sqrt_discr));
        } else {
            return v_zero();
        }
    }

    __forceinline uint32_t pack_float_to_byte ( float4 value ) {
        return v_float_to_byte(value);
    }

    __forceinline float4 unpack_byte_to_float ( uint32_t value ) {
        return v_byte_to_float(value);
    }
}
