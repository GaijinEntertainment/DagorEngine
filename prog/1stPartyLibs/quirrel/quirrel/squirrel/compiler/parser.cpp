#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "opcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "parser.h"
#include "compiler.h"
#include "compilationcontext.h"
#include "sqtypeparser.h"
#include <stdarg.h>
#include <sq_char_class.h>

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

SQParser::SQParser(SQVM *v, const char *sourceText, size_t sourceTextSize, const char* sourcename, Arena *astArena, SQCompilationContext &ctx, Comments *comments)
    : _lex(_ss(v), ctx, comments)
    , _ctx(ctx)
    , _astArena(astArena)
    , _docObjectStack(astArena)
{
    _vm=v;
    _lex.Init(sourceText, sourceTextSize);
    _sourcename = sourcename;
    _expression_context = SQE_REGULAR;
    _lang_features = _ss(v)->defaultLangFeatures;
    _depth = 0;
    _token = 0;
    _docObjectStack.push_back(&_moduleDocObject);
}


void SQParser::reportDiagnostic(int32_t id, ...) {
    va_list vargs;
    va_start(vargs, id);

    SourceSpan span = _lex.tokenSpan();
    _ctx.vreportDiagnostic((enum DiagnosticsId)id, span.start.line, span.start.column, span.textWidth(), vargs);

    va_end(vargs);
}


bool SQParser::ProcessPosDirective()
{
    const char *sval = _lex._svalue;
    if (strncmp(sval, "pos:", 4) != 0)
        return false;

    sval += 4;
    if (!sq_isdigit(*sval))
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_LINENUM);
    char * next = NULL;
    _lex._currentline = scstrtol(sval, &next, 10);
    if (!next || *next != ':') {
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ":");
        return false;
    }
    next++;
    if (!sq_isdigit(*next))
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_COLNUM);
    _lex._currentcolumn = scstrtol(next, NULL, 10);
    return true;
}

struct SQPragmaDescriptor {
  const char *id;
  SQInteger setFlags, clearFlags;
};

static const SQPragmaDescriptor pragmaDescriptors[] = {
  { "strict", LF_STRICT, 0 },
  { "relaxed", 0, LF_STRICT },
  { "forbid-root-table", LF_FORBID_ROOT_TABLE, 0 },
  { "allow-root-table", 0, LF_FORBID_ROOT_TABLE },
  { "disable-optimizer", LF_DISABLE_OPTIMIZER, 0 },
  { "enable-optimizer", 0, LF_DISABLE_OPTIMIZER },
  { "forbid-delete-operator", LF_FORBID_DELETE_OP, 0 },
  { "allow-delete-operator", 0, LF_FORBID_DELETE_OP },
  { "forbid-clone-operator", LF_FORBID_CLONE_OP, 0 },
  { "allow-clone-operator", 0, LF_FORBID_CLONE_OP },
  { "forbid-switch-statement", LF_FORBID_SWITCH_STMT, 0 },
  { "allow-switch-statement", 0, LF_FORBID_SWITCH_STMT },
  { "forbid-implicit-type-methods", LF_FORBID_IMPLICIT_TYPE_METHODS, 0 },
  { "allow-implicit-type-methods", 0, LF_FORBID_IMPLICIT_TYPE_METHODS },
  { "forbid-auto-freeze", 0, LF_ALLOW_AUTO_FREEZE },
  { "allow-auto-freeze", LF_ALLOW_AUTO_FREEZE, 0 },
  { "forbid-compiler-internals", 0, LF_ALLOW_COMPILER_INTERNALS },
  { "allow-compiler-internals", LF_ALLOW_COMPILER_INTERNALS, 0 },
};

Statement* SQParser::parseDirectiveStatement()
{
    const char *sval = _lex._svalue;

    bool applyToDefault = false;
    if (strncmp(sval, "default:", 8) == 0) {
        applyToDefault = true;
        sval += 8;
    }

    const SQPragmaDescriptor *pragmaDesc = nullptr;

    for (const auto &desc : pragmaDescriptors) {
      if (strcmp(sval, desc.id) == 0) {
        pragmaDesc = &desc;
        break;
      }
    }

    if (pragmaDesc == nullptr) {
      reportDiagnostic(DiagnosticsId::DI_UNSUPPORTED_DIRECTIVE, sval);
      return nullptr;
    }

    SQInteger setFlags = pragmaDesc->setFlags, clearFlags = pragmaDesc->clearFlags;

    DirectiveStmt *d = newNode<DirectiveStmt>(_lex.tokenSpan());
    d->applyToDefault = applyToDefault;
    d->setFlags = setFlags;
    d->clearFlags = clearFlags;

    _lang_features = (_lang_features | setFlags) & ~clearFlags;
    if (applyToDefault)
        _ss(_vm)->defaultLangFeatures = (_ss(_vm)->defaultLangFeatures | setFlags) & ~clearFlags;

    Lex();
    return d;
}

void SQParser::checkBraceIndentationStyle()
{
  if (_token == '{' && (_lex._prevflags & TF_PREP_EOL))
    reportDiagnostic(DiagnosticsId::DI_EGYPT_BRACES);
}

void SQParser::Lex()
{
    _token = _lex.Lex();

    while (_token == TK_DIRECTIVE)
    {
        bool endOfLine = (_lex._prevtoken == '\n');
        if (ProcessPosDirective()) {
            _token = _lex.Lex();
            if (endOfLine)
                _lex._prevtoken = '\n';
        } else
            break;
    }

    const bool forceIdentifier =
           (_token == TK_CLONE && (_lang_features & LF_FORBID_CLONE_OP))
        || ((_token == TK_SWITCH || _token == TK_CASE || _token == TK_DEFAULT) && (_lang_features & LF_FORBID_SWITCH_STMT));

    if (forceIdentifier) {
      _token = TK_IDENTIFIER;
      _lex.SetStringValue();
    }
}


Expr* SQParser::Expect(SQInteger tok)
{
    if(_token != tok) {
        if(_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
            //do nothing
        }
        else {
            if(tok > 255) {
                const char *etypename;
                switch(tok)
                {
                case TK_IDENTIFIER:
                    etypename = "IDENTIFIER";
                    break;
                case TK_STRING_LITERAL:
                    etypename = "STRING_LITERAL";
                    break;
                case TK_INTEGER:
                    etypename = "INTEGER";
                    break;
                case TK_FLOAT:
                    etypename = "FLOAT";
                    break;
                default:
                    etypename = _lex.Tok2Str(tok);
                }
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, etypename);
            }
            else {
                char s[2] = {(char)tok, 0};
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, s);
            }
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
        ret = newNode<LiteralExpr>(_lex.tokenSpan(), _lex._nvalue);
        break;
    case TK_FLOAT:
        ret = newNode<LiteralExpr>(_lex.tokenSpan(), _lex._fvalue);
        break;
    }
    Lex();
    return ret;
}


void SQParser::OptionalSemicolon()
{
    if(_token == ';') { Lex(); return; }
    if(!IsEndOfStatement()) {
        reportDiagnostic(DiagnosticsId::DI_END_OF_STMT_EXPECTED);
    }
}


RootBlock* SQParser::parse()
{
    try {
        Lex();
        SourceLoc start = _lex.tokenStart();
        RootBlock *rootBlock = newNode<RootBlock>(arena(), start);
        while(_token > 0){
            rootBlock->addStatement(parseStatement());
            if(_lex._prevtoken != '}' && _lex._prevtoken != ';') OptionalSemicolon();
        }
        rootBlock->setSpanEnd(_lex.currentPos());
        return rootBlock;
    }
    catch (SQCompilation::CompilerError &) {
        return NULL;
    }
}


Block* SQParser::parseStatements(SourceLoc start)
{
    NestingChecker nc(this);
    Block *result = newNode<Block>(arena(), start);
    while(_token != '}' && _token != TK_DEFAULT && _token != TK_CASE) {
        Statement *stmt = parseStatement();
        result->addStatement(stmt);
        if(_lex._prevtoken != '}' && _lex._prevtoken != ';') OptionalSemicolon();
    }
    result->setSpanEnd(_lex.currentPos());
    return result;
}


