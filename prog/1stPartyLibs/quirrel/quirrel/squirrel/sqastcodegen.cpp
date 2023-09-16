#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqopcodes.h"
#include "sqcompiler.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqfuncstate.h"
#include "sqastcodegen.h"
#include "sqvm.h"
#include "sqoptimizer.h"
#include "sqtable.h"


namespace SQCompilation {

static bool isObject(const Expr *expr) {
    if (expr->isConst()) return false;
    if (expr->isAccessExpr()) return expr->asAccessExpr()->receiver()->op() != TO_BASE;
    if (expr->op() == TO_ID && expr->asId()->isField()) return true;
    return expr->op() == TO_ROOT;
}

static bool isOuter(const Expr *expr) {
    if (expr->op() != TO_ID) return false;
    return expr->asId()->isOuter();
}


CodegenVisitor::CodegenVisitor(Arena *arena, const HSQOBJECT *bindings, SQVM *vm, const SQChar *sourceName, SQCompilationContext &ctx, bool lineinfo)
    : Visitor(),
    _fs(NULL),
    _childFs(NULL),
    _ctx(ctx),
    _scopedconsts(_ss(vm)->_alloc_ctx),
    _sourceName(sourceName),
    _vm(vm),
    _donot_get(false),
    _lineinfo(lineinfo),
    _arena(arena),
    _scope() {

    _num_initial_bindings = 0;

    if (bindings) {
        assert(sq_type(*bindings) == OT_TABLE || sq_type(*bindings) == OT_NULL);
        if (sq_type(*bindings) == OT_TABLE) {
            _scopedconsts.push_back(*bindings);
            _num_initial_bindings = 1;
        }
    }
}

void CodegenVisitor::reportDiagnostic(Node *n, int32_t id, ...) {
    va_list vargs;
    va_start(vargs, id);

    _ctx.vreportDiagnostic((enum DiagnosticsId)id, n->lineStart(), n->columnStart(), n->columnEnd() - n->columnStart(), vargs);

    va_end(vargs);
}

bool CodegenVisitor::generate(RootBlock *root, SQObjectPtr &out) {
    SQFuncState funcstate(_ss(_vm), NULL, _ctx);

    if (setjmp(_ctx.errorJump()) == 0) {

        _fs = &funcstate;
        _childFs = NULL;

        _fs->_name = SQString::Create(_ss(_vm), _SC("__main__"));
        _fs->AddParameter(_fs->CreateString(_SC("this")));
        _fs->AddParameter(_fs->CreateString(_SC("vargv")));
        _fs->_varparams = true;
        _fs->_sourcename = SQString::Create(_ss(_vm), _sourceName);

        SQInteger stacksize = _fs->GetStackSize();

        root->visit(this);

        _fs->SetStackSize(stacksize);
        _fs->AddLineInfos(root->lineEnd(), _lineinfo, true);
        _fs->AddInstruction(_OP_RETURN, 0xFF);

        if (!(_fs->lang_features & LF_DISABLE_OPTIMIZER)) {
            SQOptimizer opt(funcstate);
            opt.optimize();
        }

        _fs->SetStackSize(0);

        out = _fs->BuildProto();

        _fs = NULL;
        return true;
    } else {
        return false;
    }
}

void CodegenVisitor::CheckDuplicateLocalIdentifier(Node *n, SQObject name, const SQChar *desc, bool ignore_global_consts) {
    bool assignable = false;
    if (_fs->GetLocalVariable(name, assignable) >= 0)
        reportDiagnostic(n, DiagnosticsId::DI_CONFLICTS_WITH, desc, _string(name)->_val, "existing local variable");
    if (_string(name) == _string(_fs->_name))
        reportDiagnostic(n, DiagnosticsId::DI_CONFLICTS_WITH, desc, _stringval(name), "function name");

    SQObject constant;
    if (ignore_global_consts ? IsLocalConstant(name, constant) : IsConstant(name, constant))
        reportDiagnostic(n, DiagnosticsId::DI_CONFLICTS_WITH, desc, _stringval(name), "existing constant/enum/import");
}

static bool compareLiterals(LiteralExpr *a, LiteralExpr *b) {
    if (a->kind() != b->kind()) return false;
    if (a->kind() == LK_STRING) {
        return strcmp(a->s(), b->s()) == 0;
    }

    return a->raw() == b->raw();
}

#ifdef _SQ64
# define SQ_UINT_FMT PRIu64
#else
# define SQ_UINT_FMT PRIuPTR
#endif // _SQ64

bool CodegenVisitor::CheckMemberUniqueness(ArenaVector<Expr *> &vec, Expr *obj) {

    if (obj->op() != TO_LITERAL && obj->op() != TO_ID) return true;

    for (SQUnsignedInteger i = 0, n = vec.size(); i < n; ++i) {
        Expr *vecobj = vec[i];
        if (vecobj->op() == TO_ID && obj->op() == TO_ID) {
            if (strcmp(vecobj->asId()->id(), obj->asId()->id()) == 0) {
                reportDiagnostic(obj, DiagnosticsId::DI_DUPLICATE_KEY, obj->asId()->id());
                return false;
            }
            continue;
        }
        if (vecobj->op() == TO_LITERAL && obj->op() == TO_LITERAL) {
            LiteralExpr *a = (LiteralExpr*)vecobj;
            LiteralExpr *b = (LiteralExpr*)obj;
            if (compareLiterals(a, b)) {
                if (a->kind() == LK_STRING) {
                    reportDiagnostic(obj, DiagnosticsId::DI_DUPLICATE_KEY, a->s());
                }
                else {
                    char b[32] = { 0 };
                    snprintf(b, sizeof b, "%" SQ_UINT_FMT, a->raw());
                    reportDiagnostic(obj, DiagnosticsId::DI_DUPLICATE_KEY, b);
                }
                return false;
            }
            continue;
        }
    }

    vec.push_back(obj);
    return true;
}

void CodegenVisitor::Emit2ArgsOP(SQOpcode op, SQInteger p3)
{
    SQInteger p2 = _fs->PopTarget(); //src in OP_GET
    SQInteger p1 = _fs->PopTarget(); //key in OP_GET
    _fs->AddInstruction(op, _fs->PushTarget(), p1, p2, p3);
}

void CodegenVisitor::EmitLoadConstInt(SQInteger value, SQInteger target)
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

void CodegenVisitor::EmitLoadConstFloat(SQFloat value, SQInteger target)
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

void CodegenVisitor::ResolveBreaks(SQFuncState *funcstate, SQInteger ntoresolve)
{
    while (ntoresolve > 0) {
        SQInteger pos = funcstate->_unresolvedbreaks.back();
        funcstate->_unresolvedbreaks.pop_back();
        //set the jmp instruction
        funcstate->SetInstructionParams(pos, 0, funcstate->GetCurrentPos() - pos, 0);
        ntoresolve--;
    }
}


void CodegenVisitor::ResolveContinues(SQFuncState *funcstate, SQInteger ntoresolve, SQInteger targetpos)
{
    while (ntoresolve > 0) {
        SQInteger pos = funcstate->_unresolvedcontinues.back();
        funcstate->_unresolvedcontinues.pop_back();
        //set the jmp instruction
        funcstate->SetInstructionParams(pos, 0, targetpos - pos, 0);
        ntoresolve--;
    }
}


void CodegenVisitor::EmitDerefOp(SQOpcode op)
{
    SQInteger val = _fs->PopTarget();
    SQInteger key = _fs->PopTarget();
    SQInteger src = _fs->PopTarget();
    _fs->AddInstruction(op, _fs->PushTarget(), src, key, val);
}


void CodegenVisitor::visitBlock(Block *block) {
    addLineNumber(block);

    BEGIN_SCOPE();

    ArenaVector<Statement *> &statements = block->statements();

    for (auto stmt : statements) {
        stmt->visit(this);
        _fs->SnoozeOpt();
    }

    if (block->isBody() || block->isRoot()) {
        END_SCOPE_NO_CLOSE();
    }
    else {
        END_SCOPE();
    }
}


void CodegenVisitor::visitIfStatement(IfStatement *ifStmt) {
    addLineNumber(ifStmt);
    BEGIN_SCOPE();

    visitForceGet(ifStmt->condition());

    _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
    SQInteger jnepos = _fs->GetCurrentPos();

    ifStmt->thenBranch()->visit(this);

    SQInteger endifblock = _fs->GetCurrentPos();

    if (ifStmt->elseBranch()) {
        _fs->AddInstruction(_OP_JMP);
        SQInteger jmppos = _fs->GetCurrentPos();
        ifStmt->elseBranch()->visit(this);
        _fs->SetInstructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
    }

    _fs->SetInstructionParam(jnepos, 1, endifblock - jnepos + (ifStmt->elseBranch() ? 1 : 0));
    END_SCOPE();
}

void CodegenVisitor::visitWhileStatement(WhileStatement *whileLoop) {
    addLineNumber(whileLoop);

    BEGIN_SCOPE();
    {
        SQInteger jmppos = _fs->GetCurrentPos();

        visitForceGet(whileLoop->condition());

        BEGIN_BREAKBLE_BLOCK();

        _fs->AddInstruction(_OP_JZ, _fs->PopTarget());

        SQInteger jzpos = _fs->GetCurrentPos();

        BEGIN_SCOPE();

        whileLoop->body()->visit(this);

        END_SCOPE();

        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
        _fs->SetInstructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);

        END_BREAKBLE_BLOCK(jmppos);
    }
    END_SCOPE();
}

