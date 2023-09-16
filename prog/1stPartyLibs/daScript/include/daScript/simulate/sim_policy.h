#pragma once

namespace  das {

    class Context;

    // POLICY BASED OPERATIONS

    template <typename TT>
    struct SimPolicy_CoreType {
        static __forceinline void Set ( TT & a, TT b, Context &, LineInfo * ) { a = b;}
        static __forceinline bool Equ     ( TT a, TT b, Context &, LineInfo * ) { return a == b;  }
        static __forceinline bool NotEqu  ( TT a, TT b, Context &, LineInfo * ) { return a != b;  }
    };

    template <typename TT>
    struct SimPolicy_GroupByAdd : SimPolicy_CoreType<TT> {
        static __forceinline TT Unp ( TT x, Context &, LineInfo * ) { return x; }
        static __forceinline TT Unm ( TT x, Context &, LineInfo * ) { return -x; }
        static __forceinline TT Add ( TT a, TT b, Context &, LineInfo * ) { return a + b; }
        static __forceinline TT Sub ( TT a, TT b, Context &, LineInfo * ) { return a - b; }
        static __forceinline void SetAdd  ( TT & a, TT b, Context &, LineInfo * ) { a += b; }
        static __forceinline void SetSub  ( TT & a, TT b, Context &, LineInfo * ) { a -= b; }
    };

    template <typename TT>
    struct SimPolicy_Ordered {
        // ordered
        static __forceinline bool LessEqu ( TT a, TT b, Context &, LineInfo * ) { return a <= b;  }
        static __forceinline bool GtEqu   ( TT a, TT b, Context &, LineInfo * ) { return a >= b;  }
        static __forceinline bool Less    ( TT a, TT b, Context &, LineInfo * ) { return a < b;  }
        static __forceinline bool Gt      ( TT a, TT b, Context &, LineInfo * ) { return a > b;  }
    };

    template <typename TT>
    struct SimPolicy_Type : SimPolicy_GroupByAdd<TT>, SimPolicy_Ordered<TT> {
        // numeric
        static __forceinline TT Inc ( TT & x, Context &, LineInfo * ) { return ++x; }
        static __forceinline TT Dec ( TT & x, Context &, LineInfo * ) { return --x; }
        static __forceinline TT IncPost ( TT & x, Context &, LineInfo * ) { return x++; }
        static __forceinline TT DecPost ( TT & x, Context &, LineInfo * ) { return x--; }
        static __forceinline TT Div ( TT a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero");
            return a / b;
        }
        static __forceinline TT Mul ( TT a, TT b, Context &, LineInfo * ) { return a * b; }
        static __forceinline void SetDiv  ( TT & a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero");
            a /= b;
        }
        static __forceinline void SetMul  ( TT & a, TT b, Context &, LineInfo * ) { a *= b; }
    };

    struct SimPolicy_Bool : SimPolicy_CoreType<bool> {
        static __forceinline bool BoolNot ( bool x, Context &, LineInfo * ) { return !x; }
        static __forceinline bool BoolAnd ( bool a, bool b, Context &, LineInfo * ) { return a && b; }
        static __forceinline bool BoolOr  ( bool a, bool b, Context &, LineInfo * ) { return a || b; }
        static __forceinline bool BoolXor ( bool a, bool b, Context &, LineInfo * ) { return a ^ b; }
        static __forceinline void SetBoolAnd  ( bool & a, bool b, Context &, LineInfo * ) { a = a && b; }
        static __forceinline void SetBoolOr   ( bool & a, bool b, Context &, LineInfo * ) { a = a || b; }
        static __forceinline void SetBoolXor  ( bool & a, bool b, Context &, LineInfo * ) { a = a ^ b; }
    };

    template <typename TT, typename UTT>
    struct SimPolicy_Bin : SimPolicy_Type<TT> {
        enum { BITS = sizeof(TT)*8 };
        static __forceinline TT Mod ( TT a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero in modulo");
            return a % b;
        }
        static __forceinline TT BinNot ( TT x, Context &, LineInfo * ) { return ~x; }
        static __forceinline TT BinAnd ( TT a, TT b, Context &, LineInfo * ) { return a & b; }
        static __forceinline TT BinOr  ( TT a, TT b, Context &, LineInfo * ) { return a | b; }
        static __forceinline TT BinXor ( TT a, TT b, Context &, LineInfo * ) { return a ^ b; }
        static __forceinline TT BinShl ( TT a, TT b, Context &, LineInfo * ) { return a << b; }
        static __forceinline TT BinShr ( TT a, TT b, Context &, LineInfo * ) { return a >> b; }
        static __forceinline TT BinRotl ( TT _a, TT _b, Context &, LineInfo * ) {
            auto a = (UTT)_a; auto b = (UTT)_b;
            b &= (BITS-1);
            return (a << b) | (a >> (BITS - b));
        }
        static __forceinline TT BinRotr ( TT _a, TT _b, Context &, LineInfo * ) {
            auto a = (UTT)_a; auto b = (UTT)_b;
            b &= (BITS-1);
            return (a >> b) | (a << (BITS - b));
        }
        static __forceinline void SetBinAnd ( TT & a, TT b, Context &, LineInfo * ) { a &= b; }
        static __forceinline void SetBinOr  ( TT & a, TT b, Context &, LineInfo * ) { a |= b; }
        static __forceinline void SetBinXor ( TT & a, TT b, Context &, LineInfo * ) { a ^= b; }
        static __forceinline void SetBinShl ( TT & a, TT b, Context &, LineInfo * ) { a <<= b; }
        static __forceinline void SetBinShr ( TT & a, TT b, Context &, LineInfo * ) { a >>= b; }
        static __forceinline void SetBinRotl ( TT & _a, TT _b, Context &, LineInfo * ) {
            auto a = (UTT)_a; auto b = (UTT)_b;
            b &= (BITS-1);
            _a = TT((a << b) | (a >> (BITS - b)));
        }
        static __forceinline void SetBinRotr ( TT & _a, TT _b, Context &, LineInfo * ) {
            auto a = (UTT)_a; auto b = (UTT)_b;
            b &= (BITS-1);
            _a = TT((a >> b) | (a << (BITS - b)));
        }
        static __forceinline void SetMod    ( TT & a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero in modulo");
            a %= b;
        }
    };

