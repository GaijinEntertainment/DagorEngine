#pragma once

#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <ctype.h>
#include <setjmp.h>
#include <algorithm>
#include "sqcompiler.h"
#include "sqlexer.h"
#include "sqvm.h"
#include "sqcompilationcontext.h"
#include "sqast.h"

namespace SQCompilation {

class SQParser
{
    enum SQExpressionContext {
        SQE_REGULAR = 0,
        SQE_IF,
        SQE_SWITCH,
        SQE_LOOP_CONDITION,
        SQE_FUNCTION_ARG,
        SQE_RVALUE,
        SQE_ARRAY_ELEM,
    };

    using FuncAttrFlagsType = uint8_t;
    enum FuncAttribute : FuncAttrFlagsType {
        FATTR_PURE = 0x01,
    };

    Arena *_astArena;

    template<typename N, typename ... Args>
    N *newNode(Args... args) {
        return new (arena()) N(args...);
    }

    SQChar *copyString(const SQChar *s) {
        size_t len = strlen(s);
        size_t memLen = (len + 1) * sizeof(SQChar);
        SQChar *buf = (SQChar *)arena()->allocate(memLen);
        memcpy(buf, s, memLen);
        return buf;
    }

    Id *newId(const SQChar *name) {
        Id *r = newNode<Id>(copyString(name));
        r->setLineEndPos(_lex._currentline);
        r->setColumnEndPos(_lex._currentcolumn);
        return r;
    }

    LiteralExpr *newStringLiteral(const SQChar *s) {
        LiteralExpr *r = newNode<LiteralExpr>(copyString(s));
        r->setLineEndPos(_lex._currentline);
        r->setColumnEndPos(_lex._currentcolumn);
        return r;
    }

    Arena *arena() { return _astArena; }

    template<typename N>
    N *copyCoordinates(const Node *from, N *to) {
      to->setLineStartPos(from->lineStart());
      to->setColumnStartPos(from->columnStart());
      to->setLineEndPos(from->lineEnd());
      to->setColumnEndPos(from->columnEnd());
      return to;
    }

    template<typename N>
    N *setCoordinates(N *to, SQInteger l, SQInteger c) {
      to->setLineStartPos(l);
      to->setColumnStartPos(c);
      to->setLineEndPos(_lex._currentline);
      to->setColumnEndPos(_lex._currentcolumn);
      return to;
    }


    SQInteger line() const { return _lex._tokenline; }
    SQInteger column() const { return _lex._tokencolumn; }
    SQInteger width() const { return _lex._currentcolumn - _lex._tokencolumn; }

    void checkSuspiciousUnaryOp(SQInteger prevTok, SQInteger tok, unsigned prevFlags);
    void checkSuspiciousBracket();
public:
    SQCompilationContext &_ctx;

    void reportDiagnostic(int32_t id, ...);

    uint32_t _depth;

    SQParser(SQVM *v, const char *sourceText, size_t sourceTextSize, const SQChar* sourcename, Arena *astArena, SQCompilationContext &ctx, Comments *comments);

    bool ProcessPosDirective();
    void Lex();

    void checkBraceIndentationStyle();

    void Consume(SQInteger tok) {
        assert(tok == _token);
        Lex();
    }

    Expr*   Expect(SQInteger tok);
    bool    IsEndOfStatement() {
        return ((_lex._prevtoken == _SC('\n')) || (_token == SQUIRREL_EOB) || (_token == _SC('}')) || (_token == _SC(';')))
            || (_token == TK_DIRECTIVE);
    }
    void    OptionalSemicolon();

    RootBlock*  parse();
    Block*      parseStatements();
    Statement*  parseStatement(bool closeframe = true);
    Expr*       parseCommaExpr(SQExpressionContext expression_context);
    Expr*       Expression(SQExpressionContext expression_context);

    template<typename T> Expr *BIN_EXP(T f, enum TreeOp top, Expr *lhs);

    Expr*   LogicalNullCoalesceExp();
    Expr*   LogicalOrExp();
    Expr*   LogicalAndExp();
    Expr*   BitwiseOrExp();
    Expr*   BitwiseXorExp();
    Expr*   BitwiseAndExp();
    Expr*   EqExp();
    Expr*   CompExp();
    Expr*   ShiftExp();
    Expr*   PlusExp();
    Expr*   MultExp();
    Expr*   PrefixedExpr();
    Expr*   Factor(SQInteger &pos);
    Expr*   UnaryOP(enum TreeOp op);

    void ParseTableOrClass(TableDecl *decl, SQInteger separator, SQInteger terminator);

    Decl* parseLocalDeclStatement();
    Decl *parseLocalFunctionDeclStmt(bool assignable);
    Decl *parseLocalClassDeclStmt(bool assignable);

    Statement* IfLikeBlock(bool &wrapped);
    IfStatement* parseIfStatement();
    WhileStatement* parseWhileStatement();
    DoWhileStatement* parseDoWhileStatement();
    ForStatement* parseForStatement();
    ForeachStatement* parseForEachStatement();
    SwitchStatement* parseSwitchStatement();
    Expr *parseStringTemplate();
    LiteralExpr* ExpectScalar();
    ConstDecl* parseConstStatement(bool global);
    EnumDecl* parseEnumStatement(bool global);
    TryStatement* parseTryCatchStatement();
    Id* generateSurrogateFunctionName();
    DeclExpr* FunctionExp(bool lambda);
    FuncAttrFlagsType ParseFunctionAttributes();
    ClassDecl* ClassExp(Expr *key);
    Expr* DeleteExpr();
    Expr* PrefixIncDec(SQInteger token);
    FunctionDecl* CreateFunction(Id *name, bool lambda = false, bool ctor = false);
    Statement* parseDirectiveStatement();
    void onDocString(const SQChar *doc_string);

    DocObject& getModuleDocObject() { return _moduleDocObject; }

private:
    ArenaVector<DocObject *> _docObjectStack;
    DocObject _moduleDocObject;
    SQInteger _token;
    const SQChar *_sourcename;
    SQLexer _lex;
    SQExpressionContext _expression_context;
    SQUnsignedInteger _lang_features;
    SQVM *_vm;
};

} // namespace SQCompilation

#endif
