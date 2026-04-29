#pragma once

#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <algorithm>
#include "lexer.h"
#include "lex_tokens.h"
#include "sqvm.h"
#include "compilationcontext.h"
#include "ast.h"

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
        FATTR_NODISCARD = 0x02,
    };

    Arena *_astArena;

    template<typename N, typename ... Args>
    N *newNode(Args&&... args) const {
        return new (arena()) N(std::forward<Args>(args)...);
    }

    char *copyString(const char *s) const {
        size_t len = strlen(s);
        size_t memLen = (len + 1) * sizeof(char);
        char *buf = (char *)arena()->allocate(memLen);
        memcpy(buf, s, memLen);
        return buf;
    }

    Id *newId(const char *name) const {
        return newNode<Id>(_lex.tokenSpan(), copyString(name));
    }

    LiteralExpr *newStringLiteral(const char *s) const {
        return newNode<LiteralExpr>(_lex.tokenSpan(), copyString(s));
    }

    Arena *arena() const { return _astArena; }

    void checkSuspiciousUnaryOp(SQInteger prevTok, SQInteger tok, unsigned prevFlags);
    void checkSuspiciousBracket();
public:
    SQCompilationContext &_ctx;

    void reportDiagnostic(int32_t id, ...);

    uint32_t _depth;

    SQParser(SQVM *v, const char *sourceText, size_t sourceTextSize, const char* sourcename, Arena *astArena, SQCompilationContext &ctx, Comments *comments);

    bool ProcessPosDirective();
    void Lex();

    void checkBraceIndentationStyle();

    void Consume(SQInteger tok) {
        assert(tok == _token);
        Lex();
    }

    Expr*   Expect(SQInteger tok);
    bool    IsEndOfStatement() const {
        return ((_lex._prevtoken == '\n') || (_token == SQUIRREL_EOB) || (_token == '}') || (_token == ';'))
            || (_token == TK_DIRECTIVE);
    }
    void    OptionalSemicolon();

    RootBlock*  parse();
    Block*      parseStatements(SourceLoc start);
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

    void ParseTableOrClass(TableExpr *decl, SQInteger separator, SQInteger terminator);

    Decl *parseLocalDeclStatement(bool onlySingleVariable = false);
    Decl *parseLocalFunctionExprStmt(bool assignable, SourceLoc keywordStart);
    Decl *parseLocalClassExprStmt(bool assignable, SourceLoc keywordStart);

    Statement* IfLikeBlock(bool &wrapped);
    Statement* parseIfStatement();
    WhileStatement* parseWhileStatement();
    DoWhileStatement* parseDoWhileStatement();
    ForStatement* parseForStatement();
    ForeachStatement* parseForEachStatement();
    SwitchStatement* parseSwitchStatement();
    Expr *parseStringTemplate();
    unsigned parseTypeMask(bool eol_breaks_type_parsing);
    LiteralExpr* ExpectScalar();
    ConstDecl* parseConstStatement(bool global, SourceLoc globalStart = SourceLoc::invalid());
    ConstDecl* parseConstFunctionExprStmt(bool global, SourceLoc globalStart = SourceLoc::invalid());
    EnumDecl* parseEnumStatement(bool global, SourceLoc globalStart = SourceLoc::invalid());
    TryStatement* parseTryCatchStatement();
    Id* generateSurrogateFunctionName();
    FunctionExpr* FunctionExp(bool lambda);
    FuncAttrFlagsType ParseFunctionAttributes();
    ClassExpr* ClassExp(SourceLoc classStart, Expr *key);
    Expr* DeleteExpr();
    Expr* PrefixIncDec(SQInteger token);
    FunctionExpr* CreateFunction(SourceLoc start, Id *name, bool lambda = false, bool ctor = false);
    Statement* parseDirectiveStatement();
    ImportStmt* parseImportStatement();
    void onDocString(const char *doc_string);

    DocObject& getModuleDocObject() { return _moduleDocObject; }

private:
    ArenaVector<DocObject *> _docObjectStack;
    DocObject _moduleDocObject;
    SQInteger _token;
    const char *_sourcename;
    SQLexer _lex;
    SQExpressionContext _expression_context;
    SQUnsignedInteger _lang_features;
    SQVM *_vm;
    bool _are_imports_still_allowed = true;
};

} // namespace SQCompilation

#endif
