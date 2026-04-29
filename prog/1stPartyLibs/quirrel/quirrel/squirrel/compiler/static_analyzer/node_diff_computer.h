#pragma once

namespace SQCompilation
{


class NodeDiffComputer
{

  const struct {
    const int32_t OpDiffCost = 40;
    const int32_t SizeDiffCost = 30;
    const int32_t SizeDiffCoeff = 11;
    const int32_t NullDiffCost = SizeDiffCost;
    const int32_t IncFormDiffCost = 2;
    const int32_t IncValDiffCost = 1;
    const int32_t NameDiffCost = 1;
    const int32_t LiteralDiffCost = 1;
    const int32_t MutabilityDiffCost = 1;
    const int32_t StaticMemberDiffCost = 2;
    const int32_t NullabilityDiffCost = 3;
  } DiffCosts;

  const int32_t limit;

  int32_t sizeDiff(const int32_t l, const int32_t r) const {
    return DiffCosts.SizeDiffCost + std::abs(l - r) * DiffCosts.SizeDiffCoeff;
  }

  NodeDiffComputer(int32_t l) : limit(l) {}

  int32_t average(int32_t total, int32_t size) const {
    int32_t aver = size ? total / size : 0;

    if (aver == 0)
      aver = 1;

    return aver;
  }

  int32_t diffBlock(const Block *lhs, const Block *rhs) {
    const auto &l = lhs->statements();
    const auto &r = rhs->statements();

    if (l.size() != r.size())
      return sizeDiff(l.size(), r.size());

    int32_t total = 0;

    for (int32_t i = 0; i < l.size(); ++i) {
      int32_t tmp = diffNodes(l[i], r[i]);
      if (tmp > limit)
        return tmp;
      total += tmp;
    }

    return total;
  }

  int32_t diffIf(const IfStatement *lhs, const IfStatement *rhs) {

    int32_t condDiff = diffNodes(lhs->condition(), rhs->condition());

    if (condDiff > limit)
      return condDiff;

    int32_t thenDiff = diffNodes(lhs->thenBranch(), rhs->thenBranch());

    if (thenDiff > limit)
      return thenDiff;

    int32_t elseDiff = diffNodes(lhs->elseBranch(), rhs->elseBranch());

    if (elseDiff > limit)
      return elseDiff;

    return condDiff + thenDiff + elseDiff;
  }

  int32_t diffWhile(const WhileStatement *lhs, const WhileStatement *rhs) {
    int32_t condDiff = diffNodes(lhs->condition(), rhs->condition());

    if (condDiff > limit)
      return condDiff;

    int32_t bodyDiff = diffNodes(lhs->body(), rhs->body());

    if (bodyDiff > limit)
      return bodyDiff;

    return condDiff + bodyDiff;
  }

  int32_t diffDoWhile(const DoWhileStatement *lhs, const DoWhileStatement *rhs) {
    int32_t bodyDiff = diffNodes(lhs->body(), rhs->body());

    if (bodyDiff > limit)
      return bodyDiff;

    int32_t condDiff = diffNodes(lhs->condition(), rhs->condition());

    if (condDiff > limit)
      return condDiff;

    return bodyDiff + condDiff;
  }

  int32_t diffFor(const ForStatement *lhs, const ForStatement *rhs) {
    int32_t initDiff = diffNodes(lhs->initializer(), rhs->initializer());

    if (initDiff > limit)
      return initDiff;

    int32_t condDiff = diffNodes(lhs->condition(), rhs->condition());

    if (condDiff > limit)
      return condDiff;

    int32_t modDiff = diffNodes(lhs->modifier(), rhs->modifier());

    if (modDiff > limit)
      return modDiff;

    int32_t bodyDiff = diffNodes(lhs->body(), rhs->body());

    if (bodyDiff > limit)
      return bodyDiff;

    return initDiff + condDiff + modDiff + bodyDiff;
  }

