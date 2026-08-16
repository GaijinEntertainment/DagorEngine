#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/simulate/sim_policy.h"

namespace das
{
    // VECTOR C-TOR
    template <typename TT, typename Policy, int vecS>
    struct SimNode_VecCtor;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4244)
#endif

    template <typename TT, typename Policy>
    struct SimNode_VecCtor<TT,Policy,1> : SimNode_CallBase {
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_1);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValue;
            evalArgs(context, &argValue);
            return Policy::splats(cast<TT>::to(argValue));
        }
    };

    template <typename TT, typename Policy>
    struct SimNode_VecCtor<TT,Policy,2> : SimNode_CallBase {
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_2);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValues[2];
            evalArgs(context, argValues);
            alignas(16) TT ret[2];
            ret[0] = cast<TT>::to(argValues[0]);
            ret[1] = cast<TT>::to(argValues[1]);
            return Policy::setXY(ret);
        }
    };

    template <typename TT, typename Policy>
    struct SimNode_VecCtor<TT,Policy,3> : SimNode_CallBase {
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_3);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_SUB(arguments[2]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValues[3];
            evalArgs(context, argValues);
            alignas(16) TT ret[4];
            ret[0] = cast<TT>::to(argValues[0]);
            ret[1] = cast<TT>::to(argValues[1]);
            ret[2] = cast<TT>::to(argValues[2]);
            ret[3] = 0;
            return Policy::setAligned(ret);
        }
    };

    template <typename TT, typename Policy>
    struct SimNode_VecCtor<TT,Policy,4> : SimNode_CallBase {
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_4);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_SUB(arguments[2]);
            V_SUB(arguments[3]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValues[4];
            evalArgs(context, argValues);
            alignas(16) TT ret[4];
            ret[0] = cast<TT>::to(argValues[0]);
            ret[1] = cast<TT>::to(argValues[1]);
            ret[2] = cast<TT>::to(argValues[2]);
            ret[3] = cast<TT>::to(argValues[3]);
            return Policy::setAligned(ret);
        }
    };

    template <typename TT, typename Policy>
    struct SimNode_Range1Ctor: SimNode_CallBase {
        SimNode_Range1Ctor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Range1Ctor);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValue;
            evalArgs(context, &argValue);
            alignas(16) TT ret[2];
            ret[0] = 0;
            ret[1] = cast<TT>::to(argValue);
            return Policy::setXY(ret);
        }
    };

    struct SimNode_VecPassThrough: SimNode_CallBase {
        SimNode_VecPassThrough(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecPassThrough);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValue;
            evalArgs(context, &argValue);
            return argValue;
        }
    };

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#define ADD_VEC_CTOR_1(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_Zero,const VTYPE>(#VTYPE,lib,"v_zero",false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,1>,const VTYPE,float>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,1>,const VTYPE,double>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,1>,const VTYPE,int32_t>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,1>,const VTYPE,uint32_t>(#VTYPE,lib,VNAME,false)) );

#define ADD_RANGE_CTOR_1(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_Zero,const VTYPE>(#VTYPE,lib,"v_zero",false)) ); \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<float,   SimPolicy<VTYPE>>,const VTYPE,float>(#VTYPE,lib,"mk_" VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<double,  SimPolicy<VTYPE>>,const VTYPE,double>(#VTYPE,lib,"mk_" VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<int32_t, SimPolicy<VTYPE>>,const VTYPE,int32_t>(#VTYPE,lib,"mk_" VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<uint32_t,SimPolicy<VTYPE>>,const VTYPE,uint32_t>(#VTYPE,lib,"mk_" VNAME,false)) );

