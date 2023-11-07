#pragma once

#include "daScript/misc/vectypes.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/rangetype.h"

namespace das
{
    template <typename TT> struct WrapType { enum { value = false }; typedef TT type; };
    template <typename TT> struct WrapArgType { typedef TT type; };
    template <typename TT> struct WrapRetType { typedef TT type; };

    template <typename TT>
    struct das_alias;

    template <typename TT, typename PT>
    struct prune {
        static __forceinline TT from(const PT & v) {
            static_assert(sizeof(TT) <= sizeof(PT), "type too big to be pruned");
#if defined(_MSC_VER) && !defined(__clang__)
            return *(TT *)(&v);
#else
            TT r;
            memcpy(&r, &v, sizeof(TT));
            return r;
#endif
        }
    };

    template <typename PT, typename VT>
    struct das_alias_ref {
        static __forceinline VT & from ( PT & value ) {
            return *((VT *)&value);
        }
        static __forceinline const VT & from ( const PT & value ) {
            return *((const VT *)&value);
        }
        static __forceinline VT & from ( VT & value ) {
            return value;
        }
        static __forceinline VT from ( const VT & value ) {
            return value;
        }
        static __forceinline PT & to ( VT & value ) {
            return *((PT *)&value);
        }
        static __forceinline const PT & to ( const VT & value ) {
            return *((const PT *)&value);
        }
        static __forceinline PT & to ( PT & value ) {
            return value;
        }
        static __forceinline PT to ( const PT & value ) {
            return value;
        }
    };

    template <typename PT, typename VT>
    struct das_alias_vec : das_alias_ref<PT,VT> {
        static __forceinline VT & from ( PT & value ) {
            return *((VT *)&value);
        }
        static __forceinline VT * from ( PT * value ) {
            return (VT *) value;
        }
        static __forceinline const VT & from ( const PT & value ) {
            return *((const VT *)&value);
        }
        static __forceinline const VT * from ( const PT * value ) {
            return (const VT *)value;
        }
        static __forceinline VT & from ( VT & value ) {
            return value;
        }
        static __forceinline const VT & from ( const VT & value ) {
            return value;
        }
        static __forceinline PT & to ( VT & value ) {
            return *((PT *)&value);
        }
        static __forceinline const PT & to ( const VT & value ) {
            return *((const PT *)&value);
        }
        static __forceinline PT & to ( PT & value ) {
            return value;
        }
        static __forceinline PT to ( const PT & value ) {
            return value;
        }
        static __forceinline PT & to ( vec4f & value ) {
            return *((PT *)&value);
        }
        static __forceinline PT to ( vec4f value ) {
            return prune<PT,vec4f>::from(value);
        }
    };

    template <typename TT>
    struct cast;

    template <typename TT>
    struct has_cast {
    private:
        static int detect(...);
        template<typename U>
        static decltype(cast<U>::from(declval<U>())) detect(const U&);
    public:
        enum { value = is_same<vec4f, decltype(detect(declval<TT>()))>::value };
    };

    template <typename TT>
    struct cast <const TT> : cast<TT> {};

    template <typename TT>
    __forceinline vec4f cast_result ( TT arg ) {
        return cast<TT>::from(arg);
    }

    template <typename TT>
    struct cast <TT *> {
        static __forceinline TT * to ( vec4f a )               { return (TT *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const TT * p )       { return v_cast_vec4f(v_ldu_ptr((const void *)p)); }
    };

    template <typename TT>
    struct cast <TT &> {
        static __forceinline TT & to ( vec4f a )               { return *(TT *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const TT & p )       { return v_cast_vec4f(v_ldu_ptr((const void *)&p)); }
    };

    template <typename TT>
    struct cast <const TT *> {
        static __forceinline const TT * to ( vec4f a )         { return (const TT *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const TT * p )       { return v_cast_vec4f(v_ldu_ptr((const void *)p)); }
    };

    template <typename TT>
    struct cast <const TT &> {
        static __forceinline const TT & to ( vec4f a )         { return *(const TT *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const TT & p )       { return v_cast_vec4f(v_ldu_ptr((const void *)&p)); }
    };

    template <typename T>
    struct cast <smart_ptr<T>> {
        static __forceinline smart_ptr<T> to ( vec4f a )               { return (T *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const smart_ptr<T> p )       { return v_cast_vec4f(v_ldu_ptr((const void *)p.get())); }
    };

