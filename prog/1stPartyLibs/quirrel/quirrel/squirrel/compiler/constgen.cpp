#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <stdarg.h>
#include "opcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqfuncstate.h"
#include "codegen.h"
#include "constgen.h"
#include "sqvm.h"
#include "optimizer.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqclass.h"

namespace SQCompilation {

void ConstGenVisitor::throwUnsupported(Node *n, const char *type)
{
    _ctx.reportDiagnostic(DiagnosticsId::DI_NOT_ALLOWED_IN_CONST, n->lineStart(), n->columnStart(), n->textWidth(), type);
}

void ConstGenVisitor::throwGeneralError(Node *n, const char *msg)
{
    _ctx.reportDiagnostic(DiagnosticsId::DI_GENERAL_COMPILE_ERROR, n->lineStart(), n->columnStart(), n->textWidth(), msg);
}

void ConstGenVisitor::throwGeneralErrorFmt(Node *n, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    _ctx.reportDiagnostic(DiagnosticsId::DI_GENERAL_COMPILE_ERROR, n->lineStart(), n->columnStart(), n->textWidth(), buf);
}

void ConstGenVisitor::process(Expr *expr, SQObjectPtr &out)
{
    assert(sq_type(_result) == OT_NULL);
    expr->visit(this);
    out = _result;
}

SQObjectPtr ConstGenVisitor::convertLiteral(LiteralExpr *lit)
{
    SQObjectPtr ret{};
    switch (lit->kind()) {
    case LK_STRING: return _fs->CreateString(lit->s());
    case LK_FLOAT: ret._type = OT_FLOAT; ret._unVal.fFloat = lit->f(); break;
    case LK_INT:  ret._type = OT_INTEGER; ret._unVal.nInteger = lit->i(); break;
    case LK_BOOL: ret._type = OT_BOOL; ret._unVal.nInteger = lit->b() ? 1 : 0; break;
    case LK_NULL: ret._type = OT_NULL; ret._unVal.raw = 0; break;
    }
    return ret;
}

void ConstGenVisitor::visitLiteralExpr(LiteralExpr *expr)
{
    _result = convertLiteral(expr);
    _call_target.Null();
}

void ConstGenVisitor::visitArrayExpr(ArrayExpr *expr)
{
    SQArray *arr = SQArray::Create(_ss(_vm), expr->initializers().size());

    for (SQUnsignedInteger i = 0; i < expr->initializers().size(); ++i) {
        Expr *valExpr = expr->initializers()[i];
        valExpr->visit(this);
        arr->Set(i, _result);
    }

    _result = SQObjectPtr(arr);
    _result._flags = SQOBJ_FLAG_IMMUTABLE;

    _call_target.Null();
}

void ConstGenVisitor::visitTableExpr(TableExpr *tblExpr)
{
    SQTable *table = SQTable::Create(_ss(_vm), tblExpr->members().size());

    const auto &members = tblExpr->members();
    for (SQUnsignedInteger i = 0; i < members.size(); ++i) {
        const TableMember &m = members[i];

        m.key->visit(this);
        SQObjectPtr key(_result);

        m.value->visit(this);
        SQObjectPtr value(_result);

        table->NewSlot(key, value);
    }

    _result = SQObjectPtr(table);
    _result._flags = SQOBJ_FLAG_IMMUTABLE;

    _call_target.Null();
}


void ConstGenVisitor::visitId(Id *id)
{
    SQObjectPtr idObj = _fs->CreateString(id->name());
    SQObjectPtr constant;
    if (_codegen.IsConstant(idObj, constant)) {
        _result = constant;
    } else {
        _ctx.reportDiagnostic(DiagnosticsId::DI_ID_IS_NOT_CONST,
            id->lineStart(), id->columnStart(), id->textWidth(),
            _stringval(idObj));
    }

    _call_target.Null();
}


void ConstGenVisitor::visitCallExpr(CallExpr *expr)
{
    if (expr->isNullable()) {
        throwGeneralError(expr, "Nullable calls are not supported in constant expressions");
        return;
    }

    ConstGenVisitor funcEval(_vm, _fs, _ctx, _codegen);

    expr->callee()->visit(&funcEval);
    SQObjectPtr func = funcEval._result;

    if (!sq_is_pure_function(&func))
        throwGeneralError(expr, "Only calls to pure functions are allowed in constant expressions");

    sqvector<SQObjectPtr> argValues(nullptr);
    argValues.reserve(expr->arguments().size());

    auto &argExprs = expr->arguments();

    for (SQUnsignedInteger i = 0; i < argExprs.size(); ++i) {
        ConstGenVisitor argEval(_vm, _fs, _ctx, _codegen);
        argExprs[i]->visit(&argEval);
        argValues.push_back(argEval._result);
    }

    SQInteger prevTop = sq_gettop(_vm);

    sq_pushobject(_vm, func);
    sq_pushobject(_vm, funcEval._call_target);
    for (SQUnsignedInteger i = 0; i < argValues.size(); ++i) {
        sq_pushobject(_vm, argValues[i]);
    }
    if (SQ_FAILED(sq_call(_vm, 1 + argValues.size(), SQTrue, SQTrue))) {
        sq_settop(_vm, prevTop);
        throwGeneralError(expr, "Failed to evaluate constant initializer call");
    }

    HSQOBJECT callRes;
    sq_getstackobj(_vm, -1, &callRes);
    callRes._flags = SQOBJ_FLAG_IMMUTABLE;
    _result = SQObjectPtr(callRes);

    _call_target.Null();

    sq_settop(_vm, prevTop);
}


void ConstGenVisitor::visitGetFieldExpr(GetFieldExpr *expr)
{
    expr->visitChildren(this);
    SQObjectPtr container(_result);
    _call_target = container;

    SQObjectPtr slotName = _fs->CreateString(expr->fieldName());

    SQInteger prevTop = sq_gettop(_vm);
    sq_pushobject(_vm, container);
    sq_pushobject(_vm, slotName);
    bool hasSlot = SQ_SUCCEEDED(sq_get(_vm, -2));
    if (hasSlot) {
        HSQOBJECT value;
        sq_getstackobj(_vm, -1, &value);
        value._flags = SQOBJ_FLAG_IMMUTABLE;
        _result = SQObjectPtr(value);
    }
    else if (expr->isNullable()) {
        if (!sq_isnull(_vm->_lasterror)) {
            SQObjectPtr err = _vm->_lasterror;
            sq_reseterror(_vm);
            sq_settop(_vm, prevTop);
            throwGeneralErrorFmt(expr, "error in get operation: %s",
                sq_isstring(err) ? _stringval(err) : "<unknown>");
        }
        _result.Null();
    }
    else {
        sq_settop(_vm, prevTop);
        _ctx.reportDiagnostic(DiagnosticsId::DI_CONSTANT_FIELD_NOT_FOUND,
            expr->lineStart(), expr->columnStart(), expr->textWidth(),
            _stringval(slotName));
    }

    sq_settop(_vm, prevTop);
}


void ConstGenVisitor::visitGetSlotExpr(GetSlotExpr *expr)
{
    expr->receiver()->visit(this);
    SQObjectPtr container(_result);
    _call_target = container;

    expr->key()->visit(this);
    SQObjectPtr key(_result);

    SQInteger prevTop = sq_gettop(_vm);
    sq_pushobject(_vm, container);
    sq_pushobject(_vm, key);
    bool hasSlot = SQ_SUCCEEDED(sq_get(_vm, -2));
    if (hasSlot) {
        HSQOBJECT value;
        sq_getstackobj(_vm, -1, &value);
        value._flags = SQOBJ_FLAG_IMMUTABLE;
        _result = SQObjectPtr(value);
    }
    else if (expr->isNullable()) {
        if (!sq_isnull(_vm->_lasterror)) {
            SQObjectPtr err = _vm->_lasterror;
            sq_reseterror(_vm);
            sq_settop(_vm, prevTop);
            throwGeneralErrorFmt(expr, "error in get operation: %s",
                sq_isstring(err) ? _stringval(err) : "<unknown>");
        }
        _result.Null();
    }
    else {
        SQObjectPtr keyAsString;
        sq_pushobject(_vm, key);
        if (SQ_SUCCEEDED(sq_tostring(_vm, -1))) {
            SQObject t;
            sq_getstackobj(_vm, -1, &t);
            keyAsString = t;
        } else {
            keyAsString = _fs->CreateString("<???>", 5);
        }

        sq_settop(_vm, prevTop);

        Expr *errNode = expr->key();
        _ctx.reportDiagnostic(DiagnosticsId::DI_CONSTANT_SLOT_NOT_FOUND,
            errNode->lineStart(), errNode->columnStart(), errNode->textWidth(),
            IdType2Name(sq_type(key)), _stringval(keyAsString));
    }
    sq_settop(_vm, prevTop);
}


void ConstGenVisitor::visitUnExpr(UnExpr *unary)
{
    sq_reseterror(_vm);
    _call_target.Null();

    switch (unary->op())
    {
    case TO_NEG:
        unary->argument()->visit(this);
        if (!_vm->NEG_OP(_result, _result)) {
            SQObjectPtr err = _vm->_lasterror;
            sq_reseterror(_vm);
            throwGeneralError(unary->argument(),
                sq_isstring(err) ? _stringval(err) : "negation failed");
        }
        break;
    case TO_NOT:
        unary->argument()->visit(this);
        _result = SQObjectPtr(_vm->IsFalse(_result));
        break;
    case TO_BNOT:
        unary->argument()->visit(this);
        if(sq_type(_result) != OT_INTEGER)
            throwGeneralError(unary->argument(), "attempt to perform a bitwise op on a non-integer");

        _result = SQObjectPtr(~(_integer(_result)));
        break;
    case TO_TYPEOF:
        unary->argument()->visit(this);
        if (!_vm->TypeOf(_result, _result))
            throwGeneralError(unary->argument(), "typeof call failed");
        break;
    case TO_INLINE_CONST:
    case TO_PAREN:
        // bypass
        unary->argument()->visit(this);
        break;
    default:
        throwUnsupported(unary, "this unary expression");
    }
}

void ConstGenVisitor::visitBinExpr(BinExpr *expr)
{
    sq_reseterror(_vm);
    _call_target.Null();

    switch (expr->op()) {
        case TO_NEWSLOT:
            throwUnsupported(expr, "new slot expression");
        case TO_ASSIGN:
            throwUnsupported(expr, "assignment expression");
        case TO_PLUSEQ:
        case TO_MINUSEQ:
        case TO_MULEQ:
        case TO_DIVEQ:
        case TO_MODEQ:
            throwUnsupported(expr, "compound arithmetic expression");
        default:
            break;
    }

    expr->lhs()->visit(this);
    SQObjectPtr lhs(_result);

    expr->rhs()->visit(this);
    SQObjectPtr rhs(_result);

    assert(sq_isnull(_vm->_lasterror) && "Error thrown while evaluating binary expression operands");

    bool ok = true;

    switch (expr->op()) {
        case TO_NULLC:
            _result = sq_isnull(lhs) ? rhs : lhs;
            break;
        case TO_OROR:
            _result = SQVM::IsFalse(lhs) ? rhs : lhs;
            break;
        case TO_ANDAND:
            _result = SQVM::IsFalse(lhs) ? lhs : rhs;
            break;
        case TO_ADD:
            ok = _vm->ARITH_OP('+', _result, lhs, rhs);
            break;
        case TO_SUB:
            ok = _vm->ARITH_OP('-', _result, lhs, rhs);
            break;
        case TO_MUL:
            ok = _vm->ARITH_OP('*', _result, lhs, rhs);
            break;
        case TO_DIV:
            ok = _vm->ARITH_OP('/', _result, lhs, rhs);
            break;
        case TO_MOD:
            ok = _vm->ARITH_OP('%', _result, lhs, rhs);
            break;
        case TO_OR:
            ok = _vm->BW_OP(BW_OR, _result, lhs, rhs);
            break;
        case TO_AND:
            ok = _vm->BW_OP(BW_AND, _result, lhs, rhs);
            break;
        case TO_XOR:
            ok = _vm->BW_OP(BW_XOR, _result, lhs, rhs);
            break;
        case TO_USHR:
            ok = _vm->BW_OP(BW_USHIFTR, _result, lhs, rhs);
            break;
        case TO_SHR:
            ok = _vm->BW_OP(BW_SHIFTR, _result, lhs, rhs);
            break;
        case TO_SHL:
            ok = _vm->BW_OP(BW_SHIFTL, _result, lhs, rhs);
            break;

        case TO_EQ:
            _result = SQObjectPtr(_vm->IsEqual(lhs, rhs));
            break;
        case TO_NE:
            _result = SQObjectPtr(!_vm->IsEqual(lhs, rhs));
            break;

        case TO_GE:
            ok = _vm->CMP_OP(CMP_GE, lhs, rhs, _result);
            break;
        case TO_GT:
            ok = _vm->CMP_OP(CMP_G, lhs, rhs, _result);
            break;
        case TO_LE:
            ok = _vm->CMP_OP(CMP_LE, lhs, rhs, _result);
            break;
        case TO_LT:
            ok = _vm->CMP_OP(CMP_L, lhs, rhs, _result);
            break;
        case TO_3CMP:
            ok = _vm->CMP_OP(CMP_3W, lhs, rhs, _result);
            break;

        case TO_IN: {
            SQObjectPtr tmpVal;
            bool exists = _vm->Get(rhs, lhs, tmpVal, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_NO_TYPE_METHODS);
            _result = SQObjectPtr(exists);
            if (!sq_isnull(_vm->_lasterror)) { // handle errors that can happen in _get() metamethod
                ok = false;
                _vm->Raise_Error("Error while applying 'in' operator: %s",
                    sq_isstring(_vm->_lasterror) ? _stringval(_vm->_lasterror) : "<unknown>");
            }
            break;
        }
        case TO_INSTANCEOF: {
            if (sq_type(rhs) != OT_CLASS)
                throwGeneralError(expr->rhs(), "checking instance with non-class");
            _result = SQObjectPtr(SQVM::IsInstanceOf(lhs, _class(rhs)));
            break;
        }
        default:
            assert(!"Unknown binary expression");
            break;
    }

    assert(ok == sq_isnull(_vm->_lasterror));
    if (!ok) {
        SQObjectPtr err = _vm->_lasterror;
        sq_reseterror(_vm);
        throwGeneralError(expr, sq_isstring(err) ? _stringval(err) : "internal error in binary operation");
    }
}


void ConstGenVisitor::visitTerExpr(TerExpr *expr)
{
    _call_target.Null();

    expr->a()->visit(this);
    Expr *resBranch = SQVM::IsFalse(_result) ? expr->c() : expr->b();
    resBranch->visit(this);
}


void ConstGenVisitor::visitFunctionExpr(FunctionExpr *funcExpr)
{
    _call_target.Null();
    _result = _codegen.compileConstFunc(funcExpr);
}

}

#endif
