#include "sqast.h"

#define DEF_TREE_OP(arg) #arg

const char* sq_tree_op_names[] = {
    TREE_OPS
};
#undef DEF_TREE_OP

namespace SQCompilation {

void Node::visitChildren(Visitor *visitor) {
    switch (op())
    {
    case TO_BLOCK:      static_cast<Block *>(this)->visitChildren(visitor); return;
    case TO_IF:         static_cast<IfStatement *>(this)->visitChildren(visitor); return;
    case TO_WHILE:      static_cast<WhileStatement *>(this)->visitChildren(visitor); return;
    case TO_DOWHILE:    static_cast<DoWhileStatement *>(this)->visitChildren(visitor); return;
    case TO_FOR:        static_cast<ForStatement *>(this)->visitChildren(visitor); return;
    case TO_FOREACH:    static_cast<ForeachStatement *>(this)->visitChildren(visitor); return;
    case TO_SWITCH:     static_cast<SwitchStatement *>(this)->visitChildren(visitor); return;
    case TO_RETURN:     static_cast<ReturnStatement *>(this)->visitChildren(visitor); return;
    case TO_YIELD:      static_cast<YieldStatement *>(this)->visitChildren(visitor); return;
    case TO_THROW:      static_cast<ThrowStatement *>(this)->visitChildren(visitor); return;
    case TO_TRY:        static_cast<TryStatement *>(this)->visitChildren(visitor); return;
    case TO_BREAK:      static_cast<BreakStatement *>(this)->visitChildren(visitor); return;
    case TO_CONTINUE:   static_cast<ContinueStatement *>(this)->visitChildren(visitor); return;
    case TO_EXPR_STMT:  static_cast<ExprStatement *>(this)->visitChildren(visitor); return;
    case TO_EMPTY:      static_cast<EmptyStatement *>(this)->visitChildren(visitor); return;
        //case TO_STATEMENT_MARK:
    case TO_ID:         static_cast<Id *>(this)->visitChildren(visitor); return;
    case TO_COMMA:      static_cast<CommaExpr *>(this)->visitChildren(visitor); return;
    case TO_NULLC:
    case TO_ASSIGN:
    case TO_OROR:
    case TO_ANDAND:
    case TO_OR:
    case TO_XOR:
    case TO_AND:
    case TO_NE:
    case TO_EQ:
    case TO_3CMP:
    case TO_GE:
    case TO_GT:
    case TO_LE:
    case TO_LT:
    case TO_IN:
    case TO_INSTANCEOF:
    case TO_USHR:
    case TO_SHR:
    case TO_SHL:
    case TO_MUL:
    case TO_DIV:
    case TO_MOD:
    case TO_ADD:
    case TO_SUB:
    case TO_NEWSLOT:
    case TO_INEXPR_ASSIGN:
    case TO_PLUSEQ:
    case TO_MINUSEQ:
    case TO_MULEQ:
    case TO_DIVEQ:
    case TO_MODEQ:
        static_cast<BinExpr *>(this)->visitChildren(visitor); return;
    case TO_NOT:
    case TO_BNOT:
    case TO_NEG:
    case TO_TYPEOF:
    case TO_RESUME:
    case TO_CLONE:
    case TO_PAREN:
    case TO_DELETE:
        static_cast<UnExpr *>(this)->visitChildren(visitor); return;
    case TO_LITERAL:
        static_cast<LiteralExpr *>(this)->visitChildren(visitor); return;
    case TO_BASE:
        static_cast<BaseExpr *>(this)->visitChildren(visitor); return;
    case TO_ROOT:
        static_cast<RootExpr *>(this)->visitChildren(visitor); return;
    case TO_INC:
        static_cast<IncExpr *>(this)->visitChildren(visitor); return;
    case TO_DECL_EXPR:
        static_cast<DeclExpr *>(this)->visitChildren(visitor); return;
    case TO_ARRAYEXPR:
        static_cast<ArrayExpr *>(this)->visitChildren(visitor); return;
    case TO_GETFIELD:
        static_cast<GetFieldExpr *>(this)->visitChildren(visitor); return;
    case TO_SETFIELD:
        static_cast<SetFieldExpr *>(this)->visitChildren(visitor); return;
    case TO_GETTABLE:
        static_cast<GetTableExpr *>(this)->visitChildren(visitor); return;
    case TO_SETTABLE:
        static_cast<SetTableExpr *>(this)->visitChildren(visitor); return;
    case TO_CALL:
        static_cast<CallExpr *>(this)->visitChildren(visitor); return;
    case TO_TERNARY:
        static_cast<TerExpr *>(this)->visitChildren(visitor); return;
        //case TO_EXPR_MARK:
    case TO_VAR:
        static_cast<VarDecl *>(this)->visitChildren(visitor); return;
    case TO_PARAM:
        static_cast<ParamDecl *>(this)->visitChildren(visitor); return;
    case TO_CONST:
        static_cast<ConstDecl *>(this)->visitChildren(visitor); return;
    case TO_DECL_GROUP:
        static_cast<DeclGroup *>(this)->visitChildren(visitor); return;
    case TO_DESTRUCTURE:
        static_cast<DestructuringDecl *>(this)->visitChildren(visitor); return;
    case TO_FUNCTION:
        static_cast<FunctionDecl *>(this)->visitChildren(visitor); return;
    case TO_CONSTRUCTOR:
        static_cast<ConstructorDecl *>(this)->visitChildren(visitor); return;
    case TO_CLASS:
        static_cast<ClassDecl *>(this)->visitChildren(visitor); return;
    case TO_ENUM:
        static_cast<EnumDecl *>(this)->visitChildren(visitor); return;
    case TO_TABLE:
        static_cast<TableDecl *>(this)->visitChildren(visitor); return;
    default:
        break;
    }
}

void Node::transformChildren(Transformer *transformer) {
  switch (op())
  {
  case TO_BLOCK:      static_cast<Block *>(this)->transformChildren(transformer); return;
  case TO_IF:         static_cast<IfStatement *>(this)->transformChildren(transformer); return;
  case TO_WHILE:      static_cast<WhileStatement *>(this)->transformChildren(transformer); return;
  case TO_DOWHILE:    static_cast<DoWhileStatement *>(this)->transformChildren(transformer); return;
  case TO_FOR:        static_cast<ForStatement *>(this)->transformChildren(transformer); return;
  case TO_FOREACH:    static_cast<ForeachStatement *>(this)->transformChildren(transformer); return;
  case TO_SWITCH:     static_cast<SwitchStatement *>(this)->transformChildren(transformer); return;
  case TO_RETURN:     static_cast<ReturnStatement *>(this)->transformChildren(transformer); return;
  case TO_YIELD:      static_cast<YieldStatement *>(this)->transformChildren(transformer); return;
  case TO_THROW:      static_cast<ThrowStatement *>(this)->transformChildren(transformer); return;
  case TO_TRY:        static_cast<TryStatement *>(this)->transformChildren(transformer); return;
  case TO_BREAK:      static_cast<BreakStatement *>(this)->transformChildren(transformer); return;
  case TO_CONTINUE:   static_cast<ContinueStatement *>(this)->transformChildren(transformer); return;
  case TO_EXPR_STMT:  static_cast<ExprStatement *>(this)->transformChildren(transformer); return;
  case TO_EMPTY:      static_cast<EmptyStatement *>(this)->transformChildren(transformer); return;
    //case TO_STATEMENT_MARK:
  case TO_ID:         static_cast<Id *>(this)->transformChildren(transformer); return;
  case TO_COMMA:      static_cast<CommaExpr *>(this)->transformChildren(transformer); return;
  case TO_NULLC:
  case TO_ASSIGN:
  case TO_OROR:
  case TO_ANDAND:
  case TO_OR:
  case TO_XOR:
  case TO_AND:
  case TO_NE:
  case TO_EQ:
  case TO_3CMP:
  case TO_GE:
  case TO_GT:
  case TO_LE:
  case TO_LT:
  case TO_IN:
  case TO_INSTANCEOF:
  case TO_USHR:
  case TO_SHR:
  case TO_SHL:
  case TO_MUL:
  case TO_DIV:
  case TO_MOD:
  case TO_ADD:
  case TO_SUB:
  case TO_NEWSLOT:
  case TO_INEXPR_ASSIGN:
  case TO_PLUSEQ:
  case TO_MINUSEQ:
  case TO_MULEQ:
  case TO_DIVEQ:
  case TO_MODEQ:
    static_cast<BinExpr *>(this)->transformChildren(transformer); return;
  case TO_NOT:
  case TO_BNOT:
  case TO_NEG:
  case TO_TYPEOF:
  case TO_RESUME:
  case TO_CLONE:
  case TO_PAREN:
  case TO_DELETE:
    static_cast<UnExpr *>(this)->transformChildren(transformer); return;
  case TO_LITERAL:
    static_cast<LiteralExpr *>(this)->transformChildren(transformer); return;
  case TO_BASE:
    static_cast<BaseExpr *>(this)->transformChildren(transformer); return;
  case TO_ROOT:
    static_cast<RootExpr *>(this)->transformChildren(transformer); return;
  case TO_INC:
    static_cast<IncExpr *>(this)->transformChildren(transformer); return;
  case TO_DECL_EXPR:
    static_cast<DeclExpr *>(this)->transformChildren(transformer); return;
  case TO_ARRAYEXPR:
    static_cast<ArrayExpr *>(this)->transformChildren(transformer); return;
  case TO_GETFIELD:
    static_cast<GetFieldExpr *>(this)->transformChildren(transformer); return;
  case TO_SETFIELD:
    static_cast<SetFieldExpr *>(this)->transformChildren(transformer); return;
  case TO_GETTABLE:
    static_cast<GetTableExpr *>(this)->transformChildren(transformer); return;
  case TO_SETTABLE:
    static_cast<SetTableExpr *>(this)->transformChildren(transformer); return;
  case TO_CALL:
    static_cast<CallExpr *>(this)->transformChildren(transformer); return;
  case TO_TERNARY:
    static_cast<TerExpr *>(this)->transformChildren(transformer); return;
    //case TO_EXPR_MARK:
  case TO_VAR:
    static_cast<VarDecl *>(this)->transformChildren(transformer); return;
  case TO_PARAM:
    static_cast<ParamDecl *>(this)->transformChildren(transformer); return;
  case TO_CONST:
    static_cast<ConstDecl *>(this)->transformChildren(transformer); return;
  case TO_DECL_GROUP:
    static_cast<DeclGroup *>(this)->transformChildren(transformer); return;
  case TO_DESTRUCTURE:
    static_cast<DestructuringDecl *>(this)->transformChildren(transformer); return;
  case TO_FUNCTION:
    static_cast<FunctionDecl *>(this)->transformChildren(transformer); return;
  case TO_CONSTRUCTOR:
    static_cast<ConstructorDecl *>(this)->transformChildren(transformer); return;
  case TO_CLASS:
    static_cast<ClassDecl *>(this)->transformChildren(transformer); return;
  case TO_ENUM:
    static_cast<EnumDecl *>(this)->transformChildren(transformer); return;
  case TO_TABLE:
    static_cast<TableDecl *>(this)->transformChildren(transformer); return;
  default:
    break;
  }
}

void UnExpr::visitChildren(Visitor *visitor) { _arg->visit(visitor); }

void UnExpr::transformChildren(Transformer *transformer) {
  _arg = _arg->transform(transformer)->asExpression();
}

void BinExpr::visitChildren(Visitor *visitor) {
    _lhs->visit(visitor);
    _rhs->visit(visitor);
}

void BinExpr::transformChildren(Transformer *transformer) {
  _lhs = _lhs->transform(transformer)->asExpression();
  _rhs = _rhs->transform(transformer)->asExpression();
}

void TerExpr::visitChildren(Visitor *visitor) {
    _a->visit(visitor);
    _b->visit(visitor);
    _c->visit(visitor);
}

void TerExpr::transformChildren(Transformer *transformer) {
  _a = _a->transform(transformer)->asExpression();
  _b = _b->transform(transformer)->asExpression();
  _c = _c->transform(transformer)->asExpression();
}

void GetFieldExpr::visitChildren(Visitor *visitor) {
    receiver()->visit(visitor);
}

void GetFieldExpr::transformChildren(Transformer *transformer) {
  _receiver = receiver()->transform(transformer)->asExpression();
}

void SetFieldExpr::visitChildren(Visitor *visitor) {
    receiver()->visit(visitor);
    value()->visit(visitor);
}

void SetFieldExpr::transformChildren(Transformer *transformer) {
  _receiver = receiver()->transform(transformer)->asExpression();
  _value = value()->transform(transformer)->asExpression();
}

void GetTableExpr::visitChildren(Visitor *visitor) {
    receiver()->visit(visitor);
    key()->visit(visitor);
}

void GetTableExpr::transformChildren(Transformer *transformer) {
  _receiver = receiver()->transform(transformer)->asExpression();
  _key = key()->transform(transformer)->asExpression();
}

void SetTableExpr::visitChildren(Visitor *visitor) {
    receiver()->visit(visitor);
    key()->visit(visitor);
    value()->visit(visitor);
}

void SetTableExpr::transformChildren(Transformer *transformer) {
  _receiver = receiver()->transform(transformer)->asExpression();
  _key = key()->transform(transformer)->asExpression();
  _val = value()->transform(transformer)->asExpression();
}

void IncExpr::visitChildren(Visitor *visitor) { _arg->visit(visitor); }

void IncExpr::transformChildren(Transformer *transformer) {
  _arg = _arg->transform(transformer)->asExpression();
}

void DeclExpr::visitChildren(Visitor *visitor) { _decl->visit(visitor); }

void DeclExpr::transformChildren(Transformer *transformer) {
  _decl = _decl->transform(transformer)->asDeclaration();
}

void CallExpr::visitChildren(Visitor *visitor) {
    _callee->visit(visitor);
    for (auto arg : arguments())
        arg->visit(visitor);
}

void CallExpr::transformChildren(Transformer *transformer) {
  _callee = _callee->transform(transformer)->asExpression();

  for (auto &arg : arguments())
    arg = arg->transform(transformer)->asExpression();
}

void ArrayExpr::visitChildren(Visitor *visitor) {
    for (auto init : initializers())
        init->visit(visitor);
}

void ArrayExpr::transformChildren(Transformer *transformer) {
  for (auto &init : initializers())
    init = init->transform(transformer)->asExpression();
}

void CommaExpr::visitChildren(Visitor *visitor) {
    for (auto expr : expressions())
        expr->visit(visitor);
}

void CommaExpr::transformChildren(Transformer *transformer) {
  for (auto &expr : expressions())
    expr = expr->transform(transformer)->asExpression();
}

void ValueDecl::visitChildren(Visitor *visitor) {
    if (_expr) _expr->visit(visitor);
}

void ValueDecl::transformChildren(Transformer *transformer) {
  if (_expr) {
    _expr = _expr->transform(transformer)->asExpression();
  }
}

void TableDecl::visitChildren(Visitor *visitor) {
    for (auto &member : members()) {
        member.key->visit(visitor);
        member.value->visit(visitor);
    }
}

void TableDecl::transformChildren(Transformer *transformer) {
  for (auto &member : members()) {
    member.key = member.key->transform(transformer)->asExpression();
    member.value = member.value->transform(transformer)->asExpression();
  }
}

void ClassDecl::visitChildren(Visitor *visitor)  {
    if (_key) _key->visit(visitor);
    if (_base) _base->visit(visitor);
    TableDecl::visitChildren(visitor);
}

void ClassDecl::transformChildren(Transformer *transformer) {
  if (_key) _key = _key->transform(transformer)->asExpression();
  if (_base) _base = _base->transform(transformer)->asExpression();
  TableDecl::transformChildren(transformer);
}

void FunctionDecl::visitChildren(Visitor *visitor) {
    for (auto param : parameters())
        param->visit(visitor);

    body()->visit(visitor);
}

void FunctionDecl::transformChildren(Transformer *transformer) {
  for (auto &param : parameters()) {
    param = param->transform(transformer)->asDeclaration()->asParam();
  }

  setBody(body()->transform(transformer)->asStatement()->asBlock());
}

void FunctionDecl::setBody(Block *body) { _body = body; body->setIsBody(); }

void ConstDecl::visitChildren(Visitor *visitor) {
  _value->visit(visitor);
}

void ConstDecl::transformChildren(Transformer *transformer) {
  _value = _value->transform(transformer)->asExpression()->asLiteral();
}

void DeclGroup::visitChildren(Visitor *visitor) {

    for (auto decl : declarations())
        decl->visit(visitor);
}

void DeclGroup::transformChildren(Transformer *transformer) {

  for (auto &decl : declarations())
    decl = decl->transform(transformer)->asDeclaration()->asVarDecl();
}

void DestructuringDecl::visitChildren(Visitor *visitor) {
    DeclGroup::visitChildren(visitor);
    _expr->visit(visitor);
}

void DestructuringDecl::transformChildren(Transformer *transformer) {
  DeclGroup::transformChildren(transformer);
  _expr = _expr->transform(transformer)->asExpression();
}

void Block::visitChildren(Visitor *visitor) {

    for (auto stmt : statements())
        stmt->visit(visitor);
}

void Block::transformChildren(Transformer *transformer) {

  for (auto &stmt : statements())
    stmt = stmt->transform(transformer)->asStatement();
}

void IfStatement::visitChildren(Visitor *visitor) {
    _cond->visit(visitor);
    _thenB->visit(visitor);
    if (_elseB) _elseB->visit(visitor);
}

void IfStatement::transformChildren(Transformer *transformer) {
  _cond = _cond->transform(transformer)->asExpression();
  _thenB = _thenB->transform(transformer)->asStatement();
  if (_elseB) _elseB = _elseB->transform(transformer)->asStatement();
}

void LoopStatement::visitChildren(Visitor *visitor) {
    _body->visit(visitor);
}

void LoopStatement::transformChildren(Transformer *transformer) {
  _body = _body->transform(transformer)->asStatement();
}

void WhileStatement::visitChildren(Visitor *visitor) {
    _cond->visit(visitor);
    LoopStatement::visitChildren(visitor);
}

void WhileStatement::transformChildren(Transformer *transformer) {
  _cond = _cond->transform(transformer)->asExpression();
  LoopStatement::transformChildren(transformer);
}

void DoWhileStatement::visitChildren(Visitor *visitor) {
    LoopStatement::visitChildren(visitor);
    _cond->visit(visitor);
}

void DoWhileStatement::transformChildren(Transformer *transformer) {
  LoopStatement::transformChildren(transformer);
  _cond = _cond->transform(transformer)->asExpression();
}

void ForStatement::visitChildren(Visitor *visitor) {
    if (_init) _init->visit(visitor);
    if (_cond) _cond->visit(visitor);
    if (_mod) _mod->visit(visitor);

    LoopStatement::visitChildren(visitor);
}

void ForStatement::transformChildren(Transformer *transformer) {
  if (_init) _init = _init->transform(transformer);
  if (_cond) _cond = _cond->transform(transformer)->asExpression();
  if (_mod) _mod = _mod->transform(transformer)->asExpression();

  LoopStatement::transformChildren(transformer);
}

void ForeachStatement::visitChildren(Visitor *visitor) {
    if (_idx) _idx->visit(visitor);
    if (_val) _val->visit(visitor);
    if (_container) _container->visit(visitor);

    LoopStatement::visitChildren(visitor);
}

void ForeachStatement::transformChildren(Transformer *transformer) {
  if (_idx) _idx = _idx->transform(transformer)->asDeclaration()->asVarDecl();
  if (_val) _val = _val->transform(transformer)->asDeclaration()->asVarDecl();
  if (_container) _container = _container->transform(transformer)->asExpression();

  LoopStatement::transformChildren(transformer);
}

void SwitchStatement::visitChildren(Visitor *visitor) {
    _expr->visit(visitor);

    for (auto &c : cases()) {
        c.val->visit(visitor);
        c.stmt->visit(visitor);
    }

    if (_defaultCase.stmt) {
        _defaultCase.stmt->visit(visitor);
    }
}

void SwitchStatement::transformChildren(Transformer *transformer) {
  _expr = _expr->transform(transformer)->asExpression();

  for (auto &c : cases()) {
    c.val = c.val->transform(transformer)->asExpression();
    c.stmt = c.stmt->transform(transformer)->asStatement();
  }

  if (_defaultCase.stmt) {
    _defaultCase.stmt = _defaultCase.stmt->transform(transformer)->asStatement();
  }
}

void TryStatement::visitChildren(Visitor *visitor) {
    _tryStmt->visit(visitor);
    visitor->visitId(_exception);
    _catchStmt->visit(visitor);
}

void TryStatement::transformChildren(Transformer *transformer) {
  _tryStmt = _tryStmt->transform(transformer)->asStatement();
  _exception = _exception->transform(transformer)->asId();
  _catchStmt = _catchStmt->transform(transformer)->asStatement();
}

void TerminateStatement::visitChildren(Visitor *visitor) {
    if (_arg) _arg->visit(visitor);
}

void TerminateStatement::transformChildren(Transformer *transformer) {
  if (_arg) _arg = _arg->transform(transformer)->asExpression();
}

void ExprStatement::visitChildren(Visitor *visitor) { _expr->visit(visitor); }

void ExprStatement::transformChildren(Transformer *transformer) {
  _expr = _expr->transform(transformer)->asExpression();
}

const char* treeopStr(enum TreeOp op) {
  switch (op)
  {
  case TO_NULLC: return "??";
  case TO_ASSIGN: return "=";
  case TO_OROR: return "||";
  case TO_ANDAND: return "&&";
  case TO_OR: return "|";
  case TO_XOR: return "^";
  case TO_AND: return "&";
  case TO_NE: return "!=";
  case TO_EQ: return "==";
  case TO_3CMP: return "<=>";
  case TO_GE: return ">=";
  case TO_GT: return ">";
  case TO_LE: return "<=";
  case TO_LT: return "<";
  case TO_USHR: return ">>>";
  case TO_SHR: return ">>";
  case TO_SHL: return "<<";
  case TO_MUL: return "*";
  case TO_DIV: return "/";
  case TO_MOD: return "%";
  case TO_ADD: return "+";
  case TO_SUB:
  case TO_NEG: return "-";
  case TO_NOT: return "!";
  case TO_BNOT: return "~";
  case TO_INEXPR_ASSIGN: return ":=";
  case TO_NEWSLOT: return "<-";
  case TO_PLUSEQ: return "+=";
  case TO_MINUSEQ: return "-=";
  case TO_MULEQ: return "*=";
  case TO_DIVEQ: return "/=";
  case TO_MODEQ: return "%=";
  default: return nullptr;
  }
}

};
