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
            addField<DAS_BIND_MANAGED_FIELD(isInvokeMethod)>("isInvokeMethod");
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
            addField<DAS_BIND_MANAGED_FIELD(block)>("block");
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
        addExpressionAnnotation(make_smart<AstExprLabelAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprGotoAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprRef2ValueAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprRef2PtrAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprAddrAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprAssertAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprQuoteAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprStaticAssertAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprDebugAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprInvokeAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprEraseAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprSetInsertAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprFindAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprKeyExistsAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprAscendAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprCastAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprDeleteAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprVarAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprTagAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprSwizzleAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprFieldAnnotation<ExprField>>("ExprField",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprSafeFieldAnnotation>(lib))->from("ExprField");
        addExpressionAnnotation(make_smart<AstExprFieldAnnotation<ExprIsVariant>>("ExprIsVariant",lib))->from("ExprField");
        addExpressionAnnotation(make_smart<AstExprFieldAnnotation<ExprAsVariant>>("ExprAsVariant",lib))->from("ExprField");
        addExpressionAnnotation(make_smart<AstExprSafeAsVariantAnnotation>(lib))->from("ExprField");
        addExpressionAnnotation(make_smart<AstExprOp1Annotation>(lib))->from("ExprOp");
        addExpressionAnnotation(make_smart<AstExprReturnAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprYieldAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExpressionAnnotation<ExprBreak>>("ExprBreak",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExpressionAnnotation<ExprContinue>>("ExprContinue",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprConstAnnotation<ExprConst>>("ExprConst",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprConstAnnotation<ExprFakeContext>>("ExprFakeContext",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprFakeLineInfo,void *>>("ExprFakeLineInfo",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstPtr,void *>>("ExprConstPtr",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt8 ,int8_t>> ("ExprConstInt8",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt16,int16_t>>("ExprConstInt16",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt64,int64_t>>("ExprConstInt64",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt  ,int32_t>>("ExprConstInt",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt2 ,int2>>   ("ExprConstInt2",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt3 ,int3>>   ("ExprConstInt3",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstInt4 ,int4>>   ("ExprConstInt4",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt8 ,uint8_t>> ("ExprConstUInt8",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt16,uint16_t>>("ExprConstUInt16",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt64,uint64_t>>("ExprConstUInt64",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt  ,uint32_t>>("ExprConstUInt",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt2 ,uint2>>   ("ExprConstUInt2",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt3 ,uint3>>   ("ExprConstUInt3",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstUInt4 ,uint4>>   ("ExprConstUInt4",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstRange ,range>> ("ExprConstRange",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstURange,urange>>("ExprConstURange",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstRange64 ,range64>> ("ExprConstRange64",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstURange64,urange64>>("ExprConstURange64",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstFloat  ,float>> ("ExprConstFloat",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstFloat2 ,float2>>("ExprConstFloat2",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstFloat3 ,float3>>("ExprConstFloat3",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstFloat4 ,float4>>("ExprConstFloat4",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstDouble,double>> ("ExprConstDouble",lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstTAnnotation<ExprConstBool,bool>> ("ExprConstBool",lib))->from("ExprConst");
        addAnnotation(make_smart<AstCaptureEntryAnnotation>(lib));
        addExpressionAnnotation(make_smart<AstExprMakeBlockAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprMakeGeneratorAnnotation>(lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprLikeCallAnnotation<ExprMemZero>>("ExprMemZero",lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprConstEnumerationAnnotation>(lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstBitfieldAnnotation>(lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprConstStringAnnotation>(lib))->from("ExprConst");
        addExpressionAnnotation(make_smart<AstExprUnsafeAnnotation>(lib))->from("Expression");
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