void SQParser::onDocString(const char *doc_string)
{
    if (_docObjectStack.back()->getDocString() != nullptr)
        reportDiagnostic(DiagnosticsId::DI_MULTIPLE_DOCSTRINGS);
    else {
        SQString *docStringCopy = SQString::Create(_ss(_vm), doc_string);
        if (docStringCopy)
            _docObjectStack.back()->setDocString(docStringCopy->_val);
    }
}


Statement* SQParser::parseStatement(bool closeframe)
{
    NestingChecker nc(this);
    Statement *result = NULL;

    bool allowsImport = false;

    switch(_token) {
    case ';':
        allowsImport = true;
        result = newNode<EmptyStatement>(_lex.tokenSpan());
        Lex();
        break;
    case TK_DOCSTRING: {
        SourceSpan span = _lex.tokenSpan();
        onDocString(_lex._svalue);
        result = newNode<EmptyStatement>(span);
        Lex();
        break;
    }
    case TK_DIRECTIVE:
        allowsImport = true;
        result = parseDirectiveStatement();
        break;
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
        bool isReturn = (_token == TK_RETURN);
        SourceSpan keywordSpan = _lex.tokenSpan();
        Lex();

        Expr *arg = NULL;
        if(!IsEndOfStatement()) {
            arg = Expression(SQE_RVALUE);
        }

        if (isReturn) {
            return newNode<ReturnStatement>(keywordSpan, arg);
        } else {
            return newNode<YieldStatement>(keywordSpan, arg);
        }
    }
    case TK_BREAK: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        return newNode<BreakStatement>(span, nullptr);
    }
    case TK_CONTINUE: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        return newNode<ContinueStatement>(span, nullptr);
    }
    case TK_FUNCTION: {
        SourceLoc funcStart = _lex.tokenStart();
        result = parseLocalFunctionExprStmt(false, funcStart);
        break;
    }
    case TK_CLASS: {
        SourceLoc classStart = _lex.tokenStart();
        result = parseLocalClassExprStmt(false, classStart);
        break;
    }
    case TK_ENUM:
        result = parseEnumStatement(false);
        break;
    case '{':
    {
        SQUnsignedInteger savedLangFeatures = _lang_features;
        SourceLoc blockStart = _lex.tokenStart();
        Lex();
        Block *block = parseStatements(blockStart);
        block->setSpanEnd(_lex.currentPos());
        Expect('}');
        _lang_features = savedLangFeatures;
        result = block;
        break;
    }
    case TK_TRY:
        result = parseTryCatchStatement();
        break;
    case TK_THROW: {
        SourceSpan keywordSpan = _lex.tokenSpan();
        Lex();
        Expr *e = Expression(SQE_RVALUE);
        return newNode<ThrowStatement>(keywordSpan, e);
    }
    case TK_CONST:
        result = parseConstStatement(false);
        break;
    case TK_GLOBAL: {
        SourceLoc globalStart = _lex.tokenStart();
        Lex();
        if (_token == TK_CONST)
            result = parseConstStatement(true, globalStart);
        else if (_token == TK_ENUM)
            result = parseEnumStatement(true, globalStart);
        else
            reportDiagnostic(DiagnosticsId::DI_GLOBAL_CONSTS_ONLY);
        break;
    }
    default: {
        // Check for contextual import keywords to switch parsing mode
        if (_are_imports_still_allowed && _token == TK_IDENTIFIER) {
            if (strcmp(_lex._svalue, "import") == 0 || strcmp(_lex._svalue, "from") == 0) {
                allowsImport = true;
                result = parseImportStatement();
                break;
            }
        }
        Expr *e = Expression(SQE_REGULAR);
        return newNode<ExprStatement>(e);
      }
    }

    assert(result);

    if (!allowsImport)
        _are_imports_still_allowed = false;

    return result;
}


Expr* SQParser::parseCommaExpr(SQExpressionContext expression_context)
{
    NestingChecker nc(this);
    Expr *expr = Expression(expression_context);

    if (_token == ',') {
        ArenaVector<Expr *> exprs(arena());
        exprs.push_back(expr);
        while (_token == ',') {
            Lex();
            exprs.push_back(Expression(expression_context));
        }
        expr = newNode<CommaExpr>(arena(), std::move(exprs));
    }

    return expr;
}


