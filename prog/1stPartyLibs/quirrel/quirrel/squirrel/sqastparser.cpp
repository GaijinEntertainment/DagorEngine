#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqopcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqcompiler.h"
#include "sqastparser.h"
#include "sqcompiler.h"
#include "sqcompilationcontext.h"
#include <stdarg.h>

namespace SQCompilation {

struct NestingChecker {
    SQParser *_p;
    const uint32_t _max_depth;
    uint32_t _depth;
    NestingChecker(SQParser *p) : _p(p), _depth(0), _max_depth(500) {
        inc();
    }

    ~NestingChecker() {
        _p->_depth -= _depth;
    }

    void inc() {
        if (_p->_depth > _max_depth) {
            _p->reportDiagnostic(DiagnosticsId::DI_TOO_BIG_AST);
        }
        _p->_depth += 1;
        _depth += 1;
    }
};

SQParser::SQParser(SQVM *v, const char *sourceText, size_t sourceTextSize, const SQChar* sourcename, Arena *astArena, SQCompilationContext &ctx)
    : _lex(_ss(v), ctx, LM_AST)
    , _ctx(ctx)
    , _astArena(astArena)
{
    _vm=v;
    _lex.Init(_ss(v), sourceText, sourceTextSize);
    _sourcename = sourcename;
    _expression_context = SQE_REGULAR;
    _lang_features = _ss(v)->defaultLangFeatures;
    _depth = 0;
    _token = 0;
}


void SQParser::reportDiagnostic(int32_t id, ...) {
    va_list vargs;
    va_start(vargs, id);

    _ctx.vreportDiagnostic((enum DiagnosticsId)id, line(), column(), width(), vargs);

    va_end(vargs);
}


bool SQParser::ProcessPosDirective()
{
    const SQChar *sval = _lex._svalue;
    if (strncmp(sval, _SC("pos:"), 4) != 0)
        return false;

    sval += 4;
    if (!isdigit(*sval))
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_LINENUM);
    SQChar * next = NULL;
    _lex._currentline = scstrtol(sval, &next, 10);
    if (!next || *next != ':') {
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ":");
        return false;
    }
    next++;
    if (!isdigit(*next))
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_COLNUM);
    _lex._currentcolumn = scstrtol(next, NULL, 10);
    return true;
}


Statement* SQParser::parseDirectiveStatement()
{
    const SQChar *sval = _lex._svalue;

    SQInteger setFlags = 0, clearFlags = 0;
    bool applyToDefault = false;
    if (strncmp(sval, _SC("default:"), 8) == 0) {
        applyToDefault = true;
        sval += 8;
    }

    if (strcmp(sval, _SC("strict")) == 0)
        setFlags = LF_STRICT;
    else if (strcmp(sval, _SC("relaxed")) == 0)
        clearFlags = LF_STRICT;
    else if (strcmp(sval, _SC("strict-bool")) == 0)
        setFlags = LF_STRICT_BOOL;
    else if (strcmp(sval, _SC("relaxed-bool")) == 0)
        clearFlags = LF_STRICT_BOOL;
    else if (strcmp(sval, _SC("no-plus-concat")) == 0)
        setFlags = LF_NO_PLUS_CONCAT;
    else if (strcmp(sval, _SC("allow-plus-concat")) == 0)
        clearFlags = LF_NO_PLUS_CONCAT;
    else if (strcmp(sval, _SC("forbid-root-table")) == 0)
        setFlags = LF_FORBID_ROOT_TABLE;
    else if (strcmp(sval, _SC("allow-root-table")) == 0)
        clearFlags = LF_FORBID_ROOT_TABLE;
    else if (strcmp(sval, _SC("disable-optimizer")) == 0)
        setFlags = LF_DISABLE_OPTIMIZER;
    else if (strcmp(sval, _SC("enable-optimizer")) == 0)
        clearFlags = LF_DISABLE_OPTIMIZER;
    else
        reportDiagnostic(DiagnosticsId::DI_UNSUPPORTED_DIRECTIVE, sval);

    DirectiveStmt *d = newNode<DirectiveStmt>();
    d->setLineStartPos(_lex._tokenline);
    d->setColumnStartPos(_lex._tokencolumn);
    d->setLineEndPos(_lex._currentline);
    d->setColumnEndPos(_lex._currentcolumn);

    d->applyToDefault = applyToDefault;
    d->setFlags = setFlags;
    d->clearFlags = clearFlags;

    _lang_features = (_lang_features | setFlags) & ~clearFlags;
    if (applyToDefault)
        _ss(_vm)->defaultLangFeatures = (_ss(_vm)->defaultLangFeatures | setFlags) & ~clearFlags;

    Lex();
    return d;
}


void SQParser::Lex()
{
    _token = _lex.Lex();

    while (_token == TK_DIRECTIVE)
    {
        bool endOfLine = (_lex._prevtoken == _SC('\n'));
        if (ProcessPosDirective()) {
            _token = _lex.Lex();
            if (endOfLine)
                _lex._prevtoken = _SC('\n');
        } else
            break;
    }
}

static const char *tok2Str(SQInteger tok) {
    static char s[3];
    snprintf(s, sizeof s, "%c", (SQChar)tok);
    return s;
}

Expr* SQParser::Expect(SQInteger tok)
{
    if(_token != tok) {
        if(_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
            //do nothing
        }
        else {
            const SQChar *etypename;
            if(tok > 255) {
                switch(tok)
                {
                case TK_IDENTIFIER:
                    etypename = _SC("IDENTIFIER");
                    break;
                case TK_STRING_LITERAL:
                    etypename = _SC("STRING_LITERAL");
                    break;
                case TK_INTEGER:
                    etypename = _SC("INTEGER");
                    break;
                case TK_FLOAT:
                    etypename = _SC("FLOAT");
                    break;
                default:
                    etypename = _lex.Tok2Str(tok);
                }
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, etypename);
            }
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, tok2Str(tok));
        }
    }
    Expr *ret = NULL;
    switch(tok)
    {
    case TK_IDENTIFIER:
        ret = newId(_lex._svalue);
        break;
    case TK_STRING_LITERAL:
        ret = newStringLiteral(_lex._svalue);
        break;
    case TK_INTEGER:
        ret = setCoordinates(newNode<LiteralExpr>(_lex._nvalue), line(), column());
        break;
    case TK_FLOAT:
        ret = setCoordinates(newNode<LiteralExpr>(_lex._fvalue), line(), column());
        break;
    }
    Lex();
    return ret;
}


