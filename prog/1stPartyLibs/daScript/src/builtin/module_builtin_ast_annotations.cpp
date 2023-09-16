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
    class VisitorAdapter;
};

IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeDecl,TypeDecl)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FieldDeclaration, Structure::FieldDeclaration)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Structure,Structure)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumEntry,Enumeration::EnumEntry)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Enumeration,Enumeration)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Expression,Expression)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Function,Function)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(InferHistory, InferHistory)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Variable,Variable)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(VisitorAdapter,VisitorAdapter)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FunctionAnnotation,FunctionAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(StructureAnnotation,StructureAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumerationAnnotation,EnumerationAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(PassMacro,PassMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(VariantMacro,VariantMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ForLoopMacro,ForLoopMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(SimulateMacro,SimulateMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CaptureMacro,CaptureMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ReaderMacro,ReaderMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CommentReader,CommentReader)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CallMacro,CallMacro)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ModuleGroup,ModuleGroup)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ModuleLibrary,ModuleLibrary)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AstContext,AstContext)

IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprBlock,ExprBlock)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprLet,ExprLet)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprStringBuilder,ExprStringBuilder)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprNew,ExprNew)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprNamedCall,ExprNamedCall)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(MakeFieldDecl,MakeFieldDecl)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(MakeStruct,MakeStruct)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprCall,ExprCall)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprCallFunc,ExprCallFunc)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprLooksLikeCall,ExprLooksLikeCall)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprNullCoalescing,ExprNullCoalescing)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAt,ExprAt)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprSafeAt,ExprSafeAt)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprPtr2Ref,ExprPtr2Ref)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprIs,ExprIs)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprOp,ExprOp)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprOp2,ExprOp2)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprOp3,ExprOp3)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprCopy,ExprCopy)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMove,ExprMove)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprClone,ExprClone)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprWith,ExprWith)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAssume,ExprAssume)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprWhile,ExprWhile)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprTryCatch,ExprTryCatch)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprIfThenElse,ExprIfThenElse)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprFor,ExprFor)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeLocal,ExprMakeLocal)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeVariant,ExprMakeVariant)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeStruct,ExprMakeStruct)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeArray,ExprMakeArray)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeTuple,ExprMakeTuple)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprArrayComprehension,ExprArrayComprehension)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeInfoMacro,TypeInfoMacro);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprTypeInfo,ExprTypeInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprTypeDecl,ExprTypeDecl)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprLabel,ExprLabel);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprGoto,ExprGoto);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprRef2Value,ExprRef2Value);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprRef2Ptr,ExprRef2Ptr);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAddr,ExprAddr);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAssert,ExprAssert);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprStaticAssert,ExprStaticAssert);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprQuote,ExprQuote);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprDebug,ExprDebug);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprInvoke,ExprInvoke);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprErase,ExprErase);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprSetInsert,ExprSetInsert);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprFind,ExprFind);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprKeyExists,ExprKeyExists);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAscend,ExprAscend);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprCast,ExprCast);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprDelete,ExprDelete);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprVar,ExprVar);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprTag,ExprTag);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprSwizzle,ExprSwizzle);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprField,ExprField);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprSafeField,ExprSafeField);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprIsVariant,ExprIsVariant);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprAsVariant,ExprAsVariant);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprSafeAsVariant,ExprSafeAsVariant);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprOp1,ExprOp1);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprReturn,ExprReturn);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprYield,ExprYield);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprBreak,ExprBreak);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprContinue,ExprContinue);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConst,ExprConst);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprFakeContext,ExprFakeContext);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprFakeLineInfo,ExprFakeLineInfo);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstPtr,ExprConstPtr);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstEnumeration,ExprConstEnumeration);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstBitfield,ExprConstBitfield);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt8,ExprConstInt8);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt16,ExprConstInt16);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt64,ExprConstInt64);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt,ExprConstInt);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt2,ExprConstInt2);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt3,ExprConstInt3);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstInt4,ExprConstInt4);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt8,ExprConstUInt8);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt16,ExprConstUInt16);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt64,ExprConstUInt64);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt,ExprConstUInt);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt2,ExprConstUInt2);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt3,ExprConstUInt3);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstUInt4,ExprConstUInt4);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstRange,ExprConstRange);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstURange,ExprConstURange);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstRange64,ExprConstRange64);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstURange64,ExprConstURange64);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstBool,ExprConstBool);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstFloat,ExprConstFloat);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstFloat2,ExprConstFloat2);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstFloat3,ExprConstFloat3);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstFloat4,ExprConstFloat4);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstString,ExprConstString);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprConstDouble,ExprConstDouble);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CaptureEntry,CaptureEntry);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeBlock,ExprMakeBlock);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMakeGenerator,ExprMakeGenerator);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprMemZero,ExprMemZero);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprReader,ExprReader);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprCallMacro,ExprCallMacro);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ExprUnsafe,ExprUnsafe);

