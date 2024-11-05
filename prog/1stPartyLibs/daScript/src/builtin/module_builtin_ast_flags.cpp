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
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprLetFlags";
        ft->argNames = { "inScope", "hasEarlyOut", "itTupleExpansion" };
        return ft;
    }

    TypeDeclPtr makeExprGenFlagsFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprGenFlags";
        ft->argNames = { "alwaysSafe", "generated", "userSaidItsSafe" };
        return ft;
    }

    TypeDeclPtr makeExprFlagsFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprFlags";
        ft->argNames = { "constexpression", "noSideEffects", "noNativeSideEffects", "isForLoopSource", "isCallArgument" };
        return ft;
    }

    TypeDeclPtr makeExprPrintFlagsFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprPrintFlags";
        ft->argNames = { "topLevel", "argLevel", "bottomLevel" };
        return ft;
    }

    TypeDeclPtr makeExprBlockFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprBlockFlags";
        ft->argNames = { "isClosure", "hasReturn", "copyOnReturn", "moveOnReturn",
            "inTheLoop", "finallyBeforeBody", "finallyDisabled","aotSkipMakeBlock",
            "aotDoNotSkipAnnotationData", "isCollapseable", "needCollapse", "hasMakeBlock",
            "hasEarlyOut", "forLoop" };
        return ft;
    }

    TypeDeclPtr makeMakeFieldDeclFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "MakeFieldDeclFlags";
        ft->argNames = { "moveSemantics", "cloneSemantics" };
        return ft;
    }

    TypeDeclPtr makeExprAtFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprAtFlags";
        ft->argNames = { "r2v", "r2cr", "write", "no_promotion" };
        return ft;
    }

    TypeDeclPtr makeExprMakeLocalFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprMakeLocalFlags";
        ft->argNames = { "useStackRef", "useCMRES", "doesNotNeedSp",
            "doesNotNeedInit", "initAllFields", "alwaysAlias" };
        return ft;
    }

    TypeDeclPtr makeExprMakeStructFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprMakeStructFlags";
        ft->argNames = { "useInitializer", "isNewHandle", "usedInitializer", "nativeClassInitializer",
            "isNewClass", "forceClass", "forceStruct", "forceVariant", "forceTuple", "alwaysUseInitializer" };
        return ft;
    }

    TypeDeclPtr makeExprAscendFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprAscendFlags";
        ft->argNames = { "useStackRef", "needTypeInfo", "isMakeLambda" };
        return ft;
    }

    TypeDeclPtr makeExprCastFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprCastFlags";
        ft->argNames = { "upcastCast", "reinterpretCast" };
        return ft;
    }
    TypeDeclPtr makeExprVarFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprVarFlags";
        ft->argNames = { "local", "argument", "_block",
            "thisBlock", "r2v", "r2cr", "write", "under_clone" };
        return ft;
    }

   TypeDeclPtr makeExprFieldDerefFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprFieldDerefFlags";
        ft->argNames = { "unsafeDeref", "ignoreCaptureConst" };
        return ft;
    }

    TypeDeclPtr makeExprFieldFieldFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprFieldFieldFlags";
        ft->argNames = { "r2v", "r2cr", "write", "no_promotion", "under_clone" };
        return ft;
    }

    TypeDeclPtr makeExprSwizzleFieldFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprSwizzleFieldFlags";
        ft->argNames = { "r2v", "r2cr", "write" };
        return ft;
    }

    TypeDeclPtr makeExprYieldFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprYieldFlags";
        ft->argNames = { "moveSemantics", "skipLockCheck" };
        return ft;
    }

    TypeDeclPtr makeExprReturnFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprReturnFlags";
        ft->argNames = { "moveSemantics", "returnReference", "returnInBlock", "takeOverRightStack",
        "returnCallCMRES", "returnCMRES", "fromYield", "fromComprehension", "skipLockCheck" };
        return ft;
    }

    TypeDeclPtr makeExprMakeBlockFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ExprMakeBlockFlags";
        ft->argNames = { "isLambda", "isLocalFunction" };
        return ft;
    }

    TypeDeclPtr makeTypeDeclFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "TypeDeclFlags";
        ft->argNames = { "ref", "constant", "temporary", "_implicit",
            "removeRef", "removeConstant", "removeDim",
            "removeTemporary", "explicitConst", "aotAlias", "smartPtr",
            "smartPtrNative", "isExplicit", "isNativeDim", "isTag", "explicitRef",
            "isPrivateAlias", "autoToAlias" };
        return ft;
    }

    TypeDeclPtr makeFieldDeclarationFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "FieldDeclarationFlags";
        ft->argNames = { "moveSemantics", "parentType", "capturedConstant",
            "generated", "capturedRef", "doNotDelete", "privateField", "_sealed",
            "implemented" };
        return ft;
    }

    TypeDeclPtr makeStructureFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "StructureFlags";
        ft->argNames = { "isClass", "genCtor", "cppLayout", "cppLayoutNotPod",
            "generated", "persistent", "isLambda", "privateStructure",
            "macroInterface", "_sealed", "skipLockCheck", "circular",
            "_generator", "hasStaticMembers", "hasStaticFunctions",
            "hasInitFields", "safeWhenUninitialized" };
        return ft;
    }

    TypeDeclPtr makeFunctionFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "FunctionFlags";
        ft->argNames = {
            "builtIn", "policyBased", "callBased", "interopFn", "hasReturn", "copyOnReturn", "moveOnReturn", "exports",
            "init", "addr", "used", "fastCall", "knownSideEffects", "hasToRunAtCompileTime", "unsafeOperation", "unsafeDeref",
            "hasMakeBlock", "aotNeedPrologue", "noAot", "aotHybrid", "aotTemplate", "generated", "privateFunction", "_generator",
            "_lambda", "firstArgReturnType", "noPointerCast", "isClassMethod", "isTypeConstructor", "shutdown", "anyTemplate", "macroInit"
        };
        return ft;
    }

    TypeDeclPtr makeMoreFunctionFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "MoreFunctionFlags";
        ft->argNames = {
            "macroFunction", "needStringCast", "aotHashDeppendsOnArguments", "lateInit", "requestJit",
            "unsafeOutsideOfFor", "skipLockCheck", "safeImplicit", "deprecated", "aliasCMRES", "neverAliasCMRES",
            "addressTaken", "propertyFunction", "pinvoke", "jitOnly", "isStaticClassMethod", "requestNoJit",
            "jitContextAndLineInfo", "nodiscard", "captureString", "callCaptureString", "hasStringBuilder"
        };
        return ft;
    }

    TypeDeclPtr makeFunctionSideEffectFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "FunctionSideEffectFlags";
        ft->argNames = { "_unsafe", "userScenario","modifyExternal",
            "modifyArgument","accessGlobal","invoke"
        };
        return ft;
    }

    TypeDeclPtr makeVariableFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "VariableFlags";
        ft->argNames = { "init_via_move", "init_via_clone", "used", "aliasCMRES",
            "marked_used", "global_shared", "do_not_delete", "generated", "capture_as_ref",
            "can_shadow", "private_variable", "tag", "global", "inScope", "no_capture", "early_out",
            "used_in_finally", "static_class_member" };
        return ft;
    }

    TypeDeclPtr makeVariableAccessFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "VariableAccessFlags";
        ft->argNames = { "access_extern", "access_get", "access_ref",
            "access_init", "access_pass" };
        return ft;
    }

    TypeDeclPtr makeExprCopyFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "CopyFlags";
        ft->argNames = { "allowCopyTemp", "takeOverRightStack", "promoteToClone" };
        return ft;
    }

    TypeDeclPtr makeExprMoveFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "MoveFlags";
        ft->argNames = { "skipLockCheck", "takeOverRightStack" };
        return ft;
    }

    TypeDeclPtr makeExprIfFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "IfFlags";
        ft->argNames = { "isStatic", "doNotFold" };
        return ft;
    }

    TypeDeclPtr makeExprStringBuilderFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
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
        addAlias(makeFunctionSideEffectFlags());
        addAlias(makeVariableFlags());
        addAlias(makeVariableAccessFlags());
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
        addAlias(makeExprIfFlags());
    }
}
