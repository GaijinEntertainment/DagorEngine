#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
namespace das {

#define FN_PREVISIT(WHAT)  fnPreVisit##WHAT
#define FN_VISIT(WHAT)      fnVisit##WHAT

#define DECL_VISIT(WHAT) \
    Func        FN_PREVISIT(WHAT); \
    Func        FN_VISIT(WHAT);

#define IMPL_BIND_EXPR(WHAT) \
    virtual void preVisit ( WHAT * expr ) override; \
    virtual ExpressionPtr visit ( WHAT * expr ) override;

    class VisitorAdapter : public Visitor {
    public:
        VisitorAdapter ( char * pClass, const StructInfo * info, Context * ctx );
    protected:
        void *      classPtr;
        Context *   context;
    protected:
        DECL_VISIT(Program);
        Func FN_PREVISIT(ProgramBody);
        DECL_VISIT(Module);
        DECL_VISIT(TypeDecl);
        DECL_VISIT(Expression);
        DECL_VISIT(Alias);
        Func fnCanVisitEnumeration;
        DECL_VISIT(Enumeration);
        DECL_VISIT(EnumerationValue);
        Func fnCanVisitStructure;
        DECL_VISIT(Structure);
        DECL_VISIT(StructureField);
        Func fnCanVisitFunction;
        Func fnCanVisitArgumentInit;
        DECL_VISIT(Function);
        DECL_VISIT(FunctionArgument);
        DECL_VISIT(FunctionArgumentInit);
        DECL_VISIT(FunctionBody);
        DECL_VISIT(ExprBlock);
        DECL_VISIT(ExprBlockArgument);
        DECL_VISIT(ExprBlockArgumentInit);
        DECL_VISIT(ExprBlockExpression);
        DECL_VISIT(ExprBlockFinal);
        DECL_VISIT(ExprBlockFinalExpression);
        DECL_VISIT(ExprLet);
        DECL_VISIT(ExprLetVariable);
        DECL_VISIT(ExprLetVariableInit);
        Func fnCanVisitGlobalVariable;
        DECL_VISIT(GlobalLet);
        DECL_VISIT(GlobalLetVariable);
        DECL_VISIT(GlobalLetVariableInit);
        DECL_VISIT(ExprStringBuilder);
        DECL_VISIT(ExprStringBuilderElement);
        DECL_VISIT(ExprNew);
        DECL_VISIT(ExprNewArgument);
        DECL_VISIT(ExprNamedCall);
        DECL_VISIT(ExprNamedCallArgument);
        Func fnCanVisitCall;
        DECL_VISIT(ExprCall);
        DECL_VISIT(ExprCallArgument);
        DECL_VISIT(ExprLooksLikeCall);
        DECL_VISIT(ExprLooksLikeCallArgument);
        DECL_VISIT(ExprNullCoalescing);
        Func FN_PREVISIT(ExprNullCoalescingDefault);
        DECL_VISIT(ExprAt);
        Func FN_PREVISIT(ExprAtIndex);
        DECL_VISIT(ExprSafeAt);
        Func FN_PREVISIT(ExprSafeAtIndex);
        DECL_VISIT(ExprIs);
        Func FN_PREVISIT(ExprIsType);
        DECL_VISIT(ExprOp2);
        Func FN_PREVISIT(ExprOp2Right);
        DECL_VISIT(ExprOp3);
        Func FN_PREVISIT(ExprOp3Left);
        Func FN_PREVISIT(ExprOp3Right);
        DECL_VISIT(ExprCopy);
        Func FN_PREVISIT(ExprCopyRight);
        DECL_VISIT(ExprMove);
        Func FN_PREVISIT(ExprMoveRight);
        DECL_VISIT(ExprClone);
        Func FN_PREVISIT(ExprCloneRight);
        Func fnCanVisitWithAliasSubexpression;
        DECL_VISIT(ExprAssume);
        DECL_VISIT(ExprWith);
        Func FN_PREVISIT(ExprWithBody);
        DECL_VISIT(ExprWhile);
        Func FN_PREVISIT(ExprWhileBody);
        DECL_VISIT(ExprTryCatch);
        Func FN_PREVISIT(ExprTryCatchCatch);
        DECL_VISIT(ExprIfThenElse);
        Func FN_PREVISIT(ExprIfThenElseIfBlock);
        Func FN_PREVISIT(ExprIfThenElseElseBlock);
        DECL_VISIT(ExprFor);
        DECL_VISIT(ExprForVariable);
        DECL_VISIT(ExprForSource);
        Func FN_PREVISIT(ExprForStack);
        Func FN_PREVISIT(ExprForBody);
        DECL_VISIT(ExprMakeVariant);
        DECL_VISIT(ExprMakeVariantField);
        Func fnCanVisitMakeStructBody;
        Func fnCanVisitMakeStructBlock;
        DECL_VISIT(ExprMakeStruct);
        DECL_VISIT(ExprMakeStructIndex);
        DECL_VISIT(ExprMakeStructField);
        DECL_VISIT(ExprMakeArray);
        DECL_VISIT(ExprMakeArrayIndex);
        DECL_VISIT(ExprMakeTuple);
        DECL_VISIT(ExprMakeTupleIndex);
        DECL_VISIT(ExprArrayComprehension);
        Func FN_PREVISIT(ExprArrayComprehensionSubexpr);
        Func FN_PREVISIT(ExprArrayComprehensionWhere);
        DECL_VISIT(ExprTypeInfo);
        DECL_VISIT(ExprTypeDecl);
        DECL_VISIT(ExprLabel);
        DECL_VISIT(ExprGoto);
        DECL_VISIT(ExprRef2Value);
        DECL_VISIT(ExprRef2Ptr);
        DECL_VISIT(ExprPtr2Ref);
        DECL_VISIT(ExprAddr);
        DECL_VISIT(ExprAssert);
        DECL_VISIT(ExprStaticAssert);
        DECL_VISIT(ExprQuote);
        DECL_VISIT(ExprDebug);
        DECL_VISIT(ExprInvoke);
        DECL_VISIT(ExprErase);
        DECL_VISIT(ExprSetInsert);
        DECL_VISIT(ExprFind);
        DECL_VISIT(ExprKeyExists);
        DECL_VISIT(ExprAscend);
        DECL_VISIT(ExprCast);
        DECL_VISIT(ExprDelete);
        DECL_VISIT(ExprVar);
        DECL_VISIT(ExprTag);
        Func FN_PREVISIT(ExprTagValue);
        DECL_VISIT(ExprSwizzle);
        DECL_VISIT(ExprField);
        DECL_VISIT(ExprSafeField);
        DECL_VISIT(ExprIsVariant);
        DECL_VISIT(ExprAsVariant);
        DECL_VISIT(ExprSafeAsVariant);
        DECL_VISIT(ExprOp1);
        DECL_VISIT(ExprReturn);
        DECL_VISIT(ExprYield);
        DECL_VISIT(ExprBreak);
        DECL_VISIT(ExprContinue);
        DECL_VISIT(ExprConst);
        DECL_VISIT(ExprFakeContext);
        DECL_VISIT(ExprFakeLineInfo);
        DECL_VISIT(ExprConstPtr);
        DECL_VISIT(ExprConstEnumeration);
        DECL_VISIT(ExprConstBitfield);
        DECL_VISIT(ExprConstInt8);
        DECL_VISIT(ExprConstInt16);
        DECL_VISIT(ExprConstInt64);
        DECL_VISIT(ExprConstInt);
        DECL_VISIT(ExprConstInt2);
        DECL_VISIT(ExprConstInt3);
        DECL_VISIT(ExprConstInt4);
        DECL_VISIT(ExprConstUInt8);
        DECL_VISIT(ExprConstUInt16);
        DECL_VISIT(ExprConstUInt64);
        DECL_VISIT(ExprConstUInt);
        DECL_VISIT(ExprConstUInt2);
        DECL_VISIT(ExprConstUInt3);
        DECL_VISIT(ExprConstUInt4);
        DECL_VISIT(ExprConstRange);
        DECL_VISIT(ExprConstURange);
        DECL_VISIT(ExprConstRange64);
        DECL_VISIT(ExprConstURange64);
        DECL_VISIT(ExprConstBool);
        DECL_VISIT(ExprConstFloat);
        DECL_VISIT(ExprConstFloat2);
        DECL_VISIT(ExprConstFloat3);
        DECL_VISIT(ExprConstFloat4);
        DECL_VISIT(ExprConstString);
        DECL_VISIT(ExprConstDouble);
        Func fnCanVisitMakeBlockBody;
        DECL_VISIT(ExprMakeBlock);
        DECL_VISIT(ExprMakeGenerator);
        DECL_VISIT(ExprMemZero);
        DECL_VISIT(ExprReader);
        DECL_VISIT(ExprUnsafe);
        DECL_VISIT(ExprCallMacro);
    protected:
    // whole program
        virtual void preVisitProgram ( Program * expr ) override;
        virtual void visitProgram ( Program * expr ) override;
        virtual void preVisitProgramBody ( Program * expr, Module * mod ) override;
    // module
        virtual void preVisitModule ( Module * mod ) override;
        virtual void visitModule ( Module * mod ) override;
    // type
        virtual void preVisit ( TypeDecl * expr ) override;
        virtual TypeDeclPtr visit ( TypeDecl * expr ) override;
    // alias
        virtual void preVisitAlias ( TypeDecl * expr, const string & name ) override;
        virtual TypeDeclPtr visitAlias ( TypeDecl * expr, const string & name ) override;
    // enumeration
        virtual bool canVisitEnumeration ( Enumeration * en ) override;
        virtual void preVisit ( Enumeration * expr ) override;
        virtual EnumerationPtr visit ( Enumeration * expr ) override;
        virtual void preVisitEnumerationValue ( Enumeration * expr, const string & name, Expression * value, bool last ) override;
        virtual ExpressionPtr visitEnumerationValue ( Enumeration * expr, const string & name, Expression * value, bool last ) override;
    // structure
        virtual bool canVisitStructure ( Structure * fun ) override;
        virtual void preVisit ( Structure * expr ) override;
        virtual void preVisitStructureField ( Structure * expr, Structure::FieldDeclaration & decl, bool last ) override;
        virtual void visitStructureField ( Structure * expr, Structure::FieldDeclaration & decl, bool last ) override;
        virtual StructurePtr visit ( Structure * expr ) override;
    // function
        virtual bool canVisitFunction ( Function * fun ) override;
        virtual bool canVisitArgumentInit ( Function * fun, const VariablePtr & var, Expression * init ) override;
        virtual void preVisit ( Function * expr ) override;
        virtual FunctionPtr visit ( Function * expr ) override;
        virtual void preVisitArgument ( Function * expr, const VariablePtr & var, bool lastArg ) override;
        virtual VariablePtr visitArgument ( Function * expr, const VariablePtr & var, bool lastArg ) override;
        virtual void preVisitArgumentInit ( Function * expr, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitArgumentInit ( Function * expr, const VariablePtr & var, Expression * init ) override;
        virtual void preVisitFunctionBody ( Function * expr, Expression * that ) override;
        virtual ExpressionPtr visitFunctionBody ( Function * expr, Expression * that ) override;
    // expression
        virtual void preVisitExpression ( Expression * expr ) override;
        virtual ExpressionPtr visitExpression ( Expression * expr ) override;
    // block
        IMPL_BIND_EXPR(ExprBlock);
        virtual void preVisitBlockArgument ( ExprBlock * expr, const VariablePtr & var, bool lastArg ) override;
        virtual VariablePtr visitBlockArgument ( ExprBlock * expr, const VariablePtr & var, bool lastArg ) override;
        virtual void preVisitBlockArgumentInit ( ExprBlock * expr, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitBlockArgumentInit ( ExprBlock * expr, const VariablePtr & var, Expression * init ) override;
        virtual void preVisitBlockExpression ( ExprBlock * expr, Expression * bexpr ) override;
        virtual ExpressionPtr visitBlockExpression (  ExprBlock * expr, Expression * bexpr ) override;
        virtual void preVisitBlockFinal ( ExprBlock * expr ) override;
        virtual void visitBlockFinal ( ExprBlock * expr ) override;
        virtual void preVisitBlockFinalExpression ( ExprBlock * expr, Expression * bexpr ) override;
        virtual ExpressionPtr visitBlockFinalExpression (  ExprBlock * expr, Expression * bexpr ) override;
    // let
        IMPL_BIND_EXPR(ExprLet);
        virtual void preVisitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override;
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override;
        virtual void preVisitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init ) override;
    // global let
        virtual bool canVisitGlobalVariable ( Variable * var ) override;
        virtual void preVisitGlobalLetBody ( Program * expr ) override;
        virtual void visitGlobalLetBody ( Program * expr ) override;
        virtual void preVisitGlobalLet ( const VariablePtr & expr ) override;
        virtual VariablePtr visitGlobalLet ( const VariablePtr & expr ) override;
        virtual void preVisitGlobalLetInit ( const VariablePtr & expr, Expression * init ) override;
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & expr, Expression * init ) override;
    // string builder
        IMPL_BIND_EXPR(ExprStringBuilder);
        virtual void preVisitStringBuilderElement ( ExprStringBuilder * expr, Expression * element, bool last ) override;
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * expr, Expression * element, bool last ) override;
    // new
        IMPL_BIND_EXPR(ExprNew);
        virtual void preVisitNewArg ( ExprNew * expr, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitNewArg ( ExprNew * expr, Expression * arg , bool last ) override;
    // named call
        IMPL_BIND_EXPR(ExprNamedCall);
        virtual void preVisitNamedCallArg ( ExprNamedCall * expr, MakeFieldDecl * arg, bool last ) override;
        virtual MakeFieldDeclPtr visitNamedCallArg ( ExprNamedCall * expr, MakeFieldDecl * arg , bool last ) override;
    // call
        IMPL_BIND_EXPR(ExprCall);
        virtual bool canVisitCall ( ExprCall * expr ) override;
        virtual void preVisitCallArg ( ExprCall * expr, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitCallArg ( ExprCall * expr, Expression * arg , bool last ) override;
    // looks like call
        IMPL_BIND_EXPR(ExprLooksLikeCall);
        virtual void preVisitLooksLikeCallArg ( ExprLooksLikeCall * expr, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * expr, Expression * arg , bool last ) override;
    // null coaelescing
        IMPL_BIND_EXPR(ExprNullCoalescing);
        virtual void preVisitNullCoaelescingDefault ( ExprNullCoalescing * expr, Expression * defval ) override;
    // at
        IMPL_BIND_EXPR(ExprAt);
        virtual void preVisitAtIndex ( ExprAt * expr, Expression * index ) override;
    // safe at
        IMPL_BIND_EXPR(ExprSafeAt);
        virtual void preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index ) override;
    // is
        IMPL_BIND_EXPR(ExprIs);
        virtual void preVisitType ( ExprIs * expr, TypeDecl * typeDecl ) override;
    // op2
        IMPL_BIND_EXPR(ExprOp2);
        virtual void preVisitRight ( ExprOp2 * expr, Expression * right ) override;
    // op3
        IMPL_BIND_EXPR(ExprOp3);
        virtual void preVisitLeft  ( ExprOp3 * expr, Expression * left ) override;
        virtual void preVisitRight ( ExprOp3 * expr, Expression * right ) override;
    // copy
        IMPL_BIND_EXPR(ExprCopy);
        virtual void preVisitRight ( ExprCopy * expr, Expression * right ) override;
    // move
        IMPL_BIND_EXPR(ExprMove);
        virtual void preVisitRight ( ExprMove * expr, Expression * right ) override;
    // clone
        IMPL_BIND_EXPR(ExprClone);
        virtual void preVisitRight ( ExprClone * expr, Expression * right ) override;
    // assume
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * expr) override;
        IMPL_BIND_EXPR(ExprAssume);
    // with
        IMPL_BIND_EXPR(ExprWith);
        virtual void preVisitWithBody ( ExprWith * expr, Expression * body ) override;
    // while
        IMPL_BIND_EXPR(ExprWhile);
        virtual void preVisitWhileBody ( ExprWhile * expr, Expression * body ) override;
    // try-catch
        IMPL_BIND_EXPR(ExprTryCatch);
        virtual void preVisitCatch ( ExprTryCatch * expr, Expression * body ) override;
    // if-then-else
        IMPL_BIND_EXPR(ExprIfThenElse);
        virtual void preVisitIfBlock ( ExprIfThenElse * expr, Expression * ifBlock ) override;
        virtual void preVisitElseBlock ( ExprIfThenElse * expr, Expression * elseBlock ) override;
    // for
        IMPL_BIND_EXPR(ExprFor);
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override;
        virtual VariablePtr visitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override;
        virtual void preVisitForStack ( ExprFor * expr ) override;
        virtual void preVisitForBody ( ExprFor * expr, Expression * body ) override;
        virtual void preVisitForSource ( ExprFor * expr, Expression * source, bool last ) override;
        virtual ExpressionPtr visitForSource ( ExprFor * expr, Expression * source , bool last ) override;
    // make variant
        IMPL_BIND_EXPR(ExprMakeVariant);
        virtual void preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override;
        virtual MakeFieldDeclPtr visitMakeVariantField(ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last) override;
    // make structure
        virtual bool canVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) override;
        virtual bool canVisitMakeStructureBody ( ExprMakeStruct * expr ) override;
        IMPL_BIND_EXPR(ExprMakeStruct);
        virtual void preVisitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool last ) override;
        virtual void visitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool last ) override;
        virtual void preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override;
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override;
    // make array
        IMPL_BIND_EXPR(ExprMakeArray);
        virtual void preVisitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool last ) override;
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool last ) override;
    // make tuple
        IMPL_BIND_EXPR(ExprMakeTuple);
        virtual void preVisitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool last ) override;
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool last ) override;
    // array comprehension
        IMPL_BIND_EXPR(ExprArrayComprehension);
        virtual void preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr ) override;
        virtual void preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where ) override;
    // type info
        IMPL_BIND_EXPR(ExprTypeInfo);
    // all other expressions
        IMPL_BIND_EXPR(ExprLabel);
        IMPL_BIND_EXPR(ExprGoto);
        IMPL_BIND_EXPR(ExprRef2Value);
        IMPL_BIND_EXPR(ExprRef2Ptr);
        IMPL_BIND_EXPR(ExprPtr2Ref);
        IMPL_BIND_EXPR(ExprAddr);
        IMPL_BIND_EXPR(ExprAssert);
        IMPL_BIND_EXPR(ExprStaticAssert);
        IMPL_BIND_EXPR(ExprQuote);
        IMPL_BIND_EXPR(ExprDebug);
        IMPL_BIND_EXPR(ExprInvoke);
        IMPL_BIND_EXPR(ExprErase);
        IMPL_BIND_EXPR(ExprSetInsert);
        IMPL_BIND_EXPR(ExprFind);
        IMPL_BIND_EXPR(ExprKeyExists);
        IMPL_BIND_EXPR(ExprAscend);
        IMPL_BIND_EXPR(ExprCast);
        IMPL_BIND_EXPR(ExprDelete);
        IMPL_BIND_EXPR(ExprVar);
        IMPL_BIND_EXPR(ExprTag);
        virtual void preVisitTagValue ( ExprTag *, Expression * ) override;
        IMPL_BIND_EXPR(ExprSwizzle);
        IMPL_BIND_EXPR(ExprField);
        IMPL_BIND_EXPR(ExprSafeField);
        IMPL_BIND_EXPR(ExprIsVariant);
        IMPL_BIND_EXPR(ExprAsVariant);
        IMPL_BIND_EXPR(ExprSafeAsVariant);
        IMPL_BIND_EXPR(ExprOp1);
        IMPL_BIND_EXPR(ExprReturn);
        IMPL_BIND_EXPR(ExprYield);
        IMPL_BIND_EXPR(ExprBreak);
        IMPL_BIND_EXPR(ExprContinue);
        IMPL_BIND_EXPR(ExprConst);
        IMPL_BIND_EXPR(ExprFakeContext);
        IMPL_BIND_EXPR(ExprFakeLineInfo);
        IMPL_BIND_EXPR(ExprConstPtr);
        IMPL_BIND_EXPR(ExprConstEnumeration);
        IMPL_BIND_EXPR(ExprConstBitfield);
        IMPL_BIND_EXPR(ExprConstInt8);
        IMPL_BIND_EXPR(ExprConstInt16);
        IMPL_BIND_EXPR(ExprConstInt64);
        IMPL_BIND_EXPR(ExprConstInt);
        IMPL_BIND_EXPR(ExprConstInt2);
        IMPL_BIND_EXPR(ExprConstInt3);
        IMPL_BIND_EXPR(ExprConstInt4);
        IMPL_BIND_EXPR(ExprConstUInt8);
        IMPL_BIND_EXPR(ExprConstUInt16);
        IMPL_BIND_EXPR(ExprConstUInt64);
        IMPL_BIND_EXPR(ExprConstUInt);
        IMPL_BIND_EXPR(ExprConstUInt2);
        IMPL_BIND_EXPR(ExprConstUInt3);
        IMPL_BIND_EXPR(ExprConstUInt4);
        IMPL_BIND_EXPR(ExprConstRange);
        IMPL_BIND_EXPR(ExprConstURange);
        IMPL_BIND_EXPR(ExprConstRange64);
        IMPL_BIND_EXPR(ExprConstURange64);
        IMPL_BIND_EXPR(ExprConstBool);
        IMPL_BIND_EXPR(ExprConstFloat);
        IMPL_BIND_EXPR(ExprConstFloat2);
        IMPL_BIND_EXPR(ExprConstFloat3);
        IMPL_BIND_EXPR(ExprConstFloat4);
        IMPL_BIND_EXPR(ExprConstString);
        IMPL_BIND_EXPR(ExprConstDouble);
        bool canVisitMakeBlockBody ( ExprMakeBlock * expr ) override;
        IMPL_BIND_EXPR(ExprMakeBlock);
        IMPL_BIND_EXPR(ExprMakeGenerator);
        IMPL_BIND_EXPR(ExprMemZero);
        IMPL_BIND_EXPR(ExprReader);
        IMPL_BIND_EXPR(ExprUnsafe);
        IMPL_BIND_EXPR(ExprCallMacro);
        IMPL_BIND_EXPR(ExprTypeDecl);
    };

