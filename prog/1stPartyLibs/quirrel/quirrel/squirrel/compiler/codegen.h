#pragma once

#include "ast.h"
#include "opcodes.h"
#include "compilationcontext.h"

struct SQFuncState;

namespace SQCompilation {

struct SQScope {
    SQInteger outers;
    SQInteger stacksize;
};

class CodeGenVisitor : public Visitor {

    SQFuncState *_fs;
    SQFuncState *_childFs;
    SQVM *_vm;

    SQScope _scope;
    SQObjectPtrVec _scopedconsts;

    // Handling of chain slot reference expressions such as `a.b.c` or `a[b][c]`.
    enum class ExprChainResolveMode {
        Value, // Fully resolve and fetch the value now (emit GET/GETOUTER opcodes, push constants, etc.).
        Target // Do not fetch; leave the chain in a writable/callable form (e.g., receiver+key or outer ref)
    };
    ExprChainResolveMode _resolve_mode;

    bool _inside_static_memo;
    int _complexity_level;

    const char *_sourceName;

    Arena *_arena;

    // This is used to pass variable stack positions from visitVarDecl() to visitDestructuringDecl()
    // This is somewhat implicit and should be redone.
    SQInteger _last_declared_var_pos = -1;

    SQCompilationContext &_ctx;

public:
    CodeGenVisitor(Arena *arena, const HSQOBJECT *bindings, SQVM *vm, const char *sourceName, SQCompilationContext &ctx);

    bool generate(RootBlock *root, SQObjectPtr &out);

    // API for const building
    bool IsConstant(const SQObject &name, SQObjectPtr &e);
    bool IsLocalConstant(const SQObject &name, SQObjectPtr &e);
    bool IsGlobalConstant(const SQObject &name, SQObjectPtr &e);

    SQObjectPtr compileConstFunc(FunctionExpr *funcExpr);

private:

    void reportDiagnostic(Node *n, int32_t id, ...);

    void CheckDuplicateLocalIdentifier(Node *n, SQObject name, const char *desc, bool ignore_global_consts);
    bool CheckMemberUniqueness(ArenaVector<Expr *> &vec, Expr *obj);

    void EmitLoadConstInt(SQInteger value, SQInteger target);
    void EmitLoadConstFloat(SQFloat value, SQInteger target);

    void ResolveBreaks(SQFuncState *funcstate, SQInteger ntoresolve);
    void ResolveContinues(SQFuncState *funcstate, SQInteger ntoresolve, SQInteger targetpos);

    void EmitDerefOp(SQOpcode op);
    void EmitCheckType(int target, uint32_t type_mask);

    void generateTableExpr(TableExpr *tableExpr);

    SQTable* GetScopedConstsTable();
    void SaveDocstringToVM(void *key, const DocObject &docObject);

    void emitUnaryOp(SQOpcode op, UnExpr *arg);
    void emitDelete(UnExpr *argument);
    void emitSimpleBinaryOp(SQOpcode op, Expr *lhs, Expr *rhs, SQInteger op3 = 0);
    void emitShortCircuitLogicalOp(SQOpcode op, Expr *lhs, Expr *rhs);
    void emitCompoundArith(SQOpcode op, SQInteger opcode, Expr *lvalue, Expr *rvalue);
    void emitStaticMemo(Expr *static_memo_arg, bool is_auto_memo = false);
    void emitInlineConst(Expr *const_initializer);

    bool _visit_arrays_and_tables;
    Node *_variable_node;
    int getSubtreeConstScore(Node *node, bool visit_arrays_and_tables);
    int getSubtreeConstScoreImpl(Node *node);
    bool findConstScoredVar(const SQObjectPtr &name, SQCompiletimeVarInfo &varInfo,
                            unsigned requiredFlags);
    SQInteger inferReceiverType(Expr *receiver);
    bool isConstScoredMethodCall(GetFieldExpr *getField);
    bool isConstScoredDirectCall(CallExpr *callExpr);
    bool isConstEvaluable(Expr *expr);

    void emitNewSlot(Expr *lvalue, Expr *rvalue);
    void emitAssign(Expr *lvalue, Expr * rvalue);
    void emitFieldAssign(int isLiteralIndex);

