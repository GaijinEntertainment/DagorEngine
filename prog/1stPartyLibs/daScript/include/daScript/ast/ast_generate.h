#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // ExprVar, where name matches
    bool isExpressionVariable(const ExpressionPtr & expr, const string & name);         // only var
    bool isExpressionVariableDeref(const ExpressionPtr & expr, const string & name);    // r2v(var) or var
    // ExprConstPtr, where value is nullptr
    bool isExpressionNull(const ExpressionPtr & expr);

    // check if block is inferred enough to be promoted to lambda\local_function, return uninferred value or nullptr
    TypeDecl *isFullyInferredBlock ( ExprBlock * block );

    // make sure generated code contains line information etc
    void verifyGenerated ( const ExpressionPtr & expr );

    // make sure fake context and fake line info are pre-assigned
    void assignDefaultArguments ( Function * func );

    // puts all expression's subexpressions at new location
    ExpressionPtr forceAt ( const ExpressionPtr & expr, const LineInfo & at );
    FunctionPtr forceAtFunction ( const FunctionPtr & func, const LineInfo & at );

    // change generated flag for all subexpressions and variables
    ExpressionPtr forceGenerated ( const ExpressionPtr & expr, bool setGenerated );
    FunctionPtr forceGeneratedFunction ( const FunctionPtr & expr, bool setGenerated );

    // gives combined region for all subexpressions
    LineInfo encloseAt ( const ExpressionPtr & expr );

    // replaces all occurrences of block argument name
    void renameBlockArgument ( ExprBlock * block, const string & name, const string & newName );

    /*
        $(self:MKS)
            ...
    or
        $(self:MKS[])
            ...
    */
    struct ExprMakeStruct;
    ExpressionPtr makeStructWhereBlock ( ExprMakeStruct * mks );

    /*
        self.decl := decl.value or self[index].decl = decl.value
    */
    struct MakeFieldDecl;
    ExpressionPtr convertToCloneExpr ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool ignoreCaptureConst = false );


    /*
        def foo ( var self : Foo; ... )
            with self
                ...
    */
    DAS_API void modifyToClassMember ( Function * func, Structure * baseClass, bool isExplicit, bool isConstant );

    /*
        def Foo ( ... )
            var self = [[Foo()]]
            Foo`Foo(self,...)
            return self
    */
    DAS_API FunctionPtr makeClassConstructor ( Structure * baseClass, Function * method );

    /*
        __rtti : void? = typeinfo(rtti_classinfo type<Foo>)
    */
    DAS_API void makeClassRtti ( Structure * baseClass );

    /*
        override def __finalize()
            delete self
    */
    DAS_API FunctionPtr makeClassFinalize ( Structure * baseClass );

    /*
     def STRUCT_NAME
        return [[STRUCT_NAME field1=init1, field2=init2, ...]]
     */
    DAS_API FunctionPtr makeConstructor ( Structure * str, bool isPrivate );

    /*
     def clone(var a:STRUCT_NAME; b:STRUCT_NAME)
        a.f1 := b.f1
        a.f2 := b.f2
        ...
    */
    DAS_API FunctionPtr makeClone ( Structure * str );

    /*
     def clone(var a:tuple<...>; var b:tuple<...>)
      a._0 := b._0
      a._1 := b._1
      ...
    */
    DAS_API FunctionPtr makeCloneTuple(const LineInfo & at, const TypeDeclPtr & tupleType, bool fromConst);

    /*
     def clone(var a:tuple<...>; var b:tuple<...>)
      if b is _0
        a._0 := b._0
      else if b is _1
        a._1 := b._1
      ...
    */
    DAS_API FunctionPtr makeCloneVariant(const LineInfo & at, const TypeDeclPtr & variantType, bool fromConst);

    /*
        delete var
     */
    DAS_API ExpressionPtr makeDelete ( const VariablePtr & var );

    /*
        a->b(args) is short for invoke(a.b, a, args)
     */
    struct ExprInvoke;
    DAS_API ExprInvoke * makeInvokeMethod ( const LineInfo & at, Expression * a, const string & b );

    /*
        this is short for invoke(type<callStruct>.b, a, args)
    */
    DAS_API ExprInvoke * makeInvokeMethod ( const LineInfo & at, Structure * callStruct, Expression * a, const string & b );

    /*
     pointer finalizer, i.e.
      def finalize(var THIS:PtrType&)
        if THIS != null
            delete *THIS
            delete native THIS
            THIS = null
     */
    DAS_API FunctionPtr generatePointerFinalizer ( const TypeDeclPtr & ptrType, const LineInfo & at );

    /*
     structure finalizer, i.e.
      def __lambda_finalizer_at_line_xxx(var THIS:StructureType)
         with THIS
            delete field1
            delete field2
            memzero(THIS)
     */
    DAS_API FunctionPtr generateStructureFinalizer ( const StructurePtr & ls );

    /*
     tuple finalizer, i.e.
      def __lambda_finalizer_at_line_xxx(var THIS:tuple<...>)
        delete _0
        delete _1
        memzero(THIS)
     */
    DAS_API FunctionPtr generateTupleFinalizer(const LineInfo & at, const TypeDeclPtr & tupleType);

    /*
     variant finalizer, i.e.
      def __lambda_finalizer_at_line_xxx(var THIS:tuple<...>)
        if THIS is _0
            delete _0
        elif THIS is _1
            delete _1
        ....
        memzero(THIS)
     */
    DAS_API FunctionPtr generateVariantFinalizer(const LineInfo & at, const TypeDeclPtr & variantType);

    /*
     variant finalizer, i.e.
      def clone(dest:smart_ptr<...>; src : any-ptr<...> )
        smart_ptr_clone(dest, src)
     */
    DAS_API FunctionPtr makeCloneSmartPtr ( const LineInfo & at, const TypeDeclPtr & left, const TypeDeclPtr & right );

    /*
     struct __lambda_at_line_xxx
         __lambda = @lambda_fn
        (yield : int)
        capture_field_1
        capture_field_2
     */
    DAS_API StructurePtr generateLambdaStruct ( const string & lambdaName, ExprBlock * block,
                                       const safe_var_set & capt, const vector<CaptureEntry> & capture, bool needYield = false );

    /*
        lambda function, i.e.
         def __lambda_function_at_line_xxx(THIS:__lambda_at_line_xxx;...block_args...)
            with THIS
                ...block_body...    // with finally section removed
     */
    enum {
        generator_needYield = (1<<0),
        generator_jit = (1<<1),
        generator_nojit = (1<<2)
    };
    DAS_API FunctionPtr generateLambdaFunction ( const string & lambdaName, ExprBlock * block,
                                        const StructurePtr & ls, const safe_var_set & capt,
                                        const vector<CaptureEntry> & capture, uint32_t genFlags, Program * thisProgram );

    /*
        local function, i.e.
         def __localfunction_function_at_line_xxx(...block_args...)
     */
    DAS_API FunctionPtr generateLocalFunction ( const string & lambdaName, ExprBlock * block );

    /*
        lambda finalizer, i.e.
         def __lambda_finalizer_at_line_xxx(THIS:__lambda_at_line_xxx)
            with THIS
                ...block_finally...
     */
    DAS_API FunctionPtr generateLambdaFinalizer ( const string & lambdaName, ExprBlock * block,
                                         const StructurePtr & ls, Program * thisProgram );

    /*
         [[__lambda_at_line_xxx
            THIS=@__lambda_function_at_line_xxx;
            FINALIZER=@__lambda_finalier_at_line_xx;
            _ba1=ba1; ba2=ba2; ... ]]
     */
    DAS_API ExpressionPtr generateLambdaMakeStruct ( const StructurePtr & ls, const FunctionPtr & lf, const FunctionPtr & lff,
                                            const safe_var_set & capt, const vector<CaptureEntry> & capture, const LineInfo & at,
                                            const LineInfo & captureAt, Program * thisProgram );

    /*
         array comprehension [{ for x in src; x_expr; where x_expr }]
         invoke() <| $()
             let temp : Array<expr->subexpr->type>
             for .....
                 if where ....
                     push(temp, subexpr)
             return temp
    */
    struct ExprArrayComprehension;
    DAS_API ExpressionPtr generateComprehension ( ExprArrayComprehension * expr, bool tableSyntax );

    /*
         array comprehension [[ for x in src; x_expr; where x_expr ]]
         generate() <| $()
             for .....
                 if where ....
                     yield subexpr
             return false
    */
    DAS_API ExpressionPtr generateComprehensionIterator ( ExprArrayComprehension * expr );

    /*
        replace reference with pointer
            var blah & = ....
            blah
        onto
            ...
            deref(blah)
     */
    DAS_API void replaceRef2Ptr ( const ExpressionPtr & expr, const string & name );

    /*
        give variables in the scope of 'expr' block unique names
        only for the top-level block
     */
    DAS_API void giveBlockVariablesUniqueNames  ( const ExpressionPtr & expr );

    /*
        replace break and continue of a particular loop
        with 'goto label bg' and 'goto label cg' accordingly
     */
    DAS_API void replaceBreakAndContinue ( Expression * expr, int32_t bg, int32_t cg );

    /*
        replace
            yield A
        with
            result = A
            __yield = X
            return true
            label X
     */
    struct ExprYield;
    DAS_API ExpressionPtr generateYield( ExprYield * expr, const FunctionPtr & func );

    /*
        replace
            let a = b
            let c
        with
            this.a = b
            memzero(c)
        if variable is & - swap to pointer
     */
    struct ExprLet;
    DAS_API ExpressionPtr replaceGeneratorLet ( ExprLet * expr, const FunctionPtr & func, ExprBlock * scope );

    /*
        replace
            if cond
                if_true
            else
                if_false
        with
            if ( !cond ) goto else_label;
            if_true;
            goto end_label;
            else_label:
            if_false;
            end_label:

        replace
            if cond
                if_true
        with
            if ( !cond ) goto end_label;
            if_true;
            end_label:

     */
    struct ExprIfThenElse;
    DAS_API ExpressionPtr replaceGeneratorIfThenElse ( ExprIfThenElse * expr, const FunctionPtr & func );

    /*
    replace
        while cond
            body
        finally
            finally
    with
        label beginloop                 continue -> goto beginloop
        if ! cond goto endloop          break -> goto endloop
        body
        goto beginloop
        label endloop
        finally
    */
    struct ExprWhile;
    DAS_API ExpressionPtr replaceGeneratorWhile ( ExprWhile * expr, const FunctionPtr & func );

    /*
    replace
        for it0.. in src0..
            body
         finally
             finally
        with
            loop = true                                             continue -> goto midloop
            loop &= _builtin_iterator_first(it0,src0)               break -> goto endloop
            label beginloop
            if ! loop goto endloop
            body
            label midloop
            loop &= _builtin_iterator_next(it0,src0)
            goto beginloop
            label endloop
            finally
            _builtin_itersator_close(it0,src0) ...

     */
    struct ExprFor;
    DAS_API ExpressionPtr replaceGeneratorFor ( ExprFor * expr, const FunctionPtr & func );
}

