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

namespace das {

#define FN_PREVISIT(WHAT)  fnPreVisit##WHAT
#define FN_VISIT(WHAT)      fnVisit##WHAT

#define IMPL_PREVISIT1(WHAT,WHATTYPE) \
    if ( FN_PREVISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>> \
            (context,nullptr,FN_PREVISIT(WHAT),classPtr,expr); \
    }

#define IMPL_PREVISIT2(WHAT,WHATTYPE,ARG1T,ARG1) \
    if ( FN_PREVISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T> \
            (context,nullptr,FN_PREVISIT(WHAT),classPtr,expr,ARG1); \
    }

#define IMPL_PREVISIT3(WHAT,WHATTYPE,ARG1T,ARG1,ARG2T,ARG2) \
    if ( FN_PREVISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T> \
            (context,nullptr,FN_PREVISIT(WHAT),classPtr,expr,ARG1,ARG2); \
    }

#define IMPL_PREVISIT4(WHAT,WHATTYPE,ARG1T,ARG1,ARG2T,ARG2,ARG3T,ARG3) \
    if ( FN_PREVISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T,ARG3T> \
            (context,nullptr,FN_PREVISIT(WHAT),classPtr,expr,ARG1,ARG2,ARG3); \
    }

#define IMPL_PREVISIT(WHAT) IMPL_PREVISIT1(WHAT,WHAT)

#define IMPL_VISIT_VOID1(WHAT,WHATTYPE) \
    if ( FN_VISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr); \
    }

#define IMPL_VISIT_VOID2(WHAT,WHATTYPE,ARG1T,ARG1) \
    if ( FN_VISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1); \
    }

#define IMPL_VISIT_VOID3(WHAT,WHATTYPE,ARG1T,ARG1,ARG2T,ARG2) \
    if ( FN_VISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1,ARG2); \
    }

#define IMPL_VISIT_VOID4(WHAT,WHATTYPE,ARG1T,ARG1,ARG2T,ARG2,ARG3T,ARG3) \
    if ( FN_VISIT(WHAT) ) { \
        das_invoke_function<void>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T,ARG3T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1,ARG2,ARG3); \
    }

#define IMPL_VISIT1(WHAT,WHATTYPE,RETTYPE,RETVALUE) \
    if ( FN_VISIT(WHAT) ) { \
        return das_invoke_function<smart_ptr_raw<RETTYPE>>::invoke<void *,smart_ptr_raw<WHATTYPE>> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr).marshal(RETVALUE); \
    } else { \
        return RETVALUE; \
    }

#define IMPL_VISIT2(WHAT,WHATTYPE,RETTYPE,RETVALUE,ARG1T,ARG1) \
    if ( FN_VISIT(WHAT) ) { \
        return das_invoke_function<smart_ptr_raw<RETTYPE>>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1).marshal(RETVALUE); \
    } else { \
        return RETVALUE; \
    }

#define IMPL_VISIT3(WHAT,WHATTYPE,RETTYPE,RETVALUE,ARG1T,ARG1,ARG2T,ARG2) \
    if ( FN_VISIT(WHAT) ) { \
        return das_invoke_function<smart_ptr_raw<RETTYPE>>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1,ARG2).marshal(RETVALUE); \
    } else { \
        return RETVALUE; \
    }

#define IMPL_VISIT4(WHAT,WHATTYPE,RETTYPE,RETVALUE,ARG1T,ARG1,ARG2T,ARG2,ARG3T,ARG3) \
    if ( FN_VISIT(WHAT) ) { \
        return das_invoke_function<smart_ptr_raw<RETTYPE>>::invoke<void *,smart_ptr_raw<WHATTYPE>,ARG1T,ARG2T,ARG3T> \
            (context,nullptr,FN_VISIT(WHAT),classPtr,expr,ARG1,ARG2,ARG3).marshal(RETVALUE); \
    } else { \
        return RETVALUE; \
    }


#define IMPL_VISIT(WHAT) IMPL_VISIT1(WHAT,WHAT,WHAT,expr)

#define IMPL_VISIT_VOID(WHAT) IMPL_VISIT_VOID1(WHAT,WHAT)

