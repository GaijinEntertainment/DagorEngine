/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <stdarg.h>
#include <ctype.h>
#include <setjmp.h>
#include <algorithm>
#include "sqopcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqcompiler.h"
#include "sqfuncstate.h"
#include "sqoptimizer.h"
#include "sqlexer.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqast.h"
#include "sqastparser.h"
#include "sqastrender.h"
#include "sqastcodegen.h"
#include "optimizations/closureHoisting.h"
#include "sqbinaryast.h"
#include "sqcompilationcontext.h"
#include "static_analyser/analyser.h"


#define EXPR   1
#define OBJECT 2
#define BASE   3
#define LOCAL  4
#define OUTER  5

struct SQExpState {
    SQInteger  etype;       /* expr. type; one of EXPR, OBJECT, BASE, OUTER or LOCAL */
    SQInteger  epos;        /* expr. location on stack; -1 for OBJECT and BASE */
    bool       donot_get;   /* signal not to deref the next value */
    bool       is_assignable_var; // for LOCAL and OUTER
    bool       literal_field;

    bool isBinding() { return (etype == LOCAL || etype == OUTER) && !is_assignable_var; }
};

using namespace SQCompilation;

class SQCompiler //-V553
{
public:
    SQCompiler(SQVM *v, const char *code, size_t codeSize, const HSQOBJECT *bindings, const SQChar* sourcename, SQCompilationContext &ctx, bool lineinfo) :
        _lex(_ss(v), ctx, LM_LEGACY),
        _scopedconsts(_ss(v)->_alloc_ctx),
        _member_constant_keys_check(_ss(v)->_alloc_ctx),
        _ctx(ctx),
        _es()
    {
        _vm = v;
        _lex.Init(_ss(v), code, codeSize);
        _sourcename = SQString::Create(_ss(v), sourcename);
        _lineinfo = lineinfo;
        _scope.outers = 0;
        _scope.stacksize = 0;
        _expression_context = SQE_REGULAR;
        _num_initial_bindings = 0;
        _token = 0;
        _fs = nullptr;

        if (bindings) {
            assert(sq_type(*bindings) == OT_TABLE || sq_type(*bindings) == OT_NULL);
            if (sq_type(*bindings) == OT_TABLE) {
                _scopedconsts.push_back(*bindings);
                _num_initial_bindings = 1;
            }
        }
    }

    void reportDiagnostic(int32_t id, ...) {
        va_list vargs;
        va_start(vargs, id);

        _ctx.vreportDiagnostic((enum DiagnosticsId)id, line(), column(), width(), vargs);

        va_end(vargs);
    }

    bool IsConstant(const SQObject &name, SQObject &e)
    {
        if (IsLocalConstant(name, e))
            return true;
        if (IsGlobalConstant(name, e))
            return true;
        return false;
    }

    bool IsLocalConstant(const SQObject &name, SQObject &e)
    {
        SQObjectPtr val;
        for (SQInteger i = SQInteger(_scopedconsts.size()) - 1; i >= 0; --i) {
            SQObjectPtr &tbl = _scopedconsts[i];
            if (!sq_isnull(tbl) && _table(tbl)->Get(name, val)) {
                e = val;
                if (tbl._flags & SQOBJ_FLAG_IMMUTABLE)
                    e._flags |= SQOBJ_FLAG_IMMUTABLE;
                return true;
            }
        }
        return false;
    }

    bool IsGlobalConstant(const SQObject &name, SQObject &e)
    {
        SQObjectPtr val;
        if (_table(_ss(_vm)->_consts)->Get(name, val)) {
            e = val;
            return true;
        }
        return false;
    }

    SQInteger line() const { return _lex._tokenline; }
    SQInteger column() const { return _lex._tokencolumn; }
    SQInteger width() const { return _lex._currentcolumn - _lex._tokencolumn; }