Expr* SQParser::Expression(SQExpressionContext expression_context)
{
    NestingChecker nc(this);
    SQExpressionContext saved_expression_context = _expression_context;
    _expression_context = expression_context;

    Expr *expr = LogicalNullCoalesceExp();

    switch(_token)  {
    case '=':
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
        case TK_NEWSLOT:
            expr = newNode<BinExpr>(TO_NEWSLOT, expr, e2);
            break;
        case '=': //ASSIGN
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
            case SQE_ARRAY_ELEM:
                reportDiagnostic(DiagnosticsId::DI_ASSIGN_INSIDE_FORBIDDEN, "array element");
                break;
            case SQE_REGULAR:
                break;
            }
            expr = newNode<BinExpr>(TO_ASSIGN, expr, e2);
            break;
        case TK_MINUSEQ: expr = newNode<BinExpr>(TO_MINUSEQ, expr, e2); break;
        case TK_PLUSEQ: expr = newNode<BinExpr>(TO_PLUSEQ, expr, e2); break;
        case TK_MULEQ: expr = newNode<BinExpr>(TO_MULEQ, expr, e2); break;
        case TK_DIVEQ: expr = newNode<BinExpr>(TO_DIVEQ, expr, e2); break;
        case TK_MODEQ: expr = newNode<BinExpr>(TO_MODEQ, expr, e2); break;
        }
    }
    break;
    case '?': {
        Consume('?');

        Expr *ifTrue = Expression(SQE_RVALUE);

        Expect(':');

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

    SQInteger prevTok = _lex._prevtoken, tok = _token;
    unsigned prevFlags = _lex._prevflags;

    Lex();

    checkSuspiciousUnaryOp(prevTok, tok, prevFlags);

    SQExpressionContext old = _expression_context;
    _expression_context = SQE_RVALUE;

    Expr *rhs = (this->*f)();

    _expression_context = old;

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
        if (_token == '|') {
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
        if (_token == '^') {
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
        if (_token == '&') {
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
        case '>': lhs = BIN_EXP(&SQParser::ShiftExp, TO_GT, lhs); break;
        case '<': lhs = BIN_EXP(&SQParser::ShiftExp, TO_LT, lhs); break;
        case TK_GE: lhs = BIN_EXP(&SQParser::ShiftExp, TO_GE, lhs); break;
        case TK_LE: lhs = BIN_EXP(&SQParser::ShiftExp, TO_LE, lhs); break;
        case TK_IN: lhs = BIN_EXP(&SQParser::ShiftExp, TO_IN, lhs); break;
        case TK_INSTANCEOF: lhs = BIN_EXP(&SQParser::ShiftExp, TO_INSTANCEOF, lhs); break;
        case TK_NOT: {
            SourceLoc notStart = _lex.tokenStart();
            Lex();
            if (_token == TK_IN) {
                lhs = BIN_EXP(&SQParser::ShiftExp, TO_IN, lhs);
                lhs = newNode<UnExpr>(TO_NOT, notStart, lhs);
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

void SQParser::checkSuspiciousUnaryOp(SQInteger pprevTok, SQInteger tok, unsigned pprevFlags) {
  if (tok == '+' || tok == '-') {
    if (_expression_context == SQE_ARRAY_ELEM || _expression_context == SQE_FUNCTION_ARG) {
      if ((pprevFlags & (TF_PREP_EOL | TF_PREP_SPACE)) && (pprevTok != ',') && ((_lex._prevflags & TF_PREP_SPACE) == 0)) {
        reportDiagnostic(DiagnosticsId::DI_NOT_UNARY_OP, tok == '+' ? "+" : "-");
      }
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
        case '+': lhs = BIN_EXP(&SQParser::MultExp, TO_ADD, lhs); break;
        case '-': lhs = BIN_EXP(&SQParser::MultExp, TO_SUB, lhs); break;

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
        case '*': lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_MUL, lhs); break;
        case '/': lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_DIV, lhs); break;
        case '%': lhs = BIN_EXP(&SQParser::PrefixedExpr, TO_MOD, lhs); break;

        default: return lhs;
        }
    }
}

void SQParser::checkSuspiciousBracket() {
  assert(_token == '(' || _token == '[');
  if (_expression_context == SQE_ARRAY_ELEM || _expression_context == SQE_FUNCTION_ARG) {
    if (_lex._prevtoken != ',') {
      if (_lex._prevflags & (TF_PREP_EOL | TF_PREP_SPACE)) {
        char op[] = { (char)_token, '\0' };
        reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_BRACKET, op, _token == '(' ? "function call" : "access to member");
      }
    }
  }
}

static const char *opname(SQInteger op) {
  switch (op)
  {
    case '.': return ".";
    case TK_NULLGETSTR: return "?.";
    case TK_TYPE_METHOD_GETSTR: return ".$";
    case TK_NULLABLE_TYPE_METHOD_GETSTR: return "?.$";
    default: return "<unknown>";
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
        case '.':
        case TK_NULLGETSTR:
        case TK_TYPE_METHOD_GETSTR:
        case TK_NULLABLE_TYPE_METHOD_GETSTR: {
            if (_token == TK_NULLGETSTR || _token == TK_NULLABLE_TYPE_METHOD_GETSTR || nextIsNullable) {
                nextIsNullable = true;
            }

            bool isTypeMethod = _token == TK_TYPE_METHOD_GETSTR || _token == TK_NULLABLE_TYPE_METHOD_GETSTR;

            SQInteger tok = _token;

            Lex();

            if ((_lex._prevflags & (TF_PREP_SPACE | TF_PREP_EOL)) != 0) {
              reportDiagnostic(DiagnosticsId::DI_SPACE_SEP_FIELD_NAME, opname(tok));
            }

            Expr *receiver = e;
            Id *id = (Id *)Expect(TK_IDENTIFIER);
            assert(id);
            // Use end from Id's span (after the identifier)
            e = newNode<GetFieldExpr>(receiver, id->name(), nextIsNullable, isTypeMethod, id->sourceSpan().end); //-V522
            break;
        }
        case '[':
        case TK_NULLGETOBJ: {
            if (_token == TK_NULLGETOBJ || nextIsNullable)
            {
                nextIsNullable = true;
            }
            if(_lex._prevtoken == '\n')
                reportDiagnostic(DiagnosticsId::DI_BROKEN_SLOT_DECLARATION);
            if (_token == '[')
              checkSuspiciousBracket();

            Lex();
            Expr *receiver = e;
            Expr *key = Expression(SQE_RVALUE);
            SourceLoc end = _lex.currentPos();
            Expect(']');
            e = newNode<GetSlotExpr>(receiver, key, nextIsNullable, end);
            break;
        }
        case TK_MINUSMINUS:
        case TK_PLUSPLUS:
            {
                nextIsNullable = false;
                if(IsEndOfStatement()) return e;
                SQInteger diff = (_token==TK_MINUSMINUS) ? -1 : 1;
                SourceLoc opEnd = _lex.currentPos();
                Lex();
                e = newNode<IncExpr>(e, diff, IF_POSTFIX, opEnd);
            }
            return e;
        case '(':
        case TK_NULLCALL: {
            if (_lex._prevflags & TF_PREP_EOL && _token == '(') {
                reportDiagnostic(DiagnosticsId::DI_PAREN_IS_FUNC_CALL);
            }
            SQInteger nullcall = (_token==TK_NULLCALL || nextIsNullable);
            nextIsNullable = !!nullcall;
            CallExpr *call = newNode<CallExpr>(arena(), e, nullcall);

            if (_token == '(')
              checkSuspiciousBracket();

            Lex();
            while (_token != ')') {
                call->addArgument(Expression(SQE_FUNCTION_ARG));
                if (_token == ',') {
                    Lex();
                }
            }

            call->setSpanEnd(_lex.currentPos());
            Lex();
            e = call;
            break;
        }
        default: return e;
        }
    }
}

static void appendStringData(sqvector<char> &dst, const char *b) {
  while (*b) {
    dst.push_back(*b++);
  }
}

Expr *SQParser::parseStringTemplate() {

    // '$' TK_TEMPLATE_PREFIX? (arg TK_TEMPLATE_INFIX)* arg? TK_TEMPLATE_SUFFX

    _lex._state = LS_TEMPLATE;
    _lex._expectedToken = TK_TEMPLATE_PREFIX;

    Lex();
    int idx = 0;

    sqvector<char> formatString(_ctx.allocContext());
    sqvector<Expr *> args(_ctx.allocContext());
    char buffer[64] = {0};

    SourceLoc fmtStart = _lex.tokenStart();
    SQInteger tok = -1;

    while ((tok = _token) != SQUIRREL_EOB) {

      if (tok != TK_TEMPLATE_PREFIX && tok != TK_TEMPLATE_INFIX && tok != TK_TEMPLATE_SUFFIX) {
          reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "STRING_LITERAL");
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

        if (_token != '}') {
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "}");
            return nullptr;
        }

        formatString.push_back('}');

        _lex._state = LS_TEMPLATE;
        _lex._expectedToken = TK_TEMPLATE_INFIX;
      }
      else {
        break;
      }
      Lex();
    }

    formatString.push_back('\0');

    SourceLoc templateEnd = _lex.currentPos();

    Expr *result = nullptr;
    SourceSpan fmtSpan = {fmtStart, templateEnd};
    LiteralExpr *fmt = newNode<LiteralExpr>(fmtSpan, copyString(&formatString[0]));

    if (args.empty()) {
      result = fmt;
    }
    else {
      // GetFieldExpr: from fmt start to current position (synthetic .subst access)
      Expr *callee = newNode<GetFieldExpr>(fmt, "subst", false, /* force type method */ true, templateEnd);
      // CallExpr: from callee start to end of arguments (which is templateEnd for string templates)
      CallExpr *call = newNode<CallExpr>(arena(), callee, false, templateEnd);

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

    switch(_token)
    {
    case TK_STRING_LITERAL:
        r = newStringLiteral(_lex._svalue);
        Lex();
        break;
    case TK_TEMPLATE_OP:
        r = parseStringTemplate();
        break;
    case TK_BASE: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        r = newNode<BaseExpr>(span);
        break;
    }
    case TK_IDENTIFIER:
        r = newId(_lex._svalue);
        Lex();
        break;
    case TK_CONSTRUCTOR: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        r = newNode<Id>(span, "constructor");
        break;
    }
    case TK_THIS: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        r = newNode<Id>(span, "this");
        break;
    }
    case TK_DOUBLE_COLON: { // "::"
        if (_lang_features & LF_FORBID_ROOT_TABLE)
            reportDiagnostic(DiagnosticsId::DI_ROOT_TABLE_FORBIDDEN);
        SourceSpan span = _lex.tokenSpan();
        _token = '.'; /* hack: drop into PrefixExpr, case '.'*/
        r = newNode<RootTableAccessExpr>(span);
        break;
    }
    case TK_NULL: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        r = newNode<LiteralExpr>(span);
        break;
    }
    case TK_INTEGER: {
        SourceSpan span = _lex.tokenSpan();
        SQInteger val = _lex._nvalue;
        Lex();
        r = newNode<LiteralExpr>(span, val);
        break;
    }
    case TK_FLOAT: {
        SourceSpan span = _lex.tokenSpan();
        SQFloat val = _lex._fvalue;
        Lex();
        r = newNode<LiteralExpr>(span, val);
        break;
    }
    case TK_TRUE: case TK_FALSE: {
        SourceSpan span = _lex.tokenSpan();
        bool val = (_token == TK_TRUE);
        Lex();
        r = newNode<LiteralExpr>(span, val);
        break;
    }
    case '[': {
            SourceLoc start = _lex.tokenStart();
            Lex();
            ArrayExpr *arr = newNode<ArrayExpr>(arena(), start);
            bool commaSeparated = false;
            bool spaceSeparated = false;
            bool reported = false;
            while(_token != ']') {
                Expr *v = Expression(SQE_ARRAY_ELEM);
                arr->addValue(v);
                if (_token == ',') {
                    commaSeparated = true;
                }
                else if (_token != ']') {
                    spaceSeparated = true;
                }

                if (commaSeparated && spaceSeparated && !reported) {
                    reported = true; // do not spam in output, a single diag seems to be enough
                    reportDiagnostic(DiagnosticsId::DI_MIXED_SEPARATORS, "elements of array");
                }

                if (_token == ',') {
                    Lex();
                }
            }
            arr->setSpanEnd(_lex.currentPos());
            Lex();
            r = arr;
        }
        break;
    case '{': {
        SourceLoc start = _lex.tokenStart();
        Lex();
        TableExpr *t = newNode<TableExpr>(arena(), start);
        _docObjectStack.push_back(&t->docObject);
        ParseTableOrClass(t, ',', '}');
        _docObjectStack.pop_back();
        // TableExpr end is set by ParseTableOrClass via Lex() after '}'
        r = t;
        break;
    }
    case TK_FUNCTION:
        r = FunctionExp(false);
        break;
    case '@':
        r = FunctionExp(true);
        break;
    case TK_CLASS: {
        SourceLoc classStart = _lex.tokenStart();
        Lex();
        r = ClassExp(classStart, NULL);
        break;
    }
    case '-': {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        switch(_token) {
        case TK_INTEGER: {
            SourceSpan span = {opStart, _lex.currentPos()};
            SQInteger val = -_lex._nvalue;
            Lex();
            r = newNode<LiteralExpr>(span, val);
            break;
        }
        case TK_FLOAT: {
            SourceSpan span = {opStart, _lex.currentPos()};
            SQFloat val = -_lex._fvalue;
            Lex();
            r = newNode<LiteralExpr>(span, val);
            break;
        }
        default:
            r = newNode<UnExpr>(TO_NEG, opStart, PrefixedExpr());
            break;
        }
        break;
    }
    case '!': {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_NOT, opStart, PrefixedExpr());
        break;
    }
    case '~': {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        if(_token == TK_INTEGER)  {
            SourceSpan span = {opStart, _lex.currentPos()};
            SQInteger val = ~_lex._nvalue;
            Lex();
            r = newNode<LiteralExpr>(span, val);
        }
        else {
            r = newNode<UnExpr>(TO_BNOT, opStart, PrefixedExpr());
        }
        break;
    }
    case TK_TYPEOF: {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_TYPEOF, opStart, PrefixedExpr());
        break;
    }
    case TK_RESUME: {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_RESUME, opStart, PrefixedExpr());
        break;
    }
    case TK_CLONE: {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_CLONE, opStart, PrefixedExpr());
        break;
    }
    case TK_STATIC: {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_STATIC_MEMO, opStart, PrefixedExpr());
        break;
    }
    case TK_CONST: {
        SourceLoc opStart = _lex.tokenStart();
        Lex();
        r = newNode<UnExpr>(TO_INLINE_CONST, opStart, PrefixedExpr());
        break;
    }
    case TK_MINUSMINUS :
    case TK_PLUSPLUS :
        r = PrefixIncDec(_token);
        break;
    case TK_DELETE :
        if (_lang_features & LF_FORBID_DELETE_OP) {
            reportDiagnostic(DiagnosticsId::DI_DELETE_OP_FORBIDDEN);
        }
        r = DeleteExpr();
        break;
    case '(': {
        SourceLoc start = _lex.tokenStart();
        Lex();
        Expr *inner = Expression(_expression_context);
        Expect(')');
        r = newNode<UnExpr>(TO_PAREN, start, inner);
        break;
    }
    case TK_CODE_BLOCK_EXPR: {
        if ((_lang_features & LF_ALLOW_COMPILER_INTERNALS) == 0)
            reportDiagnostic(DiagnosticsId::DI_COMPILER_INTERNALS_FORBIDDEN);
        SQExpressionContext saved_expression_context = _expression_context;
        _expression_context = SQE_REGULAR;
        SQUnsignedInteger savedLangFeatures = _lang_features;
        SourceLoc start = _lex.tokenStart();
        Lex();
        Block *blk = parseStatements(start);
        blk->setIsExprBlock();
        blk->setSpanEnd(_lex.currentPos());
        r = newNode<CodeBlockExpr>(blk);
        Expect('}');
        _lang_features = savedLangFeatures;
        _expression_context = saved_expression_context;
        break;
    }
    case TK___LINE__: {
        SourceSpan span = _lex.tokenSpan();
        SQInteger val = _lex._currentline;
        Lex();
        r = newNode<LiteralExpr>(span, val);
        break;
    }
    case TK___FILE__: {
        SourceSpan span = _lex.tokenSpan();
        Lex();
        r = newNode<LiteralExpr>(span, _sourcename);
        break;
    }
    default:
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "expression");
    }
    return r;
}