void SQParser::OptionalSemicolon()
{
    if(_token == _SC(';')) { Lex(); return; }
    if(!IsEndOfStatement()) {
        reportDiagnostic(DiagnosticsId::DI_END_OF_STMT_EXPECTED);
    }
}


RootBlock* SQParser::parse()
{
    RootBlock *rootBlock = newNode<RootBlock>(arena());

    if(setjmp(_ctx.errorJump()) == 0) {
        Lex();
        rootBlock->setLineStartPos(line());
        rootBlock->setColumnStartPos(column());
        while(_token > 0){
            rootBlock->addStatement(parseStatement());
            if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
        }
        rootBlock->setLineEndPos(_lex._currentline);
        rootBlock->setColumnEndPos(_lex._currentcolumn);
    }
    else {
        return NULL;
    }

    return rootBlock;
}


Block* SQParser::parseStatements()
{
    NestingChecker nc(this);
    Block *result = newNode<Block>(arena());
    while(_token != _SC('}') && _token != TK_DEFAULT && _token != TK_CASE) {
        Statement *stmt = parseStatement();
        result->addStatement(stmt);
        if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
    }
    return result;
}


Statement* SQParser::parseStatement(bool closeframe)
{
    NestingChecker nc(this);
    Statement *result = NULL;
    SQInteger l = line(), c = column();

    switch(_token) {
    case _SC(';'):  result = newNode<EmptyStatement>(); Lex(); break;
    case TK_DIRECTIVE:  result = parseDirectiveStatement();    break;
    case TK_IF:     result = parseIfStatement();          break;
    case TK_WHILE:  result = parseWhileStatement();       break;
    case TK_DO:     result = parseDoWhileStatement();     break;
    case TK_FOR:    result = parseForStatement();         break;
    case TK_FOREACH:result = parseForEachStatement();     break;
    case TK_SWITCH: result = parseSwitchStatement();      break;
    case TK_LOCAL:
    case TK_LET:
        result = parseLocalDeclStatement();
        break;
    case TK_RETURN:
    case TK_YIELD: {
        SQOpcode op;
        if(_token == TK_RETURN) {
            op = _OP_RETURN;
        }
        else {
            op = _OP_YIELD;
        }
        SQInteger le = _lex._currentline;
        SQInteger ce = _lex._currentcolumn;
        Lex();

        Expr *arg = NULL;
        if(!IsEndOfStatement()) {
            arg = Expression(SQE_RVALUE);
        }

        result = op == _OP_RETURN ? static_cast<TerminateStatement *>(newNode<ReturnStatement>(arg)) : newNode<YieldStatement>(arg);
        if (arg) {
            copyCoordinates(arg, result);
        }
        else {
            result->setLineEndPos(le);
            result->setColumnEndPos(ce);
        }

        result->setLineStartPos(l);
        result->setColumnStartPos(c);

        return result;
    }
    case TK_BREAK:
        result = newNode<BreakStatement>(nullptr);
        Lex();
        break;
    case TK_CONTINUE:
        result = newNode<ContinueStatement>(nullptr);
        Lex();
        break;
    case TK_FUNCTION:
        result = parseLocalFunctionDeclStmt(false);
        break;
    case TK_CLASS:
        result = parseLocalClassDeclStmt(false);
        break;
    case TK_ENUM:
        result = parseEnumStatement(false);
        break;
    case _SC('{'):
    {
        SQUnsignedInteger savedLangFeatures = _lang_features;
        Lex();
        result = parseStatements();
        Expect(_SC('}'));
        _lang_features = savedLangFeatures;
        break;
    }
    case TK_TRY:
        result = parseTryCatchStatement();
        break;
    case TK_THROW: {
        Lex();
        Expr *e = Expression(SQE_RVALUE);
        result = copyCoordinates(e, newNode<ThrowStatement>(e));
        result->setLineStartPos(l); result->setColumnStartPos(c);
        return result;
    }
    case TK_CONST:
        result = parseConstStatement(false);
        break;
    case TK_GLOBAL:
        Lex();
        if (_token == TK_CONST)
            result = parseConstStatement(true);
        else if (_token == TK_ENUM)
            result = parseEnumStatement(true);
        else
            reportDiagnostic(DiagnosticsId::DI_GLOBAL_CONSTS_ONLY);
        break;
    default: {
        Expr *e = Expression(SQE_REGULAR);
        return copyCoordinates(e, newNode<ExprStatement>(e));
      }
    }

    setCoordinates(result, l, c);

    assert(result);
    return result;
}


Expr* SQParser::parseCommaExpr(SQExpressionContext expression_context)
{
    NestingChecker nc(this);
    Expr *expr = Expression(expression_context);

    if (_token == ',') {
        CommaExpr *cm = newNode<CommaExpr>(arena());
        cm->addExpression(expr);
        expr = cm;
        while (_token == ',') {
            Lex();
            cm->addExpression(Expression(expression_context));
        }
    }

    return expr;
}