#define IMPL_ADAPT(WHAT) \
    FN_PREVISIT(WHAT) = adapt("preVisit" #WHAT,pClass,info); \
    FN_VISIT(WHAT) = adapt("visit" #WHAT,pClass,info);

#define IMPL_BIND_EXPR(WHAT) \
    void VisitorAdapter::preVisit ( WHAT * expr ) \
        { Visitor::preVisit(expr); IMPL_PREVISIT(WHAT); } \
    ExpressionPtr VisitorAdapter::visit ( WHAT * expr ) \
        { auto that = ([&]() -> ExpressionPtr {IMPL_VISIT(WHAT);})(); Visitor::visit(expr); return that; }

    VisitorAdapter::VisitorAdapter ( char * pClass, const StructInfo * info, Context * ctx ) {
        context = ctx;
        classPtr = pClass;
        // adapt
        IMPL_ADAPT(Program);
        FN_PREVISIT(ProgramBody) = adapt("preVisitProgramBody",pClass,info);
        IMPL_ADAPT(Module);
        IMPL_ADAPT(TypeDecl);
        IMPL_ADAPT(Expression);
        IMPL_ADAPT(Alias);
        fnCanVisitEnumeration = adapt("canVisitEnumeration",pClass,info);
        IMPL_ADAPT(Enumeration);
        IMPL_ADAPT(EnumerationValue);
        fnCanVisitStructure = adapt("canVisitStructure",pClass,info);
        IMPL_ADAPT(Structure);
        IMPL_ADAPT(StructureField);
        fnCanVisitFunction = adapt("canVisitFunction",pClass,info);
        fnCanVisitArgumentInit = adapt("canVisitFunctionArgumentInit",pClass,info);
        IMPL_ADAPT(Function);
        IMPL_ADAPT(FunctionArgument);
        IMPL_ADAPT(FunctionArgumentInit);
        IMPL_ADAPT(FunctionBody);
        IMPL_ADAPT(ExprBlock);
        IMPL_ADAPT(ExprBlockArgument);
        IMPL_ADAPT(ExprBlockArgumentInit);
        IMPL_ADAPT(ExprBlockExpression);
        IMPL_ADAPT(ExprBlockFinal);
        IMPL_ADAPT(ExprBlockFinalExpression);
        IMPL_ADAPT(ExprLet);
        IMPL_ADAPT(ExprLetVariable);
        IMPL_ADAPT(ExprLetVariableInit);
        fnCanVisitGlobalVariable = adapt("canVisitGlobalVariable",pClass,info);
        IMPL_ADAPT(GlobalLet);
        IMPL_ADAPT(GlobalLetVariable);
        IMPL_ADAPT(GlobalLetVariableInit);
        IMPL_ADAPT(ExprStringBuilder);
        IMPL_ADAPT(ExprStringBuilderElement);
        IMPL_ADAPT(ExprNew);
        IMPL_ADAPT(ExprNewArgument);
        IMPL_ADAPT(ExprNamedCall);
        IMPL_ADAPT(ExprNamedCallArgument);
        fnCanVisitCall = adapt("canVisitCall",pClass,info);
        IMPL_ADAPT(ExprCall);
        IMPL_ADAPT(ExprCallArgument);
        IMPL_ADAPT(ExprLooksLikeCall);
        IMPL_ADAPT(ExprLooksLikeCallArgument);
        IMPL_ADAPT(ExprNullCoalescing);
        FN_PREVISIT(ExprNullCoalescingDefault) = adapt("preVisitExprNullCoalescingDefault",pClass,info);
        IMPL_ADAPT(ExprAt);
        FN_PREVISIT(ExprAtIndex) = adapt("preVisitExprAtIndex",pClass,info);
        IMPL_ADAPT(ExprSafeAt);
        FN_PREVISIT(ExprSafeAtIndex) = adapt("preVisitExprSafeAtIndex",pClass,info);
        IMPL_ADAPT(ExprIs);
        FN_PREVISIT(ExprIsType) = adapt("preVisitExprIsType",pClass,info);
        IMPL_ADAPT(ExprOp2);
        FN_PREVISIT(ExprOp2Right) = adapt("preVisitExprOp2Right",pClass,info);
        IMPL_ADAPT(ExprOp3);
        FN_PREVISIT(ExprOp3Left) = adapt("preVisitExprOp3Left",pClass,info);
        FN_PREVISIT(ExprOp3Right) = adapt("preVisitExprOp3Right",pClass,info);
        IMPL_ADAPT(ExprCopy);
        FN_PREVISIT(ExprCopyRight) = adapt("preVisitExprCopyRight",pClass,info);
        IMPL_ADAPT(ExprMove);
        FN_PREVISIT(ExprMoveRight) = adapt("preVisitExprMoveRight",pClass,info);
        IMPL_ADAPT(ExprClone);
        FN_PREVISIT(ExprCloneRight) = adapt("preVisitExprCloneRight",pClass,info);
        fnCanVisitWithAliasSubexpression = adapt("canVisitWithAliasSubexpression",pClass,info);
        IMPL_ADAPT(ExprAssume);
        IMPL_ADAPT(ExprWith);
        FN_PREVISIT(ExprWithBody) = adapt("preVisitExprWithBody",pClass,info);
        IMPL_ADAPT(ExprWhile);
        FN_PREVISIT(ExprWhileBody) = adapt("preVisitExprWhileBody",pClass,info);
        IMPL_ADAPT(ExprTryCatch);
        FN_PREVISIT(ExprTryCatchCatch) = adapt("preVisitExprTryCatchCatch",pClass,info);
        IMPL_ADAPT(ExprIfThenElse);
        FN_PREVISIT(ExprIfThenElseIfBlock) = adapt("preVisitExprIfThenElseIfBlock",pClass,info);
        FN_PREVISIT(ExprIfThenElseElseBlock) = adapt("preVisitExprIfThenElseElseBlock",pClass,info);
        IMPL_ADAPT(ExprFor);
        IMPL_ADAPT(ExprForVariable);
        IMPL_ADAPT(ExprForSource);
        FN_PREVISIT(ExprForStack) = adapt("preVisitExprForStack",pClass,info);
        FN_PREVISIT(ExprForBody) = adapt("preVisitExprForBody",pClass,info);
        IMPL_ADAPT(ExprMakeVariant);
        IMPL_ADAPT(ExprMakeVariantField);
        fnCanVisitMakeStructBody = adapt("canVisitMakeStructBody",pClass,info);
        fnCanVisitMakeStructBlock = adapt("canVisitMakeStructBlock",pClass,info);
        IMPL_ADAPT(ExprMakeStruct);
        IMPL_ADAPT(ExprMakeStructIndex);
        IMPL_ADAPT(ExprMakeStructField);
        IMPL_ADAPT(ExprMakeArray);
        IMPL_ADAPT(ExprMakeArrayIndex);
        IMPL_ADAPT(ExprMakeTuple);
        IMPL_ADAPT(ExprMakeTupleIndex);
        IMPL_ADAPT(ExprArrayComprehension);
        FN_PREVISIT(ExprArrayComprehensionSubexpr) = adapt("preVisitExprArrayComprehensionSubexpr",pClass,info);
        FN_PREVISIT(ExprArrayComprehensionWhere) = adapt("preVisitExprArrayComprehensionWhere",pClass,info);
        IMPL_ADAPT(ExprTypeInfo);
        IMPL_ADAPT(ExprLabel);
        IMPL_ADAPT(ExprGoto);
        IMPL_ADAPT(ExprRef2Value);
        IMPL_ADAPT(ExprRef2Ptr);
        IMPL_ADAPT(ExprPtr2Ref);
        IMPL_ADAPT(ExprAddr);
        IMPL_ADAPT(ExprAssert);
        IMPL_ADAPT(ExprStaticAssert);
        IMPL_ADAPT(ExprQuote);
        IMPL_ADAPT(ExprDebug);
        IMPL_ADAPT(ExprInvoke);
        IMPL_ADAPT(ExprErase);
        IMPL_ADAPT(ExprSetInsert);
        IMPL_ADAPT(ExprFind);
        IMPL_ADAPT(ExprKeyExists);
        IMPL_ADAPT(ExprAscend);
        IMPL_ADAPT(ExprCast);
        IMPL_ADAPT(ExprDelete);
        IMPL_ADAPT(ExprVar);
        IMPL_ADAPT(ExprTag);
        FN_PREVISIT(ExprTagValue) = adapt("preVisitExprTagValue",pClass,info);
        IMPL_ADAPT(ExprSwizzle);
        IMPL_ADAPT(ExprField);
        IMPL_ADAPT(ExprSafeField);
        IMPL_ADAPT(ExprIsVariant);
        IMPL_ADAPT(ExprAsVariant);
        IMPL_ADAPT(ExprSafeAsVariant);
        IMPL_ADAPT(ExprOp1);
        IMPL_ADAPT(ExprReturn);
        IMPL_ADAPT(ExprYield);
        IMPL_ADAPT(ExprBreak);
        IMPL_ADAPT(ExprContinue);
        IMPL_ADAPT(ExprConst);
        IMPL_ADAPT(ExprFakeContext);
        IMPL_ADAPT(ExprFakeLineInfo);
        IMPL_ADAPT(ExprConstPtr);
        IMPL_ADAPT(ExprConstEnumeration);
        IMPL_ADAPT(ExprConstBitfield);
        IMPL_ADAPT(ExprConstInt8);
        IMPL_ADAPT(ExprConstInt16);
        IMPL_ADAPT(ExprConstInt64);
        IMPL_ADAPT(ExprConstInt);
        IMPL_ADAPT(ExprConstInt2);
        IMPL_ADAPT(ExprConstInt3);
        IMPL_ADAPT(ExprConstInt4);
        IMPL_ADAPT(ExprConstUInt8);
        IMPL_ADAPT(ExprConstUInt16);
        IMPL_ADAPT(ExprConstUInt64);
        IMPL_ADAPT(ExprConstUInt);
        IMPL_ADAPT(ExprConstUInt2);
        IMPL_ADAPT(ExprConstUInt3);
        IMPL_ADAPT(ExprConstUInt4);
        IMPL_ADAPT(ExprConstRange);
        IMPL_ADAPT(ExprConstURange);
        IMPL_ADAPT(ExprConstRange64);
        IMPL_ADAPT(ExprConstURange64);
        IMPL_ADAPT(ExprConstBool);
        IMPL_ADAPT(ExprConstFloat);
        IMPL_ADAPT(ExprConstFloat2);
        IMPL_ADAPT(ExprConstFloat3);
        IMPL_ADAPT(ExprConstFloat4);
        IMPL_ADAPT(ExprConstString);
        IMPL_ADAPT(ExprConstDouble);
        fnCanVisitMakeBlockBody = adapt("canVisitMakeBlockBody",pClass,info);
        IMPL_ADAPT(ExprMakeBlock);
        IMPL_ADAPT(ExprMakeGenerator);
        IMPL_ADAPT(ExprMemZero);
        IMPL_ADAPT(ExprReader);
        IMPL_ADAPT(ExprUnsafe);
        IMPL_ADAPT(ExprCallMacro);
    }
// whole program
    void VisitorAdapter::preVisitProgram ( Program * expr )
        { IMPL_PREVISIT(Program); }
    void VisitorAdapter::visitProgram ( Program * expr )
        { IMPL_VISIT_VOID(Program); }
    void VisitorAdapter::preVisitProgramBody ( Program * expr, Module * mod )
        { IMPL_PREVISIT2(ProgramBody,Program,Module *,mod); }
// module
    void VisitorAdapter::preVisitModule ( Module * expr )
        { IMPL_PREVISIT(Module); }
    void VisitorAdapter::visitModule ( Module * expr )
        { IMPL_VISIT_VOID(Module); }
// type
    void VisitorAdapter::preVisit ( TypeDecl * expr )
        { IMPL_PREVISIT(TypeDecl); }
    TypeDeclPtr VisitorAdapter::visit ( TypeDecl * expr )
        { IMPL_VISIT(TypeDecl); }
// alias
    void VisitorAdapter::preVisitAlias ( TypeDecl * expr, const string & name )
        { IMPL_PREVISIT2(Alias,TypeDecl,const string &,name); }
    TypeDeclPtr VisitorAdapter::visitAlias ( TypeDecl * expr, const string & name )
        { IMPL_VISIT2(Alias,TypeDecl,TypeDecl,expr,const string &,name); }
// enumeration
    bool VisitorAdapter::canVisitEnumeration ( Enumeration * enu ) {
        if ( fnCanVisitEnumeration ) {
            return das_invoke_function<bool>::invoke<void *,Enumeration *>
                (context,nullptr,fnCanVisitEnumeration,classPtr,enu);
        } else {
            return true;
        }
    }
    void VisitorAdapter::preVisit ( Enumeration * expr )
        { IMPL_PREVISIT(Enumeration); }
    EnumerationPtr VisitorAdapter::visit ( Enumeration * expr )
        { IMPL_VISIT(Enumeration); }
    void VisitorAdapter::preVisitEnumerationValue ( Enumeration * expr, const string & name, Expression * value, bool last )
        { IMPL_PREVISIT4(EnumerationValue,Enumeration,const string &,name,ExpressionPtr,value,bool,last); }
    ExpressionPtr VisitorAdapter::visitEnumerationValue ( Enumeration * expr, const string & name, Expression * value, bool last )
        { IMPL_VISIT4(EnumerationValue,Enumeration,Expression,value,const string &,name,ExpressionPtr,value,bool,last); }
// structure
    bool VisitorAdapter::canVisitStructure ( Structure * var ) {
        if ( fnCanVisitStructure ) {
            return das_invoke_function<bool>::invoke<void *,Structure *>
                (context,nullptr,fnCanVisitStructure,classPtr,var);
        } else {
            return true;
        }
    }
    void VisitorAdapter::preVisit ( Structure * expr )
        { IMPL_PREVISIT(Structure); }
    void VisitorAdapter::preVisitStructureField ( Structure * expr, Structure::FieldDeclaration & decl, bool last )
        { IMPL_PREVISIT3(StructureField,Structure,Structure::FieldDeclaration &,decl,bool,last); }
    void VisitorAdapter::visitStructureField ( Structure * expr, Structure::FieldDeclaration & decl, bool last )  {
        if ( FN_VISIT(StructureField) ) {
            das_invoke_function<void>::invoke<void *,StructurePtr,Structure::FieldDeclaration&,bool>
                (context,nullptr,FN_VISIT(StructureField),classPtr,expr,decl,last);
        }
    }
    StructurePtr VisitorAdapter::visit ( Structure * expr )
        { IMPL_VISIT(Structure); }
// function
    bool VisitorAdapter::canVisitFunction ( Function * fun ) {
        if ( fnCanVisitFunction ) {
            return das_invoke_function<bool>::invoke<void *,Function *>
                (context,nullptr,fnCanVisitFunction,classPtr,fun);
        } else {
            return true;
        }
    }
    bool VisitorAdapter::canVisitArgumentInit ( Function * fun, const VariablePtr & var, Expression * init ) {
        if ( fnCanVisitArgumentInit ) {
            return das_invoke_function<bool>::invoke<void *,Function *,VariablePtr,ExpressionPtr>
                (context,nullptr,fnCanVisitArgumentInit,classPtr,fun,var,init);
        } else {
            return true;
        }
    }
    void VisitorAdapter::preVisit ( Function * expr )
        { IMPL_PREVISIT(Function); }
    FunctionPtr VisitorAdapter::visit ( Function * expr )
        { IMPL_VISIT(Function); }
    void VisitorAdapter::preVisitArgument ( Function * expr, const VariablePtr & var, bool lastArg )
        { IMPL_PREVISIT3(FunctionArgument,Function,VariablePtr,var,bool,lastArg); }
    VariablePtr VisitorAdapter::visitArgument ( Function * expr, const VariablePtr & var, bool lastArg )
        { IMPL_VISIT3(FunctionArgument,Function,Variable,var,VariablePtr,var,bool,lastArg); }
    void VisitorAdapter::preVisitArgumentInit ( Function * expr, const VariablePtr & var, Expression * init )
        { IMPL_PREVISIT3(FunctionArgument,Function,VariablePtr,var,ExpressionPtr,init); }
    ExpressionPtr VisitorAdapter::visitArgumentInit ( Function * expr, const VariablePtr & var, Expression * init )
        { IMPL_VISIT3(FunctionArgumentInit,Function,Expression,init,VariablePtr,var,ExpressionPtr,init); }
    void VisitorAdapter::preVisitFunctionBody ( Function * expr, Expression * that )
        { IMPL_PREVISIT2(FunctionBody,Function,ExpressionPtr,that); }
    ExpressionPtr VisitorAdapter::visitFunctionBody ( Function * expr, Expression * that )
        { IMPL_VISIT2(FunctionBody,Function,Expression,that,ExpressionPtr,that); }
// expression
    void VisitorAdapter::preVisitExpression ( Expression * expr )
        { IMPL_PREVISIT(Expression); }
    ExpressionPtr VisitorAdapter::visitExpression ( Expression * expr )
        { IMPL_VISIT(Expression); }
// block
    IMPL_BIND_EXPR(ExprBlock);
    void VisitorAdapter::preVisitBlockArgument ( ExprBlock * expr, const VariablePtr & var, bool lastArg )
        { IMPL_PREVISIT3(ExprBlockArgument,ExprBlock,VariablePtr,var,bool,lastArg); }
    VariablePtr VisitorAdapter::visitBlockArgument ( ExprBlock * expr, const VariablePtr & var, bool lastArg )
        { IMPL_VISIT3(ExprBlockArgument,ExprBlock,Variable,var,VariablePtr,var,bool,lastArg); }
    void VisitorAdapter::preVisitBlockArgumentInit ( ExprBlock * expr, const VariablePtr & var, Expression * init )
        { IMPL_PREVISIT3(ExprBlockArgumentInit,ExprBlock,VariablePtr,var,ExpressionPtr,init); }
    ExpressionPtr VisitorAdapter::visitBlockArgumentInit ( ExprBlock * expr, const VariablePtr & var, Expression * init )
        { IMPL_VISIT3(ExprBlockArgumentInit,ExprBlock,Expression,init,VariablePtr,var,ExpressionPtr,init); }
    void VisitorAdapter::preVisitBlockExpression ( ExprBlock * expr, Expression * bexpr )
        { IMPL_PREVISIT2(ExprBlockExpression,ExprBlock,ExpressionPtr,bexpr); }
    ExpressionPtr VisitorAdapter::visitBlockExpression (  ExprBlock * expr, Expression * bexpr )
        { IMPL_VISIT2(ExprBlockExpression,ExprBlock,Expression,bexpr,ExpressionPtr,bexpr); }
    void VisitorAdapter::preVisitBlockFinal ( ExprBlock * expr )
        { IMPL_PREVISIT1(ExprBlockFinal,ExprBlock); }
    void VisitorAdapter::visitBlockFinal ( ExprBlock * expr )  {
        if ( FN_VISIT(ExprBlockFinal) ) {
            das_invoke_function<void>::invoke<void *,smart_ptr_raw<ExprBlock>>
                (context,nullptr,FN_VISIT(ExprBlockFinal),classPtr,expr);
        }
    }
    void VisitorAdapter::preVisitBlockFinalExpression ( ExprBlock * expr, Expression * bexpr )
        { IMPL_PREVISIT2(ExprBlockFinalExpression,ExprBlock,ExpressionPtr,bexpr); }
    ExpressionPtr VisitorAdapter::visitBlockFinalExpression (  ExprBlock * expr, Expression * bexpr )
        { IMPL_VISIT2(ExprBlockFinalExpression,ExprBlock,Expression,bexpr,ExpressionPtr,bexpr); }
// let
    IMPL_BIND_EXPR(ExprLet);
    void VisitorAdapter::preVisitLet ( ExprLet * expr, const VariablePtr & var, bool last )
        { IMPL_PREVISIT3(ExprLetVariable,ExprLet,VariablePtr,var,bool,last); }
    VariablePtr VisitorAdapter::visitLet ( ExprLet * expr, const VariablePtr & var, bool last )
        { IMPL_VISIT3(ExprLetVariable,ExprLet,Variable,var,VariablePtr,var,bool,last); }
    void VisitorAdapter::preVisitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init )
        { IMPL_PREVISIT3(ExprLetVariableInit,ExprLet,VariablePtr,var,ExpressionPtr,init); }
    ExpressionPtr VisitorAdapter::visitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init )
        { IMPL_VISIT3(ExprLetVariableInit,ExprLet,Expression,init,VariablePtr,var,ExpressionPtr,init); }
