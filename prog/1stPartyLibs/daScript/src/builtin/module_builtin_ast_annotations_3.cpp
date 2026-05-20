#include "daScript/misc/platform.h"

#include "module_builtin_rtti.h"

#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/simulate/aot_builtin_string.h"
#include "daScript/misc/performance_time.h"

#include "module_builtin_ast.h"

using namespace das;

namespace das {
    struct AstExprLabelAnnotation : AstExpressionAnnotation<ExprLabel> {
        AstExprLabelAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprLabel> ("ExprLabel", ml) {
            addField<DAS_BIND_MANAGED_FIELD(label)>("labelName","label");
            addField<DAS_BIND_MANAGED_FIELD(comment)>("comment");
        }
    };

    struct AstExprGotoAnnotation : AstExpressionAnnotation<ExprGoto> {
        AstExprGotoAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprGoto> ("ExprGoto", ml) {
            addField<DAS_BIND_MANAGED_FIELD(label)>("labelName","label");
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
        }
    };

    struct AstExprRef2ValueAnnotation : AstExpressionAnnotation<ExprRef2Value> {
        AstExprRef2ValueAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprRef2Value> ("ExprRef2Value", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
        }
    };

    struct AstExprRef2PtrAnnotation : AstExpressionAnnotation<ExprRef2Ptr> {
        AstExprRef2PtrAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprRef2Ptr> ("ExprRef2Ptr", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
        }
    };

    struct AstExprAddrAnnotation : AstExpressionAnnotation<ExprAddr> {
        AstExprAddrAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprAddr> ("ExprAddr", ml) {
            addField<DAS_BIND_MANAGED_FIELD(target)>("target");
            addField<DAS_BIND_MANAGED_FIELD(funcType)>("funcType");
            addField<DAS_BIND_MANAGED_FIELD(func)>("func");
        }
    };

    struct AstExprAssertAnnotation : AstExprLikeCallAnnotation<ExprAssert> {
        AstExprAssertAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprAssert> ("ExprAssert", ml) {
            addField<DAS_BIND_MANAGED_FIELD(isVerify)>("isVerify");
        }
    };

    struct AstExprQuoteAnnotation : AstExprLikeCallAnnotation<ExprQuote> {
        AstExprQuoteAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprQuote> ("ExprQuote", ml) {
        }
    };

    struct AstExprStaticAssertAnnotation : AstExprLikeCallAnnotation<ExprStaticAssert> {
        AstExprStaticAssertAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprStaticAssert> ("ExprStaticAssert", ml) {
        }
    };

    struct AstExprDebugAnnotation : AstExprLikeCallAnnotation<ExprDebug> {
        AstExprDebugAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprDebug> ("ExprDebug", ml) {
        }
    };

    struct AstExprInvokeAnnotation : AstExprLikeCallAnnotation<ExprInvoke> {
        AstExprInvokeAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprInvoke> ("ExprInvoke", ml) {
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(doesNotNeedSp)>("doesNotNeedSp");
            addField<DAS_BIND_MANAGED_FIELD(cmresAlias)>("cmresAlias");
            addField<DAS_BIND_MANAGED_FIELD(isInvokeMethod)>("isInvokeMethod");
            addProperty<DAS_BIND_MANAGED_PROP(isCopyOrMove)>("isCopyOrMove");
        }
    };

    struct AstExprEraseAnnotation : AstExprLikeCallAnnotation<ExprErase> {
        AstExprEraseAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprErase> ("ExprErase", ml) {
        }
    };

    struct AstExprSetInsertAnnotation : AstExprLikeCallAnnotation<ExprSetInsert> {
        AstExprSetInsertAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprSetInsert> ("ExprSetInsert", ml) {
        }
    };

    struct AstExprFindAnnotation : AstExprLikeCallAnnotation<ExprFind> {
        AstExprFindAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprFind> ("ExprFind", ml) {
        }
    };