void SQParser::ParseTableOrClass(TableExpr *decl, SQInteger separator, SQInteger terminator)
{
    NestingChecker nc(this);
    NewObjectType otype = separator==',' ? NEWOBJ_TABLE : NEWOBJ_CLASS;

    while(_token != terminator) {
        SourceSpan keySpan = _lex.tokenSpan();
        unsigned flags = 0;
        //check if is an static
        if(otype == NEWOBJ_CLASS) {
            if(_token == TK_STATIC) {
                flags |= TMF_STATIC;
                Lex();
                keySpan = _lex.tokenSpan();
            }
        }

        switch (_token) {
        case TK_DOCSTRING: {
            onDocString(_lex._svalue);
            Lex();
            break;
        }
        case TK_FUNCTION:
        case TK_CONSTRUCTOR: {
            SQInteger tk = _token;
            SourceLoc funcStart = _lex.tokenStart();
            Lex();
            FuncAttrFlagsType attr = ParseFunctionAttributes();
            Id *funcName = tk == TK_FUNCTION ? (Id *)Expect(TK_IDENTIFIER) : newNode<Id>(_lex.tokenSpan(), "constructor");
            assert(funcName);
            LiteralExpr *key = newNode<LiteralExpr>(funcName->sourceSpan(), funcName->name()); //-V522
            Expect('(');
            FunctionExpr *f = CreateFunction(funcStart, funcName, false, tk == TK_CONSTRUCTOR);
            f->setPure(attr & FATTR_PURE);
            f->setNodiscard(attr & FATTR_NODISCARD);
            decl->addMember(key, f, flags);
        }
        break;
        case '[': {
            Lex();

            Expr *key = Expression(SQE_RVALUE); //-V522
            assert(key);
            Expect(']');
            Expect('=');
            Expr *value = Expression(SQE_RVALUE);
            decl->addMember(key, value, flags | TMF_DYNAMIC_KEY);
            break;
        }
        case TK_STRING_LITERAL: //JSON
            if (otype == NEWOBJ_TABLE) { //only works for tables
                LiteralExpr *key = (LiteralExpr *)Expect(TK_STRING_LITERAL);  //-V522
                assert(key);
                Expect(':');
                Expr *expr = Expression(SQE_RVALUE);
                decl->addMember(key, expr, flags | TMF_JSON);
                break;
            }  //-V796
        default: {
            Id *id = (Id *)Expect(TK_IDENTIFIER);
            assert(id);
            LiteralExpr *key = newNode<LiteralExpr>(keySpan, id->name()); //-V522

            if ((otype == NEWOBJ_TABLE) &&
                (_token == TK_IDENTIFIER || _token == separator || _token == terminator || _token == '['
                    || _token == TK_FUNCTION)) {
                decl->addMember(key, id, flags);
            }
            else {
                Expect('=');
                Expr *expr = Expression(SQE_RVALUE);
                decl->addMember(key, expr, flags);
            }
        }
        }
        if (_token == separator) Lex(); //optional comma/semicolon
    }

    decl->setSpanEnd(_lex.currentPos());
    Lex();
}