// global let
    bool VisitorAdapter::canVisitGlobalVariable ( Variable * var ) {
        if ( fnCanVisitGlobalVariable ) {
            return das_invoke_function<bool>::invoke<void *,Variable *>
                (context,nullptr,fnCanVisitGlobalVariable,classPtr,var);
        } else {
            return true;
        }
    }
    void VisitorAdapter::preVisitGlobalLetBody ( Program * expr )
        { IMPL_PREVISIT1(GlobalLet,Program); }
    void VisitorAdapter::visitGlobalLetBody ( Program * expr )
        { IMPL_VISIT_VOID1(GlobalLet,Program); }
    void VisitorAdapter::preVisitGlobalLet ( const VariablePtr & expr )
        { IMPL_PREVISIT1(GlobalLetVariable,Variable); }
    VariablePtr VisitorAdapter::visitGlobalLet ( const VariablePtr & expr )
        { IMPL_VISIT1(GlobalLetVariable,Variable,Variable,expr); }
    void VisitorAdapter::preVisitGlobalLetInit ( const VariablePtr & expr, Expression * init )
        { IMPL_PREVISIT2(GlobalLetVariableInit,Variable,ExpressionPtr,init); }
    ExpressionPtr VisitorAdapter::visitGlobalLetInit ( const VariablePtr & expr, Expression * init )
        { IMPL_VISIT2(GlobalLetVariableInit,Variable,Expression,init,ExpressionPtr,init); }
// string builder
    IMPL_BIND_EXPR(ExprStringBuilder);
    void VisitorAdapter::preVisitStringBuilderElement ( ExprStringBuilder * expr, Expression * element, bool last )
        { IMPL_PREVISIT3(ExprStringBuilderElement,ExprStringBuilder,ExpressionPtr,element,bool,last); }
    ExpressionPtr VisitorAdapter::visitStringBuilderElement ( ExprStringBuilder * expr, Expression * element, bool last )
        { IMPL_VISIT3(ExprStringBuilderElement,ExprStringBuilder,Expression,element,ExpressionPtr,element,bool,last); }
// new
    IMPL_BIND_EXPR(ExprNew);
    void VisitorAdapter::preVisitNewArg ( ExprNew * expr, Expression * arg, bool last )
        { IMPL_PREVISIT3(ExprNewArgument,ExprNew,ExpressionPtr,arg,bool,last); }
    ExpressionPtr VisitorAdapter::visitNewArg ( ExprNew * expr, Expression * arg , bool last )
        { IMPL_VISIT3(ExprNewArgument,ExprNew,Expression,arg,ExpressionPtr,arg,bool,last); }
// named call
    IMPL_BIND_EXPR(ExprNamedCall);
    void VisitorAdapter::preVisitNamedCallArg ( ExprNamedCall * expr, MakeFieldDecl * arg, bool last )
        { IMPL_PREVISIT3(ExprNamedCallArgument,ExprNamedCall,MakeFieldDeclPtr,arg,bool,last); }
    MakeFieldDeclPtr VisitorAdapter::visitNamedCallArg ( ExprNamedCall * expr, MakeFieldDecl * arg , bool last )
        { IMPL_VISIT3(ExprNamedCallArgument,ExprNamedCall,MakeFieldDecl,arg,MakeFieldDeclPtr,arg,bool,last); }
// call
    IMPL_BIND_EXPR(ExprCall);
    bool VisitorAdapter::canVisitCall ( ExprCall * expr ) {
        if ( fnCanVisitCall ) {
            return das_invoke_function<bool>::invoke<void *,ExprCall *>
                (context,nullptr,fnCanVisitCall,classPtr,expr);
        } else {
            return true;
        }
    }
    void VisitorAdapter::preVisitCallArg ( ExprCall * expr, Expression * arg, bool last )
        { IMPL_PREVISIT3(ExprCallArgument,ExprCall,ExpressionPtr,arg,bool,last); }
    ExpressionPtr VisitorAdapter::visitCallArg ( ExprCall * expr, Expression * arg , bool last )
        { IMPL_VISIT3(ExprCallArgument,ExprCall,Expression,arg,ExpressionPtr,arg,bool,last); }
// looks like call
    IMPL_BIND_EXPR(ExprLooksLikeCall);
    void VisitorAdapter::preVisitLooksLikeCallArg ( ExprLooksLikeCall * expr, Expression * arg, bool last )
        { IMPL_PREVISIT3(ExprLooksLikeCallArgument,ExprLooksLikeCall,ExpressionPtr,arg,bool,last); }
    ExpressionPtr VisitorAdapter::visitLooksLikeCallArg ( ExprLooksLikeCall * expr, Expression * arg , bool last )
        { IMPL_VISIT3(ExprLooksLikeCallArgument,ExprLooksLikeCall,Expression,arg,ExpressionPtr,arg,bool,last); }
// null coaelescing
    IMPL_BIND_EXPR(ExprNullCoalescing);
    void VisitorAdapter::preVisitNullCoaelescingDefault ( ExprNullCoalescing * expr, Expression * defval )
        { IMPL_PREVISIT2(ExprNullCoalescingDefault,ExprNullCoalescing,ExpressionPtr, defval); }
// at
    IMPL_BIND_EXPR(ExprAt);
    void VisitorAdapter::preVisitAtIndex ( ExprAt * expr, Expression * index )
        { IMPL_PREVISIT2(ExprAtIndex,ExprAt,ExpressionPtr,index); }
// safe at
    IMPL_BIND_EXPR(ExprSafeAt);
    void VisitorAdapter::preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index )
        { IMPL_PREVISIT2(ExprSafeAtIndex,ExprSafeAt,ExpressionPtr,index); }
// is
    IMPL_BIND_EXPR(ExprIs);
    void VisitorAdapter::preVisitType ( ExprIs * expr, TypeDecl * typeDecl )
        { IMPL_PREVISIT2(ExprIsType,ExprIs,TypeDeclPtr,typeDecl); }
// op2
    IMPL_BIND_EXPR(ExprOp2);
    void VisitorAdapter::preVisitRight ( ExprOp2 * expr, Expression * right )
        { IMPL_PREVISIT2(ExprOp2Right,ExprOp2,ExpressionPtr,right); }