namespace das {
    void getAstContext ( smart_ptr_raw<Program> prog, smart_ptr_raw<Expression> expr, const TBlock<void,bool,AstContext> & block, Context * context, LineInfoArg * at ) {
        AstContext astc = generateAstContext(prog,expr.get());
        vec4f args[2];
        args[0] = cast<bool>::from ( astc.valid );
        args[1] = astc.valid ? cast<AstContext&>::from(astc) : v_zero();
        context->invoke(block, args, nullptr, at);
    }

    void init_expr ( BasicStructureAnnotation & ann ) {
        ann.addFieldEx("at", "at", offsetof(Expression, at), makeType<LineInfo>(*ann.mlib));
        ann.addFieldEx("_type", "type", offsetof(Expression, type), makeType<TypeDeclPtr>(*ann.mlib));
        ann.addFieldEx("__rtti", "__rtti", offsetof(Expression, __rtti), makeType<const char *>(*ann.mlib));
        ann.addFieldEx("genFlags", "genFlags", offsetof(Expression, genFlags), makeExprGenFlagsFlags());
        ann.addFieldEx("flags", "flags", offsetof(Expression, flags), makeExprFlagsFlags());
        ann.addFieldEx("printFlags", "printFlags", offsetof(Expression, printFlags), makeExprPrintFlagsFlags());
    }

    void init_expr_looks_like_call ( BasicStructureAnnotation & ann ) {
        ann.addFieldEx("name", "name", offsetof(ExprLooksLikeCall, name), makeType<string>(*ann.mlib));
        ann.addFieldEx("arguments", "arguments", offsetof(ExprLooksLikeCall, arguments), makeType<vector<ExpressionPtr>>(*ann.mlib));
        ann.addFieldEx("argumentsFailedToInfer", "argumentsFailedToInfer", offsetof(ExprLooksLikeCall, argumentsFailedToInfer), makeType<bool>(*ann.mlib));
        ann.addFieldEx("atEnclosure", "atEnclosure", offsetof(ExprLooksLikeCall, atEnclosure), makeType<LineInfo>(*ann.mlib));
    }

    bool canSubstituteExpr ( const TypeAnnotation* thisAnn, TypeAnnotation* ann ) {
        if (ann == thisAnn) return true;
        if (thisAnn->module != ann->module) return false;
        if (memcmp(ann->name.c_str(), "Expr", 4) != 0) return false;
        auto* AEA = static_cast<BasicStructureAnnotation*>(ann);
        for (auto p : AEA->parents) {
            if (p == thisAnn) return true;
        }
        return false;
    }

    struct AstExprReaderAnnotation : AstExpressionAnnotation<ExprReader> {
        AstExprReaderAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprReader> ("ExprReader", ml) {
            addField<DAS_BIND_MANAGED_FIELD(macro)>("macro");
            addField<DAS_BIND_MANAGED_FIELD(sequence)>("sequence");
        }
    };