void CodegenVisitor::visitDoWhileStatement(DoWhileStatement *doWhileLoop) {
    addLineNumber(doWhileLoop);

    BEGIN_SCOPE();
    {
        SQInteger jmptrg = _fs->GetCurrentPos();
        BEGIN_BREAKBLE_BLOCK();

        BEGIN_SCOPE();
        doWhileLoop->body()->visit(this);
        END_SCOPE();

        SQInteger continuetrg = _fs->GetCurrentPos();
        visitForceGet(doWhileLoop->condition());

        _fs->AddInstruction(_OP_JZ, _fs->PopTarget(), 1);
        _fs->AddInstruction(_OP_JMP, 0, jmptrg - _fs->GetCurrentPos() - 1);
        END_BREAKBLE_BLOCK(continuetrg);

    }
    END_SCOPE();
}

void CodegenVisitor::visitForStatement(ForStatement *forLoop) {
    addLineNumber(forLoop);

    BEGIN_SCOPE();

    if (forLoop->initializer()) {
        Node *init = forLoop->initializer();
        visitForceGet(init);
        if (init->isExpression()) {
            _fs->PopTarget();
        }
    }

    _fs->SnoozeOpt();
    SQInteger jmppos = _fs->GetCurrentPos();
    SQInteger jzpos = -1;

    if (forLoop->condition()) {
        visitForceGet(forLoop->condition());
        _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
        jzpos = _fs->GetCurrentPos();
    }

    _fs->SnoozeOpt();

    SQInteger expstart = _fs->GetCurrentPos() + 1;

    if (forLoop->modifier()) {
        visitForceGet(forLoop->modifier());
        _fs->PopTarget();
    }

    _fs->SnoozeOpt();

    SQInteger expend = _fs->GetCurrentPos();
    SQInteger expsize = (expend - expstart) + 1;
    ArenaVector<SQInstruction> exp(_arena);

    if (expsize > 0) {
        for (SQInteger i = 0; i < expsize; i++)
            exp.push_back(_fs->GetInstruction(expstart + i));
        _fs->PopInstructions(expsize);
    }

    BEGIN_BREAKBLE_BLOCK();
    forLoop->body()->visit(this);
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

void CodegenVisitor::visitForeachStatement(ForeachStatement *foreachLoop) {
    addLineNumber(foreachLoop);

    BEGIN_SCOPE();

    visitForceGet(foreachLoop->container());

    SQInteger container = _fs->TopTarget();

    SQObject idxName;
    SQInteger indexpos = -1;
    if (foreachLoop->idx()) {
        foreachLoop->idx()->visit(this);
        indexpos = _fs->_vlocals.back()._pos;
    }
    else {
        idxName = _fs->CreateString(_SC("@INDEX@"));
        indexpos = _fs->PushLocalVariable(idxName, false);
    }
    _fs->AddInstruction(_OP_LOADNULLS, indexpos, 1);

    foreachLoop->val()->visit(this);
    SQInteger valuepos = _fs->_vlocals.back()._pos;
    _fs->AddInstruction(_OP_LOADNULLS, valuepos, 1);

    //push reference index
    SQInteger itrpos = _fs->PushLocalVariable(_fs->CreateString(_SC("@ITERATOR@")), false); //use invalid id to make it inaccessible
    _fs->AddInstruction(_OP_LOADNULLS, itrpos, 1);
    SQInteger jmppos = _fs->GetCurrentPos();
    _fs->AddInstruction(_OP_FOREACH, container, 0, indexpos);
    SQInteger foreachpos = _fs->GetCurrentPos();
    _fs->AddInstruction(_OP_POSTFOREACH, container, 0, indexpos);

    BEGIN_BREAKBLE_BLOCK();
    foreachLoop->body()->visit(this);
    _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
    _fs->SetInstructionParam(foreachpos, 1, _fs->GetCurrentPos() - foreachpos);
    _fs->SetInstructionParam(foreachpos + 1, 1, _fs->GetCurrentPos() - foreachpos);
    END_BREAKBLE_BLOCK(foreachpos - 1);
    //restore the local variable stack(remove index,val and ref idx)
    _fs->PopTarget();
    END_SCOPE();
}

void CodegenVisitor::visitSwitchStatement(SwitchStatement *swtch) {
    addLineNumber(swtch);
    BEGIN_SCOPE();

    visitForceGet(swtch->expression());

    SQInteger expr = _fs->TopTarget();
    SQInteger tonextcondjmp = -1;
    SQInteger skipcondjmp = -1;
    SQInteger __nbreaks__ = _fs->_unresolvedbreaks.size();

    _fs->_breaktargets.push_back(0);
    _fs->_blockstacksizes.push_back(_scope.stacksize);
    ArenaVector<SwitchCase> &cases = swtch->cases();

    for (SQUnsignedInteger i = 0; i < cases.size(); ++i) {
        if (i) {
            _fs->AddInstruction(_OP_JMP, 0, 0);
            skipcondjmp = _fs->GetCurrentPos();
            _fs->SetInstructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
        }

        const SwitchCase &c = cases[i];

        visitForceGet(c.val);

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
        c.stmt->visit(this);
        END_SCOPE();
    }

    if (tonextcondjmp != -1)
        _fs->SetInstructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);

    const SwitchCase &d = swtch->defaultCase();

    if (d.stmt) {
        BEGIN_SCOPE();
        d.stmt->visit(this);
        END_SCOPE();
    }

    _fs->PopTarget();
    __nbreaks__ = _fs->_unresolvedbreaks.size() - __nbreaks__;
    if (__nbreaks__ > 0) ResolveBreaks(_fs, __nbreaks__);
    _fs->_breaktargets.pop_back();
    _fs->_blockstacksizes.pop_back();
    END_SCOPE();
}