  int32_t diffForeach(const ForeachStatement *lhs, const ForeachStatement *rhs) {
    int32_t idxDiff = diffNodes(lhs->idx(), rhs->idx());

    if (idxDiff > limit)
      return idxDiff;

    int32_t valDiff = diffNodes(lhs->val(), rhs->val());

    if (valDiff > limit)
      return valDiff;

    int32_t contDiff = diffNodes(lhs->container(), rhs->container());

    if (contDiff > limit)
      return contDiff;

    int32_t bodyDiff = diffNodes(lhs->body(), rhs->body());

    if (bodyDiff > limit)
      return bodyDiff;

    return idxDiff + valDiff + contDiff + bodyDiff;
  }

  int32_t diffSwitch(const SwitchStatement *lhs, const SwitchStatement *rhs) {
    int32_t exprDiff = diffNodes(lhs->expression(), rhs->expression());

    if (exprDiff > limit)
      return exprDiff;

    const auto &casesLeft = lhs->cases();
    const auto &casesRight = rhs->cases();

    if (casesLeft.size() != casesRight.size())
      return sizeDiff(casesLeft.size(), casesRight.size());

    bool leftHasDefault = lhs->defaultCase().stmt != nullptr;
    bool rightHasDefault = rhs->defaultCase().stmt != nullptr;

    if (leftHasDefault != rightHasDefault) // shortcut
      return DiffCosts.NullDiffCost;

    int32_t casesDiff = 0;

    for (int32_t i = 0; i < int32_t(casesLeft.size()); ++i) {
      const auto &l = casesLeft[i];
      const auto &r = casesRight[i];

      int32_t valDiff = diffNodes(l.val, r.val);

      if (valDiff > limit)
        return valDiff;

      int32_t bodyDiff = diffNodes(l.stmt, r.stmt);

      if (bodyDiff > limit)
        return bodyDiff;

      casesDiff += (valDiff + bodyDiff);
    }

    int32_t defaultDiff = diffNodes(lhs->defaultCase().stmt, rhs->defaultCase().stmt);
    if (defaultDiff > limit)
      return defaultDiff;

    return casesDiff + defaultDiff + exprDiff;
  }

  int32_t diffTerminator(const TerminateStatement *lhs, const TerminateStatement *rhs) {
    assert(lhs->op() == rhs->op());

    return diffNodes(lhs->argument(), rhs->argument());
  }

  int32_t diffTry(const TryStatement *lhs, const TryStatement *rhs) {
    int32_t tryDiff = diffNodes(lhs->tryStatement(), rhs->tryStatement());

    if (tryDiff > limit)
      return tryDiff;

    int32_t exIdDiff = diffNodes(lhs->exceptionId(), rhs->exceptionId());
    int32_t catchDiff = diffNodes(lhs->catchStatement(), rhs->catchStatement());

    if (catchDiff > limit)
      return catchDiff;

    return tryDiff + exIdDiff + catchDiff;
  }

  int32_t diffExprStmt(const ExprStatement *lhs, const ExprStatement *rhs) {
    return diffNodes(lhs->expression(), rhs->expression());
  }

  int32_t diffId(const Id *lhs, const Id *rhs) {
    return strcmp(lhs->name(), rhs->name()) != 0 ? DiffCosts.NameDiffCost : 0;
  }

  int32_t diffComma(const CommaExpr *lhs, const CommaExpr *rhs) {
    const auto &leftExpressions = lhs->expressions();
    const auto &rightExpression = rhs->expressions();

    if (leftExpressions.size() != rightExpression.size())
      return sizeDiff(leftExpressions.size(), rightExpression.size());

    int32_t result = 0;

    for (int32_t i = 0; i < leftExpressions.size(); ++i) {
      const Expr *l = leftExpressions[i];
      const Expr *r = rightExpression[i];

      int32_t tmp = diffNodes(l, r);

      if (tmp > limit)
        return tmp;

      result += tmp;
    }

    return result;
  }

