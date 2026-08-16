#pragma once

#include <limits>
#include <type_traits>

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
    __forceinline vec4f vec_loadu3(const float *v)
    {
#ifdef __clang__
      vec4f vv = v_zero();
      memcpy(&vv, v, sizeof(float) * 3);
      return vv;
#else
      return v_ldu_p3(v);
#endif
    }
    __forceinline vec4f vec_loadu3(const int *v)
    {
#ifdef __clang__
      vec4i vv = v_zeroi();
      memcpy(&vv, v, sizeof(int) * 3);
      return v_cast_vec4f(vv);
#else
      return v_cast_vec4f(v_ldui_p3(v));
#endif
    }
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

    // ===== 16/8-bit type lattice =====
    // Unlike vec2/3/4 above (32-bit lane extraction), small-element vectors are byte-packed
    // in the low bytes of the vec4f slot, so all conversions are memcpy-based — correct for
    // every element size, including the odd ones (half3 = 6B, byte3 = 3B).

    // scalar IEEE-754 binary16 carrier. NOT named `half` — that word means "low 64 bits of a
    // vec4f register" throughout the C++ internals (v_ldu_half / cast_fVec_half).
    __forceinline float das_float16_to_float ( uint16_t h ) {
        uint32_t sign = uint32_t(h & 0x8000u) << 16;
        uint32_t em = h & 0x7fffu;
        uint32_t r;
        if ( em >= (31u << 10) ) {              // inf / nan
            r = sign | 0x7f800000u | ((em & 0x3ffu) << 13);
        } else if ( em >= (1u << 10) ) {        // normal
            r = sign | ((em + ((127u - 15u) << 10)) << 13);
        } else if ( em != 0 ) {                 // subnormal -> normalized float
            uint32_t m = em;
            int e = -1;
            do { m <<= 1; e++; } while ( (m & (1u << 10)) == 0 );
            r = sign | ((uint32_t(127 - 15 - e) << 23) | ((m & 0x3ffu) << 13));
        } else {                                // +-0
            r = sign;
        }
        float f;
        memcpy(&f, &r, sizeof(f));
        return f;
    }

    __forceinline uint16_t das_float_to_float16 ( float f ) {   // round-to-nearest-even
        uint32_t x;
        memcpy(&x, &f, sizeof(x));
        uint32_t sign = (x >> 16) & 0x8000u;
        uint32_t em = x & 0x7fffffffu;
        if ( em >= 0x7f800000u ) {              // inf / nan
            uint32_t mant = (em > 0x7f800000u) ? ((em >> 13) & 0x3ffu) | 1u : 0u;
            return uint16_t(sign | (31u << 10) | mant);
        }
        if ( em >= ((127u + 16u) << 23) ) {     // overflow -> inf
            return uint16_t(sign | (31u << 10));
        }
        if ( em < ((127u - 25u) << 23) ) {      // underflow -> +-0
            return uint16_t(sign);
        }
        uint32_t base, sh;
        if ( em < ((127u - 14u) << 23) ) {      // subnormal half
            sh = 126u - (em >> 23) + 13u + 1u;
            base = (em & 0x7fffffu) | 0x800000u;
        } else {                                // normal half
            sh = 13u;
            base = em - ((127u - 15u) << 23);
        }
        uint32_t rem = base & ((1u << sh) - 1u);
        base >>= sh;
        uint32_t halfway = 1u << (sh - 1u);
        if ( rem > halfway || (rem == halfway && (base & 1u)) ) base++;
        return uint16_t(sign | base);
    }

    struct float16_t {
        uint16_t bits;
        __forceinline float16_t() = default;
        __forceinline float16_t(const float16_t &) = default;
        __forceinline float16_t & operator = (const float16_t &) = default;
        __forceinline explicit float16_t(float f) : bits(das_float_to_float16(f)) {}
        __forceinline float toFloat() const { return das_float16_to_float(bits); }
        __forceinline explicit operator float() const { return toFloat(); }
        // IEEE semantics via promote-compare: -0 == +0, NaN != NaN (bitwise would violate both)
        __forceinline bool operator == ( const float16_t & v ) const { return toFloat() == v.toFloat(); }
        __forceinline bool operator != ( const float16_t & v ) const { return toFloat() != v.toFloat(); }
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const float16_t & v) {
            stream << v.toFloat();
            return stream;
        }
    };
    static_assert(sizeof(float16_t) == 2, "float16_t must be 2 bytes");

    // print 8-bit lanes numerically, not as characters
    template <typename TT> struct svec_print { typedef TT type; };
    template <> struct svec_print<int8_t>  { typedef int32_t type; };
    template <> struct svec_print<uint8_t> { typedef uint32_t type; };

    template <typename TT>
    struct svec2 {
        TT x, y;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const svec2<TT> & vec) {
            stream << typename svec_print<TT>::type(vec.x) << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.y);
            return stream;
        }
        __forceinline bool operator == ( const svec2<TT> & vec ) const { return x==vec.x && y==vec.y; }
        __forceinline bool operator != ( const svec2<TT> & vec ) const { return x!=vec.x || y!=vec.y; }
        __forceinline svec2() = default;
        __forceinline svec2(const svec2 &) = default;
        __forceinline svec2 & operator = (const svec2 &) = default;
        __forceinline svec2(vec4f t) { memcpy(this, &t, sizeof(*this)); }
        __forceinline svec2(TT X, TT Y) : x(X), y(Y) {}
        __forceinline svec2(TT t) : x(t), y(t) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec2(QQ X, QQ Y) : x(TT(X)), y(TT(Y)) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec2(QQ t) : x(TT(t)), y(TT(t)) {}
        __forceinline operator vec4f() const { vec4f r = v_zero(); memcpy(&r, this, sizeof(*this)); return r; }
    };

    template <typename TT>
    struct svec3 {
        TT x, y, z;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const svec3<TT> & vec) {
            stream << typename svec_print<TT>::type(vec.x) << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.y)
                << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.z);
            return stream;
        }
        __forceinline bool operator == ( const svec3<TT> & vec ) const { return x==vec.x && y==vec.y && z==vec.z; }
        __forceinline bool operator != ( const svec3<TT> & vec ) const { return x!=vec.x || y!=vec.y || z!=vec.z; }
        __forceinline svec3() = default;
        __forceinline svec3(const svec3 &) = default;
        __forceinline svec3 & operator = (const svec3 &) = default;
        __forceinline svec3(vec4f t) { memcpy(this, &t, sizeof(*this)); }
        __forceinline svec3(TT X, TT Y, TT Z) : x(X), y(Y), z(Z) {}
        __forceinline svec3(TT t) : x(t), y(t), z(t) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec3(QQ X, QQ Y, QQ Z) : x(TT(X)), y(TT(Y)), z(TT(Z)) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec3(QQ t) : x(TT(t)), y(TT(t)), z(TT(t)) {}
        __forceinline operator vec4f() const { vec4f r = v_zero(); memcpy(&r, this, sizeof(*this)); return r; }
    };

    template <typename TT>
    struct svec4 {
        TT x, y, z, w;
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const svec4<TT> & vec) {
            stream << typename svec_print<TT>::type(vec.x) << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.y)
                << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.z) << DAS_PRINT_VEC_SEPARATROR << typename svec_print<TT>::type(vec.w);
            return stream;
        }
        __forceinline bool operator == ( const svec4<TT> & vec ) const { return x==vec.x && y==vec.y && z==vec.z && w==vec.w; }
        __forceinline bool operator != ( const svec4<TT> & vec ) const { return x!=vec.x || y!=vec.y || z!=vec.z || w!=vec.w; }
        __forceinline svec4() = default;
        __forceinline svec4(const svec4 &) = default;
        __forceinline svec4 & operator = (const svec4 &) = default;
        __forceinline svec4(vec4f t) { memcpy(this, &t, sizeof(*this)); }
        __forceinline svec4(TT X, TT Y, TT Z, TT W) : x(X), y(Y), z(Z), w(W) {}
        __forceinline svec4(TT t) : x(t), y(t), z(t), w(t) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec4(QQ X, QQ Y, QQ Z, QQ W) : x(TT(X)), y(TT(Y)), z(TT(Z)), w(TT(W)) {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec4(QQ t) : x(TT(t)), y(TT(t)), z(TT(t)), w(TT(t)) {}
        __forceinline operator vec4f() const { vec4f r = v_zero(); memcpy(&r, this, sizeof(*this)); return r; }
    };

    template <typename TT>
    struct svec8 {
        TT s[8];
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const svec8<TT> & vec) {
            for ( int i = 0; i != 8; ++i ) {
                if ( i ) stream << DAS_PRINT_VEC_SEPARATROR;
                stream << typename svec_print<TT>::type(vec.s[i]);
            }
            return stream;
        }
        __forceinline bool operator == ( const svec8<TT> & vec ) const {
            for ( int i = 0; i != 8; ++i ) if ( !(s[i] == vec.s[i]) ) return false;
            return true;
        }
        __forceinline bool operator != ( const svec8<TT> & vec ) const { return !(*this == vec); }
        __forceinline svec8() = default;
        __forceinline svec8(const svec8 &) = default;
        __forceinline svec8 & operator = (const svec8 &) = default;
        __forceinline svec8(vec4f t) { memcpy(this, &t, sizeof(*this)); }
        __forceinline svec8(TT s0, TT s1, TT s2, TT s3, TT s4, TT s5, TT s6, TT s7) : s{s0, s1, s2, s3, s4, s5, s6, s7} {}
        __forceinline svec8(TT t) { for ( int i = 0; i != 8; ++i ) s[i] = t; }
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec8(QQ s0, QQ s1, QQ s2, QQ s3, QQ s4, QQ s5, QQ s6, QQ s7)
            : s{TT(s0), TT(s1), TT(s2), TT(s3), TT(s4), TT(s5), TT(s6), TT(s7)} {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec8(QQ t) { for ( int i = 0; i != 8; ++i ) s[i] = TT(t); }
        __forceinline operator vec4f() const { vec4f r = v_zero(); memcpy(&r, this, sizeof(*this)); return r; }
    };

    template <typename TT>
    struct svec16 {
        TT s[16];
        __forceinline friend StringWriter & operator<< (StringWriter & stream, const svec16<TT> & vec) {
            for ( int i = 0; i != 16; ++i ) {
                if ( i ) stream << DAS_PRINT_VEC_SEPARATROR;
                stream << typename svec_print<TT>::type(vec.s[i]);
            }
            return stream;
        }
        __forceinline bool operator == ( const svec16<TT> & vec ) const {
            for ( int i = 0; i != 16; ++i ) if ( !(s[i] == vec.s[i]) ) return false;
            return true;
        }
        __forceinline bool operator != ( const svec16<TT> & vec ) const { return !(*this == vec); }
        __forceinline svec16() = default;
        __forceinline svec16(const svec16 &) = default;
        __forceinline svec16 & operator = (const svec16 &) = default;
        __forceinline svec16(vec4f t) { memcpy(this, &t, sizeof(*this)); }
        __forceinline svec16(TT s0, TT s1, TT s2, TT s3, TT s4, TT s5, TT s6, TT s7,
                             TT s8, TT s9, TT sa, TT sb, TT sc, TT sd, TT se, TT sf)
            : s{s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sa, sb, sc, sd, se, sf} {}
        __forceinline svec16(TT t) { for ( int i = 0; i != 16; ++i ) s[i] = t; }
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec16(QQ s0, QQ s1, QQ s2, QQ s3, QQ s4, QQ s5, QQ s6, QQ s7,
                             QQ s8, QQ s9, QQ sa, QQ sb, QQ sc, QQ sd, QQ se, QQ sf)
            : s{TT(s0), TT(s1), TT(s2), TT(s3), TT(s4), TT(s5), TT(s6), TT(s7),
                TT(s8), TT(s9), TT(sa), TT(sb), TT(sc), TT(sd), TT(se), TT(sf)} {}
        template <typename QQ, typename = typename std::enable_if<std::is_arithmetic<QQ>::value>::type>
        __forceinline svec16(QQ t) { for ( int i = 0; i != 16; ++i ) s[i] = TT(t); }
        __forceinline operator vec4f() const { vec4f r = v_zero(); memcpy(&r, this, sizeof(*this)); return r; }
    };

    typedef svec2<float16_t> half2;
    typedef svec3<float16_t> half3;
    typedef svec4<float16_t> half4;
    typedef svec8<float16_t> half8;

    typedef svec2<int16_t> short2;
    typedef svec3<int16_t> short3;
    typedef svec4<int16_t> short4;
    typedef svec8<int16_t> short8;

    typedef svec2<uint16_t> ushort2;
    typedef svec3<uint16_t> ushort3;
    typedef svec4<uint16_t> ushort4;
    typedef svec8<uint16_t> ushort8;

    typedef svec2<int8_t> byte2;      // byte = SIGNED int8 (house naming: bare=signed)
    typedef svec3<int8_t> byte3;
    typedef svec4<int8_t> byte4;
    typedef svec8<int8_t> byte8;
    typedef svec16<int8_t> byte16;

    typedef svec2<uint8_t> ubyte2;
    typedef svec3<uint8_t> ubyte3;
    typedef svec4<uint8_t> ubyte4;
    typedef svec8<uint8_t> ubyte8;
    typedef svec16<uint8_t> ubyte16;

    static_assert(sizeof(half3) == 6 && sizeof(half8) == 16 && sizeof(byte3) == 3 && sizeof(byte16) == 16,
        "small-element vectors must be tightly packed");

    // lane-wise conversions between lattice vectors and the 32-bit families (and between
    // lattice families). The names carry the template args so AOT can emit them verbatim.
    template <typename TO, typename TOE, typename FROM, typename FE>
    __forceinline TO das_sv_cvt ( FROM v ) {
        TO r;
        memset(&r, 0, sizeof(TO));
        constexpr int n = int(sizeof(FROM) / sizeof(FE));
        static_assert(n * sizeof(TOE) == sizeof(TO), "lane count mismatch");
        const FE * s = (const FE *) &v;
        TOE * d = (TOE *) &r;
        for ( int i = 0; i != n; ++i ) d[i] = TOE(s[i]);
        return r;
    }

    // typed zero for AOT emission — the interp zero ctor is SimNode_Zero (a raw vec4f),
    // but emitted C++ needs the value typed or template deduction around it breaks
    template <typename VT>
    __forceinline VT das_svec_zero () {
        VT r;
        memset(&r, 0, sizeof(r));
        return r;
    }

    // half8 <-> 2x float4 pack/unpack (the fp16 SIMD register shape)
    __forceinline half8 das_half8_pack ( float4 lo, float4 hi ) {
        half8 r;
        const float * s = &lo.x;
        for ( int i = 0; i != 4; ++i ) r.s[i] = float16_t(s[i]);
        s = &hi.x;
        for ( int i = 0; i != 4; ++i ) r.s[i + 4] = float16_t(s[i]);
        return r;
    }
    __forceinline float4 das_half8_lo ( half8 v ) {
        return float4(v.s[0].toFloat(), v.s[1].toFloat(), v.s[2].toFloat(), v.s[3].toFloat());
    }
    __forceinline float4 das_half8_hi ( half8 v ) {
        return float4(v.s[4].toFloat(), v.s[5].toFloat(), v.s[6].toFloat(), v.s[7].toFloat());
    }

    // The idot family — the lattice's first compute ops: exact integer dot products over the
    // 8-bit vectors. idot4 = per-dword-lane 4x i8 (or u8) x i8 MACs accumulated into int32
    // lanes (the sdot/udot/vpdpbusd hardware shape); idot = the full 16-lane reduce the quant
    // kernels accumulate per block. Results are ALWAYS signed; operand signedness selects the
    // overload (the OpSDot / OpSUDot split). Names carry the signedness so AOT emits verbatim.
    __forceinline int4 das_idot4_ss ( int4 acc, byte16 a, byte16 b ) {
        int32_t * r = &acc.x;
        for ( int i = 0; i != 4; ++i )
            for ( int j = 0; j != 4; ++j )
                r[i] += int32_t(a.s[i * 4 + j]) * int32_t(b.s[i * 4 + j]);
        return acc;
    }
    __forceinline int4 das_idot4_us ( int4 acc, ubyte16 a, byte16 b ) {
        int32_t * r = &acc.x;
        for ( int i = 0; i != 4; ++i )
            for ( int j = 0; j != 4; ++j )
                r[i] += int32_t(a.s[i * 4 + j]) * int32_t(b.s[i * 4 + j]);
        return acc;
    }
    __forceinline int4 das_idot4z_ss ( byte16 a, byte16 b ) {
        return das_idot4_ss(int4(0, 0, 0, 0), a, b);
    }
    __forceinline int4 das_idot4z_us ( ubyte16 a, byte16 b ) {
        return das_idot4_us(int4(0, 0, 0, 0), a, b);
    }
    __forceinline int32_t das_idot_ss ( byte16 a, byte16 b ) {
        int32_t s = 0;
        for ( int i = 0; i != 16; ++i ) s += int32_t(a.s[i]) * int32_t(b.s[i]);
        return s;
    }
    __forceinline int32_t das_idot_us ( ubyte16 a, byte16 b ) {
        int32_t s = 0;
        for ( int i = 0; i != 16; ++i ) s += int32_t(a.s[i]) * int32_t(b.s[i]);
        return s;
    }
    // byte table select (tbl1/pshufb): out[i] = lut[idx[i] & 15] — indices masked to the
    // 16-entry table so every input is total (pshufb's bit-7 zeroing never engages)
    __forceinline byte16 das_shuffle_b16 ( byte16 lut, ubyte16 idx ) {
        byte16 r;
        for ( int i = 0; i != 16; ++i ) r.s[i] = lut.s[idx.s[i] & 15];
        return r;
    }
    // the integer-lattice bit surface (parity with the 32-bit vector families): per-lane
    // shifts by a scalar count masked to the LANE width (the scalar &31 rule at lane size;
    // signed >> is arithmetic, unsigned logical; << wraps lanes like the C cast), and the
    // lane-agnostic bitwise & | ^. Names carry the template args so AOT emits verbatim.
    template <typename VT, typename ET>
    __forceinline VT das_sv_shr ( VT v, int32_t count ) {
        constexpr int bits = int(sizeof(ET) * 8);
        const int s = count & (bits - 1);
        constexpr int n = int(sizeof(VT) / sizeof(ET));
        ET * d = (ET *) &v;
        for ( int i = 0; i != n; ++i ) d[i] = ET(d[i] >> s);
        return v;
    }
    template <typename VT, typename ET>
    __forceinline VT das_sv_shl ( VT v, int32_t count ) {
        constexpr int bits = int(sizeof(ET) * 8);
        const int s = count & (bits - 1);
        constexpr int n = int(sizeof(VT) / sizeof(ET));
        ET * d = (ET *) &v;
        for ( int i = 0; i != n; ++i ) d[i] = ET(uint32_t(d[i]) << s);
        return v;
    }
    template <typename VT, typename ET>
    __forceinline void das_sv_set_shr ( VT & v, int32_t count ) { v = das_sv_shr<VT,ET>(v, count); }
    template <typename VT, typename ET>
    __forceinline void das_sv_set_shl ( VT & v, int32_t count ) { v = das_sv_shl<VT,ET>(v, count); }
    template <typename VT>
    __forceinline VT das_sv_and ( VT a, VT b ) {
        uint8_t * pa = (uint8_t *) &a;
        const uint8_t * pb = (const uint8_t *) &b;
        for ( int i = 0; i != int(sizeof(VT)); ++i ) pa[i] &= pb[i];
        return a;
    }
    template <typename VT>
    __forceinline VT das_sv_or ( VT a, VT b ) {
        uint8_t * pa = (uint8_t *) &a;
        const uint8_t * pb = (const uint8_t *) &b;
        for ( int i = 0; i != int(sizeof(VT)); ++i ) pa[i] |= pb[i];
        return a;
    }
    template <typename VT>
    __forceinline VT das_sv_xor ( VT a, VT b ) {
        uint8_t * pa = (uint8_t *) &a;
        const uint8_t * pb = (const uint8_t *) &b;
        for ( int i = 0; i != int(sizeof(VT)); ++i ) pa[i] ^= pb[i];
        return a;
    }
    template <typename VT>
    __forceinline void das_sv_set_and ( VT & a, VT b ) { a = das_sv_and(a, b); }
    template <typename VT>
    __forceinline void das_sv_set_or ( VT & a, VT b ) { a = das_sv_or(a, b); }
    template <typename VT>
    __forceinline void das_sv_set_xor ( VT & a, VT b ) { a = das_sv_xor(a, b); }

    template <typename TO, typename TOE, typename FROM, typename FE>
    __forceinline TO das_sv_cvt_sat ( FROM v ) {
        TO r;
        memset(&r, 0, sizeof(TO));
        constexpr int n = int(sizeof(FROM) / sizeof(FE));
        static_assert(n * sizeof(TOE) == sizeof(TO), "lane count mismatch");
        constexpr int64_t lo = int64_t(std::numeric_limits<TOE>::min());
        constexpr int64_t hi = int64_t(std::numeric_limits<TOE>::max());
        const FE * s = (const FE *) &v;
        TOE * d = (TOE *) &r;
        for ( int i = 0; i != n; ++i ) {
            int64_t x = int64_t(s[i]);
            d[i] = TOE(x < lo ? lo : (x > hi ? hi : x));
        }
        return r;
    }
}