    struct AstExprKeyExistsAnnotation : AstExprLikeCallAnnotation<ExprKeyExists> {
        AstExprKeyExistsAnnotation(ModuleLibrary & ml)
            :  AstExprLikeCallAnnotation<ExprKeyExists> ("ExprKeyExists", ml) {
        }
    };

    struct AstExprAscendAnnotation : AstExpressionAnnotation<ExprAscend> {
        AstExprAscendAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprAscend> ("ExprAscend", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(ascType)>("ascType");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addFieldEx ( "ascendFlags", "ascendFlags", offsetof(ExprAscend, ascendFlags), makeExprAscendFlags() );
        }
    };

    struct AstExprCastAnnotation : AstExpressionAnnotation<ExprCast> {
        AstExprCastAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprCast> ("ExprCast", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(castType)>("castType");
            addFieldEx ( "castFlags", "castFlags", offsetof(ExprCast, castFlags), makeExprCastFlags() );
        }
    };

    struct AstExprDeleteAnnotation : AstExpressionAnnotation<ExprDelete> {
        AstExprDeleteAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprDelete> ("ExprDelete", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(sizeexpr)>("sizeexpr");
            addField<DAS_BIND_MANAGED_FIELD(native)>("native");
        }
    };

    struct AstExprVarAnnotation : AstExpressionAnnotation<ExprVar> {
        AstExprVarAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprVar> ("ExprVar", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(variable)>("variable");
            addField<DAS_BIND_MANAGED_FIELD(pBlock)>("pBlock");
            addField<DAS_BIND_MANAGED_FIELD(argumentIndex)>("argumentIndex");
            addFieldEx ( "varFlags", "varFlags", offsetof(ExprVar, varFlags), makeExprVarFlags() );
        }
    };

    struct AstExprTagAnnotation : AstExpressionAnnotation<ExprTag> {
        AstExprTagAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprTag> ("ExprTag", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct AstExprSafeFieldAnnotation : AstExprFieldAnnotation<ExprSafeField> {
        AstExprSafeFieldAnnotation(ModuleLibrary & ml)
            :  AstExprFieldAnnotation<ExprSafeField> ("ExprSafeField", ml) {
            addField<DAS_BIND_MANAGED_FIELD(skipQQ)>("skipQQ");
        }
    };

    struct AstExprSwizzleAnnotation : AstExpressionAnnotation<ExprSwizzle> {
        AstExprSwizzleAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprSwizzle> ("ExprSwizzle", ml) {
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            addField<DAS_BIND_MANAGED_FIELD(mask)>("mask");
            addField<DAS_BIND_MANAGED_FIELD(fields)>("fields");
            addFieldEx ( "fieldFlags", "fieldFlags", offsetof(ExprSwizzle, fieldFlags), makeExprSwizzleFieldFlags() );
        }
    };

    struct AstExprSafeAsVariantAnnotation : AstExprFieldAnnotation<ExprSafeAsVariant> {
        AstExprSafeAsVariantAnnotation(ModuleLibrary & ml)
            :  AstExprFieldAnnotation<ExprSafeAsVariant> ("ExprSafeAsVariant", ml) {
            addField<DAS_BIND_MANAGED_FIELD(skipQQ)>("skipQQ");
        }
    };

    struct AstExprOp1Annotation : AstExprOpAnnotation<ExprOp1> {
        AstExprOp1Annotation(ModuleLibrary & ml)
            :  AstExprOpAnnotation<ExprOp1> ("ExprOp1", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
        }
    };

    struct AstExprYieldAnnotation : AstExpressionAnnotation<ExprYield> {
        AstExprYieldAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprYield> ("ExprYield", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addFieldEx ( "returnFlags", "returnFlags", offsetof(ExprYield, returnFlags), makeExprYieldFlags() );
        }
    };

    struct AstExprReturnAnnotation : AstExpressionAnnotation<ExprReturn> {
        AstExprReturnAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprReturn> ("ExprReturn", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(refStackTop)>("refStackTop");
            addField<DAS_BIND_MANAGED_FIELD(returnFunc)>("returnFunc");
            addField<DAS_BIND_MANAGED_FIELD(block)>("_block","block");
            addFieldEx ( "returnFlags", "returnFlags", offsetof(ExprReturn, returnFlags), makeExprReturnFlags() );
        }
    };

    struct AstCaptureEntryAnnotation : ManagedStructureAnnotation<CaptureEntry> {
        AstCaptureEntryAnnotation(ModuleLibrary & ml)
            :  ManagedStructureAnnotation<CaptureEntry> ("CaptureEntry", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(mode)>("mode");
        }
    };

    struct AstExprMakeBlockAnnotation : AstExpressionAnnotation<ExprMakeBlock> {
        AstExprMakeBlockAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprMakeBlock> ("ExprMakeBlock", ml) {
            addField<DAS_BIND_MANAGED_FIELD(block)>("_block","block");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(capture)>("_capture", "capture");
            addField<DAS_BIND_MANAGED_FIELD(aotFunctorName)>("aotFunctorName");
            addFieldEx ( "mmFlags", "mmFlags", offsetof(ExprMakeBlock, mmFlags), makeExprMakeBlockFlags() );
        }
    };

    struct AstExprMakeGeneratorAnnotation : AstExprLooksLikeCallAnnotation<ExprMakeGenerator> {
        AstExprMakeGeneratorAnnotation(ModuleLibrary & ml)
            :  AstExprLooksLikeCallAnnotation<ExprMakeGenerator> ("ExprMakeGenerator", ml) {
            addField<DAS_BIND_MANAGED_FIELD(iterType)>("iterType");
            addField<DAS_BIND_MANAGED_FIELD(capture)>("_capture", "capture");
        }
    };

    struct AstExprConstEnumerationAnnotation : AstExprConstAnnotation<ExprConstEnumeration> {
        AstExprConstEnumerationAnnotation(ModuleLibrary & ml)
            :  AstExprConstAnnotation<ExprConstEnumeration> ("ExprConstEnumeration", ml) {
            addField<DAS_BIND_MANAGED_FIELD(enumType)>("enumType");
            addField<DAS_BIND_MANAGED_FIELD(text)>("value","text");
        }
    };

    template <typename EXPR, typename TT>
    struct AstExprConstTAnnotation : AstExprConstAnnotation<EXPR> {
        AstExprConstTAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprConstAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template init<TT>(ml);
            this->template addProperty<DAS_BIND_MANAGED_PROP(getValue)>("getValue");
        }
    };