    struct AstExprCallMacroAnnotation : AstExprLooksLikeCallAnnotation<ExprCallMacro> {
        AstExprCallMacroAnnotation(ModuleLibrary & ml)
            :  AstExprLooksLikeCallAnnotation<ExprCallMacro> ("ExprCallMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(macro)>("macro");
        }
    };

    struct SimNode_AstGetTypeDecl : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_AstGetTypeDecl ( const LineInfo & at, const TypeDeclPtr & d, char * dE )
            : SimNode_CallBase(at) {
            typeExpr = d.get();
            descr = dE;
        }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override {
            auto that = (SimNode_AstGetTypeDecl *) SimNode::copyNode(context, code);
            that->descr = code->allocateName(descr);
            return that;
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(AstGetTypeDecl);
            V_ARG(descr);
            V_END();
        }
        __forceinline char * compute(Context &) {
            DAS_PROFILE_NODE
            typeExpr->addRef();
            return (char *) typeExpr;
        }
        TypeDecl *  typeExpr;   // requires RTTI
        char *      descr;
    };

    struct AstTypeDeclMacro : TypeInfoMacro {
        AstTypeDeclMacro() : TypeInfoMacro("ast_typedecl") {}
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, const ExpressionPtr &, string & ) override {
            return typeFactory<smart_ptr<TypeDecl>>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, const ExpressionPtr & expr, string & ) override {
            auto exprTypeInfo = static_pointer_cast<ExprTypeInfo>(expr);
            char * descr = context->code->allocateName(exprTypeInfo->typeexpr->getMangledName());
            return context->code->makeNode<SimNode_AstGetTypeDecl>(expr->at, exprTypeInfo->typeexpr, descr);
        }
        virtual bool noAot ( const ExpressionPtr & ) const override {
            return true;
        }
    };

    struct SimNode_AstGetFunction : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_AstGetFunction ( const LineInfo & at, Function * f, char * dE )
            : SimNode_CallBase(at) {
            func = f;
            descr = dE;
        }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override {
            auto that = (SimNode_AstGetFunction *) SimNode::copyNode(context, code);
            that->descr = code->allocateName(descr);
            return that;
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(AstGetTypeDecl);
            V_ARG(descr);
            V_END();
        }
        __forceinline char * compute(Context &) {
            DAS_PROFILE_NODE
            return (char *) func;
        }
        Function *  func;   // requires RTTI
        char *      descr;
    };


    struct AstFunctionMacro : TypeInfoMacro {
        AstFunctionMacro() : TypeInfoMacro("ast_function") {}
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, const ExpressionPtr &, string & ) override {
            return typeFactory<smart_ptr<Function>>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, const ExpressionPtr & expr, string & errors ) override {
            auto exprTypeInfo = static_pointer_cast<ExprTypeInfo>(expr);
            if ( exprTypeInfo->subexpr && exprTypeInfo->subexpr->rtti_isAddr() ) {
                auto exprAddr = static_pointer_cast<ExprAddr>(exprTypeInfo->subexpr);
                char * descr = context->code->allocateName(exprAddr->func->getMangledName());
                return context->code->makeNode<SimNode_AstGetFunction>(expr->at, exprAddr->func, descr);
            } else {
                errors = "ast_expression requires @@func expression";
                return nullptr;
            }
        }
        virtual bool noAot ( const ExpressionPtr & ) const override {
            return true;
        }
    };

    void Module_Ast::registerAnnotations(ModuleLibrary &) {
        // ENUMS
        addEnumeration(make_smart<EnumerationSideEffects>());
        addEnumeration(make_smart<EnumerationCaptureMode>());
        // THE MAGNIFICENT TWO
        addTypeInfoMacro(make_smart<AstTypeDeclMacro>());
        addTypeInfoMacro(make_smart<AstFunctionMacro>());
    }

    void Module_Ast::registerMacroExpressions(ModuleLibrary & lib){
        addExpressionAnnotation(make_smart<AstExprReaderAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprCallMacroAnnotation>(lib))->from("ExprLooksLikeCall");
    }
}
