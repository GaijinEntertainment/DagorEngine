#pragma once

#include "ast.h"
#include "opcodes.h"
#include "compilationcontext.h"
#include <setjmp.h>

struct SQFuncState;

namespace SQCompilation {

class ConstGenVisitor : public Visitor {
private:
    SQCompilationContext &_ctx;
    SQVM *_vm;
    SQFuncState *_fs;
    CodeGenVisitor &_codegen;

    SQObjectPtr _result;
    SQObjectPtr _call_target; //< must null it in every node visit and assign the container in get operations

public:
    ConstGenVisitor(SQVM *vm, SQFuncState *fs, SQCompilationContext &ctx, CodeGenVisitor &codegen)
        : _ctx(ctx), _fs(fs), _vm(vm), _codegen(codegen)
    {
    }

public:
    virtual ~ConstGenVisitor() {}

    void  process(Expr *expr, SQObjectPtr &out);

    virtual void visitNode(Node *node) { node->visitChildren(this); }

    virtual void visitExpr(Expr *expr) { visitNode(expr); }
    virtual void visitUnExpr(UnExpr *expr);
    virtual void visitBinExpr(BinExpr *expr);
    virtual void visitTerExpr(TerExpr *expr);
    virtual void visitCallExpr(CallExpr *expr);
    virtual void visitId(Id *id);
    virtual void visitAccessExpr(AccessExpr *expr) { throwUnsupported(expr, "access expression"); }
    virtual void visitGetFieldExpr(GetFieldExpr *expr);
    virtual void visitSetFieldExpr(SetFieldExpr *expr) { throwUnsupported(expr, "set field expression"); }
    virtual void visitGetSlotExpr(GetSlotExpr *expr);
    virtual void visitSetSlotExpr(SetSlotExpr *expr) { throwUnsupported(expr, "set slot expression"); }
    virtual void visitBaseExpr(BaseExpr *expr) { throwUnsupported(expr, "base expression"); }
    virtual void visitRootTableAccessExpr(RootTableAccessExpr *expr) { throwUnsupported(expr, "root table access expression"); }

    virtual void visitLiteralExpr(LiteralExpr *expr);

    virtual void visitIncExpr(IncExpr *expr) { throwUnsupported(expr, "increment expression"); }

    virtual void visitArrayExpr(ArrayExpr *expr);
    virtual void visitTableExpr(TableExpr *tbl);
    virtual void visitClassExpr(ClassExpr *cls) { visitTableExpr(cls); }
    virtual void visitFunctionExpr(FunctionExpr *f);

    virtual void visitCommaExpr(CommaExpr *expr) { throwUnsupported(expr, "comma expression"); }
    virtual void visitExternalValueExpr(ExternalValueExpr *expr) { throwUnsupported(expr, "external value expression"); }

    virtual void visitStmt(Statement *stmt) { throwUnsupported(stmt, "statement"); }
    virtual void visitBlock(Block *block) { throwUnsupported(block, "block statement"); }
    virtual void visitIfStatement(IfStatement *ifstmt) { throwUnsupported(ifstmt, "if statement"); }
    virtual void visitLoopStatement(LoopStatement *loop) { throwUnsupported(loop, "loop statement"); }
    virtual void visitWhileStatement(WhileStatement *loop) { throwUnsupported(loop, "while statement"); }
    virtual void visitDoWhileStatement(DoWhileStatement *loop) { throwUnsupported(loop, "do-while statement"); }
    virtual void visitForStatement(ForStatement *loop) { throwUnsupported(loop, "for statement"); }
    virtual void visitForeachStatement(ForeachStatement *loop) { throwUnsupported(loop, "foreach statement"); }
    virtual void visitSwitchStatement(SwitchStatement *swtch) { throwUnsupported(swtch, "switch statement"); }
    virtual void visitTryStatement(TryStatement *tr) { throwUnsupported(tr, "try statement"); }
    virtual void visitTerminateStatement(TerminateStatement *term) { throwUnsupported(term, "terminate statement"); }
    virtual void visitReturnStatement(ReturnStatement *ret) { throwUnsupported(ret, "return statement"); }
    virtual void visitYieldStatement(YieldStatement *yld) { throwUnsupported(yld, "yield statement"); }
    virtual void visitThrowStatement(ThrowStatement *thr) { throwUnsupported(thr, "throw statement"); }
    virtual void visitJumpStatement(JumpStatement *jmp) { throwUnsupported(jmp, "jump statement"); }
    virtual void visitBreakStatement(BreakStatement *jmp) { throwUnsupported(jmp, "break statement"); }
    virtual void visitContinueStatement(ContinueStatement *jmp) { throwUnsupported(jmp, "continue statement"); }
    virtual void visitExprStatement(ExprStatement *estmt) { throwUnsupported(estmt, "expression statement"); }
    virtual void visitEmptyStatement(EmptyStatement *empty) { throwUnsupported(empty, "empty statement"); }

    virtual void visitDecl(Decl *decl) { throwUnsupported(decl, "declaration"); }
    virtual void visitValueDecl(ValueDecl *decl) { throwUnsupported(decl, "value declaration"); }
    virtual void visitVarDecl(VarDecl *decl) { throwUnsupported(decl, "variable declaration"); }
    virtual void visitParamDecl(ParamDecl *decl) { throwUnsupported(decl, "parameter declaration"); }
    virtual void visitConstDecl(ConstDecl *cnst) { visitDecl(cnst); }
    virtual void visitEnumDecl(EnumDecl *enm) { visitDecl(enm); }
    virtual void visitDeclGroup(DeclGroup *grp) { visitDecl(grp); }
    virtual void visitDestructuringDecl(DestructuringDecl  *destruct) { visitDecl(destruct); }

    virtual void visitDirectiveStatement(DirectiveStmt *dir) { throwUnsupported(dir, "directive statement"); }

private:
    void throwUnsupported(Node *n, const char *type);
    void throwGeneralError(Node *n, const char *msg);
    void throwGeneralErrorFmt(Node *n, const char *fmt, ...);
    SQObjectPtr convertLiteral(LiteralExpr *lit);
};

} // namespace SQCompilation
