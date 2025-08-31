// © 2021 NVIDIA Corporation

#pragma once

//======================================================================================================================
// uint2
//======================================================================================================================

union uint2 {
    v2i mm;

    struct {
        uint32_t a[COORD_2D];
    };

    struct {
        uint32_t x, y;
    };

    ML_SWIZZLE_2(uint2, uint32_t);

public:
    ML_INLINE uint2()
        : mm(0) {
    }

    ML_INLINE uint2(uint32_t c)
        : x(c), y(c) {
    }

    ML_INLINE uint2(uint32_t _x, uint32_t _y)
        : x(_x), y(_y) {
    }

    ML_INLINE uint2(const uint2& v) = default;

    // Set

    ML_INLINE void operator=(const uint2& v) {
        mm = v.mm;
    }

    // Conversion

    ML_INLINE operator int2() const;
    ML_INLINE operator float2() const;
    ML_INLINE operator double2() const;

    // Compare

    ML_COMPARE_UNOPT(bool2, uint2, <)
    ML_COMPARE_UNOPT(bool2, uint2, <=)
    ML_COMPARE_UNOPT(bool2, uint2, ==)
    ML_COMPARE_UNOPT(bool2, uint2, >=)
    ML_COMPARE_UNOPT(bool2, uint2, >)
    ML_COMPARE_UNOPT(bool2, uint2, !=)

    // Ops

    ML_OP_UNOPT(uint2, uint32_t, -, -=)
    ML_OP_UNOPT(uint2, uint32_t, +, +=)
    ML_OP_UNOPT(uint2, uint32_t, *, *=)
    ML_OP_UNOPT(uint2, uint32_t, /, /=)
    ML_OP_UNOPT(uint2, uint32_t, %, %=)
    ML_OP_UNOPT(uint2, uint32_t, <<, <<=)
    ML_OP_UNOPT(uint2, uint32_t, >>, >>=)
    ML_OP_UNOPT(uint2, uint32_t, &, &=)
    ML_OP_UNOPT(uint2, uint32_t, |, |=)
    ML_OP_UNOPT(uint2, uint32_t, ^, ^=)
};

ML_INLINE uint2 min(const uint2& x, const uint2& y) {
    return uint2(min(x.x, y.x), min(x.y, y.y));
}

ML_INLINE uint2 max(const uint2& x, const uint2& y) {
    return uint2(max(x.x, y.x), max(x.y, y.y));
}

//======================================================================================================================
// uint3
//======================================================================================================================

union uint3 {
    v4i xmm;

    struct {
        uint32_t a[COORD_3D];
    };

    struct {
        uint32_t x, y, z;
    };

    ML_SWIZZLE_3(v4u_swizzle2, uint2, v4u_swizzle3, uint3);

public:
    ML_INLINE uint3()
        : xmm(_mm_setzero_si128()) {
    }

    ML_INLINE uint3(uint32_t c)
        : xmm(_mm_set1_epi32(c)) {
    }

    ML_INLINE uint3(uint32_t _x, uint32_t _y, uint32_t _z)
        : xmm(v4i_set(_x, _y, _z, 1)) {
    }

    ML_INLINE uint3(const uint2& v, uint32_t _z)
        : xmm(v4i_set(v.x, v.y, _z, 1)) {
    }

    ML_INLINE uint3(uint32_t _x, const uint2& v)
        : xmm(v4i_set(_x, v.x, v.y, 1)) {
    }

    ML_INLINE uint3(const v4i& v)
        : xmm(v) {
    }

    ML_INLINE uint3(const uint32_t* v3)
        : xmm(v4i_set(v3[0], v3[1], v3[2], 1)) {
    }

    ML_INLINE uint3(const uint3& v) = default;

    // Set