    template <typename TT, typename UTT, TT INTMIN>
    struct SimPolicy_IntBin : SimPolicy_Bin<TT,UTT> {
        static __forceinline TT Div ( TT a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero");
            else if ( a==INTMIN && b==-1 ) context.throw_error_at(at, "division overflow");
            return a / b;
        }
        static __forceinline void SetDiv  ( TT & a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero");
            else if ( a==INTMIN && b==-1 ) context.throw_error_at(at, "division overflow");
            a /= b;
        }
        static __forceinline TT Mod ( TT a, TT b, Context & context, LineInfo * at ) {
            if ( b==0 ) context.throw_error_at(at, "division by zero in modulo");
            else if ( a==INTMIN && b==-1 ) return 0;
            return a % b;
        }
    };

    template <typename TT>
    struct SimPolicy_MathTT {
        static __forceinline TT Min   ( TT a, TT b, Context &, LineInfo * ) { return a < b ? a : b; }
        static __forceinline TT Max   ( TT a, TT b, Context &, LineInfo * ) { return a > b ? a : b; }
        static __forceinline TT Sat   ( TT a, Context &, LineInfo * )    { return a > 0 ? (a < 1 ? a : 1) : 0; }
        static __forceinline TT Clamp ( TT t, TT a, TT b, Context &, LineInfo * ) { return t>a ? (t<b ? t : b) : a; }
        static __forceinline TT Sign  ( TT a, Context &, LineInfo * )    { return a == 0 ? 0 : (a > 0 ? 1 : -1); }
        static __forceinline TT Abs   ( TT a, Context &, LineInfo * )    { return a >= 0 ? a : -a; }
        static __forceinline TT Mad   ( TT a, TT b, TT c, Context &, LineInfo * ) { return a*b + c; }
    };

    struct SimPolicy_Int : SimPolicy_IntBin<int32_t,uint32_t,INT32_MIN>, SimPolicy_MathTT<int32_t> {};
    struct SimPolicy_UInt : SimPolicy_Bin<uint32_t,uint32_t>, SimPolicy_MathTT<uint32_t> {};
    struct SimPolicy_Int64 : SimPolicy_IntBin<int64_t,uint64_t,INT64_MIN>, SimPolicy_MathTT<int64_t> {};
    struct SimPolicy_UInt64 : SimPolicy_Bin<uint64_t,uint64_t>, SimPolicy_MathTT<uint64_t> {};

    struct SimPolicy_Float : SimPolicy_Type<float> {
        static __forceinline float Div ( float a, float b, Context &, LineInfo * ) { return a / b; }
        static __forceinline void SetDiv  ( float & a, float b, Context &, LineInfo * ) { a /= b; }
        static __forceinline float Mod ( float a, float b, Context &, LineInfo * ) { return (float)fmod(a,b); }
        static __forceinline void SetMod ( float & a, float b, Context &, LineInfo * ) { a = (float)fmod(a,b); }
    };

    struct SimPolicy_Double : SimPolicy_Type<double>, SimPolicy_MathTT<double> {
        static __forceinline double Div ( double a, double b, Context &, LineInfo * ) { return a / b; }
        static __forceinline void SetDiv  ( double & a, double b, Context &, LineInfo * ) { a /= b; }
        static __forceinline double Mod ( double a, double b, Context &, LineInfo * ) { return fmod(a,b); }
        static __forceinline void SetMod ( double & a, double b, Context &, LineInfo * ) { a = fmod(a,b); }
        static __forceinline int Trunci ( double a, Context &, LineInfo * )          { return int(trunc(a)); }
        static __forceinline int Roundi ( double a, Context &, LineInfo * )          { return int(round(a)); }
        static __forceinline int Floori ( double a, Context &, LineInfo * )          { return int(floor(a)); }
        static __forceinline int Ceili  ( double a, Context &, LineInfo * )          { return int(ceil(a)); }
        static __forceinline double Lerp ( double a, double b, double t, Context &, LineInfo * ) { return (b-a)*t +a; }
    };

