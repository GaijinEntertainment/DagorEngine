// © 2021 NVIDIA Corporation

#pragma once

#define ML_X 0
#define ML_Y 1
#define ML_Z 2
#define ML_W 3

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
    swizzle<C, T, ML_X, ML_X> xx; \
    swizzle<C, T, ML_X, ML_Y> xy; \
    swizzle<C, T, ML_Y, ML_X> yx; \
    swizzle<C, T, ML_Y, ML_Y> yy

#define ML_SWIZZLE_3(S2, C2, S3, C3) \
    S2<C2, ML_X, ML_X> xx; \
    S2<C2, ML_X, ML_Y> xy; \
    S2<C2, ML_X, ML_Z> xz; \
    S2<C2, ML_Y, ML_X> yx; \
    S2<C2, ML_Y, ML_Y> yy; \
    S2<C2, ML_Y, ML_Z> yz; \
    S2<C2, ML_Z, ML_X> zx; \
    S2<C2, ML_Z, ML_Y> zy; \
    S2<C2, ML_Z, ML_Z> zz; \
    S3<C3, ML_X, ML_X, ML_X> xxx; \
    S3<C3, ML_X, ML_X, ML_Y> xxy; \
    S3<C3, ML_X, ML_X, ML_Z> xxz; \
    S3<C3, ML_X, ML_Y, ML_X> xyx; \
    S3<C3, ML_X, ML_Y, ML_Y> xyy; \
    S3<C3, ML_X, ML_Y, ML_Z> xyz; \
    S3<C3, ML_X, ML_Z, ML_X> xzx; \
    S3<C3, ML_X, ML_Z, ML_Y> xzy; \
    S3<C3, ML_X, ML_Z, ML_Z> xzz; \
    S3<C3, ML_Y, ML_X, ML_X> yxx; \
    S3<C3, ML_Y, ML_X, ML_Y> yxy; \
    S3<C3, ML_Y, ML_X, ML_Z> yxz; \
    S3<C3, ML_Y, ML_Y, ML_X> yyx; \
    S3<C3, ML_Y, ML_Y, ML_Y> yyy; \
    S3<C3, ML_Y, ML_Y, ML_Z> yyz; \
    S3<C3, ML_Y, ML_Z, ML_X> yzx; \
    S3<C3, ML_Y, ML_Z, ML_Y> yzy; \
    S3<C3, ML_Y, ML_Z, ML_Z> yzz; \
    S3<C3, ML_Z, ML_X, ML_X> zxx; \
    S3<C3, ML_Z, ML_X, ML_Y> zxy; \
    S3<C3, ML_Z, ML_X, ML_Z> zxz; \
    S3<C3, ML_Z, ML_Y, ML_X> zyx; \
    S3<C3, ML_Z, ML_Y, ML_Y> zyy; \
    S3<C3, ML_Z, ML_Y, ML_Z> zyz; \
    S3<C3, ML_Z, ML_Z, ML_X> zzx; \
    S3<C3, ML_Z, ML_Z, ML_Y> zzy; \
    S3<C3, ML_Z, ML_Z, ML_Z> zzz