Expr* SQParser::Expression(SQExpressionContext expression_context)
{
    NestingChecker nc(this);
    SQExpressionContext saved_expression_context = _expression_context;
    _expression_context = expression_context;

    Expr *expr = LogicalNullCoalesceExp();

    if (_token == TK_INEXPR_ASSIGNMENT && (expression_context == SQE_REGULAR || expression_context == SQE_FUNCTION_ARG))
        reportDiagnostic(DiagnosticsId::DI_INCORRECT_INTRA_ASSIGN);

    switch(_token)  {
    case _SC('='):
    case TK_INEXPR_ASSIGNMENT:
    case TK_NEWSLOT:
    case TK_MINUSEQ:
    case TK_PLUSEQ:
    case TK_MULEQ:
    case TK_DIVEQ:
    case TK_MODEQ: {
        SQInteger op = _token;
        Lex();
        Expr *e2 = Expression(SQE_RVALUE);

        switch (op) {
        case TK_NEWSLOT: expr = newNode<BinExpr>(TO_NEWSLOT, expr, e2); break;
        case TK_INEXPR_ASSIGNMENT:
        case _SC('='): //ASSIGN
            if (op == _SC('='))
                switch (expression_context)
                {
                case SQE_IF:
                    reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "if");
                    break;
                case SQE_LOOP_CONDITION:
                    reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "loop condition");
                    break;
                case SQE_SWITCH:
                    reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "switch");
                    break;
                case SQE_FUNCTION_ARG:
                    reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "function argument");
                    break;
                case SQE_RVALUE:
                    reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "expression");
                    break;
                case SQE_REGULAR:
                    break;
                }
            expr = newNode<BinExpr>(op == TK_INEXPR_ASSIGNMENT ? TO_INEXPR_ASSIGN : TO_ASSIGN, expr, e2);
            break;
        case TK_MINUSEQ: expr = newNode<BinExpr>(TO_MINUSEQ, expr, e2); break;
        case TK_PLUSEQ: expr = newNode<BinExpr>(TO_PLUSEQ, expr, e2); break;
        case TK_MULEQ: expr = newNode<BinExpr>(TO_MULEQ, expr, e2); break;
        case TK_DIVEQ: expr = newNode<BinExpr>(TO_DIVEQ, expr, e2); break;
        case TK_MODEQ: expr = newNode<BinExpr>(TO_MODEQ, expr, e2); break;
        }
    }
    break;
    case _SC('?'): {
        Consume('?');

        Expr *ifTrue = Expression(SQE_RVALUE);

        Expect(_SC(':'));

        Expr *ifFalse = Expression(SQE_RVALUE);

        expr = newNode<TerExpr>(expr, ifTrue, ifFalse);
    }
    break;
    }

    _expression_context = saved_expression_context;
    return expr;
}


template<typename T> Expr* SQParser::BIN_EXP(T f, enum TreeOp top, Expr *lhs)
{
    _expression_context = SQE_RVALUE;

    Lex();

    Expr *rhs = (this->*f)();

    return newNode<BinExpr>(top, lhs, rhs);
}


Expr* SQParser::LogicalNullCoalesceExp()
{
    NestingChecker nc(this);
    Expr *lhs = LogicalOrExp();
    for (;;) {
        nc.inc();
        if (_token == TK_NULLCOALESCE) {
            Lex();

            Expr *rhs = LogicalNullCoalesceExp();
            lhs = newNode<BinExpr>(TO_NULLC, lhs, rhs);
        }
        else return lhs;
    }
}


Expr* SQParser::LogicalOrExp()
{
    NestingChecker nc(this);
    Expr *lhs = LogicalAndExp();
    for (;;) {
        nc.inc();
        if (_token == TK_OR) {
            Lex();

            Expr *rhs = LogicalOrExp();
            lhs = newNode<BinExpr>(TO_OROR, lhs, rhs);
        }
        else return lhs;
    }
}

Expr* SQParser::LogicalAndExp()
{
    NestingChecker nc(this);
    Expr *lhs = BitwiseOrExp();
    for (;;) {
        nc.inc();
        switch (_token) {
        case TK_AND: {
            Lex();

            Expr *rhs = LogicalAndExp();
            lhs = newNode<BinExpr>(TO_ANDAND, lhs, rhs);
        }
        default:
            return lhs;
        }
    }
}

Expr* SQParser::BitwiseOrExp()
{
    NestingChecker nc(this);
    Expr *lhs = BitwiseXorExp();
    for (;;) {
        nc.inc();
        if (_token == _SC('|')) {
            return BIN_EXP(&SQParser::BitwiseOrExp, TO_OR, lhs);
        }
        else return lhs;
    }
}

Expr* SQParser::BitwiseXorExp()
{
    NestingChecker nc(this);
    Expr * lhs = BitwiseAndExp();
    for (;;) {
        nc.inc();
        if (_token == _SC('^')) {
            lhs = BIN_EXP(&SQParser::BitwiseAndExp, TO_XOR, lhs);
        }
        else return lhs;
    }
}

Expr* SQParser::BitwiseAndExp()
{
    NestingChecker nc(this);
    Expr *lhs = EqExp();
    for (;;) {
        nc.inc();
        if (_token == _SC('&')) {
            lhs = BIN_EXP(&SQParser::EqExp, TO_AND, lhs);
        }
        else return lhs;
    }
}

Expr* SQParser::EqExp()
{
    NestingChecker nc(this);
    Expr *lhs = CompExp();
    for (;;) {
        nc.inc();
        switch (_token) {
        case TK_EQ: lhs = BIN_EXP(&SQParser::CompExp, TO_EQ, lhs); break;
        case TK_NE: lhs = BIN_EXP(&SQParser::CompExp, TO_NE, lhs); break;
        case TK_3WAYSCMP: lhs = BIN_EXP(&SQParser::CompExp, TO_3CMP, lhs); break;
        default: return lhs;
        }
    }
}

Expr* SQParser::CompExp()
{
    NestingChecker nc(this);
    Expr *lhs = ShiftExp();
    for (;;) {
        nc.inc();
        switch (_token) {
        case _SC('>'): lhs = BIN_EXP(&SQParser::ShiftExp, TO_GT, lhs); break;
        case _SC('<'): lhs = BIN_EXP(&SQParser::ShiftExp, TO_LT, lhs); break;
        case TK_GE: lhs = BIN_EXP(&SQParser::ShiftExp, TO_GE, lhs); break;
        case TK_LE: lhs = BIN_EXP(&SQParser::ShiftExp, TO_LE, lhs); break;
        case TK_IN: lhs = BIN_EXP(&SQParser::ShiftExp, TO_IN, lhs); break;
        case TK_INSTANCEOF: lhs = BIN_EXP(&SQParser::ShiftExp, TO_INSTANCEOF, lhs); break;
        case TK_NOT: {
            Lex();
            SQInteger l = line(), c = column(), w = width();
            if (_token == TK_IN) {
                lhs = BIN_EXP(&SQParser::ShiftExp, TO_IN, lhs);
                lhs = setCoordinates(newNode<UnExpr>(TO_NOT, lhs), l, c);
            }
            else
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "in");
        }
        default: return lhs;
        }
    }
}