void CodegenVisitor::visitTryStatement(TryStatement *tryStmt) {
    addLineNumber(tryStmt);
    _fs->AddInstruction(_OP_PUSHTRAP, 0, 0);
    _fs->_traps++;

    if (_fs->_breaktargets.size()) _fs->_breaktargets.top()++;
    if (_fs->_continuetargets.size()) _fs->_continuetargets.top()++;

    SQInteger trappos = _fs->GetCurrentPos();
    {
        BEGIN_SCOPE();
        tryStmt->tryStatement()->visit(this);
        END_SCOPE();
    }

    _fs->_traps--;
    _fs->AddInstruction(_OP_POPTRAP, 1, 0);
    if (_fs->_breaktargets.size()) _fs->_breaktargets.top()--;
    if (_fs->_continuetargets.size()) _fs->_continuetargets.top()--;
    _fs->AddInstruction(_OP_JMP, 0, 0);
    SQInteger jmppos = _fs->GetCurrentPos();
    _fs->SetInstructionParam(trappos, 1, (_fs->GetCurrentPos() - trappos));

    {
        BEGIN_SCOPE();
        SQInteger ex_target = _fs->PushLocalVariable(_fs->CreateString(tryStmt->exceptionId()->id()), false);
        _fs->SetInstructionParam(trappos, 0, ex_target);
        tryStmt->catchStatement()->visit(this);
        _fs->SetInstructionParams(jmppos, 0, (_fs->GetCurrentPos() - jmppos), 0);
        END_SCOPE();
    }
}

void CodegenVisitor::visitBreakStatement(BreakStatement *breakStmt) {
    addLineNumber(breakStmt);
    if (_fs->_breaktargets.size() <= 0)
        reportDiagnostic(breakStmt, DiagnosticsId::DI_LOOP_CONTROLLER_NOT_IN_LOOP, "break");
    if (_fs->_breaktargets.top() > 0) {
        _fs->AddInstruction(_OP_POPTRAP, _fs->_breaktargets.top(), 0);
    }
    RESOLVE_OUTERS();
    _fs->AddInstruction(_OP_JMP, 0, -1234);
    _fs->_unresolvedbreaks.push_back(_fs->GetCurrentPos());
}

void CodegenVisitor::visitContinueStatement(ContinueStatement *continueStmt) {
    addLineNumber(continueStmt);
    if (_fs->_continuetargets.size() <= 0)
        reportDiagnostic(continueStmt, DiagnosticsId::DI_LOOP_CONTROLLER_NOT_IN_LOOP, "continue");
    if (_fs->_continuetargets.top() > 0) {
        _fs->AddInstruction(_OP_POPTRAP, _fs->_continuetargets.top(), 0);
    }
    RESOLVE_OUTERS();
    _fs->AddInstruction(_OP_JMP, 0, -1234);
    _fs->_unresolvedcontinues.push_back(_fs->GetCurrentPos());
}

void CodegenVisitor::visitTerminateStatement(TerminateStatement *terminator) {
    addLineNumber(terminator);

    if (terminator->argument()) {
        visitForceGet(terminator->argument());
    }
}

void CodegenVisitor::visitReturnStatement(ReturnStatement *retStmt) {
    SQInteger retexp = _fs->GetCurrentPos() + 1;
    visitTerminateStatement(retStmt);

    if (_fs->_traps > 0) {
        _fs->AddInstruction(_OP_POPTRAP, _fs->_traps, 0);
    }

    if (retStmt->argument()) {
        _fs->_returnexp = retexp;
        _fs->AddInstruction(_OP_RETURN, 1, _fs->PopTarget(), _fs->GetStackSize());
    }
    else {
        _fs->_returnexp = -1;
        _fs->AddInstruction(_OP_RETURN, 0xFF, 0, _fs->GetStackSize());
    }
}

void CodegenVisitor::visitYieldStatement(YieldStatement *yieldStmt) {
    SQInteger retexp = _fs->GetCurrentPos() + 1;
    _fs->_bgenerator = true;
    visitTerminateStatement(yieldStmt);

    if (yieldStmt->argument()) {
        _fs->_returnexp = retexp;
        _fs->AddInstruction(_OP_YIELD, 1, _fs->PopTarget(), _fs->GetStackSize());
    }
    else {
        _fs->_returnexp = -1;
        _fs->AddInstruction(_OP_YIELD, 0xFF, 0, _fs->GetStackSize());
    }
}

void CodegenVisitor::visitThrowStatement(ThrowStatement *throwStmt) {
    visitTerminateStatement(throwStmt);
    _fs->AddInstruction(_OP_THROW, _fs->PopTarget());
}

void CodegenVisitor::visitExprStatement(ExprStatement *stmt) {
    addLineNumber(stmt);
    visitForceGet(stmt->expression());
    _fs->DiscardTarget();
}

void CodegenVisitor::generateTableDecl(TableDecl *tableDecl) {
    bool isKlass = tableDecl->op() == TO_CLASS;
    const auto members = tableDecl->members();

    ArenaVector<Expr *> memberConstantKeys(_arena);

    for (SQUnsignedInteger i = 0; i < members.size(); ++i) {
        const TableMember &m = members[i];
#if SQ_LINE_INFO_IN_STRUCTURES
        if (i < 100 && m.key->lineStart() != -1) {
            _fs->AddLineInfos(m.key->lineStart(), false);
        }
#endif
        CheckMemberUniqueness(memberConstantKeys, m.key);

        visitForceGet(m.key);
        visitForceGet(m.value);

        SQInteger val = _fs->PopTarget();
        SQInteger key = _fs->PopTarget();
        SQInteger table = _fs->TopTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE

        if (isKlass) {
            _fs->AddInstruction(_OP_NEWSLOTA, m.isStatic() ? NEW_SLOT_STATIC_FLAG : 0, table, key, val);
        }
        else {
            _fs->AddInstruction(_OP_NEWSLOT, 0xFF, table, key, val);
        }
    }
}

void CodegenVisitor::visitTableDecl(TableDecl *tableDecl) {
    addLineNumber(tableDecl);
    _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), tableDecl->members().size(), 0, NOT_TABLE);
    generateTableDecl(tableDecl);
}

void CodegenVisitor::checkClassKey(Expr *key) {
    switch (key->op())
    {
    case TO_GETFIELD:
    case TO_GETTABLE:
    case TO_ROOT:
        return;
    case TO_BASE:
    case TO_ID:
        reportDiagnostic(key, DiagnosticsId::DI_LOCAL_CLASS_SYNTAX);
        break;
    default:
        reportDiagnostic(key, DiagnosticsId::DI_INVALID_CLASS_NAME);
        break;
    }
}

