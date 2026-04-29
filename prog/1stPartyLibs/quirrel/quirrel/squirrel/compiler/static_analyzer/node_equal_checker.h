#pragma once

#include "compiler/ast.h"


namespace SQCompilation
{

class NodeEqualChecker
{
  template<typename N>
  bool cmpNodeVector(const ArenaVector<N *> &lhs, const ArenaVector<N *> &rhs) const {
    if (lhs.size() != rhs.size())
      return false;

    for (int32_t i = 0; i < lhs.size(); ++i) {
      if (!check(lhs[i], rhs[i]))
        return false;
    }

    return true;
  }

  bool cmpId(const Id *l, const Id* r) const {
    return strcmp(l->name(), r->name()) == 0;
  }

  bool cmpLiterals(const LiteralExpr *l, const LiteralExpr *r) const {
    if (l->kind() != r->kind())
      return false;

    switch (l->kind())
    {
    case LK_STRING: return strcmp(l->s(), r->s()) == 0;
    default: return l->raw() == r->raw();
    }
  }

  bool cmpBinary(const BinExpr *l, const BinExpr *r) const {
    return check(l->lhs(), r->lhs()) && check(l->rhs(), r->rhs());
  }

  bool cmpUnary(const UnExpr *l, const UnExpr *r) const {
    return check(l->argument(), r->argument());
  }

  bool cmpTernary(const TerExpr *l, const TerExpr *r) const {
    return check(l->a(), r->a()) && check(l->b(), r->b()) && check(l->c(), r->c());
  }

  bool cmpBlock(const Block *l, const Block *r) const {
    if (l->isRoot() != r->isRoot())
      return false;

    if (l->isBody() != r->isBody())
      return false;

    return cmpNodeVector(l->statements(), r->statements());
  }

  bool cmpIf(const IfStatement *l, const IfStatement *r) const {
    if (!check(l->condition(), r->condition()))
      return false;

    if (!check(l->thenBranch(), r->thenBranch()))
      return false;

    return check(l->elseBranch(), r->elseBranch());
  }

  bool cmpWhile(const WhileStatement *l, const WhileStatement *r) const {
    if (!check(l->condition(), r->condition()))
      return false;

    return check(l->body(), r->body());
  }

  bool cmpDoWhile(const DoWhileStatement *l, const DoWhileStatement *r) const {
    if (!check(l->body(), r->body()))
      return false;

    return check(l->condition(), r->condition());
  }

  bool cmpFor(const ForStatement *l, const ForStatement *r) const {
    if (!check(l->initializer(), r->initializer()))
      return false;

    if (!check(l->condition(), r->condition()))
      return false;

    if (!check(l->modifier(), r->modifier()))
      return false;

    return check(l->body(), r->body());
  }

  bool cmpForeach(const ForeachStatement *l, const ForeachStatement *r) const {
    if (!check(l->idx(), r->idx()))
      return false;

    if (!check(l->val(), r->val()))
      return false;

    if (!check(l->container(), r->container()))
      return false;

    return check(l->body(), r->body());
  }

  bool cmpSwitch(const SwitchStatement *l, const SwitchStatement *r) const {
    if (!check(l->expression(), r->expression()))
      return false;

    const auto &lcases = l->cases();
    const auto &rcases = r->cases();

    if (lcases.size() != rcases.size())
      return false;

    for (int32_t i = 0; i < lcases.size(); ++i) {
      const auto &lc = lcases[i];
      const auto &rc = rcases[i];

      if (!check(lc.val, rc.val))
        return false;

      if (!check(lc.stmt, rc.stmt))
        return false;
    }

    return check(l->defaultCase().stmt, r->defaultCase().stmt);
  }

  bool cmpTry(const TryStatement *l, const TryStatement *r) const {
    if (!check(l->tryStatement(), r->tryStatement()))
      return false;

    if (!check(l->exceptionId(), r->exceptionId()))
      return false;

    return check(l->catchStatement(), r->catchStatement());
  }

  bool cmpTerminate(const TerminateStatement *l, const TerminateStatement *r) const {
    return check(l->argument(), r->argument());
  }

  bool cmpReturn(const ReturnStatement *l, const ReturnStatement *r) const {
    return l->isLambdaReturn() == r->isLambdaReturn() && cmpTerminate(l, r);
  }

  bool cmpExprStmt(const ExprStatement *l, const ExprStatement *r) const {
    return check(l->expression(), r->expression());
  }

  bool cmpComma(const CommaExpr *l, const CommaExpr *r) const {
    return cmpNodeVector(l->expressions(), r->expressions());
  }

