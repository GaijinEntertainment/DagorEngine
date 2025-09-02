// © 2021 NVIDIA Corporation

#pragma once

#define _X 0
#define _Y 1
#define _Z 2
#define _W 3

template <class C, typename T, uint32_t... Indices>
class swizzle {
private:
    // Based on: https://kiorisyshen.github.io/2018/08/27/Vector%20Swizzling%20and%20Parameter%20Pack%20in%20C++/
    T a[sizeof...(Indices)];

public:
    static constexpr uint32_t i[] = {Indices...};
    static constexpr size_t N = sizeof...(Indices);

    ML_INLINE void operator=(const C& rhs) {
        for (size_t n = 0; n < N; n++)
            a[i[n]] = rhs[n];
    }

    ML_INLINE operator C() const {
        return C(a[Indices]...);
    }
};

// Swizzle ops

#define ML_SWIZZLE_2_OP(op, f, swizzle) \
    ML_INLINE void operator op(const C& v) { \
        ML_StaticAssertMsg(X != Y, "Wrong swizzle in " ML_Stringify(op)); \
        a[X] op v.x; \
        a[Y] op v.y; \
    }

#define ML_SWIZZLE_3_OP(op, f, swizzle) \
    ML_INLINE void operator op(const C& v) { \
        ML_StaticAssertMsg(X != Y && Y != Z && Z != X, "Wrong swizzle in " ML_Stringify(op)); \
        a[X] op v.x; \
        a[Y] op v.y; \
        a[Z] op v.z; \
    }

#if 0
#    define ML_SWIZZLE_4_OP(op, f, swizzle) \
        ML_INLINE void operator op(const C& v) { \
            ML_StaticAssertMsg(X + Y + Z + W == 6, "Wrong swizzle in " ML_Stringify(op)); \
            a[X] op v.x; \
            a[Y] op v.y; \
            a[Z] op v.z; \
            a[W] op v.w; \
        }
#else
#    define ML_SWIZZLE_4_OP(op, f, swizzle) \
        ML_INLINE void operator op(const C& v) { \
            ML_StaticAssertMsg(X + Y + Z + W == 6, "Wrong swizzle in " ML_Stringify(op)); \
            vec = f(swizzle(vec, X, Y, Z, W), v); \
        }
#endif

// v4i

