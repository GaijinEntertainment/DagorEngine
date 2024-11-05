#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/aot.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"

namespace das
{
    // string
    DEFINE_OP2_EVAL_BASIC_POLICY(char *);
    DEFINE_OP2_EVAL_ORDERED_POLICY(char *);
    DEFINE_OP2_EVAL_GROUPBYADD_POLICY(char *);

    template <typename QQ>
    struct cast <EnumStubAny<QQ>> {
        static __forceinline struct EnumStubAny<QQ> to(vec4f x)     { return prune<EnumStubAny<QQ>,vec4f>::from(x); }
        static __forceinline vec4f from ( EnumStubAny<QQ> x )       { return prune<vec4f, EnumStubAny<QQ>>::from(x); }
    };


    template<typename QQ>
    struct SimPolicy<EnumStubAny<QQ>> {
        static __forceinline auto to_enum ( vec4f val ) {
            return cast<QQ>::to(val);
        }
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_enum(a) == to_enum(b);
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_enum(a) != to_enum(b);
        }
    };

    template <> struct typeName<EnumStub>    { constexpr static const char * name() { return "enum"; } };
    template <> struct typeName<EnumStub8>   { constexpr static const char * name() { return "enum8"; } };
    template <> struct typeName<EnumStub16>  { constexpr static const char * name() { return "enum16"; } };
    template <> struct typeName<EnumStub64>  { constexpr static const char * name() { return "enum64"; } };

    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ,EnumStub);
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu,EnumStub);

    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ,EnumStub8);
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu,EnumStub8);

    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ,EnumStub16);
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu,EnumStub16);

    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ,EnumStub64);
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu,EnumStub64);

    template <>
    struct SimPolicy<Func> {
        static __forceinline SimFunction * to_func ( vec4f val ) {
            return cast<Func>::to(val).PTR;
        }
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_func(a) == to_func(b);
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_func(a) != to_func(b);
        }
    };

    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ,Func);
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu,Func);

    struct Sim_EqFunPtr : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_EqFunPtr ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "EqFunPtr", sizeof(Func), "Func");
        }
        __forceinline bool compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto lv = cast<Func>::to(l->eval(context));
            auto rv = r->evalPtr(context);
            return !rv && !lv.PTR;      // they only equal if both null
        }
    };

    struct Sim_NEqFunPtr : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_NEqFunPtr ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "NEqFunPtr", sizeof(Func), "Func");
        }
        __forceinline bool compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto lv = cast<Func>::to(l->eval(context));
            auto rv = r->evalPtr(context);
            return rv || lv.PTR;
        }
    };

    struct Sim_EqLambdaPtr : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_EqLambdaPtr ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "EqLambdaPtr", sizeof(Lambda), "Lambda");
        }
        __forceinline bool compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto lv = cast<Lambda>::to(l->eval(context));
            auto rv = r->evalPtr(context);
            return !rv && !lv.capture;      // they only equal if both null
        }
    };

    struct Sim_NEqLambdaPtr : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_NEqLambdaPtr ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "NEqLambdaPtr", sizeof(Lambda), "Lambda");
        }
        __forceinline bool compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto lv = cast<Lambda>::to(l->eval(context));
            auto rv = r->evalPtr(context);
            return rv || lv.capture;
        }
    };

    void Module_BuiltIn::addMiscTypes(ModuleLibrary & lib) {
        // enum
        addFunctionBasic<EnumStub>(*this,lib);
        addExtern<DAS_BIND_FUN(enum_to_int)>(*this, lib, "int", SideEffects::none, "int32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_uint)>(*this, lib, "uint", SideEffects::none, "uint32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_int8)>(*this, lib, "int8", SideEffects::none, "int8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_uint8)>(*this, lib, "uint8", SideEffects::none, "uint8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_int16)>(*this, lib, "int16", SideEffects::none, "int16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_uint16)>(*this, lib, "uint16", SideEffects::none, "uint16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_int64)>(*this, lib, "int64", SideEffects::none, "int64_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum_to_uint64)>(*this, lib, "uint64", SideEffects::none, "uint64_t")->arg("src");
        // enum8
        addFunctionBasic<EnumStub8>(*this,lib);
        addExtern<DAS_BIND_FUN(enum8_to_int)>(*this, lib, "int", SideEffects::none, "int32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_uint)>(*this, lib, "uint", SideEffects::none, "uint32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_int8)>(*this, lib, "int8", SideEffects::none, "int8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_uint8)>(*this, lib, "uint8", SideEffects::none, "uint8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_int16)>(*this, lib, "int16", SideEffects::none, "int16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_uint16)>(*this, lib, "uint16", SideEffects::none, "uint16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_int64)>(*this, lib, "int64", SideEffects::none, "int64_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum8_to_uint64)>(*this, lib, "uint64", SideEffects::none, "uint64_t")->arg("src");
        // enum16
        addFunctionBasic<EnumStub16>(*this,lib);
        addExtern<DAS_BIND_FUN(enum16_to_int)>(*this, lib, "int", SideEffects::none, "int32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_uint)>(*this, lib, "uint", SideEffects::none, "uint32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_int8)>(*this, lib, "int8", SideEffects::none, "int8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_uint8)>(*this, lib, "uint8", SideEffects::none, "uint8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_int16)>(*this, lib, "int16", SideEffects::none, "int16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_uint16)>(*this, lib, "uint16", SideEffects::none, "uint16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_int64)>(*this, lib, "int64", SideEffects::none, "int64_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum16_to_uint64)>(*this, lib, "uint64", SideEffects::none, "uint64_t")->arg("src");
        // enum64
        addFunctionBasic<EnumStub64>(*this,lib);
        addExtern<DAS_BIND_FUN(enum64_to_int)>(*this, lib, "int", SideEffects::none, "int32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_uint)>(*this, lib, "uint", SideEffects::none, "uint32_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_int8)>(*this, lib, "int8", SideEffects::none, "int8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_uint8)>(*this, lib, "uint8", SideEffects::none, "uint8_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_int16)>(*this, lib, "int16", SideEffects::none, "int16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_uint16)>(*this, lib, "uint16", SideEffects::none, "uint16_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_int64)>(*this, lib, "int64", SideEffects::none, "int64_t")->arg("src");
        addExtern<DAS_BIND_FUN(enum64_to_uint64)>(*this, lib, "uint64", SideEffects::none, "uint64_t")->arg("src");
        // function
        addFunctionBasic<Func>(*this,lib);
        addFunction( make_smart<BuiltInFn<Sim_EqFunPtr, bool,const Func,const void *>>("==",lib,"==",false) );
        addFunction( make_smart<BuiltInFn<Sim_NEqFunPtr,bool,const Func,const void *>>("!=",lib,"!=",false) );
        // lambda
        addFunction( make_smart<BuiltInFn<Sim_EqLambdaPtr, bool,const Lambda,const void *>>("==",lib,"==",false) );
        addFunction( make_smart<BuiltInFn<Sim_NEqLambdaPtr,bool,const Lambda,const void *>>("!=",lib,"!=",false) );
        // string
        addFunctionBasic<char *>(*this,lib);
        addFunctionOrdered<char *>(*this,lib);
        addFunctionConcat<char *>(*this,lib);
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_i8)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_i8")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_u8)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_u8")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_i16)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_i16")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_u16)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_u16")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_i32)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_i32")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_u32)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_u32")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_i64)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_i64")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_int_u64)>(*this, lib, "string",
            SideEffects::none, "das_lexical_cast_int_u64")->args({"value","hex","context","at"})->arg_init(1,make_smart<ExprConstBool>(false));
        addExtern<DAS_BIND_FUN(das_lexical_cast_fp_f)>(*this, lib, "string", SideEffects::none,
            "das_lexical_cast_fp_f")->args({"value","context","at"});
        addExtern<DAS_BIND_FUN(das_lexical_cast_fp_d)>(*this, lib, "string", SideEffects::none,
            "das_lexical_cast_fp_d")->args({"value","context","at"});
    }
}
