#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/simulate/sim_policy.h"

namespace das
{
    DEFINE_VECTOR_POLICY(float2);
    DEFINE_VECTOR_POLICY(float3);
    DEFINE_VECTOR_POLICY(float4);
    DEFINE_VECTOR_POLICY(int2);
    DEFINE_VECTOR_POLICY(int3);
    DEFINE_VECTOR_POLICY(int4);
    DEFINE_VECTOR_POLICY(uint2);
    DEFINE_VECTOR_POLICY(uint3);
    DEFINE_VECTOR_POLICY(uint4);

    DEFINE_OP2_EVAL_BASIC_POLICY(range);
    DEFINE_OP2_EVAL_BASIC_POLICY(urange);
    DEFINE_OP2_EVAL_BASIC_POLICY(range64);
    DEFINE_OP2_EVAL_BASIC_POLICY(urange64);

#define DEFINE_VECTOR_BIN_POLICY(CTYPE)                 \
    IMPLEMENT_OP2_EVAL_POLICY(BinAnd, CTYPE);           \
    IMPLEMENT_OP2_EVAL_POLICY(BinOr,  CTYPE);           \
    IMPLEMENT_OP2_EVAL_POLICY(BinXor, CTYPE);           \
    IMPLEMENT_OP2_EVAL_POLICY(BinShl, CTYPE);           \
    IMPLEMENT_OP2_EVAL_POLICY(BinShr, CTYPE);           \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetBinAnd, CTYPE);    \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetBinOr,  CTYPE);    \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetBinXor, CTYPE);    \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetBinShl, CTYPE);    \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetBinShr, CTYPE);

    DEFINE_VECTOR_BIN_POLICY(int2);
    DEFINE_VECTOR_BIN_POLICY(int3);
    DEFINE_VECTOR_BIN_POLICY(int4);
    DEFINE_VECTOR_BIN_POLICY(uint2);
    DEFINE_VECTOR_BIN_POLICY(uint3);
    DEFINE_VECTOR_BIN_POLICY(uint4);

    // built-in numeric types
    template <typename TT>
    void addFunctionVecBit(Module & mod, const ModuleLibrary & lib) {
        //                                     policy              ret   arg1 arg2    name
     // mod.addFunction( make_smart<BuiltInFn<Sim_BinNot<TT>,     TT,   TT>            >("~",      lib) );
        mod.addFunction( make_smart<BuiltInFn<Sim_BinAnd<TT>,     TT,   TT,  TT>       >("&",      lib, "BinAnd") );
        mod.addFunction( make_smart<BuiltInFn<Sim_BinOr<TT>,      TT,   TT,  TT>       >("|",      lib, "BinOr") );
        mod.addFunction( make_smart<BuiltInFn<Sim_BinXor<TT>,     TT,   TT,  TT>       >("^",      lib, "BinXor") );
        mod.addFunction( make_smart<BuiltInFn<Sim_BinShl<TT>,     TT,   TT,  int32_t>  >("<<",     lib, "BinShl") );
        mod.addFunction( make_smart<BuiltInFn<Sim_BinShr<TT>,     TT,   TT,  int32_t>  >(">>",     lib, "BinShr") );
        mod.addFunction( make_smart<BuiltInFn<Sim_SetBinAnd<TT>,  void, TT&, TT>       >("&=",     lib, "SetBinAnd")
                        ->setSideEffects(SideEffects::modifyArgument) );
        mod.addFunction( make_smart<BuiltInFn<Sim_SetBinOr<TT>,   void, TT&, TT>       >("|=",     lib, "SetBinOr")
                        ->setSideEffects(SideEffects::modifyArgument) );
        mod.addFunction( make_smart<BuiltInFn<Sim_SetBinXor<TT>,  void, TT&, TT>       >("^=",     lib, "SetBinXor")
                        ->setSideEffects(SideEffects::modifyArgument) );
        mod.addFunction( make_smart<BuiltInFn<Sim_SetBinShl<TT>,  void, TT&, int32_t>  >("<<=",    lib, "SetBinShl")
                        ->setSideEffects(SideEffects::modifyArgument) );
        mod.addFunction( make_smart<BuiltInFn<Sim_SetBinShr<TT>,  void, TT&, int32_t>  >(">>=",    lib, "SetBinShr")
                        ->setSideEffects(SideEffects::modifyArgument) );
    }

    void Module_BuiltIn::addVectorTypes(ModuleLibrary & lib) {
        // float2
        addFunctionBasic<float2>(*this,lib);
        addFunctionNumeric<float2>(*this,lib);
        addFunctionVecNumeric<float2,float>(*this,lib);
        // float3
        addFunctionBasic<float3>(*this,lib);
        addFunctionNumeric<float3>(*this,lib);
        addFunctionVecNumeric<float3, float>(*this,lib);
        // float4
        addFunctionBasic<float4>(*this,lib);
        addFunctionNumeric<float4>(*this,lib);
        addFunctionVecNumeric<float4, float>(*this,lib);
        // int2
        addFunctionBasic<int2>(*this,lib);
        addFunctionNumeric<int2>(*this,lib);
        addFunctionVecNumeric<int2, int32_t>(*this,lib);
        addFunctionVecBit<int2>(*this,lib);
        // int3
        addFunctionBasic<int3>(*this,lib);
        addFunctionNumeric<int3>(*this,lib);
        addFunctionVecNumeric<int3, int32_t>(*this,lib);
        addFunctionVecBit<int3>(*this,lib);
        // int4
        addFunctionBasic<int4>(*this,lib);
        addFunctionNumeric<int4>(*this,lib);
        addFunctionVecNumeric<int4,int32_t>(*this,lib);
        addFunctionVecBit<int4>(*this,lib);
        // uint2
        addFunctionBasic<uint2>(*this,lib);
        addFunctionNumeric<uint2>(*this,lib);
        addFunctionVecNumeric<uint2, uint32_t>(*this,lib);
        addFunctionVecBit<uint2>(*this,lib);
        // uint3
        addFunctionBasic<uint3>(*this,lib);
        addFunctionNumeric<uint3>(*this,lib);
        addFunctionVecNumeric<uint3,uint32_t>(*this,lib);
        addFunctionVecBit<uint3>(*this,lib);
        // uint4
        addFunctionBasic<uint4>(*this,lib);
        addFunctionNumeric<uint4>(*this,lib);
        addFunctionVecNumeric<uint4, uint32_t>(*this,lib);
        addFunctionVecBit<uint4>(*this,lib);
        // range
        addFunctionBasic<range>(*this,lib);
        // urange
        addFunctionBasic<urange>(*this,lib);
        // range64
        addFunctionBasic<range64>(*this,lib);
        // urange64
        addFunctionBasic<urange64>(*this,lib);
    }
}