Expr* SQParser::ShiftExp()
{
    NestingChecker nc(this);
    Expr *lhs = PlusExp();
    for (;;) {
        nc.inc();
        switch (_token) {
        case TK_USHIFTR: lhs = BIN_EXP(&SQParser::PlusExp, TO_USHR, lhs); break;
        case TK_SHIFTL: lhs = BIN_EXP(&SQParser::PlusExp, TO_SHL, lhs); break;
        case TK_SHIFTR: lhs = BIN_EXP(&SQParser::PlusExp, TO_SHR, lhs); break;
        default: return lhs;
        }
    }
}

Expr* SQParser::PlusExp()
{
    NestingChecker nc(this);
    Expr *lhs = MultExp();
    for (;;) {
        nc.inc();
        switch (_token) {
        case _SC('+'): lhs = BIN_EXP(&SQParser::MultExp, TO_ADD, lhs); break;
        case _SC('-'): lhs = BIN_EXP(&SQParser::MultExp, TO_SUB, lhs); break;

        default: return lhs;
        }
    }
}

Expr* SQParser::MultExp()
{
    NestingChecker nc(this);
    Expr *lhs = PrefixedExpr();
    for (;;) {
        nc.inc();
        switch (_token) {
        case _SC('*'): lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_MUL, lhs); break;
        case _SC('/'): lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_DIV, lhs); break;
        case _SC('%'): lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_MOD, lhs); break;

        default: return lhs;
        }
    }
}

Expr* SQParser::PrefixedExpr()
{
    NestingChecker nc(this);
    //if 'pos' != -1 the previous variable is a local variable
    SQInteger pos;
    Expr *e = Factor(pos);
    bool nextIsNullable = false;
    for(;;) {
        nc.inc();
        switch(_token) {
        case _SC('.'):
        case TK_NULLGETSTR: {
            if (_token == TK_NULLGETSTR || nextIsNullable)
            {
                nextIsNullable = true;
            }
            Lex();

            SQInteger l = _lex._currentline, c = _lex._currentcolumn;
            Expr *receiver = e;
            Id *id = (Id *)Expect(TK_IDENTIFIER);
            assert(id);
            e = newNode<GetFieldExpr>(receiver, id->id(), nextIsNullable); //-V522
            e->setLineStartPos(receiver->lineStart()); e->setColumnStartPos(receiver->columnStart());
            e->setLineEndPos(l); e->setColumnEndPos(c);
            break;
        }
        case _SC('['):
        case TK_NULLGETOBJ: {
            if (_token == TK_NULLGETOBJ || nextIsNullable)
            {
                nextIsNullable = true;
            }
            if(_lex._prevtoken == _SC('\n'))
                reportDiagnostic(DiagnosticsId::DI_BROKEN_SLOT_DECLARATION);
            Lex();
            Expr *receiver = e;
            Expr *key = Expression(SQE_RVALUE);
            Expect(_SC(']'));
            e = setCoordinates(newNode<GetTableExpr>(receiver, key, nextIsNullable), receiver->lineStart(), receiver->columnStart());
            break;
        }
        case TK_MINUSMINUS:
        case TK_PLUSPLUS:
            {
                nextIsNullable = false;
                if(IsEndOfStatement()) return e;
                SQInteger diff = (_token==TK_MINUSMINUS) ? -1 : 1;
                Lex();
                e = setCoordinates(newNode<IncExpr>(e, diff, IF_POSTFIX), e->lineStart(), e->columnStart());
            }
            return e;
        case _SC('('):
        case TK_NULLCALL: {
            SQInteger nullcall = (_token==TK_NULLCALL || nextIsNullable);
            nextIsNullable = !!nullcall;
            SQInteger l = e->lineStart(), c = e->columnStart();
            CallExpr *call = newNode<CallExpr>(arena(), e, nullcall);
            Lex();
            while (_token != _SC(')')) {
                call->addArgument(Expression(SQE_FUNCTION_ARG));
                if (_token == _SC(',')) {
                    Lex();
                }
            }

            setCoordinates(call, l, c);

            Lex();
            e = call;
            break;
        }
        default: return e;
        }
    }
}

static void appendStringData(sqvector<SQChar> &dst, const SQChar *b) {
  while (*b) {
    dst.push_back(*b++);
  }
}

Expr *SQParser::parseStringTemplate() {

    // '$' TK_TEMPLATE_PREFIX? (arg TK_TEMPLATE_INFIX)* arg? TK_TEMPLATE_SUFFX

    _lex._state = LS_TEMPALTE;
    _lex._expectedToken = TK_TEMPLATE_PREFIX;

    SQInteger l = line(), c = column();

    Lex();
    int idx = 0;

    sqvector<SQChar> formatString(_ctx.allocContext());
    sqvector<Expr *> args(_ctx.allocContext());
    char buffer[64] = {0};

    SQInteger fmtL = line(), fmtC = column();
    SQInteger tok = -1;

    while ((tok = _token) != SQUIRREL_EOB) {

      if (tok != TK_TEMPLATE_PREFIX && tok != TK_TEMPLATE_INFIX && tok != TK_TEMPLATE_SUFFIX) {
          reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, _SC("STRING_LITERAL"));
          return nullptr;
      }

      appendStringData(formatString, _lex._svalue);
      if (tok != TK_TEMPLATE_SUFFIX) {
        snprintf(buffer, sizeof buffer, "%d", idx++);
        appendStringData(formatString, buffer);
        _lex._expectedToken = -1;
        _lex._state = LS_REGULAR;
        Lex();
        Expr *arg = Expression(SQE_FUNCTION_ARG);
        args.push_back(arg);

        if (_token != _SC('}')) {
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, _SC("}"));
            return nullptr;
        }

        formatString.push_back(_SC('}'));

        _lex._state = LS_TEMPALTE;
        _lex._expectedToken = TK_TEMPLATE_INFIX;
      }
      else {
        break;
      }
      Lex();
    }

    formatString.push_back('\0');

    Expr *result = nullptr;
    LiteralExpr *fmt = setCoordinates(newStringLiteral(&formatString[0]), fmtL, fmtC);

    if (args.empty()) {
      result = fmt;
    }
    else {
      Expr *callee = setCoordinates(newNode<GetFieldExpr>(fmt, "subst", false), l, c);
      CallExpr *call = setCoordinates(newNode<CallExpr>(arena(), callee, false), l, c);

      for (Expr *arg : args)
        call->addArgument(arg);

      result = call;
    }

    _lex._expectedToken = -1;
    _lex._state = LS_REGULAR;
    Lex();

    return result;
}