    template <typename T>
    struct cast <smart_ptr_raw<T>> {
        static __forceinline smart_ptr_raw<T> to ( vec4f a )           { smart_ptr_raw<T> p; p.ptr = (T *) v_extract_ptr(v_cast_vec4i((a))); return p; }
        static __forceinline vec4f from ( const smart_ptr_raw<T> p )   { return v_cast_vec4f(v_ldu_ptr((const void *)p.get())); }
    };

    template <>
    struct cast <vec4f> {
        static __forceinline vec4f to ( vec4f x )              { return x; }
        static __forceinline vec4f from ( vec4f x )            { return x; }
    };

    template <>
    struct cast <bool> {
        static __forceinline bool to ( vec4f x )               { return (bool) v_extract_xi(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( bool x )             { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <int8_t> {
        static __forceinline int8_t to ( vec4f x )             { return int8_t(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( int8_t x )           { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <uint8_t> {
        static __forceinline uint8_t to ( vec4f x )            { return uint8_t(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( uint8_t x )          { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <int16_t> {
        static __forceinline int16_t to ( vec4f x )            { return int16_t(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( int16_t x )          { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <uint16_t> {
        static __forceinline uint16_t to ( vec4f x )           { return uint16_t(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( uint16_t x )         { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <int32_t> {
        static __forceinline int32_t to ( vec4f x )            { return v_extract_xi(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( int32_t x )          { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <uint32_t> {
        static __forceinline uint32_t to ( vec4f x )           { return v_extract_xi(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( uint32_t x )         { return v_cast_vec4f(v_seti_x(x)); }
    };

    template <>
    struct cast <Bitfield> {
        static __forceinline Bitfield to ( vec4f x )           { return v_extract_xi(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( Bitfield x )         { return v_cast_vec4f(v_seti_x(x.value)); }
    };


    template <>
    struct cast <int64_t> {
        static __forceinline int64_t to ( vec4f x )            { return v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( int64_t x )          { return v_cast_vec4f(v_ldui_half(&x)); }
    };

    template <>
    struct cast <char> {
        static __forceinline char to ( vec4f x )            { return char(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( char x )          { return v_cast_vec4f(v_seti_x(x)); }
    };


#if defined(_MSC_VER)

    template <>
    struct cast <long> {
        static __forceinline long to ( vec4f x )               { return (long) v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( long x )             { return v_cast_vec4f(v_ldui_half(&x)); }
    };

    template <>
    struct cast <unsigned long> {
        static __forceinline unsigned long to ( vec4f x )      { return (unsigned long) v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( unsigned long x )    { return v_cast_vec4f(v_ldui_half(&x)); }
    };

    template <>
    struct cast <long double> {
        static __forceinline long double to(vec4f x)            { union { vec4f v; long double t; } A; A.v = x; return A.t; }
        static __forceinline vec4f from ( long double x )       { union { vec4f v; long double t; } A; A.t = x; return A.v; }
    };

    template <>
    struct cast <wchar_t> {
        static __forceinline wchar_t to ( vec4f x )           { return wchar_t(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( wchar_t x )         { return v_cast_vec4f(v_seti_x(x)); }
    };
#endif

#if defined(__APPLE__)
    #if __LP64__
        template <>
        struct cast <size_t> {
            static __forceinline size_t to ( vec4f x )           { return v_extract_xi64(v_cast_vec4i(x)); }
            static __forceinline vec4f from ( size_t x )         { return v_cast_vec4f(v_ldui_half(&x)); }
        };
    #else
        template <>
        struct cast <size_t> {
            static __forceinline size_t to ( vec4f x )           { return v_extract_xi(v_cast_vec4i(x)); }
            static __forceinline vec4f from ( size_t x )         { return v_cast_vec4f(v_seti_x(x)); }
        };
    #endif
#endif

    template <>
    struct cast <uint64_t> {
        static __forceinline uint64_t to ( vec4f x )           { return v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( uint64_t x )         { return v_cast_vec4f(v_ldui_half(&x)); }
    };

#if defined(__linux__) || defined __HAIKU__
    template <>
    struct cast <long long int> {
        static __forceinline long long int to ( vec4f x )            { return v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( long long int x )          { return v_cast_vec4f(v_ldui_half(&x)); }
    };
    template <>
    struct cast <unsigned long long int> {
        static __forceinline unsigned long long int to ( vec4f x )           { return v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( unsigned long long int x )         { return v_cast_vec4f(v_ldui_half(&x)); }
    };
#endif

#ifdef _EMSCRIPTEN_VER

    template <>
    struct cast <unsigned long> {
        static __forceinline unsigned long to ( vec4f x )      { return (unsigned long) v_extract_xi64(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( unsigned long x )    { return v_cast_vec4f(v_ldui_half(&x)); }
    };

#endif

    template <>
    struct cast <float> {
        static __forceinline float to ( vec4f x )              { return v_extract_x(x); }
        static __forceinline vec4f from ( float x )            { return v_set_x(x); }
    };

    template <>
    struct cast <double> {
        static __forceinline double to(vec4f x)                { union { vec4f v; double t; } A; A.v = x; return A.t; }
        static __forceinline vec4f from ( double x )           { union { vec4f v; double t; } A; A.t = x; return A.v; }
    };

    template <>
    struct cast <Func> {
        static __forceinline Func to ( vec4f x )             { return Func(cast<struct SimFunction *>::to(x)); }
        static __forceinline vec4f from ( const Func x )     { return cast<struct SimFunction *>::from(x.PTR); }
    };

    template <typename Result, typename ...Args>
    struct cast <TFunc<Result,Args...>> {
        static __forceinline Func to ( vec4f x )             { return Func(cast<struct SimFunction *>::to(x)); }
        static __forceinline vec4f from ( const Func x )     { return cast<struct SimFunction *>::from(x.PTR); }
    };

    template <>
    struct cast <Lambda> {
        static __forceinline Lambda to ( vec4f x )           { return Lambda(cast<void *>::to(x)); }
        static __forceinline vec4f from ( const Lambda x )   { return cast<void *>::from(x.capture); }
    };

    template <typename Result, typename ...Args>
    struct cast <TLambda<Result,Args...>> {
        static __forceinline Lambda to ( vec4f x )           { return Lambda(cast<void *>::to(x)); }
        static __forceinline vec4f from ( const Lambda x )   { return cast<void *>::from(x.capture); }
    };

    template <typename TT>
    struct cast_fVec {
        static __forceinline TT to ( vec4f x ) {
            return prune<TT,vec4f>::from(x);
        }
        static __forceinline vec4f from ( const TT & x )       {
#if __SANITIZE_THREAD__
            if ( sizeof(TT) != sizeof(float) * 4 )
            {
              vec4f v;
              memcpy(&v, &x, sizeof(x));
              return v;
            }
#endif
            return v_ldu((const float*)&x);
        }
    };

    template <typename TT>
    struct cast_fVec_half {
        static __forceinline TT to ( vec4f x ) {
            return prune<TT,vec4f>::from(x);
        }
        static __forceinline vec4f from ( const TT & x )       {
            return v_ldu_half((const float*)&x);
        }
    };

    template <> struct cast <float2>  : cast_fVec_half<float2> {};
    template <> struct cast <float3>  : cast_fVec<float3> {};
    template <> struct cast <float4>  : cast_fVec<float4> {};

    template <typename TT>
    struct cast_iVec {
        static __forceinline TT to ( vec4f x ) {
            return prune<TT,vec4f>::from(x);
        }
        static __forceinline vec4f from ( const TT & x ) {
#if __SANITIZE_THREAD__
            if ( sizeof(TT) != sizeof(int) * 4 )
            {
              vec4f v;
              memcpy(&v, &x, sizeof(x));
              return v;
            }
#endif
            return  v_cast_vec4f(v_ldui((const int*)&x));
        }
    };

    template <typename TT>
    struct cast_iVec_half {
        static __forceinline TT to ( vec4f x ) {
            return prune<TT,vec4f>::from(x);
        }
        static __forceinline vec4f from ( const TT & x ) {
            return  v_cast_vec4f(v_ldui_half((const int*)&x));
        }
    };

    template <> struct cast <int2>  : cast_iVec_half<int2> {};
    template <> struct cast <int3>  : cast_iVec<int3> {};
    template <> struct cast <int4>  : cast_iVec<int4> {};

    template <> struct cast <uint2>  : cast_iVec_half<uint2> {};
    template <> struct cast <uint3>  : cast_iVec<uint3> {};
    template <> struct cast <uint4>  : cast_iVec<uint4> {};

    template <> struct cast <range> : cast_iVec_half<range> {};
    template <> struct cast <urange> : cast_iVec_half<urange> {};

    template <> struct cast <range64> : cast_iVec<range64> {};
    template <> struct cast <urange64> : cast_iVec<urange64> {};

    template <typename TT>
    struct cast_enum {
      static __forceinline TT to ( vec4f x )            { return (TT) v_extract_xi(v_cast_vec4i(x)); }
      static __forceinline vec4f from ( TT x )          { return v_cast_vec4f(v_seti_x(int32_t(x))); }
      static __forceinline vec4f from ( int x )         { return v_cast_vec4f(v_seti_x(int32_t(x))); }
    };
}