void CodegenVisitor::visitClassDecl(ClassDecl *klass) {
    addLineNumber(klass);
    if (klass->context() == DC_SLOT) {
        assert(klass->classKey());
        checkClassKey(klass->classKey());
        visitNoGet(klass->classKey());
    }

    Expr *baseExpr = klass->classBase();
    SQInteger baseIdx = -1;
    if (baseExpr) {
        visitForceGet(baseExpr);
        baseIdx = _fs->PopTarget();
    }

    _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), baseIdx, 0, NOT_CLASS);

    generateTableDecl(klass);

    if (klass->context() == DC_SLOT) {
        EmitDerefOp(_OP_NEWSLOT);
        _fs->PopTarget();
    }
}

static const SQChar *varDescriptor(VarDecl *var) {
    Expr *init = var->initializer();
    if (init == NULL) {
        return var->isAssignable() ? _SC("Local variable") : _SC("Named binding");
    }
    else {
        if (init->op() == TO_DECL_EXPR) {
            Decl *decl = static_cast<DeclExpr *>(init)->declaration();
            switch (decl->op())
            {
            case TO_FUNCTION: return _SC("Function");
            case TO_CLASS: return _SC("Class");
            case TO_TABLE:
                return _SC("Named binding"); // <== is that correct?
            default:
                assert(0 && "Unexpected declaration kind");
                break;
            }
        }
        else {
            return var->isAssignable() ? _SC("Local variable") : _SC("Named binding");
        }
    }
    assert(0);
    return "<error>";
}

void CodegenVisitor::visitVarDecl(VarDecl *var) {
    addLineNumber(var);
    const SQChar *name = var->name();

    SQObject varName = _fs->CreateString(name);

    CheckDuplicateLocalIdentifier(var, varName, varDescriptor(var), false);

    if (var->initializer()) {
        visitForceGet(var->initializer());
        SQInteger src = _fs->PopTarget();
        SQInteger dest = _fs->PushTarget();
        if (dest != src) {
            if (_fs->IsLocal(src)) {
                _fs->SnoozeOpt();
            }
            _fs->AddInstruction(_OP_MOVE, dest, src);
        }
    }
    else {
        _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(), 1);
    }

    _last_pop = _fs->PopTarget();
    _fs->PushLocalVariable(varName, var->isAssignable());
}

void CodegenVisitor::visitDeclGroup(DeclGroup *group) {
    addLineNumber(group);
    const auto declarations = group->declarations();

    for (Decl *d : declarations) {
        d->visit(this);
    }
}

void CodegenVisitor::visitDestructuringDecl(DestructuringDecl *destruct) {
    addLineNumber(destruct);
    ArenaVector<SQInteger> targets(_arena);

    const auto declarations = destruct->declarations();

    for (auto d : declarations) {
        d->visit(this);
        assert(_last_pop != -1);
        targets.push_back(_last_pop);
        _last_pop = -1;
    }

    visitForceGet(destruct->initiExpression());

    SQInteger src = _fs->TopTarget();
    SQInteger key_pos = _fs->PushTarget();

    for (SQUnsignedInteger i = 0; i < declarations.size(); ++i) {
        VarDecl *d = declarations[i];
        SQInteger flags = d->initializer() ? OP_GET_FLAG_NO_ERROR | OP_GET_FLAG_KEEP_VAL : 0;
        if (destruct->type() == DT_ARRAY) {
            EmitLoadConstInt(i, key_pos);
            _fs->AddInstruction(_OP_GET, targets[i], src, key_pos, flags);
        }
        else {
            _fs->AddInstruction(_OP_LOAD, key_pos, _fs->GetConstant(_fs->CreateString(d->name())));
            _fs->AddInstruction(_OP_GET, targets[i], src, key_pos, flags);
        }
    }

    _fs->PopTarget();
    _fs->PopTarget();
}

void CodegenVisitor::visitParamDecl(ParamDecl *param) {
    _childFs->AddParameter(_fs->CreateString(param->name()));
    if (param->hasDefaultValue()) {
        visitForceGet(param->defaultValue());
    }
}

void CodegenVisitor::visitFunctionDecl(FunctionDecl *funcDecl) {
    addLineNumber(funcDecl);

    SQFuncState *savedChildFsAtRoot = _childFs; // may be non-null when processing default argument that is declared as function/lambda
    _childFs = _fs->PushChildState(_ss(_vm));

    _childFs->_name = _fs->CreateString(funcDecl->name());
    _childFs->_sourcename = _fs->_sourcename = SQString::Create(_ss(_vm), funcDecl->sourceName());
    _childFs->_varparams = funcDecl->isVararg();

    SQInteger defparams = 0;

    for (auto param : funcDecl->parameters()) {
        param->visit(this);
        if (param->hasDefaultValue()) {
            _childFs->AddDefaultParam(_fs->TopTarget());
            ++defparams;
        }
    }

    for (SQInteger n = 0; n < defparams; n++) {
        _fs->PopTarget();
    }

    Block *body = funcDecl->body();
    SQInteger startLine = body->lineStart();
    if (startLine != -1) {
        _childFs->AddLineInfos(startLine, _lineinfo, false);
    }

    {
        assert(_childFs->_parent == _fs);
        SQFuncState *savedChildFsAtBody = _childFs;
        _fs = _childFs;
        _childFs = NULL;

        funcDecl->body()->visit(this);

        _childFs = savedChildFsAtBody;
        _fs = _childFs->_parent;
    }

    _childFs->AddLineInfos(body->lineEnd(), _lineinfo, true);
    _childFs->AddInstruction(_OP_RETURN, -1);

    if (!(_childFs->lang_features & LF_DISABLE_OPTIMIZER)) {
        SQOptimizer opt(*_childFs);
        opt.optimize();
    }

    _childFs->SetStackSize(0);
    _childFs->_hoistLevel = funcDecl->hoistingLevel();
    SQFunctionProto *funcProto = _childFs->BuildProto();

    _fs->_functions.push_back(funcProto);

    _fs->PopChildState();
    _childFs = savedChildFsAtRoot;

    _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, funcDecl->isLambda() ? 1 : 0);
}

SQTable* CodegenVisitor::GetScopedConstsTable()
{
    assert(!_scopedconsts.empty());
    SQObjectPtr &consts = _scopedconsts.top();
    if (sq_type(consts) != OT_TABLE)
        consts = SQTable::Create(_ss(_vm), 0);
    return _table(consts);
}

SQObject CodegenVisitor::selectLiteral(LiteralExpr *lit) {
    SQObject ret;
    switch (lit->kind()) {
    case LK_STRING: return _fs->CreateString(lit->s());
    case LK_FLOAT: ret._type = OT_FLOAT; ret._unVal.fFloat = lit->f(); break;
    case LK_INT:  ret._type = OT_INTEGER; ret._unVal.nInteger = lit->i(); break;
    case LK_BOOL: ret._type = OT_BOOL; ret._unVal.nInteger = lit->b() ? 1 : 0; break;
    case LK_NULL: ret._type = OT_NULL; ret._unVal.raw = 0; break;
    }
    return ret;
}