template <class C, uint32_t X, uint32_t Y>
class v4i_swizzle2 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            int32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return C(a[X], a[Y]);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_2_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_2_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(/=, _mm_div_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(%=, v4i_mod, v4i_swizzle)
    ML_SWIZZLE_2_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_2_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_2_OP(^=, _mm_xor_si128, v4i_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z>
class v4i_swizzle3 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            int32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4i_swizzle(vec, X, Y, Z, 3);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_3_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_3_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(/=, _mm_div_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(%=, v4i_mod, v4i_swizzle)
    ML_SWIZZLE_3_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_3_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_3_OP(^=, _mm_xor_si128, v4i_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
class v4i_swizzle4 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            int32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4i_swizzle(vec, X, Y, Z, W);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_4_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_4_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(/=, _mm_div_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(%=, v4i_mod, v4i_swizzle)
    ML_SWIZZLE_4_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_4_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_4_OP(^=, _mm_xor_si128, v4i_swizzle)
};

// v4u

template <class C, uint32_t X, uint32_t Y>
class v4u_swizzle2 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            uint32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return C(a[X], a[Y]);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_2_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_2_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(/=, _mm_div_epu32, v4i_swizzle)
    ML_SWIZZLE_2_OP(%=, v4u_mod, v4i_swizzle)
    ML_SWIZZLE_2_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_2_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_2_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_2_OP(^=, _mm_xor_si128, v4i_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z>
class v4u_swizzle3 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            uint32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4i_swizzle(vec, X, Y, Z, 3);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_3_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_3_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(/=, _mm_div_epu32, v4i_swizzle)
    ML_SWIZZLE_3_OP(%=, v4u_mod, v4i_swizzle)
    ML_SWIZZLE_3_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_3_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_3_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_3_OP(^=, _mm_xor_si128, v4i_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
class v4u_swizzle4 {
private:
    union {
        struct {
            v4i vec;
        };

        struct {
            uint32_t a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4i_swizzle(vec, X, Y, Z, W);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_4_OP(=, _mm_copy, v4i_swizzle)
    ML_SWIZZLE_4_OP(-=, _mm_sub_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(+=, _mm_add_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(*=, _mm_mullo_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(/=, _mm_div_epu32, v4i_swizzle)
    ML_SWIZZLE_4_OP(%=, v4u_mod, v4i_swizzle)
    ML_SWIZZLE_4_OP(<<=, _mm_sllv_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(>>=, _mm_srlv_epi32, v4i_swizzle)
    ML_SWIZZLE_4_OP(&=, _mm_and_si128, v4i_swizzle)
    ML_SWIZZLE_4_OP(|=, _mm_or_si128, v4i_swizzle)
    ML_SWIZZLE_4_OP(^=, _mm_xor_si128, v4i_swizzle)
};

// v4f

template <class C, uint32_t X, uint32_t Y>
class v4f_swizzle2 {
private:
    union {
        struct {
            v4f vec;
        };

        struct {
            float a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return C(a[X], a[Y]);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_2_OP(=, _mm_copy, v4f_swizzle)
    ML_SWIZZLE_2_OP(-=, _mm_sub_ps, v4f_swizzle)
    ML_SWIZZLE_2_OP(+=, _mm_add_ps, v4f_swizzle)
    ML_SWIZZLE_2_OP(*=, _mm_mul_ps, v4f_swizzle)
    ML_SWIZZLE_2_OP(/=, _mm_div_ps, v4f_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z>
class v4f_swizzle3 {
private:
    union {
        struct {
            v4f vec;
        };

        struct {
            float a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4f_swizzle(vec, X, Y, Z, 3);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_3_OP(=, _mm_copy, v4f_swizzle)
    ML_SWIZZLE_3_OP(-=, _mm_sub_ps, v4f_swizzle)
    ML_SWIZZLE_3_OP(+=, _mm_add_ps, v4f_swizzle)
    ML_SWIZZLE_3_OP(*=, _mm_mul_ps, v4f_swizzle)
    ML_SWIZZLE_3_OP(/=, _mm_div_ps, v4f_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
class v4f_swizzle4 {
private:
    union {
        struct {
            v4f vec;
        };

        struct {
            float a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4f_swizzle(vec, X, Y, Z, W);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_4_OP(=, _mm_copy, v4f_swizzle)
    ML_SWIZZLE_4_OP(-=, _mm_sub_ps, v4f_swizzle)
    ML_SWIZZLE_4_OP(+=, _mm_add_ps, v4f_swizzle)
    ML_SWIZZLE_4_OP(*=, _mm_mul_ps, v4f_swizzle)
    ML_SWIZZLE_4_OP(/=, _mm_div_ps, v4f_swizzle)
};

// v4d

template <class C, uint32_t X, uint32_t Y>
class v4d_swizzle2 {
private:
    union {
        struct {
            v4d vec;
        };

        struct {
            double a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return C(a[X], a[Y]);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_2_OP(=, _mm_copy, v4d_swizzle)
    ML_SWIZZLE_2_OP(-=, _mm256_sub_pd, v4d_swizzle)
    ML_SWIZZLE_2_OP(+=, _mm256_add_pd, v4d_swizzle)
    ML_SWIZZLE_2_OP(*=, _mm256_mul_pd, v4d_swizzle)
    ML_SWIZZLE_2_OP(/=, _mm256_div_pd, v4d_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z>
class v4d_swizzle3 {
private:
    union {
        struct {
            v4d vec;
        };

        struct {
            double a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4d_swizzle(vec, X, Y, Z, 3);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_3_OP(=, _mm_copy, v4d_swizzle)
    ML_SWIZZLE_3_OP(-=, _mm256_sub_pd, v4d_swizzle)
    ML_SWIZZLE_3_OP(+=, _mm256_add_pd, v4d_swizzle)
    ML_SWIZZLE_3_OP(*=, _mm256_mul_pd, v4d_swizzle)
    ML_SWIZZLE_3_OP(/=, _mm256_div_pd, v4d_swizzle)
};

template <class C, uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
class v4d_swizzle4 {
private:
    union {
        struct {
            v4d vec;
        };

        struct {
            double a[COORD_4D];
        };
    };

public:
    // Read-only: fast
    ML_INLINE operator C() const {
        return v4d_swizzle(vec, X, Y, Z, W);
    }

    // Read-write: most likely slow
    ML_SWIZZLE_4_OP(=, _mm_copy, v4d_swizzle)
    ML_SWIZZLE_4_OP(-=, _mm256_sub_pd, v4d_swizzle)
    ML_SWIZZLE_4_OP(+=, _mm256_add_pd, v4d_swizzle)
    ML_SWIZZLE_4_OP(*=, _mm256_mul_pd, v4d_swizzle)
    ML_SWIZZLE_4_OP(/=, _mm256_div_pd, v4d_swizzle)
};

#undef ML_SWIZZLE_2_OP
#undef ML_SWIZZLE_3_OP
#undef ML_SWIZZLE_4_OP

// swizzles

#define ML_SWIZZLE_2(C, T) \
    swizzle<C, T, _X, _X> xx; \
    swizzle<C, T, _X, _Y> xy; \
    swizzle<C, T, _Y, _X> yx; \
    swizzle<C, T, _Y, _Y> yy

#define ML_SWIZZLE_3(S2, C2, S3, C3) \
    S2<C2, _X, _X> xx; \
    S2<C2, _X, _Y> xy; \
    S2<C2, _X, _Z> xz; \
    S2<C2, _Y, _X> yx; \
    S2<C2, _Y, _Y> yy; \
    S2<C2, _Y, _Z> yz; \
    S2<C2, _Z, _X> zx; \
    S2<C2, _Z, _Y> zy; \
    S2<C2, _Z, _Z> zz; \
    S3<C3, _X, _X, _X> xxx; \
    S3<C3, _X, _X, _Y> xxy; \
    S3<C3, _X, _X, _Z> xxz; \
    S3<C3, _X, _Y, _X> xyx; \
    S3<C3, _X, _Y, _Y> xyy; \
    S3<C3, _X, _Y, _Z> xyz; \
    S3<C3, _X, _Z, _X> xzx; \
    S3<C3, _X, _Z, _Y> xzy; \
    S3<C3, _X, _Z, _Z> xzz; \
    S3<C3, _Y, _X, _X> yxx; \
    S3<C3, _Y, _X, _Y> yxy; \
    S3<C3, _Y, _X, _Z> yxz; \
    S3<C3, _Y, _Y, _X> yyx; \
    S3<C3, _Y, _Y, _Y> yyy; \
    S3<C3, _Y, _Y, _Z> yyz; \
    S3<C3, _Y, _Z, _X> yzx; \
    S3<C3, _Y, _Z, _Y> yzy; \
    S3<C3, _Y, _Z, _Z> yzz; \
    S3<C3, _Z, _X, _X> zxx; \
    S3<C3, _Z, _X, _Y> zxy; \
    S3<C3, _Z, _X, _Z> zxz; \
    S3<C3, _Z, _Y, _X> zyx; \
    S3<C3, _Z, _Y, _Y> zyy; \
    S3<C3, _Z, _Y, _Z> zyz; \
    S3<C3, _Z, _Z, _X> zzx; \
    S3<C3, _Z, _Z, _Y> zzy; \
    S3<C3, _Z, _Z, _Z> zzz

#define ML_SWIZZLE_4(S2, C2, S3, C3, S4, C4) \
    S2<C2, _X, _X> xx; \
    S2<C2, _X, _Y> xy; \
    S2<C2, _X, _Z> xz; \
    S2<C2, _X, _W> xw; \
    S2<C2, _Y, _X> yx; \
    S2<C2, _Y, _Y> yy; \
    S2<C2, _Y, _Z> yz; \
    S2<C2, _Y, _W> yw; \
    S2<C2, _Z, _X> zx; \
    S2<C2, _Z, _Y> zy; \
    S2<C2, _Z, _Z> zz; \
    S2<C2, _Z, _W> zw; \
    S2<C2, _W, _X> wx; \
    S2<C2, _W, _Y> wy; \
    S2<C2, _W, _Z> wz; \
    S2<C2, _W, _W> ww; \
    S3<C3, _X, _X, _X> xxx; \
    S3<C3, _X, _X, _Y> xxy; \
    S3<C3, _X, _X, _Z> xxz; \
    S3<C3, _X, _X, _W> xxw; \
    S3<C3, _X, _Y, _X> xyx; \
    S3<C3, _X, _Y, _Y> xyy; \
    S3<C3, _X, _Y, _Z> xyz; \
    S3<C3, _X, _Y, _W> xyw; \
    S3<C3, _X, _Z, _X> xzx; \
    S3<C3, _X, _Z, _Y> xzy; \
    S3<C3, _X, _Z, _Z> xzz; \
    S3<C3, _X, _Z, _W> xzw; \
    S3<C3, _X, _W, _X> xwx; \
    S3<C3, _X, _W, _Y> xwy; \
    S3<C3, _X, _W, _Z> xwz; \
    S3<C3, _X, _W, _W> xww; \
    S3<C3, _Y, _X, _X> yxx; \
    S3<C3, _Y, _X, _Y> yxy; \
    S3<C3, _Y, _X, _Z> yxz; \
    S3<C3, _Y, _X, _W> yxw; \
    S3<C3, _Y, _Y, _X> yyx; \
    S3<C3, _Y, _Y, _Y> yyy; \
    S3<C3, _Y, _Y, _Z> yyz; \
    S3<C3, _Y, _Y, _W> yyw; \
    S3<C3, _Y, _Z, _X> yzx; \
    S3<C3, _Y, _Z, _Y> yzy; \
    S3<C3, _Y, _Z, _Z> yzz; \
    S3<C3, _Y, _Z, _W> yzw; \
    S3<C3, _Y, _W, _X> ywx; \
    S3<C3, _Y, _W, _Y> ywy; \
    S3<C3, _Y, _W, _Z> ywz; \
    S3<C3, _Y, _W, _W> yww; \
    S3<C3, _Z, _X, _X> zxx; \
    S3<C3, _Z, _X, _Y> zxy; \
    S3<C3, _Z, _X, _Z> zxz; \
    S3<C3, _Z, _X, _W> zxw; \
    S3<C3, _Z, _Y, _X> zyx; \
    S3<C3, _Z, _Y, _Y> zyy; \
    S3<C3, _Z, _Y, _Z> zyz; \
    S3<C3, _Z, _Y, _W> zyw; \
    S3<C3, _Z, _Z, _X> zzx; \
    S3<C3, _Z, _Z, _Y> zzy; \
    S3<C3, _Z, _Z, _Z> zzz; \
    S3<C3, _Z, _Z, _W> zzw; \
    S3<C3, _Z, _W, _X> zwx; \
    S3<C3, _Z, _W, _Y> zwy; \
    S3<C3, _Z, _W, _Z> zwz; \
    S3<C3, _Z, _W, _W> zww; \
    S3<C3, _W, _X, _X> wxx; \
    S3<C3, _W, _X, _Y> wxy; \
    S3<C3, _W, _X, _Z> wxz; \
    S3<C3, _W, _X, _W> wxw; \
    S3<C3, _W, _Y, _X> wyx; \
    S3<C3, _W, _Y, _Y> wyy; \
    S3<C3, _W, _Y, _Z> wyz; \
    S3<C3, _W, _Y, _W> wyw; \
    S3<C3, _W, _Z, _X> wzx; \
    S3<C3, _W, _Z, _Y> wzy; \
    S3<C3, _W, _Z, _Z> wzz; \
    S3<C3, _W, _Z, _W> wzw; \
    S3<C3, _W, _W, _X> wwx; \
    S3<C3, _W, _W, _Y> wwy; \
    S3<C3, _W, _W, _Z> wwz; \
    S3<C3, _W, _W, _W> www; \
    S4<C4, _X, _X, _X, _X> xxxx; \
    S4<C4, _X, _X, _X, _Y> xxxy; \
    S4<C4, _X, _X, _X, _Z> xxxz; \
    S4<C4, _X, _X, _X, _W> xxxw; \
    S4<C4, _X, _X, _Y, _X> xxyx; \
    S4<C4, _X, _X, _Y, _Y> xxyy; \
    S4<C4, _X, _X, _Y, _Z> xxyz; \
    S4<C4, _X, _X, _Y, _W> xxyw; \
    S4<C4, _X, _X, _Z, _X> xxzx; \
    S4<C4, _X, _X, _Z, _Y> xxzy; \
    S4<C4, _X, _X, _Z, _Z> xxzz; \
    S4<C4, _X, _X, _Z, _W> xxzw; \
    S4<C4, _X, _X, _W, _X> xxwx; \
    S4<C4, _X, _X, _W, _Y> xxwy; \
    S4<C4, _X, _X, _W, _Z> xxwz; \
    S4<C4, _X, _X, _W, _W> xxww; \
    S4<C4, _X, _Y, _X, _X> xyxx; \
    S4<C4, _X, _Y, _X, _Y> xyxy; \
    S4<C4, _X, _Y, _X, _Z> xyxz; \
    S4<C4, _X, _Y, _X, _W> xyxw; \
    S4<C4, _X, _Y, _Y, _X> xyyx; \
    S4<C4, _X, _Y, _Y, _Y> xyyy; \
    S4<C4, _X, _Y, _Y, _Z> xyyz; \
    S4<C4, _X, _Y, _Y, _W> xyyw; \
    S4<C4, _X, _Y, _Z, _X> xyzx; \
    S4<C4, _X, _Y, _Z, _Y> xyzy; \
    S4<C4, _X, _Y, _Z, _Z> xyzz; \
    S4<C4, _X, _Y, _Z, _W> xyzw; \
    S4<C4, _X, _Y, _W, _X> xywx; \
    S4<C4, _X, _Y, _W, _Y> xywy; \
    S4<C4, _X, _Y, _W, _Z> xywz; \
    S4<C4, _X, _Y, _W, _W> xyww; \
    S4<C4, _X, _Z, _X, _X> xzxx; \
    S4<C4, _X, _Z, _X, _Y> xzxy; \
    S4<C4, _X, _Z, _X, _Z> xzxz; \
    S4<C4, _X, _Z, _X, _W> xzxw; \
    S4<C4, _X, _Z, _Y, _X> xzyx; \
    S4<C4, _X, _Z, _Y, _Y> xzyy; \
    S4<C4, _X, _Z, _Y, _Z> xzyz; \
    S4<C4, _X, _Z, _Y, _W> xzyw; \
    S4<C4, _X, _Z, _Z, _X> xzzx; \
    S4<C4, _X, _Z, _Z, _Y> xzzy; \
    S4<C4, _X, _Z, _Z, _Z> xzzz; \
    S4<C4, _X, _Z, _Z, _W> xzzw; \
    S4<C4, _X, _Z, _W, _X> xzwx; \
    S4<C4, _X, _Z, _W, _Y> xzwy; \
    S4<C4, _X, _Z, _W, _Z> xzwz; \
    S4<C4, _X, _Z, _W, _W> xzww; \
    S4<C4, _X, _W, _X, _X> xwxx; \
    S4<C4, _X, _W, _X, _Y> xwxy; \
    S4<C4, _X, _W, _X, _Z> xwxz; \
    S4<C4, _X, _W, _X, _W> xwxw; \
    S4<C4, _X, _W, _Y, _X> xwyx; \
    S4<C4, _X, _W, _Y, _Y> xwyy; \
    S4<C4, _X, _W, _Y, _Z> xwyz; \
    S4<C4, _X, _W, _Y, _W> xwyw; \
    S4<C4, _X, _W, _Z, _X> xwzx; \
    S4<C4, _X, _W, _Z, _Y> xwzy; \
    S4<C4, _X, _W, _Z, _Z> xwzz; \
    S4<C4, _X, _W, _Z, _W> xwzw; \
    S4<C4, _X, _W, _W, _X> xwwx; \
    S4<C4, _X, _W, _W, _Y> xwwy; \
    S4<C4, _X, _W, _W, _Z> xwwz; \
    S4<C4, _X, _W, _W, _W> xwww; \
    S4<C4, _Y, _X, _X, _X> yxxx; \
    S4<C4, _Y, _X, _X, _Y> yxxy; \
    S4<C4, _Y, _X, _X, _Z> yxxz; \
    S4<C4, _Y, _X, _X, _W> yxxw; \
    S4<C4, _Y, _X, _Y, _X> yxyx; \
    S4<C4, _Y, _X, _Y, _Y> yxyy; \
    S4<C4, _Y, _X, _Y, _Z> yxyz; \
    S4<C4, _Y, _X, _Y, _W> yxyw; \
    S4<C4, _Y, _X, _Z, _X> yxzx; \
    S4<C4, _Y, _X, _Z, _Y> yxzy; \
    S4<C4, _Y, _X, _Z, _Z> yxzz; \
    S4<C4, _Y, _X, _Z, _W> yxzw; \
    S4<C4, _Y, _X, _W, _X> yxwx; \
    S4<C4, _Y, _X, _W, _Y> yxwy; \
    S4<C4, _Y, _X, _W, _Z> yxwz; \
    S4<C4, _Y, _X, _W, _W> yxww; \
    S4<C4, _Y, _Y, _X, _X> yyxx; \
    S4<C4, _Y, _Y, _X, _Y> yyxy; \
    S4<C4, _Y, _Y, _X, _Z> yyxz; \
    S4<C4, _Y, _Y, _X, _W> yyxw; \
    S4<C4, _Y, _Y, _Y, _X> yyyx; \
    S4<C4, _Y, _Y, _Y, _Y> yyyy; \
    S4<C4, _Y, _Y, _Y, _Z> yyyz; \
    S4<C4, _Y, _Y, _Y, _W> yyyw; \
    S4<C4, _Y, _Y, _Z, _X> yyzx; \
    S4<C4, _Y, _Y, _Z, _Y> yyzy; \
    S4<C4, _Y, _Y, _Z, _Z> yyzz; \
    S4<C4, _Y, _Y, _Z, _W> yyzw; \
    S4<C4, _Y, _Y, _W, _X> yywx; \
    S4<C4, _Y, _Y, _W, _Y> yywy; \
    S4<C4, _Y, _Y, _W, _Z> yywz; \
    S4<C4, _Y, _Y, _W, _W> yyww; \
    S4<C4, _Y, _Z, _X, _X> yzxx; \
    S4<C4, _Y, _Z, _X, _Y> yzxy; \
    S4<C4, _Y, _Z, _X, _Z> yzxz; \
    S4<C4, _Y, _Z, _X, _W> yzxw; \
    S4<C4, _Y, _Z, _Y, _X> yzyx; \
    S4<C4, _Y, _Z, _Y, _Y> yzyy; \
    S4<C4, _Y, _Z, _Y, _Z> yzyz; \
    S4<C4, _Y, _Z, _Y, _W> yzyw; \
    S4<C4, _Y, _Z, _Z, _X> yzzx; \
    S4<C4, _Y, _Z, _Z, _Y> yzzy; \
    S4<C4, _Y, _Z, _Z, _Z> yzzz; \
    S4<C4, _Y, _Z, _Z, _W> yzzw; \
    S4<C4, _Y, _Z, _W, _X> yzwx; \
    S4<C4, _Y, _Z, _W, _Y> yzwy; \
    S4<C4, _Y, _Z, _W, _Z> yzwz; \
    S4<C4, _Y, _Z, _W, _W> yzww; \
    S4<C4, _Y, _W, _X, _X> ywxx; \
    S4<C4, _Y, _W, _X, _Y> ywxy; \
    S4<C4, _Y, _W, _X, _Z> ywxz; \
    S4<C4, _Y, _W, _X, _W> ywxw; \
    S4<C4, _Y, _W, _Y, _X> ywyx; \
    S4<C4, _Y, _W, _Y, _Y> ywyy; \
    S4<C4, _Y, _W, _Y, _Z> ywyz; \
    S4<C4, _Y, _W, _Y, _W> ywyw; \
    S4<C4, _Y, _W, _Z, _X> ywzx; \
    S4<C4, _Y, _W, _Z, _Y> ywzy; \
    S4<C4, _Y, _W, _Z, _Z> ywzz; \
    S4<C4, _Y, _W, _Z, _W> ywzw; \
    S4<C4, _Y, _W, _W, _X> ywwx; \
    S4<C4, _Y, _W, _W, _Y> ywwy; \
    S4<C4, _Y, _W, _W, _Z> ywwz; \
    S4<C4, _Y, _W, _W, _W> ywww; \
    S4<C4, _Z, _X, _X, _X> zxxx; \
    S4<C4, _Z, _X, _X, _Y> zxxy; \
    S4<C4, _Z, _X, _X, _Z> zxxz; \
    S4<C4, _Z, _X, _X, _W> zxxw; \
    S4<C4, _Z, _X, _Y, _X> zxyx; \
    S4<C4, _Z, _X, _Y, _Y> zxyy; \
    S4<C4, _Z, _X, _Y, _Z> zxyz; \
    S4<C4, _Z, _X, _Y, _W> zxyw; \
    S4<C4, _Z, _X, _Z, _X> zxzx; \
    S4<C4, _Z, _X, _Z, _Y> zxzy; \
    S4<C4, _Z, _X, _Z, _Z> zxzz; \
    S4<C4, _Z, _X, _Z, _W> zxzw; \
    S4<C4, _Z, _X, _W, _X> zxwx; \
    S4<C4, _Z, _X, _W, _Y> zxwy; \
    S4<C4, _Z, _X, _W, _Z> zxwz; \
    S4<C4, _Z, _X, _W, _W> zxww; \
    S4<C4, _Z, _Y, _X, _X> zyxx; \
    S4<C4, _Z, _Y, _X, _Y> zyxy; \
    S4<C4, _Z, _Y, _X, _Z> zyxz; \
    S4<C4, _Z, _Y, _X, _W> zyxw; \
    S4<C4, _Z, _Y, _Y, _X> zyyx; \
    S4<C4, _Z, _Y, _Y, _Y> zyyy; \
    S4<C4, _Z, _Y, _Y, _Z> zyyz; \
    S4<C4, _Z, _Y, _Y, _W> zyyw; \
    S4<C4, _Z, _Y, _Z, _X> zyzx; \
    S4<C4, _Z, _Y, _Z, _Y> zyzy; \
    S4<C4, _Z, _Y, _Z, _Z> zyzz; \
    S4<C4, _Z, _Y, _Z, _W> zyzw; \
    S4<C4, _Z, _Y, _W, _X> zywx; \
    S4<C4, _Z, _Y, _W, _Y> zywy; \
    S4<C4, _Z, _Y, _W, _Z> zywz; \
    S4<C4, _Z, _Y, _W, _W> zyww; \
    S4<C4, _Z, _Z, _X, _X> zzxx; \
    S4<C4, _Z, _Z, _X, _Y> zzxy; \
    S4<C4, _Z, _Z, _X, _Z> zzxz; \
    S4<C4, _Z, _Z, _X, _W> zzxw; \
    S4<C4, _Z, _Z, _Y, _X> zzyx; \
    S4<C4, _Z, _Z, _Y, _Y> zzyy; \
    S4<C4, _Z, _Z, _Y, _Z> zzyz; \
    S4<C4, _Z, _Z, _Y, _W> zzyw; \
    S4<C4, _Z, _Z, _Z, _X> zzzx; \
    S4<C4, _Z, _Z, _Z, _Y> zzzy; \
    S4<C4, _Z, _Z, _Z, _Z> zzzz; \
    S4<C4, _Z, _Z, _Z, _W> zzzw; \
    S4<C4, _Z, _Z, _W, _X> zzwx; \
    S4<C4, _Z, _Z, _W, _Y> zzwy; \
    S4<C4, _Z, _Z, _W, _Z> zzwz; \
    S4<C4, _Z, _Z, _W, _W> zzww; \
    S4<C4, _Z, _W, _X, _X> zwxx; \
    S4<C4, _Z, _W, _X, _Y> zwxy; \
    S4<C4, _Z, _W, _X, _Z> zwxz; \
    S4<C4, _Z, _W, _X, _W> zwxw; \
    S4<C4, _Z, _W, _Y, _X> zwyx; \
    S4<C4, _Z, _W, _Y, _Y> zwyy; \
    S4<C4, _Z, _W, _Y, _Z> zwyz; \
    S4<C4, _Z, _W, _Y, _W> zwyw; \
    S4<C4, _Z, _W, _Z, _X> zwzx; \
    S4<C4, _Z, _W, _Z, _Y> zwzy; \
    S4<C4, _Z, _W, _Z, _Z> zwzz; \
    S4<C4, _Z, _W, _Z, _W> zwzw; \
    S4<C4, _Z, _W, _W, _X> zwwx; \
    S4<C4, _Z, _W, _W, _Y> zwwy; \
    S4<C4, _Z, _W, _W, _Z> zwwz; \
    S4<C4, _Z, _W, _W, _W> zwww; \
    S4<C4, _W, _X, _X, _X> wxxx; \
    S4<C4, _W, _X, _X, _Y> wxxy; \
    S4<C4, _W, _X, _X, _Z> wxxz; \
    S4<C4, _W, _X, _X, _W> wxxw; \
    S4<C4, _W, _X, _Y, _X> wxyx; \
    S4<C4, _W, _X, _Y, _Y> wxyy; \
    S4<C4, _W, _X, _Y, _Z> wxyz; \
    S4<C4, _W, _X, _Y, _W> wxyw; \
    S4<C4, _W, _X, _Z, _X> wxzx; \
    S4<C4, _W, _X, _Z, _Y> wxzy; \
    S4<C4, _W, _X, _Z, _Z> wxzz; \
    S4<C4, _W, _X, _Z, _W> wxzw; \
    S4<C4, _W, _X, _W, _X> wxwx; \
    S4<C4, _W, _X, _W, _Y> wxwy; \
    S4<C4, _W, _X, _W, _Z> wxwz; \
    S4<C4, _W, _X, _W, _W> wxww; \
    S4<C4, _W, _Y, _X, _X> wyxx; \
    S4<C4, _W, _Y, _X, _Y> wyxy; \
    S4<C4, _W, _Y, _X, _Z> wyxz; \
    S4<C4, _W, _Y, _X, _W> wyxw; \
    S4<C4, _W, _Y, _Y, _X> wyyx; \
    S4<C4, _W, _Y, _Y, _Y> wyyy; \
    S4<C4, _W, _Y, _Y, _Z> wyyz; \
    S4<C4, _W, _Y, _Y, _W> wyyw; \
    S4<C4, _W, _Y, _Z, _X> wyzx; \
    S4<C4, _W, _Y, _Z, _Y> wyzy; \
    S4<C4, _W, _Y, _Z, _Z> wyzz; \
    S4<C4, _W, _Y, _Z, _W> wyzw; \
    S4<C4, _W, _Y, _W, _X> wywx; \
    S4<C4, _W, _Y, _W, _Y> wywy; \
    S4<C4, _W, _Y, _W, _Z> wywz; \
    S4<C4, _W, _Y, _W, _W> wyww; \
    S4<C4, _W, _Z, _X, _X> wzxx; \
    S4<C4, _W, _Z, _X, _Y> wzxy; \
    S4<C4, _W, _Z, _X, _Z> wzxz; \
    S4<C4, _W, _Z, _X, _W> wzxw; \
    S4<C4, _W, _Z, _Y, _X> wzyx; \
    S4<C4, _W, _Z, _Y, _Y> wzyy; \
    S4<C4, _W, _Z, _Y, _Z> wzyz; \
    S4<C4, _W, _Z, _Y, _W> wzyw; \
    S4<C4, _W, _Z, _Z, _X> wzzx; \
    S4<C4, _W, _Z, _Z, _Y> wzzy; \
    S4<C4, _W, _Z, _Z, _Z> wzzz; \
    S4<C4, _W, _Z, _Z, _W> wzzw; \
    S4<C4, _W, _Z, _W, _X> wzwx; \
    S4<C4, _W, _Z, _W, _Y> wzwy; \
    S4<C4, _W, _Z, _W, _Z> wzwz; \
    S4<C4, _W, _Z, _W, _W> wzww; \
    S4<C4, _W, _W, _X, _X> wwxx; \
    S4<C4, _W, _W, _X, _Y> wwxy; \
    S4<C4, _W, _W, _X, _Z> wwxz; \
    S4<C4, _W, _W, _X, _W> wwxw; \
    S4<C4, _W, _W, _Y, _X> wwyx; \
    S4<C4, _W, _W, _Y, _Y> wwyy; \
    S4<C4, _W, _W, _Y, _Z> wwyz; \
    S4<C4, _W, _W, _Y, _W> wwyw; \
    S4<C4, _W, _W, _Z, _X> wwzx; \
    S4<C4, _W, _W, _Z, _Y> wwzy; \
    S4<C4, _W, _W, _Z, _Z> wwzz; \
    S4<C4, _W, _W, _Z, _W> wwzw; \
    S4<C4, _W, _W, _W, _X> wwwx; \
    S4<C4, _W, _W, _W, _Y> wwwy; \
    S4<C4, _W, _W, _W, _Z> wwwz; \
    S4<C4, _W, _W, _W, _W> wwww