    struct AstExprConstBitfieldAnnotation : AstExprConstTAnnotation<ExprConstBitfield,Bitfield> {
        AstExprConstBitfieldAnnotation(ModuleLibrary & ml)
            :  AstExprConstTAnnotation<ExprConstBitfield,Bitfield> ("ExprConstBitfield", ml) {
            addField<DAS_BIND_MANAGED_FIELD(bitfieldType)>("bitfieldType");

        }
    };

    struct AstExprConstStringAnnotation : AstExprConstAnnotation<ExprConstString> {
        AstExprConstStringAnnotation(ModuleLibrary & ml)
            :  AstExprConstAnnotation<ExprConstString> ("ExprConstString", ml) {
            addField<DAS_BIND_MANAGED_FIELD(text)>("value","text");
        }
    };

    struct AstExprUnsafeAnnotation : AstExpressionAnnotation<ExprUnsafe> {
        AstExprUnsafeAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprUnsafe> ("ExprUnsafe", ml) {
            addField<DAS_BIND_MANAGED_FIELD(body)>("body");
        }
    };

    void Module_Ast::registerAnnotations3(ModuleLibrary & lib) {
        // expressions with no extra syntax
        addExpressionAnnotation(new AstExprLabelAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprGotoAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprRef2ValueAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprRef2PtrAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprAddrAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprAssertAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprQuoteAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprStaticAssertAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprDebugAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprInvokeAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprEraseAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprSetInsertAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprFindAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprKeyExistsAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprAscendAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprCastAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprDeleteAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprVarAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprTagAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprSwizzleAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprFieldAnnotation<ExprField>("ExprField",lib))->from("Expression");
        addExpressionAnnotation(new AstExprSafeFieldAnnotation(lib))->from("ExprField");
        addExpressionAnnotation(new AstExprFieldAnnotation<ExprIsVariant>("ExprIsVariant",lib))->from("ExprField");
        addExpressionAnnotation(new AstExprFieldAnnotation<ExprAsVariant>("ExprAsVariant",lib))->from("ExprField");
        addExpressionAnnotation(new AstExprSafeAsVariantAnnotation(lib))->from("ExprField");
        addExpressionAnnotation(new AstExprOp1Annotation(lib))->from("ExprOp");
        addExpressionAnnotation(new AstExprReturnAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprYieldAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExpressionAnnotation<ExprBreak>("ExprBreak",lib))->from("Expression");
        addExpressionAnnotation(new AstExpressionAnnotation<ExprContinue>("ExprContinue",lib))->from("Expression");
        addExpressionAnnotation(new AstExprConstAnnotation<ExprConst>("ExprConst",lib))->from("Expression");
        addExpressionAnnotation(new AstExprConstAnnotation<ExprFakeContext>("ExprFakeContext",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprFakeLineInfo,void *>("ExprFakeLineInfo",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstPtr,void *>("ExprConstPtr",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt8 ,int8_t> ("ExprConstInt8",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt16,int16_t>("ExprConstInt16",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt64,int64_t>("ExprConstInt64",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt  ,int32_t>("ExprConstInt",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt2 ,int2>   ("ExprConstInt2",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt3 ,int3>   ("ExprConstInt3",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstInt4 ,int4>   ("ExprConstInt4",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt8 ,uint8_t> ("ExprConstUInt8",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt16,uint16_t>("ExprConstUInt16",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt64,uint64_t>("ExprConstUInt64",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt  ,uint32_t>("ExprConstUInt",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt2 ,uint2>   ("ExprConstUInt2",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt3 ,uint3>   ("ExprConstUInt3",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstUInt4 ,uint4>   ("ExprConstUInt4",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstRange ,range> ("ExprConstRange",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstURange,urange>("ExprConstURange",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstRange64 ,range64> ("ExprConstRange64",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstURange64,urange64>("ExprConstURange64",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstFloat  ,float> ("ExprConstFloat",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstFloat2 ,float2>("ExprConstFloat2",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstFloat3 ,float3>("ExprConstFloat3",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstFloat4 ,float4>("ExprConstFloat4",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstDouble,double> ("ExprConstDouble",lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstTAnnotation<ExprConstBool,bool> ("ExprConstBool",lib))->from("ExprConst");
        addAnnotation(new AstCaptureEntryAnnotation(lib));
        addExpressionAnnotation(new AstExprMakeBlockAnnotation(lib))->from("Expression");
        addExpressionAnnotation(new AstExprMakeGeneratorAnnotation(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprLikeCallAnnotation<ExprMemZero>("ExprMemZero",lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(new AstExprConstEnumerationAnnotation(lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstBitfieldAnnotation(lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprConstStringAnnotation(lib))->from("ExprConst");
        addExpressionAnnotation(new AstExprUnsafeAnnotation(lib))->from("Expression");
        // vector functions for custom containers
        addExtern<DAS_BIND_FUN(mks_vector_emplace)>(*this, lib, "emplace",
            SideEffects::modifyArgument, "mks_vector_emplace")
                ->args({"vec","value"})->generated = true;
        addExtern<DAS_BIND_FUN(mks_vector_pop)>(*this, lib, "pop",
            SideEffects::modifyArgument, "mks_vector_pop")
                ->arg("vec")->generated = true;
        addExtern<DAS_BIND_FUN(mks_vector_clear)>(*this, lib, "clear",
            SideEffects::modifyArgument, "mks_vector_clear")
                ->arg("vec")->generated = true;
        addExtern<DAS_BIND_FUN(mks_vector_resize)>(*this, lib, "resize",
            SideEffects::modifyArgument, "mks_vector_resize")
                ->args({"vec","newSize"})->generated = true;
    }
}