void CodegenVisitor::visitConstDecl(ConstDecl *decl) {
    addLineNumber(decl);

    SQObject id = _fs->CreateString(decl->name());
    SQObject value = selectLiteral(decl->value());

    CheckDuplicateLocalIdentifier(decl, id, _SC("Constant"), decl->isGlobal() && !(_fs->lang_features & LF_FORBID_GLOBAL_CONST_REWRITE));

    SQTable *enums = decl->isGlobal() ? _table(_ss(_vm)->_consts) : GetScopedConstsTable();
    enums->NewSlot(SQObjectPtr(id), SQObjectPtr(value));
}

void CodegenVisitor::visitEnumDecl(EnumDecl *enums) {
    addLineNumber(enums);
    SQObject table = _fs->CreateTable();
    table._flags = SQOBJ_FLAG_IMMUTABLE;
    SQInteger nval = 0;

    SQObject id = _fs->CreateString(enums->name());

    CheckDuplicateLocalIdentifier(enums, id, _SC("Enum"), enums->isGlobal() && !(_fs->lang_features & LF_FORBID_GLOBAL_CONST_REWRITE));

    for (auto &c : enums->consts()) {
        SQObject key = _fs->CreateString(c.id);
        _table(table)->NewSlot(SQObjectPtr(key), SQObjectPtr(selectLiteral(c.val)));
    }

    SQTable *enumsTable = enums->isGlobal() ? _table(_ss(_vm)->_consts) : GetScopedConstsTable();
    enumsTable->NewSlot(SQObjectPtr(id), SQObjectPtr(table));
}

void CodegenVisitor::MoveIfCurrentTargetIsLocal() {
    SQInteger trg = _fs->TopTarget();
    if (_fs->IsLocal(trg)) {
        trg = _fs->PopTarget(); //pops the target and moves it
        _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), trg);
    }
}

void CodegenVisitor::maybeAddInExprLine(Expr *expr) {
    if (!_ss(_vm)->_lineInfoInExpressions) return;

    if (expr->lineStart() != -1) {
        _fs->AddLineInfos(expr->lineStart(), _lineinfo, false);
    }
}

void CodegenVisitor::addLineNumber(Statement *stmt) {
    SQInteger line = stmt->lineStart();
    if (line != -1) {
        _fs->AddLineInfos(line, _lineinfo, false);
    }
}

Expr *CodegenVisitor::deparen(Expr *e) const {
  if (e->op() == TO_PAREN) {
    return deparen(((UnExpr *)e)->argument());
  }
  return e;
}


void CodegenVisitor::visitCallExpr(CallExpr *call) {

    maybeAddInExprLine(call);

    Expr *callee = deparen(call->callee());
    bool isNullCall = call->isNullable();

    visitNoGet(callee);

    if (isObject(callee)) {
        if (!isNullCall) {
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
    }
    else if (isOuter(callee)) {
        _fs->AddInstruction(_OP_GETOUTER, _fs->PushTarget(), callee->asId()->outerPos());
        _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
    }
    else {
        _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
    }

    const auto &args = call->arguments();

    for (auto arg : args) {
        arg->visit(this);
        MoveIfCurrentTargetIsLocal();
    }

    for (SQUnsignedInteger i = 0; i < args.size(); ++i) {
        _fs->PopTarget();
    }

    SQInteger stackbase = _fs->PopTarget();
    SQInteger closure = _fs->PopTarget();
    SQInteger target = _fs->PushTarget();
    assert(target >= -1);
    assert(target < 255);
    _fs->AddInstruction(isNullCall ? _OP_NULLCALL : _OP_CALL, target, closure, stackbase, args.size() + 1);
}

void CodegenVisitor::visitBaseExpr(BaseExpr *base) {
    maybeAddInExprLine(base);
    _fs->AddInstruction(_OP_GETBASE, _fs->PushTarget());
    base->setPos(_fs->TopTarget());
}

void CodegenVisitor::visitRootExpr(RootExpr *expr) {
    maybeAddInExprLine(expr);
    _fs->AddInstruction(_OP_LOADROOT, _fs->PushTarget());
}

void CodegenVisitor::visitLiteralExpr(LiteralExpr *lit) {
    maybeAddInExprLine(lit);
    switch (lit->kind()) {
    case LK_STRING:
        _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_fs->CreateString(lit->s())));
        break;
    case LK_FLOAT:EmitLoadConstFloat(lit->f(), -1); break;
    case LK_INT:  EmitLoadConstInt(lit->i(), -1); break;
    case LK_BOOL: _fs->AddInstruction(_OP_LOADBOOL, _fs->PushTarget(), lit->b()); break;
    case LK_NULL: _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(), 1); break;
    }
}

void CodegenVisitor::visitArrayExpr(ArrayExpr *expr) {
    maybeAddInExprLine(expr);
    const auto inits = expr->initialziers();

    _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), inits.size(), 0, NOT_ARRAY);

    for (SQUnsignedInteger i = 0; i < inits.size(); ++i) {
        Expr *valExpr = inits[i];
#if SQ_LINE_INFO_IN_STRUCTURES
        if (i < 100 && valExpr->lineStart() != -1)
            _fs->AddLineInfos(valExpr->lineStart(), false);
#endif
        visitForceGet(valExpr);
        SQInteger val = _fs->PopTarget();
        SQInteger array = _fs->TopTarget();
        _fs->AddInstruction(_OP_APPENDARRAY, array, val, AAT_STACK);
    }
}

void CodegenVisitor::emitUnaryOp(SQOpcode op, UnExpr *u) {
    Expr *arg = u->argument();
    visitForceGet(arg);

    if (_fs->_targetstack.size() == 0)
        reportDiagnostic(u, DiagnosticsId::DI_CANNOT_EVAL_UNARY);

    SQInteger src = _fs->PopTarget();
    _fs->AddInstruction(op, _fs->PushTarget(), src);
}

void CodegenVisitor::emitDelete(UnExpr *ud) {
    Expr *argument = ud->argument();
    visitNoGet(argument);

    switch (argument->op())
    {
    case TO_GETFIELD:
    case TO_GETTABLE: break;
    case TO_BASE:
        reportDiagnostic(ud, DiagnosticsId::DI_CANNOT_DELETE, "'base'");
        break;
    case TO_ID:
        reportDiagnostic(ud, DiagnosticsId::DI_CANNOT_DELETE, "an (outer) local");
        break;
    default:
        reportDiagnostic(ud, DiagnosticsId::DI_CANNOT_DELETE, "an expression");
        break;
    }

    SQInteger table = _fs->PopTarget(); //src in OP_GET
    SQInteger key = _fs->PopTarget(); //key in OP_GET
    _fs->AddInstruction(_OP_DELETE, _fs->PushTarget(), key, table);
}

void CodegenVisitor::visitUnExpr(UnExpr *unary) {
    maybeAddInExprLine(unary);

    switch (unary->op())
    {
    case TO_NEG: emitUnaryOp(_OP_NEG, unary); break;
    case TO_NOT: emitUnaryOp(_OP_NOT, unary); break;
    case TO_BNOT:emitUnaryOp(_OP_BWNOT, unary); break;
    case TO_TYPEOF: emitUnaryOp(_OP_TYPEOF, unary); break;
    case TO_RESUME: emitUnaryOp(_OP_RESUME, unary); break;
    case TO_CLONE: emitUnaryOp(_OP_CLONE, unary); break;
    case TO_PAREN: unary->argument()->visit(this); break;
    case TO_DELETE: emitDelete(unary);
        break;
    default:
        break;
    }
}