Decl *SQParser::parseLocalFunctionExprStmt(bool assignable, SourceLoc keywordStart)
{
    SourceLoc funcStart = _lex.tokenStart();

    assert(_token == TK_FUNCTION);
    Lex();

    FuncAttrFlagsType attr = ParseFunctionAttributes();
    Id *varname = (Id *)Expect(TK_IDENTIFIER);
    Expect('(');
    FunctionExpr *f = CreateFunction(funcStart, varname, false);
    f->setPure(attr & FATTR_PURE);
    f->setNodiscard(attr & FATTR_NODISCARD);
    VarDecl *d = newNode<VarDecl>(keywordStart, varname, f, assignable); //-V522
    return d;
}

Decl *SQParser::parseLocalClassExprStmt(bool assignable, SourceLoc keywordStart)
{
    SourceLoc classStart = _lex.tokenStart();

    assert(_token == TK_CLASS);
    Lex();

    Id *varname = (Id *)Expect(TK_IDENTIFIER);
    ClassExpr *cls = ClassExp(classStart, NULL);
    VarDecl *d = newNode<VarDecl>(keywordStart, varname, cls, assignable); //-V522
    return d;
}

Decl* SQParser::parseLocalDeclStatement(bool onlySingleVariable)
{
    NestingChecker nc(this);

    assert(_token == TK_LET || _token == TK_LOCAL);
    SourceLoc keywordStart = _lex.tokenStart();
    bool assignable = _token == TK_LOCAL;
    Lex();

    if (_token == TK_FUNCTION) {
        return parseLocalFunctionExprStmt(assignable, keywordStart);
    } else if (_token == TK_CLASS) {
        return parseLocalClassExprStmt(assignable, keywordStart);
    }


    DeclGroup *decls = NULL;
    DestructuringDecl  *dd = NULL;
    Decl *decl = NULL;
    SQInteger destructurer = 0;

    if (_token == '{' || _token == '[') {
        if (onlySingleVariable)
            reportDiagnostic(DiagnosticsId::DI_ONLY_SINGLE_VARIABLE_DECLARATION);

        destructurer = _token;
        Lex();
        // Use keywordStart (position of 'local'/'let') so span includes the keyword
        decls = dd = newNode<DestructuringDecl>(arena(), keywordStart, destructurer == '{' ? DT_TABLE : DT_ARRAY);
    }

    do {
        Id *varname = (Id *)Expect(TK_IDENTIFIER);
        assert(varname);
        VarDecl *cur = NULL;
        unsigned typeMask = _token == ':' ? parseTypeMask(destructurer == 0) : ~0u;

        // Determine start position:
        // - For destructured vars, use varname's span start (keyword covered by DestructuringDecl)
        // - For first non-destructured var, use keywordStart (to include 'local'/'let')
        // - For subsequent vars in a group, use varname's span start (group covers keyword)
        bool isFirstNonDestructured = (!destructurer && decl == NULL && decls == NULL);
        SourceLoc varStart = isFirstNonDestructured ? keywordStart : varname->sourceSpan().start; //-V522

        if(_token == '=') {
            Lex();
            Expr *expr = Expression(SQE_REGULAR);
            if (!assignable && expr->op() == TO_FUNCTION) {
              FunctionExpr *f = expr->asFunctionExpr();
              if (!f->name() || f->name()[0] == '(') {
                f->setName(varname->name());
              }
            }
            cur = newNode<VarDecl>(varStart, varname, expr, assignable, destructurer != 0); //-V522
        }
        else {
            if (!assignable && !destructurer)
                _ctx.reportDiagnostic(DiagnosticsId::DI_UNINITIALIZED_BINDING, varname->lineStart(), varname->columnStart(), varname->textWidth(), varname->name()); //-V522
            cur = newNode<VarDecl>(varStart, varname, nullptr, assignable, destructurer != 0);
        }

        cur->setTypeMask(typeMask);

        if (decls) {
            decls->addDeclaration(cur);
        } else if (decl) {
            decls = newNode<DeclGroup>(arena(), keywordStart);
            decls->addDeclaration(static_cast<VarDecl *>(decl));
            decls->addDeclaration(cur);
            decl = decls;
        } else {
            decl = cur;
        }

        if (destructurer) {
            if (_token == ',') {
                Lex();
                if (_token == ']' || _token == '}')
                    break;
            }
            else if (_token == TK_IDENTIFIER)
                continue;
            else
                break;
        }
        else {
            if (_token == ',') {
                if (onlySingleVariable)
                    reportDiagnostic(DiagnosticsId::DI_ONLY_SINGLE_VARIABLE_DECLARATION);
                Lex();
            }
            else
                break;
        }
    } while(1);

    if (destructurer) {
        Expect(destructurer=='[' ? ']' : '}');
        Expect('=');
        assert(dd);
        dd->setExpression(Expression(SQE_RVALUE)); //-V522
        return dd;
    } else {
        return decls ? static_cast<Decl*>(decls) : decl;
    }
}

Statement* SQParser::IfLikeBlock(bool &wrapped)
{
    NestingChecker nc(this);
    Statement *stmt = NULL;
    if (_token == '{')
    {
        wrapped = false;
        SourceLoc blockStart = _lex.tokenStart();
        Lex();
        Block *block = parseStatements(blockStart);
        block->setSpanEnd(_lex.currentPos());
        Expect('}');
        stmt = block;
    }
    else {
        wrapped = true;
        stmt = parseStatement();
        SourceSpan stmtSpan = stmt->sourceSpan(); //-V522
        Block *block = newNode<Block>(arena(), stmtSpan.start);
        block->addStatement(stmt);
        block->setSpanEnd(stmtSpan.end);
        stmt = block;
        if (_lex._prevtoken != '}' && _lex._prevtoken != ';') OptionalSemicolon();
    }

    return stmt;
}

Statement * SQParser::parseIfStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    SQInteger prevTok = _lex._prevtoken;

    Consume(TK_IF);

    Statement *initializer = NULL;
    Expr *cond = NULL;

    Expect('(');
    if (_token == TK_LOCAL || _token == TK_LET) {
        initializer = parseLocalDeclStatement(true);
        assert(initializer);
        if (initializer->asDeclaration()->asVarDecl()->initializer() == NULL) //-V522
            reportDiagnostic(DiagnosticsId::DI_INITIALIZATION_REQUIRED);

        if (_token == ';') {
            Lex();
            cond = Expression(SQE_IF);
        }
        else {
            Id* nameId = initializer->asDeclaration()->asVarDecl()->nameId(); //-V522
            cond = newNode<Id>(nameId->sourceSpan(), nameId->name());
        }
    }
    else {
        cond = Expression(SQE_IF);
    }
    Expect(')');

    checkBraceIndentationStyle();

    bool wrapped = false;

    Statement *thenB = IfLikeBlock(wrapped);
    Statement *elseB = NULL;
    if (_token == TK_ELSE) {
        SourceSpan elseSpan = _lex.tokenSpan();
        Lex();
        if (_token != '{' && prevTok != TK_ELSE && wrapped && start.line != elseSpan.start.line && start.column != elseSpan.start.column) {
          _ctx.reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_FMT, elseSpan.start.line, elseSpan.start.column, elseSpan.textWidth());
        }
        checkBraceIndentationStyle();
        elseB = IfLikeBlock(wrapped);
        if (!IsEndOfStatement()) {
          reportDiagnostic(DiagnosticsId::DI_STMT_SAME_LINE, "else");
        }
    }
    else if (!IsEndOfStatement()) {
      reportDiagnostic(DiagnosticsId::DI_STMT_SAME_LINE, "then");
    }


    if (_token != SQUIRREL_EOB && _token != '}' && _token != ']' && _token != ')' && _token != ',') {
      int32_t lastLine = elseB ? elseB->lineEnd() : thenB->lineEnd();
      SourceLoc curTok = _lex.tokenStart();
      if (curTok.line != lastLine && curTok.column > start.column) {
        reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_FMT);
      }
    }

    if (initializer) {
        // wrap initializer and condition in a {} block so they are properly scoped
        Block *block = newNode<Block>(arena(), start);
        block->addStatement(initializer);
        block->addStatement(newNode<IfStatement>(start, cond, thenB, elseB));
        block->setSpanEnd((elseB ? elseB : thenB)->sourceSpan().end);
        return block;
    }
    else {
        return newNode<IfStatement>(start, cond, thenB, elseB);
    }
}

