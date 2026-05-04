#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {
    DAS_API char * ast_describe_typedecl ( smart_ptr_raw<TypeDecl> t, bool d_extra, bool d_contracts, bool d_module, Context * context, LineInfoArg * at );
    DAS_API char * ast_describe_typedecl_cpp ( smart_ptr_raw<TypeDecl> t, bool d_substitureRef, bool d_skipRef, bool d_skipConst, bool d_redundantConst, bool d_ChooseSmartPtr, Context * context, LineInfoArg * at );
    DAS_API char * ast_describe_expression ( smart_ptr_raw<Expression> t, Context * context, LineInfoArg * at );
    DAS_API char * ast_describe_function ( smart_ptr_raw<Function> t, Context * context, LineInfoArg * at );
    DAS_API char * ast_das_to_string ( Type bt, Context * context, LineInfoArg * at );
    DAS_API char * ast_find_bitfield_name ( smart_ptr_raw<TypeDecl> bft, Bitfield value, Context * context, LineInfoArg * at );
    DAS_API char * ast_find_enum_name ( Enumeration * enu, int64_t value, Context * context, LineInfoArg * at );
    DAS_API int64_t ast_find_enum_value ( EnumerationPtr enu, const char * value );
    DAS_API int64_t ast_find_enum_value_ex ( Enumeration * enu, const char * value );

    DAS_API int32_t any_array_size ( void * _arr );
    DAS_API int32_t any_table_size ( void * _tab );
    DAS_API void any_array_foreach ( void * _arr, int stride, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    DAS_API void any_table_foreach ( void * _tab, int keyStride, int valueStride, const TBlock<void,void *,void *> & blk, Context * context, LineInfoArg * at );

    DAS_API int32_t get_variant_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );
    DAS_API int32_t get_tuple_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );

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

    DAS_API Module * compileModule ( Context * context, LineInfoArg * at );
    DAS_API smart_ptr_raw<Program> compileProgram ( Context * context, LineInfoArg * at );


    DAS_API void aotSuffix( StructureAnnotation *structure, StructurePtr st, const AnnotationArgumentList & args, StringBuilderWriter *writer, Context * context, LineInfoArg * at);
    DAS_API void aotMacroPrefix( TypeInfoMacro *macro, StringBuilderWriter *ss, ExpressionPtr expr);
    DAS_API void aotMacroSuffix( TypeInfoMacro *macro, StringBuilderWriter *ss, ExpressionPtr expr);
    DAS_API const char *getAotName(Function* func, ExprCallFunc *call, Context * context, LineInfoArg * at );
    DAS_API void aotBody( StructureAnnotation *structure, StructurePtr st, const AnnotationArgumentList & args, StringBuilderWriter *writer, Context * context, LineInfoArg * at);
    DAS_API void aotPreVisitGetFieldPtr(TypeAnnotation *ann, StringBuilderWriter *ss, const char *name, Context * context, LineInfoArg * at);
    DAS_API void aotPreVisitGetField(TypeAnnotation *ann, StringBuilderWriter *ss, const char *name, Context * context, LineInfoArg * at);
    DAS_API void aotVisitGetField(TypeAnnotation *ann, StringBuilderWriter *ss, const char *name, Context * context, LineInfoArg * at);
    DAS_API bool aotNeedTypeInfo(const TypeInfoMacro *macro, ExpressionPtr expr);
    DAS_API void aotVisitGetFieldPtr( TypeAnnotation* ann, StringBuilderWriter *ss, const char *name, Context * context, LineInfoArg * at );
    DAS_API const char * getAotArgumentSuffix(Function* func, ExprCallFunc * call, int argIndex, Context * context, LineInfoArg * at );
    DAS_API const char * getAotArgumentPrefix(Function* func, ExprCallFunc * call, int argIndex, Context * context, LineInfoArg * at );
    DAS_API bool isAstSameType(smart_ptr<TypeDecl> argType, smart_ptr<TypeDecl> passType, bool refMatters,
                       bool constMatters,
                       bool temporaryMatters,
                       bool allowSubstitute, Context * context, LineInfoArg * at );
    DAS_API void aotFuncPrefix(FunctionAnnotation* ann, StringBuilderWriter * stg, ExprCallFunc *call, Context * context, LineInfoArg * at );
    DAS_API void aotStructPrefix(StructureAnnotation* ann, Structure *structure, const AnnotationArgumentList &args,
                         StringBuilderWriter * stg, Context * context, LineInfoArg * at );
    DAS_API const char * stringBuilderStr(StringBuilderWriter *ss, Context * context, LineInfoArg * at);
    DAS_API void stringBuilderClear(StringBuilderWriter *ss);

    DAS_API const Structure * findFieldParent( smart_ptr_raw<Structure> structure, const char *name, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr makeBlockType(ExprBlock *blk);
    // Note: it will be removed once DebugInfoHelper rewritten in das

    DAS_API TypeInfo * makeTypeInfo ( smart_ptr<DebugInfoHelper> helper, TypeInfo * info, const TypeDeclPtr & type );
    DAS_API VarInfo * makeVariableDebugInfo ( smart_ptr<DebugInfoHelper> helper, Variable *var );
    DAS_API VarInfo * makeStructVariableDebugInfo ( smart_ptr<DebugInfoHelper> helper, const Structure * st, const Structure::FieldDeclaration * var );
    DAS_API StructInfo * makeStructureDebugInfo ( smart_ptr<DebugInfoHelper> helper, const Structure * st );
    DAS_API FuncInfo * makeFunctionDebugInfo ( smart_ptr<DebugInfoHelper> helper, const Function * fn );
    DAS_API EnumInfo * makeEnumDebugInfo ( smart_ptr<DebugInfoHelper> helper, const Enumeration * en );
    DAS_API FuncInfo * makeInvokeableTypeDebugInfo ( smart_ptr<DebugInfoHelper> helper, TypeDeclPtr blk, const LineInfo & at );

    template <typename T>
    using DebugBlockT = TBlock<void,const char *, T>;

    DAS_API void debug_helper_iter_structs(smart_ptr<DebugInfoHelper> helper, const DebugBlockT<StructInfo*> & block, Context * context, LineInfoArg * at);
    DAS_API void debug_helper_iter_types(smart_ptr<DebugInfoHelper> helper, const DebugBlockT<TypeInfo*> & block, Context * context, LineInfoArg * at);
    DAS_API void debug_helper_iter_vars(smart_ptr<DebugInfoHelper> helper, const DebugBlockT<VarInfo*> & block, Context * context, LineInfoArg * at);
    DAS_API void debug_helper_iter_funcs(smart_ptr<DebugInfoHelper> helper, const DebugBlockT<FuncInfo*> & block, Context * context, LineInfoArg * at);
    DAS_API void debug_helper_iter_enums(smart_ptr<DebugInfoHelper> helper, const DebugBlockT<EnumInfo*> & block, Context * context, LineInfoArg * at);
    DAS_API const char *debug_helper_find_type_cppname(const smart_ptr<DebugInfoHelper> &helper, TypeInfo *info, Context * context, LineInfoArg * at);
    DAS_API const char *debug_helper_find_struct_cppname(const smart_ptr<DebugInfoHelper> &helper, StructInfo *info, Context * context, LineInfoArg * at);
    DAS_API bool macro_aot_infix(TypeInfoMacro *macro, StringBuilderWriter *ss, ExpressionPtr expr);
    DAS_API FileInfo *clone_file_info(const char *name, int tabSize, Context * context, LineInfoArg * at);
    DAS_API void for_each_module_function(Module *module, const TBlock<void,FunctionPtr> &blk, Context * context, LineInfoArg * at);
    DAS_API uint64_t getInitSemanticHashWithDep(ProgramPtr program, uint64_t semH);
    DAS_API uint64_t getFunctionHashById(Function *fun, int id, void * pctx, Context * context, LineInfoArg * at);
    DAS_API bool modAotRequire(Module *mod, StringBuilderWriter *ss, Context * context, LineInfoArg * at);

    #include "daScript/builtin/ast_gen.inc"

    template <typename TT, typename QQ>
    inline smart_ptr_raw<TT> return_smart ( smart_ptr<TT> & ptr, const QQ * p ) {
        if ( ptr.get() == p ) {
            return ptr.orphan();
        } else {
            return ptr;
        }
    }

    DAS_API void runMacroFunction ( Context * context, const string & message, const callable<void()> & subexpr );

    class DAS_API VisitorAdapter : public Visitor, AstVisitor_Adapter {
    public:
        VisitorAdapter ( char * pClass, const StructInfo * info, Context * ctx );
    public:
        // what do we visit
        virtual bool canVisitStructure ( Structure * st ) override;
        virtual bool canVisitGlobalVariable ( Variable * fun ) override;
        virtual bool canVisitFunction ( Function * fun ) override;
        virtual bool canVisitEnumeration ( Enumeration * en ) override;
        /*
        // TODO: implement on daslang side
        virtual bool canVisitStructureFieldInit ( Structure * var ) override {
            if ( auto fnCanVisit = get_canVisitStructureFieldInit(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "canVisitStructureFieldInit", [&]() {
                    result = invoke_canVisitStructureFieldInit(context,fnCanVisit,classPtr,var);
                });
                return result;
            } else {
                return true;
            }
        }
        */

        virtual bool canVisitExpr ( ExprTypeInfo * expr, Expression *subexpr ) override;

        /*
        // TODO: implement
        virtual bool canVisitIfSubexpr ( ExprIfThenElse * ) override {
            if ( auto fnCanVisit = get_canVisitIfSubexpr(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "canVisitIfSubexpr", [&]() {
                    result = invoke_canVisitIfSubexpr(context,fnCanVisit,classPtr);
                });
                return result;
            } else {
                return true;
            }
        }
        */
        virtual bool canVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) override;
        virtual bool canVisitMakeStructureBody ( ExprMakeStruct * expr ) override;
        virtual bool canVisitArgumentInit ( Function * fun, const VariablePtr & var, Expression * init ) override;
        /*
        TODO: implement
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override {
            if ( auto fnCanVisit = get_canVisitQuoteSubexpression(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "canVisitQuoteSubexpression", [&]() {
                    result = invoke_canVisitQuoteSubexpression(context,fnCanVisit,classPtr);
                });
                return result;
            } else {
                return true;
            }
        }
        */
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * expr ) override;
        virtual bool canVisitMakeBlockBody ( ExprMakeBlock * expr ) override;
        virtual bool canVisitCall ( ExprCall * expr ) override;
        virtual bool canVisitNamedCall ( ExprNamedCall * expr ) override;
        virtual bool canVisitLooksLikeCall ( ExprLooksLikeCall * expr ) override;
        // WHOLE PROGRAM
        virtual void preVisitProgram ( Program * prog ) override;
        virtual void visitProgram ( Program * prog ) override;
        // EACH MODULE
        virtual void preVisitModule ( Module * mod ) override;
        virtual void visitModule ( Module * mod ) override;
        // TYPE
        virtual void preVisit ( TypeDecl * td ) override;
        virtual TypeDeclPtr visit ( TypeDecl * td ) override;
        // ALIAS
        virtual void preVisitAlias ( TypeDecl * td, const string & name ) override;
        virtual TypeDeclPtr visitAlias ( TypeDecl * td, const string & name ) override;
        // ENUMERATOIN
        virtual void preVisit ( Enumeration * enu ) override;
        virtual void preVisitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override;
        virtual ExpressionPtr visitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override;
        virtual EnumerationPtr visit ( Enumeration * enu ) override;
        // STRUCTURE
        virtual void preVisit ( Structure * var ) override;
        virtual void preVisitStructureAlias ( Structure * var, const string & name, TypeDecl * at ) override;
        virtual TypeDeclPtr visitStructureAlias ( Structure * var, const string & name, TypeDecl * at ) override;
        virtual void preVisitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override;
        virtual bool canVisitStructureFieldInit ( Structure * var ) override;
        virtual void visitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override;
        virtual StructurePtr visit ( Structure * var ) override;
        // REAL THINGS (AFTER STRUCTS AND ENUMS)
        virtual void preVisitProgramBody ( Program * prog, Module * mod ) override;
        // FUNCTON
        virtual void preVisit ( Function * that ) override;
        virtual FunctionPtr visit ( Function * that ) override;
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override;
        virtual VariablePtr visitArgument ( Function * fn, const VariablePtr & that, bool lastArg ) override;
        virtual void preVisitArgumentInit ( Function * fn, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitArgumentInit ( Function * fn, const VariablePtr & var, Expression * that ) override;
        virtual void preVisitFunctionBody ( Function * fn, Expression * that ) override;
        virtual ExpressionPtr visitFunctionBody ( Function * fn, Expression * that ) override;
        // ANY
        virtual void preVisitExpression ( Expression * expr ) override;
        virtual ExpressionPtr visitExpression ( Expression * expr ) override;
        // BLOCK
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override;
        virtual VariablePtr visitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override;
        virtual void preVisitBlockArgumentInit ( ExprBlock * block, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitBlockArgumentInit ( ExprBlock * block, const VariablePtr & var, Expression * that ) override;
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override;
        virtual ExpressionPtr visitBlockExpression (  ExprBlock * block, Expression * expr ) override;
        virtual void preVisitBlockFinal ( ExprBlock * block ) override;
        virtual void visitBlockFinal ( ExprBlock * block ) override;
        virtual void preVisitBlockFinalExpression ( ExprBlock * block, Expression * expr ) override;
        virtual ExpressionPtr visitBlockFinalExpression (  ExprBlock * block, Expression * expr ) override;
        // LET
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override;
        virtual VariablePtr visitLet ( ExprLet * let, const VariablePtr & var, bool last ) override;
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * init ) override;
        virtual ExpressionPtr visitLetInit ( ExprLet * let, const VariablePtr & var, Expression * that ) override;
        // GLOBAL LET
        virtual void preVisitGlobalLetBody ( Program * prog ) override;
        virtual void visitGlobalLetBody ( Program * prog ) override;
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override;
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override;
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * that ) override;
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & var, Expression * that ) override;
        // STRING BUILDER
        virtual void preVisit ( ExprStringBuilder * expr ) override;
        virtual void preVisitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override;
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override;
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override;
        // NEW
        virtual void preVisitNewArg ( ExprNew * call, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitNewArg ( ExprNew * call, Expression * arg , bool last ) override;
        // NAMED CALL
        virtual void preVisitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg, bool last ) override;
        virtual MakeFieldDeclPtr visitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg , bool last ) override;
        // CALL
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg , bool last ) override;
        // CALL
        virtual bool canVisitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) override;
        virtual void preVisitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) override;
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg , bool last ) override;
        // NULL COAELESCING
        virtual void preVisitNullCoaelescingDefault ( ExprNullCoalescing * expr, Expression * def ) override;
        // TAG
        virtual void preVisitTagValue ( ExprTag * expr, Expression * val ) override;
        // AT
        virtual void preVisitAtIndex ( ExprAt * expr, Expression * index ) override;
        // SAFE AT
        virtual void preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index ) override;
        // IS
        virtual void preVisitType ( ExprIs * expr, TypeDecl * val ) override;
        // OP2
        virtual void preVisitRight ( ExprOp2 * expr, Expression * right ) override;
        // OP3
        virtual void preVisitLeft  ( ExprOp3 * expr, Expression * left ) override;
        virtual void preVisitRight ( ExprOp3 * expr, Expression * right ) override;
        // COPY
        virtual bool isRightFirst ( ExprCopy * expr ) override;
        virtual void preVisitRight ( ExprCopy * expr, Expression * right ) override;
        // MOVE
        virtual bool isRightFirst ( ExprMove * expr ) override;
        virtual void preVisitRight ( ExprMove * expr, Expression * right ) override;
        // CLONE
        virtual bool isRightFirst ( ExprClone * expr ) override;
        virtual void preVisitRight ( ExprClone * expr, Expression * right ) override;
        // WITH
        virtual void preVisitWithBody ( ExprWith * expr, Expression * body ) override;
        // WHILE
        virtual void preVisitWhileBody ( ExprWhile * expr, Expression * right ) override;
        // TRY-CATCH
        virtual void preVisitCatch ( ExprTryCatch * expr, Expression * that ) override;
        // IF-THEN-ELSE
        virtual void preVisitIfBlock ( ExprIfThenElse * expr, Expression * that ) override;
        virtual void preVisitElseBlock ( ExprIfThenElse * expr, Expression * that ) override;
        // FOR
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override;
        virtual VariablePtr visitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override;
        virtual void preVisitForStack ( ExprFor * expr ) override;
        virtual void preVisitForBody ( ExprFor * expr, Expression * /*that*/ ) override;
        virtual void preVisitForSource ( ExprFor * expr, Expression * that, bool last ) override;
        virtual ExpressionPtr visitForSource ( ExprFor * expr, Expression * that , bool last ) override;
        // MAKE VARIANT
        virtual void preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool lastField ) override;
        virtual MakeFieldDeclPtr visitMakeVariantField(ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool lastField) override;