void CodegenVisitor::emitSimpleBin(SQOpcode op, Expr *lhs, Expr *rhs, SQInteger op3) {
    visitForceGet(lhs);
    visitForceGet(rhs);
    SQInteger op1 = _fs->PopTarget();
    SQInteger op2 = _fs->PopTarget();
    _fs->AddInstruction(op, _fs->PushTarget(), op1, op2, op3);
}

void CodegenVisitor::emitJpmArith(SQOpcode op, Expr *lhs, Expr *rhs) {
    visitForceGet(lhs);

    SQInteger first_exp = _fs->PopTarget();
    SQInteger trg = _fs->PushTarget();
    _fs->AddInstruction(op, trg, 0, first_exp, 0);
    SQInteger jpos = _fs->GetCurrentPos();
    if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
    visitForceGet(rhs);
    _fs->SnoozeOpt();
    SQInteger second_exp = _fs->PopTarget();
    if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
    _fs->SnoozeOpt();
    _fs->SetInstructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
}

void CodegenVisitor::emitCompoundArith(SQOpcode op, SQInteger opcode, Expr *lvalue, Expr *rvalue) {

    visitNoGet(lvalue);

    visitForceGet(rvalue);

    if (lvalue->op() == TO_ID) {
        Id *id = lvalue->asId();

        if (!id->isAssignable()) {
            reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_BINDING, id->id());
        }

        if (id->isOuter()) {
            SQInteger val = _fs->TopTarget();
            SQInteger tmp = _fs->PushTarget();
            _fs->AddInstruction(_OP_GETOUTER, tmp, lvalue->asId()->outerPos());
            _fs->AddInstruction(op, tmp, val, tmp, 0);
            _fs->PopTarget();
            _fs->PopTarget();
            _fs->AddInstruction(_OP_SETOUTER, _fs->PushTarget(), lvalue->asId()->outerPos(), tmp);
        }
        else if (id->isLocal()) {
            SQInteger p2 = _fs->PopTarget(); //src in OP_GET
            SQInteger p1 = _fs->PopTarget(); //key in OP_GET
            _fs->PushTarget(p1);
            //EmitCompArithLocal(tok, p1, p1, p2);
            _fs->AddInstruction(op, p1, p2, p1, 0);
            _fs->SnoozeOpt();
        }
        else if (id->isField()) {
            SQInteger val = _fs->PopTarget();
            SQInteger key = _fs->PopTarget();
            SQInteger src = _fs->PopTarget();
            /* _OP_COMPARITH mixes dest obj and source val in the arg1 */
            _fs->AddInstruction(_OP_COMPARITH, _fs->PushTarget(), (src << 16) | val, key, opcode);
        }
        else {
            reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_EXPR);
        }
    }
    else if (lvalue->isAccessExpr()) {
        SQInteger val = _fs->PopTarget();
        SQInteger key = _fs->PopTarget();
        SQInteger src = _fs->PopTarget();
        /* _OP_COMPARITH mixes dest obj and source val in the arg1 */
        _fs->AddInstruction(_OP_COMPARITH, _fs->PushTarget(), (src << 16) | val, key, opcode);
    }
    else {
        reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_EXPR);
    }
}

bool CodegenVisitor::isLValue(Expr *expr) {
    switch (expr->op())
    {
    case TO_GETFIELD:
    case TO_GETTABLE: return expr->asAccessExpr()->receiver()->op() != TO_BASE;
    case TO_ID:
        return !expr->asId()->isBinding();
    default:
        return false;
    }
}

void CodegenVisitor::emitNewSlot(Expr *lvalue, Expr *rvalue) {

    visitNoGet(lvalue);

    visitForceGet(rvalue);

    if (lvalue->isAccessExpr()) { // d.f || d["f"]
        SQInteger val = _fs->PopTarget();
        SQInteger key = _fs->PopTarget();
        SQInteger src = _fs->PopTarget();
        _fs->AddInstruction(_OP_NEWSLOT, _fs->PushTarget(), src, key, val);
    }
    else {
        reportDiagnostic(lvalue, DiagnosticsId::DI_LOCAL_SLOT_CREATE);
    }
}

void CodegenVisitor::emitFieldAssign(bool isLiteral) {
    SQInteger val = _fs->PopTarget();
    SQInteger key = _fs->PopTarget();
    SQInteger src = _fs->PopTarget();

    _fs->AddInstruction(isLiteral ? _OP_SET_LITERAL : _OP_SET, _fs->PushTarget(), src, key, val);
    SQ_STATIC_ASSERT(_OP_DATA_NOP == 0);
    if (isLiteral)
        _fs->AddInstruction(SQOpcode(0), 0, 0, 0, 0);//hint
}

void CodegenVisitor::emitAssign(Expr *lvalue, Expr * rvalue, bool inExpr) {

    visitNoGet(lvalue);

    visitForceGet(rvalue);

    if (lvalue->op() == TO_ID) {
        Id *id = lvalue->asId();

        if (!id->isAssignable()) {
            reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_BINDING, id->id());
            return;
        }

        if (id->isOuter()) {
            SQInteger src = _fs->PopTarget();
            SQInteger dst = _fs->PushTarget();
            _fs->AddInstruction(_OP_SETOUTER, dst, id->outerPos(), src);
        }
        else if (id->isLocal()) {
            SQInteger src = _fs->PopTarget();
            SQInteger dst = _fs->TopTarget();
            _fs->AddInstruction(_OP_MOVE, dst, src);
        }
        else if (id->isField()) {
            emitFieldAssign(false);
        }
        else {
            reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_EXPR);
        }
    }
    else if (lvalue->isAccessExpr()) {
        if (lvalue->asAccessExpr()->receiver()->op() != TO_BASE) {
            emitFieldAssign(canBeLiteral(lvalue->asAccessExpr()));
        }
        else {
            reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_EXPR);
        }
    }
    else {
        reportDiagnostic(lvalue, DiagnosticsId::DI_ASSIGN_TO_EXPR);
    }
}

bool CodegenVisitor::canBeLiteral(AccessExpr *expr) {
    if (!expr->isFieldAccessExpr()) return false;

    FieldAccessExpr *field = expr->asFieldAccessExpr();

    return field->canBeLiteral(CanBeDefaultDelegate(field->fieldName()));
}


bool CodegenVisitor::CanBeDefaultDelegate(const SQChar *key)
{
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
        if (delegTbls[i]->GetStr(key, strlen(key), tmp))
            return true;
    }
    return false;
}


void CodegenVisitor::selectConstant(SQInteger target, const SQObject &constant) {
    SQObjectType ctype = sq_type(constant);
    switch (ctype) {
    case OT_INTEGER: EmitLoadConstInt(_integer(constant), target); break;
    case OT_FLOAT: EmitLoadConstFloat(_float(constant), target); break;
    case OT_BOOL: _fs->AddInstruction(_OP_LOADBOOL, target, _integer(constant)); break;
    default: _fs->AddInstruction(_OP_LOAD, target, _fs->GetConstant(constant)); break;
    }
}

