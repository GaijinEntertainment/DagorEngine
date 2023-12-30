#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/sim_policy.h"

#include "daScript/ast/ast.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_policy_types.h"

#include "daScript/misc/performance_time.h"

namespace das
{
    // unary
    DEFINE_OP1_NUMERIC(Unp);
    DEFINE_OP1_NUMERIC(Unm);
    DEFINE_OP1_SET_NUMERIC(Inc);
    DEFINE_OP1_SET_NUMERIC(Dec);
    DEFINE_OP1_SET_NUMERIC(IncPost);
    DEFINE_OP1_SET_NUMERIC(DecPost);
    DEFINE_OP1_NUMERIC_INTEGER(BinNot);
    DEFINE_POLICY(BoolNot);
    IMPLEMENT_OP1_POLICY(BoolNot, Bool, bool);
    // binary
    // +,-,*,/,%
    DEFINE_OP2_NUMERIC(Add);
    DEFINE_OP2_NUMERIC(Sub);
    DEFINE_OP2_NUMERIC(Mul);
    DEFINE_OP2_NUMERIC(Div);
    DEFINE_OP2_NUMERIC(Mod);
    DEFINE_OP2_SET_NUMERIC(SetAdd);
    DEFINE_OP2_SET_NUMERIC(SetSub);
    DEFINE_OP2_SET_NUMERIC(SetMul);
    DEFINE_OP2_SET_NUMERIC(SetDiv);
    DEFINE_OP2_SET_NUMERIC(SetMod);
    // comparisons
    DEFINE_OP2_BOOL_NUMERIC(Equ);
    DEFINE_OP2_BOOL_NUMERIC(NotEqu);
    DEFINE_OP2_BOOL_NUMERIC(LessEqu);
    DEFINE_OP2_BOOL_NUMERIC(GtEqu);
    DEFINE_OP2_BOOL_NUMERIC(Less);
    DEFINE_OP2_BOOL_NUMERIC(Gt);
    DEFINE_OP2_BASIC_POLICY(Bool,bool);
    DEFINE_OP2_BASIC_POLICY(Ptr,void *);
    // binary and, or, xor
    DEFINE_OP2_NUMERIC_INTEGER(BinAnd);
    DEFINE_OP2_NUMERIC_INTEGER(BinOr);
    DEFINE_OP2_NUMERIC_INTEGER(BinXor);
    DEFINE_OP2_NUMERIC_INTEGER(BinShl);
    DEFINE_OP2_NUMERIC_INTEGER(BinShr);
    DEFINE_OP2_NUMERIC_INTEGER(BinRotl);
    DEFINE_OP2_NUMERIC_INTEGER(BinRotr);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinAnd);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinOr);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinXor);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinShl);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinShr);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinRotl);
    DEFINE_OP2_SET_NUMERIC_INTEGER(SetBinRotr);
    // boolean and, or, xor
    DEFINE_POLICY(SetBoolXor);
    IMPLEMENT_OP2_SET_POLICY(SetBoolXor, Bool, bool);
    DEFINE_POLICY(BoolXor);
    IMPLEMENT_OP2_POLICY(BoolXor, Bool, bool);

