#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // NOTE: parameters here are unreferenced for a reason
    //            the idea is you copy the function defintion, and paste to your code
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4100)    // unreferenced formal parameter
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

    class Visitor : public ptr_ref_count {
    protected:
        virtual ~Visitor() {}
    public:
        // what do we visit
        virtual bool canVisitStructure ( Structure * st ) { return true; }
        virtual bool canVisitGlobalVariable ( Variable * fun ) { return true; }
        virtual bool canVisitFunction ( Function * fun ) { return true; }
        virtual bool canVisitEnumeration ( Enumeration * en ) { return true; }
        virtual bool canVisitStructureFieldInit ( Structure * var ) { return true; }
        virtual bool canVisitIfSubexpr ( ExprIfThenElse * ) { return true; }
        virtual bool canVisitExpr ( ExprTypeInfo * expr, Expression * subexpr ) { return true; }
        virtual bool canVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) { return true; }
        virtual bool canVisitMakeStructureBody ( ExprMakeStruct * expr ) { return true; }
        virtual bool canVisitArgumentInit ( Function * fun, const VariablePtr & var, Expression * init ) { return true; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) { return false; }
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * ) { return false; }
        virtual bool canVisitMakeBlockBody ( ExprMakeBlock * expr ) { return true; }
        virtual bool canVisitCall ( ExprCall * expr ) { return true; }
        // WHOLE PROGRAM
        virtual void preVisitProgram ( Program * prog ) {}
        virtual void visitProgram ( Program * prog ) {}
        // EACH MODULE
        virtual void preVisitModule ( Module * mod ) {}
        virtual void visitModule ( Module * mod ) {}
        // TYPE
        virtual void preVisit ( TypeDecl * td ) {}
        virtual TypeDeclPtr visit ( TypeDecl * td ) { return td; }
        // ALIAS
        virtual void preVisitAlias ( TypeDecl * td, const string & name ) {}
        virtual  TypeDeclPtr visitAlias ( TypeDecl * td, const string & name ) { return td; }
        // ENUMERATOIN
        virtual void preVisit ( Enumeration * enu ) { }
        virtual void preVisitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) { }
        virtual ExpressionPtr visitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) { return value; }
        virtual EnumerationPtr visit ( Enumeration * enu ) { return enu; }
        // STRUCTURE
        virtual void preVisit ( Structure * var ) { }
        virtual void preVisitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) {}
        virtual void visitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) {}
        virtual StructurePtr visit ( Structure * var ) { return var; }
        // REAL THINGS (AFTER STRUCTS AND ENUMS)
        virtual void preVisitProgramBody ( Program * prog, Module * mod ) {}
        // FUNCTON
        virtual void preVisit ( Function * ) {}
        virtual FunctionPtr visit ( Function * that ) { return that; }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) {}
        virtual VariablePtr visitArgument ( Function *, const VariablePtr & that, bool lastArg ) { return that; }
        virtual void preVisitArgumentInit ( Function * fn, const VariablePtr & var, Expression * init ) {}
        virtual ExpressionPtr visitArgumentInit ( Function *, const VariablePtr &, Expression * that ) { return that; }
        virtual void preVisitFunctionBody ( Function *,Expression * ) {}
        virtual ExpressionPtr visitFunctionBody ( Function *, Expression * that ) { return that; }
        // ANY
        virtual void preVisitExpression ( Expression * expr ) {}
        virtual ExpressionPtr visitExpression ( Expression * expr ) { return expr; }
        // BLOCK
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) {}
        virtual VariablePtr visitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) { return var; }
        virtual void preVisitBlockArgumentInit ( ExprBlock * block, const VariablePtr & var, Expression * init ) {}
        virtual ExpressionPtr visitBlockArgumentInit ( ExprBlock * block, const VariablePtr &, Expression * that ) { return that; }
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) {}
        virtual ExpressionPtr visitBlockExpression (  ExprBlock * block, Expression * expr ) { return expr; }
        virtual void preVisitBlockFinal ( ExprBlock * block ) {}
        virtual void visitBlockFinal ( ExprBlock * block ) {}
        virtual void preVisitBlockFinalExpression ( ExprBlock * block, Expression * expr ) {}
        virtual ExpressionPtr visitBlockFinalExpression (  ExprBlock * block, Expression * expr ) { return expr; }
        // LET
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) {}
        virtual VariablePtr visitLet ( ExprLet * let, const VariablePtr & var, bool last ) { return var; }
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * init ) {}
        virtual ExpressionPtr visitLetInit ( ExprLet *, const VariablePtr & var, Expression * that ) { return that; }
        // GLOBAL LET
        virtual void preVisitGlobalLetBody ( Program * prog ) {}
        virtual void preVisitGlobalLet ( const VariablePtr & ) {}
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) { return var; }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * ) {}
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & var, Expression * that ) { return that; }
        virtual void visitGlobalLetBody ( Program * prog ) {}
        // STRING BUILDER
        virtual void preVisit ( ExprStringBuilder * expr ) {}
        virtual void preVisitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) {}
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) { return expr; }
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) { return expr; }
        // NEW
        virtual void preVisitNewArg ( ExprNew * call, Expression * arg, bool last ) {}
        virtual ExpressionPtr visitNewArg ( ExprNew * call, Expression * arg , bool last ) { return arg; }
        // NAMED CALL
        virtual void preVisitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg, bool last ) {}
        virtual MakeFieldDeclPtr visitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg , bool last ) { return arg; }
        // CALL
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) {}
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg , bool last ) { return arg; }
        // CALL
        virtual void preVisitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) {}
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg , bool last ) { return arg; }
        // NULL COAELESCING
        virtual void preVisitNullCoaelescingDefault ( ExprNullCoalescing * , Expression * ) {}
        // TAG
        virtual void preVisitTagValue ( ExprTag *, Expression * ) {}
        // AT
        virtual void preVisitAtIndex ( ExprAt *, Expression * ) {}
        // SAFE AT
        virtual void preVisitSafeAtIndex ( ExprSafeAt *, Expression * ) {}
        // IS
        virtual void preVisitType ( ExprIs *, TypeDecl * ) {}
        // OP2
        virtual void preVisitRight ( ExprOp2 *, Expression * ) {}
        // OP3
        virtual void preVisitLeft  ( ExprOp3 *, Expression * ) {}
        virtual void preVisitRight ( ExprOp3 *, Expression * ) {}
        // COPY
        virtual void preVisitRight ( ExprCopy *, Expression * ) {}
        // MOVE
        virtual void preVisitRight ( ExprMove *, Expression * ) {}
        // CLONE
        virtual void preVisitRight ( ExprClone *, Expression * ) {}
        // WITH
        virtual void preVisitWithBody ( ExprWith *, Expression * ) {}
        // WHILE
        virtual void preVisitWhileBody ( ExprWhile *, Expression * ) {}
        // TRY-CATCH
        virtual void preVisitCatch ( ExprTryCatch *, Expression * ) {}
        // IF-THEN-ELSE
        virtual void preVisitIfBlock ( ExprIfThenElse *, Expression * ) {}
        virtual void preVisitElseBlock ( ExprIfThenElse *, Expression * ) {}
        // FOR
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) {}
        virtual VariablePtr visitFor ( ExprFor * expr, const VariablePtr & var, bool last ) { return var; }
        virtual void preVisitForStack ( ExprFor * ) {}
        virtual void preVisitForBody ( ExprFor *, Expression * ) {}
        virtual void preVisitForSource ( ExprFor *, Expression *, bool ) {}
        virtual ExpressionPtr visitForSource ( ExprFor *, Expression * that , bool ) { return that; }
        // MAKE VARIANT
        virtual void preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool lastField ) {}
        virtual MakeFieldDeclPtr visitMakeVariantField(ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool lastField) { return decl; }
            // MAKE STRUCTURE
        virtual void preVisitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool lastIndex ) {}
        virtual void visitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool lastField ) {}
        virtual void preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool lastField ) {}
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool lastField ) { return decl; }
        virtual void preVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) {}
        virtual ExpressionPtr visitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) { return blk; }
        // MAKE ARRAY
        virtual void preVisitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastIndex ) {}
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastField ) { return init; }
        // MAKE TUPLE
        virtual void preVisitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastIndex ) {}
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) { return init; }
        // ARRAY COMPREHENSION
        virtual void preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr ) {}
        virtual void preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where ) {}
        // DELETE
        virtual void preVisitDeleteSizeExpression ( ExprDelete *, Expression * ) {}
        // EXPRESSIONS
