// © 2021 NVIDIA Corporation

#pragma once

//======================================================================================================================
// int2
//======================================================================================================================

union int2 {
    v2i mm;

    struct {
        int32_t a[COORD_2D];
    };

    struct {
        int32_t x, y;
    };

    ML_SWIZZLE_2(int2, int32_t);

public:
    ML_INLINE int2()
        : mm(0) {
    }

    ML_INLINE int2(int32_t c)
        : x(c), y(c) {
    }

    ML_INLINE int2(int32_t _x, int32_t _y)
        : x(_x), y(_y) {
    }

    ML_INLINE int2(const int2& v) = default;

    // Set

    ML_INLINE void operator=(const int2& v) {
        mm = v.mm;
    }

    // Conversion

    ML_INLINE operator uint2() const;
    ML_INLINE operator float2() const;
    ML_INLINE operator double2() const;

    // Compare

    ML_COMPARE_UNOPT(bool2, int2, <)
    ML_COMPARE_UNOPT(bool2, int2, <=)
    ML_COMPARE_UNOPT(bool2, int2, ==)
    ML_COMPARE_UNOPT(bool2, int2, >=)
    ML_COMPARE_UNOPT(bool2, int2, >)
    ML_COMPARE_UNOPT(bool2, int2, !=)

    // Ops

    ML_INLINE int2 operator-() const {
        return int2(-x, -y);
    }

    ML_OP_UNOPT(int2, int32_t, -, -=)
    ML_OP_UNOPT(int2, int32_t, +, +=)
    ML_OP_UNOPT(int2, int32_t, *, *=)
    ML_OP_UNOPT(int2, int32_t, /, /=)
    ML_OP_UNOPT(int2, int32_t, %, %=)
    ML_OP_UNOPT(int2, int32_t, <<, <<=)
    ML_OP_UNOPT(int2, int32_t, >>, >>=)
    ML_OP_UNOPT(int2, int32_t, &, &=)
    ML_OP_UNOPT(int2, int32_t, |, |=)
    ML_OP_UNOPT(int2, int32_t, ^, ^=)
};

ML_INLINE int2 min(const int2& x, const int2& y) {
    return int2(min(x.x, y.x), min(x.y, y.y));
}

ML_INLINE int2 max(const int2& x, const int2& y) {
    return int2(max(x.x, y.x), max(x.y, y.y));
}

//======================================================================================================================
// int3
//======================================================================================================================

union int3 {
    v4i xmm;

    struct {
        int32_t a[COORD_3D];
    };

    struct {
        int32_t x, y, z;
    };

    ML_SWIZZLE_3(v4i_swizzle2, int2, v4i_swizzle3, int3);

public:
    ML_INLINE int3()
        : xmm(_mm_setzero_si128()) {
    }

    ML_INLINE int3(int32_t c)
        : xmm(_mm_set1_epi32(c)) {
    }

    ML_INLINE int3(int32_t _x, int32_t _y, int32_t _z)
        : xmm(v4i_set(_x, _y, _z, 1)) {
    }

    ML_INLINE int3(const int2& v, int32_t _z)
        : xmm(v4i_set(v.x, v.y, _z, 1)) {
    }

    ML_INLINE int3(int32_t _x, const int2& v)
        : xmm(v4i_set(_x, v.x, v.y, 1)) {
    }

    ML_INLINE int3(const v4i& v)
        : xmm(v) {
    }

    ML_INLINE int3(const int32_t* v3)
        : xmm(v4i_set(v3[0], v3[1], v3[2], 1)) {
    }

    ML_INLINE int3(const int3& v) = default;

    // Set

