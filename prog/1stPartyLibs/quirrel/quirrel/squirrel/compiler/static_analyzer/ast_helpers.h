#pragma once

namespace SQCompilation
{

inline const Expr *deparen(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_PAREN)
    return deparen(static_cast<const UnExpr *>(e)->argument());
  return e;
}

inline const Expr *deparenStatic(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_PAREN || e->op() == TO_STATIC_MEMO)
    return deparen(static_cast<const UnExpr *>(e)->argument());
  return e;
}

inline const Expr *skipUnary(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_INC) {
    return skipUnary(static_cast<const IncExpr *>(e)->argument());
  }

  if (TO_NOT <= e->op() && e->op() <= TO_DELETE) {
    return skipUnary(static_cast<const UnExpr *>(e)->argument());
  }

  return e;
}

inline const Statement *lastNonEmpty(const Block *b, int32_t &effectiveSize) {
  const Statement *r = nullptr;
  effectiveSize = 0;
  for (auto stmt : b->statements()) {
    if (stmt->op() != TO_EMPTY) {
      r = stmt;
      effectiveSize += 1;
    }
  }

  return r;
}

inline const Statement *unwrapBody(const Statement *stmt) {
  if (!stmt)
    return nullptr;

  if (stmt->op() != TO_BLOCK)
    return stmt;

  auto &stmts = stmt->asBlock()->statements();

  if (stmts.empty())
    return nullptr;

  return unwrapBody(stmts.back());
}

// in contrast to `unwrapBody(...)` above this function skips empty statements
inline const Statement *unwrapBodyNonEmpty(const Statement *stmt) {

  if (stmt == nullptr)
    return stmt;

  if (stmt->op() != TO_BLOCK)
    return stmt;

  int32_t effectiveSize = 0;
  const Statement *last = lastNonEmpty(stmt->asBlock(), effectiveSize);

  if (effectiveSize == 0)
    return nullptr;

  return unwrapBodyNonEmpty(last);
}

inline const Statement *unwrapSingleBlock(const Statement *stmt) {
  if (!stmt)
    return nullptr;

  if (stmt->op() != TO_BLOCK)
    return stmt;

  int32_t effectiveSize = 0;
  const Statement *last = lastNonEmpty(stmt->asBlock(), effectiveSize);

  if (effectiveSize == 0)
    return nullptr;

  if (effectiveSize != 1)
    return stmt;

  return unwrapSingleBlock(last);
}

inline Expr *unwrapExprStatement(Statement *stmt) {
  return stmt->op() == TO_EXPR_STMT ? static_cast<ExprStatement *>(stmt)->expression() : nullptr;
}


inline const FunctionExpr *extractFunction(const Node *n) {
  if (!n)
    return nullptr;

  if (n->op() == TO_FUNCTION)
    return static_cast<const FunctionExpr *>(n);

  if (n->op() == TO_VAR)
    return extractFunction(n->asDeclaration()->asVarDecl()->initializer());

  return nullptr;
}


inline const Expr *extractAssignedExpression(const Node *n) {
  if (n->op() == TO_ASSIGN || n->op() == TO_NEWSLOT)
    return static_cast<const BinExpr *>(n)->rhs();

  if (n->op() == TO_VAR)
    return static_cast<const VarDecl *>(n)->initializer();

  return nullptr;
}


inline const Id *extractReceiver(const Expr *e) {
  const Expr *last = e;

  for (;;) {
    e = deparenStatic(e);

    if (e->op() == TO_CALL) { // -V522
      const CallExpr *c = e->asCallExpr();
      e = c->callee();
      if (c->isNullable())
        last = c->callee();
    } else if (e->isAccessExpr()) { // -V522
      const AccessExpr *acc = e->asAccessExpr();
      e = acc->receiver();
      if (acc->isNullable())
        last = acc->receiver();
    } else {
      break;
    }
  }

  return last->op() == TO_ID ? last->asId() : nullptr;
}


inline bool isFallThroughStatement(const Statement *stmt) {
  TreeOp op = stmt->op();

  return op != TO_RETURN && op != TO_THROW && op != TO_BREAK && op != TO_CONTINUE;
}


inline bool isFallThroughBranch(const Statement *stmt) {
  if (stmt->op() != TO_BLOCK)
    return isFallThroughStatement(stmt);

  const Block *blk = stmt->asBlock();

  for (const Statement *s : blk->statements()) {
    if (!isFallThroughStatement(s))
      return false;
  }

  return true;
}

}