#define ML_SWIZZLE_4(S2, C2, S3, C3, S4, C4) \
    S2<C2, ML_X, ML_X> xx; \
    S2<C2, ML_X, ML_Y> xy; \
    S2<C2, ML_X, ML_Z> xz; \
    S2<C2, ML_X, ML_W> xw; \
    S2<C2, ML_Y, ML_X> yx; \
    S2<C2, ML_Y, ML_Y> yy; \
    S2<C2, ML_Y, ML_Z> yz; \
    S2<C2, ML_Y, ML_W> yw; \
    S2<C2, ML_Z, ML_X> zx; \
    S2<C2, ML_Z, ML_Y> zy; \
    S2<C2, ML_Z, ML_Z> zz; \
    S2<C2, ML_Z, ML_W> zw; \
    S2<C2, ML_W, ML_X> wx; \
    S2<C2, ML_W, ML_Y> wy; \
    S2<C2, ML_W, ML_Z> wz; \
    S2<C2, ML_W, ML_W> ww; \
    S3<C3, ML_X, ML_X, ML_X> xxx; \
    S3<C3, ML_X, ML_X, ML_Y> xxy; \
    S3<C3, ML_X, ML_X, ML_Z> xxz; \
    S3<C3, ML_X, ML_X, ML_W> xxw; \
    S3<C3, ML_X, ML_Y, ML_X> xyx; \
    S3<C3, ML_X, ML_Y, ML_Y> xyy; \
    S3<C3, ML_X, ML_Y, ML_Z> xyz; \
    S3<C3, ML_X, ML_Y, ML_W> xyw; \
    S3<C3, ML_X, ML_Z, ML_X> xzx; \
    S3<C3, ML_X, ML_Z, ML_Y> xzy; \
    S3<C3, ML_X, ML_Z, ML_Z> xzz; \
    S3<C3, ML_X, ML_Z, ML_W> xzw; \
    S3<C3, ML_X, ML_W, ML_X> xwx; \
    S3<C3, ML_X, ML_W, ML_Y> xwy; \
    S3<C3, ML_X, ML_W, ML_Z> xwz; \
    S3<C3, ML_X, ML_W, ML_W> xww; \
    S3<C3, ML_Y, ML_X, ML_X> yxx; \
    S3<C3, ML_Y, ML_X, ML_Y> yxy; \
    S3<C3, ML_Y, ML_X, ML_Z> yxz; \
    S3<C3, ML_Y, ML_X, ML_W> yxw; \
    S3<C3, ML_Y, ML_Y, ML_X> yyx; \
    S3<C3, ML_Y, ML_Y, ML_Y> yyy; \
    S3<C3, ML_Y, ML_Y, ML_Z> yyz; \
    S3<C3, ML_Y, ML_Y, ML_W> yyw; \
    S3<C3, ML_Y, ML_Z, ML_X> yzx; \
    S3<C3, ML_Y, ML_Z, ML_Y> yzy; \
    S3<C3, ML_Y, ML_Z, ML_Z> yzz; \
    S3<C3, ML_Y, ML_Z, ML_W> yzw; \
    S3<C3, ML_Y, ML_W, ML_X> ywx; \
    S3<C3, ML_Y, ML_W, ML_Y> ywy; \
    S3<C3, ML_Y, ML_W, ML_Z> ywz; \
    S3<C3, ML_Y, ML_W, ML_W> yww; \
    S3<C3, ML_Z, ML_X, ML_X> zxx; \
    S3<C3, ML_Z, ML_X, ML_Y> zxy; \
    S3<C3, ML_Z, ML_X, ML_Z> zxz; \
    S3<C3, ML_Z, ML_X, ML_W> zxw; \
    S3<C3, ML_Z, ML_Y, ML_X> zyx; \
    S3<C3, ML_Z, ML_Y, ML_Y> zyy; \
    S3<C3, ML_Z, ML_Y, ML_Z> zyz; \
    S3<C3, ML_Z, ML_Y, ML_W> zyw; \
    S3<C3, ML_Z, ML_Z, ML_X> zzx; \
    S3<C3, ML_Z, ML_Z, ML_Y> zzy; \
    S3<C3, ML_Z, ML_Z, ML_Z> zzz; \
    S3<C3, ML_Z, ML_Z, ML_W> zzw; \
    S3<C3, ML_Z, ML_W, ML_X> zwx; \
    S3<C3, ML_Z, ML_W, ML_Y> zwy; \
    S3<C3, ML_Z, ML_W, ML_Z> zwz; \
    S3<C3, ML_Z, ML_W, ML_W> zww; \
    S3<C3, ML_W, ML_X, ML_X> wxx; \
    S3<C3, ML_W, ML_X, ML_Y> wxy; \
    S3<C3, ML_W, ML_X, ML_Z> wxz; \
    S3<C3, ML_W, ML_X, ML_W> wxw; \
    S3<C3, ML_W, ML_Y, ML_X> wyx; \
    S3<C3, ML_W, ML_Y, ML_Y> wyy; \
    S3<C3, ML_W, ML_Y, ML_Z> wyz; \
    S3<C3, ML_W, ML_Y, ML_W> wyw; \
    S3<C3, ML_W, ML_Z, ML_X> wzx; \
    S3<C3, ML_W, ML_Z, ML_Y> wzy; \
    S3<C3, ML_W, ML_Z, ML_Z> wzz; \
    S3<C3, ML_W, ML_Z, ML_W> wzw; \
    S3<C3, ML_W, ML_W, ML_X> wwx; \
    S3<C3, ML_W, ML_W, ML_Y> wwy; \
    S3<C3, ML_W, ML_W, ML_Z> wwz; \
    S3<C3, ML_W, ML_W, ML_W> www; \
    S4<C4, ML_X, ML_X, ML_X, ML_X> xxxx; \
    S4<C4, ML_X, ML_X, ML_X, ML_Y> xxxy; \
    S4<C4, ML_X, ML_X, ML_X, ML_Z> xxxz; \
    S4<C4, ML_X, ML_X, ML_X, ML_W> xxxw; \
    S4<C4, ML_X, ML_X, ML_Y, ML_X> xxyx; \
    S4<C4, ML_X, ML_X, ML_Y, ML_Y> xxyy; \
    S4<C4, ML_X, ML_X, ML_Y, ML_Z> xxyz; \
    S4<C4, ML_X, ML_X, ML_Y, ML_W> xxyw; \
    S4<C4, ML_X, ML_X, ML_Z, ML_X> xxzx; \
    S4<C4, ML_X, ML_X, ML_Z, ML_Y> xxzy; \
    S4<C4, ML_X, ML_X, ML_Z, ML_Z> xxzz; \
    S4<C4, ML_X, ML_X, ML_Z, ML_W> xxzw; \
    S4<C4, ML_X, ML_X, ML_W, ML_X> xxwx; \
    S4<C4, ML_X, ML_X, ML_W, ML_Y> xxwy; \
    S4<C4, ML_X, ML_X, ML_W, ML_Z> xxwz; \
    S4<C4, ML_X, ML_X, ML_W, ML_W> xxww; \
    S4<C4, ML_X, ML_Y, ML_X, ML_X> xyxx; \
    S4<C4, ML_X, ML_Y, ML_X, ML_Y> xyxy; \
    S4<C4, ML_X, ML_Y, ML_X, ML_Z> xyxz; \
    S4<C4, ML_X, ML_Y, ML_X, ML_W> xyxw; \
    S4<C4, ML_X, ML_Y, ML_Y, ML_X> xyyx; \
    S4<C4, ML_X, ML_Y, ML_Y, ML_Y> xyyy; \
    S4<C4, ML_X, ML_Y, ML_Y, ML_Z> xyyz; \
    S4<C4, ML_X, ML_Y, ML_Y, ML_W> xyyw; \
    S4<C4, ML_X, ML_Y, ML_Z, ML_X> xyzx; \
    S4<C4, ML_X, ML_Y, ML_Z, ML_Y> xyzy; \
    S4<C4, ML_X, ML_Y, ML_Z, ML_Z> xyzz; \
    S4<C4, ML_X, ML_Y, ML_Z, ML_W> xyzw; \
    S4<C4, ML_X, ML_Y, ML_W, ML_X> xywx; \
    S4<C4, ML_X, ML_Y, ML_W, ML_Y> xywy; \
    S4<C4, ML_X, ML_Y, ML_W, ML_Z> xywz; \
    S4<C4, ML_X, ML_Y, ML_W, ML_W> xyww; \
    S4<C4, ML_X, ML_Z, ML_X, ML_X> xzxx; \
    S4<C4, ML_X, ML_Z, ML_X, ML_Y> xzxy; \
    S4<C4, ML_X, ML_Z, ML_X, ML_Z> xzxz; \
    S4<C4, ML_X, ML_Z, ML_X, ML_W> xzxw; \
    S4<C4, ML_X, ML_Z, ML_Y, ML_X> xzyx; \
    S4<C4, ML_X, ML_Z, ML_Y, ML_Y> xzyy; \
    S4<C4, ML_X, ML_Z, ML_Y, ML_Z> xzyz; \
    S4<C4, ML_X, ML_Z, ML_Y, ML_W> xzyw; \
    S4<C4, ML_X, ML_Z, ML_Z, ML_X> xzzx; \
    S4<C4, ML_X, ML_Z, ML_Z, ML_Y> xzzy; \
    S4<C4, ML_X, ML_Z, ML_Z, ML_Z> xzzz; \
    S4<C4, ML_X, ML_Z, ML_Z, ML_W> xzzw; \
    S4<C4, ML_X, ML_Z, ML_W, ML_X> xzwx; \
    S4<C4, ML_X, ML_Z, ML_W, ML_Y> xzwy; \
    S4<C4, ML_X, ML_Z, ML_W, ML_Z> xzwz; \
    S4<C4, ML_X, ML_Z, ML_W, ML_W> xzww; \
    S4<C4, ML_X, ML_W, ML_X, ML_X> xwxx; \
    S4<C4, ML_X, ML_W, ML_X, ML_Y> xwxy; \
    S4<C4, ML_X, ML_W, ML_X, ML_Z> xwxz; \
    S4<C4, ML_X, ML_W, ML_X, ML_W> xwxw; \
    S4<C4, ML_X, ML_W, ML_Y, ML_X> xwyx; \
    S4<C4, ML_X, ML_W, ML_Y, ML_Y> xwyy; \
    S4<C4, ML_X, ML_W, ML_Y, ML_Z> xwyz; \
    S4<C4, ML_X, ML_W, ML_Y, ML_W> xwyw; \
    S4<C4, ML_X, ML_W, ML_Z, ML_X> xwzx; \
    S4<C4, ML_X, ML_W, ML_Z, ML_Y> xwzy; \
    S4<C4, ML_X, ML_W, ML_Z, ML_Z> xwzz; \
    S4<C4, ML_X, ML_W, ML_Z, ML_W> xwzw; \
    S4<C4, ML_X, ML_W, ML_W, ML_X> xwwx; \
    S4<C4, ML_X, ML_W, ML_W, ML_Y> xwwy; \
    S4<C4, ML_X, ML_W, ML_W, ML_Z> xwwz; \
    S4<C4, ML_X, ML_W, ML_W, ML_W> xwww; \
    S4<C4, ML_Y, ML_X, ML_X, ML_X> yxxx; \
    S4<C4, ML_Y, ML_X, ML_X, ML_Y> yxxy; \
    S4<C4, ML_Y, ML_X, ML_X, ML_Z> yxxz; \
    S4<C4, ML_Y, ML_X, ML_X, ML_W> yxxw; \
    S4<C4, ML_Y, ML_X, ML_Y, ML_X> yxyx; \
    S4<C4, ML_Y, ML_X, ML_Y, ML_Y> yxyy; \
    S4<C4, ML_Y, ML_X, ML_Y, ML_Z> yxyz; \
    S4<C4, ML_Y, ML_X, ML_Y, ML_W> yxyw; \
    S4<C4, ML_Y, ML_X, ML_Z, ML_X> yxzx; \
    S4<C4, ML_Y, ML_X, ML_Z, ML_Y> yxzy; \
    S4<C4, ML_Y, ML_X, ML_Z, ML_Z> yxzz; \
    S4<C4, ML_Y, ML_X, ML_Z, ML_W> yxzw; \
    S4<C4, ML_Y, ML_X, ML_W, ML_X> yxwx; \
    S4<C4, ML_Y, ML_X, ML_W, ML_Y> yxwy; \
    S4<C4, ML_Y, ML_X, ML_W, ML_Z> yxwz; \
    S4<C4, ML_Y, ML_X, ML_W, ML_W> yxww; \
    S4<C4, ML_Y, ML_Y, ML_X, ML_X> yyxx; \
    S4<C4, ML_Y, ML_Y, ML_X, ML_Y> yyxy; \
    S4<C4, ML_Y, ML_Y, ML_X, ML_Z> yyxz; \
    S4<C4, ML_Y, ML_Y, ML_X, ML_W> yyxw; \
    S4<C4, ML_Y, ML_Y, ML_Y, ML_X> yyyx; \
    S4<C4, ML_Y, ML_Y, ML_Y, ML_Y> yyyy; \
    S4<C4, ML_Y, ML_Y, ML_Y, ML_Z> yyyz; \
    S4<C4, ML_Y, ML_Y, ML_Y, ML_W> yyyw; \
    S4<C4, ML_Y, ML_Y, ML_Z, ML_X> yyzx; \
    S4<C4, ML_Y, ML_Y, ML_Z, ML_Y> yyzy; \
    S4<C4, ML_Y, ML_Y, ML_Z, ML_Z> yyzz; \
    S4<C4, ML_Y, ML_Y, ML_Z, ML_W> yyzw; \
    S4<C4, ML_Y, ML_Y, ML_W, ML_X> yywx; \
    S4<C4, ML_Y, ML_Y, ML_W, ML_Y> yywy; \
    S4<C4, ML_Y, ML_Y, ML_W, ML_Z> yywz; \
    S4<C4, ML_Y, ML_Y, ML_W, ML_W> yyww; \
    S4<C4, ML_Y, ML_Z, ML_X, ML_X> yzxx; \
    S4<C4, ML_Y, ML_Z, ML_X, ML_Y> yzxy; \
    S4<C4, ML_Y, ML_Z, ML_X, ML_Z> yzxz; \
    S4<C4, ML_Y, ML_Z, ML_X, ML_W> yzxw; \
    S4<C4, ML_Y, ML_Z, ML_Y, ML_X> yzyx; \
    S4<C4, ML_Y, ML_Z, ML_Y, ML_Y> yzyy; \
    S4<C4, ML_Y, ML_Z, ML_Y, ML_Z> yzyz; \
    S4<C4, ML_Y, ML_Z, ML_Y, ML_W> yzyw; \
    S4<C4, ML_Y, ML_Z, ML_Z, ML_X> yzzx; \
    S4<C4, ML_Y, ML_Z, ML_Z, ML_Y> yzzy; \
    S4<C4, ML_Y, ML_Z, ML_Z, ML_Z> yzzz; \
    S4<C4, ML_Y, ML_Z, ML_Z, ML_W> yzzw; \
    S4<C4, ML_Y, ML_Z, ML_W, ML_X> yzwx; \
    S4<C4, ML_Y, ML_Z, ML_W, ML_Y> yzwy; \
    S4<C4, ML_Y, ML_Z, ML_W, ML_Z> yzwz; \
    S4<C4, ML_Y, ML_Z, ML_W, ML_W> yzww; \
    S4<C4, ML_Y, ML_W, ML_X, ML_X> ywxx; \
    S4<C4, ML_Y, ML_W, ML_X, ML_Y> ywxy; \
    S4<C4, ML_Y, ML_W, ML_X, ML_Z> ywxz; \
    S4<C4, ML_Y, ML_W, ML_X, ML_W> ywxw; \
    S4<C4, ML_Y, ML_W, ML_Y, ML_X> ywyx; \
    S4<C4, ML_Y, ML_W, ML_Y, ML_Y> ywyy; \
    S4<C4, ML_Y, ML_W, ML_Y, ML_Z> ywyz; \
    S4<C4, ML_Y, ML_W, ML_Y, ML_W> ywyw; \
    S4<C4, ML_Y, ML_W, ML_Z, ML_X> ywzx; \
    S4<C4, ML_Y, ML_W, ML_Z, ML_Y> ywzy; \
    S4<C4, ML_Y, ML_W, ML_Z, ML_Z> ywzz; \
    S4<C4, ML_Y, ML_W, ML_Z, ML_W> ywzw; \
    S4<C4, ML_Y, ML_W, ML_W, ML_X> ywwx; \
    S4<C4, ML_Y, ML_W, ML_W, ML_Y> ywwy; \
    S4<C4, ML_Y, ML_W, ML_W, ML_Z> ywwz; \
    S4<C4, ML_Y, ML_W, ML_W, ML_W> ywww; \
    S4<C4, ML_Z, ML_X, ML_X, ML_X> zxxx; \
    S4<C4, ML_Z, ML_X, ML_X, ML_Y> zxxy; \
    S4<C4, ML_Z, ML_X, ML_X, ML_Z> zxxz; \
    S4<C4, ML_Z, ML_X, ML_X, ML_W> zxxw; \
    S4<C4, ML_Z, ML_X, ML_Y, ML_X> zxyx; \
    S4<C4, ML_Z, ML_X, ML_Y, ML_Y> zxyy; \
    S4<C4, ML_Z, ML_X, ML_Y, ML_Z> zxyz; \
    S4<C4, ML_Z, ML_X, ML_Y, ML_W> zxyw; \
    S4<C4, ML_Z, ML_X, ML_Z, ML_X> zxzx; \
    S4<C4, ML_Z, ML_X, ML_Z, ML_Y> zxzy; \
    S4<C4, ML_Z, ML_X, ML_Z, ML_Z> zxzz; \
    S4<C4, ML_Z, ML_X, ML_Z, ML_W> zxzw; \
    S4<C4, ML_Z, ML_X, ML_W, ML_X> zxwx; \
    S4<C4, ML_Z, ML_X, ML_W, ML_Y> zxwy; \
    S4<C4, ML_Z, ML_X, ML_W, ML_Z> zxwz; \
    S4<C4, ML_Z, ML_X, ML_W, ML_W> zxww; \
    S4<C4, ML_Z, ML_Y, ML_X, ML_X> zyxx; \
    S4<C4, ML_Z, ML_Y, ML_X, ML_Y> zyxy; \
    S4<C4, ML_Z, ML_Y, ML_X, ML_Z> zyxz; \
    S4<C4, ML_Z, ML_Y, ML_X, ML_W> zyxw; \
    S4<C4, ML_Z, ML_Y, ML_Y, ML_X> zyyx; \
    S4<C4, ML_Z, ML_Y, ML_Y, ML_Y> zyyy; \
    S4<C4, ML_Z, ML_Y, ML_Y, ML_Z> zyyz; \
    S4<C4, ML_Z, ML_Y, ML_Y, ML_W> zyyw; \
    S4<C4, ML_Z, ML_Y, ML_Z, ML_X> zyzx; \
    S4<C4, ML_Z, ML_Y, ML_Z, ML_Y> zyzy; \
    S4<C4, ML_Z, ML_Y, ML_Z, ML_Z> zyzz; \
    S4<C4, ML_Z, ML_Y, ML_Z, ML_W> zyzw; \
    S4<C4, ML_Z, ML_Y, ML_W, ML_X> zywx; \
    S4<C4, ML_Z, ML_Y, ML_W, ML_Y> zywy; \
    S4<C4, ML_Z, ML_Y, ML_W, ML_Z> zywz; \
    S4<C4, ML_Z, ML_Y, ML_W, ML_W> zyww; \
    S4<C4, ML_Z, ML_Z, ML_X, ML_X> zzxx; \
    S4<C4, ML_Z, ML_Z, ML_X, ML_Y> zzxy; \
    S4<C4, ML_Z, ML_Z, ML_X, ML_Z> zzxz; \
    S4<C4, ML_Z, ML_Z, ML_X, ML_W> zzxw; \
    S4<C4, ML_Z, ML_Z, ML_Y, ML_X> zzyx; \
    S4<C4, ML_Z, ML_Z, ML_Y, ML_Y> zzyy; \
    S4<C4, ML_Z, ML_Z, ML_Y, ML_Z> zzyz; \
    S4<C4, ML_Z, ML_Z, ML_Y, ML_W> zzyw; \
    S4<C4, ML_Z, ML_Z, ML_Z, ML_X> zzzx; \
    S4<C4, ML_Z, ML_Z, ML_Z, ML_Y> zzzy; \
    S4<C4, ML_Z, ML_Z, ML_Z, ML_Z> zzzz; \
    S4<C4, ML_Z, ML_Z, ML_Z, ML_W> zzzw; \
    S4<C4, ML_Z, ML_Z, ML_W, ML_X> zzwx; \
    S4<C4, ML_Z, ML_Z, ML_W, ML_Y> zzwy; \
    S4<C4, ML_Z, ML_Z, ML_W, ML_Z> zzwz; \
    S4<C4, ML_Z, ML_Z, ML_W, ML_W> zzww; \
    S4<C4, ML_Z, ML_W, ML_X, ML_X> zwxx; \
    S4<C4, ML_Z, ML_W, ML_X, ML_Y> zwxy; \
    S4<C4, ML_Z, ML_W, ML_X, ML_Z> zwxz; \
    S4<C4, ML_Z, ML_W, ML_X, ML_W> zwxw; \
    S4<C4, ML_Z, ML_W, ML_Y, ML_X> zwyx; \
    S4<C4, ML_Z, ML_W, ML_Y, ML_Y> zwyy; \
    S4<C4, ML_Z, ML_W, ML_Y, ML_Z> zwyz; \
    S4<C4, ML_Z, ML_W, ML_Y, ML_W> zwyw; \
    S4<C4, ML_Z, ML_W, ML_Z, ML_X> zwzx; \
    S4<C4, ML_Z, ML_W, ML_Z, ML_Y> zwzy; \
    S4<C4, ML_Z, ML_W, ML_Z, ML_Z> zwzz; \
    S4<C4, ML_Z, ML_W, ML_Z, ML_W> zwzw; \
    S4<C4, ML_Z, ML_W, ML_W, ML_X> zwwx; \
    S4<C4, ML_Z, ML_W, ML_W, ML_Y> zwwy; \
    S4<C4, ML_Z, ML_W, ML_W, ML_Z> zwwz; \
    S4<C4, ML_Z, ML_W, ML_W, ML_W> zwww; \
    S4<C4, ML_W, ML_X, ML_X, ML_X> wxxx; \
    S4<C4, ML_W, ML_X, ML_X, ML_Y> wxxy; \
    S4<C4, ML_W, ML_X, ML_X, ML_Z> wxxz; \
    S4<C4, ML_W, ML_X, ML_X, ML_W> wxxw; \
    S4<C4, ML_W, ML_X, ML_Y, ML_X> wxyx; \
    S4<C4, ML_W, ML_X, ML_Y, ML_Y> wxyy; \
    S4<C4, ML_W, ML_X, ML_Y, ML_Z> wxyz; \
    S4<C4, ML_W, ML_X, ML_Y, ML_W> wxyw; \
    S4<C4, ML_W, ML_X, ML_Z, ML_X> wxzx; \
    S4<C4, ML_W, ML_X, ML_Z, ML_Y> wxzy; \
    S4<C4, ML_W, ML_X, ML_Z, ML_Z> wxzz; \
    S4<C4, ML_W, ML_X, ML_Z, ML_W> wxzw; \
    S4<C4, ML_W, ML_X, ML_W, ML_X> wxwx; \
    S4<C4, ML_W, ML_X, ML_W, ML_Y> wxwy; \
    S4<C4, ML_W, ML_X, ML_W, ML_Z> wxwz; \
    S4<C4, ML_W, ML_X, ML_W, ML_W> wxww; \
    S4<C4, ML_W, ML_Y, ML_X, ML_X> wyxx; \
    S4<C4, ML_W, ML_Y, ML_X, ML_Y> wyxy; \
    S4<C4, ML_W, ML_Y, ML_X, ML_Z> wyxz; \
    S4<C4, ML_W, ML_Y, ML_X, ML_W> wyxw; \
    S4<C4, ML_W, ML_Y, ML_Y, ML_X> wyyx; \
    S4<C4, ML_W, ML_Y, ML_Y, ML_Y> wyyy; \
    S4<C4, ML_W, ML_Y, ML_Y, ML_Z> wyyz; \
    S4<C4, ML_W, ML_Y, ML_Y, ML_W> wyyw; \
    S4<C4, ML_W, ML_Y, ML_Z, ML_X> wyzx; \
    S4<C4, ML_W, ML_Y, ML_Z, ML_Y> wyzy; \
    S4<C4, ML_W, ML_Y, ML_Z, ML_Z> wyzz; \
    S4<C4, ML_W, ML_Y, ML_Z, ML_W> wyzw; \
    S4<C4, ML_W, ML_Y, ML_W, ML_X> wywx; \
    S4<C4, ML_W, ML_Y, ML_W, ML_Y> wywy; \
    S4<C4, ML_W, ML_Y, ML_W, ML_Z> wywz; \
    S4<C4, ML_W, ML_Y, ML_W, ML_W> wyww; \
    S4<C4, ML_W, ML_Z, ML_X, ML_X> wzxx; \
    S4<C4, ML_W, ML_Z, ML_X, ML_Y> wzxy; \
    S4<C4, ML_W, ML_Z, ML_X, ML_Z> wzxz; \
    S4<C4, ML_W, ML_Z, ML_X, ML_W> wzxw; \
    S4<C4, ML_W, ML_Z, ML_Y, ML_X> wzyx; \
    S4<C4, ML_W, ML_Z, ML_Y, ML_Y> wzyy; \
    S4<C4, ML_W, ML_Z, ML_Y, ML_Z> wzyz; \
    S4<C4, ML_W, ML_Z, ML_Y, ML_W> wzyw; \
    S4<C4, ML_W, ML_Z, ML_Z, ML_X> wzzx; \
    S4<C4, ML_W, ML_Z, ML_Z, ML_Y> wzzy; \
    S4<C4, ML_W, ML_Z, ML_Z, ML_Z> wzzz; \
    S4<C4, ML_W, ML_Z, ML_Z, ML_W> wzzw; \
    S4<C4, ML_W, ML_Z, ML_W, ML_X> wzwx; \
    S4<C4, ML_W, ML_Z, ML_W, ML_Y> wzwy; \
    S4<C4, ML_W, ML_Z, ML_W, ML_Z> wzwz; \
    S4<C4, ML_W, ML_Z, ML_W, ML_W> wzww; \
    S4<C4, ML_W, ML_W, ML_X, ML_X> wwxx; \
    S4<C4, ML_W, ML_W, ML_X, ML_Y> wwxy; \
    S4<C4, ML_W, ML_W, ML_X, ML_Z> wwxz; \
    S4<C4, ML_W, ML_W, ML_X, ML_W> wwxw; \
    S4<C4, ML_W, ML_W, ML_Y, ML_X> wwyx; \
    S4<C4, ML_W, ML_W, ML_Y, ML_Y> wwyy; \
    S4<C4, ML_W, ML_W, ML_Y, ML_Z> wwyz; \
    S4<C4, ML_W, ML_W, ML_Y, ML_W> wwyw; \
    S4<C4, ML_W, ML_W, ML_Z, ML_X> wwzx; \
    S4<C4, ML_W, ML_W, ML_Z, ML_Y> wwzy; \
    S4<C4, ML_W, ML_W, ML_Z, ML_Z> wwzz; \
    S4<C4, ML_W, ML_W, ML_Z, ML_W> wwzw; \
    S4<C4, ML_W, ML_W, ML_W, ML_X> wwwx; \
    S4<C4, ML_W, ML_W, ML_W, ML_Y> wwwy; \
    S4<C4, ML_W, ML_W, ML_W, ML_Z> wwwz; \
    S4<C4, ML_W, ML_W, ML_W, ML_W> wwww