  int32_t diffBinary(const BinExpr *lhs, const BinExpr *rhs) {
    assert(lhs->op() == rhs->op());

    int32_t diffLeft = diffNodes(lhs->lhs(), rhs->lhs());

    if (diffLeft > limit)
      return diffLeft;

    int32_t diffRight = diffNodes(lhs->rhs(), rhs->rhs());

    if (diffRight > limit)
      return diffRight;

    return diffLeft + diffRight;
  }

  int32_t diffUnary(const UnExpr *lhs, const UnExpr *rhs) {
    assert(lhs->op() == rhs->op());

    return diffNodes(lhs->argument(), rhs->argument());
  }

  int32_t diffLiterals(const LiteralExpr *lhs, const LiteralExpr *rhs) {
    if (lhs->kind() != rhs->kind())
      return DiffCosts.LiteralDiffCost;

    switch (lhs->kind()) {
    case LK_NULL: return 0;
    case LK_BOOL: return lhs->b() != rhs->b() ? DiffCosts.LiteralDiffCost : 0;
    case LK_INT: return lhs->i() != rhs->i() ? DiffCosts.LiteralDiffCost : 0;
    case LK_FLOAT: return lhs->f() != rhs->f() ? DiffCosts.LiteralDiffCost : 0;
    case LK_STRING: return strcmp(lhs->s(), rhs->s()) != 0 ? DiffCosts.LiteralDiffCost : 0;
    default: assert(0); return -1000;
    }
  }

  int32_t diffIncExpr(const IncExpr *lhs, const IncExpr *rhs) {
    int32_t result = 0;
    if (lhs->form() != rhs->form())
      result += DiffCosts.IncFormDiffCost;

    if (lhs->diff() != rhs->diff())
      result += DiffCosts.IncValDiffCost;

    int32_t argDiff = diffNodes(lhs->argument(), rhs->argument());

    if (argDiff > limit)
      return argDiff;

    return result + argDiff;
  }

  int32_t diffArrayExpr(const ArrayExpr *lhs, const ArrayExpr *rhs) {
    const auto &leftInits = lhs->initializers();
    const auto &rightInits = rhs->initializers();

    if (leftInits.size() != rightInits.size())
      return sizeDiff(leftInits.size(), rightInits.size());

    int32_t result = 0;

    for (int32_t i = 0; i < leftInits.size(); ++i) {
      const Expr *l = leftInits[i];
      const Expr *r = rightInits[i];

      int32_t tmp = diffNodes(l, r);

      if (tmp > limit)
        return tmp;

      result += tmp;
    }

    return result;
  }

  int32_t diffGetField(const GetFieldExpr *lhs, const GetFieldExpr *rhs) {
    int32_t receiverDiff = diffNodes(lhs->receiver(), rhs->receiver());

    if (receiverDiff > limit)
      return receiverDiff;

    if (strcmp(lhs->fieldName(), rhs->fieldName()) != 0)
      receiverDiff += DiffCosts.NameDiffCost;

    if (lhs->isNullable() != rhs->isNullable())
      receiverDiff += DiffCosts.NullabilityDiffCost;

    return receiverDiff;
  }

  int32_t diffGetSlot(const GetSlotExpr *lhs, const GetSlotExpr *rhs) {
    int32_t receiverDiff = diffNodes(lhs->receiver(), rhs->receiver());

    if (receiverDiff > limit)
      return receiverDiff;

    int32_t keyDiff = diffNodes(lhs->key(), rhs->key());

    if (keyDiff > limit)
      return keyDiff;

    if (lhs->isNullable() != rhs->isNullable())
      receiverDiff += DiffCosts.NullabilityDiffCost;

    return receiverDiff + keyDiff;
  }