WhileStatement* SQParser::parseWhileStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_WHILE);

    Expect('(');
    Expr *cond = Expression(SQE_LOOP_CONDITION);
    Expect(')');

    bool wrapped = false;
    checkBraceIndentationStyle();
    Statement *body = IfLikeBlock(wrapped);

    if (!IsEndOfStatement()) {
      reportDiagnostic(DiagnosticsId::DI_STMT_SAME_LINE, "while loop body");
    }

    if (_token != SQUIRREL_EOB && _token != '}') {
      SourceLoc curTok = _lex.tokenStart();
      if (curTok.line != body->lineEnd() && curTok.column > start.column) {
        reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_FMT);
      }
    }

    return newNode<WhileStatement>(start, cond, body);
}

DoWhileStatement* SQParser::parseDoWhileStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_DO); // DO

    bool wrapped = false;
    checkBraceIndentationStyle();
    Statement *body = IfLikeBlock(wrapped);

    Expect(TK_WHILE);

    Expect('(');
    Expr *cond = Expression(SQE_LOOP_CONDITION);
    SourceLoc end = _lex.currentPos();
    Expect(')');

    return newNode<DoWhileStatement>(start, body, cond, end);
}

ForStatement* SQParser::parseForStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_FOR);

    Expect('(');

    Node *init = NULL;
    if (_token == TK_LOCAL)
        init = parseLocalDeclStatement();
    else if (_token != ';') {
        init = parseCommaExpr(SQE_REGULAR);
    }
    Expect(';');

    Expr *cond = NULL;
    if(_token != ';') {
        cond = Expression(SQE_LOOP_CONDITION);
    }
    Expect(';');

    Expr *mod = NULL;
    if(_token != ')') {
        mod = parseCommaExpr(SQE_REGULAR);
    }
    Expect(')');

    bool wrapped = false;
    checkBraceIndentationStyle();
    Statement *body = IfLikeBlock(wrapped);

    if (!IsEndOfStatement()) {
      reportDiagnostic(DiagnosticsId::DI_STMT_SAME_LINE, "for loop body");
    }

    if (_token != SQUIRREL_EOB && _token != '}') {
      SourceLoc curTok = _lex.tokenStart();
      if (curTok.line != body->lineEnd() && curTok.column > start.column) {
        reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_FMT);
      }
    }

    return newNode<ForStatement>(start, init, cond, mod, body);
}

void SQParser::parseDestructuringFields(DestructuringDecl *destr)
{
    while (_token != '}' && _token != ']' && _token != SQUIRREL_EOB) {
        Id *fieldname = (Id *)Expect(TK_IDENTIFIER);
        assert(fieldname);
        if (!fieldname)
            break;

        unsigned typeMask = _token == ':' ? parseTypeMask(false) : ~0u;
        SourceLoc varStart = fieldname->sourceSpan().start;

        Expr *defaultExpr = nullptr;
        if (_token == '=') {
            Lex();
            defaultExpr = Expression(SQE_REGULAR);
        }

        VarDecl *fieldDecl = newNode<VarDecl>(varStart, fieldname, defaultExpr, /*assignable*/false, /*destructured*/true);
        fieldDecl->setTypeMask(typeMask);
        destr->addDeclaration(fieldDecl);

        if (_token == ',') {
            Lex();
            if (_token == ']' || _token == '}')
                break;
        }
        else if (_token == TK_IDENTIFIER)
            continue;
        else
            break;
    }
}

ForeachStatement* SQParser::parseForEachStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_FOREACH);

    Expect('(');

    Id *idxname = NULL;
    Id *valname = NULL;
    DestructuringDecl *valDestr = NULL;

    auto parseValDestructuring = [this, &valDestr]() -> Id* {
        SQInteger destructurer = _token;
        SourceLoc keywordStart = _lex.tokenStart();
        char buf[32];
        snprintf(buf, sizeof(buf), "@FE_VAL%u", _foreachDestrCounter++);
        Id *surrogateName = newId(buf);
        Lex(); // consume '{' or '['
        valDestr = newNode<DestructuringDecl>(arena(), keywordStart, destructurer == '{' ? DT_TABLE : DT_ARRAY);
        parseDestructuringFields(valDestr);
        Expect(destructurer == '{' ? '}' : ']');
        valDestr->setExpression(surrogateName);
        return surrogateName;
    };

    if (_token == '{' || _token == '[') {
        valname = parseValDestructuring();
    }
    else {
        valname = (Id *)Expect(TK_IDENTIFIER);
        assert(valname);

        if (_token == ',') {
            idxname = valname;
            Lex();
            if (_token == '{' || _token == '[') {
                valname = parseValDestructuring();
            }
            else {
                valname = (Id *)Expect(TK_IDENTIFIER);
                assert(valname);
                if (strcmp(idxname->name(), valname->name()) == 0) //-V522
                    _ctx.reportDiagnostic(DiagnosticsId::DI_SAME_FOREACH_KV_NAMES, valname->lineStart(), valname->columnStart(), valname->textWidth(), valname->name());
            }
        }
    }

    Expect(TK_IN);

    Expr *contnr = Expression(SQE_RVALUE);
    Expect(')');

    bool wrapped = false;
    checkBraceIndentationStyle();
    Statement *body = IfLikeBlock(wrapped);

    if (!IsEndOfStatement()) {
      reportDiagnostic(DiagnosticsId::DI_STMT_SAME_LINE, "foreach loop body");
    }

    if (_token != SQUIRREL_EOB && _token != '}') {
      SourceLoc curTok = _lex.tokenStart();
      if (curTok.line != body->lineEnd() && curTok.column > start.column) {
        reportDiagnostic(DiagnosticsId::DI_SUSPICIOUS_FMT);
      }
    }

    if (valDestr) {
        Block *wrap = newNode<Block>(arena(), body->sourceSpan().start);
        wrap->addStatement(valDestr);
        wrap->addStatement(body);
        wrap->setSpanEnd(body->sourceSpan().end);
        body = wrap;
    }

    VarDecl *idxDecl = idxname ? newNode<VarDecl>(idxname->sourceSpan().start, idxname, nullptr, false) : NULL;
    VarDecl *valDecl = valname ? newNode<VarDecl>(valname->sourceSpan().start, valname, nullptr, false) : NULL;

    return newNode<ForeachStatement>(start, idxDecl, valDecl, contnr, body);
}

SwitchStatement* SQParser::parseSwitchStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_SWITCH);

    Expect('(');
    Expr *switchExpr = Expression(SQE_SWITCH);
    Expect(')');

    checkBraceIndentationStyle();
    Expect('{');

    SwitchStatement *switchStmt = newNode<SwitchStatement>(start, arena(), switchExpr);

    while(_token == TK_CASE) {
        Consume(TK_CASE);

        Expr *cond = Expression(SQE_RVALUE);
        Expect(':');
        SourceLoc caseBodyStart = _lex.currentPos();

        checkBraceIndentationStyle();
        Statement *caseBody = parseStatements(caseBodyStart);
        switchStmt->addCases(cond, caseBody);
    }

    if(_token == TK_DEFAULT) {
        Consume(TK_DEFAULT);
        Expect(':');
        SourceLoc defaultBodyStart = _lex.currentPos();

        checkBraceIndentationStyle();
        switchStmt->addDefault(parseStatements(defaultBodyStart));
    }

    switchStmt->setSpanEnd(_lex.currentPos());
    Expect('}');

    return switchStmt;
}