#define ADD_RANGE64_CTOR_1(VTYPE,VNAME) \
ADD_RANGE_CTOR_1(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<int64_t, SimPolicy<VTYPE>>,const VTYPE,int64_t>(#VTYPE,lib,"mk_" VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_Range1Ctor<uint64_t,SimPolicy<VTYPE>>,const VTYPE,uint64_t>(#VTYPE,lib,"mk_" VNAME,false)) );

#define ADD_VEC_CTOR_2(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,2>,const VTYPE,float,float>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,2>,const VTYPE,double,double>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,2>,const VTYPE,int32_t,int32_t>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,2>,const VTYPE,uint32_t,uint32_t>(#VTYPE,lib,VNAME,false)) );

#define ADD_VEC64_CTOR_2(VTYPE,VNAME) \
ADD_VEC_CTOR_2(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_VecCtor<int64_t, SimPolicy<VTYPE>,2>,const VTYPE,int64_t,int64_t>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<uint64_t,SimPolicy<VTYPE>,2>,const VTYPE,uint64_t,uint64_t>(#VTYPE,lib,VNAME,false)) );

#define ADD_VEC_CTOR_3(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,3>,const VTYPE,float,float,float>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,3>,const VTYPE,double,double,double>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,3>,const VTYPE,int32_t,int32_t,int32_t>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,3>,const VTYPE,uint32_t,uint32_t,uint32_t>(#VTYPE,lib,VNAME,false)) );

#define ADD_VEC_CTOR_4(VTYPE,VNAME) \
addFunction ( (new BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,4>,const VTYPE,float,float,float,float>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,4>,const VTYPE,double,double,double,double>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,4>,const VTYPE,int32_t,int32_t,int32_t,int32_t>(#VTYPE,lib,VNAME,false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,4>,const VTYPE,uint32_t,uint32_t,uint32_t,uint32_t>(#VTYPE,lib,VNAME,false)) );

    struct SimNode_Int4ToFloat4 : SimNode_CallBase {
        SimNode_Int4ToFloat4(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Int4ToFloat4);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f i4 = arguments[0]->eval(context);
            return v_cvt_vec4f(v_cast_vec4i(i4));
        }
    };

    struct SimNode_UInt4ToFloat4 : SimNode_CallBase {
        SimNode_UInt4ToFloat4(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(UInt4ToFloat4);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f i4 = arguments[0]->eval(context);
            return v_cvtu_vec4f_ieee(v_cast_vec4i(i4));
        }
    };

    struct SimNode_Float4ToInt4 : SimNode_CallBase {
        SimNode_Float4ToInt4(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Float4ToInt4);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f f4 = arguments[0]->eval(context);
            return v_cast_vec4f(v_cvt_vec4i(f4));
        }
    };

    struct SimNode_Float4ToUInt4 : SimNode_CallBase {
        SimNode_Float4ToUInt4(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Float4ToUInt4);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f f4 = arguments[0]->eval(context);
            return v_cast_vec4f(v_cvtu_vec4i_ieee(f4));
        }
    };

    struct SimNode_AnyIntToAnyInt : SimNode_CallBase {
        SimNode_AnyIntToAnyInt(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(AnyIntToAnyInt);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            return arguments[0]->eval(context);
        }
    };

    // ===== 16/8-bit lattice ctors =====
    // Byte-packed lanes converted per element from the scalar argument type. No SimPolicy
    // dependency — the storage-only int families deliberately have none.

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4244)
#endif

    template <typename ET> struct SVecConv {
        template <typename AT> static __forceinline ET conv ( AT v ) { return ET(v); }
    };
    template <> struct SVecConv<float16_t> {
        template <typename AT> static __forceinline float16_t conv ( AT v ) { return float16_t(float(v)); }
    };

    template <typename VT, typename ET, typename AT>
    struct SimNode_SVecSplat : SimNode_CallBase {
        SimNode_SVecSplat(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(SVecSplat);
            V_SUB(arguments[0]);
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValue;
            evalArgs(context, &argValue);
            VT ret ( SVecConv<ET>::conv(cast<AT>::to(argValue)) );
            return (vec4f) ret;
        }
    };

    template <typename VT, typename ET, typename AT, int vecS>
    struct SimNode_SVecCtor : SimNode_CallBase {
        SimNode_SVecCtor(const LineInfo & at) : SimNode_CallBase(at,"") {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(SVecCtor);
            for ( int i=0; i!=vecS; ++i ) {
                arguments[i] = vis.sub(arguments[i],"argument");
            }
            V_END();
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValues[vecS];
            evalArgs(context, argValues);
            VT ret;
            memset(&ret, 0, sizeof(VT));
            ET * el = (ET *) &ret;
            for ( int i=0; i!=vecS; ++i ) {
                el[i] = SVecConv<ET>::conv(cast<AT>::to(argValues[i]));
            }
            return (vec4f) ret;
        }
    };

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#define ADD_SVEC_SPLAT(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecSplat<VTYPE,ETYPE,ATYPE>,const VTYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

#define ADD_SVEC_CTOR_2(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecCtor<VTYPE,ETYPE,ATYPE,2>,const VTYPE,ATYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

#define ADD_SVEC_CTOR_3(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecCtor<VTYPE,ETYPE,ATYPE,3>,const VTYPE,ATYPE,ATYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

#define ADD_SVEC_CTOR_4(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecCtor<VTYPE,ETYPE,ATYPE,4>,const VTYPE,ATYPE,ATYPE,ATYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

#define ADD_SVEC_CTOR_8(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecCtor<VTYPE,ETYPE,ATYPE,8>,const VTYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

#define ADD_SVEC_CTOR_16(VTYPE,ETYPE,ATYPE) \
addFunction ( (new BuiltInFn<SimNode_SVecCtor<VTYPE,ETYPE,ATYPE,16>,const VTYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE,ATYPE>(#VTYPE,lib,#VTYPE,false)) );

// zero + pass-through + {float,double,int,uint} x {splat, N-arg} — mirrors the arg-type
// coverage the 32-bit vector families get from ADD_VEC_CTOR_*
#define ADD_SVEC_FAMILY(VTYPE,ETYPE,N) \
addFunction ( (new BuiltInFn<SimNode_Zero,const VTYPE>(#VTYPE,lib,"das_svec_zero<" #VTYPE ">",false)) ); \
addFunction ( (new BuiltInFn<SimNode_VecPassThrough, VTYPE, VTYPE>(#VTYPE,lib,#VTYPE,false)) ); \
ADD_SVEC_SPLAT(VTYPE,ETYPE,float)    ADD_SVEC_CTOR_##N(VTYPE,ETYPE,float) \
ADD_SVEC_SPLAT(VTYPE,ETYPE,double)   ADD_SVEC_CTOR_##N(VTYPE,ETYPE,double) \
ADD_SVEC_SPLAT(VTYPE,ETYPE,int32_t)  ADD_SVEC_CTOR_##N(VTYPE,ETYPE,int32_t) \
ADD_SVEC_SPLAT(VTYPE,ETYPE,uint32_t) ADD_SVEC_CTOR_##N(VTYPE,ETYPE,uint32_t)

    void Module_BuiltIn::addVectorCtor(ModuleLibrary & lib) {
        // float2
        ADD_VEC_CTOR_1(float2,"v_splats");
        ADD_VEC_CTOR_2(float2,"float2");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, float2, float2>("float2",lib,"float2",false)) );
        addFunction( (new BuiltInFn<SimNode_Int4ToFloat4, float2, int2>("float2",lib,"cvt_ifloat2",false)) );
        addFunction( (new BuiltInFn<SimNode_UInt4ToFloat4,float2,uint2>("float2",lib,"cvt_ufloat2",false)) );
        // float3
        ADD_VEC_CTOR_1(float3,"v_splats");
        ADD_VEC_CTOR_3(float3,"float3");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, float3, float3>("float3",lib,"float3",false)) );
        addFunction( (new BuiltInFn<SimNode_Int4ToFloat4, float3, int3>("float3",lib,"cvt_ifloat3",false)) );
        addFunction( (new BuiltInFn<SimNode_UInt4ToFloat4,float3,uint3>("float3",lib,"cvt_ufloat3",false)) );
        addExtern<DAS_BIND_FUN(float3_from_xy_z)> (*this, lib, "float3", SideEffects::none, "float3_from_xy_z");
        addExtern<DAS_BIND_FUN(float3_from_x_yz)> (*this, lib, "float3", SideEffects::none, "float3_from_x_yz");
        // float4
        ADD_VEC_CTOR_1(float4,"v_splats");
        ADD_VEC_CTOR_4(float4,"float4");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, float4, float4>("float4",lib,"float4",false)) );
        addFunction( (new BuiltInFn<SimNode_Int4ToFloat4, float4, int4>("float4",lib,"cvt_ifloat4",false)) );
        addFunction( (new BuiltInFn<SimNode_UInt4ToFloat4,float4,uint4>("float4",lib,"cvt_ufloat4",false)) );
        addExtern<DAS_BIND_FUN(float4_from_xyz_w)>  (*this, lib, "float4", SideEffects::none, "float4_from_xyz_w");
        addExtern<DAS_BIND_FUN(float4_from_x_yzw)>  (*this, lib, "float4", SideEffects::none, "float4_from_x_yzw");
        addExtern<DAS_BIND_FUN(float4_from_xy_zw)>  (*this, lib, "float4", SideEffects::none, "float4_from_xy_zw");
        addExtern<DAS_BIND_FUN(float4_from_xy_z_w)> (*this, lib, "float4", SideEffects::none, "float4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(float4_from_x_yz_w)> (*this, lib, "float4", SideEffects::none, "float4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(float4_from_x_y_zw)> (*this, lib, "float4", SideEffects::none, "float4_from_x_y_zw");
        // int2
        ADD_VEC_CTOR_1(int2,"int2");
        ADD_VEC_CTOR_2(int2,"int2");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, int2, int2>("int2",lib,"int2",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToInt4, int2, float2>("int2",lib,"cvt_int2",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,int2, uint2>("int2",lib,"cvt_pass",false)) );
        // int3
        ADD_VEC_CTOR_1(int3,"int3");
        ADD_VEC_CTOR_3(int3,"int3");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, int3, int3>("int3",lib,"int3",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToInt4, int3, float3>("int3",lib,"cvt_int3",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,int3, uint3>("int3",lib,"cvt_pass",false)) );
        addExtern<DAS_BIND_FUN(int3_from_xy_z)> (*this, lib, "int3", SideEffects::none, "int3_from_xy_z");
        addExtern<DAS_BIND_FUN(int3_from_x_yz)> (*this, lib, "int3", SideEffects::none, "int3_from_x_yz");
        // int4
        ADD_VEC_CTOR_1(int4,"int4");
        ADD_VEC_CTOR_4(int4,"int4");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, int4, int4>("int4",lib,"int4",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToInt4, int4, float4>("int4",lib,"cvt_int4",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,int4, uint4>("int4",lib,"cvt_pass",false)) );
        addExtern<DAS_BIND_FUN(int4_from_xyz_w)>  (*this, lib, "int4", SideEffects::none, "int4_from_xyz_w");
        addExtern<DAS_BIND_FUN(int4_from_x_yzw)>  (*this, lib, "int4", SideEffects::none, "int4_from_x_yzw");
        addExtern<DAS_BIND_FUN(int4_from_xy_zw)>  (*this, lib, "int4", SideEffects::none, "int4_from_xy_zw");
        addExtern<DAS_BIND_FUN(int4_from_xy_z_w)> (*this, lib, "int4", SideEffects::none, "int4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(int4_from_x_yz_w)> (*this, lib, "int4", SideEffects::none, "int4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(int4_from_x_y_zw)> (*this, lib, "int4", SideEffects::none, "int4_from_x_y_zw");
        // uint2
        ADD_VEC_CTOR_1(uint2,"uint2");
        ADD_VEC_CTOR_2(uint2,"uint2");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, uint2, uint2>("uint2",lib,"uint2",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToUInt4,uint2,float2>("uint2",lib,"cvt_uint2",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,uint2, int2>("uint2",lib,"cvt_pass",false)) );
        // uint3
        ADD_VEC_CTOR_1(uint3,"uint3");
        ADD_VEC_CTOR_3(uint3,"uint3");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, uint3, uint3>("uint3",lib,"uint3",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToUInt4,uint3,float3>("uint3",lib,"cvt_uint3",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,uint3, int3>("uint3",lib,"cvt_pass",false)) );
        addExtern<DAS_BIND_FUN(uint3_from_xy_z)> (*this, lib, "uint3", SideEffects::none, "uint3_from_xy_z");
        addExtern<DAS_BIND_FUN(uint3_from_x_yz)> (*this, lib, "uint3", SideEffects::none, "uint3_from_x_yz");
        // uint4
        ADD_VEC_CTOR_1(uint4,"uint4");
        ADD_VEC_CTOR_4(uint4,"uint4");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, uint4, uint4>("uint4",lib,"uint4",false)) );
        addFunction( (new BuiltInFn<SimNode_Float4ToUInt4,uint4,float4>("uint4",lib,"cvt_uint4",false)) );
        addFunction( (new BuiltInFn<SimNode_AnyIntToAnyInt,uint4, int4>("uint4",lib,"cvt_pass",false)) );
        addExtern<DAS_BIND_FUN(uint4_from_xyz_w)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_xyz_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_yzw)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_x_yzw");
        addExtern<DAS_BIND_FUN(uint4_from_xy_zw)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_xy_zw");
        addExtern<DAS_BIND_FUN(uint4_from_xy_z_w)> (*this, lib, "uint4", SideEffects::none, "uint4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_yz_w)> (*this, lib, "uint4", SideEffects::none, "uint4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_y_zw)> (*this, lib, "uint4", SideEffects::none, "uint4_from_x_y_zw");
        // range
        ADD_RANGE_CTOR_1(range,"range");
        ADD_VEC_CTOR_2(range,"range");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, range, range>("range",lib,"range",false)) );
        addFunction ( (new BuiltInFn<SimNode_VecCtor<int32_t,SimPolicy<range>,2>,range,int32_t,int32_t>("interval",lib,"range",false)) );
        // urange
        ADD_RANGE_CTOR_1(urange,"urange");
        ADD_VEC_CTOR_2(urange,"urange");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, urange, urange>("urange",lib,"urange",false)) );
        addFunction ( (new BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<urange>,2>,urange,uint32_t,uint32_t>("interval",lib,"urange",false)) );
        // range64
        ADD_RANGE64_CTOR_1(range64,"range64");
        ADD_VEC64_CTOR_2(range64,"range64");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, range64, range64>("range64",lib,"range64",false)) );
        addFunction ( (new BuiltInFn<SimNode_VecCtor<int64_t,SimPolicy<range64>,2>,range64,int64_t,int64_t>("interval",lib,"range64",false)) );
        // urange64
        ADD_RANGE64_CTOR_1(urange64,"urange64");
        ADD_VEC64_CTOR_2(urange64,"urange64");
        addFunction( (new BuiltInFn<SimNode_VecPassThrough, urange64, urange64>("urange64",lib,"urange64",false)) );
        addFunction ( (new BuiltInFn<SimNode_VecCtor<uint64_t,SimPolicy<urange64>,2>,urange64,uint64_t,uint64_t>("interval",lib,"urange64",false)) );
        // ===== 16/8-bit lattice =====
        ADD_SVEC_FAMILY(half2,  float16_t, 2);
        ADD_SVEC_FAMILY(half3,  float16_t, 3);
        ADD_SVEC_FAMILY(half4,  float16_t, 4);
        ADD_SVEC_FAMILY(half8,  float16_t, 8);
        // fp16-scalar-argument ctors: half2(1.h, 2.h) and splats from a float16 value
        ADD_SVEC_SPLAT(half2, float16_t, float16_t) ADD_SVEC_CTOR_2(half2, float16_t, float16_t)
        ADD_SVEC_SPLAT(half3, float16_t, float16_t) ADD_SVEC_CTOR_3(half3, float16_t, float16_t)
        ADD_SVEC_SPLAT(half4, float16_t, float16_t) ADD_SVEC_CTOR_4(half4, float16_t, float16_t)
        ADD_SVEC_SPLAT(half8, float16_t, float16_t) ADD_SVEC_CTOR_8(half8, float16_t, float16_t)
        ADD_SVEC_FAMILY(short2, int16_t,   2);
        ADD_SVEC_FAMILY(short3, int16_t,   3);
        ADD_SVEC_FAMILY(short4, int16_t,   4);
        ADD_SVEC_FAMILY(short8, int16_t,   8);
        ADD_SVEC_FAMILY(ushort2, uint16_t, 2);
        ADD_SVEC_FAMILY(ushort3, uint16_t, 3);
        ADD_SVEC_FAMILY(ushort4, uint16_t, 4);
        ADD_SVEC_FAMILY(ushort8, uint16_t, 8);
        ADD_SVEC_FAMILY(byte2,  int8_t,    2);
        ADD_SVEC_FAMILY(byte3,  int8_t,    3);
        ADD_SVEC_FAMILY(byte4,  int8_t,    4);
        ADD_SVEC_FAMILY(byte8,  int8_t,    8);
        ADD_SVEC_FAMILY(byte16, int8_t,    16);
        ADD_SVEC_FAMILY(ubyte2,  uint8_t,  2);
        ADD_SVEC_FAMILY(ubyte3,  uint8_t,  3);
        ADD_SVEC_FAMILY(ubyte4,  uint8_t,  4);
        ADD_SVEC_FAMILY(ubyte8,  uint8_t,  8);
        ADD_SVEC_FAMILY(ubyte16, uint8_t,  16);
        // lane-wise converts: widen = ctor-name overload on the wider type; narrow =
        // ctor-name overload (C-truncation, mirrors the int8(x) scalar cast) plus a
        // `_sat` variant that clamps to the target range (the Q8-quant shape; the native
        // sqxtn/PACK lowering is a later JIT wave)
