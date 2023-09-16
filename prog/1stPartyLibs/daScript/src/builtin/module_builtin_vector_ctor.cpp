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
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_1);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f argValue;
            evalArgs(context, &argValue);
            return Policy::splats(cast<TT>::to(argValue));
        }
    };

    template <typename TT, typename Policy>
    struct SimNode_VecCtor<TT,Policy,2> : SimNode_CallBase {
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_2);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
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
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_3);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_SUB(arguments[2]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
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
        SimNode_VecCtor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecCtor_4);
            V_SUB(arguments[0]);
            V_SUB(arguments[1]);
            V_SUB(arguments[2]);
            V_SUB(arguments[3]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
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
        SimNode_Range1Ctor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Range1Ctor);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
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
        SimNode_VecPassThrough(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(VecPassThrough);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
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
addFunction ( make_smart<BuiltInFn<SimNode_Zero,VTYPE>> (#VTYPE,lib,"v_zero",false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,1>,VTYPE,float>>   (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,1>,VTYPE,double>>  (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,1>,VTYPE,int32_t>> (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,1>,VTYPE,uint32_t>>(#VTYPE,lib,VNAME,false) );

#define ADD_RANGE_CTOR_1(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_Zero,VTYPE>> (#VTYPE,lib,"v_zero",false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<float,   SimPolicy<VTYPE>>,VTYPE,float>>   (#VTYPE,lib,"mk_" VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<double,  SimPolicy<VTYPE>>,VTYPE,double>>  (#VTYPE,lib,"mk_" VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<int32_t, SimPolicy<VTYPE>>,VTYPE,int32_t>> (#VTYPE,lib,"mk_" VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<uint32_t,SimPolicy<VTYPE>>,VTYPE,uint32_t>>(#VTYPE,lib,"mk_" VNAME,false) );

#define ADD_RANGE64_CTOR_1(VTYPE,VNAME) \
ADD_RANGE_CTOR_1(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<int64_t, SimPolicy<VTYPE>>,VTYPE,int64_t>> (#VTYPE,lib,"mk_" VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_Range1Ctor<uint64_t,SimPolicy<VTYPE>>,VTYPE,uint64_t>>(#VTYPE,lib,"mk_" VNAME,false) );

#define ADD_VEC_CTOR_2(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,2>,VTYPE,float,float>>      (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,2>,VTYPE,double,double>>    (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,2>,VTYPE,int32_t,int32_t>>  (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,2>,VTYPE,uint32_t,uint32_t>>(#VTYPE,lib,VNAME,false) );

#define ADD_VEC64_CTOR_2(VTYPE,VNAME) \
ADD_VEC_CTOR_2(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int64_t, SimPolicy<VTYPE>,2>,VTYPE,int64_t,int64_t>>  (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint64_t,SimPolicy<VTYPE>,2>,VTYPE,uint64_t,uint64_t>>(#VTYPE,lib,VNAME,false) );

#define ADD_VEC_CTOR_3(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,3>,VTYPE,float,float,float>>         (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,3>,VTYPE,double,double,double>>      (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,3>,VTYPE,int32_t,int32_t,int32_t>>   (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,3>,VTYPE,uint32_t,uint32_t,uint32_t>>(#VTYPE,lib,VNAME,false) );

#define ADD_VEC_CTOR_4(VTYPE,VNAME) \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<float,   SimPolicy<VTYPE>,4>,VTYPE,float,float,float,float>>            (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<double,  SimPolicy<VTYPE>,4>,VTYPE,double,double,double,double>>        (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int32_t, SimPolicy<VTYPE>,4>,VTYPE,int32_t,int32_t,int32_t,int32_t>>    (#VTYPE,lib,VNAME,false) ); \
addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<VTYPE>,4>,VTYPE,uint32_t,uint32_t,uint32_t,uint32_t>>(#VTYPE,lib,VNAME,false) );

    struct SimNode_Int4ToFloat4 : SimNode_CallBase {
        SimNode_Int4ToFloat4(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Int4ToFloat4);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f i4 = arguments[0]->eval(context);
            return v_cvt_vec4f(v_cast_vec4i(i4));
        }
    };

    struct SimNode_UInt4ToFloat4 : SimNode_CallBase {
        SimNode_UInt4ToFloat4(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(UInt4ToFloat4);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f i4 = arguments[0]->eval(context);
            return v_cvtu_vec4f_ieee(v_cast_vec4i(i4));
        }
    };

    struct SimNode_Float4ToInt4 : SimNode_CallBase {
        SimNode_Float4ToInt4(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Float4ToInt4);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f f4 = arguments[0]->eval(context);
            return v_cast_vec4f(v_cvt_vec4i(f4));
        }
    };

    struct SimNode_Float4ToUInt4 : SimNode_CallBase {
        SimNode_Float4ToUInt4(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(Float4ToUInt4);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            vec4f f4 = arguments[0]->eval(context);
            return v_cast_vec4f(v_cvtu_vec4i_ieee(f4));
        }
    };

    struct SimNode_AnyIntToAnyInt : SimNode_CallBase {
        SimNode_AnyIntToAnyInt(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(AnyIntToAnyInt);
            V_SUB(arguments[0]);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            DAS_PROFILE_NODE
            return arguments[0]->eval(context);
        }
    };

    void Module_BuiltIn::addVectorCtor(ModuleLibrary & lib) {
        // float2
        ADD_VEC_CTOR_1(float2,"v_splats");
        ADD_VEC_CTOR_2(float2,"float2");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, float2, float2>>("float2",lib,"float2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Int4ToFloat4, float2, int2>>("float2",lib,"cvt_ifloat2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_UInt4ToFloat4,float2,uint2>>("float2",lib,"cvt_ufloat2",false) );
        // float3
        ADD_VEC_CTOR_1(float3,"v_splats");
        ADD_VEC_CTOR_3(float3,"float3");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, float3, float3>>("float3",lib,"float3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Int4ToFloat4, float3, int3>>("float3",lib,"cvt_ifloat3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_UInt4ToFloat4,float3,uint3>>("float3",lib,"cvt_ufloat3",false) );
        addExtern<DAS_BIND_FUN(float3_from_xy_z)> (*this, lib, "float3", SideEffects::none, "float3_from_xy_z");
        addExtern<DAS_BIND_FUN(float3_from_x_yz)> (*this, lib, "float3", SideEffects::none, "float3_from_x_yz");
        // float4
        ADD_VEC_CTOR_1(float4,"v_splats");
        ADD_VEC_CTOR_4(float4,"float4");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, float4, float4>>("float4",lib,"float4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Int4ToFloat4, float4, int4>>("float4",lib,"cvt_ifloat4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_UInt4ToFloat4,float4,uint4>>("float4",lib,"cvt_ufloat4",false) );
        addExtern<DAS_BIND_FUN(float4_from_xyz_w)>  (*this, lib, "float4", SideEffects::none, "float4_from_xyz_w");
        addExtern<DAS_BIND_FUN(float4_from_x_yzw)>  (*this, lib, "float4", SideEffects::none, "float4_from_x_yzw");
        addExtern<DAS_BIND_FUN(float4_from_xy_zw)>  (*this, lib, "float4", SideEffects::none, "float4_from_xy_zw");
        addExtern<DAS_BIND_FUN(float4_from_xy_z_w)> (*this, lib, "float4", SideEffects::none, "float4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(float4_from_x_yz_w)> (*this, lib, "float4", SideEffects::none, "float4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(float4_from_x_y_zw)> (*this, lib, "float4", SideEffects::none, "float4_from_x_y_zw");
        // int2
        ADD_VEC_CTOR_1(int2,"int2");
        ADD_VEC_CTOR_2(int2,"int2");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, int2, int2>>("int2",lib,"int2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToInt4, int2, float2>>("int2",lib,"cvt_int2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,int2, uint2>>("int2",lib,"cvt_pass",false) );
        // int3
        ADD_VEC_CTOR_1(int3,"int3");
        ADD_VEC_CTOR_3(int3,"int3");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, int3, int3>>("int3",lib,"int3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToInt4, int3, float3>>("int3",lib,"cvt_int3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,int3, uint3>>("int3",lib,"cvt_pass",false) );
        addExtern<DAS_BIND_FUN(int3_from_xy_z)> (*this, lib, "int3", SideEffects::none, "int3_from_xy_z");
        addExtern<DAS_BIND_FUN(int3_from_x_yz)> (*this, lib, "int3", SideEffects::none, "int3_from_x_yz");
        // int4
        ADD_VEC_CTOR_1(int4,"int4");
        ADD_VEC_CTOR_4(int4,"int4");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, int4, int4>>("int4",lib,"int4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToInt4, int4, float4>>("int4",lib,"cvt_int4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,int4, uint4>>("int4",lib,"cvt_pass",false) );
        addExtern<DAS_BIND_FUN(int4_from_xyz_w)>  (*this, lib, "int4", SideEffects::none, "int4_from_xyz_w");
        addExtern<DAS_BIND_FUN(int4_from_x_yzw)>  (*this, lib, "int4", SideEffects::none, "int4_from_x_yzw");
        addExtern<DAS_BIND_FUN(int4_from_xy_zw)>  (*this, lib, "int4", SideEffects::none, "int4_from_xy_zw");
        addExtern<DAS_BIND_FUN(int4_from_xy_z_w)> (*this, lib, "int4", SideEffects::none, "int4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(int4_from_x_yz_w)> (*this, lib, "int4", SideEffects::none, "int4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(int4_from_x_y_zw)> (*this, lib, "int4", SideEffects::none, "int4_from_x_y_zw");
        // uint2
        ADD_VEC_CTOR_1(uint2,"uint2");
        ADD_VEC_CTOR_2(uint2,"uint2");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, uint2, uint2>>("uint2",lib,"uint2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToUInt4,uint2,float2>>("uint2",lib,"cvt_uint2",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,uint2, int2>>("uint2",lib,"cvt_pass",false) );
        // uint3
        ADD_VEC_CTOR_1(uint3,"uint3");
        ADD_VEC_CTOR_3(uint3,"uint3");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, uint3, uint3>>("uint3",lib,"uint3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToUInt4,uint3,float3>>("uint3",lib,"cvt_uint3",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,uint3, int3>>("uint3",lib,"cvt_pass",false) );
        addExtern<DAS_BIND_FUN(uint3_from_xy_z)> (*this, lib, "uint3", SideEffects::none, "uint3_from_xy_z");
        addExtern<DAS_BIND_FUN(uint3_from_x_yz)> (*this, lib, "uint3", SideEffects::none, "uint3_from_x_yz");
        // uint4
        ADD_VEC_CTOR_1(uint4,"uint4");
        ADD_VEC_CTOR_4(uint4,"uint4");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, uint4, uint4>>("uint4",lib,"uint4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_Float4ToUInt4,uint4,float4>>("uint4",lib,"cvt_uint4",false) );
        addFunction( make_smart<BuiltInFn<SimNode_AnyIntToAnyInt,uint4, int4>>("uint4",lib,"cvt_pass",false) );
        addExtern<DAS_BIND_FUN(uint4_from_xyz_w)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_xyz_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_yzw)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_x_yzw");
        addExtern<DAS_BIND_FUN(uint4_from_xy_zw)>  (*this, lib, "uint4", SideEffects::none, "uint4_from_xy_zw");
        addExtern<DAS_BIND_FUN(uint4_from_xy_z_w)> (*this, lib, "uint4", SideEffects::none, "uint4_from_xy_z_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_yz_w)> (*this, lib, "uint4", SideEffects::none, "uint4_from_x_yz_w");
        addExtern<DAS_BIND_FUN(uint4_from_x_y_zw)> (*this, lib, "uint4", SideEffects::none, "uint4_from_x_y_zw");
        // range
        ADD_RANGE_CTOR_1(range,"range");
        ADD_VEC_CTOR_2(range,"range");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, range, range>>("range",lib,"range",false) );
        addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int32_t,SimPolicy<range>,2>,range,int32_t,int32_t>>("interval",lib,"range",false) );
        // urange
        ADD_RANGE_CTOR_1(urange,"urange");
        ADD_VEC_CTOR_2(urange,"urange");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, urange, urange>>("urange",lib,"urange",false) );
        addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint32_t,SimPolicy<urange>,2>,urange,uint32_t,uint32_t>>("interval",lib,"urange",false) );
        // range64
        ADD_RANGE64_CTOR_1(range64,"range64");
        ADD_VEC64_CTOR_2(range64,"range64");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, range64, range64>>("range64",lib,"range64",false) );
        addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<int64_t,SimPolicy<range64>,2>,range64,int64_t,int64_t>>("interval",lib,"range64",false) );
        // urange64
        ADD_RANGE64_CTOR_1(urange64,"urange64");
        ADD_VEC64_CTOR_2(urange64,"urange64");
        addFunction( make_smart<BuiltInFn<SimNode_VecPassThrough, urange64, urange64>>("urange64",lib,"urange64",false) );
        addFunction ( make_smart<BuiltInFn<SimNode_VecCtor<uint64_t,SimPolicy<urange64>,2>,urange64,uint64_t,uint64_t>>("interval",lib,"urange64",false) );
    }
}