    void ProcessDirective()
    {
        const SQChar *sval = _lex._svalue;

        if (strncmp(sval, _SC("pos:"), 4) == 0) {
            sval += 4;
            if (!isdigit(*sval))
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_LINENUM);
            SQChar * next = NULL;
            _lex._currentline = scstrtol(sval, &next, 10);
            if (!next || *next != ':') {
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ":");
                return;
            }
            next++;
            if (!isdigit(*next))
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_COLNUM);
            _lex._currentcolumn = scstrtol(next, NULL, 10);

            return;
        }

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

        _fs->lang_features = (_fs->lang_features | setFlags) & ~clearFlags;
        if (applyToDefault)
            _ss(_vm)->defaultLangFeatures = (_ss(_vm)->defaultLangFeatures | setFlags) & ~clearFlags;
    }

    void Lex()
    {
        _token = _lex.Lex();
        while (_token == TK_DIRECTIVE)
        {
            bool endOfLine = (_lex._prevtoken == _SC('\n'));
            ProcessDirective();
            _token = _lex.Lex();
            if (endOfLine)
                _lex._prevtoken = _SC('\n');
        }
    }

    const char *tok2str(SQInteger tok) {
        static char s[3];
        snprintf(s, sizeof s, "%c", (SQChar)tok);
        return s;
    }

    SQObject Expect(SQInteger tok)
    {

        if (_token != tok) {
            if (_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
                //do nothing
            }
            else {
                const SQChar *etypename;
                if (tok > 255) {
                    switch (tok)
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
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, tok2str(tok));
            }
        }
        SQObjectPtr ret;
        switch (tok)
        {
        case TK_IDENTIFIER:
            ret = _fs->CreateString(_lex._svalue);
            break;
        case TK_STRING_LITERAL:
            ret = _fs->CreateString(_lex._svalue, _lex._longstr.size() - 1);
            break;
        case TK_INTEGER:
            ret = SQObjectPtr(_lex._nvalue);
            break;
        case TK_FLOAT:
            ret = SQObjectPtr(_lex._fvalue);
            break;
        }
        Lex();
        return ret;
    }
    bool IsEndOfStatement() { return ((_lex._prevtoken == _SC('\n')) || (_token == SQUIRREL_EOB) || (_token == _SC('}')) || (_token == _SC(';'))); }
    void OptionalSemicolon()
    {
        if (_token == _SC(';')) { Lex(); return; }
        if (!IsEndOfStatement()) {
            reportDiagnostic(DiagnosticsId::DI_END_OF_STMT_EXPECTED);
        }
    }
    void MoveIfCurrentTargetIsLocal() {
        SQInteger trg = _fs->TopTarget();
        if (_fs->IsLocal(trg)) {
            trg = _fs->PopTarget(); //pops the target and moves it
            _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), trg);
        }
    }
    void CleanupAfterError() {
        for (SQUnsignedInteger i = 0, n = _member_constant_keys_check.size(); i < n; ++i)
            delete _member_constant_keys_check[i];
        _member_constant_keys_check.resize(0);
    }
    bool Compile(SQObjectPtr &o)
    {
        _scopedconsts.push_back();
        SQFuncState funcstate(_ss(_vm), NULL, _ctx);
        funcstate._name = SQString::Create(_ss(_vm), _SC("__main__"));
        _fs = &funcstate; //-V506 It's ok to store address of local funcsate, since no code is executed after Compile() returns
        _fs->AddParameter(_fs->CreateString(_SC("this")));
        _fs->AddParameter(_fs->CreateString(_SC("vargv")));
        _fs->_varparams = true;
        _fs->_sourcename = _sourcename;
        SQInteger stacksize = _fs->GetStackSize();
        if (setjmp(_ctx.errorJump()) == 0) {
            Lex();
            while (_token > 0) {
                Statement();
                if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
            }
            _fs->SetStackSize(stacksize);
            _fs->AddLineInfos(_lex._currentline, _lineinfo, true);
            _fs->AddInstruction(_OP_RETURN, 0xFF);

            assert(_member_constant_keys_check.empty());

            if (!(_fs->lang_features & LF_DISABLE_OPTIMIZER)) {
                SQOptimizer opt(*_fs);
                opt.optimize();
            }

            _fs->SetStackSize(0);
            o = _fs->BuildProto();
        }
        else {
            CleanupAfterError();
            return false;
        }
        assert(_scopedconsts.size() == 1 + _num_initial_bindings);
        return true;
    }
    void Statements()
    {
        while (_token != _SC('}') && _token != TK_DEFAULT && _token != TK_CASE) {
            Statement();
            if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
        }
    }
    void Statement(bool closeframe = true)
    {
        _fs->AddLineInfos(_lex._currentline, _lineinfo);
        switch (_token) {
        case _SC(';'):  Lex();                  break;
        case TK_IF:     IfStatement();          break;
        case TK_WHILE:      WhileStatement();       break;
        case TK_DO:     DoWhileStatement();     break;
        case TK_FOR:        ForStatement();         break;
        case TK_FOREACH:    ForEachStatement();     break;
        case TK_SWITCH: SwitchStatement();      break;
        case TK_LOCAL:
        case TK_LET:
            LocalDeclStatement();
            break;
        case TK_RETURN:
        case TK_YIELD: {
            SQOpcode op;
            if (_token == TK_RETURN) {
                op = _OP_RETURN;
            }
            else {
                op = _OP_YIELD;
                _fs->_bgenerator = true;
            }
            Lex();
            if (!IsEndOfStatement()) {
                SQInteger retexp = _fs->GetCurrentPos() + 1;
                Expression(SQE_RVALUE);
                if (op == _OP_RETURN && _fs->_traps > 0)
                    _fs->AddInstruction(_OP_POPTRAP, _fs->_traps, 0);
                _fs->_returnexp = retexp;
                _fs->AddInstruction(op, 1, _fs->PopTarget(), _fs->GetStackSize());
            }
            else {
                if (op == _OP_RETURN && _fs->_traps > 0)
                    _fs->AddInstruction(_OP_POPTRAP, _fs->_traps, 0);
                _fs->_returnexp = -1;
                _fs->AddInstruction(op, 0xFF, 0, _fs->GetStackSize());
            }
            break; }
        case TK_BREAK:
            if (_fs->_breaktargets.size() <= 0)
                reportDiagnostic(DiagnosticsId::DI_LOOP_CONTROLLER_NOT_IN_LOOP, "break");
            if (_fs->_breaktargets.top() > 0) {
                _fs->AddInstruction(_OP_POPTRAP, _fs->_breaktargets.top(), 0);
            }
            RESOLVE_OUTERS();
            _fs->AddInstruction(_OP_JMP, 0, -1234);
            _fs->_unresolvedbreaks.push_back(_fs->GetCurrentPos());
            Lex();
            break;
        case TK_CONTINUE:
            if (_fs->_continuetargets.size() <= 0)
                reportDiagnostic(DiagnosticsId::DI_LOOP_CONTROLLER_NOT_IN_LOOP, "continue");
            if (_fs->_continuetargets.top() > 0) {
                _fs->AddInstruction(_OP_POPTRAP, _fs->_continuetargets.top(), 0);
            }
            RESOLVE_OUTERS();
            _fs->AddInstruction(_OP_JMP, 0, -1234);
            _fs->_unresolvedcontinues.push_back(_fs->GetCurrentPos());
            Lex();
            break;
        case TK_FUNCTION:
            LocalFunctionDeclStmt(false);
            break;
        case TK_CLASS:
            LocalClassDeclStmt(false);
            break;
        case TK_ENUM:
            EnumStatement(false);
            break;
        case _SC('{'): {
            BEGIN_SCOPE();
            Lex();
            Statements();
            Expect(_SC('}'));
            if (closeframe) {
                END_SCOPE();
            }
            else {
                END_SCOPE_NO_CLOSE();
            }
        }
        break;
        case TK_TRY:
            TryCatchStatement();
            break;
        case TK_THROW:
            Lex();
            Expression(SQE_RVALUE);
            _fs->AddInstruction(_OP_THROW, _fs->PopTarget());
            break;
        case TK_CONST:
            ConstStatement(false);
            break;
        case TK_GLOBAL:
            Lex();
            if (_token == TK_CONST)
                ConstStatement(true);
            else if (_token == TK_ENUM)
                EnumStatement(true);
            else
                reportDiagnostic(DiagnosticsId::DI_GLOBAL_CONSTS_ONLY);
            break;
        default:
            Expression(SQE_REGULAR);
            _fs->DiscardTarget();
            //_fs->PopTarget();
            break;
        }
        _fs->SnoozeOpt();
    }
    void EmitDerefOp(SQOpcode op)
    {
        SQInteger val = _fs->PopTarget();
        SQInteger key = _fs->PopTarget();
        SQInteger src = _fs->PopTarget();
        _fs->AddInstruction(op, _fs->PushTarget(), src, key, val);
    }
    void Emit2ArgsOP(SQOpcode op, SQInteger p3 = 0)
    {
        SQInteger p2 = _fs->PopTarget(); //src in OP_GET
        SQInteger p1 = _fs->PopTarget(); //key in OP_GET
        _fs->AddInstruction(op, _fs->PushTarget(), p1, p2, p3);
    }
    void EmitCompoundArith(SQInteger tok, SQInteger etype, SQInteger pos)
    {
        /* Generate code depending on the expression type */
        switch (etype) {
        case LOCAL: {
            SQInteger p2 = _fs->PopTarget(); //src in OP_GET
            SQInteger p1 = _fs->PopTarget(); //key in OP_GET
            _fs->PushTarget(p1);
            //EmitCompArithLocal(tok, p1, p1, p2);
            _fs->AddInstruction(ChooseArithOpByToken(tok), p1, p2, p1, 0);
            _fs->SnoozeOpt();
        }
                    break;
        case OBJECT:
        case BASE:
        {
            SQInteger val = _fs->PopTarget();
            SQInteger key = _fs->PopTarget();
            SQInteger src = _fs->PopTarget();
            /* _OP_COMPARITH mixes dest obj and source val in the arg1 */
            _fs->AddInstruction(_OP_COMPARITH, _fs->PushTarget(), (src << 16) | val, key, ChooseCompArithCharByToken(tok));
        }
        break;
        case OUTER:
        {
            SQInteger val = _fs->TopTarget();
            SQInteger tmp = _fs->PushTarget();
            _fs->AddInstruction(_OP_GETOUTER, tmp, pos);
            _fs->AddInstruction(ChooseArithOpByToken(tok), tmp, val, tmp, 0);
            _fs->PopTarget();
            _fs->PopTarget();
            _fs->AddInstruction(_OP_SETOUTER, _fs->PushTarget(), pos, tmp);
        }
        break;
        }
    }
    void CommaExpr(SQExpressionContext expression_context)
    {
        for (Expression(expression_context); _token == ','; _fs->PopTarget(), Lex(), CommaExpr(expression_context));
    }
    void Expression(SQExpressionContext expression_context)
    {
        SQExpressionContext saved_expression_context = _expression_context;
        _expression_context = expression_context;

        if (_ss(_vm)->_lineInfoInExpressions && _fs)
            _fs->AddLineInfos(_lex._prevtoken == _SC('\n') ? _lex._lasttokenline : _lex._currentline, _lineinfo, false);

        SQExpState es = _es;
        _es.etype = EXPR;
        _es.epos = -1;
        _es.donot_get = false;
        _es.literal_field = false;
        const SQChar *sval = _lex._svalue;
        LogicalNullCoalesceExp();

        if (_token == TK_INEXPR_ASSIGNMENT && (expression_context == SQE_REGULAR || expression_context == SQE_FUNCTION_ARG))
            reportDiagnostic(DiagnosticsId::DI_INCORRECT_INTRA_ASSIGN);

        switch (_token) {
        case _SC('='):
        case TK_INEXPR_ASSIGNMENT:
        case TK_NEWSLOT:
        case TK_MINUSEQ:
        case TK_PLUSEQ:
        case TK_MULEQ:
        case TK_DIVEQ:
        case TK_MODEQ: {
            SQInteger op = _token;
            SQInteger ds = _es.etype;
            SQInteger pos = _es.epos;
            bool literalField = _es.literal_field;
            if (ds == EXPR)
                reportDiagnostic(DiagnosticsId::DI_ASSIGN_TO_EXPR);
            else if (ds == BASE)
                reportDiagnostic(DiagnosticsId::DI_BASE_NOT_MODIFIABLE);
            else if (_es.isBinding() && _token != TK_INEXPR_ASSIGNMENT)
                reportDiagnostic(DiagnosticsId::DI_ASSIGN_TO_BINDING, sval);
            Lex(); Expression(SQE_RVALUE);

            switch (op) {
            case TK_NEWSLOT:
                if (ds == OBJECT)
                    EmitDerefOp(_OP_NEWSLOT);
                else //if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
                    reportDiagnostic(DiagnosticsId::DI_LOCAL_SLOT_CREATE);
                break;

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

                switch (ds) {
                case LOCAL:
                {
                    SQInteger src = _fs->PopTarget();
                    SQInteger dst = _fs->TopTarget();
                    _fs->AddInstruction(_OP_MOVE, dst, src);
                }
                break;
                case OBJECT:
                case BASE:
                    EmitDerefOp(literalField ? _OP_SET_LITERAL : _OP_SET);
                    SQ_STATIC_ASSERT(_OP_DATA_NOP == 0);
                    if (literalField)
                        _fs->AddInstruction(SQOpcode(0), 0, 0, 0, 0);//hint
                    break;
                case OUTER:
                {
                    SQInteger src = _fs->PopTarget();
                    SQInteger dst = _fs->PushTarget();
                    _fs->AddInstruction(_OP_SETOUTER, dst, pos, src);
                }
                }
                break;
            case TK_MINUSEQ:
            case TK_PLUSEQ:
            case TK_MULEQ:
            case TK_DIVEQ:
            case TK_MODEQ:
                EmitCompoundArith(op, ds, pos);
                break;
            }
        }
                       break;
        case _SC('?'): {
            Lex();
            _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
            SQInteger jzpos = _fs->GetCurrentPos();
            SQInteger trg = _fs->PushTarget();
            Expression(SQE_RVALUE);
            SQInteger first_exp = _fs->PopTarget();
            if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            SQInteger endfirstexp = _fs->GetCurrentPos();
            _fs->AddInstruction(_OP_JMP, 0, 0);
            Expect(_SC(':'));
            SQInteger jmppos = _fs->GetCurrentPos();
            Expression(SQE_RVALUE);
            SQInteger second_exp = _fs->PopTarget();
            if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SetInstructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
            _fs->SetInstructionParam(jzpos, 1, endfirstexp - jzpos + 1);
            _fs->SnoozeOpt();
        }
                       break;
        }
        _es = es;
        _expression_context = saved_expression_context;
    }
    template<typename T> void INVOKE_EXP(T f)
    {
        SQExpState es = _es;
        _es.etype = EXPR;
        _es.epos = -1;
        _es.donot_get = false;
        _es.literal_field = false;
        (this->*f)();
        _es = es;
    }
    template<typename T> void BIN_EXP(SQOpcode op, T f, SQInteger op3 = 0)
    {
        _expression_context = SQE_RVALUE;
        Lex();
        INVOKE_EXP(f);
        SQInteger op1 = _fs->PopTarget(); SQInteger op2 = _fs->PopTarget();
        _fs->AddInstruction(op, _fs->PushTarget(), op1, op2, op3);
        _es.etype = EXPR;
    }
    void LogicalNullCoalesceExp()
    {
        LogicalOrExp();
        for (;;) if (_token == TK_NULLCOALESCE) {
            SQInteger first_exp = _fs->PopTarget();
            SQInteger trg = _fs->PushTarget();
            _fs->AddInstruction(_OP_NULLCOALESCE, trg, 0, first_exp, 0);
            SQInteger jpos = _fs->GetCurrentPos();
            if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            Lex(); INVOKE_EXP(&SQCompiler::LogicalNullCoalesceExp);
            _fs->SnoozeOpt();
            SQInteger second_exp = _fs->PopTarget();
            if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SnoozeOpt();
            _fs->SetInstructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
            _es.etype = EXPR;
            break;
        }
        else return;
    }
    void LogicalOrExp()
    {
        LogicalAndExp();
        for (;;) if (_token == TK_OR) {
            SQInteger first_exp = _fs->PopTarget();
            SQInteger trg = _fs->PushTarget();
            _fs->AddInstruction(_OP_OR, trg, 0, first_exp, 0);
            SQInteger jpos = _fs->GetCurrentPos();
            if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            Lex(); INVOKE_EXP(&SQCompiler::LogicalOrExp);
            _fs->SnoozeOpt();
            SQInteger second_exp = _fs->PopTarget();
            if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SnoozeOpt();
            _fs->SetInstructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
            _es.etype = EXPR;
            break;
        }
        else return;
    }
    void LogicalAndExp()
    {
        BitwiseOrExp();
        for (;;) switch (_token) {
        case TK_AND: {
            SQInteger first_exp = _fs->PopTarget();
            SQInteger trg = _fs->PushTarget();
            _fs->AddInstruction(_OP_AND, trg, 0, first_exp, 0);
            SQInteger jpos = _fs->GetCurrentPos();
            if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            Lex(); INVOKE_EXP(&SQCompiler::LogicalAndExp);
            _fs->SnoozeOpt();
            SQInteger second_exp = _fs->PopTarget();
            if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SnoozeOpt();
            _fs->SetInstructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
            _es.etype = EXPR;
            break;
        }

        default:
            return;
        }
    }
    void BitwiseOrExp()
    {
        BitwiseXorExp();
        for (;;) if (_token == _SC('|'))
        {
            BIN_EXP(_OP_BITW, &SQCompiler::BitwiseXorExp, BW_OR);
        }
        else return;
    }
    void BitwiseXorExp()
    {
        BitwiseAndExp();
        for (;;) if (_token == _SC('^'))
        {
            BIN_EXP(_OP_BITW, &SQCompiler::BitwiseAndExp, BW_XOR);
        }
        else return;
    }
    void BitwiseAndExp()
    {
        EqExp();
        for (;;) if (_token == _SC('&'))
        {
            BIN_EXP(_OP_BITW, &SQCompiler::EqExp, BW_AND);
        }
        else return;
    }
    void EqExp()
    {
        CompExp();
        for (;;) switch (_token) {
        case TK_EQ: BIN_EXP(_OP_EQ, &SQCompiler::CompExp); break;
        case TK_NE: BIN_EXP(_OP_NE, &SQCompiler::CompExp); break;
        case TK_3WAYSCMP: BIN_EXP(_OP_CMP, &SQCompiler::CompExp, CMP_3W); break;
        default: return;
        }
    }
    void CompExp()
    {
        ShiftExp();
        for (;;) switch (_token) {
        case _SC('>'): BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp, CMP_G); break;
        case _SC('<'): BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp, CMP_L); break;
        case TK_GE: BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp, CMP_GE); break;
        case TK_LE: BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp, CMP_LE); break;
        case TK_IN: BIN_EXP(_OP_EXISTS, &SQCompiler::ShiftExp); break;
        case TK_INSTANCEOF: BIN_EXP(_OP_INSTANCEOF, &SQCompiler::ShiftExp); break;
        case TK_NOT: {
            Lex();
            if (_token == TK_IN) {
                BIN_EXP(_OP_EXISTS, &SQCompiler::ShiftExp);
                SQInteger src = _fs->PopTarget();
                _fs->AddInstruction(_OP_NOT, _fs->PushTarget(), src);
            }
            else
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "in");
        }
        default: return;
        }
    }
    void ShiftExp()
    {
        PlusExp();
        for (;;) switch (_token) {
        case TK_USHIFTR: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp, BW_USHIFTR); break;
        case TK_SHIFTL: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp, BW_SHIFTL); break;
        case TK_SHIFTR: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp, BW_SHIFTR); break;
        default: return;
        }
    }
    SQOpcode ChooseArithOpByToken(SQInteger tok)
    {
        switch (tok) {
        case TK_PLUSEQ: case '+': return _OP_ADD;
        case TK_MINUSEQ: case '-': return _OP_SUB;
        case TK_MULEQ: case '*': return _OP_MUL;
        case TK_DIVEQ: case '/': return _OP_DIV;
        case TK_MODEQ: case '%': return _OP_MOD;
        default: assert(0);
        }
        return _OP_ADD;
    }
    SQInteger ChooseCompArithCharByToken(SQInteger tok)
    {
        SQInteger oper;
        switch (tok) {
        case TK_MINUSEQ: oper = '-'; break;
        case TK_PLUSEQ: oper = '+'; break;
        case TK_MULEQ: oper = '*'; break;
        case TK_DIVEQ: oper = '/'; break;
        case TK_MODEQ: oper = '%'; break;
        default: oper = 0; //shut up compiler
            assert(0); break;
        };
        return oper;
    }
    void PlusExp()
    {
        MultExp();
        for (;;) switch (_token) {
        case _SC('+'): case _SC('-'):
            BIN_EXP(ChooseArithOpByToken(_token), &SQCompiler::MultExp); break;
        default: return;
        }
    }

    void MultExp()
    {
        PrefixedExpr();
        for (;;) switch (_token) {
        case _SC('*'): case _SC('/'): case _SC('%'):
            BIN_EXP(ChooseArithOpByToken(_token), &SQCompiler::PrefixedExpr); break;
        default: return;
        }
    }
    //if 'pos' != -1 the previous variable is a local variable
    void PrefixedExpr()
    {
        SQInteger pos = Factor();
        bool nextIsNullable = false;
        for (;;) {
            switch (_token) {
            case _SC('.'):
            case TK_NULLGETSTR: {
                SQInteger flags = 0;
                if (_token == TK_NULLGETSTR || nextIsNullable)
                {
                    flags = OP_GET_FLAG_NO_ERROR;
                    nextIsNullable = true;
                }
                pos = -1;
                bool canBeLiteral = _es.etype != BASE && _token == _SC('.');//todo: we can support def delegate and nullable also.
                Lex();

                SQObjectPtr constant = Expect(TK_IDENTIFIER);
                if (CanBeDefaultDelegate(constant))
                    flags |= OP_GET_FLAG_ALLOW_DEF_DELEGATE;
                _es.literal_field = canBeLiteral && !(flags & (OP_GET_FLAG_NO_ERROR | OP_GET_FLAG_ALLOW_DEF_DELEGATE));//todo: we can support def delegate and nullable also.

                SQInteger constantI = _fs->GetConstant(constant);
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), constantI);
                if (_es.etype == BASE) {
                    Emit2ArgsOP(_OP_GET, flags);
                    pos = _fs->TopTarget();
                    _es.etype = EXPR;
                    _es.epos = pos;
                }
                else {
                    //todo:we can support null navigation as well
                    if (NeedGet()) {
                        if (!_es.literal_field)
                        {
                            Emit2ArgsOP(_OP_GET, flags);
                        }
                        else {
                            Emit2ArgsOP(_OP_GET_LITERAL, flags);
                            SQ_STATIC_ASSERT(_OP_DATA_NOP == 0);
                            _fs->AddInstruction(SQOpcode(0), 0, 0, 0, 0);//hint
                        }
                    }
                    _es.etype = OBJECT;
                }
                break;
            }
            case _SC('['):
            case TK_NULLGETOBJ: {
                SQInteger flags = 0;
                if (_token == TK_NULLGETOBJ || nextIsNullable)
                {
                    flags = OP_GET_FLAG_NO_ERROR;
                    nextIsNullable = true;
                }
                if (_lex._prevtoken == _SC('\n'))
                    reportDiagnostic(DiagnosticsId::DI_BROKEN_SLOT_DECLARATION);
                _es.literal_field = false;
                Lex(); Expression(SQE_RVALUE); Expect(_SC(']'));
                pos = -1;
                if (_es.etype == BASE) {
                    Emit2ArgsOP(_OP_GET, flags);
                    pos = _fs->TopTarget();
                    _es.etype = EXPR;
                    _es.epos = pos;
                }
                else {
                    if (NeedGet()) {
                        Emit2ArgsOP(_OP_GET, flags);
                    }
                    _es.etype = OBJECT;
                }
                break;
            }
            case TK_MINUSMINUS:
            case TK_PLUSPLUS:
            {
                nextIsNullable = false;
                if (IsEndOfStatement()) return;
                SQInteger diff = (_token == TK_MINUSMINUS) ? -1 : 1;
                Lex();
                if (_es.isBinding())
                    reportDiagnostic(DiagnosticsId::DI_CANNOT_INC_DEC, "a binding");
                switch (_es.etype)
                {
                case EXPR:
                    reportDiagnostic(DiagnosticsId::DI_CANNOT_INC_DEC, "an expression");
                    break;
                case BASE:
                    reportDiagnostic(DiagnosticsId::DI_BASE_NOT_MODIFIABLE);
                    break;
                case OBJECT:
                    if (_es.donot_get == true) {
                        reportDiagnostic(DiagnosticsId::DI_CANNOT_INC_DEC, "an expression");
                        break;
                    } //mmh dor this make sense?
                    Emit2ArgsOP(_OP_PINC, diff);
                    break;
                case LOCAL: {
                    SQInteger src = _fs->PopTarget();
                    _fs->AddInstruction(_OP_PINCL, _fs->PushTarget(), src, 0, diff);
                }
                            break;
                case OUTER: {
                    SQInteger tmp1 = _fs->PushTarget();
                    SQInteger tmp2 = _fs->PushTarget();
                    _fs->AddInstruction(_OP_GETOUTER, tmp2, _es.epos);
                    _fs->AddInstruction(_OP_PINCL, tmp1, tmp2, 0, diff);
                    _fs->AddInstruction(_OP_SETOUTER, tmp2, _es.epos, tmp2);
                    _fs->PopTarget();
                }
                }
                _es.etype = EXPR;
            }
            return;
            break;
            case _SC('('):
            case TK_NULLCALL: {
                SQInteger nullcall = (_token == TK_NULLCALL || nextIsNullable);
                nextIsNullable = !!nullcall;
                switch (_es.etype) {
                case OBJECT: {
                    if (!nullcall) {
                        SQInteger key = _fs->PopTarget();  /* location of the key */
                        SQInteger table = _fs->PopTarget();  /* location of the object */
                        SQInteger closure = _fs->PushTarget(); /* location for the closure */
                        SQInteger ttarget = _fs->PushTarget(); /* location for 'this' pointer */
                        _fs->AddInstruction(_OP_PREPCALL, closure, key, table, ttarget);
                    }
                    else {
                        SQInteger self = _fs->GetUpTarget(1);  /* location of the object */
                        SQInteger storedSelf = _fs->PushTarget();
                        _fs->AddInstruction(_OP_MOVE, storedSelf, self);
                        _fs->PopTarget();
                        Emit2ArgsOP(_OP_GET, OP_GET_FLAG_NO_ERROR | OP_GET_FLAG_ALLOW_DEF_DELEGATE);
                        SQInteger ttarget = _fs->PushTarget();
                        _fs->AddInstruction(_OP_MOVE, ttarget, storedSelf);
                    }
                    break;
                }
                case BASE:
                    //Emit2ArgsOP(_OP_GET);
                    _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
                    break;
                case OUTER:
                    _fs->AddInstruction(_OP_GETOUTER, _fs->PushTarget(), _es.epos);
                    _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
                    break;
                default:
                    _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
                }
                _es.etype = EXPR;
                Lex();
                FunctionCallArgs(nullcall);
                break;
            }
            default: return;
            }
        }
    }
    SQInteger Factor()
    {
        if ((_token == TK_LOCAL || _token == TK_LET)
            && (_expression_context == SQE_IF || _expression_context == SQE_SWITCH || _expression_context == SQE_LOOP_CONDITION))
        {
            Lex();
            if (_token != TK_IDENTIFIER)
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "identifier");

            SQObject id = _fs->CreateString(_lex._svalue);
            CheckDuplicateLocalIdentifier(id, _SC("In-expr local"), false);
            _fs->PushLocalVariable(id, _token == TK_LOCAL);
            SQInteger res = Factor();
            if (_token != TK_INEXPR_ASSIGNMENT)
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ":=");
            return res;
        }

        //_es.etype = EXPR;
        switch (_token)
        {
        case TK_STRING_LITERAL:
            _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_fs->CreateString(_lex._svalue, _lex._longstr.size() - 1)));
            Lex();
            break;
        case TK_BASE:
            Lex();
            _fs->AddInstruction(_OP_GETBASE, _fs->PushTarget());
            _es.etype = BASE;
            _es.epos = _fs->TopTarget();
            return (_es.epos);
            break;
        case TK_IDENTIFIER:
        case TK_CONSTRUCTOR:
        case TK_THIS: {
            bool assignable = false;
            if (_token == TK_IDENTIFIER && sq_isstring(_fs->_name)
                && strcmp(_stringval(_fs->_name), _lex._svalue) == 0
                && _fs->GetLocalVariable(_fs->_name, assignable) == -1)
            {
                _fs->AddInstruction(_OP_LOADCALLEE, _fs->PushTarget());
                Lex();
                break;
            }

            SQObject id;
            SQObject constant;

            switch (_token) {
            case TK_IDENTIFIER:  id = _fs->CreateString(_lex._svalue);       break;
            case TK_THIS:        id = _fs->CreateString(_SC("this"), 4);        break;
            case TK_CONSTRUCTOR: id = _fs->CreateString(_SC("constructor"), 11); break;
            }

            if (_string(id) == _string(_fs->_name)) {
                reportDiagnostic(DiagnosticsId::DI_VARNAME_CONFLICTS, _stringval(id));
            }

            SQInteger pos = -1;
            Lex();
            if ((pos = _fs->GetLocalVariable(id, assignable)) != -1) {
                /* Handle a local variable (includes 'this') */
                _fs->PushTarget(pos);
                _es.etype = LOCAL;
                _es.epos = pos;
                _es.is_assignable_var = assignable;
            }

            else if ((pos = _fs->GetOuterVariable(id, assignable)) != -1) {
                /* Handle a free var */
                if (NeedGet()) {
                    _es.epos = _fs->PushTarget();
                    _fs->AddInstruction(_OP_GETOUTER, _es.epos, pos);
                    /* _es.etype = EXPR; already default value */
                }
                else {
                    _es.etype = OUTER;
                    _es.epos = pos;
                    _es.is_assignable_var = assignable;
                }
            }

            else if (IsConstant(id, constant)) {
                /* Handle named constant */
                SQObjectPtr constval = constant;
                while (sq_type(constval) == OT_TABLE && (sq_objflags(constval) & SQOBJ_FLAG_IMMUTABLE) && _token == _SC('.')) {
                    Expect('.');
                    SQObject constid = Expect(TK_IDENTIFIER);
                    if (!_table(constval)->Get(constid, constval)) {
                        constval.Null();
                        reportDiagnostic(DiagnosticsId::DI_INVALID_ENUM, _stringval(constid), _stringval(id));
                    }
                }
                _es.epos = _fs->PushTarget();

                /* generate direct or literal function depending on size */
                SQObjectType ctype = sq_type(constval);
                switch (ctype) {
                case OT_INTEGER: EmitLoadConstInt(_integer(constval), _es.epos); break;
                case OT_FLOAT: EmitLoadConstFloat(_float(constval), _es.epos); break;
                case OT_BOOL: _fs->AddInstruction(_OP_LOADBOOL, _es.epos, _integer(constval)); break;
                default: _fs->AddInstruction(_OP_LOAD, _es.epos, _fs->GetConstant(constval)); break;
                }
                _es.etype = EXPR;
            }
            else {
                /* Handle a non-local variable, aka a field. Push the 'this' pointer on
                * the virtual stack (always found in offset 0, so no instruction needs to
                * be generated), and push the key next. Generate an _OP_LOAD instruction
                * for the latter. If we are not using the variable as a dref expr, generate
                * the _OP_GET instruction.
                */
                if (!(_fs->lang_features & LF_TOOLS_COMPILE_CHECK))
                    reportDiagnostic(DiagnosticsId::DI_UNKNOWN_SYMBOL, _stringval(id));

                _fs->PushTarget(0);
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
                if (NeedGet()) {
                    Emit2ArgsOP(_OP_GET);
                }
                _es.etype = OBJECT;
            }
            return _es.epos;
        }
                      break;
        case TK_DOUBLE_COLON:  // "::"
            if (_fs->lang_features & LF_FORBID_ROOT_TABLE)
                reportDiagnostic(DiagnosticsId::DI_ROOT_TABLE_FORBIDDEN);
            _fs->AddInstruction(_OP_LOADROOT, _fs->PushTarget());
            _es.etype = OBJECT;
            _token = _SC('.'); /* hack: drop into PrefixExpr, case '.'*/
            _es.epos = -1;
            return _es.epos;
            break;
        case TK_NULL:
            _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(), 1);
            Lex();
            break;
        case TK_INTEGER: EmitLoadConstInt(_lex._nvalue, -1); Lex();  break;
        case TK_FLOAT: EmitLoadConstFloat(_lex._fvalue, -1); Lex(); break;
        case TK_TRUE: case TK_FALSE:
            _fs->AddInstruction(_OP_LOADBOOL, _fs->PushTarget(), _token == TK_TRUE ? 1 : 0);
            Lex();
            break;
        case _SC('['): {
            _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), 0, 0, NOT_ARRAY);
            SQInteger apos = _fs->GetCurrentPos(), key = 0;
            Lex();
            while (_token != _SC(']')) {
#if SQ_LINE_INFO_IN_STRUCTURES
                if (key < 100)
                    _fs->AddLineInfos(_lex._currentline, false);
#endif
                Expression(SQE_RVALUE);
                if (_token == _SC(',')) Lex();
                SQInteger val = _fs->PopTarget();
                SQInteger array = _fs->TopTarget();
                _fs->AddInstruction(_OP_APPENDARRAY, array, val, AAT_STACK);
                key++;
            }
            _fs->SetInstructionParam(apos, 1, key);
            Lex();
        }
                       break;
        case _SC('{'):
            _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), 0, 0, NOT_TABLE);
            Lex(); ParseTableOrClass(_SC(','), _SC('}'));
            break;
        case TK_FUNCTION: FunctionExp(false); break;
        case _SC('@'): FunctionExp(true); break;
        case TK_CLASS: Lex(); ClassExp(); break;
        case _SC('-'):
            Lex();
            switch (_token) {
            case TK_INTEGER: EmitLoadConstInt(-_lex._nvalue, -1); Lex(); break;
            case TK_FLOAT: EmitLoadConstFloat(-_lex._fvalue, -1); Lex(); break;
            default: UnaryOP(_OP_NEG);
            }
            break;
        case _SC('!'): Lex(); UnaryOP(_OP_NOT); break;
        case _SC('~'):
            Lex();
            if (_token == TK_INTEGER) { EmitLoadConstInt(~_lex._nvalue, -1); Lex(); break; }
            UnaryOP(_OP_BWNOT);
            break;
        case TK_TYPEOF: Lex(); UnaryOP(_OP_TYPEOF); break;
        case TK_RESUME: Lex(); UnaryOP(_OP_RESUME); break;
        case TK_CLONE: Lex(); UnaryOP(_OP_CLONE); break;
        case TK_MINUSMINUS:
        case TK_PLUSPLUS:
            PrefixIncDec(_token);
            break;
        case TK_DELETE: DeleteExpr(); break;
        case _SC('('): Lex(); Expression(_expression_context); Expect(_SC(')'));
            break;
        case TK___LINE__: EmitLoadConstInt(_lex._currentline, -1); Lex(); break;
        case TK___FILE__: _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_sourcename)); Lex(); break;
        default:
            reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "expression");
        }
        _es.etype = EXPR;
        return -1;
    }
    void EmitLoadConstInt(SQInteger value, SQInteger target)
    {
        if (target < 0) {
            target = _fs->PushTarget();
        }
        if (value <= INT_MAX && value > INT_MIN) { //does it fit in 32 bits?
            _fs->AddInstruction(_OP_LOADINT, target, value);
        }
        else {
            _fs->AddInstruction(_OP_LOAD, target, _fs->GetNumericConstant(value));
        }
    }
    void EmitLoadConstFloat(SQFloat value, SQInteger target)
    {
        if (target < 0) {
            target = _fs->PushTarget();
        }
        if (sizeof(SQFloat) == sizeof(SQInt32)) {
            _fs->AddInstruction(_OP_LOADFLOAT, target, *((SQInt32 *)&value));
        }
        else {
            _fs->AddInstruction(_OP_LOAD, target, _fs->GetNumericConstant(value));
        }
    }
    void UnaryOP(SQOpcode op)
    {
        PrefixedExpr();
        if (_fs->_targetstack.size() == 0)
            reportDiagnostic(DiagnosticsId::DI_CANNOT_EVAL_UNARY);
        SQInteger src = _fs->PopTarget();
        _fs->AddInstruction(op, _fs->PushTarget(), src);
    }
    bool NeedGet()
    {
        switch (_token) {
        case _SC('('): case TK_NULLCALL:
            return false;
        case _SC('='): case TK_NEWSLOT: case TK_MODEQ: case TK_MULEQ:
        case TK_DIVEQ: case TK_MINUSEQ: case TK_PLUSEQ:
            if (_expression_context != SQE_REGULAR)
                reportDiagnostic(DiagnosticsId::DI_ASSIGN_TO_EXPR);
            return false;
        case TK_PLUSPLUS: case TK_MINUSMINUS:
            if (!IsEndOfStatement()) {
                return false;
            }
            break;
        }
        return !_es.donot_get || _token == _SC('.') || _token == _SC('[');
    }
    void FunctionCallArgs(bool nullcall)
    {
        SQInteger nargs = 1;//this
        while (_token != _SC(')')) {
            Expression(SQE_FUNCTION_ARG);
            MoveIfCurrentTargetIsLocal();
            nargs++;
            if (_token == _SC(',')) {
                Lex();
            }
        }
        Lex();
        for (SQInteger i = 0; i < (nargs - 1); i++) _fs->PopTarget();
        SQInteger stackbase = _fs->PopTarget();
        SQInteger closure = _fs->PopTarget();
        SQInteger target = _fs->PushTarget();
        assert(target >= -1);
        assert(target < 255);
        _fs->AddInstruction(nullcall ? _OP_NULLCALL : _OP_CALL, target, closure, stackbase, nargs);
    }
    bool CheckMemberUniqueness(sqvector<SQObject> &vec, SQObject &obj) {
        for (SQUnsignedInteger i = 0, n = vec.size(); i < n; ++i) {
            if (vec[i]._type == obj._type && vec[i]._unVal.raw == obj._unVal.raw) {
                if (sq_isstring(obj))
                    reportDiagnostic(DiagnosticsId::DI_DUPLICATE_KEY, sq_objtostring(&obj));
                else
                    reportDiagnostic(DiagnosticsId::DI_DUPLICATE_KEY, "");
                return false;
            }
        }
        vec.push_back(obj);
        return true;
    }
    void ParseTableOrClass(SQInteger separator, SQInteger terminator)
    {
        SQInteger tpos = _fs->GetCurrentPos(), nkeys = 0;
        sqvector<SQObject> *memberConstantKeys = new sqvector<SQObject>(_fs->_sharedstate->_alloc_ctx);
        _member_constant_keys_check.push_back(memberConstantKeys);
        NewObjectType otype = separator == _SC(',') ? NOT_TABLE : NOT_CLASS;
        while (_token != terminator) {
#if SQ_LINE_INFO_IN_STRUCTURES
            if (nkeys < 100)
                _fs->AddLineInfos(_lex._currentline, false);
#endif
            bool isstatic = false;
            //check if is an static
            if (otype == NOT_CLASS) {
                if (_token == TK_STATIC) {
                    isstatic = true;
                    Lex();
                }
            }
            switch (_token) {
            case TK_FUNCTION:
            case TK_CONSTRUCTOR: {
                SQInteger tk = _token;
                Lex();
                SQObject id = tk == TK_FUNCTION ? Expect(TK_IDENTIFIER) : _fs->CreateString(_SC("constructor"));
                CheckMemberUniqueness(*memberConstantKeys, id);
                Expect(_SC('('));
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
                CreateFunction(id);
                _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
            }
                                 break;
            case _SC('['): {
                Lex();

                SQObjectPtr firstId;
                SQUnsignedInteger prevInstrSize = _fs->_instructions.size();
                if (_token == TK_STRING_LITERAL)
                    firstId = _fs->CreateString(_lex._svalue, _lex._longstr.size() - 1);
                else if (_token == TK_INTEGER)
                    firstId = SQObjectPtr(_lex._nvalue);
                Expression(SQE_RVALUE);
                if (!sq_isnull(firstId) && _fs->_instructions.size() == prevInstrSize + 1) {
                    unsigned char op = _fs->_instructions.back().op;
                    if (op == _OP_LOAD || op == _OP_LOADINT)
                        CheckMemberUniqueness(*memberConstantKeys, firstId);
                }
                Expect(_SC(']'));
                Expect(_SC('=')); Expression(SQE_RVALUE);
                break;
            }
            case TK_STRING_LITERAL: //JSON
                if (otype == NOT_TABLE) { //only works for tables
                    SQObject id = Expect(TK_STRING_LITERAL);
                    CheckMemberUniqueness(*memberConstantKeys, id);
                    _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
                    Expect(_SC(':')); Expression(SQE_RVALUE);
                    break;
                }  //-V796
            default: {
                SQObject id = Expect(TK_IDENTIFIER);
                CheckMemberUniqueness(*memberConstantKeys, id);
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));

                if ((otype == NOT_TABLE) &&
                    (_token == TK_IDENTIFIER || _token == separator || _token == terminator || _token == _SC('[')
                        || _token == TK_FUNCTION)) {
                    SQObject constant;
                    SQInteger pos = -1;
                    bool assignable = false;
                    if ((pos = _fs->GetLocalVariable(id, assignable)) != -1)
                        _fs->PushTarget(pos);
                    else if ((pos = _fs->GetOuterVariable(id, assignable)) != -1)
                        _fs->AddInstruction(_OP_GETOUTER, _fs->PushTarget(), pos);
                    else if (IsConstant(id, constant))
                        _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(constant));
                    else
                        reportDiagnostic(DiagnosticsId::DI_INCORRECT_INTRA_ASSIGN, _stringval(id));
                }
                else {
                    Expect(_SC('=')); Expression(SQE_RVALUE);
                }
            }
            }
            if (_token == separator) Lex();//optional comma/semicolon
            nkeys++;
            SQInteger val = _fs->PopTarget();
            SQInteger key = _fs->PopTarget();
            unsigned char flags = isstatic ? NEW_SLOT_STATIC_FLAG : 0;
            SQInteger table = _fs->TopTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE
            if (otype == NOT_TABLE) {
                _fs->AddInstruction(_OP_NEWSLOT, 0xFF, table, key, val);
            }
            else {
                _fs->AddInstruction(_OP_NEWSLOTA, flags, table, key, val); //for classes only
            }
        }
        if (otype == NOT_TABLE)
            _fs->SetInstructionParam(tpos, 1, nkeys);
        Lex();
        delete memberConstantKeys;
        _member_constant_keys_check.pop_back();
    }
    void CheckDuplicateLocalIdentifier(const SQObject &name, const SQChar *desc, bool ignore_global_consts)
    {
        bool assignable = false;
        if (_fs->GetLocalVariable(name, assignable) >= 0)
            reportDiagnostic(DiagnosticsId::DI_CONFLICTS_WITH, desc, _string(name)->_val, "existing local variable");
        if (_string(name) == _string(_fs->_name))
            reportDiagnostic(DiagnosticsId::DI_CONFLICTS_WITH, desc, _stringval(name), "function name");

        SQObject constant;
        if (ignore_global_consts ? IsLocalConstant(name, constant) : IsConstant(name, constant))
            reportDiagnostic(DiagnosticsId::DI_CONFLICTS_WITH, desc, _stringval(name), "existing constant/enum/import");
    }

    void LocalFunctionDeclStmt(bool assignable)
    {
        assert(_token == TK_FUNCTION);
        Lex(); // consume TK_FUNCTION
        SQObject varname = Expect(TK_IDENTIFIER);
        CheckDuplicateLocalIdentifier(varname, _SC("Function"), false);
        Expect(_SC('('));
        CreateFunction(varname, false);
        _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
        _fs->PopTarget();
        _fs->PushLocalVariable(varname, assignable);

    }

    void LocalClassDeclStmt(bool assignable)
    {
        assert(_token == TK_CLASS);
        Lex(); // consume TK_CLASS
        SQObject varname = Expect(TK_IDENTIFIER);
        CheckDuplicateLocalIdentifier(varname, _SC("Class"), false);
        ClassExp();
        _fs->PopTarget();
        _fs->PushLocalVariable(varname, assignable);
    }

    void LocalDeclStatement()
    {
        assert(_token == TK_LET || _token == TK_LOCAL);
        bool assignable = (_token == TK_LOCAL);
        Lex();

        if (_token == TK_FUNCTION) {
            LocalFunctionDeclStmt(assignable);
            return;
        }
        else if (_token == TK_CLASS) {
            LocalClassDeclStmt(assignable);
            return;
        }

        SQInteger destructurer = 0;
        if (_token == _SC('{') || _token == _SC('[')) {
            destructurer = _token;
            Lex();
        }

        SQObject varname;

        sqvector<SQInteger> targets(_ss(_vm)->_alloc_ctx);
        sqvector<SQInteger> flags(_ss(_vm)->_alloc_ctx);
        SQObjectPtrVec names(_ss(_vm)->_alloc_ctx);

        do {
            varname = Expect(TK_IDENTIFIER);
            CheckDuplicateLocalIdentifier(varname, assignable ? _SC("Local variable") : _SC("Named binding"), false);
            if (_token == _SC('=')) {
                Lex(); Expression(SQE_REGULAR);
                SQInteger src = _fs->PopTarget();
                SQInteger dest = _fs->PushTarget();
                if (dest != src) {
                    if (_fs->IsLocal(src)) {
                        _fs->SnoozeOpt();
                    }
                    _fs->AddInstruction(_OP_MOVE, dest, src);
                }
                flags.push_back(OP_GET_FLAG_NO_ERROR | OP_GET_FLAG_KEEP_VAL);
            }
            else {
                if (!assignable && !destructurer)
                    reportDiagnostic(DiagnosticsId::DI_UNINITIALIZED_BINDING, _stringval(varname));
                _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(), 1);
                flags.push_back(0);
            }
            targets.push_back(_fs->PopTarget());
            _fs->PushLocalVariable(varname, assignable);
            names.push_back(varname);

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
        } while (1);

        if (destructurer) {
            Expect(destructurer == _SC('[') ? _SC(']') : _SC('}'));
            Expect(_SC('='));
            Expression(SQE_RVALUE);
            SQInteger src = _fs->TopTarget();
            SQInteger key_pos = _fs->PushTarget();
            if (destructurer == _SC('[')) {
                for (SQUnsignedInteger i = 0; i < targets.size(); ++i) {
                    EmitLoadConstInt(i, key_pos);
                    _fs->AddInstruction(_OP_GET, targets[i], src, key_pos, flags[i]);
                }
            }
            else {
                for (SQUnsignedInteger i = 0; i < targets.size(); ++i) {
                    _fs->AddInstruction(_OP_LOAD, key_pos, _fs->GetConstant(names[i]));
                    _fs->AddInstruction(_OP_GET, targets[i], src, key_pos, flags[i]);
                }
            }
            _fs->PopTarget();
            _fs->PopTarget();
        }
    }
    void IfBlock()
    {
        if (_token == _SC('{'))
        {
            BEGIN_SCOPE();
            Lex();
            Statements();
            Expect(_SC('}'));
            if (true) {
                END_SCOPE();
            }
            else {
                END_SCOPE_NO_CLOSE();
            }
        }
        else {
            BEGIN_SCOPE();
            Statement();
            if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
            END_SCOPE();
        }
    }
    void IfStatement()
    {
        BEGIN_SCOPE();

        SQInteger jmppos;
        bool haselse = false;

        Lex(); Expect(_SC('(')); Expression(SQE_IF); Expect(_SC(')'));
        _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
        SQInteger jnepos = _fs->GetCurrentPos();

        IfBlock();

        SQInteger endifblock = _fs->GetCurrentPos();
        if (_token == TK_ELSE) {
            haselse = true;
            //BEGIN_SCOPE();
            _fs->AddInstruction(_OP_JMP);
            jmppos = _fs->GetCurrentPos();
            Lex();
            //Statement(); if(_lex._prevtoken != _SC('}')) OptionalSemicolon();
            IfBlock();
            //END_SCOPE();
            _fs->SetInstructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
        }
        _fs->SetInstructionParam(jnepos, 1, endifblock - jnepos + (haselse ? 1 : 0));
        END_SCOPE();
    }
    void WhileStatement()
    {
        BEGIN_SCOPE();
        {

            SQInteger jzpos, jmppos;
            jmppos = _fs->GetCurrentPos();
            Lex(); Expect(_SC('(')); Expression(SQE_LOOP_CONDITION); Expect(_SC(')'));

            BEGIN_BREAKBLE_BLOCK();
            _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
            jzpos = _fs->GetCurrentPos();
            BEGIN_SCOPE();

            Statement();

            END_SCOPE();
            _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
            _fs->SetInstructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);

            END_BREAKBLE_BLOCK(jmppos);

        }
        END_SCOPE();
    }
    void DoWhileStatement()
    {
        BEGIN_SCOPE();
        {

            Lex();
            SQInteger jmptrg = _fs->GetCurrentPos();
            BEGIN_BREAKBLE_BLOCK()
                BEGIN_SCOPE();
            Statement();
            END_SCOPE();
            Expect(TK_WHILE);
            SQInteger continuetrg = _fs->GetCurrentPos();
            Expect(_SC('(')); Expression(SQE_LOOP_CONDITION); Expect(_SC(')'));
            _fs->AddInstruction(_OP_JZ, _fs->PopTarget(), 1);
            _fs->AddInstruction(_OP_JMP, 0, jmptrg - _fs->GetCurrentPos() - 1);
            END_BREAKBLE_BLOCK(continuetrg);

        }
        END_SCOPE();
    }
    void ForStatement()
    {
        Lex();
        BEGIN_SCOPE();
        Expect(_SC('('));
        if (_token == TK_LOCAL)
            LocalDeclStatement();
        else if (_token != _SC(';')) {
            CommaExpr(SQE_REGULAR);
            _fs->PopTarget();
        }
        Expect(_SC(';'));
        _fs->SnoozeOpt();
        SQInteger jmppos = _fs->GetCurrentPos();
        SQInteger jzpos = -1;
        if (_token != _SC(';')) { Expression(SQE_LOOP_CONDITION); _fs->AddInstruction(_OP_JZ, _fs->PopTarget()); jzpos = _fs->GetCurrentPos(); }
        Expect(_SC(';'));
        _fs->SnoozeOpt();
        SQInteger expstart = _fs->GetCurrentPos() + 1;
        if (_token != _SC(')')) {
            CommaExpr(SQE_REGULAR);
            _fs->PopTarget();
        }
        Expect(_SC(')'));
        _fs->SnoozeOpt();
        SQInteger expend = _fs->GetCurrentPos();
        SQInteger expsize = (expend - expstart) + 1;
        SQInstructionVec exp(_fs->_sharedstate->_alloc_ctx);
        if (expsize > 0) {
            for (SQInteger i = 0; i < expsize; i++)
                exp.push_back(_fs->GetInstruction(expstart + i));
            _fs->PopInstructions(expsize);
        }
        BEGIN_BREAKBLE_BLOCK()
            Statement();
        SQInteger continuetrg = _fs->GetCurrentPos();
        if (expsize > 0) {
            for (SQInteger i = 0; i < expsize; i++)
                _fs->AddInstruction(exp[i]);
        }
        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1, 0);
        if (jzpos > 0) _fs->SetInstructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);

        END_BREAKBLE_BLOCK(continuetrg);

        END_SCOPE();
    }
    void ForEachStatement()
    {
        SQObject idxname, valname;
        Lex(); Expect(_SC('(')); valname = Expect(TK_IDENTIFIER);
        CheckDuplicateLocalIdentifier(valname, _SC("Iterator"), false);

        if (_token == _SC(',')) {
            idxname = valname;
            Lex(); valname = Expect(TK_IDENTIFIER);
            CheckDuplicateLocalIdentifier(valname, _SC("Iterator"), false);
            if (_string(idxname) == _string(valname))
                reportDiagnostic(DiagnosticsId::DI_SAME_FOREACH_KV_NAMES, _stringval(valname));
        }
        else {
            idxname = _fs->CreateString(_SC("@INDEX@"));
        }
        Expect(TK_IN);

        //save the stack size
        BEGIN_SCOPE();
        //put the table in the stack(evaluate the table expression)
        Expression(SQE_RVALUE); Expect(_SC(')'));
        SQInteger container = _fs->TopTarget();
        //push the index local var
        SQInteger indexpos = _fs->PushLocalVariable(idxname, false);
        _fs->AddInstruction(_OP_LOADNULLS, indexpos, 1);
        //push the value local var
        SQInteger valuepos = _fs->PushLocalVariable(valname, false);
        _fs->AddInstruction(_OP_LOADNULLS, valuepos, 1);
        //push reference index
        SQInteger itrpos = _fs->PushLocalVariable(_fs->CreateString(_SC("@ITERATOR@")), false); //use invalid id to make it inaccessible
        _fs->AddInstruction(_OP_LOADNULLS, itrpos, 1);
        SQInteger jmppos = _fs->GetCurrentPos();
        _fs->AddInstruction(_OP_FOREACH, container, 0, indexpos);
        SQInteger foreachpos = _fs->GetCurrentPos();
        _fs->AddInstruction(_OP_POSTFOREACH, container, 0, indexpos);
        //generate the statement code
        BEGIN_BREAKBLE_BLOCK()
            Statement();
        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
        _fs->SetInstructionParam(foreachpos, 1, _fs->GetCurrentPos() - foreachpos);
        _fs->SetInstructionParam(foreachpos + 1, 1, _fs->GetCurrentPos() - foreachpos);
        END_BREAKBLE_BLOCK(foreachpos - 1);
        //restore the local variable stack(remove index,val and ref idx)
        _fs->PopTarget();
        END_SCOPE();
    }
    void SwitchStatement()
    {
        BEGIN_SCOPE();
        Lex(); Expect(_SC('(')); Expression(SQE_SWITCH); Expect(_SC(')'));
        Expect(_SC('{'));
        SQInteger expr = _fs->TopTarget();
        bool bfirst = true;
        SQInteger tonextcondjmp = -1;
        SQInteger skipcondjmp = -1;
        SQInteger __nbreaks__ = _fs->_unresolvedbreaks.size();
        _fs->_breaktargets.push_back(0);
        _fs->_blockstacksizes.push_back(_scope.stacksize);
        while (_token == TK_CASE) {
            if (!bfirst) {
                _fs->AddInstruction(_OP_JMP, 0, 0);
                skipcondjmp = _fs->GetCurrentPos();
                _fs->SetInstructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
            }
            //condition
            Lex(); Expression(SQE_RVALUE); Expect(_SC(':'));
            SQInteger trg = _fs->PopTarget();
            SQInteger eqtarget = trg;
            bool local = _fs->IsLocal(trg);
            if (local) {
                eqtarget = _fs->PushTarget(); //we need to allocate a extra reg
            }
            _fs->AddInstruction(_OP_EQ, eqtarget, trg, expr);
            _fs->AddInstruction(_OP_JZ, eqtarget, 0);
            if (local) {
                _fs->PopTarget();
            }

            //end condition
            if (skipcondjmp != -1) {
                _fs->SetInstructionParam(skipcondjmp, 1, (_fs->GetCurrentPos() - skipcondjmp));
            }
            tonextcondjmp = _fs->GetCurrentPos();
            BEGIN_SCOPE();
            Statements();
            END_SCOPE();
            bfirst = false;
        }
        if (tonextcondjmp != -1)
            _fs->SetInstructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
        if (_token == TK_DEFAULT) {
            Lex(); Expect(_SC(':'));
            BEGIN_SCOPE();
            Statements();
            END_SCOPE();
        }
        Expect(_SC('}'));
        _fs->PopTarget();
        __nbreaks__ = _fs->_unresolvedbreaks.size() - __nbreaks__;
        if (__nbreaks__ > 0)ResolveBreaks(_fs, __nbreaks__);
        _fs->_breaktargets.pop_back();
        _fs->_blockstacksizes.pop_back();
        END_SCOPE();
    }
    SQObject ExpectScalar()
    {
        SQObject val;
        val._type = OT_NULL; val._unVal.nInteger = 0; //shut up GCC 4.x
        val._flags = 0;
        switch (_token) {
        case TK_INTEGER:
            val._type = OT_INTEGER;
            val._unVal.nInteger = _lex._nvalue;
            break;
        case TK_FLOAT:
            val._type = OT_FLOAT;
            val._unVal.fFloat = _lex._fvalue;
            break;
        case TK_STRING_LITERAL:
            val = _fs->CreateString(_lex._svalue, _lex._longstr.size() - 1);
            break;
        case TK_TRUE:
        case TK_FALSE:
            val._type = OT_BOOL;
            val._unVal.nInteger = _token == TK_TRUE ? 1 : 0;
            break;
        case '-':
            Lex();
            switch (_token)
            {
            case TK_INTEGER:
                val._type = OT_INTEGER;
                val._unVal.nInteger = -_lex._nvalue;
                break;
            case TK_FLOAT:
                val._type = OT_FLOAT;
                val._unVal.fFloat = -_lex._fvalue;
                break;
            default:
                reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float");
            }
            break;
        default:
            reportDiagnostic(DiagnosticsId::DI_SCALAR_EXPECTED, "integer, float, or string");
        }
        Lex();
        return val;
    }

    SQTable* GetScopedConstsTable()
    {
        assert(!_scopedconsts.empty());
        SQObjectPtr &consts = _scopedconsts.top();
        if (sq_type(consts) != OT_TABLE)
            consts = SQTable::Create(_ss(_vm), 0);
        return _table(consts);
    }

    void ConstStatement(bool global)
    {
        Lex();
        SQObject id = Expect(TK_IDENTIFIER);
        bool ignoreGlobalConst = global && !(_fs->lang_features & LF_FORBID_GLOBAL_CONST_REWRITE);
        CheckDuplicateLocalIdentifier(id, _SC("Constant"), ignoreGlobalConst);

        Expect('=');
        SQObject val = ExpectScalar();
        OptionalSemicolon();

        SQTable *enums = global ? _table(_ss(_vm)->_consts) : GetScopedConstsTable();

        SQObjectPtr strongid = id;
        enums->NewSlot(strongid, SQObjectPtr(val));
        strongid.Null();
    }

    void EnumStatement(bool global)
    {
        Lex();
        SQObject id = Expect(TK_IDENTIFIER);
        bool ignoreGlobalConst = global && !(_fs->lang_features & LF_FORBID_GLOBAL_CONST_REWRITE);
        CheckDuplicateLocalIdentifier(id, _SC("Enum"), ignoreGlobalConst);

        Expect(_SC('{'));

        SQObject table = _fs->CreateTable();
        table._flags = SQOBJ_FLAG_IMMUTABLE;
        SQInteger nval = 0;
        while (_token != _SC('}')) {
            SQObject key = Expect(TK_IDENTIFIER);
            SQObject val;
            if (_token == _SC('=')) {
                Lex();
                val = ExpectScalar();
            }
            else {
                val._type = OT_INTEGER;
                val._unVal.nInteger = nval++;
                val._flags = 0;
            }
            _table(table)->NewSlot(SQObjectPtr(key), SQObjectPtr(val));
            if (_token == ',') Lex();
        }

        SQTable *enums = global ? _table(_ss(_vm)->_consts) : GetScopedConstsTable();

        SQObjectPtr strongid = id;
        enums->NewSlot(SQObjectPtr(strongid), SQObjectPtr(table));
        strongid.Null();
        Lex();
    }
    void TryCatchStatement()
    {
        SQObject exid;
        Lex();
        _fs->AddInstruction(_OP_PUSHTRAP, 0, 0);
        _fs->_traps++;
        if (_fs->_breaktargets.size()) _fs->_breaktargets.top()++;
        if (_fs->_continuetargets.size()) _fs->_continuetargets.top()++;
        SQInteger trappos = _fs->GetCurrentPos();
        {
            BEGIN_SCOPE();
            Statement();
            END_SCOPE();
        }
        _fs->_traps--;
        _fs->AddInstruction(_OP_POPTRAP, 1, 0);
        if (_fs->_breaktargets.size()) _fs->_breaktargets.top()--;
        if (_fs->_continuetargets.size()) _fs->_continuetargets.top()--;
        _fs->AddInstruction(_OP_JMP, 0, 0);
        SQInteger jmppos = _fs->GetCurrentPos();
        _fs->SetInstructionParam(trappos, 1, (_fs->GetCurrentPos() - trappos));
        Expect(TK_CATCH); Expect(_SC('(')); exid = Expect(TK_IDENTIFIER); Expect(_SC(')'));
        {
            BEGIN_SCOPE();
            SQInteger ex_target = _fs->PushLocalVariable(exid, false);
            _fs->SetInstructionParam(trappos, 0, ex_target);
            Statement();
            _fs->SetInstructionParams(jmppos, 0, (_fs->GetCurrentPos() - jmppos), 0);
            END_SCOPE();
        }
    }

    SQObject generateSurrogateFunctionName()
    {
        const SQChar * fileName = (sq_type(_sourcename) == OT_STRING) ? _stringval(_sourcename) : _SC("unknown");
        int lineNum = int(_lex._currentline);

        const SQChar * rightSlash = std::max(strrchr(fileName, _SC('/')), strrchr(fileName, _SC('\\')));

        SQChar buf[MAX_FUNCTION_NAME_LEN];
        scsprintf(buf, MAX_FUNCTION_NAME_LEN, _SC("(%s:%d)"), rightSlash ? (rightSlash + 1) : fileName, lineNum);
        return _fs->CreateString(buf);
    }

    void FunctionExp(bool lambda)
    {
        Lex();
        SQObject functionName = (_token == TK_IDENTIFIER) ? Expect(TK_IDENTIFIER) : generateSurrogateFunctionName();
        Expect(_SC('('));

        CreateFunction(functionName, lambda);
        _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1);
    }
    void ClassExp()
    {
        SQInteger base = -1;
        if (_token == TK_EXTENDS) {
            Lex(); Expression(SQE_RVALUE);
            base = _fs->TopTarget();
        }
        Expect(_SC('{'));
        if (base != -1) _fs->PopTarget();
        _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), base, 0, NOT_CLASS);
        ParseTableOrClass(_SC(';'), _SC('}'));
    }
    void DeleteExpr()
    {
        SQExpState es;
        Lex();
        es = _es;
        _es.donot_get = true;
        PrefixedExpr();
        if (_es.etype == EXPR)
            reportDiagnostic(DiagnosticsId::DI_CANNOT_DELETE, "an expression");
        if (_es.etype == BASE)
            reportDiagnostic(DiagnosticsId::DI_CANNOT_DELETE, "'base'");
        if (_es.etype == OBJECT) {
            Emit2ArgsOP(_OP_DELETE);
        }
        else {
            reportDiagnostic(DiagnosticsId::DI_CANNOT_DELETE, "an (outer) local");
        }
        _es = es;
    }
    void PrefixIncDec(SQInteger token)
    {
        SQExpState  es;
        SQInteger diff = (token == TK_MINUSMINUS) ? -1 : 1;
        Lex();
        es = _es;
        _es.donot_get = true;
        PrefixedExpr();
        if (_es.etype == EXPR) {
            reportDiagnostic(DiagnosticsId::DI_CANNOT_INC_DEC, "an expression");
        }
        else if (_es.etype == BASE) {
            reportDiagnostic(DiagnosticsId::DI_BASE_NOT_MODIFIABLE);
        }
        else if (_es.etype == OBJECT) {
            Emit2ArgsOP(_OP_INC, diff);
        }
        else if (_es.isBinding()) {
            reportDiagnostic(DiagnosticsId::DI_CANNOT_INC_DEC, "a binding");
        }
        else if (_es.etype == LOCAL) {
            SQInteger src = _fs->TopTarget();
            _fs->AddInstruction(_OP_INCL, src, src, 0, diff);

        }
        else if (_es.etype == OUTER) {
            SQInteger tmp = _fs->PushTarget();
            _fs->AddInstruction(_OP_GETOUTER, tmp, _es.epos);
            _fs->AddInstruction(_OP_INCL, tmp, tmp, 0, diff);
            _fs->AddInstruction(_OP_SETOUTER, tmp, _es.epos, tmp);
        }
        _es = es;
    }
    void CreateFunction(SQObject &name, bool lambda = false)
    {
        SQFuncState *funcstate = _fs->PushChildState(_ss(_vm));
        funcstate->_name = name;
        SQObject paramname;
        funcstate->AddParameter(_fs->CreateString(_SC("this")));
        funcstate->_sourcename = _sourcename;
        SQInteger defparams = 0;
        while (_token != _SC(')')) {
            if (_token == TK_VARPARAMS) {
                if (defparams > 0)
                    reportDiagnostic(DiagnosticsId::DI_VARARG_WITH_DEFAULT_ARG);
                funcstate->AddParameter(_fs->CreateString(_SC("vargv")));
                funcstate->_varparams = true;
                Lex();
                if (_token != _SC(')'))
                    reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ")");
                break;
            }
            else {
                paramname = Expect(TK_IDENTIFIER);
                funcstate->AddParameter(paramname);
                if (_token == _SC('=')) {
                    Lex();
                    Expression(SQE_RVALUE);
                    funcstate->AddDefaultParam(_fs->TopTarget());
                    defparams++;
                }
                else {
                    if (defparams > 0)
                        reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "=");
                }
                if (_token == _SC(',')) Lex();
                else if (_token != _SC(')'))
                    reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, ") or ,");
            }
        }
        Expect(_SC(')'));
        for (SQInteger n = 0; n < defparams; n++) {
            _fs->PopTarget();
        }

        SQFuncState *currchunk = _fs;
        _fs = funcstate;
        if (lambda) {
            _fs->AddLineInfos(_lex._prevtoken == _SC('\n') ? _lex._lasttokenline : _lex._currentline, _lineinfo, true);
            Expression(SQE_REGULAR);
            _fs->AddInstruction(_OP_RETURN, 1, _fs->PopTarget());
        }
        else {
            if (_token != '{')
                reportDiagnostic(DiagnosticsId::DI_EXPECTED_TOKEN, "{");
            Statement(false);
        }
        funcstate->AddLineInfos(_lex._prevtoken == _SC('\n') ? _lex._lasttokenline : _lex._currentline, _lineinfo, true);
        funcstate->AddInstruction(_OP_RETURN, -1);

        if (!(funcstate->lang_features & LF_DISABLE_OPTIMIZER)) {
            SQOptimizer opt(*funcstate);
            opt.optimize();
        }

        funcstate->SetStackSize(0);

        SQFunctionProto *func = funcstate->BuildProto();

        _fs = currchunk;
        _fs->_functions.push_back(func);
        _fs->PopChildState();
    }
    void ResolveBreaks(SQFuncState *funcstate, SQInteger ntoresolve)
    {
        while (ntoresolve > 0) {
            SQInteger pos = funcstate->_unresolvedbreaks.back();
            funcstate->_unresolvedbreaks.pop_back();
            //set the jmp instruction
            funcstate->SetInstructionParams(pos, 0, funcstate->GetCurrentPos() - pos, 0);
            ntoresolve--;
        }
    }
    void ResolveContinues(SQFuncState *funcstate, SQInteger ntoresolve, SQInteger targetpos)
    {
        while (ntoresolve > 0) {
            SQInteger pos = funcstate->_unresolvedcontinues.back();
            funcstate->_unresolvedcontinues.pop_back();
            //set the jmp instruction
            funcstate->SetInstructionParams(pos, 0, targetpos - pos, 0);
            ntoresolve--;
        }
    }

    bool CanBeDefaultDelegate(const SQObjectPtr &key)
    {
        if (sq_type(key) != OT_STRING)
            return false;

        // this can be optimized by keeping joined list/table of used keys
        SQTable *delegTbls[] = {
            _table(_fs->_sharedstate->_table_default_delegate),
            _table(_fs->_sharedstate->_array_default_delegate),
            _table(_fs->_sharedstate->_string_default_delegate),
            _table(_fs->_sharedstate->_number_default_delegate),
            _table(_fs->_sharedstate->_generator_default_delegate),
            _table(_fs->_sharedstate->_closure_default_delegate),
            _table(_fs->_sharedstate->_thread_default_delegate),
            _table(_fs->_sharedstate->_class_default_delegate),
            _table(_fs->_sharedstate->_instance_default_delegate),
            _table(_fs->_sharedstate->_weakref_default_delegate),
            _table(_fs->_sharedstate->_userdata_default_delegate)
        };
        SQObjectPtr tmp;
        for (SQInteger i = 0; i < sizeof(delegTbls) / sizeof(delegTbls[0]); ++i) {
            if (delegTbls[i]->Get(key, tmp))
                return true;
        }
        return false;
    }