    struct SimPolicy_Pointer : SimPolicy_CoreType<void *> {
    };

    struct SimPolicy_MathVecI {
        static __forceinline vec4f Min   ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_cast_vec4f(v_mini(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Max   ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_cast_vec4f(v_maxi(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Sat   ( vec4f a, Context &, LineInfo * ) {
            return v_cast_vec4f(v_mini(v_maxi(v_cast_vec4i(a),v_cast_vec4i(v_zero())),v_splatsi(1)));
        }
        static __forceinline vec4f Clamp ( vec4f t, vec4f a, vec4f b, Context & ctx, LineInfo * at ) { return Max(a, Min(t, b, ctx, at), ctx, at); }
        static __forceinline vec4f Abs   ( vec4f a, Context &, LineInfo * ) { return v_cast_vec4f(v_absi(v_cast_vec4i(a))); }
        static __forceinline vec4f Sign  ( vec4f a, Context &, LineInfo * ) {
            vec4i positive = v_andi(v_splatsi(1), v_cmp_gti(v_cast_vec4i(a), v_zeroi()));
            vec4i negative = v_andi(v_splatsi(-1), v_cmp_lti(v_cast_vec4i(a), v_zeroi()));
            return v_cast_vec4f(v_ori(positive, negative));
        }
    };

    struct SimPolicy_MathVecU {
        static __forceinline vec4f Min   ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_cast_vec4f(v_minu(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Max   ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_cast_vec4f(v_maxu(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Sat   ( vec4f a, Context &, LineInfo * ) {
            return v_cast_vec4f(v_minu(v_cast_vec4i(a),v_splatsi(1)));
        }
        static __forceinline vec4f Clamp ( vec4f t, vec4f a, vec4f b, Context & ctx, LineInfo * at ) { return Max(a, Min(t, b, ctx, at), ctx, at); }
        static __forceinline vec4f Abs   ( vec4f a, Context &, LineInfo * ) { return a; }
        static __forceinline vec4f Sign  ( vec4f, Context &, LineInfo * ) { return v_zero(); }
    };

    struct SimPolicy_MathFloat {
        static __forceinline float Sign     ( float a, Context &, LineInfo * )          { return a == 0.0f ? 0.0f : (a > 0.0f) ? 1.0f : -1.0f; }
        static __forceinline float Abs      ( float a, Context &, LineInfo * )          { return v_extract_x(v_abs(v_set_x(a))); }
        static __forceinline float Floor    ( float a, Context &, LineInfo * )          { return v_extract_x(v_floor(v_set_x(a))); }
        static __forceinline float Ceil     ( float a, Context &, LineInfo * )          { return v_extract_x(v_ceil(v_set_x(a))); }
        static __forceinline float Fract    ( float a, Context &, LineInfo * )          { return a - v_extract_x(v_floor(v_set_x(a))); }
        static __forceinline float Sqrt     ( float a, Context &, LineInfo * )          { return v_extract_x(v_sqrt_x(v_set_x(a))); }
        static __forceinline float RSqrt    ( float a, Context &, LineInfo * )          { return v_extract_x(v_rsqrt_x(v_set_x(a))); }
        static __forceinline float RSqrtEst ( float a, Context &, LineInfo * )          { return v_extract_x(v_rsqrt_fast_x(v_set_x(a))); }
        static __forceinline float Min      ( float a, float b, Context &, LineInfo * ) { return a < b ? a : b; }
        static __forceinline float Max      ( float a, float b, Context &, LineInfo * ) { return a > b ? a : b; }
        static __forceinline float Sat      ( float a, Context &, LineInfo * )          { return a > 0 ? (a < 1 ? a : 1) : 0; }
        static __forceinline float Mad      ( float a, float b, float c, Context &, LineInfo * ) { return a*b + c; }
        static __forceinline float Lerp     ( float a, float b, float t, Context &, LineInfo * ) { return (b-a)*t +a; }
        static __forceinline float Clamp    ( float t, float a, float b, Context &, LineInfo * ) { return t>a ? (t<b ? t : b) : a; }

        static __forceinline int Trunci ( float a, Context &, LineInfo * )          { return v_extract_xi(v_cvt_vec4i(v_set_x(a))); }
        static __forceinline int Roundi ( float a, Context &, LineInfo * )          { return v_extract_xi(v_cvt_roundi(v_set_x(a))); }
        static __forceinline int Floori ( float a, Context &, LineInfo * )          { return v_extract_xi(v_cvt_floori(v_set_x(a))); }
        static __forceinline int Ceili  ( float a, Context &, LineInfo * )          { return v_extract_xi(v_cvt_ceili(v_set_x(a))); }

        static __forceinline float Exp   ( float a, Context &, LineInfo * )          { return v_extract_x(v_exp(v_set_x(a))); }
        static __forceinline float Log   ( float a, Context &, LineInfo * )          { return v_extract_x(v_log(v_set_x(a))); }
        static __forceinline float Exp2  ( float a, Context &, LineInfo * )          { return v_extract_x(v_exp2(v_set_x(a))); }
        static __forceinline float Log2  ( float a, Context &, LineInfo * )          { return v_extract_x(v_log2_est_p5(v_set_x(a))); }
        static __forceinline float Pow   ( float a, float b, Context &, LineInfo * ) { return v_extract_x(v_pow(v_set_x(a), v_set_x(b))); }
        static __forceinline float Rcp   ( float a, Context &, LineInfo * )          { return v_extract_x(v_rcp_x(v_set_x(a))); }
        static __forceinline float RcpEst( float a, Context &, LineInfo * )          { return v_extract_x(v_rcp_est_x(v_set_x(a))); }

        static __forceinline float Sin   ( float a, Context &, LineInfo * )          { return v_extract_x(v_sin_x(v_set_x(a))); }
        static __forceinline float Cos   ( float a, Context &, LineInfo * )          { return v_extract_x(v_cos_x(v_set_x(a))); }
        static __forceinline float Tan   ( float a, Context &, LineInfo * )          { return v_extract_x(v_tan_x(v_set_x(a))); }
        static __forceinline float ATan   ( float a, Context &, LineInfo * )         { return v_extract_x(v_atan_x(v_set_x(a))); }
        static __forceinline float ATan_est ( float a, Context &, LineInfo * )       { return v_extract_x(v_atan_est_x(v_set_x(a))); }
        static __forceinline float ASin   ( float a, Context &, LineInfo * )         { return v_extract_x(v_asin_x(v_set_x(a))); }
        static __forceinline float ACos   ( float a, Context &, LineInfo * )         { return v_extract_x(v_acos_x(v_set_x(a))); }
        static __forceinline float ATan2 ( float a, float b, Context &, LineInfo * ) { return v_extract_x(v_atan2_x(v_set_x(a), v_set_x(b))); }
        static __forceinline float ATan2_est ( float a, float b, Context &, LineInfo * ) { return v_extract_x(v_atan2_est_x(v_set_x(a), v_set_x(b))); }
    };

    struct SimPolicy_F2IVec {
        static __forceinline vec4f Trunci ( vec4f a, Context &, LineInfo * )          { return v_cast_vec4f(v_cvt_vec4i(a)); }
        static __forceinline vec4f Roundi ( vec4f a, Context &, LineInfo * )          { return v_cast_vec4f(v_cvt_roundi(a)); }
        static __forceinline vec4f Floori ( vec4f a, Context &, LineInfo * )          { return v_cast_vec4f(v_cvt_floori(a)); }
        static __forceinline vec4f Ceili  ( vec4f a, Context &, LineInfo * )          { return v_cast_vec4f(v_cvt_ceili(a)); }
    };

    struct SimPolicy_MathVec {

        static __forceinline vec4f Sign     ( vec4f a, Context &, LineInfo * ) {
            return v_or(v_and(v_splats(1.0f), v_cmp_gt(a, v_zero())), v_and(v_splats(-1.0f), v_cmp_lt(a, v_zero())));
        }

        static __forceinline vec4f Abs      ( vec4f a, Context &, LineInfo * )          { return v_abs(a); }
        static __forceinline vec4f Floor    ( vec4f a, Context &, LineInfo * )          { return v_floor(a); }
        static __forceinline vec4f Ceil     ( vec4f a, Context &, LineInfo * )          { return v_ceil(a); }
        static __forceinline vec4f Fract    ( vec4f a, Context &, LineInfo * )          { return v_sub(a, v_floor(a)); }
        static __forceinline vec4f Sqrt     ( vec4f a, Context &, LineInfo * )          { return v_sqrt4(a); }
        static __forceinline vec4f RSqrt    ( vec4f a, Context &, LineInfo * )          { return v_rsqrt4(a); }
        static __forceinline vec4f RSqrtEst ( vec4f a, Context &, LineInfo * )          { return v_rsqrt4_fast(a); }
        static __forceinline vec4f Min      ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_min(a,b); }
        static __forceinline vec4f Max      ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_max(a,b); }
        static __forceinline vec4f Sat      ( vec4f a, Context &, LineInfo * )          { return v_min(v_max(a,v_zero()),v_splats(1.0f)); }
        static __forceinline vec4f Clamp    ( vec4f a, vec4f r0, vec4f r1, Context &, LineInfo * ) { return v_max(v_min(a,r1), r0); }
        static __forceinline vec4f Mad      ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) { return v_madd(a,b,c); }
        static __forceinline vec4f MadS     ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) { return v_madd(a,v_perm_xxxx(b),c); }
        static __forceinline vec4f Lerp     ( vec4f a, vec4f b, vec4f t, Context &, LineInfo * ) { return v_madd(v_sub(b,a),t,a); }

        static __forceinline vec4f Exp   ( vec4f a, Context &, LineInfo * )          { return v_exp(a); }
        static __forceinline vec4f Log   ( vec4f a, Context &, LineInfo * )          { return v_log(a); }
        static __forceinline vec4f Exp2  ( vec4f a, Context &, LineInfo * )          { return v_exp2(a); }
        static __forceinline vec4f Log2  ( vec4f a, Context &, LineInfo * )          { return v_log2_est_p5(a); }
        static __forceinline vec4f Pow   ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_pow(a, b); }
        static __forceinline vec4f Rcp   ( vec4f a, Context &, LineInfo * )          { return v_rcp(a); }
        static __forceinline vec4f RcpEst( vec4f a, Context &, LineInfo * )          { return v_rcp_est(a); }

        static __forceinline vec4f Sin ( vec4f a, Context &, LineInfo * ) { return v_sin(a); }
        static __forceinline vec4f Cos ( vec4f a, Context &, LineInfo * ) { return v_cos(a); }
        static __forceinline vec4f Tan ( vec4f a, Context &, LineInfo * ) { return v_tan(a); }
        static __forceinline vec4f ATan( vec4f a, Context &, LineInfo * ) { return v_atan(a); }
        static __forceinline vec4f ATan_est( vec4f a, Context &, LineInfo * ) { return v_atan_est(a); }
        static __forceinline vec4f ASin( vec4f a, Context &, LineInfo * ) { return v_asin(a); }
        static __forceinline vec4f ACos( vec4f a, Context &, LineInfo * ) { return v_acos(a); }
        static __forceinline vec4f ATan2 ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_atan2(a, b); }
        static __forceinline vec4f ATan2_est ( vec4f a, vec4f b, Context &, LineInfo * ) { return v_atan2_est(a, b); }
    };

    template <typename TT, int mask>
    struct SimPolicy_Vec {
        static __forceinline void Set  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *) a;
            *pa = cast<TT>::to ( b );
        }
        // setXYZW
        static __forceinline vec4f setXYZW ( float x, float y, float z, float w ) {
            return v_make_vec4f(x, y, z, w);
        }
        static __forceinline vec4f setAligned ( const float *__restrict x ) { return v_ld(x); }
        static __forceinline vec4f setAligned ( const double *__restrict x ) { return setXYZW(float(x[0]),float(x[1]),float(x[2]),float(x[3])); }
        static __forceinline vec4f setAligned ( const int32_t  *__restrict x ) { return v_cvt_vec4f(v_ldi(x)); }
        static __forceinline vec4f setAligned ( const uint32_t *__restrict x ) { return v_cvtu_vec4f_ieee(v_ldi((const int32_t *)x)); }
        static __forceinline vec4f setXY ( const float *__restrict x ) { return v_ldu_half(x); }
        static __forceinline vec4f setXY ( const double *__restrict X ) { float x[2] = {float(X[0]),float(X[1])}; return v_ldu_half(x); }
        static __forceinline vec4f setXY ( const int32_t  *__restrict x ) { return v_cvt_vec4f(v_ldui_half(x)); }
        static __forceinline vec4f setXY ( const uint32_t *__restrict x ) { return v_cvtu_vec4f_ieee(v_ldui_half((const int32_t *)x)); }
        static __forceinline vec4f splats ( float x ) { return v_splats(x); }
        static __forceinline vec4f splats ( double x ) { return v_splats((float)x); }
        static __forceinline vec4f splats ( int32_t  x ) { return v_splats((float)x); }
        static __forceinline vec4f splats ( uint32_t x ) { return v_splats((float)x); }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cmp_eq(a,b)) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cmp_eq(a, b)) & mask) != mask;
        }
        // numeric
        static __forceinline vec4f Unp ( vec4f x, Context &, LineInfo * ) {
            return x;
        }
        static __forceinline vec4f Unm ( vec4f x, Context &, LineInfo * ) {
            return v_neg(x);
        }
        static __forceinline vec4f Add ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_add(a,b);
        }
        static __forceinline vec4f Sub ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_sub(a,b);
        }
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_div(a,b);
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_mod(a,b);
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_mul(a,b);
        }
        static __forceinline void SetAdd  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *) a;
            *pa = cast<TT>::to ( v_add(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetSub  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_sub(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_div(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mul(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mod(cast<TT>::from(*pa), b));
        }
        // vector-scalar
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_div(a,v_splat_x(b));
        }
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_mul(a,v_splat_x(b));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_div(v_splat_x(a),b);
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_mul(v_splat_x(a),b);
        }
        static __forceinline void SetDivScal  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_div(cast<TT>::from(*pa), v_splat_x(b)));
        }
        static __forceinline void SetMulScal  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mul(cast<TT>::from(*pa), v_splat_x(b)));
        }
    };

    template <typename TT, int mask>
    struct SimPolicy_iVec {
        static __forceinline void Set  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *) a;
            *pa = cast<TT>::to ( b );
        }
        // setXYZW
        static __forceinline vec4f setXYZW ( int32_t x, int32_t y, int32_t z, int32_t w ) {
            return v_cast_vec4f(v_make_vec4i(x, y, z, w));
        }
        static __forceinline vec4f setAligned ( const float *__restrict x ) { return v_cast_vec4f(v_cvt_vec4i(v_ld(x))); }
        static __forceinline vec4f setAligned ( const double *__restrict x ) { return setXYZW(int32_t(x[0]),int32_t(x[1]),int32_t(x[2]),int32_t(x[3])); }
        static __forceinline vec4f setAligned ( const int32_t *__restrict x ) { return v_cast_vec4f(v_ldi(x)); }
        static __forceinline vec4f setAligned ( const uint32_t *__restrict x ) { return setAligned((const int32_t*)x); }
        static __forceinline vec4f setXY ( const float *__restrict x ) { return v_cast_vec4f(v_cvt_vec4i(v_ldu_half(x))); }
        static __forceinline vec4f setXY ( const double *__restrict X ) { int32_t x[2] = {int32_t(X[0]), int32_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int32_t  *__restrict x ) { return v_cast_vec4f(v_ldui_half(x)); }
        static __forceinline vec4f setXY ( const uint32_t *__restrict x ) { return setXY((const int32_t*)x); }
        static __forceinline vec4f splats ( float x ) { return v_cast_vec4f(v_splatsi((int)x)); }
        static __forceinline vec4f splats ( double x ) { return v_cast_vec4f(v_splatsi((int)x)); }
        static __forceinline vec4f splats ( int32_t  x ) { return v_cast_vec4f(v_splatsi(x)); }
        static __forceinline vec4f splats ( uint32_t x ) { return v_cast_vec4f(v_splatsi(x)); }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a),v_cast_vec4i(b)))) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a), v_cast_vec4i(b)))) & mask) != mask;
        }
        // numeric
        static __forceinline vec4f Unp ( vec4f x, Context &, LineInfo * ) {
            return x;
        }
        static __forceinline vec4f Unm ( vec4f x, Context &, LineInfo * ) {
            return v_cast_vec4f(v_negi(v_cast_vec4i(x)));
        }
        static __forceinline vec4f Add ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_addi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Sub ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_subi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divi3(v_cast_vec4i(a),v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_divi2(v_cast_vec4i(a),v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_divi4(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_modi3(v_cast_vec4i(a),v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_modi2(v_cast_vec4i(a),v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_modi4(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mad      ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) {
            return v_cast_vec4f(v_addi(v_muli(v_cast_vec4i(a),v_cast_vec4i(b)),v_cast_vec4i(c)));
        }
        static __forceinline vec4f MadS     ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) {
            return v_cast_vec4f(v_addi(v_muli(v_cast_vec4i(a),v_cast_vec4i(v_perm_xxxx(b))),v_cast_vec4i(c)));
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_muli(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinAnd ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_andi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinOr ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_ori(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinXor ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_xori(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinShl ( vec4f a, vec4f b, Context &, LineInfo * ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_sll(v_cast_vec4i(a),shift));
        }
        static __forceinline vec4f BinShr ( vec4f a, vec4f b, Context &, LineInfo * ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_sra(v_cast_vec4i(a),shift));
        }
        static __forceinline void SetAdd  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_addi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetSub  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_subi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_muli(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_divi3(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_divi2(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_divi4(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_modi3(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_modi2(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_modi4(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinAnd  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_andi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinOr  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_ori(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinXor  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_xori(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinShl  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_sll(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        static __forceinline void SetBinShr  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_sra(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        // vector-scalar
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_muli(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_muli(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
        }
        static __forceinline void SetMulScal ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_muli(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_extract_xi(v_cast_vec4i(b))==0 )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divi3(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
            else    if ( mask==3 )  return v_cast_vec4f(v_divi2(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
            else                    return v_cast_vec4f(v_divi4(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divi3(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_divi2(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_divi4(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
        }
        static __forceinline void SetDivScal ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_extract_xi(v_cast_vec4i(b))==0 )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_divi3(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_divi2(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_divi4(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
        }
    };

    template <typename TT, int mask>
    struct SimPolicy_uVec : SimPolicy_iVec<TT,mask> {
        static __forceinline vec4f setXYZW ( uint32_t x, uint32_t y, uint32_t z, uint32_t w ) {
            return v_cast_vec4f(v_make_vec4i(x, y, z, w));
        }
        static __forceinline vec4f setAligned ( const float *__restrict x ) { return v_cast_vec4f(v_cvtu_vec4i_ieee(v_ld(x))); }
        static __forceinline vec4f setAligned ( const double *__restrict x ) { return setXYZW(uint32_t(x[0]),uint32_t(x[1]),uint32_t(x[2]),uint32_t(x[3])); }
        static __forceinline vec4f setAligned ( const int32_t *__restrict x ) { return v_cast_vec4f(v_ldi(x)); }
        static __forceinline vec4f setAligned ( const uint32_t *__restrict x ) { return setAligned((const int32_t*)x); }
        static __forceinline vec4f setXY ( const float *__restrict x ) { return v_cast_vec4f(v_cvtu_vec4i_ieee(v_ldu_half(x))); }
        static __forceinline vec4f setXY ( const double *__restrict X ) { uint32_t x[2] = {uint32_t(X[0]), uint32_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int32_t  *__restrict x ) { return v_cast_vec4f(v_ldui_half(x)); }
        static __forceinline vec4f setXY ( const uint32_t *__restrict x ) { return setXY((const int32_t*)x); }
        static __forceinline vec4f splats ( float x ) { return v_cast_vec4f(v_splatsi((uint32_t)x)); }
        static __forceinline vec4f splats ( double x ) { return v_cast_vec4f(v_splatsi((uint32_t)x)); }
        static __forceinline vec4f splats ( int32_t  x ) { return v_cast_vec4f(v_splatsi(x)); }
        static __forceinline vec4f splats ( uint32_t x ) { return v_cast_vec4f(v_splatsi(x)); }
        // swapping some numeric operations
        static __forceinline vec4f Mad      ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) {
            return v_cast_vec4f(v_addi(v_mulu(v_cast_vec4i(a),v_cast_vec4i(b)),v_cast_vec4i(c)));
        }
        static __forceinline vec4f MadS     ( vec4f a, vec4f b, vec4f c, Context &, LineInfo * ) {
            return v_cast_vec4f(v_addi(v_mulu(v_cast_vec4i(a),v_cast_vec4i(v_perm_xxxx(b))),v_cast_vec4i(c)));
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_mulu(v_cast_vec4i(a), v_cast_vec4i(b)));
        }
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divu3(v_cast_vec4i(a), v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_divu2(v_cast_vec4i(a), v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_divu4(v_cast_vec4i(a), v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_modu3(v_cast_vec4i(a),v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_modu2(v_cast_vec4i(a),v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_modu4(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinShr ( vec4f a, vec4f b, Context &, LineInfo * ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_srl(v_cast_vec4i(a),shift));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_divu3(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_divu2(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_divu4(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_modu3(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_modu2(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_modu4(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_mulu(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinShr  ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_srl(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        // vector-scalar
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_mulu(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return v_cast_vec4f(v_mulu(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
        }
        static __forceinline void SetMulScal ( char * a, vec4f b, Context &, LineInfo * ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_mulu(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_extract_xi(v_cast_vec4i(b))==0 )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divu3(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
            else    if ( mask==3 )  return v_cast_vec4f(v_divu2(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
            else                    return v_cast_vec4f(v_divu4(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(v_zero()),v_cast_vec4i(b)))) & mask )
                context.throw_error_at(at,"division by zero");
                    if ( mask==7 )  return v_cast_vec4f(v_divu3(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
            else    if ( mask==3 )  return v_cast_vec4f(v_divu2(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
            else                    return v_cast_vec4f(v_divu4(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
        }
        static __forceinline void SetDivScal ( char * a, vec4f b, Context & context, LineInfo * at ) {
            if ( v_extract_xi(v_cast_vec4i(b))==0 )
                context.throw_error_at(at,"division by zero");
            TT * pa = (TT *)a;
                    if ( mask==7 )  *pa = cast<TT>::to (v_cast_vec4f(v_divu3(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
            else    if ( mask==3 )  *pa = cast<TT>::to (v_cast_vec4f(v_divu2(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
            else                    *pa = cast<TT>::to (v_cast_vec4f(v_divu4(v_cast_vec4i(cast<TT>::from(*pa)),v_splat_xi(v_cast_vec4i(b)))));
        }
    };

    struct SimPolicy_Range64 {
        enum { mask=1+2+4+8 };
        static __forceinline void Set  ( char * a, vec4f b, Context &, LineInfo * ) {
            range64 * pa = (range64 *) a;
            *pa = cast<range64>::to ( b );
        }
        // setXYZW
        static __forceinline vec4f setXY ( const float *__restrict X )    { int64_t x[2] = {int64_t(X[0]), int64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const double *__restrict X )   { int64_t x[2] = {int64_t(X[0]), int64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int32_t  *__restrict X ) { int64_t x[2] = {int64_t(X[0]), int64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const uint32_t *__restrict X ) { int64_t x[2] = {int64_t(X[0]), int64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int64_t  *__restrict x ) { return v_ldu((const float *)x); }
        static __forceinline vec4f setXY ( const uint64_t *__restrict X ) { return setXY((int64_t *)X); }
        static __forceinline vec4f splats ( float X )    { return splats((int64_t)X); }
        static __forceinline vec4f splats ( double X )   { return splats((int64_t)X); }
        static __forceinline vec4f splats ( int32_t  X ) { return splats((int64_t)X); }
        static __forceinline vec4f splats ( uint32_t X ) { return splats((int64_t)X); }
        static __forceinline vec4f splats ( int64_t X )  { int64_t x[2] = {X, X}; return v_ldu((const float *)x); }
        static __forceinline vec4f splats ( uint64_t X ) { return splats((int64_t)X); }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a),v_cast_vec4i(b)))) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a), v_cast_vec4i(b)))) & mask) != mask;
        }
    };

    struct SimPolicy_URange64 {
        enum { mask=1+2+4+8 };
        static __forceinline void Set  ( char * a, vec4f b, Context &, LineInfo * ) {
            urange64 * pa = (urange64 *) a;
            *pa = cast<urange64>::to ( b );
        }
        // setXYZW
        static __forceinline vec4f setXY ( const float *__restrict X )    { uint64_t x[2] = {uint64_t(X[0]), uint64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const double *__restrict X )   { uint64_t x[2] = {uint64_t(X[0]), uint64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int32_t  *__restrict X ) { uint64_t x[2] = {uint64_t(X[0]), uint64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const uint32_t *__restrict X ) { uint64_t x[2] = {uint64_t(X[0]), uint64_t(X[1])}; return setXY(x); }
        static __forceinline vec4f setXY ( const int64_t  *__restrict X ) { return setXY((uint64_t *)X); }
        static __forceinline vec4f setXY ( const uint64_t *__restrict x ) { return v_ldu((const float *)x); }
        static __forceinline vec4f splats ( float X )    { return splats((uint64_t)X); }
        static __forceinline vec4f splats ( double X )   { return splats((uint64_t)X); }
        static __forceinline vec4f splats ( int32_t  X ) { return splats((uint64_t)X); }
        static __forceinline vec4f splats ( uint32_t X ) { return splats((uint64_t)X); }
        static __forceinline vec4f splats ( int64_t X ) { return splats((uint64_t)X); }
        static __forceinline vec4f splats ( uint64_t X )  { uint64_t x[2] = {X, X}; return v_ldu((const float *)x); }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a),v_cast_vec4i(b)))) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a), v_cast_vec4i(b)))) & mask) != mask;
        }
    };

    struct SimPolicy_Range : SimPolicy_iVec<range,3> {};
    struct SimPolicy_URange : SimPolicy_uVec<urange,3> {};

    extern const char * rts_null;

    __forceinline const char * to_rts ( vec4f a ) {
        const char * s = cast<const char *>::to(a);
        return s ? s : rts_null;
    }

    __forceinline const char * to_rts ( const char * s ) {
        return s ? s : rts_null;
    }

    struct SimPolicy_String {
        // even more basic
        static __forceinline void Set     ( char * & a, char * b, Context &, LineInfo * ) { a = b;}
        static __forceinline bool Equ     ( char * a, char * b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))==0; }
        static __forceinline bool NotEqu  ( char * a, char * b, Context &, LineInfo * ) { return (bool) strcmp(to_rts(a), to_rts(b)); }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))==0; }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) { return (bool) strcmp(to_rts(a), to_rts(b)); }
        // ordered
        static __forceinline bool LessEqu ( vec4f a, vec4f b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))<=0; }
        static __forceinline bool GtEqu   ( vec4f a, vec4f b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))>=0; }
        static __forceinline bool Less    ( vec4f a, vec4f b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))<0; }
        static __forceinline bool Gt      ( vec4f a, vec4f b, Context &, LineInfo * ) { return strcmp(to_rts(a), to_rts(b))>0; }
        static vec4f Add  ( vec4f a, vec4f b, Context &, LineInfo * );
        static void SetAdd ( char * a, vec4f b, Context &, LineInfo *  );
    };

    // generic SimPolicy<X>

    template <typename TT>
    struct SimPolicy;

    template <> struct SimPolicy<bool> : SimPolicy_Bool {};
    template <> struct SimPolicy<int32_t> : SimPolicy_Int {};
    template <> struct SimPolicy<uint32_t> : SimPolicy_UInt {};
    template <> struct SimPolicy<int64_t> : SimPolicy_Int64 {};
    template <> struct SimPolicy<uint64_t> : SimPolicy_UInt64 {};
    template <> struct SimPolicy<float> : SimPolicy_Float, SimPolicy_MathFloat {};
    template <> struct SimPolicy<double> : SimPolicy_Double {};
    template <> struct SimPolicy<void *> : SimPolicy_Pointer {};
    template <> struct SimPolicy<float2> : SimPolicy_Vec<float2,1+2>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<float3> : SimPolicy_Vec<float3,1+2+4>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<float4> : SimPolicy_Vec<float4,1+2+4+8>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int2> : SimPolicy_iVec<int2,1+2>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int3> : SimPolicy_iVec<int3,1+2+4>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int4> : SimPolicy_iVec<int4,1+2+4+8>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint2> : SimPolicy_uVec<uint2,1+2>, SimPolicy_MathVecU, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint3> : SimPolicy_uVec<uint3,1+2+4>, SimPolicy_MathVecU, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint4> : SimPolicy_uVec<uint4,1+2+4+8>, SimPolicy_MathVecU, SimPolicy_F2IVec {};
    template <> struct SimPolicy<range> : SimPolicy_Range {};
    template <> struct SimPolicy<urange> : SimPolicy_URange {};
    template <> struct SimPolicy<range64> : SimPolicy_Range64 {};
    template <> struct SimPolicy<urange64> : SimPolicy_URange64 {};
    template <> struct SimPolicy<char *> : SimPolicy_String {};

}