// op3
    IMPL_BIND_EXPR(ExprOp3);
    void VisitorAdapter::preVisitLeft  ( ExprOp3 * expr, Expression * left )
        { IMPL_PREVISIT2(ExprOp3Left,ExprOp3,ExpressionPtr,left); }
    void VisitorAdapter::preVisitRight ( ExprOp3 * expr, Expression * right )
        { IMPL_PREVISIT2(ExprOp3Right,ExprOp3,ExpressionPtr,right); }
// copy
    IMPL_BIND_EXPR(ExprCopy);
    void VisitorAdapter::preVisitRight ( ExprCopy * expr, Expression * right )
        { IMPL_PREVISIT2(ExprCopyRight,ExprCopy,ExpressionPtr,right); }
// move
    IMPL_BIND_EXPR(ExprMove);
    void VisitorAdapter::preVisitRight ( ExprMove * expr, Expression * right )
        { IMPL_PREVISIT2(ExprMoveRight,ExprMove,ExpressionPtr,right); }
// clone
    IMPL_BIND_EXPR(ExprClone);
    void VisitorAdapter::preVisitRight ( ExprClone * expr, Expression * right )
        { IMPL_PREVISIT2(ExprCloneRight,ExprClone,ExpressionPtr,right); }
// assume
    IMPL_BIND_EXPR(ExprAssume);
    bool VisitorAdapter::canVisitWithAliasSubexpression ( ExprAssume * expr) {
        if ( fnCanVisitWithAliasSubexpression ) {
            return das_invoke_function<bool>::invoke<void *,ExprAssume *>
                (context,nullptr,fnCanVisitWithAliasSubexpression,classPtr,expr);
        } else {
            return Visitor::canVisitWithAliasSubexpression(expr);
        }
     }
// with
    IMPL_BIND_EXPR(ExprWith);
    void VisitorAdapter::preVisitWithBody ( ExprWith * expr, Expression * body )
        { IMPL_PREVISIT2(ExprWithBody,ExprWith,ExpressionPtr,body); }
// while
    IMPL_BIND_EXPR(ExprWhile);
    void VisitorAdapter::preVisitWhileBody ( ExprWhile * expr, Expression * body )
        { IMPL_PREVISIT2(ExprWhileBody,ExprWhile,ExpressionPtr,body); }
// try-catch
    IMPL_BIND_EXPR(ExprTryCatch);
    void VisitorAdapter::preVisitCatch ( ExprTryCatch * expr, Expression * body )
        { IMPL_PREVISIT2(ExprTryCatchCatch,ExprTryCatch,ExpressionPtr,body); }
// if-then-else
    IMPL_BIND_EXPR(ExprIfThenElse);
    void VisitorAdapter::preVisitIfBlock ( ExprIfThenElse * expr, Expression * ifBlock )
        { IMPL_PREVISIT2(ExprIfThenElseIfBlock,ExprIfThenElse,ExpressionPtr,ifBlock); }
    void VisitorAdapter::preVisitElseBlock ( ExprIfThenElse * expr, Expression * elseBlock )
        { IMPL_PREVISIT2(ExprIfThenElseElseBlock,ExprIfThenElse,ExpressionPtr,elseBlock); }
// for
    IMPL_BIND_EXPR(ExprFor);
    void VisitorAdapter::preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last )
        { IMPL_PREVISIT3(ExprForVariable,ExprFor,VariablePtr,var,bool,last); }
    VariablePtr VisitorAdapter::visitFor ( ExprFor * expr, const VariablePtr & var, bool last )
        { IMPL_VISIT3(ExprForVariable,ExprFor,Variable,var,VariablePtr,var,bool,last); }
    void VisitorAdapter::preVisitForStack ( ExprFor * expr )
        { IMPL_PREVISIT1(ExprForStack,ExprFor); }
    void VisitorAdapter::preVisitForBody ( ExprFor * expr, Expression * body )
        { IMPL_PREVISIT2(ExprForBody,ExprFor,ExpressionPtr,body); }
    void VisitorAdapter::preVisitForSource ( ExprFor * expr, Expression * source, bool last )
        { IMPL_PREVISIT3(ExprForSource,ExprFor,ExpressionPtr,source,bool,last); }
    ExpressionPtr VisitorAdapter::VisitorAdapter::visitForSource ( ExprFor * expr, Expression * source , bool last )
        { IMPL_VISIT3(ExprForSource,ExprFor,Expression,source,ExpressionPtr,source,bool,last); }
// make variant
    IMPL_BIND_EXPR(ExprMakeVariant);
    void VisitorAdapter::preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last )
        { IMPL_PREVISIT4(ExprMakeVariantField,ExprMakeVariant,int,index,MakeFieldDeclPtr,decl,bool,last); }
    MakeFieldDeclPtr VisitorAdapter::visitMakeVariantField(ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last)
        { IMPL_VISIT4(ExprMakeVariantField,ExprMakeVariant,MakeFieldDecl,decl,int,index,MakeFieldDeclPtr,decl,bool,last); }
// make structure
    bool VisitorAdapter::canVisitMakeStructureBlock ( ExprMakeStruct * expr, Expression * blk ) {
        if ( fnCanVisitMakeStructBlock ) {
            return das_invoke_function<bool>::invoke<void *,smart_ptr<ExprMakeStruct>,ExpressionPtr>
                (context,nullptr,fnCanVisitMakeStructBlock,classPtr,expr,blk);
        } else {
            return true;
        }
    }
    bool VisitorAdapter::canVisitMakeStructureBody ( ExprMakeStruct * expr ) {
        if ( fnCanVisitMakeStructBody ) {
            return das_invoke_function<bool>::invoke<void *,smart_ptr<ExprMakeStruct>>
                (context,nullptr,fnCanVisitMakeStructBody,classPtr,expr);
        } else {
            return true;
        }
    }
    IMPL_BIND_EXPR(ExprMakeStruct);
    void VisitorAdapter::preVisitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool last )
        { IMPL_PREVISIT3(ExprMakeStructIndex,ExprMakeStruct,int,index,bool,last); }
    void VisitorAdapter::visitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool last )
        { IMPL_VISIT_VOID3(ExprMakeStructIndex,ExprMakeStruct,int,index,bool,last); }
    void VisitorAdapter::preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last )
            { IMPL_PREVISIT4(ExprMakeStructField,ExprMakeStruct,int,index,MakeFieldDeclPtr,decl,bool,last); }
    MakeFieldDeclPtr VisitorAdapter::visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last )
        { IMPL_VISIT4(ExprMakeStructField,ExprMakeStruct,MakeFieldDecl,decl,int,index,MakeFieldDeclPtr,decl,bool,last); }
// make array
    IMPL_BIND_EXPR(ExprMakeArray);
    void VisitorAdapter::preVisitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool last )
        { IMPL_PREVISIT4(ExprMakeArrayIndex,ExprMakeArray,int,index,ExpressionPtr,init,bool,last); }
    ExpressionPtr VisitorAdapter::visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool last )
        { IMPL_VISIT4(ExprMakeArrayIndex,ExprMakeArray,Expression,init,int,index,ExpressionPtr,init,bool,last); }
// make tuple
    IMPL_BIND_EXPR(ExprMakeTuple);
    void VisitorAdapter::preVisitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool last )
        { IMPL_PREVISIT4(ExprMakeTupleIndex,ExprMakeTuple,int,index,ExpressionPtr,init,bool,last); }
    ExpressionPtr VisitorAdapter::visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool last )
        { IMPL_VISIT4(ExprMakeTupleIndex,ExprMakeTuple,Expression,init,int,index,ExpressionPtr,init,bool,last); }
// array comprehension
    IMPL_BIND_EXPR(ExprArrayComprehension);
    void VisitorAdapter::preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr )
        { IMPL_PREVISIT2(ExprArrayComprehensionSubexpr,ExprArrayComprehension,ExpressionPtr,subexpr); }
    void VisitorAdapter::preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where )
        { IMPL_PREVISIT2(ExprArrayComprehensionWhere,ExprArrayComprehension,ExpressionPtr,where); }
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
    void VisitorAdapter::preVisitTagValue ( ExprTag * expr, Expression * value )
        { IMPL_PREVISIT2(ExprTagValue,ExprTag,ExpressionPtr,value); }
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
// make block
    bool VisitorAdapter::canVisitMakeBlockBody ( ExprMakeBlock * expr ) {
        if ( fnCanVisitMakeBlockBody ) {
            return das_invoke_function<bool>::invoke<void *,smart_ptr<ExprMakeBlock>>
                (context,nullptr,fnCanVisitMakeBlockBody,classPtr,expr);
        } else {
            return true;
        }
    }
    IMPL_BIND_EXPR(ExprMakeBlock);
    IMPL_BIND_EXPR(ExprMakeGenerator);
    IMPL_BIND_EXPR(ExprMemZero);
    IMPL_BIND_EXPR(ExprReader);
    IMPL_BIND_EXPR(ExprCallMacro);
    IMPL_BIND_EXPR(ExprUnsafe);
    IMPL_BIND_EXPR(ExprTypeDecl);

