#pragma once

#include "daScript/misc/string_writer.h"

namespace das
{
    template <typename TT>
    struct vec_extract {
        static __forceinline TT x ( vec4f t ) { return v_extract_x(t); }
        static __forceinline TT y ( vec4f t ) { return v_extract_y(t); }
        static __forceinline TT z ( vec4f t ) { return v_extract_z(t); }
        static __forceinline TT w ( vec4f t ) { return v_extract_w(t); }
    };

    template <>
    struct vec_extract<int32_t> {
        static __forceinline int32_t x ( vec4f t ) { return v_extract_xi(v_cast_vec4i(t)); }
        static __forceinline int32_t y ( vec4f t ) { return v_extract_yi(v_cast_vec4i(t)); }
        static __forceinline int32_t z ( vec4f t ) { return v_extract_zi(v_cast_vec4i(t)); }
        static __forceinline int32_t w ( vec4f t ) { return v_extract_wi(v_cast_vec4i(t)); }
    };

    template <>
    struct vec_extract<uint32_t> {
        static __forceinline uint32_t x ( vec4f t ) { return v_extract_xi(v_cast_vec4i(t)); }
        static __forceinline uint32_t y ( vec4f t ) { return v_extract_yi(v_cast_vec4i(t)); }
        static __forceinline uint32_t z ( vec4f t ) { return v_extract_zi(v_cast_vec4i(t)); }
        static __forceinline uint32_t w ( vec4f t ) { return v_extract_wi(v_cast_vec4i(t)); }
    };

    template <>
    struct vec_extract<int64_t> {
        static __forceinline int64_t x ( vec4f t ) { union { vec4f t; int64_t T[2]; } temp; temp.t = t; return temp.T[0]; }
        static __forceinline int64_t y ( vec4f t ) { union { vec4f t; int64_t T[2]; } temp; temp.t = t; return temp.T[1]; }
    };

    template <>
    struct vec_extract<uint64_t> {
        static __forceinline uint64_t x ( vec4f t ) { union { vec4f t; uint64_t T[2]; } temp; temp.t = t; return temp.T[0]; }
        static __forceinline uint64_t y ( vec4f t ) { union { vec4f t; uint64_t T[2]; } temp; temp.t = t; return temp.T[1]; }
    };

    __forceinline vec4f vec_loadu_x(const float v) {return v_set_x(v);}
    __forceinline vec4f vec_loadu_x(const int v) {return v_cast_vec4f(v_seti_x(v));}
    __forceinline vec4f vec_loadu_x(const unsigned int v) {return v_cast_vec4f(v_seti_x(v));}
    __forceinline vec4f vec_load(const float *v) {return v_ld(v);}
    __forceinline vec4f vec_load(const int *v) {return v_cast_vec4f(v_ldi(v));}
    __forceinline vec4f vec_load(const unsigned int *v) {return vec_load((const int *)v);}
    __forceinline vec4f vec_loadu(const float *v) {return v_ldu(v);}
    __forceinline vec4f vec_loadu(const int *v) {return v_cast_vec4f(v_ldui(v));}
    __forceinline vec4f vec_loadu(const unsigned int *v) {return vec_loadu((const int *)v);}
    __forceinline vec4f vec_loadu3(const float *v) {return v_ldu_p3(v);}
    __forceinline vec4f vec_loadu3(const int *v) {return v_cast_vec4f(v_ldui_p3(v));}
    __forceinline vec4f vec_loadu3(const unsigned int *v) {return vec_loadu3((const int *)v);}
    __forceinline vec4f vec_loadu_half(const float *v) {return v_ldu_half(v);}
    __forceinline vec4f vec_loadu_half(const int *v) {return v_cast_vec4f(v_ldui_half(v));}
    __forceinline vec4f vec_loadu_half(const unsigned int *v) {return vec_loadu_half((const int *)v);}