Expr* SQParser::Factor(SQInteger &pos)
{
    NestingChecker nc(this);
    Expr *r = NULL;

    SQInteger l = line(), c = column();

    switch(_token)
    {
    case TK_STRING_LITERAL:
        r = newStringLiteral(_lex._svalue);
        r->setLineStartPos(l); r->setColumnStartPos(c);
        Lex();
        break;
    case _SC('$'):
        r = parseStringTemplate();
        break;
    case TK_BASE:
        r = setCoordinates(newNode<BaseExpr>(), l, c);
        Lex();
        break;
    case TK_IDENTIFIER:
        r = newId(_lex._svalue);
        r->setLineStartPos(l); r->setColumnStartPos(c);
        Lex();
        break;
    case TK_CONSTRUCTOR:
        r = setCoordinates(newNode<Id>(_SC("constructor")), l, c);
        Lex();
        break;
    case TK_THIS: r = newNode<Id>(_SC("this")); Lex(); break;
    case TK_DOUBLE_COLON:  // "::"
        if (_lang_features & LF_FORBID_ROOT_TABLE)
            reportDiagnostic(DiagnosticsId::DI_ROOT_TABLE_FORBIDDEN);
        _token = _SC('.'); /* hack: drop into PrefixExpr, case '.'*/
        r = setCoordinates(newNode<RootExpr>(), l, c);
        break;
    case TK_NULL:
        r = setCoordinates(newNode<LiteralExpr>(), l, c);
        Lex();
        break;
    case TK_INTEGER:
        r = setCoordinates(newNode<LiteralExpr>(_lex._nvalue), l, c);
        Lex();
        break;
    case TK_FLOAT:
        r = setCoordinates(newNode<LiteralExpr>(_lex._fvalue), l, c);
        Lex();
        break;
    case TK_TRUE: case TK_FALSE:
        r = setCoordinates(newNode<LiteralExpr>((bool)(_token == TK_TRUE)), l, c);
        Lex();
        break;
    case _SC('['): {
            Lex();
            ArrayExpr *arr = newNode<ArrayExpr>(arena());
            while(_token != _SC(']')) {
                Expr *v = Expression(SQE_RVALUE);
                arr->addValue(v);
                if(_token == _SC(',')) Lex();
            }
            setCoordinates(arr, l, c);
            Lex();
            r = arr;
        }
        break;
    case _SC('{'): {
        Lex();
        TableDecl *t = newNode<TableDecl>(arena());
        ParseTableOrClass(t, _SC(','), _SC('}'));
        r = setCoordinates(newNode<DeclExpr>(t), l, c);
        break;
    }
    case TK_FUNCTION:
        r = FunctionExp(false);
        break;
    case _SC('@'):
        r = FunctionExp(true);
        break;
    case TK_CLASS: {
        Lex();
        Decl *classDecl = ClassExp(NULL);
        classDecl->setContext(DC_EXPR);
        r = setCoordinates(newNode<DeclExpr>(classDecl), l, c);
        break;
    }
    case _SC('-'):
        Lex();
        switch(_token) {
        case TK_INTEGER:
            r = setCoordinates(newNode<LiteralExpr>(-_lex._nvalue), l, c);
            Lex();
            break;
        case TK_FLOAT:
            r = setCoordinates(newNode<LiteralExpr>(-_lex._fvalue), l, c);
            Lex();
            break;
        default:
            r = setCoordinates(UnaryOP(TO_NEG), l, c);
            break;
        }
        break;
    case _SC('!'):
        Lex();
        r = setCoordinates(UnaryOP(TO_NOT), l, c);
        break;
    case _SC('~'):
        Lex();
        if(_token == TK_INTEGER)  {
            r = setCoordinates(newNode<LiteralExpr>(~_lex._nvalue), l, c);
            Lex();
        }
        else {
            r = setCoordinates(UnaryOP(TO_BNOT), l, c);
        }
        break;
    case TK_TYPEOF : Lex(); r = UnaryOP(TO_TYPEOF); break;
    case TK_RESUME : Lex(); r = UnaryOP(TO_RESUME); break;
    case TK_CLONE : Lex(); r = UnaryOP(TO_CLONE); break;
    case TK_MINUSMINUS :
    case TK_PLUSPLUS :
        r = setCoordinates(PrefixIncDec(_token), l, c);
        break;
    case TK_DELETE : r = DeleteExpr(); break;
    case _SC('('):
        Lex();
        r = setCoordinates(newNode<UnExpr>(TO_PAREN, Expression(_expression_context)), l, c);
        Expect(_SC(')'));
        break;
    case TK___LINE__:
        r = setCoordinates(newNode<LiteralExpr>(_lex._currentline), l, c);
        Lex();
        break;
    case TK___FILE__:
        r = setCoordinates(newNode<LiteralExpr>(_sourcename), l, c);
        Lex();
        break;
    default:
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "expression");
    }
    return r;
}

Expr* SQParser::UnaryOP(enum TreeOp op)
{
    NestingChecker nc(this);
    Expr *arg = PrefixedExpr();
    return newNode<UnExpr>(op, arg);
}