  int32_t diffCallExpr(const CallExpr *lhs, const CallExpr *rhs) {
    auto &leftArgs = lhs->arguments();
    auto &rightArgs = rhs->arguments();

    if (leftArgs.size() != rightArgs.size())
      return sizeDiff(leftArgs.size(), rightArgs.size());

    int32_t calleeDiff = diffNodes(lhs->callee(), rhs->callee());

    if (calleeDiff > limit)
      return calleeDiff;

    int32_t argsDiff = 0;

    for (int32_t i = 0; i < leftArgs.size(); ++i) {
      const Expr *l = leftArgs[i];
      const Expr *r = rightArgs[i];

      int32_t tmp = diffNodes(l, r);

      if (tmp > limit)
        return tmp;

      argsDiff += tmp;
    }

    if (lhs->isNullable() != rhs->isNullable())
      calleeDiff += DiffCosts.NullabilityDiffCost;

    return argsDiff + calleeDiff;
  }

  int32_t diffTernary(const TerExpr *lhs, const TerExpr *rhs) {
    int32_t condDiff = diffNodes(lhs->a(), rhs->a());

    if (condDiff > limit)
      return condDiff;

    int32_t tDiff = diffNodes(lhs->b(), rhs->b());

    if (tDiff > limit)
      return tDiff;

    int32_t eDiff = diffNodes(lhs->c(), rhs->c());

    if (eDiff > limit)
      return eDiff;

    return condDiff + tDiff + eDiff;
  }

  int32_t diffValueDecl(const ValueDecl *lhs, const ValueDecl *rhs) {
    int32_t result = strcmp(lhs->name(), rhs->name()) != 0 ? DiffCosts.NameDiffCost : 0;

    result += diffNodes(lhs->expression(), rhs->expression());

    return result;
  }

  int32_t diffConst(const ConstDecl *lhs, const ConstDecl *rhs) {
    int32_t nameDiff = strcmp(lhs->name(), rhs->name()) != 0 ? DiffCosts.NameDiffCost : 0;
    int32_t valueDiff = diffNodes(lhs->value(), rhs->value());

    return nameDiff + valueDiff;
  }

  int32_t diffVarDecl(const VarDecl *lhs, const VarDecl *rhs) {
    int32_t diffV = diffValueDecl(lhs, rhs);

    if (lhs->isAssignable() != rhs->isAssignable())
      diffV += DiffCosts.MutabilityDiffCost;

    return diffV;
  }

  int32_t diffDeclGroup(const DeclGroup *lhs, const DeclGroup *rhs) {
    const auto &leftGroup = lhs->declarations();
    const auto &rightGroup = rhs->declarations();

    if (leftGroup.size() != rightGroup.size())
      return sizeDiff(leftGroup.size(), rightGroup.size());

    int32_t result = 0;

    for (int32_t i = 0; i < leftGroup.size(); ++i) {
      const VarDecl *l = leftGroup[i];
      const VarDecl *r = rightGroup[i];

      int32_t tmp = diffVarDecl(l, r);

      if (tmp > limit)
        return tmp;

      result += tmp;
    }

    return result;
  }

  int32_t diffDestructDecl(const DestructuringDecl *lhs, const DestructuringDecl *rhs) {
    int32_t valueDiff = diffNodes(lhs->initExpression(), rhs->initExpression());

    if (valueDiff > limit)
      return valueDiff;

    int32_t groupDiff = diffDeclGroup(lhs, rhs);

    if (groupDiff > limit)
      return groupDiff;

    return valueDiff + groupDiff;
  }

  const char *realFunctionName(const FunctionExpr *f) const {
    const char *name = f->name();
    assert(name);

    if (name[0] == '(') // anonymous
      return "";

    return name;
  }

  int32_t diffDirective(const DirectiveStmt *lhs, const DirectiveStmt *rhs) {
    if (lhs->setFlags == rhs->setFlags && lhs->clearFlags == rhs->clearFlags && lhs->applyToDefault == rhs->applyToDefault)
      return 0;
    else
      return DiffCosts.OpDiffCost;
  }