    ML_INLINE void operator=(const uint3& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator int3() const;
    ML_INLINE operator float3() const;
    ML_INLINE operator double3() const;

    // Compare

    ML_COMPARE(bool3, uint3, <, _mm_cmplt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, uint3, <=, _mm_cmple_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, uint3, ==, _mm_cmpeq_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, uint3, >, _mm_cmpgt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, uint3, >=, _mm_cmpge_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, uint3, !=, _mm_cmpneq_epi32, _mm_movemask_epi32, xmm)

    // Ops

    ML_OP(uint3, uint32_t, -, -=, _mm_sub_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, +, +=, _mm_add_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, *, *=, _mm_mullo_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, /, /=, _mm_div_epu32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, %, %=, v4u_mod, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, <<, <<=, _mm_sllv_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, >>, >>=, _mm_srlv_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, &, &=, _mm_and_si128, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, |, |=, _mm_or_si128, _mm_set1_epi32, xmm)
    ML_OP(uint3, uint32_t, ^, ^=, _mm_xor_si128, _mm_set1_epi32, xmm)

    // Misc

    ML_INLINE operator v4i() const {
        return xmm;
    }

    static ML_INLINE uint3 Zero() {
        return _mm_setzero_si128();
    }
};

ML_INLINE uint3 min(const uint3& x, const uint3& y) {
    return _mm_min_epu32(x.xmm, y.xmm);
}

ML_INLINE uint3 max(const uint3& x, const uint3& y) {
    return _mm_max_epu32(x.xmm, y.xmm);
}

//======================================================================================================================
// uint4
//======================================================================================================================

union uint4 {
    v4i xmm;

    struct {
        uint32_t a[COORD_4D];
    };

    struct {
        uint32_t x, y, z, w;
    };

    ML_SWIZZLE_4(v4u_swizzle2, uint2, v4u_swizzle3, uint3, v4u_swizzle4, uint4);

public:
    ML_INLINE uint4()
        : xmm(_mm_setzero_si128()) {
    }

    ML_INLINE uint4(uint32_t c)
        : xmm(_mm_set1_epi32(c)) {
    }

    ML_INLINE uint4(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _w)
        : xmm(v4i_set(_x, _y, _z, _w)) {
    }

    ML_INLINE uint4(const uint3& v, uint32_t _w)
        : xmm(v4i_set(v.x, v.y, v.z, _w)) {
    }

    ML_INLINE uint4(const uint2& a, const uint2& b)
        : xmm(v4i_set(a.x, a.y, b.x, b.y)) {
    }

    ML_INLINE uint4(uint32_t _x, const uint3& v)
        : xmm(v4i_set(_x, v.x, v.y, v.z)) {
    }

    ML_INLINE uint4(const v4i& v)
        : xmm(v) {
    }

    ML_INLINE uint4(const uint4& v) = default;

    // Set

    ML_INLINE void operator=(const uint4& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator int4() const;
    ML_INLINE operator float4() const;
    ML_INLINE operator double4() const;

    // Compare

    ML_COMPARE(bool4, uint4, <, _mm_cmplt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, uint4, <=, _mm_cmple_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, uint4, ==, _mm_cmpeq_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, uint4, >, _mm_cmpgt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, uint4, >=, _mm_cmpge_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, uint4, !=, _mm_cmpneq_epi32, _mm_movemask_epi32, xmm)

    // Ops

    ML_OP(uint4, uint32_t, -, -=, _mm_sub_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, +, +=, _mm_add_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, *, *=, _mm_mullo_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, /, /=, _mm_div_epu32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, %, %=, v4u_mod, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, <<, <<=, _mm_sllv_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, >>, >>=, _mm_srlv_epi32, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, &, &=, _mm_and_si128, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, |, |=, _mm_or_si128, _mm_set1_epi32, xmm)
    ML_OP(uint4, uint32_t, ^, ^=, _mm_xor_si128, _mm_set1_epi32, xmm)

    // Misc

    ML_INLINE operator v4i() const {
        return xmm;
    }

    static ML_INLINE uint4 Zero() {
        return _mm_setzero_si128();
    }
};

ML_INLINE uint4 min(const uint4& x, const uint4& y) {
    return _mm_min_epu32(x.xmm, y.xmm);
}

ML_INLINE uint4 max(const uint4& x, const uint4& y) {
    return _mm_max_epu32(x.xmm, y.xmm);
}