void SQParser::ParseTableOrClass(TableDecl *decl, SQInteger separator, SQInteger terminator)
{
    NestingChecker nc(this);
    NewObjectType otype = separator==_SC(',') ? NOT_TABLE : NOT_CLASS;

    while(_token != terminator) {
        SQInteger l = line();
        SQInteger c = column();
        unsigned flags = 0;
        //check if is an static
        if(otype == NOT_CLASS) {
            if(_token == TK_STATIC) {
                flags |= TMF_STATIC;
                Lex();
            }
        }

        switch (_token) {
        case TK_FUNCTION:
        case TK_CONSTRUCTOR: {
            SQInteger tk = _token;
            SQInteger l = line(), c = column();
            Lex();
            Id *funcName = tk == TK_FUNCTION ? (Id *)Expect(TK_IDENTIFIER) : newNode<Id>(_SC("constructor"));
            assert(funcName);
            LiteralExpr *key = newNode<LiteralExpr>(funcName->id()); //-V522
            Expect(_SC('('));
            FunctionDecl *f = CreateFunction(funcName, false, tk == TK_CONSTRUCTOR);
            DeclExpr *e = newNode<DeclExpr>(f);
            setCoordinates(e, l, c);
            decl->addMember(key, e, flags);
        }
        break;
        case _SC('['): {
            Lex();

            Expr *key = Expression(SQE_RVALUE); //-V522
            assert(key);
            Expect(_SC(']'));
            Expect(_SC('='));
            Expr *value = Expression(SQE_RVALUE);
            decl->addMember(key, value, flags | TMF_DYNAMIC_KEY);
            break;
        }
        case TK_STRING_LITERAL: //JSON
            if (otype == NOT_TABLE) { //only works for tables
                LiteralExpr *key = (LiteralExpr *)Expect(TK_STRING_LITERAL);  //-V522
                assert(key);
                Expect(_SC(':'));
                Expr *expr = Expression(SQE_RVALUE);
                decl->addMember(key, expr, flags | TMF_JSON);
                break;
            }  //-V796
        default: {
            Id *id = (Id *)setCoordinates(Expect(TK_IDENTIFIER), l, c);
            assert(id);
            LiteralExpr *key = newNode<LiteralExpr>(id->id()); //-V522
            copyCoordinates(id, key);
            if ((otype == NOT_TABLE) &&
                (_token == TK_IDENTIFIER || _token == separator || _token == terminator || _token == _SC('[')
                    || _token == TK_FUNCTION)) {
                decl->addMember(key, id, flags);
            }
            else {
                Expect(_SC('='));
                Expr *expr = Expression(SQE_RVALUE);
                decl->addMember(key, expr, flags);
            }
        }
        }
        if (_token == separator) Lex(); //optional comma/semicolon
    }

    Lex();
}

Decl *SQParser::parseLocalFunctionDeclStmt(bool assignable)
{
    SQInteger l = line(), c = column();

    assert(_token == TK_FUNCTION);
    Lex();

    Id *varname = (Id *)Expect(TK_IDENTIFIER);
    Expect(_SC('('));
    FunctionDecl *f = CreateFunction(varname, false);
    f->setLineStartPos(l); f->setColumnStartPos(c);
    f->setContext(DC_LOCAL);
    VarDecl *d = newNode<VarDecl>(varname->id(), copyCoordinates(f, newNode<DeclExpr>(f)), assignable); //-V522
    setCoordinates(d, l, c);
    return d;
}

Decl *SQParser::parseLocalClassDeclStmt(bool assignable)
{
    SQInteger l = line(), c = column();

    assert(_token == TK_CLASS);
    Lex();

    Id *varname = (Id *)Expect(TK_IDENTIFIER);
    ClassDecl *cls = ClassExp(NULL);
    cls->setLineStartPos(l); cls->setColumnStartPos(c);
    cls->setContext(DC_LOCAL);
    VarDecl *d = newNode<VarDecl>(varname->id(), copyCoordinates(cls, newNode<DeclExpr>(cls)), assignable); //-V522
    setCoordinates(d, l, c);
    return d;
}

Decl* SQParser::parseLocalDeclStatement()
{
    NestingChecker nc(this);

    assert(_token == TK_LET || _token == TK_LOCAL);
    bool assignable = _token == TK_LOCAL;
    Lex();

    if (_token == TK_FUNCTION) {
        return parseLocalFunctionDeclStmt(assignable);
    } else if (_token == TK_CLASS) {
        return parseLocalClassDeclStmt(assignable);
    }

    DeclGroup *decls = NULL;
    DestructuringDecl  *dd = NULL;
    Decl *decl = NULL;
    SQInteger destructurer = 0;

    if (_token == _SC('{') || _token == _SC('[')) {
        SQInteger l = line(), c = column();
        destructurer = _token;
        Lex();
        decls = dd = newNode<DestructuringDecl>(arena(), destructurer == _SC('{') ? DT_TABLE : DT_ARRAY);
        setCoordinates(dd, l, c);
    }

    do {
        SQInteger l = line();
        SQInteger c = column();
        Id *varname = (Id *)Expect(TK_IDENTIFIER);
        assert(varname);
        VarDecl *cur = NULL;
        if(_token == _SC('=')) {
            Lex();
            Expr *expr = Expression(SQE_REGULAR);
            cur = newNode<VarDecl>(varname->id(), expr, assignable, destructurer != 0); //-V522
        }
        else {
            if (!assignable && !destructurer)
                _ctx.reportDiagnostic(DiagnosticsId::DI_UNINITIALIZED_BINDING, varname->lineStart(), varname->columnStart(), varname->textWidth(), varname->id()); //-V522
            cur = newNode<VarDecl>(varname->id(), nullptr, assignable, destructurer != 0);
        }

        setCoordinates(cur, l, c);

        if (decls) {
            decls->addDeclaration(cur);
        } else if (decl) {
            decls = newNode<DeclGroup>(arena());
            decls->addDeclaration(static_cast<VarDecl *>(decl));
            decls->addDeclaration(cur);
            decl = decls;
        } else {
            decl = cur;
        }

        if (destructurer) {
            if (_token == _SC(',')) {
                Lex();
                if (_token == _SC(']') || _token == _SC('}'))
                    break;
            }
            else if (_token == TK_IDENTIFIER)
                continue;
            else
                break;
        }
        else {
            if (_token == _SC(','))
                Lex();
            else
                break;
        }
    } while(1);

    if (destructurer) {
        Expect(destructurer==_SC('[') ? _SC(']') : _SC('}'));
        Expect(_SC('='));
        assert(dd);
        dd->setExpression(Expression(SQE_RVALUE)); //-V522
        return dd;
    } else {
        return decls ? static_cast<Decl*>(decls) : decl;
    }
}

Statement* SQParser::IfBlock()
{
    NestingChecker nc(this);
    Statement *stmt = NULL;
    SQInteger l = line(), c = column();
    if (_token == _SC('{'))
    {
        Lex();
        stmt = setCoordinates(parseStatements(), l, c);
        Expect(_SC('}'));
    }
    else {
        stmt = parseStatement();
        Block *block = newNode<Block>(arena());
        block->addStatement(stmt);
        copyCoordinates(stmt, block);
        stmt = block;
        if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
    }

    return stmt;
}

