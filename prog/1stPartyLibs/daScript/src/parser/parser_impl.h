#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

#include "parser_state.h"

typedef void * yyscan_t;

namespace das {

    enum {
        OVERRIDE_NONE,
        OVERRIDE_OVERRIDE,
        OVERRIDE_SEALED,
    };

    struct VariableNameAndPosition {
        string          name;
        string          aka;
        LineInfo        at;
        ExpressionPtr   tag;
    };

    struct VariableDeclaration {
        VariableDeclaration ( vector<string> * n, const LineInfo & at, TypeDecl * t, Expression * i )
            : pNameList(nullptr), pTypeDecl(t), pInit(i) {
            pNameList = new vector<VariableNameAndPosition>;
            TextWriter ss;
            bool first = true;
            for ( auto & name : *n ) {
                if ( first ) first = false; else ss << "`";
                ss << name;
            }
            pNameList->push_back({ss.str(), "", at, nullptr});
            delete n;
        }
        VariableDeclaration ( vector<VariableNameAndPosition> * n, TypeDecl * t, Expression * i )
            : pNameList(n), pTypeDecl(t), pInit(i) {}
        virtual ~VariableDeclaration () {
            if ( pNameList ) delete pNameList;
            delete pTypeDecl;
            if ( pInit ) delete pInit;
            if ( annotation ) delete annotation;
        }
        vector<VariableNameAndPosition>   *pNameList;
        TypeDecl                *pTypeDecl = nullptr;
        Expression              *pInit = nullptr;
        bool                    init_via_move = false;
        bool                    init_via_clone = false;
        bool                    override = false;
        bool                    sealed = false;
        bool                    isPrivate = false;
        bool                    isStatic = false;
        bool                    isTupleExpansion = false;
        bool                    isClassMethod = false;
        AnnotationArgumentList  *annotation = nullptr;
    };

    void das_yyerror ( yyscan_t scanner, const string & error, const LineInfo & at, CompilationError cerr );
    void das2_yyerror ( yyscan_t scanner, const string & error, const LineInfo & at, CompilationError cerr );
    void das_checkName ( yyscan_t scanner, const string & name, const LineInfo &at );

    vector<ExpressionPtr> sequenceToList ( Expression * arguments );
    vector<ExpressionPtr> typesAndSequenceToList  ( vector<Expression *> * declL, Expression * arguments );
    Expression * sequenceToTuple ( Expression * arguments );
    ExprLooksLikeCall * parseFunctionArguments ( ExprLooksLikeCall * pCall, Expression * arguments );
    void deleteVariableDeclarationList ( vector<VariableDeclaration *> * list );
    void deleteTypeDeclarationList ( vector<Expression *> * list );
    void varDeclToTypeDecl ( yyscan_t scanner, TypeDecl * pType, vector<VariableDeclaration*> * list, bool needNames = true );
    Annotation * findAnnotation ( yyscan_t scanner, const string & name, const LineInfo & at );
    void runFunctionAnnotations ( yyscan_t scanner, DasParserState * extra, Function * func, AnnotationList * annL, const LineInfo & at );
    void appendDimExpr ( TypeDecl * typeDecl, Expression * dimExpr );
    void implAddGenericFunction ( yyscan_t scanner, Function * func );
    Expression * ast_arrayComprehension (yyscan_t scanner, const LineInfo & loc, vector<VariableNameAndPosition> * iters,
        Expression * srcs, Expression * subexpr, Expression * where, const LineInfo & forend, bool genSyntax, bool tableSyntax );
    Structure * ast_structureName ( yyscan_t scanner, bool sealed, string * name, const LineInfo & atName,
        string * parent, const LineInfo & atParent );
    void ast_structureDeclaration (  yyscan_t scanner, AnnotationList * annL, const LineInfo & loc, Structure * ps,
        const LineInfo & atPs, vector<VariableDeclaration*> * list );
    Enumeration * ast_addEmptyEnum ( yyscan_t scanner, string * name, const LineInfo & atName );
    void ast_enumDeclaration (  yyscan_t scanner, AnnotationList * annL, const LineInfo & atannL, bool pubE, Enumeration * pEnum, Enumeration * pE, Type ebt );
    void ast_globalLetList (  yyscan_t scanner, bool kwd_let, bool glob_shar, bool pub_var, vector<VariableDeclaration*> * list );
    void ast_globalLet (  yyscan_t scanner, bool kwd_let, bool glob_shar, bool pub_var, AnnotationArgumentList * ann, VariableDeclaration * decl );
    vector<VariableDeclaration*> * ast_structVarDefAbstract ( yyscan_t scanner, vector<VariableDeclaration*> * list,
        AnnotationList * annL, bool isPrivate, bool cnst, Function * func );
    vector<VariableDeclaration*> * ast_structVarDef ( yyscan_t scanner, vector<VariableDeclaration*> * list,
        AnnotationList * annL, bool isStatic, bool isPrivate, int ovr, bool cnst, Function * func, Expression * block,
            const LineInfo & fromBlock, const LineInfo & annLAt );
    Expression * ast_NameName ( yyscan_t scanner, string * ena, string * eni, const LineInfo & enaAt, const LineInfo & eniAt );
    Expression * ast_makeBlock ( yyscan_t scanner, int bal, AnnotationList * annL, vector<CaptureEntry> * clist,
        vector<VariableDeclaration*> * list, TypeDecl * result, Expression * block, const LineInfo & blockAt, const LineInfo & annLAt );
    Expression * ast_Let ( yyscan_t scanner, bool kwd_let, bool inScope, VariableDeclaration * decl, const LineInfo & kwd_letAt, const LineInfo & declAt );
    Function * ast_functionDeclarationHeader ( yyscan_t scanner, string * name, vector<VariableDeclaration*> * list,
        TypeDecl * result, const LineInfo & nameAt );
    void ast_requireModule ( yyscan_t scanner, string * name, string * modalias, bool pub, const LineInfo & atName );
    Expression * ast_forLoop ( yyscan_t scanner,  vector<VariableNameAndPosition> * iters, Expression * srcs,
        Expression * block, const LineInfo & locAt, const LineInfo & blockAt );
    AnnotationArgumentList * ast_annotationArgumentListEntry ( yyscan_t scanner, AnnotationArgumentList * argL, AnnotationArgument * arg );
    AnnotationArgumentList * ast_annotationArgumentListEntry ( yyscan_t, AnnotationArgumentList * argL, AnnotationArgument * arg );
    Expression * ast_lpipe ( yyscan_t scanner, Expression * fncall, Expression * arg, const LineInfo & locAt );
    Expression * ast_rpipe ( yyscan_t scanner, Expression * arg, Expression * fncall, const LineInfo & locAt );
    Expression * ast_makeGenerator ( yyscan_t scanner, TypeDecl * typeDecl, vector<CaptureEntry> * clist, Expression * subexpr, const LineInfo & locAt );
    ExprBlock * ast_wrapInBlock ( Expression * expr );
    int skip_underscode ( char * tok, char * buf, char * bufend );
    Expression * ast_makeStructToMakeVariant ( MakeStruct * decl, const LineInfo & locAt );
}