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

    TypeDeclPtr makeExprLetFlagsFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprLetFlags";
        ft->argNames = { "inScope", "hasEarlyOut", "itTupleExpansion" };
        return ft;
    }

    TypeDeclPtr makeExprGenFlagsFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprGenFlags";
        ft->argNames = { "alwaysSafe", "generated", "userSaidItsSafe" };
        return ft;
    }

    TypeDeclPtr makeExprFlagsFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprFlags";
        ft->argNames = { "constexpression", "noSideEffects", "noNativeSideEffects", "isForLoopSource", "isCallArgument" };
        return ft;
    }

    TypeDeclPtr makeExprPrintFlagsFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprPrintFlags";
        ft->argNames = { "topLevel", "argLevel", "bottomLevel" };
        return ft;
    }

    TypeDeclPtr makeExprBlockFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprBlockFlags";
        ft->argNames = { "isClosure", "hasReturn", "copyOnReturn", "moveOnReturn",
            "inTheLoop", "finallyBeforeBody", "finallyDisabled","aotSkipMakeBlock",
            "aotDoNotSkipAnnotationData", "isCollapseable", "needCollapse", "hasMakeBlock",
            "hasEarlyOut", "forLoop", "hasExitByLabel", "isLambdaBlock", "isGeneratorBlock" };
        return ft;
    }

    TypeDeclPtr makeMakeFieldDeclFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "MakeFieldDeclFlags";
        ft->argNames = { "moveSemantics", "cloneSemantics" };
        return ft;
    }

    TypeDeclPtr makeExprAtFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprAtFlags";
        ft->argNames = { "r2v", "r2cr", "write", "no_promotion", "under_clone", "under_deref", "no_bound_check" };
        return ft;
    }

    TypeDeclPtr makeExprMakeLocalFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprMakeLocalFlags";
        ft->argNames = { "useStackRef", "useCMRES", "doesNotNeedSp",
            "doesNotNeedInit", "initAllFields", "alwaysAlias" };
        return ft;
    }

    TypeDeclPtr makeExprMakeStructFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprMakeStructFlags";
        ft->argNames = { "useInitializer", "isNewHandle", "usedInitializer", "nativeClassInitializer",
            "isNewClass", "forceClass", "forceStruct", "forceVariant", "forceTuple", "alwaysUseInitializer",
            "ignoreVisCheck", "canShadowBlock" };
        return ft;
    }

    TypeDeclPtr makeExprAscendFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprAscendFlags";
        ft->argNames = { "useStackRef", "needTypeInfo", "allocate_on_stack" };
        return ft;
    }

    TypeDeclPtr makeExprCastFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprCastFlags";
        ft->argNames = { "upcastCast", "reinterpretCast", "fromAddrSugar" };
        return ft;
    }
    TypeDeclPtr makeExprVarFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprVarFlags";
        ft->argNames = { "local", "argument", "_block",
            "thisBlock", "r2v", "r2cr", "write", "under_clone" };
        return ft;
    }

   TypeDeclPtr makeExprFieldDerefFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprFieldDerefFlags";
        ft->argNames = { "unsafeDeref", "ignoreCaptureConst" };
        return ft;
    }

    TypeDeclPtr makeExprFieldFieldFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprFieldFieldFlags";
        ft->argNames = { "r2v", "r2cr", "write", "no_promotion", "under_clone" };
        return ft;
    }

    TypeDeclPtr makeExprSwizzleFieldFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprSwizzleFieldFlags";
        ft->argNames = { "r2v", "r2cr", "write", "no_promotion" };
        return ft;
    }

    TypeDeclPtr makeExprYieldFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprYieldFlags";
        ft->argNames = { "moveSemantics" };
        return ft;
    }

    TypeDeclPtr makeExprReturnFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprReturnFlags";
        ft->argNames = { "moveSemantics", "returnReference", "returnInBlock", "takeOverRightStack",
        "returnCallCMRES", "returnCMRES", "fromYield", "fromComprehension" };
        return ft;
    }

    TypeDeclPtr makeExprMakeBlockFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "ExprMakeBlockFlags";
        ft->argNames = { "isLambda", "isLocalFunction" };
        return ft;
    }

    TypeDeclPtr makeTypeDeclFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "TypeDeclFlags";
        ft->argNames = { "ref", "constant", "temporary", "_implicit",
            "removeRef", "removeConstant", "removeDim",
            "removeTemporary", "explicitConst", "aotAlias", "smartPtr",
            "smartPtrNative", "isExplicit", "isNativeDim", "isTag", "explicitRef",
            "isPrivateAlias", "autoToAlias", "safeWhenUninitialized",
            "enumStubBinding", "enumStubIsUnsigned" };
        return ft;
    }

    TypeDeclPtr makeFieldDeclarationFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "FieldDeclarationFlags";
        ft->argNames = { "moveSemantics", "parentType", "capturedConstant",
            "generated", "capturedRef", "doNotDelete", "privateField", "_sealed",
            "implemented", "classMethod", "_abstract", "inherited" };
        return ft;
    }

    TypeDeclPtr makeStructureFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "StructureFlags";
        ft->argNames = { "isClass", "genCtor", "cppLayout", "cppLayoutNotPod",
            "generated", "persistent", "isLambda", "privateStructure",
            "macroInterface", "_sealed", "circular",
            "_generator", "hasStaticMembers", "hasStaticFunctions",
            "hasInitFields", "safeWhenUninitialized", "isTemplate",
            "hasDefaultInitializer", "noGenCtor" };
        return ft;
    }

    TypeDeclPtr makeFunctionFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "FunctionFlags";
        ft->argNames = {
            "builtIn", "policyBased", "callBased", "interopFn", "hasReturn", "copyOnReturn", "moveOnReturn", "exports",
            "init", "addr", "fastCall", "knownSideEffects", "hasToRunAtCompileTime", "unsafeOperation", "unsafeDeref",
            "hasMakeBlock", "aotNeedPrologue", "noAot", "aotHybrid", "aotTemplate", "generated", "privateFunction", "_generator",
            "_lambda", "firstArgReturnType", "noPointerCast", "isClassMethod", "isTypeConstructor", "shutdown", "anyTemplate", "macroInit"
        };
        return ft;
    }

    TypeDeclPtr makeMoreFunctionFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "MoreFunctionFlags";
        ft->argNames = {
            "macroFunction", "aotHashDeppendsOnArguments", "lateInit", "requestJit",
            "unsafeOutsideOfFor", "mustInline", "safeImplicit", "deprecated", "aliasCMRES", "neverAliasCMRES",
            "addressTaken", "propertyFunction", "pinvoke", "jitOnly", "isStaticClassMethod", "requestNoJit",
            "jitContextAndLineInfo", "nodiscard", "captureString", "callCaptureString", "hasStringBuilder",
            "recursive", "isTemplate", "unsafeWhenNotCloneArray", "stub", "lateShutdown", "hasTryRecover",
            "hasUnsafe", "isConstClassMethod", "isCustomProperty", "neverInline"
        };
        return ft;
    }

    TypeDeclPtr makeMoreFunctionFlags2() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "MoreFunctionFlags2";
        ft->argNames = {
            "localFunction", "tempStringResult", "mayQueueTempString", "needCallerStackFrame",
            "nttp"
        };
        return ft;
    }

    TypeDeclPtr makeFunctionSideEffectFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "FunctionSideEffectFlags";
        ft->argNames = { "_unsafe", "userScenario","modifyExternal",
            "modifyArgument","accessGlobal","invoke"
        };
        return ft;
    }

    TypeDeclPtr makeVariableFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "VariableFlags";
        ft->argNames = { "init_via_move", "init_via_clone", "aliasCMRES",
            "marked_used", "global_shared", "do_not_delete", "generated", "capture_as_ref",
            "can_shadow", "private_variable", "tag", "global", "inScope", "no_capture", "early_out",
            "used_in_finally", "static_class_member", "bitfield_constant", "pod_delete",
            "pod_delete_gen", "single_return_via_move" };
        return ft;
    }

    TypeDeclPtr makeVariableAccessFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "VariableAccessFlags";
        ft->argNames = { "access_extern", "access_get", "access_ref",
            "access_init", "access_pass", "access_fold" };
        return ft;
    }

    TypeDeclPtr makeVariableAccessInfoFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "VariableAccessInfoFlags";
        ft->argNames = { "access_info_pass_mutable" };
        return ft;
    }

    TypeDeclPtr makeExprCopyFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "CopyFlags";
        ft->argNames = { "allowCopyTemp", "takeOverRightStack", "allowConstantLValue", "no_promotion" };
        return ft;
    }

    TypeDeclPtr makeExprMoveFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "MoveFlags";
        ft->argNames = { "takeOverRightStack", "allowConstantLValue", "podDelete", "no_promotion" };
        return ft;
    }

    TypeDeclPtr makeExprCloneFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "CloneFlags";
        ft->argNames = { "no_promotion" };
        return ft;
    }

    TypeDeclPtr makeExprIfFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "IfFlags";
        ft->argNames = { "isStatic", "doNotFold" };
        return ft;
    }

    TypeDeclPtr makeExprStringBuilderFlags() {
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
        ft->alias = "StringBuilderFlags";
        ft->argNames = { "isTempString" };
        return ft;
    }

    void Module_Ast::registerFlags(ModuleLibrary & ) {
        // FLAGS?
        addAlias(makeTypeDeclFlags());
        addAlias(makeFieldDeclarationFlags());
        addAlias(makeStructureFlags());
        addAlias(makeExprGenFlagsFlags());
        addAlias(makeExprLetFlagsFlags());
        addAlias(makeExprFlagsFlags());
        addAlias(makeExprPrintFlagsFlags());
        addAlias(makeFunctionFlags());
        addAlias(makeMoreFunctionFlags());
        addAlias(makeMoreFunctionFlags2());
        addAlias(makeFunctionSideEffectFlags());
        addAlias(makeVariableFlags());
        addAlias(makeVariableAccessFlags());
        addAlias(makeVariableAccessInfoFlags());
        addAlias(makeExprBlockFlags());
        addAlias(makeExprAtFlags());
        addAlias(makeExprMakeLocalFlags());
        addAlias(makeExprAscendFlags());
        addAlias(makeExprCastFlags());
        addAlias(makeExprVarFlags());
        addAlias(makeExprMakeStructFlags());
        addAlias(makeMakeFieldDeclFlags());
        addAlias(makeExprFieldDerefFlags());
        addAlias(makeExprFieldFieldFlags());
        addAlias(makeExprSwizzleFieldFlags());
        addAlias(makeExprYieldFlags());
        addAlias(makeExprReturnFlags());
        addAlias(makeExprMakeBlockFlags());
        addAlias(makeExprCopyFlags());
        addAlias(makeExprMoveFlags());
        addAlias(makeExprCloneFlags());
        addAlias(makeExprIfFlags());
        addAlias(makeExprStringBuilderFlags());
    }
}