void CodegenVisitor::visitGetFieldExpr(GetFieldExpr *expr) {
    maybeAddInExprLine(expr);


    Expr *receiver = expr->receiver();
    // TODO: figure out why this is not work
    /*
    if (receiver->op() == TO_ID) {
        SQObjectPtr constant;
        SQObject id = _fs->CreateString(receiver->asId()->id());
        if (IsConstant(id, constant)) {
            if (sq_type(constant) == OT_TABLE && (sq_objflags(constant) & SQOBJ_FLAG_IMMUTABLE)) {
                SQObjectPtr next;
                if (_table(constant)->GetStr(expr->fieldName(), strlen(expr->fieldName()), next)) {
                    selectConstant(_fs->PushTarget(), next);
                    expr->setConst();
                    //receiver->setConst();
                    _constVal = next;
                    return;
                }
                else {
                    constant.Null();

                    //error(_SC("invalid enum [no '%s' field]"), expr->fieldName());
                }
            }
        }
    }*/

    visitForceGet(receiver);

    if (receiver->isConst()) {
        SQObjectPtr constant = _constVal;
        if (sq_type(constant) == OT_TABLE && (sq_objflags(constant) & SQOBJ_FLAG_IMMUTABLE) && !expr->isNullable()) {
            SQObjectPtr next;
            if (_table(constant)->GetStr(expr->fieldName(), strlen(expr->fieldName()), next)) {
                SQInstruction &last = _fs->LastInstruction();

                SQInteger target = -1;

                if (last.op == _OP_DLOAD) {
                    SQInteger t1 = last._arg0;
                    SQInteger c1 = last._arg1;
                    assert(last._arg2 == _fs->TopTarget());
                    target = last._arg2;
                    _fs->PopInstructions(1);
                    _fs->AddInstruction(_OP_LOAD, t1, c1);
                }
                else {
                    assert(last.op == _OP_LOAD);
                    assert(last._arg0 == _fs->TopTarget());
                    target = last._arg0;
                    _fs->PopInstructions(1);
                }

                selectConstant(target, next);
                expr->setConst();
                _constVal = next;
                return;
            }
            else {
                _constVal.Null();
                reportDiagnostic(expr, DiagnosticsId::DI_INVALID_ENUM, expr->fieldName(), "enum");
                return;
            }
        }
    }

    SQObject nameObj = _fs->CreateString(expr->fieldName());
    SQInteger constantI = _fs->GetConstant(nameObj);
    _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), constantI);

    SQInteger flags = expr->isNullable() ? OP_GET_FLAG_NO_ERROR : 0;

    bool defaultDelegate = CanBeDefaultDelegate(expr->fieldName());

    if (defaultDelegate) {
        flags |= OP_GET_FLAG_ALLOW_DEF_DELEGATE;
    }

    if (expr->receiver()->op() == TO_BASE) {
        Emit2ArgsOP(_OP_GET, flags);
    } else if (!_donot_get) {
        SQInteger src = _fs->PopTarget();
        SQInteger key = _fs->PopTarget();

        if (expr->canBeLiteral(defaultDelegate)) {
            _fs->AddInstruction(_OP_GET_LITERAL, _fs->PushTarget(), key, src, flags);
            SQ_STATIC_ASSERT(_OP_DATA_NOP == 0);
            _fs->AddInstruction(SQOpcode(0), 0, 0, 0, 0); //hint
        }
        else {
            _fs->AddInstruction(_OP_GET, _fs->PushTarget(), key, src, flags);
        }
    }
}

void CodegenVisitor::visitGetTableExpr(GetTableExpr *expr) {
    // TODO: support similar optimization with contant table as in GetFieldExpr here too

    maybeAddInExprLine(expr);

    visitForceGet(expr->receiver());
    visitForceGet(expr->key());

    // TODO: wtf base?
    if (expr->receiver()->op() == TO_BASE) {
        Emit2ArgsOP(_OP_GET, expr->isNullable() ? OP_GET_FLAG_NO_ERROR : 0);
    } else if (!_donot_get) {
        SQInteger p2 = _fs->PopTarget(); //src in OP_GET
        SQInteger p1 = _fs->PopTarget(); //key in OP_GET
        _fs->AddInstruction(_OP_GET, _fs->PushTarget(), p1, p2, expr->isNullable() ? OP_GET_FLAG_NO_ERROR : 0);
    }
}

void CodegenVisitor::visitBinExpr(BinExpr *expr) {
    maybeAddInExprLine(expr);
    switch (expr->op()) {
    case TO_NEWSLOT: emitNewSlot(expr->lhs(), expr->rhs());  break;
    case TO_NULLC: emitJpmArith(_OP_NULLCOALESCE, expr->lhs(), expr->rhs()); break;
    case TO_OROR: emitJpmArith(_OP_OR, expr->lhs(), expr->rhs()); break;
    case TO_ANDAND: emitJpmArith(_OP_AND, expr->lhs(), expr->rhs()); break;
    case TO_INEXPR_ASSIGN: emitAssign(expr->lhs(), expr->rhs(), true); break;
    case TO_ASSIGN:  emitAssign(expr->lhs(), expr->rhs(), false); break;
    case TO_PLUSEQ:  emitCompoundArith(_OP_ADD, '+', expr->lhs(), expr->rhs()); break;
    case TO_MINUSEQ: emitCompoundArith(_OP_SUB, '-', expr->lhs(), expr->rhs()); break;
    case TO_MULEQ:   emitCompoundArith(_OP_MUL, '*', expr->lhs(), expr->rhs()); break;
    case TO_DIVEQ:   emitCompoundArith(_OP_DIV, '/', expr->lhs(), expr->rhs()); break;
    case TO_MODEQ:   emitCompoundArith(_OP_MOD, '%', expr->lhs(), expr->rhs()); break;
    case TO_ADD: emitSimpleBin(_OP_ADD, expr->lhs(), expr->rhs()); break;
    case TO_SUB: emitSimpleBin(_OP_SUB, expr->lhs(), expr->rhs()); break;
    case TO_MUL: emitSimpleBin(_OP_MUL, expr->lhs(), expr->rhs()); break;
    case TO_DIV: emitSimpleBin(_OP_DIV, expr->lhs(), expr->rhs()); break;
    case TO_MOD: emitSimpleBin(_OP_MOD, expr->lhs(), expr->rhs()); break;
    case TO_OR:  emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_OR); break;
    case TO_AND: emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_AND); break;
    case TO_XOR: emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_XOR); break;
    case TO_USHR:emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_USHIFTR); break;
    case TO_SHR: emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_SHIFTR); break;
    case TO_SHL: emitSimpleBin(_OP_BITW, expr->lhs(), expr->rhs(), BW_SHIFTL); break;
    case TO_EQ:  emitSimpleBin(_OP_EQ, expr->lhs(), expr->rhs()); break;
    case TO_NE:  emitSimpleBin(_OP_NE, expr->lhs(), expr->rhs()); break;
    case TO_GE:  emitSimpleBin(_OP_CMP, expr->lhs(), expr->rhs(), CMP_GE); break;
    case TO_GT:  emitSimpleBin(_OP_CMP, expr->lhs(), expr->rhs(), CMP_G); break;
    case TO_LE:  emitSimpleBin(_OP_CMP, expr->lhs(), expr->rhs(), CMP_LE); break;
    case TO_LT:  emitSimpleBin(_OP_CMP, expr->lhs(), expr->rhs(), CMP_L); break;
    case TO_3CMP: emitSimpleBin(_OP_CMP, expr->lhs(), expr->rhs(), CMP_3W); break;
    case TO_IN:  emitSimpleBin(_OP_EXISTS, expr->lhs(), expr->rhs()); break;
    case TO_INSTANCEOF: emitSimpleBin(_OP_INSTANCEOF, expr->lhs(), expr->rhs()); break;
    default:
        break;
    }
}