  int32_t diffFunction(const FunctionExpr *lhs, const FunctionExpr *rhs) {
    int32_t nameDiff = strcmp(realFunctionName(lhs), realFunctionName(rhs)) != 0 ? DiffCosts.NameDiffCost : 0;

    const auto &leftParams = lhs->parameters();
    const auto &rightParams = rhs->parameters();

    if (leftParams.size() != rightParams.size())
      return sizeDiff(leftParams.size(), rightParams.size());

    int32_t paramDiff = 0;

    for (int32_t i = 0; i < leftParams.size(); ++i) {
      const ParamDecl *l = leftParams[i];
      const ParamDecl *r = rightParams[i];

      int32_t tmp = diffValueDecl(l, r);

      if (tmp > limit)
        return tmp;

      paramDiff += tmp;
    }

    int32_t bodyDiff = diffNodes(lhs->body(), rhs->body());

    if (bodyDiff > limit)
      return bodyDiff;

    return nameDiff + paramDiff + bodyDiff;
  }

  int32_t diffTable(const TableExpr *lhs, const TableExpr *rhs) {
    const auto &leftMembers = lhs->members();
    const auto &rightMembers = rhs->members();

    if (leftMembers.size() != rightMembers.size())
      return sizeDiff(leftMembers.size(), rightMembers.size());

    int32_t tableDiff = 0;

    for (int32_t i = 0; i < leftMembers.size(); ++i) {
      const auto &l = leftMembers[i];
      const auto &r = rightMembers[i];

      int32_t keyDiff = diffNodes(l.key, r.key);

      if (keyDiff > limit)
        return keyDiff;

      int32_t valueDiff = diffNodes(l.value, r.value);

      if (valueDiff > limit)
        return valueDiff;

      tableDiff += (keyDiff + valueDiff);

      if (l.isStatic() != r.isStatic())
        tableDiff += DiffCosts.StaticMemberDiffCost;
    }

    return tableDiff;
  }

  int32_t diffClass(const ClassExpr *lhs, const ClassExpr *rhs) {
    int32_t keyDiff = diffNodes(lhs->classKey(), rhs->classKey());

    if (keyDiff > limit)
      return keyDiff;

    int32_t baseDiff = diffNodes(lhs->classBase(), rhs->classBase());

    if (baseDiff > limit)
      return baseDiff;

    int32_t bodyDiff = diffTable(lhs, rhs);

    if (bodyDiff > limit)
      return bodyDiff;

    return keyDiff + baseDiff + bodyDiff;
  }

  int32_t diffEnumDecl(const EnumDecl *lhs, const EnumDecl *rhs) {
    int32_t nameDiff = strcmp(lhs->name(), rhs->name()) != 0 ? DiffCosts.NameDiffCost : 0;

    const auto &leftValues = lhs->consts();
    const auto &rightValues = rhs->consts();

    if (leftValues.size() != rightValues.size())
      return sizeDiff(leftValues.size(), rightValues.size());

    int32_t valueDiff = 0;

    for (int32_t i = 0; i < leftValues.size(); ++i) {
      const auto &l = leftValues[i];
      const auto &r = rightValues[i];

      int32_t cNameDiff = strcmp(l.id, r.id) != 0 ? DiffCosts.NameDiffCost : 0;
      int32_t cValueDiff = diffLiterals(l.val, r.val);

      valueDiff += (cNameDiff + cValueDiff);
    }

    return nameDiff + valueDiff;
  }