IfStatement* SQParser::parseIfStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_IF);

    Expect(_SC('('));
    Expr *cond = Expression(SQE_IF);
    Expect(_SC(')'));

    Statement *thenB = IfBlock();
    Statement *elseB = NULL;
    if(_token == TK_ELSE){
        Lex();
        elseB = IfBlock();
    }

    return setCoordinates(newNode<IfStatement>(cond, thenB, elseB), l, c);
}

WhileStatement* SQParser::parseWhileStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_WHILE);

    Expect(_SC('('));
    Expr *cond = Expression(SQE_LOOP_CONDITION);
    Expect(_SC(')'));

    Statement *body = parseStatement();

    return setCoordinates(newNode<WhileStatement>(cond, body), l, c);
}

DoWhileStatement* SQParser::parseDoWhileStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_DO); // DO

    Statement *body = parseStatement();

    Expect(TK_WHILE);

    Expect(_SC('('));
    Expr *cond = Expression(SQE_LOOP_CONDITION);
    Expect(_SC(')'));

    return setCoordinates(newNode<DoWhileStatement>(body, cond), l, c);
}

ForStatement* SQParser::parseForStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_FOR);

    Expect(_SC('('));

    Node *init = NULL;
    if (_token == TK_LOCAL)
        init = parseLocalDeclStatement();
    else if (_token != _SC(';')) {
        init = parseCommaExpr(SQE_REGULAR);
    }
    Expect(_SC(';'));

    Expr *cond = NULL;
    if(_token != _SC(';')) {
        cond = Expression(SQE_LOOP_CONDITION);
    }
    Expect(_SC(';'));

    Expr *mod = NULL;
    if(_token != _SC(')')) {
        mod = parseCommaExpr(SQE_REGULAR);
    }
    Expect(_SC(')'));

    Statement *body = parseStatement();

    return setCoordinates(newNode<ForStatement>(init, cond, mod, body), l, c);
}

ForeachStatement* SQParser::parseForEachStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_FOREACH);

    Expect(_SC('('));
    Id *valname = (Id *)Expect(TK_IDENTIFIER);
    assert(valname);

    Id *idxname = NULL;
    if(_token == _SC(',')) {
        idxname = valname;
        Lex();
        valname = (Id *)Expect(TK_IDENTIFIER);
        assert(valname);

        if (strcmp(idxname->id(), valname->id()) == 0) //-V522
            _ctx.reportDiagnostic(DiagnosticsId::DI_SAME_FOREACH_KV_NAMES, valname->lineStart(), valname->columnStart(), valname->textWidth(), valname->id());
    }
    else {
        //idxname = newNode<Id>(_SC("@INDEX@"));
    }

    Expect(TK_IN);

    Expr *contnr = Expression(SQE_RVALUE);
    Expect(_SC(')'));

    Statement *body = parseStatement();

    VarDecl *idxDecl = idxname ? newNode<VarDecl>(idxname->id(), nullptr, false) : NULL;
    VarDecl *valDecl = valname ? newNode<VarDecl>(valname->id(), nullptr, false) : NULL;

    return setCoordinates(newNode<ForeachStatement>(idxDecl, valDecl, contnr, body), l, c);
}

SwitchStatement* SQParser::parseSwitchStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_SWITCH);

    Expect(_SC('('));
    Expr *switchExpr = Expression(SQE_SWITCH);
    Expect(_SC(')'));

    Expect(_SC('{'));

    SwitchStatement *switchStmt = newNode<SwitchStatement>(arena(), switchExpr);

    while(_token == TK_CASE) {
        Consume(TK_CASE);

        Expr *cond = Expression(SQE_RVALUE);
        Expect(_SC(':'));

        Statement *caseBody = parseStatements();
        switchStmt->addCases(cond, caseBody);
    }

    if(_token == TK_DEFAULT) {
        Consume(TK_DEFAULT);
        Expect(_SC(':'));

        switchStmt->addDefault(parseStatements());
    }

    Expect(_SC('}'));

    return setCoordinates(switchStmt, l, c);
}


LiteralExpr* SQParser::ExpectScalar()
{
    NestingChecker nc(this);
    LiteralExpr *ret = NULL;
    SQInteger l = line(), c = column();

    switch(_token) {
        case TK_INTEGER:
            ret = newNode<LiteralExpr>(_lex._nvalue);
            break;
        case TK_FLOAT:
            ret = newNode<LiteralExpr>(_lex._fvalue);
            break;
        case TK_STRING_LITERAL:
            ret = newStringLiteral(_lex._svalue);
            break;
        case TK_TRUE:
        case TK_FALSE:
            ret = newNode<LiteralExpr>((bool)(_token == TK_TRUE ? 1 : 0));
            break;
        case '-':
            Lex();
            switch(_token)
            {
            case TK_INTEGER:
                ret = newNode<LiteralExpr>(-_lex._nvalue);
            break;
            case TK_FLOAT:
                ret = newNode<LiteralExpr>(-_lex._fvalue);
            break;
            default:
                reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float");
            }
            break;
        default:
            reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float, or string");
    }

    setCoordinates(ret, l, c);

    Lex();
    return ret;
}


ConstDecl* SQParser::parseConstStatement(bool global)
{
    NestingChecker nc(this);
    SQInteger l = line(), c = column();
    Lex();
    Id *id = (Id *)Expect(TK_IDENTIFIER);

    Expect('=');
    LiteralExpr *valExpr = ExpectScalar();
    ConstDecl *d = setCoordinates(newNode<ConstDecl>(id->id(), valExpr, global), l, c); //-V522

    OptionalSemicolon();

    return d;
}


EnumDecl* SQParser::parseEnumStatement(bool global)
{
    NestingChecker nc(this);
    SQInteger l = line(), c = column();
    Lex();
    Id *id = (Id *)Expect(TK_IDENTIFIER);

    EnumDecl *decl = newNode<EnumDecl>(arena(), id->id(), global); //-V522

    Expect(_SC('{'));

    SQInteger nval = 0;
    while(_token != _SC('}')) {
        Id *key = (Id *)Expect(TK_IDENTIFIER);
        LiteralExpr *valExpr = NULL;
        if(_token == _SC('=')) {
            // TODO1: should it behave like C does?
            // TODO2: should float and string literal be allowed here?
            Lex();
            valExpr = ExpectScalar();
        }
        else {
            valExpr = newNode<LiteralExpr>(nval++);
        }

        decl->addConst(key->id(), valExpr); //-V522

        if(_token == ',') Lex();
    }

    setCoordinates(decl, l, c);
    Lex();

    return decl;
}