void CodegenVisitor::visitTerExpr(TerExpr *expr) {
    maybeAddInExprLine(expr);
    assert(expr->op() == TO_TERNARY);

    visitForceGet(expr->a());
    _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
    SQInteger jzpos = _fs->GetCurrentPos();

    SQInteger trg = _fs->PushTarget();
    visitForceGet(expr->b());
    SQInteger first_exp = _fs->PopTarget();
    if (trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
    SQInteger endfirstexp = _fs->GetCurrentPos();
    _fs->AddInstruction(_OP_JMP, 0, 0);
    SQInteger jmppos = _fs->GetCurrentPos();

    visitForceGet(expr->c());
    SQInteger second_exp = _fs->PopTarget();
    if (trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);

    _fs->SetInstructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
    _fs->SetInstructionParam(jzpos, 1, endfirstexp - jzpos + 1);
    _fs->SnoozeOpt();
}

void CodegenVisitor::visitIncExpr(IncExpr *expr) {
    maybeAddInExprLine(expr);
    Expr *arg = expr->argument();

    visitNoGet(arg);

    if (!isLValue(arg)) {
        reportDiagnostic(arg, DiagnosticsId::DI_INC_DEC_NOT_ASSIGNABLE);
    }

    bool isPostfix = expr->form() == IF_POSTFIX;

    if (arg->isAccessExpr()) {
        Emit2ArgsOP(isPostfix ? _OP_PINC : _OP_INC, expr->diff());
    }
    else if (arg->op() == TO_ID) {
        Id *id = arg->asId();
        if (id->isOuter()) {
            SQInteger tmp1 = _fs->PushTarget();
            SQInteger tmp2 = isPostfix ? _fs->PushTarget() : tmp1;
            _fs->AddInstruction(_OP_GETOUTER, tmp2, id->outerPos());
            _fs->AddInstruction(_OP_PINCL, tmp1, tmp2, 0, expr->diff());
            _fs->AddInstruction(_OP_SETOUTER, tmp2, id->outerPos(), tmp2);
            if (isPostfix) {
                _fs->PopTarget();
            }
        }
        else if (id->isLocal()) {
            SQInteger src = isPostfix ? _fs->PopTarget() : _fs->TopTarget();
            SQInteger dst = isPostfix ? _fs->PushTarget() : src;
            _fs->AddInstruction(isPostfix ? _OP_PINCL : _OP_INCL, dst, src, 0, expr->diff());
        }
        else if (id->isField()) {
            Emit2ArgsOP(isPostfix ? _OP_PINC : _OP_INC, expr->diff());
        }
        else {
            reportDiagnostic(arg, DiagnosticsId::DI_INC_DEC_NOT_ASSIGNABLE);
        }
    }
    else {
        reportDiagnostic(arg, DiagnosticsId::DI_INC_DEC_NOT_ASSIGNABLE);
    }
}

void CodegenVisitor::visitDirectiveStatement(DirectiveStmt *directive)
{
    _fs->lang_features = (_fs->lang_features | directive->setFlags) & ~directive->clearFlags;
    if (directive->applyToDefault)
        _ss(_vm)->defaultLangFeatures = (_ss(_vm)->defaultLangFeatures | directive->setFlags) & ~directive->clearFlags;
}


bool CodegenVisitor::IsConstant(const SQObject &name, SQObject &e)
{
    if (IsLocalConstant(name, e))
        return true;
    if (IsGlobalConstant(name, e))
        return true;
    return false;
}

bool CodegenVisitor::IsLocalConstant(const SQObject &name, SQObject &e)
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

bool CodegenVisitor::IsGlobalConstant(const SQObject &name, SQObject &e)
{
    SQObjectPtr val;
    if (_table(_ss(_vm)->_consts)->Get(name, val)) {
        e = val;
        return true;
    }
    return false;
}

void CodegenVisitor::visitCommaExpr(CommaExpr *expr) {
    for (auto e : expr->expressions()) {
        visitForceGet(e);
    }
}

void CodegenVisitor::visitId(Id *id) {
    maybeAddInExprLine(id);
    SQInteger pos = -1;
    SQObject constant;
    SQObject idObj = _fs->CreateString(id->id());
    bool assignable = false;

    if (sq_isstring(_fs->_name)
        && strcmp(_stringval(_fs->_name), id->id()) == 0
        && _fs->GetLocalVariable(_fs->_name, assignable) == -1) {
        _fs->AddInstruction(_OP_LOADCALLEE, _fs->PushTarget());
        return;
    }

    if (_string(idObj) == _string(_fs->_name)) {
        reportDiagnostic(id, DiagnosticsId::DI_CONFLICTS_WITH, "Varibale", id->id(), "function name");
    }

    if ((pos = _fs->GetLocalVariable(idObj, assignable)) != -1) {
        _fs->PushTarget(pos);
        id->setAssiagnable(assignable);
    }

    else if ((pos = _fs->GetOuterVariable(idObj, assignable)) != -1) {
        id->setOuterPos(pos);
        if (!_donot_get) {
            SQInteger stkPos = _fs->PushTarget();
            _fs->AddInstruction(_OP_GETOUTER, stkPos, pos);
        }
        id->setAssiagnable(assignable);
    }

    else if (IsConstant(idObj, constant)) {
        /* Handle named constant */
        SQObjectPtr constval = constant;

        SQInteger stkPos = _fs->PushTarget();
        id->setConst();
        _constVal = constant;

        /* generate direct or literal function depending on size */
        SQObjectType ctype = sq_type(constval);
        switch (ctype) {
        case OT_INTEGER:
            EmitLoadConstInt(_integer(constval), stkPos);
            break;
        case OT_FLOAT:
            EmitLoadConstFloat(_float(constval), stkPos);
            break;
        case OT_BOOL:
            _fs->AddInstruction(_OP_LOADBOOL, stkPos, _integer(constval));
            break;
        default:
            _fs->AddInstruction(_OP_LOAD, stkPos, _fs->GetConstant(constval));
            break;
        }
    }
    else {
        /* Handle a non-local variable, aka a field. Push the 'this' pointer on
        * the virtual stack (always found in offset 0, so no instruction needs to
        * be generated), and push the key next. Generate an _OP_LOAD instruction
        * for the latter. If we are not using the variable as a dref expr, generate
        * the _OP_GET instruction.

        */
        // TODO: probably we need a special handling for some corner cases
        if (!(_fs->lang_features & LF_TOOLS_COMPILE_CHECK))
            reportDiagnostic(id, DiagnosticsId::DI_UNKNOWN_SYMBOL, id->id());

        _fs->PushTarget(0);
        _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(idObj));
        if (!_donot_get) {
            Emit2ArgsOP(_OP_GET);
        }
        id->setField();
    }
}


void CodegenVisitor::visitNoGet(Node *n) {
    _donot_get = true;
    n->visit(this);
    _donot_get = false;
}

void CodegenVisitor::visitForceGet(Node *n) {
    bool old_dng = _donot_get;
    _donot_get = false;
    n->visit(this);
    _donot_get = old_dng;
}

} // namespace SQCompilation

#endif