  bool cmpIncExpr(const IncExpr *l, const IncExpr *r) const {
    if (l->form() != r->form())
      return false;

    if (l->diff() != r->diff())
      return false;

    return check(l->argument(), r->argument());
  }

  bool cmpCallExpr(const CallExpr *l, const CallExpr *r) const {
    if (l->isNullable() != r->isNullable())
      return false;

    if (!check(l->callee(), r->callee()))
      return false;

    return cmpNodeVector(l->arguments(), r->arguments());
  }

  bool cmpArrayExpr(const ArrayExpr *l, const ArrayExpr *r) const {
    return cmpNodeVector(l->initializers(), r->initializers());
  }

  bool cmpGetField(const GetFieldExpr *l, const GetFieldExpr *r) const {
    if (l->isNullable() != r->isNullable())
      return false;

    if (strcmp(l->fieldName(), r->fieldName()) != 0)
      return false;

    return check(l->receiver(), r->receiver());
  }

  bool cmpGetSlot(const GetSlotExpr *l, const GetSlotExpr *r) const {
    if (l->isNullable() != r->isNullable())
      return false;

    if (!check(l->key(), r->key()))
      return false;

    return check(l->receiver(), r->receiver());
  }

  bool cmpValueDecl(const ValueDecl *l, const ValueDecl *r) const {
    if (!check(l->expression(), r->expression()))
      return false;

    return strcmp(l->name(), r->name()) == 0;
  }

  bool cmpVarDecl(const VarDecl *l, const VarDecl *r) const {
    return l->isAssignable() == r->isAssignable() && cmpValueDecl(l, r);
  }

  bool cmpConst(const ConstDecl *l, const ConstDecl *r) const {
    if (l->isGlobal() != r->isGlobal())
      return false;

    if (strcmp(l->name(), r->name()) != 0)
      return false;

    return check(l->value(), r->value());
  }

  bool cmpDeclGroup(const DeclGroup *l, const DeclGroup *r) const {
    return cmpNodeVector(l->declarations(), r->declarations());
  }

  bool cmpDestructDecl(const DestructuringDecl *l, const DestructuringDecl *r) const {
    return l->type() == r->type() && check(l->initExpression(), r->initExpression());
  }

  bool cmpFunction(const FunctionExpr *l, const FunctionExpr *r) const {
    if (l->isVararg() != r->isVararg())
      return false;

    if (l->isLambda() != r->isLambda())
      return false;

    if (strcmp(l->name(), r->name()) != 0)
      return false;

    if (!cmpNodeVector(l->parameters(), r->parameters()))
      return false;

    return check(l->body(), r->body());
  }

  bool cmpTable(const TableExpr *l, const TableExpr *r) const {
    const auto &lmems = l->members();
    const auto &rmems = r->members();

    if (lmems.size() != rmems.size())
      return false;

    for (int32_t i = 0; i < lmems.size(); ++i) {
      const auto &lm = lmems[i];
      const auto &rm = rmems[i];

      if (!check(lm.key, rm.key))
        return false;

      if (!check(lm.value, rm.value))
        return false;

      if (lm.flags != rm.flags)
        return false;
    }

    return true;
  }

  bool cmpClass(const ClassExpr *l, const ClassExpr *r) const {
    if (!check(l->classBase(), r->classBase()))
      return false;

    if (!check(l->classKey(), r->classKey()))
      return false;

    return cmpTable(l, r);
  }

  bool cmpEnumDecl(const EnumDecl *l, const EnumDecl *r) const {
    if (l->isGlobal() != r->isGlobal())
      return false;

    if (strcmp(l->name(), r->name()) != 0)
      return false;

    const auto &lcs = l->consts();
    const auto &rcs = r->consts();

    if (lcs.size() != rcs.size())
      return false;

    for (int32_t i = 0; i < lcs.size(); ++i) {
      const auto &lc = lcs[i];
      const auto &rc = rcs[i];

      if (strcmp(lc.id, rc.id) != 0)
        return false;

      if (!cmpLiterals(lc.val, rc.val))
        return false;
    }

    return true;
  }

public:

  bool check(const Node *lhs, const Node *rhs) const {

    if (lhs == rhs)
      return true;

    if (!lhs || !rhs)
      return false;

    if (lhs->op() != rhs->op())
      return false;

    switch (lhs->op())
    {
    case TO_BLOCK:      return cmpBlock((const Block *)lhs, (const Block *)rhs);
    case TO_IF:         return cmpIf((const IfStatement *)lhs, (const IfStatement *)rhs);
    case TO_WHILE:      return cmpWhile((const WhileStatement *)lhs, (const WhileStatement *)rhs);
    case TO_DOWHILE:    return cmpDoWhile((const DoWhileStatement *)lhs, (const DoWhileStatement *)rhs);
    case TO_FOR:        return cmpFor((const ForStatement *)lhs, (const ForStatement *)rhs);
    case TO_FOREACH:    return cmpForeach((const ForeachStatement *)lhs, (const ForeachStatement *)rhs);
    case TO_SWITCH:     return cmpSwitch((const SwitchStatement *)lhs, (const SwitchStatement *)rhs);
    case TO_RETURN:
      return cmpReturn((const ReturnStatement *)lhs, (const ReturnStatement *)rhs);
    case TO_YIELD:
    case TO_THROW:
      return cmpTerminate((const TerminateStatement *)lhs, (const TerminateStatement *)rhs);
    case TO_TRY:
      return cmpTry((const TryStatement *)lhs, (const TryStatement *)rhs);
    case TO_BREAK:
    case TO_CONTINUE:
    case TO_EMPTY:
    case TO_BASE:
    case TO_ROOT_TABLE_ACCESS:
      return true;
    case TO_EXPR_STMT:
      return cmpExprStmt((const ExprStatement *)lhs, (const ExprStatement *)rhs);

      //case TO_STATEMENT_MARK:
    case TO_ID:         return cmpId((const Id *)lhs, (const Id *)rhs);
    case TO_COMMA:      return cmpComma((const CommaExpr *)lhs, (const CommaExpr *)rhs);
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
    case TO_PLUSEQ:
    case TO_MINUSEQ:
    case TO_MULEQ:
    case TO_DIVEQ:
    case TO_MODEQ:
      return cmpBinary((const BinExpr *)lhs, (const BinExpr *)rhs);
    case TO_NOT:
    case TO_BNOT:
    case TO_NEG:
    case TO_TYPEOF:
    case TO_STATIC_MEMO:
    case TO_INLINE_CONST:
    case TO_RESUME:
    case TO_CLONE:
    case TO_PAREN:
    case TO_DELETE:
      return cmpUnary((const UnExpr *)lhs, (const UnExpr *)rhs);
    case TO_LITERAL:
      return cmpLiterals((const LiteralExpr *)lhs, (const LiteralExpr *)rhs);
    case TO_INC:
      return cmpIncExpr((const IncExpr *)lhs, (const IncExpr *)rhs);
    case TO_TABLE:
      return cmpTable((const TableExpr *)lhs, (const TableExpr *)rhs);
    case TO_CLASS:
      return cmpClass((const ClassExpr *)lhs, (const ClassExpr *)rhs);
    case TO_FUNCTION:
      return cmpFunction((const FunctionExpr *)lhs, (const FunctionExpr *)rhs);
    case TO_ARRAY:
      return cmpArrayExpr((const ArrayExpr *)lhs, (const ArrayExpr *)rhs);
    case TO_GETFIELD:
      return cmpGetField((const GetFieldExpr *)lhs, (const GetFieldExpr *)rhs);
    case TO_GETSLOT:
      return cmpGetSlot((const GetSlotExpr *)lhs, (const GetSlotExpr *)rhs);
    case TO_CALL:
      return cmpCallExpr((const CallExpr *)lhs, (const CallExpr *)rhs);
    case TO_TERNARY:
      return cmpTernary((const TerExpr *)lhs, (const TerExpr *)rhs);
      //case TO_EXPR_MARK:
    case TO_VAR:
      return cmpVarDecl((const VarDecl *)lhs, (const VarDecl *)rhs);
    case TO_PARAM:
      return cmpValueDecl((const ValueDecl *)lhs, (const ValueDecl *)rhs);
    case TO_CONST:
      return cmpConst((const ConstDecl *)lhs, (const ConstDecl *)rhs);
    case TO_DECL_GROUP:
      return cmpDeclGroup((const DeclGroup *)lhs, (const DeclGroup *)rhs);
    case TO_DESTRUCTURE:
      return cmpDestructDecl((const DestructuringDecl *)lhs, (const DestructuringDecl *)rhs);
    case TO_ENUM:
      return cmpEnumDecl((const EnumDecl *)lhs, (const EnumDecl *)rhs);
    case TO_SETFIELD:
    case TO_SETSLOT:
    default:
      assert(0);
      return false;
    }
  }
};

}