private:
    SQInteger _token;
    SQFuncState *_fs;
    SQObjectPtr _sourcename;
    SQLexer _lex;
    bool _lineinfo;
    SQExpState   _es;
    SQScope _scope;
    SQExpressionContext _expression_context;
    SQVM *_vm;
    SQObjectPtrVec _scopedconsts;
    SQUnsignedInteger _num_initial_bindings;
    SQCompilationContext &_ctx;
    sqvector<sqvector<SQObject>*> _member_constant_keys_check;
};


bool CompileOnePass(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo) {
    SQCompilationContext ctx(vm, NULL, sourcename, sourceText, sourceTextSize, raiseerror);
    SQCompiler p(vm, sourceText, sourceTextSize, bindings, sourcename, ctx, lineinfo);

    if (vm->_on_compile_file)
        vm->_on_compile_file(vm, sourcename);

    return p.Compile(out);
}

RootBlock *ParseToAST(Arena *astArena, SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, bool raiseerror) {
  SQCompilationContext ctx(vm, astArena, sourcename, sourceText, sourceTextSize, raiseerror);
  SQParser p(vm, sourceText, sourceTextSize, sourcename, astArena, ctx);

  RootBlock *r = p.parse();

  if (r) {
    ClosureHoistingOpt opt(_ss(vm), astArena);
    opt.run(r, sourcename);
  }

  return r;
}