    bool CanBeTypeMethod(const char *key);
    bool CanBeTableTypeMethod(const char *key);
    bool canBeLiteral(AccessExpr *expr);
    SQObjectPtr GetTypeMethod(SQInteger sq_type, const char* key);

    void MoveIfCurrentTargetIsLocal();

    bool IsSimpleConstant(const SQObject &name);
    bool DoesObjectContainOnlySimpleObjects(const SQObject &obj, int depth);
    bool IsGuaranteedPureFunction(Node *expr);

    SQObjectPtr convertLiteral(LiteralExpr *lit);

    void maybeAddInExprLine(Expr *expr);

    void addLineNumber(Node *node);

    void visitForTarget(Node *n);
    void visitForValue(Node *n);
    bool visitMaybeStaticMemo(Node *n);
    bool visitForValueMaybeStaticMemo(Node *n);

    void selectConstant(SQInteger target, const SQObjectPtr &constant);
    void addPatchDocObjectInstruction(const DocObject &docObject);

    unsigned inferExprTypeMask(Expr *expr);
    unsigned inferExprTypeMaskImpl(Expr *expr);
    bool checkInferredType(Node *reportNode, Expr *expr, unsigned declaredMask);

    Expr *deparen(Expr *e) const;
    bool isFreezeCall(Expr *node);
    Expr *skipConstFreezePure(Expr *expr);
    bool isPureFunctionCall(Expr *node);

    SQObjectPtr compileFunc(FunctionExpr *funcExpr, bool is_const, sqvector<SQObjectPtr> *out_defparam_values);

public:

    void visitBlock(Block *block) override;
    void visitCodeBlockExpr(CodeBlockExpr *expr) override;
    void visitIfStatement(IfStatement *ifStmt) override;
    void visitWhileStatement(WhileStatement *whileLoop) override;
    void visitDoWhileStatement(DoWhileStatement *doWhileLoop) override;
    void visitForStatement(ForStatement *forLoop) override;
    void visitForeachStatement(ForeachStatement *foreachLoop) override;
    void visitSwitchStatement(SwitchStatement *swtch) override;
    void visitTryStatement(TryStatement *tryStmt) override;
    void visitBreakStatement(BreakStatement *breakStmt) override;
    void visitContinueStatement(ContinueStatement *continueStmt) override;
    void visitTerminateStatement(TerminateStatement *terminator) override;
    void visitReturnStatement(ReturnStatement *retStmt) override;
    void visitYieldStatement(YieldStatement *yieldStmt) override;
    void visitThrowStatement(ThrowStatement *throwStmt) override;
    void visitExprStatement(ExprStatement *stmt) override;
    void visitTableExpr(TableExpr *tableExpr) override;
    void visitClassExpr(ClassExpr *klass) override;
    void visitParamDecl(ParamDecl *param) override;
    void visitVarDecl(VarDecl *var) override;
    void visitDeclGroup(DeclGroup *group) override;
    void visitDestructuringDecl(DestructuringDecl *destruct) override;
    void visitFunctionExpr(FunctionExpr *func) override;
    void visitConstDecl(ConstDecl *decl) override;
    void visitEnumDecl(EnumDecl *enums) override;
    void visitCallExpr(CallExpr *call) override;
    void visitBaseExpr(BaseExpr *base) override;
    void visitRootTableAccessExpr(RootTableAccessExpr *expr) override;
    void visitLiteralExpr(LiteralExpr *lit) override;
    void visitArrayExpr(ArrayExpr *expr) override;
    void visitUnExpr(UnExpr *unary) override;
    void visitGetFieldExpr(GetFieldExpr *expr) override;
    void visitGetSlotExpr(GetSlotExpr *expr) override;
    void visitBinExpr(BinExpr *expr) override;
    void visitTerExpr(TerExpr *expr) override;
    void visitIncExpr(IncExpr *expr) override;
    void visitId(Id *id) override;
    void visitCommaExpr(CommaExpr *expr) override;
    void visitDirectiveStatement(DirectiveStmt *dir) override;
};

} // namespace SQCompilation