// MAKE STRUCTURE
        virtual void preVisitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool lastIndex ) override;
        virtual void visitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool lastField ) override;
        virtual void preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool lastField ) override;
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool lastField ) override;
        virtual void preVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) override;
        virtual ExpressionPtr visitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) override;
        // MAKE ARRAY
        virtual void preVisitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastIndex ) override;
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastField ) override;
        // MAKE TUPLE
        virtual void preVisitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastIndex ) override;
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) override;
        // ARRAY COMPREHENSION
        virtual void preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr ) override;
        virtual void preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where ) override;
        // DELETE
        virtual void preVisitDeleteSizeExpression ( ExprDelete * expr, Expression * that ) override {
            if ( auto fnPreVisit = get_preVisitExprDeleteSizeExpression(classPtr) ) {
                runMacroFunction(context, "preVisitDeleteSizeExpression", [&]() {
                    invoke_preVisitExprDeleteSizeExpression(context,fnPreVisit,classPtr,expr,that);
                });
            }
        }

#define VISIT_EXPR(ExprType) \
       virtual void preVisit ( ExprType * that ) override { \
            preVisitExpression(that); \
            if ( auto fnPreVisit = get_preVisit##ExprType(classPtr) ) { \
                runMacroFunction(context, "preVisit", [&]() { \
                    invoke_preVisit##ExprType(context,fnPreVisit,classPtr,that); \
                }); \
            } \
        } \
        virtual ExpressionPtr visit ( ExprType * that ) override { \
            visitExpression(that); \
            if ( auto fnVisit = get_visit##ExprType(classPtr) ) { \
                ExpressionPtr result; \
                runMacroFunction(context, "visit", [&]() { \
                    result = invoke_visit##ExprType(context,fnVisit,classPtr,that); \
                }); \
                return return_smart(result,that); \
            } else { \
                return that; \
            } \
        }
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

    protected:
        void *      classPtr;
        Context *   context;
    };

    DAS_API DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context );
    DAS_API Module * thisModule ( Context * context, LineInfoArg * lineinfo );
    DAS_API smart_ptr_raw<Program> thisProgram ( Context * context );
    DAS_API void astVisit ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    DAS_API void astVisitModule ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter,
                      Module* module, Context * context, LineInfoArg * line_info );
    DAS_API void astVisitModulesInOrder ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    DAS_API void astVisitFunction ( smart_ptr_raw<Function> func, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    DAS_API smart_ptr_raw<Expression> astVisitExpression ( smart_ptr_raw<Expression> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    DAS_API smart_ptr_raw<TypeDecl> astVisitTypeDecl ( smart_ptr_raw<TypeDecl> type, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    DAS_API void astVisitBlockFinally ( smart_ptr_raw<ExprBlock> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    DAS_API PassMacroPtr makePassMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API smart_ptr<VisitorAdapter> makeVisitor ( const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleInferMacro ( Module * module, PassMacroPtr & _newM, Context * );
    DAS_API void addModuleInferDirtyMacro ( Module * module, PassMacroPtr & newM, Context * context );
    DAS_API void addModuleLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    DAS_API void addModuleGlobalLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    DAS_API void addModuleOptimizationMacro ( Module * module, PassMacroPtr & _newM, Context * );
    DAS_API VariantMacroPtr makeVariantMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleVariantMacro ( Module * module, VariantMacroPtr & newM, Context * context );
    DAS_API ForLoopMacroPtr makeForLoopMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleForLoopMacro ( Module * module, ForLoopMacroPtr & _newM, Context * );
    DAS_API CaptureMacroPtr makeCaptureMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleCaptureMacro ( Module * module, CaptureMacroPtr & _newM, Context * );
    DAS_API TypeMacroPtr makeTypeMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleTypeMacro ( Module * module, TypeMacroPtr & _newM, Context *, LineInfoArg * );
    DAS_API SimulateMacroPtr makeSimulateMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleSimulateMacro ( Module * module, SimulateMacroPtr & _newM, Context * );
    DAS_API void addModuleFunctionAnnotation ( Module * module, FunctionAnnotationPtr & ann, Context * context, LineInfoArg * at );
    DAS_API FunctionAnnotationPtr makeFunctionAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    DAS_API StructureAnnotationPtr makeStructureAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    DAS_API EnumerationAnnotationPtr makeEnumerationAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleStructureAnnotation ( Module * module, StructureAnnotationPtr & ann, Context * context, LineInfoArg * at );
    DAS_API void addStructureStructureAnnotation ( smart_ptr_raw<Structure> st, StructureAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    DAS_API void addModuleEnumerationAnnotation ( Module * module, EnumerationAnnotationPtr & ann, Context * context, LineInfoArg * at );
    DAS_API int addEnumerationEntry ( smart_ptr<Enumeration> enu, const char* name );
    DAS_API void forEachFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    DAS_API void forEachGenericFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    DAS_API bool addModuleFunction ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    DAS_API bool addModuleGeneric ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    DAS_API bool addModuleVariable ( Module * module, VariablePtr & var, Context * context, LineInfoArg * lineInfo );
    DAS_API bool addModuleKeyword ( Module * module, char * kwd, bool needOxfordComma, Context * context, LineInfoArg * lineInfo );
    DAS_API bool addModuleTypeFunction ( Module * module, char * kwd, Context * context, LineInfoArg * lineInfo );
    DAS_API VariablePtr findModuleVariable ( Module * module, const char * name );
    DAS_API bool removeModuleStructure ( Module * module, StructurePtr & _stru );
    DAS_API bool addModuleStructure ( Module * module, StructurePtr & stru );
    DAS_API bool addModuleAlias ( Module * module, TypeDeclPtr & _ptr );
    DAS_API void ast_error ( ProgramPtr prog, const LineInfo & at, const char * message, Context * context, LineInfoArg * lineInfo );
    DAS_API void addModuleReaderMacro ( Module * module, ReaderMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    DAS_API ReaderMacroPtr makeReaderMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API CommentReaderPtr makeCommentReader ( const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleCommentReader ( Module * module, CommentReaderPtr & _newM, Context * context, LineInfoArg * lineInfo );
    DAS_API void addModuleCallMacro ( Module * module, CallMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    DAS_API CallMacroPtr makeCallMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API TypeInfoMacroPtr makeTypeInfoMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    DAS_API void addModuleTypeInfoMacro ( Module * module, TypeInfoMacroPtr & _newM, Context * context, LineInfoArg * at );
    DAS_API void addFunctionFunctionAnnotation(smart_ptr_raw<Function> func, FunctionAnnotationPtr & ann, Context* context, LineInfoArg* at);
    DAS_API void addAndApplyFunctionAnnotation ( smart_ptr_raw<Function> func, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    DAS_API void addBlockBlockAnnotation ( smart_ptr_raw<ExprBlock> block, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    DAS_API void addAndApplyBlockAnnotation ( smart_ptr_raw<ExprBlock> blk, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    DAS_API void addAndApplyStructAnnotation ( smart_ptr_raw<Structure> st, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    DAS_API void visitEnumeration ( ProgramPtr program, smart_ptr_raw<Enumeration> enumeration, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    DAS_API void visitStructure ( ProgramPtr program, smart_ptr_raw<Structure> structure, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    __forceinline ExpressionPtr clone_expression ( ExpressionPtr value ) { return value ?value->clone() : nullptr; }
    __forceinline FunctionPtr clone_function ( FunctionPtr value ) { return value ? value->clone() : nullptr; }
    __forceinline TypeDeclPtr clone_type ( TypeDeclPtr value ) { return value ? make_smart<TypeDecl>(*value) : nullptr; }
    __forceinline StructurePtr clone_structure ( const Structure * value ) { return value ? value->clone() : nullptr; }
    __forceinline VariablePtr clone_variable ( VariablePtr value ) { return value ? value->clone() : nullptr; }
    DAS_API void forceAtRaw ( const smart_ptr_raw<Expression> & expr, const LineInfo & at );
    DAS_API void getAstContext ( smart_ptr_raw<Program> prog, smart_ptr_raw<Expression> expr, const TBlock<void,bool,AstContext> & block, Context * context, LineInfoArg * at );
    DAS_API char * get_mangled_name ( smart_ptr_raw<Function> func, Context * context, LineInfoArg * at );
    DAS_API char * get_mangled_name_t ( smart_ptr_raw<TypeDecl> typ, Context * context, LineInfoArg * at );
    DAS_API char * get_mangled_name_v ( smart_ptr_raw<Variable> var, Context * context, LineInfoArg * at );
    DAS_API char * get_mangled_name_b ( smart_ptr_raw<ExprBlock> expr, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr parseMangledNameFn ( const char * txt, ModuleGroup & lib, Module * thisModule, Context * context, LineInfoArg * at );
    DAS_API void notInferred ( Function * func, Context * context, LineInfoArg * at );
    DAS_API void collectDependencies ( FunctionPtr fun, const TBlock<void,TArray<Function *>,TArray<Variable *>> & block, Context * context, LineInfoArg * line );
    DAS_API bool isExprLikeCall ( const ExpressionPtr & expr );
    DAS_API bool isExprConst ( const ExpressionPtr & expr );
    DAS_API bool isTempType ( TypeDeclPtr ptr, bool refMatters );
    DAS_API float4 evalSingleExpression ( const ExpressionPtr & expr, bool & ok );
    DAS_API ExpressionPtr makeCall ( const LineInfo & at, const char * name );
    DAS_API bool builtin_isVisibleDirectly ( Module * from, Module * too );
    DAS_API bool builtin_hasField ( TypeDeclPtr ptr, const char * field, bool constant );
    DAS_API TypeDeclPtr builtin_fieldType ( TypeDeclPtr ptr, const char * field, bool constant );
    DAS_API Module * findRttiModule ( smart_ptr<Program> THAT_PROGRAM, const char * name, Context *, LineInfoArg *);
    DAS_API smart_ptr_raw<Annotation> module_find_annotation ( const Module* module, const char *name );
    DAS_API TypeAnnotation* module_find_type_annotation ( const Module* module, const char *name );
    DAS_API smart_ptr_raw<Function> findRttiFunction ( Module * mod, Func func, Context * context, LineInfoArg * line_info );
    DAS_API void for_each_module ( Program * prog, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_module_no_order ( Program * prog, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_typedef ( Module * mod, const TBlock<void,TTemporary<char *>,smart_ptr_raw<TypeDecl>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_enumeration ( Module * mod, const TBlock<void,smart_ptr_raw<Enumeration>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_structure ( Module * mod, const TBlock<void,smart_ptr_raw<Structure>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_generic ( Module * mod, const TBlock<void,smart_ptr_raw<Function>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_global ( Module * mod, const TBlock<void,smart_ptr_raw<Variable>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_annotation_ordered ( Module * mod, const TBlock<void,uint64_t, uint64_t> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_call_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_reader_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_variant_macro ( Module * mod, const TBlock<void,VariantMacroPtr> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_typeinfo_macro ( Module * mod, const TBlock<void,TypeInfoMacroPtr> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_typemacro ( Module * mod, const TBlock<void,TypeMacroPtr> & block, Context * context, LineInfoArg * at );
    DAS_API void for_each_for_loop_macro ( Module * mod, const TBlock<void,ForLoopMacroPtr> & block, Context * context, LineInfoArg * at );
    DAS_API Annotation * get_expression_annotation ( Expression * expr, Context * context, LineInfoArg * at );
    DAS_API Structure * find_unique_structure ( smart_ptr_raw<Program> prog, const char * name, Context * context, LineInfoArg * at );
    DAS_API Structure * module_find_structure ( const Module* module, const char * name, Context * context, LineInfoArg * at );
    DAS_API void get_use_global_variables ( smart_ptr_raw<Function> func, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at );
    DAS_API void get_use_functions ( smart_ptr_raw<Function> func, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at );
    DAS_API Structure::FieldDeclaration * ast_findStructureField ( Structure * structType, const char * field, Context * context, LineInfoArg * at );
    DAS_API int32_t ast_getTupleFieldOffset ( smart_ptr_raw<TypeDecl> ttype, int32_t field, Context * context, LineInfoArg * at );
    DAS_API void das_comp_log ( const char * text, Context * context, LineInfoArg * at );
    DAS_API TypeInfo * das_make_type_info_structure ( Context & ctx, TypeDeclPtr ptr, Context * context, LineInfoArg * at );
    DAS_API bool isSameAstType ( TypeDeclPtr THIS, TypeDeclPtr decl, RefMatters refMatters, ConstMatters constMatters, TemporaryMatters temporaryMatters, Context * context, LineInfoArg * at );
    DAS_API void builtin_structure_for_each_field ( const BasicStructureAnnotation & ann,
                                            const TBlock<void,char *,char*,smart_ptr_raw<TypeDecl>,uint32_t> & block, Context * context, LineInfoArg * at );
    DAS_API void addModuleOption ( Module * mod, char * option, Type type, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr getUnderlyingValueType ( smart_ptr_raw<TypeDecl> type, Context * context, LineInfoArg * at );
    DAS_API uint32_t getHandledTypeFieldOffset ( smart_ptr_raw<TypeAnnotation> type, char * name, Context * context, LineInfoArg * at );
    DAS_API void builtin_structure_for_each_field ( const BasicStructureAnnotation & ann,
                                        const TBlock<void,char *,char*,smart_ptr_raw<TypeDecl>,uint32_t> & block, Context * context, LineInfoArg * at );
    DAS_API TypeInfo * getHandledTypeFieldType ( smart_ptr_raw<TypeAnnotation> annotation, char * name, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr getHandledTypeFieldTypeDecl ( smart_ptr_raw<TypeAnnotation> annotation, char * name, bool isConst, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr getHandledTypeIndexTypeDecl ( TypeAnnotation *annotation, Expression *src, Expression *idx, Context * context, LineInfoArg * at );
    DAS_API void* getVectorPtrAtIndex(void* vec, TypeDecl *type, int idx, Context * context, LineInfoArg * at);
    DAS_API int32_t getVectorLength(void* vec, smart_ptr_raw<TypeDecl> type, Context * context, LineInfoArg * at);
    DAS_API bool addModuleRequire ( Module * module, Module * reqModule, bool publ );
    DAS_API void findMatchingVariable ( Program * program, Function * func, const char * _name, bool seePrivate,
        const TBlock<void,TTemporary<TArray<smart_ptr_raw<Variable>>>> & block, Context * context, LineInfoArg * arg );
    DAS_API Module * getCurrentSearchModule(Program * program, Function * func, const char * _moduleName);
    DAS_API bool canAccessGlobalVariable ( const VariablePtr & pVar, Module * mod, Module * thisMod );
    DAS_API TypeDeclPtr inferGenericTypeEx ( smart_ptr_raw<TypeDecl> type, smart_ptr_raw<TypeDecl> passType, bool topLevel, bool isPassType );
    DAS_API void updateAliasMapEx ( smart_ptr_raw<Program> program, smart_ptr_raw<TypeDecl> argType, smart_ptr_raw<TypeDecl> passType, Context * context, LineInfoArg * at );
    DAS_API void forceAtRaw ( const smart_ptr_raw<Expression> & expr, const LineInfo & at );
    DAS_API void forceAtFunctionRaw ( const smart_ptr_raw<Function> & func, const LineInfo & at );
    DAS_API void forceGeneratedRaw ( const smart_ptr_raw<Expression> & expr, bool setGenerated );
    DAS_API void forceGeneratedFunctionRaw ( const smart_ptr_raw<Function> & func, bool setGenerated );
    DAS_API bool add_structure_alias ( Structure * structure, char * name, const TypeDeclPtr & aliasType, Context * context, LineInfoArg * at );
    DAS_API void for_each_structure_alias ( Structure * structure, const TBlock<void,smart_ptr_raw<TypeDecl>> & block, Context * context, LineInfoArg * at );
    DAS_API TypeDeclPtr get_structure_alias ( Structure * structure, const char * aliasName, Context * context, LineInfoArg * at );
    DAS_API smart_ptr_raw<Function> findCompilingFunctionByMangledNameHash(char * module_name, uint64_t mnh, Context * context, LineInfoArg * at);

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

