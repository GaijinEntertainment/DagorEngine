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
    struct AstExprStringBuilderAnnotation : AstExpressionAnnotation<ExprStringBuilder> {
        AstExprStringBuilderAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprStringBuilder> ("ExprStringBuilder", ml) {
            addField<DAS_BIND_MANAGED_FIELD(elements)>("elements");
            addFieldEx ( "stringBuilderFlags", "stringBuilderFlags", offsetof(ExprStringBuilder, stringBuilderFlags), makeExprStringBuilderFlags() );
        }
    };

    struct AstMakeFieldDeclAnnotation : ManagedStructureAnnotation<MakeFieldDecl> {
        AstMakeFieldDeclAnnotation(ModuleLibrary & ml)
            :  ManagedStructureAnnotation<MakeFieldDecl> ("MakeFieldDecl", ml) {
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            addField<DAS_BIND_MANAGED_FIELD(tag)>("tag");
            addFieldEx ( "flags", "flags", offsetof(MakeFieldDecl, flags), makeMakeFieldDeclFlags() );
        }
    };

    struct AstMakeStructAnnotation : ManagedVectorAnnotation<MakeStruct> {
        AstMakeStructAnnotation(ModuleLibrary & ml)
            :  ManagedVectorAnnotation<MakeStruct> ("MakeStruct", ml) {
        };
        virtual bool isSmart() const override { return true; }
        virtual bool canNew() const override { return true; }
        virtual bool canDeletePtr() const override { return true; }
        virtual string getSmartAnnotationCloneFunction () const override { return "smart_ptr_clone"; }
        virtual SimNode * simulateGetNew ( Context & context, const LineInfo & at ) const override {
            return context.code->makeNode<SimNode_NewHandle<MakeStruct,true>>(at);
        }
        virtual SimNode * simulateDeletePtr ( Context & context, const LineInfo & at, SimNode * sube, uint32_t count ) const override {
            return context.code->makeNode<SimNode_DeleteHandlePtr<MakeStruct,true>>(at,sube,count);
        }
        static void * jit_new ( Context * ) {
            auto res = new MakeStruct();
            res->addRef();
            return res;
        }
        static void jit_delete ( void * ptr, Context * ) {
            if ( ptr ) {
                auto res = (MakeStruct *) ptr;
                res->delRef();
            }
        }
        virtual void * jitGetNew() const override { return (void *) &jit_new; }
        virtual void * jitGetDelete() const override { return (void *) &jit_delete; }
    };

    struct AstExprNamedCallAnnotation : AstExpressionAnnotation<ExprNamedCall> {
        AstExprNamedCallAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprNamedCall> ("ExprNamedCall", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(nonNamedArguments)>("nonNamedArguments");
            addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
            addField<DAS_BIND_MANAGED_FIELD(argumentsFailedToInfer)>("argumentsFailedToInfer");
        }
    };

    struct AstExprNewAnnotation : AstExprCallFuncAnnotation<ExprNew> {
        AstExprNewAnnotation(ModuleLibrary & ml)
            :  AstExprCallFuncAnnotation<ExprNew> ("ExprNew", ml) {
            addField<DAS_BIND_MANAGED_FIELD(typeexpr)>("typeexpr");
            addField<DAS_BIND_MANAGED_FIELD(initializer)>("initializer");
        }
    };

    struct AstExprCallAnnotation : AstExprCallFuncAnnotation<ExprCall> {
        AstExprCallAnnotation(ModuleLibrary & ml)
            :  AstExprCallFuncAnnotation<ExprCall> ("ExprCall", ml) {
            addField<DAS_BIND_MANAGED_FIELD(doesNotNeedSp)>("doesNotNeedSp");
            addField<DAS_BIND_MANAGED_FIELD(cmresAlias)>("cmresAlias");
            addField<DAS_BIND_MANAGED_FIELD(notDiscarded)>("notDiscarded");
        }
    };

    struct AstExprNullCoalescingAnnotation : AstExprPtr2RefAnnotation<ExprNullCoalescing> {
        AstExprNullCoalescingAnnotation(ModuleLibrary & ml)
            :  AstExprPtr2RefAnnotation<ExprNullCoalescing> ("ExprNullCoalescing", ml) {
            addField<DAS_BIND_MANAGED_FIELD(defaultValue)>("defaultValue");
        }
    };

    struct AstExprIsAnnotation : AstExpressionAnnotation<ExprIs> {
        AstExprIsAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprIs> ("ExprIs", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(typeexpr)>("typeexpr");
        }
    };

    struct AstExprOp3Annotation : AstExprOpAnnotation<ExprOp3> {
        AstExprOp3Annotation(ModuleLibrary & ml)
            :  AstExprOpAnnotation<ExprOp3> ("ExprOp3", ml) {
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(left)>("left");
            addField<DAS_BIND_MANAGED_FIELD(right)>("right");
        }
    };

    struct AstExprCopyAnnotation : AstExprOp2Annotation<ExprCopy> {
        AstExprCopyAnnotation(ModuleLibrary & ml)
            :  AstExprOp2Annotation<ExprCopy> ("ExprCopy", ml) {
            addFieldEx ( "copy_flags", "copyFlags", offsetof(ExprCopy, copyFlags), makeExprCopyFlags() );
        }
    };

    struct AstExprMoveAnnotation : AstExprOp2Annotation<ExprMove> {
        AstExprMoveAnnotation(ModuleLibrary & ml)
            :  AstExprOp2Annotation<ExprMove> ("ExprMove", ml) {
            addFieldEx ( "move_flags", "moveFlags", offsetof(ExprMove, moveFlags), makeExprMoveFlags() );
        }
    };

    struct AstExprWithAnnotation : AstExpressionAnnotation<ExprWith> {
        AstExprWithAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprWith> ("ExprWith", ml) {
            addField<DAS_BIND_MANAGED_FIELD(with)>("_with", "with");
            addField<DAS_BIND_MANAGED_FIELD(body)>("body");
        }
    };

    struct AstExprAssumeAnnotation : AstExpressionAnnotation<ExprAssume> {
        AstExprAssumeAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprAssume> ("ExprAssume", ml) {
            addField<DAS_BIND_MANAGED_FIELD(alias)>("alias", "alias");
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
        }
    };

    struct AstExprWhileAnnotation : AstExpressionAnnotation<ExprWhile> {
        AstExprWhileAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprWhile> ("ExprWhile", ml) {
            addField<DAS_BIND_MANAGED_FIELD(cond)>("cond");
            addField<DAS_BIND_MANAGED_FIELD(body)>("body");
        }
    };

    struct AstExprTryCatchAnnotation : AstExpressionAnnotation<ExprTryCatch> {
        AstExprTryCatchAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprTryCatch> ("ExprTryCatch", ml) {
            addField<DAS_BIND_MANAGED_FIELD(try_block)>("try_block");
            addField<DAS_BIND_MANAGED_FIELD(catch_block)>("catch_block");
        }
    };

    struct AstExprIfThenElseAnnotation : AstExpressionAnnotation<ExprIfThenElse> {
        AstExprIfThenElseAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprIfThenElse> ("ExprIfThenElse", ml) {
            addField<DAS_BIND_MANAGED_FIELD(cond)>("cond");
            addField<DAS_BIND_MANAGED_FIELD(if_true)>("if_true");
            addField<DAS_BIND_MANAGED_FIELD(if_false)>("if_false");
            addFieldEx ( "if_flags", "ifFlags", offsetof(ExprIfThenElse, ifFlags), makeExprIfFlags() );
        }
    };

    struct AstExprForAnnotation : AstExpressionAnnotation<ExprFor> {
        AstExprForAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprFor> ("ExprFor", ml) {
            addField<DAS_BIND_MANAGED_FIELD(iterators)>("iterators");
            addField<DAS_BIND_MANAGED_FIELD(iteratorsAka)>("iteratorsAka");
            addField<DAS_BIND_MANAGED_FIELD(iteratorsAt)>("iteratorsAt");
            addField<DAS_BIND_MANAGED_FIELD(iteratorVariables)>("iteratorVariables");
            addField<DAS_BIND_MANAGED_FIELD(iteratorsTags)>("iteratorsTags");
            addField<DAS_BIND_MANAGED_FIELD(sources)>("sources");
            addField<DAS_BIND_MANAGED_FIELD(body)>("body");
            addField<DAS_BIND_MANAGED_FIELD(visibility)>("visibility");
            addField<DAS_BIND_MANAGED_FIELD(allowIteratorOptimization)>("allowIteratorOptimization");
            addField<DAS_BIND_MANAGED_FIELD(canShadow)>("canShadow");
        }
    };

    struct AstExprMakeStructAnnotation : AstExprMakeLocalAnnotation<ExprMakeStruct> {
        AstExprMakeStructAnnotation(ModuleLibrary & ml)
            :  AstExprMakeLocalAnnotation<ExprMakeStruct> ("ExprMakeStruct", ml) {
            addField<DAS_BIND_MANAGED_FIELD(structs)>("structs");
            addField<DAS_BIND_MANAGED_FIELD(block)>("_block","block");
            addField<DAS_BIND_MANAGED_FIELD(constructor)>("constructor","constructor");
            this->addFieldEx ( "makeStructFlags", "makeStructFlags", offsetof(ExprMakeStruct, makeStructFlags), makeExprMakeStructFlags() );
        }
    };

    struct AstExprMakeVariantAnnotation : AstExprMakeLocalAnnotation<ExprMakeVariant> {
        AstExprMakeVariantAnnotation(ModuleLibrary & ml)
            :  AstExprMakeLocalAnnotation<ExprMakeVariant> ("ExprMakeVariant", ml) {
            addField<DAS_BIND_MANAGED_FIELD(variants)>("variants");
        }
    };

    struct AstExprMakeTupleAnnotation : AstExprMakeArrayAnnotation<ExprMakeTuple> {
        AstExprMakeTupleAnnotation(ModuleLibrary & ml)
            :  AstExprMakeArrayAnnotation<ExprMakeTuple> ("ExprMakeTuple", ml) {
            addField<DAS_BIND_MANAGED_FIELD(isKeyValue)>("isKeyValue");
        }
    };

    struct AstExprArrayComprehensionAnnotation : AstExpressionAnnotation<ExprArrayComprehension> {
        AstExprArrayComprehensionAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprArrayComprehension> ("ExprArrayComprehension", ml) {
            addField<DAS_BIND_MANAGED_FIELD(exprFor)>("exprFor");
            addField<DAS_BIND_MANAGED_FIELD(exprWhere)>("exprWhere");
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(generatorSyntax)>("generatorSyntax");
            addField<DAS_BIND_MANAGED_FIELD(tableSyntax)>("tableSyntax");
        }
    };

    struct AstTypeInfoMacroAnnotation : ManagedStructureAnnotation<TypeInfoMacro,false,true> {
        AstTypeInfoMacroAnnotation(ModuleLibrary & ml)
            :  ManagedStructureAnnotation<TypeInfoMacro,false,true> ("TypeInfoMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
        }
    };

    struct AstExprTypeInfoAnnotation : AstExpressionAnnotation<ExprTypeInfo> {
        AstExprTypeInfoAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprTypeInfo> ("ExprTypeInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(trait)>("trait");
            addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            addField<DAS_BIND_MANAGED_FIELD(typeexpr)>("typeexpr");
            addField<DAS_BIND_MANAGED_FIELD(subtrait)>("subtrait");
            addField<DAS_BIND_MANAGED_FIELD(extratrait)>("extratrait");
            addField<DAS_BIND_MANAGED_FIELD(macro)>("macro");
        }
    };

    struct AstExprTypeDeclAnnotation : AstExpressionAnnotation<ExprTypeDecl> {
        AstExprTypeDeclAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprTypeDecl> ("ExprTypeDecl", ml) {
            addField<DAS_BIND_MANAGED_FIELD(typeexpr)>("typeexpr");
        }
    };

    void Module_Ast::registerAnnotations2(ModuleLibrary & lib) {
        // continued expressions (in order of inheritance)
        addExpressionAnnotation(make_smart<AstExprStringBuilderAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstMakeFieldDeclAnnotation>(lib));
        addExpressionAnnotation(make_smart<AstMakeStructAnnotation>(lib));
        addExpressionAnnotation(make_smart<AstExprNamedCallAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprLooksLikeCallAnnotation<ExprLooksLikeCall>>("ExprLooksLikeCall",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprCallFuncAnnotation<ExprCallFunc>>("ExprCallFunc",lib))->from("ExprLooksLikeCall");
        addExpressionAnnotation(make_smart<AstExprNewAnnotation>(lib))->from("ExprCallFunc");
        addExpressionAnnotation(make_smart<AstExprCallAnnotation>(lib))->from("ExprCallFunc");
        addExpressionAnnotation(make_smart<AstExprPtr2RefAnnotation<ExprPtr2Ref>>("ExprPtr2Ref",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprNullCoalescingAnnotation>(lib))->from("ExprPtr2Ref");
        addExpressionAnnotation(make_smart<AstExprAtAnnotation<ExprAt>>("ExprAt",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprAtAnnotation<ExprSafeAt>>("ExprSafeAt",lib))->from("ExprAt");
        addExpressionAnnotation(make_smart<AstExprIsAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprAnnotation<ExprOp>>("ExprOp",lib))->from("ExprCallFunc");
        addExpressionAnnotation(make_smart<AstExprOp2Annotation<ExprOp2>>("ExprOp2",lib))->from("ExprOp");
        addExpressionAnnotation(make_smart<AstExprOp3Annotation>(lib))->from("ExprOp");
        addExpressionAnnotation(make_smart<AstExprCopyAnnotation>(lib))->from("ExprOp2");
        addExpressionAnnotation(make_smart<AstExprMoveAnnotation>(lib))->from("ExprOp2");
        addExpressionAnnotation(make_smart<AstExprOp2Annotation<ExprClone>>("ExprClone",lib))->from("ExprOp2");
        addExpressionAnnotation(make_smart<AstExprWithAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprAssumeAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprWhileAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprTryCatchAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprIfThenElseAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprForAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprMakeLocalAnnotation<ExprMakeLocal>>("ExprMakeLocal",lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprMakeStructAnnotation>(lib))->from("ExprMakeLocal");
        addExpressionAnnotation(make_smart<AstExprMakeVariantAnnotation>(lib))->from("ExprMakeLocal");
        addExpressionAnnotation(make_smart<AstExprMakeArrayAnnotation<ExprMakeArray>>("ExprMakeArray",lib))->from("ExprMakeLocal");
        addExpressionAnnotation(make_smart<AstExprMakeTupleAnnotation>(lib))->from("ExprMakeArray");
        addExpressionAnnotation(make_smart<AstExprArrayComprehensionAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstTypeInfoMacroAnnotation>(lib));
        addExpressionAnnotation(make_smart<AstExprTypeInfoAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprTypeDeclAnnotation>(lib))->from("Expression");

        // make struct each
        addExtern<DAS_BIND_FUN(das_vector_each<MakeStruct>),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
            SideEffects::none, "das_vector_each")->generated = true;
        addExtern<DAS_BIND_FUN(das_vector_each_const<MakeStruct>),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
            SideEffects::none, "das_vector_each_const")->generated = true;
    }
}