#define VISIT_EXPR(ExprType) \
        virtual void preVisit ( ExprType * that ) { preVisitExpression(that); } \
        virtual ExpressionPtr visit ( ExprType * that ) { return visitExpression(that); }
        // all visitable expressions
        VISIT_EXPR(ExprCallMacro)
        VISIT_EXPR(ExprUnsafe)
        VISIT_EXPR(ExprReader)
        VISIT_EXPR(ExprLabel)
        VISIT_EXPR(ExprGoto)
        VISIT_EXPR(ExprRef2Value)
        VISIT_EXPR(ExprRef2Ptr)
        VISIT_EXPR(ExprPtr2Ref)
        VISIT_EXPR(ExprAddr)
        VISIT_EXPR(ExprNullCoalescing)
        VISIT_EXPR(ExprAssert)
        VISIT_EXPR(ExprStaticAssert)
        VISIT_EXPR(ExprQuote)
        VISIT_EXPR(ExprDebug)
        VISIT_EXPR(ExprInvoke)
        VISIT_EXPR(ExprErase)
        VISIT_EXPR(ExprSetInsert)
        VISIT_EXPR(ExprFind)
        VISIT_EXPR(ExprKeyExists)
        VISIT_EXPR(ExprAscend)
        VISIT_EXPR(ExprCast)
        VISIT_EXPR(ExprNew)
        VISIT_EXPR(ExprDelete)
        VISIT_EXPR(ExprAt)
        VISIT_EXPR(ExprSafeAt)
        VISIT_EXPR(ExprBlock)
        VISIT_EXPR(ExprVar)
        VISIT_EXPR(ExprSwizzle)
        VISIT_EXPR(ExprField)
        VISIT_EXPR(ExprSafeField)
        VISIT_EXPR(ExprIsVariant)
        VISIT_EXPR(ExprAsVariant)
        VISIT_EXPR(ExprSafeAsVariant)
        VISIT_EXPR(ExprOp1)
        VISIT_EXPR(ExprOp2)
        VISIT_EXPR(ExprOp3)
        VISIT_EXPR(ExprCopy)
        VISIT_EXPR(ExprMove)
        VISIT_EXPR(ExprClone)
        VISIT_EXPR(ExprTryCatch)
        VISIT_EXPR(ExprReturn)
        VISIT_EXPR(ExprYield)
        VISIT_EXPR(ExprBreak)
        VISIT_EXPR(ExprContinue)
        VISIT_EXPR(ExprConst)
        VISIT_EXPR(ExprFakeContext)
        VISIT_EXPR(ExprFakeLineInfo)
        VISIT_EXPR(ExprConstPtr)
        VISIT_EXPR(ExprConstEnumeration)
        VISIT_EXPR(ExprConstBitfield)
        VISIT_EXPR(ExprConstInt8)
        VISIT_EXPR(ExprConstInt16)
        VISIT_EXPR(ExprConstInt64)
        VISIT_EXPR(ExprConstInt)
        VISIT_EXPR(ExprConstInt2)
        VISIT_EXPR(ExprConstInt3)
        VISIT_EXPR(ExprConstInt4)
        VISIT_EXPR(ExprConstUInt8)
        VISIT_EXPR(ExprConstUInt16)
        VISIT_EXPR(ExprConstUInt64)
        VISIT_EXPR(ExprConstUInt)
        VISIT_EXPR(ExprConstUInt2)
        VISIT_EXPR(ExprConstUInt3)
        VISIT_EXPR(ExprConstUInt4)
        VISIT_EXPR(ExprConstRange)
        VISIT_EXPR(ExprConstURange)
        VISIT_EXPR(ExprConstRange64)
        VISIT_EXPR(ExprConstURange64)
        VISIT_EXPR(ExprConstBool)
        VISIT_EXPR(ExprConstFloat)
        VISIT_EXPR(ExprConstFloat2)
        VISIT_EXPR(ExprConstFloat3)
        VISIT_EXPR(ExprConstFloat4)
        VISIT_EXPR(ExprConstString)
        VISIT_EXPR(ExprConstDouble)
        VISIT_EXPR(ExprLet)
        VISIT_EXPR(ExprFor)
        VISIT_EXPR(ExprLooksLikeCall)
        VISIT_EXPR(ExprMakeBlock)
        VISIT_EXPR(ExprMakeGenerator)
        VISIT_EXPR(ExprTypeInfo)
        VISIT_EXPR(ExprIs)
        VISIT_EXPR(ExprCall)
        VISIT_EXPR(ExprNamedCall)
        VISIT_EXPR(ExprIfThenElse)
        VISIT_EXPR(ExprWith)
        VISIT_EXPR(ExprAssume)
        VISIT_EXPR(ExprWhile)
        VISIT_EXPR(ExprMakeStruct)
        VISIT_EXPR(ExprMakeVariant)
        VISIT_EXPR(ExprMakeArray)
        VISIT_EXPR(ExprMakeTuple)
        VISIT_EXPR(ExprArrayComprehension)
        VISIT_EXPR(ExprMemZero)
        VISIT_EXPR(ExprTypeDecl)
        VISIT_EXPR(ExprTag)