#define ADD_NUMERIC_CASTS(TYPE,CTYPE)                                                                               \
    addFunction ( make_smart<BuiltInFn<SimNode_Zero,CTYPE>>(#TYPE,lib,#CTYPE,false) );                             \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,float>,CTYPE,float>>(#TYPE,lib,#CTYPE,false) );          \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,double>,CTYPE,double>>(#TYPE,lib,#CTYPE,false) );        \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,int32_t>,CTYPE,int32_t>>(#TYPE,lib,#CTYPE,false) );      \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,uint32_t>,CTYPE,uint32_t>>(#TYPE,lib,#CTYPE,false) );    \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,Bitfield>,CTYPE,Bitfield>>(#TYPE,lib,#CTYPE,false) );    \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,int8_t>,CTYPE,int8_t>>(#TYPE,lib,#CTYPE,false) );        \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,uint8_t>,CTYPE,uint8_t>>(#TYPE,lib,#CTYPE,false) );      \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,int16_t>,CTYPE,int16_t>>(#TYPE,lib,#CTYPE,false) );      \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,uint16_t>,CTYPE,uint16_t>>(#TYPE,lib,#CTYPE,false) );    \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,int64_t>,CTYPE,int64_t>>(#TYPE,lib,#CTYPE,false) );      \
    addFunction ( make_smart<BuiltInFn<SimNode_Cast<CTYPE,uint64_t>,CTYPE,uint64_t>>(#TYPE,lib,#CTYPE,false) );


#define ADD_NUMERIC_LIMITS(TYPENAME,CTYPE)  \
    addConstant<CTYPE>(*this, #TYPENAME "_MIN", (CTYPE)TYPENAME##_MIN); \
    addConstant<CTYPE>(*this, #TYPENAME "_MAX", (CTYPE)TYPENAME##_MAX);

#define ADD_NUMERIC_LIMITS_UNSIGNED(TYPENAME,CTYPE)  \
    addConstant<CTYPE>(*this, #TYPENAME "_MAX", (CTYPE)TYPENAME##_MAX);

#define ADD_NUMERIC_LIMITS_EX(TYPENAME,CONSTNAME,CTYPE)  \
    addConstant<CTYPE>(*this, #TYPENAME "_MIN", (CTYPE)CONSTNAME##_MIN); \
    addConstant<CTYPE>(*this, #TYPENAME "_MAX", (CTYPE)CONSTNAME##_MAX);

#define ADD_NUMERIC_LIMITS_EX_UNSIGNED(TYPENAME,CONSTNAME,CTYPE)  \
    addConstant<CTYPE>(*this, #TYPENAME "_MAX", (CTYPE)CONSTNAME##_MAX);

    void verifyOptions();

    Module_BuiltIn::Module_BuiltIn() : Module("$") {
        DAS_PROFILE_SECTION("Module_Builtin");
        ModuleLibrary lib(this);
        // max function arguments
        addConstant<int>(*this, "DAS_MAX_FUNCTION_ARGUMENTS", DAS_MAX_FUNCTION_ARGUMENTS);
        // boolean
        addFunctionBasic<bool>(*this,lib);
        addFunctionBoolean<bool>(*this,lib);
        // pointer
        addFunctionBasic<void *>(*this,lib);
        // storage
        ADD_NUMERIC_CASTS(int8, int8_t);
        ADD_NUMERIC_CASTS(int16, int16_t);
        ADD_NUMERIC_CASTS(uint8, uint8_t);
        ADD_NUMERIC_CASTS(uint16, uint16_t);
        // int32
        addFunctionBasic<int32_t>(*this,lib);
        addFunctionNumericWithMod<int32_t>(*this,lib);
        addFunctionIncDec<int32_t>(*this,lib);
        addFunctionOrdered<int32_t>(*this,lib);
        addFunctionBit<int32_t>(*this,lib);
        ADD_NUMERIC_CASTS(int, int32_t);
        ADD_NUMERIC_LIMITS(INT, int32_t);
        // uint32
        addFunctionBasic<uint32_t>(*this,lib);
        addFunctionNumericWithMod<uint32_t>(*this,lib);
        addFunctionIncDec<uint32_t>(*this,lib);
        addFunctionOrdered<uint32_t>(*this,lib);
        addFunctionBit<uint32_t>(*this,lib);
        ADD_NUMERIC_CASTS(uint, uint32_t);
        ADD_NUMERIC_LIMITS_UNSIGNED(UINT, uint32_t);
        // bitfields
        ADD_NUMERIC_CASTS(bitfield, Bitfield);
        addFunctionBasic<Bitfield,uint32_t>(*this,lib);
        addFunctionBitLogic<Bitfield,uint32_t>(*this,lib);
        // int64
        addFunctionBasic<int64_t>(*this,lib);
        addFunctionNumericWithMod<int64_t>(*this,lib);
        addFunctionIncDec<int64_t>(*this,lib);
        addFunctionOrdered<int64_t>(*this,lib);
        addFunctionBit<int64_t>(*this,lib);
        ADD_NUMERIC_CASTS(int64, int64_t);
        ADD_NUMERIC_LIMITS_EX(LONG, INT64, int64_t);
        // uint64
        addFunctionBasic<uint64_t>(*this,lib);
        addFunctionNumericWithMod<uint64_t>(*this,lib);
        addFunctionIncDec<uint64_t>(*this,lib);
        addFunctionOrdered<uint64_t>(*this,lib);
        addFunctionBit<uint64_t>(*this,lib);
        ADD_NUMERIC_CASTS(uint64, uint64_t);
        ADD_NUMERIC_LIMITS_EX_UNSIGNED(ULONG, UINT64, uint64_t);
        // float
        addFunctionBasic<float>(*this,lib);
        addFunctionNumericWithMod<float>(*this,lib);
        addFunctionIncDec<float>(*this,lib);
        addFunctionOrdered<float>(*this,lib);
        ADD_NUMERIC_CASTS(float, float);
        ADD_NUMERIC_LIMITS(FLT, float);
        // double
        addFunctionBasic<double>(*this,lib);
        addFunctionNumericWithMod<double>(*this,lib);
        addFunctionIncDec<double>(*this,lib);
        addFunctionOrdered<double>(*this,lib);
        ADD_NUMERIC_CASTS(double, double);
        ADD_NUMERIC_LIMITS(DBL, double);
        // small types
        addEquNeqVal<int8_t>(*this,lib);
        addEquNeqVal<uint8_t>(*this,lib);
        addEquNeqVal<int16_t>(*this,lib);
        addEquNeqVal<uint16_t>(*this,lib);
        // misc types
        addMiscTypes(lib);
        // VECTOR & MATRIX TYPES
        addVectorTypes(lib);
        addVectorCtor(lib);
        // ARRAYS
        addArrayTypes(lib);
        // RUNTIME
        addRuntime(lib);
        addRuntimeSort(lib);
        // TIME
        addTime(lib);
        // NOW, for the builtin module
        appendCompiledFunctions();
        // lets verify options (it is here because its the builtin module)
        verifyOptions();
        // lets verify all names
        uint32_t verifyFlags = uint32_t(VerifyBuiltinFlags::verifyAll);
        verifyFlags &= ~VerifyBuiltinFlags::verifyGlobals;      // we skip globals due to INT_MAX etc
        verifyFlags &= ~VerifyBuiltinFlags::verifyHandleTypes;  // we skip annotatins due to StringBuilderWriter
        verifyBuiltinNames(verifyFlags);
        // lets make sure its all aot ready
        verifyAotReady();
    }
}

REGISTER_MODULE_IN_NAMESPACE(Module_BuiltIn,das);