TryStatement* SQParser::parseTryCatchStatement()
{
    NestingChecker nc(this);

    SQInteger l = line(), c = column();

    Consume(TK_TRY);

    Statement *t = parseStatement();

    Expect(TK_CATCH);

    Expect(_SC('('));
    Id *exid = (Id *)Expect(TK_IDENTIFIER);
    Expect(_SC(')'));

    Statement *cth = parseStatement();

    return setCoordinates(newNode<TryStatement>(t, exid, cth), l, c);
}


Id* SQParser::generateSurrogateFunctionName()
{
    const SQChar * fileName = _sourcename ? _sourcename : _SC("unknown");
    int lineNum = int(_lex._currentline);

    const SQChar * rightSlash = std::max(strrchr(fileName, _SC('/')), strrchr(fileName, _SC('\\')));

    SQChar buf[MAX_FUNCTION_NAME_LEN];
    scsprintf(buf, MAX_FUNCTION_NAME_LEN, _SC("(%s:%d)"), rightSlash ? (rightSlash + 1) : fileName, lineNum);
    return newId(buf);
}


DeclExpr* SQParser::FunctionExp(bool lambda)
{
    NestingChecker nc(this);
    SQInteger l = line(), c = column();
    Lex();
    Id *funcName = (_token == TK_IDENTIFIER) ? (Id *)Expect(TK_IDENTIFIER) : generateSurrogateFunctionName();
    Expect(_SC('('));

    Decl *f = CreateFunction(funcName, lambda);
    f->setLineStartPos(l);
    f->setColumnStartPos(c);

    return setCoordinates(newNode<DeclExpr>(f), l, c);
}


ClassDecl* SQParser::ClassExp(Expr *key)
{
    NestingChecker nc(this);
    SQInteger l = line(), c = column();
    Expr *baseExpr = NULL;
    if(_token == TK_EXTENDS) {
        Lex();
        baseExpr = Expression(SQE_RVALUE);
    }
    Expect(_SC('{'));
    ClassDecl *d = newNode<ClassDecl>(arena(), key, baseExpr);
    ParseTableOrClass(d, _SC(';'),_SC('}'));
    return setCoordinates(d, l, c);
}


Expr* SQParser::DeleteExpr()
{
    NestingChecker nc(this);
    SQInteger l = line(), c = column();
    Consume(TK_DELETE);
    Expr *arg = PrefixedExpr();
    Expr *d = newNode<UnExpr>(TO_DELETE, arg);
    d->setLineStartPos(l);
    d->setColumnStartPos(c);
    return d;
}


Expr* SQParser::PrefixIncDec(SQInteger token)
{
    NestingChecker nc(this);
    SQInteger diff = (token==TK_MINUSMINUS) ? -1 : 1;
    Lex();
    Expr *arg = PrefixedExpr();
    return newNode<IncExpr>(arg, diff, IF_PREFIX);
}


FunctionDecl* SQParser::CreateFunction(Id *name, bool lambda, bool ctor)
{
    NestingChecker nc(this);
    FunctionDecl *f = ctor ? newNode<ConstructorDecl>(arena(), name->id()) : newNode<FunctionDecl>(arena(), name->id());
    f->setLineStartPos(line()); f->setColumnStartPos(column());

    f->addParameter(_SC("this"));

    SQInteger defparams = 0;

    auto &params = f->parameters();

    while (_token!=_SC(')')) {
        SQInteger l = line(), c = column();
        if (_token == TK_VARPARAMS) {
            if(defparams > 0)
                reportDiagnostic(DiagnosticsId::DI_VARARG_WITH_DEFAULT_ARG);
            f->addParameter(_SC("vargv"));
            f->setVararg(true);
            params.back()->setVararg();
            Lex();
            if(_token != _SC(')'))
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ")");
            break;
        }
        else {
            Id *paramname = (Id *)Expect(TK_IDENTIFIER);
            Expr *defVal = NULL;
            if (_token == _SC('=')) {
                Lex();
                defVal = Expression(SQE_RVALUE);
                defparams++;
            }
            else {
                if (defparams > 0)
                    reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "=");
            }

            f->addParameter(paramname->id(), defVal); //-V522
            ParamDecl *p = params.back();
            p->setLineStartPos(l); p->setColumnStartPos(c);
            p->setLineEndPos(_lex._currentline); p->setColumnEndPos(_lex._currentcolumn - 1);

            if(_token == _SC(',')) Lex();
            else if(_token != _SC(')'))
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ") or ,");
        }
    }

    Expect(_SC(')'));

    Block *body = NULL;
    SQInteger startLine = _lex._currentline;

    SQUnsignedInteger savedLangFeatures = _lang_features;

    if (lambda) {
        SQInteger line = _lex._prevtoken == _SC('\n') ? _lex._lasttokenline : _lex._currentline;
        Expr *expr = Expression(SQE_REGULAR);
        ReturnStatement *retStmt = newNode<ReturnStatement>(expr);
        copyCoordinates(expr, retStmt);
        retStmt->setIsLambda();
        body = newNode<Block>(arena());
        body->addStatement(retStmt);
        copyCoordinates(retStmt, body);
    }
    else {
        if (_token != '{')
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "{");
        body = (Block *)parseStatement(false);
    }
    SQInteger line2 = _lex._prevtoken == _SC('\n') ? _lex._lasttokenline : _lex._currentline;
    SQInteger column2 = _lex._prevtoken == _SC('\n') ? _lex._lasttokencolumn : _lex._currentcolumn;

    f->setBody(body);

    f->setSourceName(_sourcename);
    f->setLambda(lambda);

    f->setLineEndPos(line2);
    f->setColumnEndPos(column2);

    _lang_features = savedLangFeatures;

    return f;
}

} // namespace SQCompilation

#endif