#undef VISIT_EXPR
    };
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

    struct AstContext {
        bool valid = false;
        FunctionPtr             func;
        vector<ExpressionPtr>   loop;
        vector<ExpressionPtr>   blocks;
        vector<ExpressionPtr>   scopes;
        vector<ExpressionPtr>   with;
    };

    AstContext generateAstContext( const ProgramPtr & prog, Expression * expr );

    class PassVisitor : public Visitor {
    public:
        virtual void preVisitProgram ( Program * prog ) override;
        virtual void visitProgram ( Program * prog ) override;
        virtual void reportFolding();
        __forceinline bool didAnything() const { return anyFolding; }
    protected:
        bool        anyFolding = false;
        Program *   program = nullptr;
    };

    class FoldingVisitor : public PassVisitor {
    public:
        FoldingVisitor(const ProgramPtr & prog)
            : ctx(prog->getContextStackSize()), helper(ctx.debugInfo) {
            ctx.thisProgram = prog.get();
            ctx.thisHelper = &helper;
            ctx.heap = make_smart<LinearHeapAllocator>();
            ctx.stringHeap = make_smart<LinearStringAllocator>();
            ctx.category = uint32_t(ContextCategory::folding_context);
            helper.rtti = true;
        }
    protected:
        Context         ctx;
        DebugInfoHelper helper;
    protected:
        DAS_EVAL_ABI vec4f eval ( Expression * expr, bool & failed );
        ExpressionPtr evalAndFold ( Expression * expr );
        ExpressionPtr evalAndFoldString ( Expression * expr );
        ExpressionPtr evalAndFoldStringBuilder ( ExprStringBuilder * expr );
        ExpressionPtr cloneWithType ( const ExpressionPtr & expr );
        bool isSameFoldValue ( const TypeDeclPtr & t, vec4f a, vec4f b ) const {
            return memcmp(&a,&b,t->getSizeOf()) == 0;
        }
    };

    template <typename TT>
    ExpressionPtr ExprLikeCall<TT>::visit(Visitor & vis) {
        vis.preVisit(static_cast<TT *>(this));
        auto llk = ExprLooksLikeCall::visit(vis);
        return llk.get()==this ? vis.visit(static_cast<TT *>(this)) : llk;
    }

    template <typename It, typename SimNodeT, bool first>
    ExpressionPtr ExprTableKeysOrValues<It,SimNodeT,first>::visit(Visitor & vis) {
        vis.preVisit(static_cast<It *>(this));
        auto llk = ExprLooksLikeCall::visit(vis);
        return llk.get()==this ? vis.visit(static_cast<It *>(this)) : llk;
    }

    template <typename It, typename SimNodeT>
    ExpressionPtr ExprArrayCallWithSizeOrIndex<It,SimNodeT>::visit(Visitor & vis) {
        vis.preVisit(static_cast<It *>(this));
        auto llk = ExprLooksLikeCall::visit(vis);
        return llk.get()==this ? vis.visit(static_cast<It *>(this)) : llk;
    }

    template <typename TT, typename ExprConstExt>
    ExpressionPtr ExprConstT<TT,ExprConstExt>::visit(Visitor & vis) {
        vis.preVisit((ExprConst*)this);
        vis.preVisit((ExprConstExt*)this);
        auto res = vis.visit((ExprConstExt*)this);
        if ( res.get()!=this ) return res;
        return vis.visit((ExprConst *)this);
    }
}