bool CompileWithAst(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo)
{
    if (vm->_on_compile_file)
      vm->_on_compile_file(vm, sourcename);

    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    RootBlock *r = ParseToAST(&astArena, vm, sourceText, sourceTextSize, sourcename, raiseerror);

    if (!r)
      return false;

    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, raiseerror);
    CodegenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx, lineinfo);

    return codegen.generate(r, out);
}

bool Compile(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo, bool use_ast) {
#ifdef FORCE_NEW_COMPILER
    use_ast = true;
#endif // FORCE_NEW_COMPILER


    return use_ast
        ? CompileWithAst(vm, sourceText, sourceTextSize, bindings, sourcename, out, raiseerror, lineinfo)
        : CompileOnePass(vm, sourceText, sourceTextSize, bindings, sourcename, out, raiseerror, lineinfo);
}

bool TranslateASTToBytecode(SQVM *vm, SqAstNode *ast, const HSQOBJECT *bindings, const SQChar *sourcename, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, bool raiseerror, bool lineinfo)
{
    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, raiseerror);
    CodegenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx, lineinfo);

    assert(ast->op() == TO_BLOCK && ast->asStatement()->asBlock()->isRoot());

    return codegen.generate((RootBlock *)ast, out);
}

void AnalyseCode(SQVM *vm, SqAstNode *ast, const HSQOBJECT *bindings, const SQChar *sourcename, const char *sourceText, size_t sourceTextSize)
{
    Arena saArena(_ss(vm)->_alloc_ctx, "Analyser");
    SQCompilationContext ctx(vm, &saArena, sourcename, sourceText, sourceTextSize, true);

    StaticAnalyser sa(ctx);

    assert(ast->op() == TO_BLOCK && ast->asStatement()->asBlock()->isRoot());

    sa.runAnalysis((RootBlock *)ast);
}