LiteralExpr* SQParser::ExpectScalar()
{
    NestingChecker nc(this);
    LiteralExpr *ret = NULL;
    SourceLoc start = _lex.tokenStart();

    switch(_token) {
        case TK_NULL:
            ret = newNode<LiteralExpr>(_lex.tokenSpan());
            break;
        case TK_INTEGER:
            ret = newNode<LiteralExpr>(_lex.tokenSpan(), _lex._nvalue);
            break;
        case TK_FLOAT:
            ret = newNode<LiteralExpr>(_lex.tokenSpan(), _lex._fvalue);
            break;
        case TK_STRING_LITERAL:
            ret = newStringLiteral(_lex._svalue);
            break;
        case TK_TRUE:
        case TK_FALSE:
            ret = newNode<LiteralExpr>(_lex.tokenSpan(), (bool)(_token == TK_TRUE ? 1 : 0));
            break;
        case '-': {
            Lex();
            SourceSpan span = {start, _lex.currentPos()};
            switch(_token)
            {
            case TK_INTEGER:
                ret = newNode<LiteralExpr>(span, -_lex._nvalue);
            break;
            case TK_FLOAT:
                ret = newNode<LiteralExpr>(span, -_lex._fvalue);
            break;
            default:
                reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float");
            }
            break;
        }
        default:
            reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float, or string");
    }

    Lex();
    return ret;
}


ConstDecl* SQParser::parseConstFunctionExprStmt(bool global, SourceLoc globalStart)
{
    SourceLoc start = globalStart.isValid() ? globalStart : _lex.tokenStart();

    assert(_token == TK_FUNCTION);
    Lex();

    FuncAttrFlagsType attr = ParseFunctionAttributes();
    Id *funcName = (Id *)Expect(TK_IDENTIFIER);
    Expect('(');
    FunctionExpr *f = CreateFunction(start, funcName, false);
    f->setPure(attr & FATTR_PURE);
    f->setNodiscard(attr & FATTR_NODISCARD);

    // Wrap function in const inline operator
    Expr *constFunc = newNode<UnExpr>(TO_INLINE_CONST, start, f);
    ConstDecl *d = newNode<ConstDecl>(start, funcName->name(), constFunc, global); //-V522
    return d;
}

ConstDecl* SQParser::parseConstStatement(bool global, SourceLoc globalStart)
{
    NestingChecker nc(this);
    SourceLoc start = globalStart.isValid() ? globalStart : _lex.tokenStart();
    Lex();

    if (_token == TK_FUNCTION) {
        return parseConstFunctionExprStmt(global, globalStart);
    }

    Id *id = (Id *)Expect(TK_IDENTIFIER);

    Expect('=');
    Expr *valExpr = Expression(SQE_RVALUE);
    ConstDecl *d = newNode<ConstDecl>(start, id->name(), valExpr, global); //-V522

    OptionalSemicolon();

    return d;
}


EnumDecl* SQParser::parseEnumStatement(bool global, SourceLoc globalStart)
{
    NestingChecker nc(this);
    SourceLoc start = globalStart.isValid() ? globalStart : _lex.tokenStart();
    Lex();
    Id *id = (Id *)Expect(TK_IDENTIFIER);

    EnumDecl *decl = newNode<EnumDecl>(arena(), start, id->name(), global); //-V522

    checkBraceIndentationStyle();
    Expect('{');

    SQInteger nval = 0;
    while(_token != '}') {
        Id *key = (Id *)Expect(TK_IDENTIFIER);

        const char* keyName = key->name(); //-V522
        for (EnumConst& ec : decl->consts())
            if (*keyName == *ec.id && !strcmp(keyName, ec.id))
                reportDiagnostic(DiagnosticsId::DI_DUPLICATE_KEY, keyName);

        LiteralExpr *valExpr = NULL;
        if(_token == '=') {
            // TODO1: should it behave like C does?
            // TODO2: should float and string literal be allowed here?
            Lex();
            valExpr = ExpectScalar();
        }
        else {
            valExpr = newNode<LiteralExpr>(key->sourceSpan(), nval++);
        }

        decl->addConst(keyName, valExpr); //-V522

        if(_token == ',') Lex();
    }

    decl->setSpanEnd(_lex.currentPos());
    Lex();

    return decl;
}


TryStatement* SQParser::parseTryCatchStatement()
{
    NestingChecker nc(this);

    SourceLoc start = _lex.tokenStart();

    Consume(TK_TRY);

    checkBraceIndentationStyle();
    Statement *t = parseStatement();

    Expect(TK_CATCH);

    Expect('(');
    Id *exid = (Id *)Expect(TK_IDENTIFIER);
    Expect(')');

    checkBraceIndentationStyle();
    Statement *cth = parseStatement();

    return newNode<TryStatement>(start, t, exid, cth);
}


Id* SQParser::generateSurrogateFunctionName()
{
    const char * fileName = _sourcename ? _sourcename : "unknown";
    int lineNum = int(_lex._currentline);

    const char * rightSlash = std::max(strrchr(fileName, '/'), strrchr(fileName, '\\'));

    constexpr int maxLen = 256;

    char buf[maxLen];
    scsprintf(buf, maxLen, "(%s:%d)", rightSlash ? (rightSlash + 1) : fileName, lineNum);
    return newId(buf);
}


SQParser::FuncAttrFlagsType SQParser::ParseFunctionAttributes() {
    if (_token != '[')
        return 0;

    Lex();

    FuncAttrFlagsType attrVal = 0;
    while (_token != ']') {
        if (_token == TK_IDENTIFIER) {
            FuncAttrFlagsType flag = 0;
            if (strcmp(_lex._svalue, "pure")==0) {
                flag = FATTR_PURE;
            } else if (strcmp(_lex._svalue, "nodiscard")==0) {
                flag = FATTR_NODISCARD;
            } else {
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "valid attribute name");
            }
            if (attrVal & flag) {
                reportDiagnostic(DiagnosticsId::DI_DUPLICATE_FUNC_ATTR, _lex._svalue);
            }
            attrVal |= flag;
        } else {
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "attribute name");
        }

        Lex();
        if (_token == ',') {
            Lex();
        }
    }

    Expect(']');
    return attrVal;
}


FunctionExpr* SQParser::FunctionExp(bool lambda)
{
    NestingChecker nc(this);
    SourceLoc start = _lex.tokenStart();
    Lex();
    FuncAttrFlagsType attr = ParseFunctionAttributes();
    Id *funcName = (_token == TK_IDENTIFIER) ? (Id *)Expect(TK_IDENTIFIER) : generateSurrogateFunctionName();
    Expect('(');

    FunctionExpr *f = CreateFunction(start, funcName, lambda);
    f->setPure(attr & FATTR_PURE);
    f->setNodiscard(attr & FATTR_NODISCARD);

    return f;
}


ClassExpr* SQParser::ClassExp(SourceLoc classStart, Expr *key)
{
    NestingChecker nc(this);
    Expr *baseExpr = NULL;
    if(_token == TK_EXTENDS) {
        Lex();
        baseExpr = Expression(SQE_RVALUE);
    }
    else if (_token == '(') {
      Lex();
      baseExpr = Expression(SQE_RVALUE);
      Expect(')');
    }
    checkBraceIndentationStyle();
    Expect('{');
    ClassExpr *d = newNode<ClassExpr>(arena(), classStart, key, baseExpr);
    _docObjectStack.push_back(&d->docObject);
    ParseTableOrClass(d, ';','}');
    _docObjectStack.pop_back();
    return d;
}


Expr* SQParser::DeleteExpr()
{
    NestingChecker nc(this);
    SourceLoc opStart = _lex.tokenStart();
    Consume(TK_DELETE);
    Expr *arg = PrefixedExpr();
    return newNode<UnExpr>(TO_DELETE, opStart, arg);
}


Expr* SQParser::PrefixIncDec(SQInteger token)
{
    NestingChecker nc(this);
    SourceLoc opStart = _lex.tokenStart();
    SQInteger diff = (token==TK_MINUSMINUS) ? -1 : 1;
    Lex();
    Expr *arg = PrefixedExpr();
    return newNode<IncExpr>(arg, diff, IF_PREFIX, opStart);
}


static bool can_be_type_name(int token)
{
    return token == TK_IDENTIFIER || token == TK_NULL || token == TK_FUNCTION || token == TK_CLASS;
}