    ML_INLINE void operator=(const int3& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator uint3() const;
    ML_INLINE operator float3() const;
    ML_INLINE operator double3() const;

    // Compare

    ML_COMPARE(bool3, int3, <, _mm_cmplt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, int3, <=, _mm_cmple_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, int3, ==, _mm_cmpeq_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, int3, >, _mm_cmpgt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, int3, >=, _mm_cmpge_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool3, int3, !=, _mm_cmpneq_epi32, _mm_movemask_epi32, xmm)

    // Ops

    ML_INLINE int3 operator-() const {
        return _mm_xor_si128(xmm, _mm_set1_epi32(0x80000000));
    }

    ML_OP(int3, int32_t, -, -=, _mm_sub_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, +, +=, _mm_add_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, *, *=, _mm_mullo_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, /, /=, _mm_div_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, %, %=, v4i_mod, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, <<, <<=, _mm_sllv_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, >>, >>=, _mm_srlv_epi32, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, &, &=, _mm_and_si128, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, |, |=, _mm_or_si128, _mm_set1_epi32, xmm)
    ML_OP(int3, int32_t, ^, ^=, _mm_xor_si128, _mm_set1_epi32, xmm)

    // Misc

    ML_INLINE operator v4i() const {
        return xmm;
    }

    static ML_INLINE int3 Zero() {
        return _mm_setzero_si128();
    }
};

ML_INLINE int3 min(const int3& x, const int3& y) {
    return _mm_min_epi32(x.xmm, y.xmm);
}

ML_INLINE int3 max(const int3& x, const int3& y) {
    return _mm_max_epi32(x.xmm, y.xmm);
}

//======================================================================================================================
// int4
//======================================================================================================================

union int4 {
    v4i xmm;

    struct {
        int32_t a[COORD_4D];
    };

    struct {
        int32_t x, y, z, w;
    };

    ML_SWIZZLE_4(v4i_swizzle2, int2, v4i_swizzle3, int3, v4i_swizzle4, int4);

public:
    ML_INLINE int4()
        : xmm(_mm_setzero_si128()) {
    }

    ML_INLINE int4(int32_t c)
        : xmm(_mm_set1_epi32(c)) {
    }

    ML_INLINE int4(int32_t _x, int32_t _y, int32_t _z, int32_t _w)
        : xmm(v4i_set(_x, _y, _z, _w)) {
    }

    ML_INLINE int4(const int3& v, int32_t _w)
        : xmm(v4i_set(v.x, v.y, v.z, _w)) {
    }

    ML_INLINE int4(const int2& a, const int2& b)
        : xmm(v4i_set(a.x, a.y, b.x, b.y)) {
    }

    ML_INLINE int4(int32_t _x, const int3& v)
        : xmm(v4i_set(_x, v.x, v.y, v.z)) {
    }

    ML_INLINE int4(const v4i& v)
        : xmm(v) {
    }

    ML_INLINE int4(const int4& v) = default;

    // Set

    ML_INLINE void operator=(const int4& v) {
        xmm = v.xmm;
    }

    // Compare

    ML_COMPARE(bool4, int4, <, _mm_cmplt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, int4, <=, _mm_cmple_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, int4, ==, _mm_cmpeq_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, int4, >, _mm_cmpgt_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, int4, >=, _mm_cmpge_epi32, _mm_movemask_epi32, xmm)
    ML_COMPARE(bool4, int4, !=, _mm_cmpneq_epi32, _mm_movemask_epi32, xmm)

    // Conversion

    ML_INLINE operator uint4() const;
    ML_INLINE operator float4() const;
    ML_INLINE operator double4() const;

    // Ops

    ML_INLINE int4 operator-() const {
        return _mm_xor_si128(xmm, _mm_set1_epi32(0x80000000));
    }

    ML_OP(int4, int32_t, -, -=, _mm_sub_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, +, +=, _mm_add_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, *, *=, _mm_mullo_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, /, /=, _mm_div_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, %, %=, v4i_mod, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, <<, <<=, _mm_sllv_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, >>, >>=, _mm_srlv_epi32, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, &, &=, _mm_and_si128, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, |, |=, _mm_or_si128, _mm_set1_epi32, xmm)
    ML_OP(int4, int32_t, ^, ^=, _mm_xor_si128, _mm_set1_epi32, xmm)

    // Misc

    ML_INLINE operator v4i() const {
        return xmm;
    }

    static ML_INLINE int4 Zero() {
        return _mm_setzero_si128();
    }
};

ML_INLINE int4 min(const int4& x, const int4& y) {
    return _mm_min_epi32(x.xmm, y.xmm);
}

ML_INLINE int4 max(const int4& x, const int4& y) {
    return _mm_max_epi32(x.xmm, y.xmm);
}