#define ADD_SVEC_CVT(TO,TOE,FROM,FE) \
    addExtern<DAS_BIND_FUN((das_sv_cvt<TO,TOE,FROM,FE>))>(*this, lib, #TO, SideEffects::none, "das_sv_cvt<" #TO "," #TOE "," #FROM "," #FE ">");
#define ADD_SVEC_CVT_SAT(TO,TOE,FROM,FE) \
    addExtern<DAS_BIND_FUN((das_sv_cvt_sat<TO,TOE,FROM,FE>))>(*this, lib, #TO "_sat", SideEffects::none, "das_sv_cvt_sat<" #TO "," #TOE "," #FROM "," #FE ">");
        // widen to 32-bit families
        ADD_SVEC_CVT(int2, int32_t, short2, int16_t);
        ADD_SVEC_CVT(int3, int32_t, short3, int16_t);
        ADD_SVEC_CVT(int4, int32_t, short4, int16_t);
        ADD_SVEC_CVT(int2, int32_t, byte2, int8_t);
        ADD_SVEC_CVT(int3, int32_t, byte3, int8_t);
        ADD_SVEC_CVT(int4, int32_t, byte4, int8_t);
        ADD_SVEC_CVT(uint2, uint32_t, ushort2, uint16_t);
        ADD_SVEC_CVT(uint3, uint32_t, ushort3, uint16_t);
        ADD_SVEC_CVT(uint4, uint32_t, ushort4, uint16_t);
        ADD_SVEC_CVT(uint2, uint32_t, ubyte2, uint8_t);
        ADD_SVEC_CVT(uint3, uint32_t, ubyte3, uint8_t);
        ADD_SVEC_CVT(uint4, uint32_t, ubyte4, uint8_t);
        // widen within the lattice (8-lane included — the Q8 dot shape)
        ADD_SVEC_CVT(short2, int16_t, byte2, int8_t);
        ADD_SVEC_CVT(short3, int16_t, byte3, int8_t);
        ADD_SVEC_CVT(short4, int16_t, byte4, int8_t);
        ADD_SVEC_CVT(short8, int16_t, byte8, int8_t);
        ADD_SVEC_CVT(ushort2, uint16_t, ubyte2, uint8_t);
        ADD_SVEC_CVT(ushort3, uint16_t, ubyte3, uint8_t);
        ADD_SVEC_CVT(ushort4, uint16_t, ubyte4, uint8_t);
        ADD_SVEC_CVT(ushort8, uint16_t, ubyte8, uint8_t);
        // narrow, truncating + saturating
        ADD_SVEC_CVT(short2, int16_t, int2, int32_t);      ADD_SVEC_CVT_SAT(short2, int16_t, int2, int32_t);
        ADD_SVEC_CVT(short3, int16_t, int3, int32_t);      ADD_SVEC_CVT_SAT(short3, int16_t, int3, int32_t);
        ADD_SVEC_CVT(short4, int16_t, int4, int32_t);      ADD_SVEC_CVT_SAT(short4, int16_t, int4, int32_t);
        ADD_SVEC_CVT(byte2, int8_t, int2, int32_t);        ADD_SVEC_CVT_SAT(byte2, int8_t, int2, int32_t);
        ADD_SVEC_CVT(byte3, int8_t, int3, int32_t);        ADD_SVEC_CVT_SAT(byte3, int8_t, int3, int32_t);
        ADD_SVEC_CVT(byte4, int8_t, int4, int32_t);        ADD_SVEC_CVT_SAT(byte4, int8_t, int4, int32_t);
        ADD_SVEC_CVT(ushort2, uint16_t, uint2, uint32_t);  ADD_SVEC_CVT_SAT(ushort2, uint16_t, uint2, uint32_t);
        ADD_SVEC_CVT(ushort3, uint16_t, uint3, uint32_t);  ADD_SVEC_CVT_SAT(ushort3, uint16_t, uint3, uint32_t);
        ADD_SVEC_CVT(ushort4, uint16_t, uint4, uint32_t);  ADD_SVEC_CVT_SAT(ushort4, uint16_t, uint4, uint32_t);
        ADD_SVEC_CVT(ubyte2, uint8_t, uint2, uint32_t);    ADD_SVEC_CVT_SAT(ubyte2, uint8_t, uint2, uint32_t);
        ADD_SVEC_CVT(ubyte3, uint8_t, uint3, uint32_t);    ADD_SVEC_CVT_SAT(ubyte3, uint8_t, uint3, uint32_t);
        ADD_SVEC_CVT(ubyte4, uint8_t, uint4, uint32_t);    ADD_SVEC_CVT_SAT(ubyte4, uint8_t, uint4, uint32_t);
        ADD_SVEC_CVT(byte8, int8_t, short8, int16_t);      ADD_SVEC_CVT_SAT(byte8, int8_t, short8, int16_t);
        ADD_SVEC_CVT(ubyte8, uint8_t, ushort8, uint16_t);  ADD_SVEC_CVT_SAT(ubyte8, uint8_t, ushort8, uint16_t);
        // fp16 <-> float per arity (narrowing rounds RNE per lane), + half8 <-> 2x float4
        ADD_SVEC_CVT(half2, float16_t, float2, float);
        ADD_SVEC_CVT(half3, float16_t, float3, float);
        ADD_SVEC_CVT(half4, float16_t, float4, float);
        ADD_SVEC_CVT(float2, float, half2, float16_t);
        ADD_SVEC_CVT(float3, float, half3, float16_t);
        ADD_SVEC_CVT(float4, float, half4, float16_t);
        addExtern<DAS_BIND_FUN(das_half8_pack)>(*this, lib, "half8", SideEffects::none, "das_half8_pack");
        addExtern<DAS_BIND_FUN(das_half8_lo)>(*this, lib, "half8_lo", SideEffects::none, "das_half8_lo");
        addExtern<DAS_BIND_FUN(das_half8_hi)>(*this, lib, "half8_hi", SideEffects::none, "das_half8_hi");
        // the idot family — the lattice's first compute ops (exact integer dots; results
        // always signed, operand signedness selects the overload — the OpSDot/OpSUDot split)
        addExtern<DAS_BIND_FUN(das_idot4_ss)>(*this, lib, "idot4", SideEffects::none, "das_idot4_ss")->args({"acc","a","b"});
        addExtern<DAS_BIND_FUN(das_idot4_us)>(*this, lib, "idot4", SideEffects::none, "das_idot4_us")->args({"acc","a","b"});
        addExtern<DAS_BIND_FUN(das_idot4z_ss)>(*this, lib, "idot4", SideEffects::none, "das_idot4z_ss")->args({"a","b"});
        addExtern<DAS_BIND_FUN(das_idot4z_us)>(*this, lib, "idot4", SideEffects::none, "das_idot4z_us")->args({"a","b"});
        addExtern<DAS_BIND_FUN(das_idot_ss)>(*this, lib, "idot", SideEffects::none, "das_idot_ss")->args({"a","b"});
        addExtern<DAS_BIND_FUN(das_idot_us)>(*this, lib, "idot", SideEffects::none, "das_idot_us")->args({"a","b"});
        addExtern<DAS_BIND_FUN(das_shuffle_b16)>(*this, lib, "shuffle", SideEffects::none, "das_shuffle_b16")->args({"lut","idx"});
        // the integer-lattice bit surface, parity with addFunctionVecBit on the 32-bit vector
        // families: per-lane <<//>> by a scalar count (masked to lane width; signed >> is
        // arithmetic), bitwise & | ^, and the five compound assigns — the JIT overrides all
        // of them with native vector IR
#define ADD_SVEC_BITOPS(VTYPE,ETYPE) \
    addExtern<DAS_BIND_FUN((das_sv_shr<VTYPE,ETYPE>))>(*this, lib, ">>", SideEffects::none, "das_sv_shr<" #VTYPE "," #ETYPE ">")->args({"v","count"}); \
    addExtern<DAS_BIND_FUN((das_sv_shl<VTYPE,ETYPE>))>(*this, lib, "<<", SideEffects::none, "das_sv_shl<" #VTYPE "," #ETYPE ">")->args({"v","count"}); \
    addExtern<DAS_BIND_FUN((das_sv_set_shr<VTYPE,ETYPE>))>(*this, lib, ">>=", SideEffects::modifyArgument, "das_sv_set_shr<" #VTYPE "," #ETYPE ">")->args({"v","count"}); \
    addExtern<DAS_BIND_FUN((das_sv_set_shl<VTYPE,ETYPE>))>(*this, lib, "<<=", SideEffects::modifyArgument, "das_sv_set_shl<" #VTYPE "," #ETYPE ">")->args({"v","count"}); \
    addExtern<DAS_BIND_FUN((das_sv_and<VTYPE>))>(*this, lib, "&", SideEffects::none, "das_sv_and<" #VTYPE ">")->args({"a","b"}); \
    addExtern<DAS_BIND_FUN((das_sv_or<VTYPE>))>(*this, lib, "|", SideEffects::none, "das_sv_or<" #VTYPE ">")->args({"a","b"}); \
    addExtern<DAS_BIND_FUN((das_sv_xor<VTYPE>))>(*this, lib, "^", SideEffects::none, "das_sv_xor<" #VTYPE ">")->args({"a","b"}); \
    addExtern<DAS_BIND_FUN((das_sv_set_and<VTYPE>))>(*this, lib, "&=", SideEffects::modifyArgument, "das_sv_set_and<" #VTYPE ">")->args({"a","b"}); \
    addExtern<DAS_BIND_FUN((das_sv_set_or<VTYPE>))>(*this, lib, "|=", SideEffects::modifyArgument, "das_sv_set_or<" #VTYPE ">")->args({"a","b"}); \
    addExtern<DAS_BIND_FUN((das_sv_set_xor<VTYPE>))>(*this, lib, "^=", SideEffects::modifyArgument, "das_sv_set_xor<" #VTYPE ">")->args({"a","b"});
        ADD_SVEC_BITOPS(byte2, int8_t);
        ADD_SVEC_BITOPS(byte3, int8_t);
        ADD_SVEC_BITOPS(byte4, int8_t);
        ADD_SVEC_BITOPS(byte8, int8_t);
        ADD_SVEC_BITOPS(byte16, int8_t);
        ADD_SVEC_BITOPS(ubyte2, uint8_t);
        ADD_SVEC_BITOPS(ubyte3, uint8_t);
        ADD_SVEC_BITOPS(ubyte4, uint8_t);
        ADD_SVEC_BITOPS(ubyte8, uint8_t);
        ADD_SVEC_BITOPS(ubyte16, uint8_t);
        ADD_SVEC_BITOPS(short2, int16_t);
        ADD_SVEC_BITOPS(short3, int16_t);
        ADD_SVEC_BITOPS(short4, int16_t);
        ADD_SVEC_BITOPS(short8, int16_t);
        ADD_SVEC_BITOPS(ushort2, uint16_t);
        ADD_SVEC_BITOPS(ushort3, uint16_t);
        ADD_SVEC_BITOPS(ushort4, uint16_t);
        ADD_SVEC_BITOPS(ushort8, uint16_t);
#undef ADD_SVEC_BITOPS
#undef ADD_SVEC_CVT
#undef ADD_SVEC_CVT_SAT
    }
}