#undef FN_PREVISIT
#undef FN_VISIT
#undef DECL_VISIT
#undef IMPL_BIND_EXPR

    char * ast_describe_typedecl ( smart_ptr_raw<TypeDecl> t, bool d_extra, bool d_contracts, bool d_module, Context * context, LineInfoArg * at );
    char * ast_describe_typedecl_cpp ( smart_ptr_raw<TypeDecl> t, bool d_substitureRef, bool d_skipRef, bool d_skipConst, bool d_redundantConst, Context * context, LineInfoArg * at );
    char * ast_describe_expression ( smart_ptr_raw<Expression> t, Context * context, LineInfoArg * at );
    char * ast_describe_function ( smart_ptr_raw<Function> t, Context * context, LineInfoArg * at );
    char * ast_das_to_string ( Type bt, Context * context );
    char * ast_find_bitfield_name ( smart_ptr_raw<TypeDecl> bft, Bitfield value, Context * context, LineInfoArg * at );
    int64_t ast_find_enum_value ( EnumerationPtr enu, const char * value );

    int32_t any_array_size ( void * _arr );
    int32_t any_table_size ( void * _tab );
    void any_array_foreach ( void * _arr, int stride, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    void any_table_foreach ( void * _tab, int keyStride, int valueStride, const TBlock<void,void *,void *> & blk, Context * context, LineInfoArg * at );

    int32_t get_variant_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );
    int32_t get_tuple_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );

    __forceinline void mks_vector_emplace ( MakeStruct & vec, MakeFieldDeclPtr & value ) {
        vec.emplace_back(das::move(value));
    }
    __forceinline void mks_vector_pop ( MakeStruct & vec ) {
        vec.pop_back();
    }
    __forceinline void mks_vector_clear ( MakeStruct & vec ) {
        vec.clear();
    }
    __forceinline void mks_vector_resize ( MakeStruct & vec, int32_t newSize ) {
        vec.resize(newSize);
    }

    Module * compileModule ( Context * context, LineInfoArg * at );
    smart_ptr_raw<Program> compileProgram ( Context * context, LineInfoArg * at );

    DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context );
    Module * thisModule ( Context * context, LineInfoArg * lineinfo );
    smart_ptr_raw<Program> thisProgram ( Context * context );
    void astVisit ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    void astVisitModulesInOrder ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    void astVisitFunction ( smart_ptr_raw<Function> func, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    smart_ptr_raw<Expression> astVisitExpression ( smart_ptr_raw<Expression> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    void astVisitBlockFinally ( smart_ptr_raw<ExprBlock> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    PassMacroPtr makePassMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    smart_ptr<VisitorAdapter> makeVisitor ( const void * pClass, const StructInfo * info, Context * context );
    void addModuleInferMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleInferDirtyMacro ( Module * module, PassMacroPtr & newM, Context * context );
    void addModuleLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleGlobalLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleOptimizationMacro ( Module * module, PassMacroPtr & _newM, Context * );
    VariantMacroPtr makeVariantMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleVariantMacro ( Module * module, VariantMacroPtr & newM, Context * context );
    ForLoopMacroPtr makeForLoopMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleForLoopMacro ( Module * module, ForLoopMacroPtr & _newM, Context * );
    CaptureMacroPtr makeCaptureMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleCaptureMacro ( Module * module, CaptureMacroPtr & _newM, Context * );
    SimulateMacroPtr makeSimulateMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleSimulateMacro ( Module * module, SimulateMacroPtr & _newM, Context * );
    void addModuleFunctionAnnotation ( Module * module, FunctionAnnotationPtr & ann, Context * context, LineInfoArg * at );
    FunctionAnnotationPtr makeFunctionAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    StructureAnnotationPtr makeStructureAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    EnumerationAnnotationPtr makeEnumerationAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    void addModuleStructureAnnotation ( Module * module, StructureAnnotationPtr & ann, Context * context, LineInfoArg * at );
    void addStructureStructureAnnotation ( smart_ptr_raw<Structure> st, StructureAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    void addModuleEnumerationAnnotation ( Module * module, EnumerationAnnotationPtr & ann, Context * context, LineInfoArg * at );
    int addEnumerationEntry ( smart_ptr<Enumeration> enu, const char* name );
    void forEachFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    void forEachGenericFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    bool addModuleFunction ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    bool addModuleGeneric ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    bool addModuleVariable ( Module * module, VariablePtr & var, Context * context, LineInfoArg * lineInfo );
    bool addModuleKeyword ( Module * module, char * kwd, bool needOxfordComma, Context * context, LineInfoArg * lineInfo );
    VariablePtr findModuleVariable ( Module * module, const char * name );
    bool removeModuleStructure ( Module * module, StructurePtr & _stru );
    bool addModuleStructure ( Module * module, StructurePtr & stru );
    bool addModuleAlias ( Module * module, TypeDeclPtr & _ptr );
    void ast_error ( ProgramPtr prog, const LineInfo & at, const char * message, Context * context, LineInfoArg * lineInfo );
    void addModuleReaderMacro ( Module * module, ReaderMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    ReaderMacroPtr makeReaderMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    CommentReaderPtr makeCommentReader ( const void * pClass, const StructInfo * info, Context * context );
    void addModuleCommentReader ( Module * module, CommentReaderPtr & _newM, Context * context, LineInfoArg * lineInfo );
    void addModuleCallMacro ( Module * module, CallMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    CallMacroPtr makeCallMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    TypeInfoMacroPtr makeTypeInfoMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleTypeInfoMacro ( Module * module, TypeInfoMacroPtr & _newM, Context * context, LineInfoArg * at );
    void addFunctionFunctionAnnotation(smart_ptr_raw<Function> func, FunctionAnnotationPtr & ann, Context* context, LineInfoArg* at);
    void addAndApplyFunctionAnnotation ( smart_ptr_raw<Function> func, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    void addBlockBlockAnnotation ( smart_ptr_raw<ExprBlock> block, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    void addAndApplyBlockAnnotation ( smart_ptr_raw<ExprBlock> blk, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    void addAndApplyStructAnnotation ( smart_ptr_raw<Structure> st, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    __forceinline ExpressionPtr clone_expression ( ExpressionPtr value ) { return value ?value->clone() : nullptr; }
    __forceinline FunctionPtr clone_function ( FunctionPtr value ) { return value ? value->clone() : nullptr; }
    __forceinline TypeDeclPtr clone_type ( TypeDeclPtr value ) { return value ? make_smart<TypeDecl>(*value) : nullptr; }
    __forceinline StructurePtr clone_structure ( const Structure * value ) { return value ? value->clone() : nullptr; }
    __forceinline VariablePtr clone_variable ( VariablePtr value ) { return value ? value->clone() : nullptr; }
    void forceAtRaw ( const smart_ptr_raw<Expression> & expr, const LineInfo & at );
    void getAstContext ( smart_ptr_raw<Program> prog, smart_ptr_raw<Expression> expr, const TBlock<void,bool,AstContext> & block, Context * context, LineInfoArg * at );
    char * get_mangled_name ( smart_ptr_raw<Function> func, Context * context, LineInfoArg * at );
    char * get_mangled_name_t ( smart_ptr_raw<TypeDecl> typ, Context * context, LineInfoArg * at );
    char * get_mangled_name_v ( smart_ptr_raw<Variable> var, Context * context, LineInfoArg * at );
    char * get_mangled_name_b ( smart_ptr_raw<ExprBlock> expr, Context * context, LineInfoArg * at );
    TypeDeclPtr parseMangledNameFn ( const char * txt, ModuleGroup & lib, Module * thisModule, Context * context, LineInfoArg * at );
    void collectDependencies ( FunctionPtr fun, const TBlock<void,TArray<Function *>,TArray<Variable *>> & block, Context * context, LineInfoArg * line );
    bool isExprLikeCall ( const ExpressionPtr & expr );
    bool isExprConst ( const ExpressionPtr & expr );
    bool isTempType ( TypeDeclPtr ptr, bool refMatters );
    float4 evalSingleExpression ( const ExpressionPtr & expr, bool & ok );
    ExpressionPtr makeCall ( const LineInfo & at, const char * name );
    bool builtin_isVisibleDirectly ( Module * from, Module * too );
    bool builtin_hasField ( TypeDeclPtr ptr, const char * field, bool constant );
    TypeDeclPtr builtin_fieldType ( TypeDeclPtr ptr, const char * field, bool constant );
    Module * findRttiModule ( smart_ptr<Program> THAT_PROGRAM, const char * name, Context *, LineInfoArg *);
    smart_ptr<Function> findRttiFunction ( Module * mod, Func func, Context * context, LineInfoArg * line_info );
    void for_each_module ( Program * prog, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at );
    void for_each_typedef ( Module * mod, const TBlock<void,TTemporary<char *>,TypeDeclPtr> & block, Context * context, LineInfoArg * at );
    void for_each_enumeration ( Module * mod, const TBlock<void,EnumerationPtr> & block, Context * context, LineInfoArg * at );
    void for_each_structure ( Module * mod, const TBlock<void,StructurePtr> & block, Context * context, LineInfoArg * at );
    void for_each_generic ( Module * mod, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at );
    void for_each_global ( Module * mod, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at );
    void for_each_call_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    void for_each_reader_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    void for_each_variant_macro ( Module * mod, const TBlock<void,VariantMacroPtr> & block, Context * context, LineInfoArg * at );
    void for_each_typeinfo_macro ( Module * mod, const TBlock<void,TypeInfoMacroPtr> & block, Context * context, LineInfoArg * at );
    void for_each_for_loop_macro ( Module * mod, const TBlock<void,ForLoopMacroPtr> & block, Context * context, LineInfoArg * at );
    Annotation * get_expression_annotation ( Expression * expr, Context * context, LineInfoArg * at );
    Structure * find_unique_structure ( smart_ptr_raw<Program> prog, const char * name, Context * context, LineInfoArg * at );
    void get_use_global_variables ( smart_ptr_raw<Function> func, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at );
    void get_use_functions ( smart_ptr_raw<Function> func, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at );
    Structure::FieldDeclaration * ast_findStructureField ( Structure * structType, const char * field, Context * context, LineInfoArg * at );
    int32_t ast_getTupleFieldOffset ( smart_ptr_raw<TypeDecl> ttype, int32_t field, Context * context, LineInfoArg * at );
    void das_comp_log ( const char * text, Context * context, LineInfoArg * at );
    TypeInfo * das_make_type_info_structure ( Context & ctx, TypeDeclPtr ptr, Context * context, LineInfoArg * at );
    bool isSameAstType ( TypeDeclPtr THIS, TypeDeclPtr decl, RefMatters refMatters, ConstMatters constMatters, TemporaryMatters temporaryMatters, Context * context, LineInfoArg * at );
    void addModuleOption ( Module * mod, char * option, Type type, Context * context, LineInfoArg * at );
    TypeDeclPtr getUnderlyingValueType ( smart_ptr_raw<TypeDecl> type, Context * context, LineInfoArg * at );
    uint32_t getHandledTypeFieldOffset ( smart_ptr_raw<TypeAnnotation> type, char * name, Context * context, LineInfoArg * at );
    TypeInfo * getHandledTypeFieldType ( smart_ptr_raw<TypeAnnotation> annotation, char * name, Context * context, LineInfoArg * at );
    void addModuleRequrie ( Module * module, Module * reqModule, bool publ );

    template <>
    struct das_iterator <AnnotationArgumentList> : das_iterator<vector<AnnotationArgument>> {
        __forceinline das_iterator(AnnotationArgumentList & r)
            : das_iterator<vector<AnnotationArgument>>(r) {
        }
    };

    template <>
    struct das_iterator <const AnnotationArgumentList> : das_iterator<const vector<AnnotationArgument>> {
        __forceinline das_iterator(const AnnotationArgumentList & r)
            : das_iterator<const vector<AnnotationArgument>>(r) {
        }
    };
}