#include "ast_gen.inc"

    void runMacroFunction ( Context * context, const string & message, const callable<void()> & subexpr ) {
        if ( !context->runWithCatch(subexpr) ) {
            DAS_ASSERTF(daScriptEnvironment::bound->g_Program, "calling macros while not compiling a program");
            daScriptEnvironment::bound->g_Program->error(
                "macro caused exception during " + message,
                context->getException(), "",
                context->exceptionAt,
                CompilationError::exception_during_macro
            );
            daScriptEnvironment::bound->g_Program->macroException = true;
        }
    }

    struct AstVisitorAdapterAnnotation : ManagedStructureAnnotation<VisitorAdapter,false,true> {
        AstVisitorAdapterAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("VisitorAdapter", ml) {
        }
    };

    class BlockAnnotationAdapter : public FunctionAnnotation, AstBlockAnnotation_Adapter {
    public:
        BlockAnnotationAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
        : FunctionAnnotation(n), AstBlockAnnotation_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &,
                            const AnnotationArgumentList &, string & err ) override {
            err = "not a function annotation";
            return false;
        }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &,
                               const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & err ) override {
            err = "not a function annotation";
            return false;
        }
        virtual bool apply ( ExprBlock * blk, ModuleGroup & group,
                            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnApply = get_apply(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "apply", [&]() {
                    result = invoke_apply(context,fnApply,classPtr,blk,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool finalize ( ExprBlock * blk, ModuleGroup & group,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & errors ) override {
            if ( auto fnFinish = get_finish(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "finish", [&]() {
                    result = invoke_finish(context,fnFinish,classPtr,blk,group,args,progArgs,errors);
                });
                return result;
            } else {
                return true;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    class FunctionAnnotationAdapter : public FunctionAnnotation, AstFunctionAnnotation_Adapter {
    public:
        FunctionAnnotationAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
        : FunctionAnnotation(n), AstFunctionAnnotation_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool apply ( const FunctionPtr & func, ModuleGroup & group,
                            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnApply = get_apply(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "apply", [&]() {
                    result = invoke_apply(context,fnApply,classPtr,func,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool generic_apply ( const FunctionPtr & func, ModuleGroup & group,
                            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnApply = get_generic_apply(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "generic_apply", [&]() {
                    result = invoke_generic_apply(context,fnApply,classPtr,func,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool finalize ( const FunctionPtr & func, ModuleGroup & group,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & errors ) override {
            if ( auto fnFinish = get_finish(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "finish", [&]() {
                    result = invoke_finish(context,fnFinish,classPtr,func,group,args,progArgs,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool lint ( const FunctionPtr & func, ModuleGroup & group,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & errors ) override {
            if ( auto fnLint = get_lint(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "lint", [&]() {
                    result = invoke_lint(context,fnLint,classPtr,func,group,args,progArgs,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool patch ( const FunctionPtr & func, ModuleGroup & group,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & errors, bool & astChanged ) override {
            if ( auto fnPatch = get_patch(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "patch", [&]() {
                    result = invoke_patch(context,fnPatch,classPtr,func,group,args,progArgs,errors,astChanged);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool fixup ( const FunctionPtr & func, ModuleGroup & group,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & errors ) override {
            if ( auto fnFixup = get_fixup(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "fixup", [&]() {
                    result = invoke_fixup(context,fnFixup,classPtr,func,group,args,progArgs,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool apply ( ExprBlock *, ModuleGroup &,
                            const AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual bool finalize ( ExprBlock *, ModuleGroup &,
                               const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
            if ( auto fnTransform = get_transform(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "transformCall", [&]() {
                    result = invoke_transform(context,fnTransform,classPtr,call,err);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual bool verifyCall ( ExprCallFunc * call, const AnnotationArgumentList & args,
                const AnnotationArgumentList & progArgs, string & err ) override {
            if ( auto fnTransform = get_verifyCall(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "verifyCall", [&]() {
                    result = invoke_verifyCall(context,fnTransform,classPtr,call,args,progArgs,err);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool isSpecialized () const override {
            if ( auto fnIsSpecialized = get_isSpecialized(classPtr) ) {
                bool result = false;
                runMacroFunction(context, "isSpecialized", [&]() {
                    result = invoke_isSpecialized(context,fnIsSpecialized,classPtr);
                });
                return result;
            } else {
                return false;
            }
        }
        virtual bool isCompatible ( const FunctionPtr & fn, const vector<TypeDeclPtr> & types,
            const AnnotationDeclaration & decl, string & err  ) const override {
            if ( auto fnIsCompatible = get_isCompatible(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "isCompatible", [&]() {
                    result = invoke_isCompatible(context,fnIsCompatible,classPtr,fn,const_cast<vector<TypeDeclPtr> &>(types),decl,err);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual void  complete (  Context * ctx, const FunctionPtr & fnp ) override {
            if ( auto fnComplete = get_complete(classPtr) ) {
                runMacroFunction(context, "complete", [&]() {
                    invoke_complete(context,fnComplete,classPtr,fnp,ctx);
                });
            }
        }
        virtual void appendToMangledName( const FunctionPtr & fnp, const AnnotationDeclaration & decl, string & mangledName ) const override {
            if ( auto fnAppend = get_appendToMangledName(classPtr) ) {
                runMacroFunction(context, "appendToMangledName", [&]() {
                    invoke_appendToMangledName(context,fnAppend,classPtr,fnp,decl,mangledName);
                });
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstFunctionAnnotationAnnotation : ManagedStructureAnnotation<FunctionAnnotation,false,true> {
        AstFunctionAnnotationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("FunctionAnnotation", ml) {
        }
    };

    struct StructureAnnotationAdapter : StructureAnnotation, AstStructureAnnotation_Adapter {
        StructureAnnotationAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : StructureAnnotation(n), AstStructureAnnotation_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool touch ( const StructurePtr & st, ModuleGroup & group,
                            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnApply = get_apply(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "apply", [&]() {
                    result = invoke_apply(context,fnApply,classPtr,st,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool look (const StructurePtr & st, ModuleGroup & group,
            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnFinish = get_finish(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "finish", [&]() {
                    result = invoke_finish(context,fnFinish,classPtr,st,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool patch (const StructurePtr & st, ModuleGroup & group,
            const AnnotationArgumentList & args, string & errors, bool & astChanged ) override {
            if ( auto fnPatch = get_patch(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "patch", [&]() {
                    result = invoke_patch(context,fnPatch,classPtr,st,group,args,errors,astChanged);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual void  complete (  Context * ctx, const StructurePtr & stp ) override {
            if ( auto fnComplete = get_complete(classPtr) ) {
                runMacroFunction(context, "complete", [&]() {
                    invoke_complete(context,fnComplete,classPtr,stp,ctx);
                });
            }
        }
        virtual void aotPrefix ( const StructurePtr & st, const AnnotationArgumentList & args,TextWriter & writer ) override {
            if ( auto fnAotPrefix = get_aotPrefix(classPtr) ) {
                runMacroFunction(context, "aotPrefix", [&]() {
                    invoke_aotPrefix(context,fnAotPrefix,classPtr,st,args,reinterpret_cast<StringBuilderWriter&>(writer));
                });
            }
        }
        virtual void aotBody  ( const StructurePtr & st, const AnnotationArgumentList & args, TextWriter & writer ) override {
            if ( auto fnAotBody = get_aotBody(classPtr) ) {
                runMacroFunction(context, "aotBody", [&]() {
                    invoke_aotBody(context,fnAotBody,classPtr,st,args,reinterpret_cast<StringBuilderWriter&>(writer));
                });
            }

        }
        virtual void aotSuffix ( const StructurePtr & st, const AnnotationArgumentList & args,TextWriter & writer ) override {
            if ( auto fnAotSuffix = get_aotSuffix(classPtr) ) {
                runMacroFunction(context, "aotSuffix", [&]() {
                    invoke_aotSuffix(context,fnAotSuffix,classPtr,st,args,reinterpret_cast<StringBuilderWriter&>(writer));
                });
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstStructureAnnotationAnnotation : ManagedStructureAnnotation<StructureAnnotation,false,true> {
        AstStructureAnnotationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("StructureAnnotation", ml) {
        }
    };

    struct EnumerationAnnotationAdapter : EnumerationAnnotation, AstEnumerationAnnotation_Adapter {
        EnumerationAnnotationAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : EnumerationAnnotation(n), AstEnumerationAnnotation_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool touch ( const EnumerationPtr & st, ModuleGroup & group,
                            const AnnotationArgumentList & args, string & errors ) override {
            if ( auto fnApply = get_apply(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "apply", [&]() {
                    result = invoke_apply(context,fnApply,classPtr,st,group,args,errors);
                });
                return result;
            } else {
                return true;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstEnumerationAnnotationAnnotation : ManagedStructureAnnotation<EnumerationAnnotation,false,true> {
        AstEnumerationAnnotationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("EnumerationAnnotation", ml) {
        }
    };

    struct PassMacroAdapter : PassMacro, AstPassMacro_Adapter {
        PassMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : PassMacro(n), AstPassMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool apply ( Program * prog, Module * mod ) override {
            if ( auto fnApply = get_apply(classPtr) ) {
                bool result = false;
                runMacroFunction(context, "apply", [&]() {
                    result = invoke_apply(context,fnApply,classPtr,prog,mod);
                });
                return result;
            } else {
                return false;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstPassMacroAnnotation : ManagedStructureAnnotation<PassMacro,false,true> {
        AstPassMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("PassMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct VariantMacroAdapter : VariantMacro, AstVarianMacro_Adapter {
        VariantMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : VariantMacro(n), AstVarianMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual ExpressionPtr visitIs ( Program * prog, Module * mod, ExprIsVariant * expr ) override {
            if ( auto fnVisitIs = get_visitExprIsVariant(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visitExprIsVariant", [&]() {
                    result = invoke_visitExprIsVariant(context,fnVisitIs,classPtr,prog,mod,expr);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual ExpressionPtr visitAs ( Program * prog, Module * mod, ExprAsVariant * expr ) override {
            if ( auto fnVisitAs = get_visitExprAsVariant(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visitExprAsVariant", [&]() {
                    result = invoke_visitExprAsVariant(context,fnVisitAs,classPtr,prog,mod,expr);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual ExpressionPtr visitSafeAs ( Program * prog, Module * mod, ExprSafeAsVariant * expr ) override {
            if ( auto fnVisitSafeAs = get_visitExprSafeAsVariant(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visitExprSafeAsVariant", [&]() {
                    result = invoke_visitExprSafeAsVariant(context,fnVisitSafeAs,classPtr,prog,mod,expr);
                });
                return result;
            } else {
                return nullptr;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstVariantMacroAnnotation : ManagedStructureAnnotation<VariantMacro,false,true> {
        AstVariantMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("VariantMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct ForLoopMacroAdapter : ForLoopMacro, AstForLoopMacro_Adapter {
        ForLoopMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : ForLoopMacro(n), AstForLoopMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual ExpressionPtr visit ( Program * prog, Module * mod, ExprFor * loop ) override {
            if ( auto fnVisit = get_visitExprFor(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visitExprFor", [&]() {
                    result = invoke_visitExprFor(context,fnVisit,classPtr,prog,mod,loop);
                });
                return result;
            } else {
                return nullptr;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstForLoopMacroAnnotation : ManagedStructureAnnotation<ForLoopMacro,false,true> {
        AstForLoopMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("ForLoopMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct CaptureMacroAdapter : CaptureMacro, AstCaptureMacro_Adapter {
        CaptureMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : CaptureMacro(n), AstCaptureMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual ExpressionPtr captureExpression ( Program * prog, Module * mod, Expression * expr, TypeDecl * typ ) override {
            if ( auto fnCaptureExpression = get_captureExpression(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "captureExpression", [&]() {
                    result = invoke_captureExpression(context,fnCaptureExpression,classPtr,prog,mod,expr,typ);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual void captureFunction ( Program * prog, Module * mod, Structure * lcs, Function * fun ) override {
            if ( auto fnCaptureFunction = get_captureFunction(classPtr) ) {
                runMacroFunction(context, "captureFunction", [&]() {
                    invoke_captureFunction(context,fnCaptureFunction,classPtr,prog,mod,lcs,fun);
                });
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstCaptureMacroAnnotation : ManagedStructureAnnotation<CaptureMacro,false,true> {
        AstCaptureMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("CaptureMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct SimulateMacroAdapter : SimulateMacro, AstSimulateMacro_Adapter {
        SimulateMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : SimulateMacro(n), AstSimulateMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool preSimulate ( Program * prog, Context * ctx ) override {
            if ( auto fnPreSimulate = get_preSimulate(classPtr) ) {
                bool result;
                runMacroFunction(context, "preSimulate", [&]() {
                    result = invoke_preSimulate(context,fnPreSimulate,classPtr,prog,ctx);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool simulate ( Program * prog, Context * ctx ) override {
            if ( auto fnSimulate = get_simulate(classPtr) ) {
                bool result;
                runMacroFunction(context, "simulate", [&]() {
                    result = invoke_simulate(context,fnSimulate,classPtr,prog,ctx);
                });
                return result;
            } else {
                return true;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstSimulateMacroAnnotation : ManagedStructureAnnotation<SimulateMacro,false,true> {
        AstSimulateMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("SimulateMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
        }
    };

    struct ReaderMacroAdapter : ReaderMacro, AstReaderMacro_Adapter {
        ReaderMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : ReaderMacro(n), AstReaderMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool accept ( Program * prog, Module * mod, ExprReader * expr, int Ch, const LineInfo & info ) override {
            if ( auto fnAccept = get_accept(classPtr) ) {
                bool result = false;
                runMacroFunction(context, "accept", [&]() {
                    result = invoke_accept(context,fnAccept,classPtr,prog,mod,expr,Ch,info);
                });
                return result;
            } else {
                return false;
            }
        }
        virtual ExpressionPtr visit (  Program * prog, Module * mod, ExprReader * expr ) override {
            if ( auto fnVisit = get_visit(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visit", [&]() {
                    result = invoke_visit(context,fnVisit,classPtr,prog,mod,expr);
                });
                return result;
            } else {
                return nullptr;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstReaderMacroAnnotation : ManagedStructureAnnotation<ReaderMacro,false,true> {
        AstReaderMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("ReaderMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
        }
    };

    struct CommentReaderAdapter : CommentReader, AstCommentReader_Adapter {
        CommentReaderAdapter ( char * pClass, const StructInfo * info, Context * ctx )
            : AstCommentReader_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual void open ( bool cppStyle, const LineInfo & info ) override {
            if ( auto fnOpen = get_open(classPtr) ) {
                runMacroFunction(context, "open", [&]() {
                    invoke_open(context,fnOpen,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            cppStyle,info);
                });
            }
        }
        virtual void accept ( int Ch, const LineInfo & info ) override {
            if ( auto fnAccept = get_accept(classPtr) ) {
                runMacroFunction(context, "accept", [&]() {
                    invoke_accept(context,fnAccept,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            Ch,info);
                });
            }
        }
        virtual void close ( const LineInfo & info ) override {
            if ( auto fnClose = get_close(classPtr) ) {
                runMacroFunction(context, "close", [&]() {
                    invoke_close(context,fnClose,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeStructure ( const LineInfo & info ) override {
            if ( auto fnBeforeStructure = get_beforeStructure(classPtr) ) {
                runMacroFunction(context, "beforeStructure", [&]() {
                    invoke_beforeStructure(context,fnBeforeStructure,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterStructure ( Structure * st, const LineInfo & info ) override {
            if ( auto fnAfterStructure = get_afterStructure(classPtr) ) {
                runMacroFunction(context, "afterStructure", [&]() {
                    invoke_afterStructure(context,fnAfterStructure,classPtr,
                        st, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }

        }
        virtual void beforeFunction ( const LineInfo & info ) override {
            if ( auto fnBeforeFunction = get_beforeFunction(classPtr) ) {
                runMacroFunction(context, "beforeFunction", [&]() {
                    invoke_beforeFunction(context,fnBeforeFunction,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterFunction ( Function * fn, const LineInfo & info ) override {
            if ( auto fnAfterFunction = get_afterFunction(classPtr) ) {
                runMacroFunction(context, "afterFunction", [&]() {
                    invoke_afterFunction(context,fnAfterFunction,classPtr,
                        fn, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeStructureFields ( const LineInfo & info ) override {
            if ( auto fnBeforeStructureFields = get_beforeStructureFields(classPtr) ) {
                runMacroFunction(context, "beforeStructureFields", [&]() {
                    invoke_beforeStructureFields(context,fnBeforeStructureFields,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterStructureField ( const char * name, const LineInfo & info ) override {
            if ( auto fnAfterStructureField = get_afterStructureField(classPtr) ) {
                runMacroFunction(context, "afterStructureField", [&]() {
                    invoke_afterStructureField(context,fnAfterStructureField,classPtr,
                        (char *) name, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterStructureFields ( const LineInfo & info ) override {
            if ( auto fnAfterStructureFields = get_afterStructureFields(classPtr) ) {
                runMacroFunction(context, "afterStructureFields", [&]() {
                    invoke_afterStructureFields(context,fnAfterStructureFields,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeGlobalVariables ( const LineInfo & info ) override {
            if ( auto fnGlobalVariables = get_beforeGlobalVariables(classPtr) ) {
                runMacroFunction(context, "beforeGlobalVariables", [&]() {
                    invoke_beforeGlobalVariables(context,fnGlobalVariables,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterGlobalVariable ( const char * name, const LineInfo & info ) override {
            if ( auto fnGlobalVariable = get_afterGlobalVariable(classPtr) ) {
                runMacroFunction(context, "afterGlobalVariable", [&]() {
                    invoke_afterGlobalVariable(context,fnGlobalVariable,classPtr,
                        (char *) name, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterGlobalVariables ( const LineInfo & info ) override {
            if ( auto fnGlobalVariables = get_afterGlobalVariables(classPtr) ) {
                runMacroFunction(context, "afterGlobalVariables", [&]() {
                    invoke_afterGlobalVariables(context,fnGlobalVariables,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeVariant ( const LineInfo & info ) override {
            if ( auto fnVariant = get_beforeVariant(classPtr) ) {
                runMacroFunction(context, "beforeVariant", [&]() {
                    invoke_beforeVariant(context,fnVariant,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterVariant ( const char * name, const LineInfo & info ) override {
            if ( auto fnVariant = get_afterVariant(classPtr) ) {
                runMacroFunction(context, "afterVariant", [&]() {
                    invoke_afterVariant(context,fnVariant,classPtr,
                        (char *) name, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeEnumeration ( const LineInfo & info ) override {
            if ( auto fnEnum = get_beforeEnumeration(classPtr) ) {
                runMacroFunction(context, "beforeEnumeration", [&]() {
                    invoke_beforeEnumeration(context,fnEnum,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterEnumeration ( const char * name, const LineInfo & info ) override {
            if ( auto fnEnum = get_afterEnumeration(classPtr) ) {
                runMacroFunction(context, "afterEnumeration", [&]() {
                    invoke_afterEnumeration(context,fnEnum,classPtr,
                        (char *) name, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void beforeAlias ( const LineInfo & info ) override {
            if ( auto fnAlias = get_beforeAlias(classPtr) ) {
                runMacroFunction(context, "beforeAlias", [&]() {
                    invoke_beforeAlias(context,fnAlias,classPtr,
                        daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
        virtual void afterAlias ( const char * name, const LineInfo & info ) override {
            if ( auto fnAlias = get_afterAlias(classPtr) ) {
                runMacroFunction(context, "afterAlias", [&]() {
                    invoke_afterAlias(context,fnAlias,classPtr,
                        (char *) name, daScriptEnvironment::bound->g_Program,
                        daScriptEnvironment::bound->g_Program->thisModule.get(),
                            info);
                });
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstCommentReaderAnnotation : ManagedStructureAnnotation<CommentReader,false,true> {
        AstCommentReaderAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("CommentReader", ml) {
        }
    };

    struct CallMacroAdapter : CallMacro, AstCallMacro_Adapter {
        CallMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : CallMacro(n), AstCallMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual void preVisit (  Program * prog, Module * mod, ExprCallMacro * expr ) override {
            if ( auto fnPreVisit = get_preVisit(classPtr) ) {
                runMacroFunction(context, "preVisit", [&]() {
                    invoke_preVisit(context,fnPreVisit,classPtr,prog,mod,expr);
                });
            }
        }
        virtual ExpressionPtr visit (  Program * prog, Module * mod, ExprCallMacro * expr ) override {
            if ( auto fnVisit = get_visit(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "visit", [&]() {
                    result = invoke_visit(context,fnVisit,classPtr,prog,mod,expr);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual bool canVisitArguments ( ExprCallMacro * expr, int index ) override {
            if ( auto fnCanVisitArguments = get_canVisitArgument(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "canVisitArguments", [&]() {
                    result = invoke_canVisitArgument(context,fnCanVisitArguments,classPtr,expr,index);
                });
                return result;
            } else {
                return true;
            }
        }
        virtual bool canFoldReturnResult ( ExprCallMacro * expr ) override {
            if ( auto fnCanFoldResult = get_canFoldReturnResult(classPtr) ) {
                bool result = true;
                runMacroFunction(context, "canFoldReturnResult", [&]() {
                    result = invoke_canFoldReturnResult(context,fnCanFoldResult,classPtr,expr);
                });
                return result;
            } else {
                return true;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstCallMacroAnnotation : ManagedStructureAnnotation<CallMacro,false,true> {
        AstCallMacroAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("CallMacro", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
        }
    };
    struct TypeInfoMacroAdapter : TypeInfoMacro, AstTypeInfoMacro_Adapter {
        TypeInfoMacroAdapter ( const string & n, char * pClass, const StructInfo * info, Context * ctx )
            : TypeInfoMacro(n), AstTypeInfoMacro_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual ExpressionPtr getAstChange ( const ExpressionPtr & expr, string & err ) override {
            if ( auto fnGetAstChange = get_getAstChange(classPtr) ) {
                ExpressionPtr result;
                runMacroFunction(context, "getAstChange", [&]() {
                    auto tinfo = static_pointer_cast<ExprTypeInfo>(expr);
                    result = invoke_getAstChange(context,fnGetAstChange,classPtr,tinfo,err);
                });
                return result;
            } else {
                return nullptr;
            }
        }
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, const ExpressionPtr & expr, string & err ) override {
            if ( auto fnGetAstType = get_getAstType(classPtr) ) {
                TypeDeclPtr result;
                runMacroFunction(context, "getAstType", [&]() {
                    auto tinfo = static_pointer_cast<ExprTypeInfo>(expr);
                    result = invoke_getAstType(context,fnGetAstType,classPtr,lib,tinfo,err);
                });
                return result;
            } else {
                return nullptr;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    ReaderMacroPtr makeReaderMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<ReaderMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleReaderMacro ( Module * module, ReaderMacroPtr & _newM, Context * context, LineInfoArg * at ) {
        ReaderMacroPtr newM = das::move(_newM);
        if ( !module->addReaderMacro(newM, true) ) {
            context->throw_error_at(at, "can't add reader macro %s to module %s", newM->name.c_str(), module->name.c_str());
        }
    }

    CommentReaderPtr makeCommentReader ( const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<CommentReaderAdapter>((char *)pClass,info,context);
    }

    void addModuleCommentReader ( Module * module, CommentReaderPtr & _newM, Context * context, LineInfoArg * at ) {
        CommentReaderPtr newM = das::move(_newM);
        if ( !module->addCommentReader(newM, true) ) {
            context->throw_error_at(at, "can't add comment reader to module %s", module->name.c_str());
        }
    }

    CallMacroPtr makeCallMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<CallMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleCallMacro ( Module * module, CallMacroPtr & _newM, Context * context, LineInfoArg * at ) {
        CallMacroPtr newM = das::move(_newM);
        if ( ! module->addCallMacro(newM->name, [=](const LineInfo & at) -> ExprLooksLikeCall * {
            auto ecm = new ExprCallMacro(at, newM->name);
            ecm->macro = newM.get();
            newM->module = module;
            return ecm;
        }) ) {
            context->throw_error_at(at, "can't add call macro %s to module %s", newM->name.c_str(), module->name.c_str());
        }
    }

    TypeInfoMacroPtr makeTypeInfoMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<TypeInfoMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleTypeInfoMacro ( Module * module, TypeInfoMacroPtr & _newM, Context * context, LineInfoArg * at ) {
        TypeInfoMacroPtr newM = das::move(_newM);
        if ( ! module->addTypeInfoMacro(newM,true) ) {
            context->throw_error_at(at, "can't add type info macro %s to module %s", newM->name.c_str(), module->name.c_str());
        }
    }

    smart_ptr<VisitorAdapter> makeVisitor ( const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<VisitorAdapter>((char *)pClass,info,context);
    }

    PassMacroPtr makePassMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<PassMacroAdapter>(name,(char *)pClass,info,context);
    }

    VariantMacroPtr makeVariantMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<VariantMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleVariantMacro ( Module * module, VariantMacroPtr & _newM, Context * ) {
        VariantMacroPtr newM = das::move(_newM);
        module->variantMacros.push_back(newM);
    }

    ForLoopMacroPtr makeForLoopMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<ForLoopMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleForLoopMacro ( Module * module, ForLoopMacroPtr & _newM, Context * ) {
        ForLoopMacroPtr newM = das::move(_newM);
        module->forLoopMacros.push_back(newM);
    }

    CaptureMacroPtr makeCaptureMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<CaptureMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleCaptureMacro ( Module * module, CaptureMacroPtr & _newM, Context * ) {
        CaptureMacroPtr newM = das::move(_newM);
        module->captureMacros.push_back(newM);
    }

    SimulateMacroPtr makeSimulateMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<SimulateMacroAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleSimulateMacro ( Module * module, SimulateMacroPtr & _newM, Context * ) {
        SimulateMacroPtr newM = das::move(_newM);
        module->simulateMacros.push_back(newM);
    }

    void addModuleInferMacro ( Module * module, PassMacroPtr & _newM, Context * ) {
        PassMacroPtr newM = das::move(_newM);
        module->macros.push_back(newM);
    }

    void addModuleInferDirtyMacro ( Module * module, PassMacroPtr & _newM, Context * ) {
        PassMacroPtr newM = das::move(_newM);
        module->inferMacros.push_back(newM);
    }

    void addModuleLintMacro ( Module * module, PassMacroPtr & _newM, Context * ) {
        PassMacroPtr newM = das::move(_newM);
        module->lintMacros.push_back(newM);
    }

    void addModuleGlobalLintMacro ( Module * module, PassMacroPtr & _newM, Context * ) {
        PassMacroPtr newM = das::move(_newM);
        module->globalLintMacros.push_back(newM);
    }

    void addModuleOptimizationMacro ( Module * module, PassMacroPtr & _newM, Context * ) {
        PassMacroPtr newM = das::move(_newM);
        module->optimizationMacros.push_back(newM);
    }

    EnumerationAnnotationPtr makeEnumerationAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<EnumerationAnnotationAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleEnumerationAnnotation ( Module * module, EnumerationAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        EnumerationAnnotationPtr ann = das::move(_ann);
        if ( !module->addAnnotation(ann, true) ) {
            context->throw_error_at(at, "can't add enumeration annotation %s to module %s",
                ann->name.c_str(), module->name.c_str());
        }
    }

    int addEnumerationEntry ( smart_ptr<Enumeration> enu, const char* name ) {
        if ( !name ) return -1;
        for ( auto & ee : enu->list ) {
            if ( ee.name==name ) {
                return -1;
            }
        }
        enu->list.push_back(Enumeration::EnumEntry());
        int index = (int)enu->list.size() - 1;
        enu->list[index].name = name;
        return index;
    }

    StructureAnnotationPtr makeStructureAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<StructureAnnotationAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleStructureAnnotation ( Module * module, StructureAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        StructureAnnotationPtr ann = das::move(_ann);
        if ( !module->addAnnotation(ann, true) ) {
            context->throw_error_at(at, "can't add structure annotation %s to module %s",
                ann->name.c_str(), module->name.c_str());
        }
    }

    void addStructureStructureAnnotation ( smart_ptr_raw<Structure> st, StructureAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        StructureAnnotationPtr ann = das::move(_ann);
        string err;
        ModuleGroup dummy;
        if ( !ann->touch(st, dummy, AnnotationArgumentList(), err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to structure %s",
                ann->name.c_str(), st->name.c_str());
        }
        auto annDecl = make_smart<AnnotationDeclaration>();
        annDecl->annotation = ann;
        st->annotations.push_back(annDecl);
    }

    FunctionAnnotationPtr makeBlockAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<BlockAnnotationAdapter>(name,(char *)pClass,info,context);
    }

    FunctionAnnotationPtr makeFunctionAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<FunctionAnnotationAdapter>(name,(char *)pClass,info,context);
    }

    void addModuleFunctionAnnotation ( Module * module, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        FunctionAnnotationPtr ann = das::move(_ann);
        if ( !module->addAnnotation(ann, true) ) {
            context->throw_error_at(at, "can't add function annotation %s to module %s",
                ann->name.c_str(), module->name.c_str());
        }
    }

    void addFunctionFunctionAnnotation ( smart_ptr_raw<Function> func, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        FunctionAnnotationPtr ann = das::move(_ann);
        string err;
        ModuleGroup dummy;
        if ( !ann->apply(func, dummy, AnnotationArgumentList(), err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to function %s",
                ann->name.c_str(), func->name.c_str());
        }
        auto annDecl = make_smart<AnnotationDeclaration>();
        annDecl->annotation = ann;
        func->annotations.push_back(annDecl);
    }

    void addBlockBlockAnnotation ( smart_ptr_raw<ExprBlock> blk, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at ) {
        FunctionAnnotationPtr ann = das::move(_ann);
        string err;
        ModuleGroup dummy;
        if ( !ann->apply(blk.ptr, dummy, AnnotationArgumentList(), err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to block %s",
                ann->name.c_str(), blk->at.describe().c_str());
        }
        auto annDecl = make_smart<AnnotationDeclaration>();
        annDecl->annotation = ann;
        blk->annotations.push_back(annDecl);
    }

    void addAndApplyFunctionAnnotation ( smart_ptr_raw<Function> func, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at ) {
        string err;
        if (!ann->annotation->rtti_isFunctionAnnotation()) {
            context->throw_error_at(at, "annotation %s failed to apply to function %s, not a FunctionAnnotation",
                ann->annotation->name.c_str(), func->name.c_str());
        }
        auto fAnn = (FunctionAnnotation*)ann->annotation.get();
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !fAnn->apply(func, *program->thisModuleGroup, ann->arguments, err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to function %s",
                ann->annotation->name.c_str(), func->name.c_str());
        }
        func->annotations.push_back(ann);
    }

    void addAndApplyBlockAnnotation ( smart_ptr_raw<ExprBlock> blk, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at ) {
        string err;
        if (!ann->annotation->rtti_isFunctionAnnotation()) {
            context->throw_error_at(at, "annotation %s failed to apply to block %s, not a FunctionAnnotation",
                ann->annotation->name.c_str(), blk->at.describe().c_str());
        }
        auto fAnn = (FunctionAnnotation*)ann->annotation.get();
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !fAnn->apply(blk.ptr, *program->thisModuleGroup, ann->arguments, err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to block %s",
                ann->annotation->name.c_str(), blk->at.describe().c_str());
        }
        blk->annotations.push_back(ann);
    }

    void addAndApplyStructAnnotation ( smart_ptr_raw<Structure> st, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at ) {
        string err;
        if (!ann->annotation->rtti_isStructureAnnotation()) {
            context->throw_error_at(at, "annotation %s failed to apply to struct %s, not a StructureAnnotation",
                ann->annotation->name.c_str(), st->name.c_str());
        }
        auto stAnn = (StructureAnnotation*)ann->annotation.get();
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !stAnn->touch(st, *program->thisModuleGroup, ann->arguments, err) ) {
            context->throw_error_at(at, "annotation %s failed to apply to struct %s",
                ann->annotation->name.c_str(), st->name.c_str());
        }
        st->annotations.push_back(ann);
    }

    void astVisit ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info ) {
        if (!adapter)
            context->throw_error_at(line_info, "adapter is required");
        if (!program)
            context->throw_error_at(line_info, "program is required");
        program->visit(*adapter);
    }

    void astVisitModulesInOrder ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info ) {
        if (!adapter)
            context->throw_error_at(line_info, "adapter is required");
        if (!program)
            context->throw_error_at(line_info, "program is required");
        program->visitModulesInOrder(*adapter);
    }

    void astVisitFunction ( smart_ptr_raw<Function> func, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info ) {
        if (!adapter)
            context->throw_error_at(line_info, "adapter is required");
        if (!func)
            context->throw_error_at(line_info, "func is required");
        func->visit(*adapter);
    }

    smart_ptr_raw<Expression> astVisitExpression ( smart_ptr_raw<Expression> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info ) {
        if (!adapter)
            context->throw_error_at(line_info, "adapter is required");
        if (!expr)
            context->throw_error_at(line_info, "expr is required");
        smart_ptr<Expression> res = expr->visit(*adapter);
        if ( res.get()!=expr.get() ) {
            DAS_VERIFYF(res->use_count()==1,"visitor returns new value, refcount must be 1 or else there will be a leak");
            res->addRef();
        }
        return res;
    }

    void astVisitBlockFinally ( smart_ptr_raw<ExprBlock> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info ) {
        if (!adapter)
            context->throw_error_at(line_info, "adapter is required");
        if (!expr)
            context->throw_error_at(line_info, "expr is required");
        expr->visitFinally(*adapter);
    }

    void Module_Ast::registerAdapterAnnotations(ModuleLibrary & lib) {
        // visitor
        addAnnotation(make_smart<AstVisitorAdapterAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeVisitor)>(*this, lib,  "make_visitor",
            SideEffects::modifyExternal, "makeVisitor")
                ->args({"class","info","context"});
        addExtern<DAS_BIND_FUN(astVisit)>(*this, lib,  "visit",
            SideEffects::accessExternal, "astVisit")
                ->args({"program","adapter","context","line"});
        addExtern<DAS_BIND_FUN(astVisitModulesInOrder)>(*this, lib,  "visit_modules",
            SideEffects::accessExternal, "astVisitModules")
                ->args({"program","adapter","context","line"});
        addExtern<DAS_BIND_FUN(astVisitFunction)>(*this, lib,  "visit",
            SideEffects::accessExternal, "astVisitFunction")
                ->args({"function","adapter","context","line"});
        addExtern<DAS_BIND_FUN(astVisitExpression)>(*this, lib,  "visit",
            SideEffects::accessExternal, "astVisitExpression")
                ->args({"expression","adapter","context","line"});
        addExtern<DAS_BIND_FUN(astVisitBlockFinally)>(*this, lib,  "visit_finally",
            SideEffects::accessExternal, "astVisitBlockFinally")
                ->args({"expression","adapter","context","line"});
        // function annotation
        addAnnotation(make_smart<AstFunctionAnnotationAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeFunctionAnnotation)>(*this, lib,  "make_function_annotation",
            SideEffects::modifyExternal, "makeFunctionAnnotation")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(makeBlockAnnotation)>(*this, lib,  "make_block_annotation",
            SideEffects::modifyExternal, "makeBlockAnnotation")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleFunctionAnnotation)>(*this, lib,  "add_function_annotation",
            SideEffects::modifyExternal, "addModuleFunctionAnnotation")
                ->args({"module","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addFunctionFunctionAnnotation)>(*this, lib,  "add_function_annotation",
            SideEffects::modifyExternal, "addFunctionFunctionAnnotation")
                ->args({"function","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addAndApplyFunctionAnnotation)>(*this, lib,  "add_function_annotation",
            SideEffects::modifyExternal, "addAndApplyFunctionAnnotation")
                ->args({"function","annotation","context","at"});
        // block annotation
        addExtern<DAS_BIND_FUN(addBlockBlockAnnotation)>(*this, lib,  "add_block_annotation",
            SideEffects::modifyExternal, "addBlockBlockAnnotation")
                ->args({"block","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addAndApplyBlockAnnotation)>(*this, lib,  "add_block_annotation",
            SideEffects::modifyExternal, "addAndApplyBlockAnnotation")
                ->args({"block","annotation","context","at"});
        // structure annotation
        addAnnotation(make_smart<AstStructureAnnotationAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeStructureAnnotation)>(*this, lib,  "make_structure_annotation",
            SideEffects::modifyExternal, "makeStructureAnnotation")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleStructureAnnotation)>(*this, lib,  "add_structure_annotation",
            SideEffects::modifyExternal, "addModuleStructureAnnotation")
                ->args({"module","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addStructureStructureAnnotation)>(*this, lib,  "add_structure_annotation",
            SideEffects::modifyExternal, "addStructureStructureAnnotation")
                ->args({"structure","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addAndApplyStructAnnotation)>(*this, lib,  "add_structure_annotation",
            SideEffects::modifyExternal, "addAndApplyStructAnnotation")
                ->args({"structure","annotation","context","at"});
        // enumeration annotation
        addAnnotation(make_smart<AstEnumerationAnnotationAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeEnumerationAnnotation)>(*this, lib,  "make_enumeration_annotation",
            SideEffects::modifyExternal, "makeEnumerationAnnotation")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleEnumerationAnnotation)>(*this, lib,  "add_enumeration_annotation",
            SideEffects::modifyExternal, "addModuleEnumerationAnnotation")
                ->args({"module","annotation","context","at"});
        addExtern<DAS_BIND_FUN(addEnumerationEntry)>(*this, lib,  "add_enumeration_entry",
            SideEffects::modifyExternal, "addEnumerationEntry")
                ->args({"enum","name"});
        // pass macro
        addAnnotation(make_smart<AstPassMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makePassMacro)>(*this, lib,  "make_pass_macro",
            SideEffects::modifyExternal, "makePassMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleInferMacro)>(*this, lib,  "add_infer_macro",
            SideEffects::modifyExternal, "addModuleInferMacro")
                ->args({"module","annotation","context"});
        addExtern<DAS_BIND_FUN(addModuleInferDirtyMacro)>(*this, lib,  "add_dirty_infer_macro",
            SideEffects::modifyExternal, "addModuleInferDirtyMacro")
                ->args({"module","annotation","context"});
        // lint macro
        addExtern<DAS_BIND_FUN(addModuleLintMacro)>(*this, lib,  "add_lint_macro",
            SideEffects::modifyExternal, "addModuleLintMacro")
                ->args({"module","annotation","context"});
        addExtern<DAS_BIND_FUN(addModuleGlobalLintMacro)>(*this, lib,  "add_global_lint_macro",
            SideEffects::modifyExternal, "addModuleGlobalLintMacro")
                ->args({"module","annotation","context"});
        // optimization macro
        addExtern<DAS_BIND_FUN(addModuleOptimizationMacro)>(*this, lib,  "add_optimization_macro",
            SideEffects::modifyExternal, "addModuleOptimizationMacro")
                ->args({"module","annotation","context"});
        // reader macro
        addAnnotation(make_smart<AstReaderMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeReaderMacro)>(*this, lib,  "make_reader_macro",
            SideEffects::modifyExternal, "makeReaderMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleReaderMacro)>(*this, lib,  "add_reader_macro",
            SideEffects::modifyExternal, "addModuleReaderMacro")
                ->args({"module","annotation","context","at"});
        // comment reader
        addAnnotation(make_smart<AstCommentReaderAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeCommentReader)>(*this, lib,  "make_comment_reader",
            SideEffects::modifyExternal, "makeCommentReader")
                ->args({"class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleCommentReader)>(*this, lib,  "add_comment_reader",
            SideEffects::modifyExternal, "addModuleCommentReader")
                ->args({"module","reader","context","at"});
        // call macro
        addAnnotation(make_smart<AstCallMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeCallMacro)>(*this, lib,  "make_call_macro",
            SideEffects::modifyExternal, "makeCallMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleCallMacro)>(*this, lib,  "add_call_macro",
            SideEffects::modifyExternal, "addModuleCallMacro")
                ->args({"module","annotation","context","at"});
        // type info macro
        addExtern<DAS_BIND_FUN(makeTypeInfoMacro)>(*this, lib,  "make_typeinfo_macro",
            SideEffects::modifyExternal, "makeTypeInfoMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleTypeInfoMacro)>(*this, lib,  "add_typeinfo_macro",
            SideEffects::modifyExternal, "addModuleTypeInfoMacro")
                ->args({"module","annotation","context","at"});
        // variant macro
        addAnnotation(make_smart<AstVariantMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeVariantMacro)>(*this, lib,  "make_variant_macro",
            SideEffects::modifyExternal, "makeVariantMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleVariantMacro)>(*this, lib,  "add_variant_macro",
            SideEffects::modifyExternal, "addModuleVariantMacro")
                ->args({"module","annotation","context"});
        // for loop macro
        addAnnotation(make_smart<AstForLoopMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeForLoopMacro)>(*this, lib,  "make_for_loop_macro",
            SideEffects::modifyExternal, "makeForLoopMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleForLoopMacro)>(*this, lib,  "add_for_loop_macro",
            SideEffects::modifyExternal, "addModuleForLoopMacro")
                ->args({"module","annotation","context"});
        // capture macro
        addAnnotation(make_smart<AstCaptureMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeCaptureMacro)>(*this, lib,  "make_capture_macro",
            SideEffects::modifyExternal, "makeCaptureMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleCaptureMacro)>(*this, lib,  "add_capture_macro",
            SideEffects::modifyExternal, "addModuleCaptureMacro")
                ->args({"module","annotation","context"});
        // simulate macro macro
        addAnnotation(make_smart<AstSimulateMacroAnnotation>(lib));
        addExtern<DAS_BIND_FUN(makeSimulateMacro)>(*this, lib,  "make_simulate_macro",
            SideEffects::modifyExternal, "makeSimulateMacro")
                ->args({"name","class","info","context"});
        addExtern<DAS_BIND_FUN(addModuleSimulateMacro)>(*this, lib,  "add_simulate_macro",
            SideEffects::modifyExternal, "addModuleSimulateMacro")
                ->args({"module","annotation","context"});
    }
}