bool TranslateBinaryASTToBytecode(SQVM *vm, const uint8_t *buffer, size_t size, const HSQOBJECT *bindings, SQObjectPtr &out, bool raiseerror, bool lineinfo) {
    assert(_ss(vm)->checkCompilationOption(CompilationOptions::CO_USE_AST_COMPILER));
    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    MemoryInputStream mis(buffer, size);
    SQASTReader reader(_ss(vm)->_alloc_ctx, vm, &astArena, &mis, raiseerror);
    const SQChar *sourcename = NULL;
    RootBlock *r = reader.readAst(sourcename);

    if (!r) {
      return false;
    }

    return TranslateASTToBytecode(vm, r, bindings, sourcename, NULL, 0, out, raiseerror, lineinfo);
}

bool ParseAndSaveBinaryAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, OutputStream *ostream, bool raiseerror) {
    assert(_ss(vm)->checkCompilationOption(CompilationOptions::CO_USE_AST_COMPILER));

    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    RootBlock *r = ParseToAST(&astArena, vm, sourceText, sourceTextSize, sourcename, raiseerror);

    if (!r) {
        return false;
    }

    SQASTWriter writer(_ss(vm)->_alloc_ctx, ostream);
    writer.writeAST(r, sourcename);

    return true;
}

#endif