  int32_t diffNodes(const Node *lhs, const Node *rhs) {
    if (lhs == rhs)
      return 0;

    if (!lhs || !rhs) {
      return DiffCosts.NullDiffCost;
    }

    if (lhs->op() != rhs->op()) {
      return DiffCosts.OpDiffCost;
    }

    switch (lhs->op())
    {
    case TO_BLOCK:      return diffBlock((const Block *)lhs, (const Block *)rhs);
    case TO_IF:         return diffIf((const IfStatement *)lhs, (const IfStatement *)rhs);
    case TO_WHILE:      return diffWhile((const WhileStatement *)lhs, (const WhileStatement *)rhs);
    case TO_DOWHILE:    return diffDoWhile((const DoWhileStatement *)lhs, (const DoWhileStatement *)rhs);
    case TO_FOR:        return diffFor((const ForStatement *)lhs, (const ForStatement *)rhs);
    case TO_FOREACH:    return diffForeach((const ForeachStatement *)lhs, (const ForeachStatement *)rhs);
    case TO_SWITCH:     return diffSwitch((const SwitchStatement *)lhs, (const SwitchStatement *)rhs);
    case TO_RETURN:
    case TO_YIELD:
    case TO_THROW:
      return diffTerminator((const TerminateStatement *)lhs, (const TerminateStatement *)rhs);
    case TO_TRY:
      return diffTry((const TryStatement *)lhs, (const TryStatement *)rhs);
    case TO_BREAK:
    case TO_CONTINUE:
    case TO_EMPTY:
    case TO_BASE:
    case TO_ROOT_TABLE_ACCESS:
      return 0;
    case TO_DIRECTIVE:
      return diffDirective((const DirectiveStmt*)lhs, (const DirectiveStmt*)rhs);
    case TO_EXPR_STMT:
      return diffExprStmt((const ExprStatement *)lhs, (const ExprStatement *)rhs);

      //case TO_STATEMENT_MARK:
    case TO_ID:         return diffId((const Id *)lhs, (const Id *)rhs);
    case TO_COMMA:      return diffComma((const CommaExpr *)lhs, (const CommaExpr *)rhs);
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
      return diffBinary((const BinExpr *)lhs, (const BinExpr *)rhs);
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
      return diffUnary((const UnExpr *)lhs, (const UnExpr *)rhs);
    case TO_LITERAL:
      return diffLiterals((const LiteralExpr *)lhs, (const LiteralExpr *)rhs);
    case TO_INC:
      return diffIncExpr((const IncExpr *)lhs, (const IncExpr *)rhs);
    case TO_TABLE:
      return diffTable((const TableExpr *)lhs, (const TableExpr *)rhs);
    case TO_CLASS:
      return diffClass((const ClassExpr *)lhs, (const ClassExpr *)rhs);
    case TO_FUNCTION:
      return diffFunction((const FunctionExpr *)lhs, (const FunctionExpr *)rhs);
    case TO_ARRAY:
      return diffArrayExpr((const ArrayExpr *)lhs, (const ArrayExpr *)rhs);
    case TO_GETFIELD:
      return diffGetField((const GetFieldExpr *)lhs, (const GetFieldExpr *)rhs);
    case TO_GETSLOT:
      return diffGetSlot((const GetSlotExpr *)lhs, (const GetSlotExpr *)rhs);
    case TO_CALL:
      return diffCallExpr((const CallExpr *)lhs, (const CallExpr *)rhs);
    case TO_TERNARY:
      return diffTernary((const TerExpr *)lhs, (const TerExpr *)rhs);
      //case TO_EXPR_MARK:
    case TO_VAR:
      return diffVarDecl((const VarDecl *)lhs, (const VarDecl *)rhs);
    case TO_PARAM:
      return diffValueDecl((const ValueDecl *)lhs, (const ValueDecl *)rhs);
    case TO_CONST:
      return diffConst((const ConstDecl *)lhs, (const ConstDecl *)rhs);
    case TO_DECL_GROUP:
      return diffDeclGroup((const DeclGroup *)lhs, (const DeclGroup *)rhs);
    case TO_DESTRUCTURE:
      return diffDestructDecl((const DestructuringDecl *)lhs, (const DestructuringDecl *)rhs);
    case TO_ENUM:
      return diffEnumDecl((const EnumDecl *)lhs, (const EnumDecl *)rhs);
    case TO_SETFIELD:
    case TO_SETSLOT:
    default:
      assert(0);
      return -1;
    }
  }

public:
  static int32_t compute(const Node *lhs, const Node *rhs, int32_t limit) {
    NodeDiffComputer c(limit);
    return c.diffNodes(lhs, rhs);
  }
};

}
