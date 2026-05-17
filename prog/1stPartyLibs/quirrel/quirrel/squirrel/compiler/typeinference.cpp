#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "opcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqfuncstate.h"
#include "codegen.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqclosure.h"
#include "sqtypeparser.h"


namespace SQCompilation {


Expr *CodeGenVisitor::deparen(Expr *e) const {
    if (e->op() == TO_PAREN)
        return deparen(((UnExpr *)e)->argument());
    else
        return e;
}


unsigned CodeGenVisitor::inferExprTypeMask(Expr *expr) {
    if (expr->isTypeInferred())
        return expr->getTypeMask();

    unsigned mask = inferExprTypeMaskImpl(expr);
    expr->setTypeMask(mask);
    expr->setTypeInferred();
    return mask;
}


unsigned CodeGenVisitor::inferExprTypeMaskImpl(Expr *expr) {
    switch (expr->op()) {
    case TO_LITERAL: {
        LiteralExpr *lit = expr->asLiteral();
        switch (lit->kind()) {
        case LK_INT:    return _RT_INTEGER;
        case LK_FLOAT:  return _RT_FLOAT;
        case LK_STRING: return _RT_STRING;
        case LK_BOOL:   return _RT_BOOL;
        case LK_NULL:   return _RT_NULL;
        }
        return ~0u;
    }

    case TO_TYPEOF:
        return _RT_STRING;

    case TO_NEG: {
        unsigned argMask = inferExprTypeMask(static_cast<UnExpr *>(expr)->argument());
        if (argMask == ~0u) return ~0u;
        if (argMask & _RT_INSTANCE) return ~0u; // _unm metamethod
        return argMask & (_RT_INTEGER | _RT_FLOAT);
    }

    case TO_INC: {
        unsigned argMask = inferExprTypeMask(static_cast<IncExpr *>(expr)->argument());
        if (argMask == ~0u) return ~0u;
        if (argMask & _RT_INSTANCE) return ~0u; // metamethods
        return argMask & (_RT_INTEGER | _RT_FLOAT);
    }

    // Comparisons -> bool
    case TO_EQ:
    case TO_NE:
    case TO_GT:
    case TO_GE:
    case TO_LT:
    case TO_LE:
    case TO_INSTANCEOF:
    case TO_IN:
    case TO_NOT:
        return _RT_BOOL;

    case TO_3CMP:
    // Bitwise -> integer
    case TO_AND: case TO_OR: case TO_XOR:
    case TO_SHL: case TO_SHR: case TO_USHR:
    case TO_BNOT:
        return _RT_INTEGER;

    // Arithmetic
    case TO_MUL: case TO_DIV: case TO_SUB: case TO_MOD: {
        BinExpr *bin = expr->asBinExpr();
        unsigned lm = inferExprTypeMask(bin->lhs());
        unsigned rm = inferExprTypeMask(bin->rhs());
        if (lm == ~0u || rm == ~0u) return ~0u;
        if ((lm | rm) & _RT_INSTANCE) return ~0u; // _mul/_div/_sub/_modulo metamethods
        if ((lm & ~(_RT_INTEGER | _RT_FLOAT)) || (rm & ~(_RT_INTEGER | _RT_FLOAT)))
            return ~0u; // non-numeric operands
        if (lm == _RT_INTEGER && rm == _RT_INTEGER)
            return _RT_INTEGER;
        if ((lm & (_RT_INTEGER | _RT_FLOAT)) && (rm & (_RT_INTEGER | _RT_FLOAT))) {
            if ((lm & _RT_FLOAT) || (rm & _RT_FLOAT))
                return _RT_FLOAT;
            return _RT_INTEGER | _RT_FLOAT;
        }
        return _RT_INTEGER | _RT_FLOAT;
    }

    case TO_ADD: {
        BinExpr *bin = expr->asBinExpr();
        unsigned lm = inferExprTypeMask(bin->lhs());
        unsigned rm = inferExprTypeMask(bin->rhs());
        if (lm == ~0u || rm == ~0u) return ~0u;
        if ((lm | rm) & _RT_INSTANCE) return ~0u; // _add metamethod
        // String concat: if either side is definitely string, result is string
        if (lm == _RT_STRING || rm == _RT_STRING)
            return _RT_STRING;
        if ((lm & ~(_RT_INTEGER | _RT_FLOAT)) || (rm & ~(_RT_INTEGER | _RT_FLOAT)))
            return ~0u; // non-numeric/non-string operands
        if (lm == _RT_INTEGER && rm == _RT_INTEGER)
            return _RT_INTEGER;
        if ((lm & _RT_FLOAT) || (rm & _RT_FLOAT))
            return _RT_FLOAT;
        return _RT_INTEGER | _RT_FLOAT;
    }


    // Logical: short-circuit, result is one of the operands
    case TO_OROR: case TO_ANDAND: {
        BinExpr *bin = expr->asBinExpr();
        unsigned lm = inferExprTypeMask(bin->lhs());
        unsigned rm = inferExprTypeMask(bin->rhs());
        if (lm == ~0u || rm == ~0u) return ~0u;
        return lm | rm;
    }

    // Null-coalesce: lhs if non-null, else rhs
    case TO_NULLC: {
        BinExpr *bin = expr->asBinExpr();
        unsigned lm = inferExprTypeMask(bin->lhs());
        unsigned rm = inferExprTypeMask(bin->rhs());
        if (lm == ~0u || rm == ~0u) return ~0u;
        return (lm & ~_RT_NULL) | rm;
    }

    // Ternary
    case TO_TERNARY: {
        TerExpr *ter = static_cast<TerExpr *>(expr);
        unsigned bm = inferExprTypeMask(ter->b());
        unsigned cm = inferExprTypeMask(ter->c());
        if (bm == ~0u || cm == ~0u) return ~0u;
        // Null-check narrowing: strip _RT_NULL from the appropriate branch
        // to avoid false positives with patterns like `x != null ? x : default`
        Expr *cond = deparen(ter->a());
        if (cond->op() == TO_NE) {
            BinExpr *bin = cond->asBinExpr();
            if ((bin->lhs()->op() == TO_LITERAL && bin->lhs()->asLiteral()->kind() == LK_NULL) ||
                (bin->rhs()->op() == TO_LITERAL && bin->rhs()->asLiteral()->kind() == LK_NULL))
                bm &= ~_RT_NULL;
        } else if (cond->op() == TO_EQ) {
            BinExpr *bin = cond->asBinExpr();
            if ((bin->lhs()->op() == TO_LITERAL && bin->lhs()->asLiteral()->kind() == LK_NULL) ||
                (bin->rhs()->op() == TO_LITERAL && bin->rhs()->asLiteral()->kind() == LK_NULL))
                cm &= ~_RT_NULL;
        }
        return bm | cm;
    }

    // Identifiers: look up declared type
    case TO_ID: {
        Id *id = expr->asId();
        SQObjectPtr nameObj(_fs->CreateString(id->name()));
        SQCompiletimeVarInfo varInfo;
        if (_fs->GetLocalVariable(nameObj, varInfo) != -1 ||
            _fs->GetOuterVariable(nameObj, varInfo) != -1) {
            // If the variable was initialized with a known-type expression, use that
            if (varInfo.initializer && (varInfo.var_flags & (VF_ASSIGNABLE | VF_PARAM)) == 0) {
                unsigned initMask = inferExprTypeMask(varInfo.initializer);
                if (initMask != ~0u) {
                    return initMask;
                }
            }
            // For functions initialized with pure calls, check the initializer
            if (varInfo.initializer && (varInfo.var_flags & VF_INIT_WITH_PURE)) {
                if (varInfo.initializer->op() == TO_CALL) {
                    CallExpr *call = varInfo.initializer->asCallExpr();
                    Expr *callee = call->callee();
                    if (callee->op() == TO_ID) {
                        SQObjectPtr calleeName(_fs->CreateString(callee->asId()->name()));
                        SQCompiletimeVarInfo calleeInfo;
                        if ((_fs->GetLocalVariable(calleeName, calleeInfo) != -1 ||
                             _fs->GetOuterVariable(calleeName, calleeInfo) != -1) &&
                            calleeInfo.initializer && calleeInfo.initializer->op() == TO_FUNCTION) {
                            return calleeInfo.initializer->asFunctionExpr()->getResultTypeMask();
                        }
                    }
                }
            }
            return varInfo.type_mask;
        }
        // Check named constants (const s = "err")
        SQObjectPtr constant;
        if (IsConstant(nameObj, constant)) {
            SQObjectType ctype = sq_type(constant);
            switch (ctype) {
            case OT_INTEGER:       return _RT_INTEGER;
            case OT_FLOAT:         return _RT_FLOAT;
            case OT_STRING:        return _RT_STRING;
            case OT_BOOL:          return _RT_BOOL;
            case OT_NULL:          return _RT_NULL;
            case OT_ARRAY:         return _RT_ARRAY;
            case OT_TABLE:         return _RT_TABLE;
            case OT_CLOSURE:       return _RT_CLOSURE;
            case OT_NATIVECLOSURE: return _RT_NATIVECLOSURE;
            case OT_CLASS:         return _RT_CLASS;
            case OT_INSTANCE:      return _RT_INSTANCE;
            default:               return ~0u;
            }
        }
        return ~0u;
    }

    // Function call: try to determine return type
    case TO_CALL: {
        CallExpr *call = expr->asCallExpr();
        Expr *callee = call->callee();

        // freeze(x) returns the same type as x
        if (isFreezeCall(expr))
            return inferExprTypeMask(call->arguments()[0]);

        ResolvedCallee info = resolveCallee(callee);
        // For an async function, the call value is the Promise wrapper (an
        // instance), not the resolved type. The resolved type is what
        // `await call(...)` produces; see TO_AWAIT below.
        unsigned retMask = info.known ? (info.isAsync ? _RT_INSTANCE : info.resultMask) : ~0u;

        if (call->isNullable() && retMask != ~0u)
            retMask |= _RT_NULL;
        return retMask;
    }

    // Container/type literals
    case TO_ARRAY:
        return _RT_ARRAY;

    case TO_CLASS:
        return _RT_CLASS;

    case TO_FUNCTION:
        return _RT_CLOSURE | _RT_NATIVECLOSURE;

    case TO_TABLE:
    case TO_ROOT_TABLE_ACCESS:
        return _RT_TABLE;


    case TO_PAREN:
    case TO_CLONE:
    // Static memo wraps an expression
    case TO_STATIC_MEMO:
    // Inline const wraps a literal
    case TO_INLINE_CONST:
        return inferExprTypeMask(static_cast<UnExpr *>(expr)->argument());

    // Comma expression returns last value
    case TO_COMMA: {
        CommaExpr *comma = static_cast<CommaExpr *>(expr);
        const auto &exprs = comma->expressions();
        if (!exprs.empty())
            return inferExprTypeMask(exprs.back());
        return ~0u;
    }


    // Only await-on-direct-async-call is handled; flowing through a variable
    // (`let p = f(); await p`) would need Promise<T> generics, which Quirrel lacks.
    case TO_AWAIT: {
        Expr *arg = deparen(static_cast<UnExpr *>(expr)->argument());
        if (arg->op() == TO_CALL) {
            ResolvedCallee info = resolveCallee(arg->asCallExpr()->callee());
            if (info.known && info.isAsync)
                return info.resultMask;
        }
        return ~0u;
    }

    case TO_DELETE:
    case TO_RESUME:
    // Compound assign are complex; fall through to runtime
    case TO_PLUSEQ:
    case TO_MINUSEQ:
    case TO_MULEQ:
    case TO_DIVEQ:
    case TO_MODEQ:
    // Unknown at compile time
    case TO_GETFIELD:
    case TO_GETSLOT:
    case TO_SETFIELD:
    case TO_SETSLOT:
    case TO_BASE:
    case TO_ASSIGN: case TO_NEWSLOT:
    case TO_CODE_BLOCK_EXPR:
    default:
        return ~0u;
    }
}


// Statically resolve a callee expression to (isAsync, isNative, resultMask).
// Returns {known=false} when the callee shape isn't statically resolvable
// (anything other than TO_ID -> known initializer/constant, or TO_FUNCTION).
// Note: class-method calls (`obj.method()`) intentionally stay opaque.
// Adding tracking would require receiver-class inference and member walks;
// skipped for now for simplicity.
CodeGenVisitor::ResolvedCallee CodeGenVisitor::resolveCallee(Expr *callee) {
    ResolvedCallee r{false, false, false, ~0u};

    if (callee->op() == TO_ID) {
        SQObjectPtr calleeName(_fs->CreateString(callee->asId()->name()));
        SQCompiletimeVarInfo calleeInfo;
        if ((_fs->GetLocalVariable(calleeName, calleeInfo) != -1 ||
             _fs->GetOuterVariable(calleeName, calleeInfo) != -1) &&
            calleeInfo.initializer) {
            if (calleeInfo.initializer->op() == TO_FUNCTION) {
                FunctionExpr *fn = calleeInfo.initializer->asFunctionExpr();
                r.known = true;
                r.isAsync = fn->isAsync();
                r.resultMask = fn->getResultTypeMask();
                return r;
            }
            if (calleeInfo.initializer->op() == TO_CLASS) {
                r.known = true;
                r.resultMask = _RT_INSTANCE;
                return r;
            }
        }
        // Named constants: const closure (compiled async possible) or native closure.
        SQObjectPtr constant;
        if (IsConstant(calleeName, constant)) {
            if (sq_type(constant) == OT_CLOSURE) {
                SQClosure *cls = _closure(constant);
                if (cls->_function) {
                    r.known = true;
                    r.isAsync = cls->_function->_isAsync;
                    r.resultMask = cls->_function->_result_type_mask;
                    return r;
                }
            } else if (sq_type(constant) == OT_NATIVECLOSURE) {
                SQNativeClosure *nc = _nativeclosure(constant);
                r.known = true;
                r.isNative = true;
                r.resultMask = nc->_result_type_mask;
                return r;
            }
        }
        return r;
    }

    if (callee->op() == TO_FUNCTION) {
        FunctionExpr *fn = callee->asFunctionExpr();
        r.known = true;
        r.isAsync = fn->isAsync();
        r.resultMask = fn->getResultTypeMask();
        return r;
    }

    return r;
}


bool CodeGenVisitor::checkInferredType(Node *reportNode, Expr *expr, unsigned declaredMask) {
#if SQ_RUNTIME_TYPE_CHECK
    if (declaredMask == ~0u)
        return false;

    // Non-Promise lvalue assigned an async-function call: that's a forgotten `await`.
    // Skip when the lvalue accepts 'instance' -- user intentionally captured the Promise.
    // deparen so `let x: int = (asyncFn())` is also caught.
    Expr *call = deparen(expr);
    if ((declaredMask & _RT_INSTANCE) == 0 && call->op() == TO_CALL) {
        Expr *callee = call->asCallExpr()->callee();
        ResolvedCallee info = resolveCallee(callee);
        if (info.known && info.isAsync) {
            const char *fnName = (callee->op() == TO_ID) ? callee->asId()->name() : "the function";
            char declaredBuf[160];
            sq_stringify_type_mask(declaredBuf, sizeof(declaredBuf), declaredMask);
            _ctx.throwError(reportNode,
                "call to async function returns a Promise, not '%s'; did you mean 'await %s(...)'?",
                declaredBuf, fnName);
            return false; // unreachable
        }
    }

    unsigned inferredMask = inferExprTypeMask(expr);
    if (inferredMask != ~0u && (inferredMask & ~declaredMask) != 0) {
        char inferredBuf[160], declaredBuf[160];
        sq_stringify_type_mask(inferredBuf, sizeof(inferredBuf), inferredMask);
        sq_stringify_type_mask(declaredBuf, sizeof(declaredBuf), declaredMask);
        _ctx.throwError(reportNode, "expression of type '%s' cannot be assigned to type '%s'",
                   inferredBuf, declaredBuf);
        return false; // unreachable
    }
    return inferredMask != ~0u;
#else
    (void)reportNode;
    (void)expr;
    (void)declaredMask;
    return false;
#endif
}


} // namespace SQCompilation

#endif // NO_COMPILER