unsigned SQParser::parseTypeMask(bool eol_breaks_type_parsing)
{
    unsigned typeMask = 0;
    Lex();
    bool requireParen = false;
    if (_token == '(') {
        requireParen = true;
        Lex();
    }

    if (!can_be_type_name(_token))
        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "TYPE_NAME");

    while (can_be_type_name(_token)) {
        const char* suggestion = nullptr;
        SQUnsignedInteger32 currentMask = 0;

        const char* name = (_token == TK_IDENTIFIER) ? _lex._svalue :
            (_token == TK_NULL) ? "null" :
            (_token == TK_FUNCTION) ? "function" :
            (_token == TK_CLASS) ? "class" :
            "<not-implemented>";

        bool found = sq_type_string_to_mask(name, currentMask, suggestion);
        if (!found) {
           if (suggestion)
               reportDiagnostic(DiagnosticsId::DI_INVALID_TYPE_NAME_SUGGESTION, name, suggestion);
           else
               reportDiagnostic(DiagnosticsId::DI_INVALID_TYPE_NAME, name);
        }
        typeMask |= currentMask;
        Lex();

        if (_lex._prevtoken == '\n' && eol_breaks_type_parsing && !requireParen)
            break;

        if (_token == '|')
            Lex();
        else
            break;

        if (!can_be_type_name(_token))
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "TYPE_NAME");
    }

    if (requireParen)
        Expect(')');

    return typeMask;
}


FunctionExpr* SQParser::CreateFunction(SourceLoc start, Id *name, bool lambda, bool ctor)
{
    NestingChecker nc(this);
    FunctionExpr *f = ctor ? newNode<FunctionExpr>(arena(), start, name->name()) : newNode<FunctionExpr>(arena(), start, name);

    SQInteger defparams = 0;

    auto &params = f->parameters();

    while (_token!=')') {
        SourceSpan paramSpan = _lex.tokenSpan();
        if (_token == TK_VARPARAMS) {
            if(defparams > 0)
                reportDiagnostic(DiagnosticsId::DI_VARARG_WITH_DEFAULT_ARG);
            f->addParameter(paramSpan, "vargv");
            f->setVararg(true);
            params.back()->setVararg();
            Lex();
            unsigned typeMask = _token == ':' ? parseTypeMask(false) : ~0u;
            params.back()->setTypeMask(typeMask);
            if(_token != ')')
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ")");
            break;
        }
        else {
            if (_token == '{' || _token == '[') {
                SourceSpan argSpan = _lex.tokenSpan();
                SourceLoc keywordStart = _lex.tokenStart();
                SQInteger destructurer = _token;
                constexpr int maxLen = 16;
                char buf[maxLen];
                snprintf(buf, maxLen, "@arg%d", int(params.size() + 1));
                Id *surrogateName = newId(buf);

                Lex(); // skip '{' or '['
                DestructuringDecl *destrDecl = newNode<DestructuringDecl>(arena(), keywordStart, destructurer == '{' ? DT_TABLE : DT_ARRAY);
                destrDecl->setExpression(surrogateName);

                parseDestructuringFields(destrDecl);

                Expect(destructurer == '{' ? '}' : ']');
                f->addParameter(argSpan, surrogateName->name(), nullptr);
                ParamDecl *p = params.back();
                p->setDestructuring(destrDecl);
                p->setTypeMask(destructurer == '{' ? (_RT_TABLE | _RT_INSTANCE | _RT_CLASS) : (_RT_ARRAY));

            }
            else {
                Id *paramname = (Id *)Expect(TK_IDENTIFIER);
                unsigned typeMask = _token == ':' ? parseTypeMask(false) : ~0u;
                Expr *defVal = NULL;
                if (_token == '=') {
                    Lex();
                    defVal = Expression(SQE_RVALUE);
                    defparams++;
                }
                else {
                    if (defparams > 0)
                        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "=");
                }

                // Use Id's span for parameter name span
                f->addParameter(paramname->sourceSpan(), paramname->name(), defVal); //-V522
                ParamDecl *p = params.back();
                p->setTypeMask(typeMask);
            }

            if(_token == ',') Lex();
            else if(_token != ')')
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ") or ,");
        }
    }

    Expect(')');
    unsigned resultTypeMask = _token == ':' ? parseTypeMask(false) : ~0u;

    Block *body = NULL;

    SQUnsignedInteger savedLangFeatures = _lang_features;
    _docObjectStack.push_back(&f->docObject);

    if (lambda) {
        Expr *expr = Expression(SQE_REGULAR);
        SourceSpan exprSpan = expr->sourceSpan();
        // For lambda, use expression span as the synthetic "return" span
        ReturnStatement *retStmt = newNode<ReturnStatement>(exprSpan, expr);
        retStmt->setIsLambda();
        body = newNode<Block>(arena(), exprSpan.start);
        body->addStatement(retStmt);
        body->setSpanEnd(exprSpan.end);
    }
    else {
        if (_token != '{')
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "{");
        checkBraceIndentationStyle();
        body = (Block *)parseStatement(false);
    }

    _docObjectStack.pop_back();

    f->setBody(body);

    f->setSourceName(_sourcename);
    f->setLambda(lambda);
    f->setResultTypeMask(resultTypeMask);

    _lang_features = savedLangFeatures;

    return f;
}


ImportStmt* SQParser::parseImportStatement()
{
    if (!_are_imports_still_allowed)
        reportDiagnostic(DiagnosticsId::DI_GENERAL_COMPILE_ERROR, "import statements must be at the top of the file");

    // Reuse lexer for the specific import syntax

    SourceLoc start = _lex.tokenStart();
    ImportStmt *importStmt = nullptr;

    const char *keyword = _lex._svalue;

    if (strcmp(keyword, "from") == 0) {
        // from "module" import x, y, z
        Lex();

        if (_token != TK_STRING_LITERAL)
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "module name");

        const char *moduleName = copyString(_lex._svalue);
        Lex();

        if (_token != TK_IDENTIFIER || strcmp(_lex._svalue, "import") != 0)
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "import");
        Lex();

        importStmt = newNode<ImportStmt>(arena(), start, moduleName, nullptr);
        importStmt->nameCol = _lex.tokenStart().column;
        importStmt->aliasCol = _lex.tokenStart().column;

        // Parse import list: x, y, z as foo, *
        do {
            SQModuleImportSlot slot{};

            if (_token != '*' && _token != TK_IDENTIFIER)
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "identifier or *");

            bool isWildcard = (_token == '*');

            slot.name = copyString(isWildcard ? "*" : _lex._svalue);
            SourceLoc slotLoc = _lex.tokenStart();
            slot.line = slotLoc.line;
            slot.column = slotLoc.column;
            Lex();

            // Check for an alias
            if (_token == TK_IDENTIFIER && strcmp(_lex._svalue, "as") == 0) {
                Lex();
                if (_token != TK_IDENTIFIER)
                    reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "identifier");
                if (isWildcard)
                    reportDiagnostic(DiagnosticsId::DI_GENERAL_COMPILE_ERROR, "cannot rename *");
                slot.alias = copyString(_lex._svalue);
                Lex();
            }

            importStmt->slots.push_back(slot);

            if (_token == ',')
                Lex();
            else
                break;
        } while (true);

    } else if (strcmp(keyword, "import") == 0) {
        // import "module" [as alias]
        Lex();

        if (_token != TK_STRING_LITERAL)
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "module name string");

        const char *moduleName = copyString(_lex._svalue);
        int32_t nameCol = _lex.tokenStart().column;
        int32_t aliasCol = nameCol;
        Lex();

        const char *moduleAlias = nullptr;
        if (_token == TK_IDENTIFIER && strcmp(_lex._svalue, "as") == 0) {
            Lex();
            if (_token != TK_IDENTIFIER)
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "identifier");
            moduleAlias = copyString(_lex._svalue);
            aliasCol = _lex.tokenStart().column;
            Lex();
        }

        importStmt = newNode<ImportStmt>(arena(), start, moduleName, moduleAlias);
        importStmt->nameCol = nameCol;
        importStmt->aliasCol = aliasCol;
    }

    if (importStmt) {
        // Set end position to current position (after the last parsed token)
        importStmt->setSpanEnd(_lex.tokenStart());
    }

    return importStmt;
}

} // namespace SQCompilation

#endif