    template <typename TT>
    struct vec2 {
        TT   x, y;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const vec2<TT> & vec) {
            stream << vec.x << DAS_PRINT_VEC_SEPARATROR << vec.y;
            return stream;
        }
        __forceinline bool operator == ( const vec2<TT> & vec ) const {
            return x==vec.x && y==vec.y;
        }
        __forceinline bool operator != ( const vec2<TT> & vec ) const {
            return x!=vec.x || y!=vec.y;
        }
        __forceinline vec2() = default;
        __forceinline vec2(const vec2 &) = default;
        __forceinline vec2 & operator = (const vec2 &) = default;
        __forceinline vec2(vec4f t) : x(vec_extract<TT>::x(t)), y(vec_extract<TT>::y(t)) {}
        __forceinline vec2(TT X, TT Y) : x(X), y(Y) {}
        __forceinline vec2(TT t) : x(t), y(t) {}
         __forceinline operator vec4f() const { return vec_loadu_half(&x); };
    };

    template <typename TT>
    struct vec3 {
        TT   x, y, z;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const vec3<TT> & vec) {
            stream << vec.x << DAS_PRINT_VEC_SEPARATROR << vec.y << DAS_PRINT_VEC_SEPARATROR << vec.z;
            return stream;
        }
        __forceinline bool operator == ( const vec3<TT> & vec ) const {
            return x==vec.x && y==vec.y && z==vec.z;
        }
        __forceinline bool operator != ( const vec3<TT> & vec ) const {
            return x!=vec.x || y!=vec.y || z!=vec.z;
        }
        __forceinline vec3() = default;
        __forceinline vec3(const vec3 &) = default;
        __forceinline vec3 & operator = (const vec3 &) = default;
        __forceinline vec3(vec4f t) : x(vec_extract<TT>::x(t)), y(vec_extract<TT>::y(t)), z(vec_extract<TT>::z(t)) {}
        __forceinline vec3(TT X, TT Y, TT Z) : x(X), y(Y), z(Z) {}
        __forceinline vec3(TT t) : x(t), y(t), z(t) {}
        __forceinline operator vec4f() const { return vec_loadu3(&x); };

    };

    template <typename TT>
    struct vec4 {
        TT  x, y, z, w;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const vec4<TT> & vec) {
            stream << vec.x << DAS_PRINT_VEC_SEPARATROR << vec.y << DAS_PRINT_VEC_SEPARATROR << vec.z << DAS_PRINT_VEC_SEPARATROR << vec.w;
            return stream;
        }
        __forceinline bool operator == ( const vec4<TT> & vec ) const {
            return x==vec.x && y==vec.y && z==vec.z && w==vec.w;
        }
        __forceinline bool operator != ( const vec4<TT> & vec ) const {
            return x!=vec.x || y!=vec.y || z!=vec.z || w!=vec.w;
        }
        __forceinline vec4() = default;
        __forceinline vec4(const vec4 &) = default;
        __forceinline vec4 & operator = (const vec4 &) = default;
        __forceinline vec4(vec4f t) : x(vec_extract<TT>::x(t)), y(vec_extract<TT>::y(t)), z(vec_extract<TT>::z(t)), w(vec_extract<TT>::w(t)) {}
        __forceinline vec4(TT X, TT Y, TT Z, TT W) : x(X), y(Y), z(Z), w(W) {}
        __forceinline vec4(TT t) : x(t), y(t), z(t), w(t) {}
        __forceinline operator vec4f() const { return vec_loadu(&x); };
    };

    typedef vec2<float> float2;
    typedef vec3<float> float3;
    typedef vec4<float> float4;

    typedef vec2<int32_t> int2;
    typedef vec3<int32_t> int3;
    typedef vec4<int32_t> int4;

    typedef vec2<uint32_t> uint2;
    typedef vec3<uint32_t> uint3;
    typedef vec4<uint32_t> uint4;
}

