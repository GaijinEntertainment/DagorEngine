#include "analyser.h"
#include <stdarg.h>
#include <cctype>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace SQCompilation {

const Expr *deparen(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_PAREN)
    return deparen(static_cast<const UnExpr *>(e)->argument());
  return e;
}

const Expr *skipUnary(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_INC) {
    return skipUnary(static_cast<const IncExpr *>(e)->argument());
  }

  if (TO_NOT <= e->op() && e->op() <= TO_DELETE) {
    return skipUnary(static_cast<const UnExpr *>(e)->argument());
  }

  return e;
}

static const Statement *lastNonEmpty(const Block *b, int32_t &effectiveSize) {
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

static const Statement *unwrapBody(const Statement *stmt) {
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
static const Statement *unwrapBodyNonEmpty(const Statement *stmt) {

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

static const Statement *unwrapSingleBlock(const Statement *stmt) {
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

static Expr *unwrapExprStatement(Statement *stmt) {
  return stmt->op() == TO_EXPR_STMT ? static_cast<ExprStatement *>(stmt)->expression() : nullptr;
}

static const SQChar *enumFqn(Arena *arena, const SQChar *enumName, const SQChar *cname) {
  int32_t l1 = strlen(enumName);
  int32_t l2 = strlen(cname);
  int32_t l = l1 + 1 + l2 + 1;
  SQChar *r = (SQChar *)arena->allocate(l);
  snprintf(r, l, "%s.%s", enumName, cname);
  return r;
}

static int32_t strhash(const SQChar *s) {
  int32_t r = 0;
  while (*s) {
    r *= 31;
    r += *s;
    ++s;
  }

  return r;
}

struct StringHasher {
  int32_t operator()(const SQChar *s) const {
    return strhash(s);
  }
};

struct StringEqualer {
  int32_t operator()(const SQChar *a, const SQChar *b) const {
    return strcmp(a, b) == 0;
  }
};

StaticAnalyser::StaticAnalyser(SQCompilationContext &ctx)
  : _ctx(ctx) {

}

class NodeEqualChecker {

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
    return strcmp(l->id(), r->id()) == 0;
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

  bool cmpDeclExpr(const DeclExpr *l, const DeclExpr *r) const {
    return check(l->declaration(), r->declaration());
  }

  bool cmpCallExpr(const CallExpr *l, const CallExpr *r) const {
    if (l->isNullable() != r->isNullable())
      return false;

    if (!check(l->callee(), r->callee()))
      return false;

    return cmpNodeVector(l->arguments(), r->arguments());
  }

  bool cmpArrayExpr(const ArrayExpr *l, const ArrayExpr *r) const {
    return cmpNodeVector(l->initialziers(), r->initialziers());
  }

  bool cmpGetField(const GetFieldExpr *l, const GetFieldExpr *r) const {
    if (l->isNullable() != r->isNullable())
      return false;

    if (strcmp(l->fieldName(), r->fieldName()) != 0)
      return false;

    return check(l->receiver(), r->receiver());
  }

  bool cmpGetTable(const GetTableExpr *l, const GetTableExpr *r) const {
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

    return cmpLiterals(l->value(), r->value());
  }

  bool cmpDeclGroup(const DeclGroup *l, const DeclGroup *r) const {
    return cmpNodeVector(l->declarations(), r->declarations());
  }

  bool cmpDestructDecl(const DestructuringDecl *l, const DestructuringDecl *r) const {
    return l->type() == r->type() && check(l->initiExpression(), r->initiExpression());
  }

  bool cmpFunction(const FunctionDecl *l, const FunctionDecl *r) const {
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

  bool cmpTable(const TableDecl *l, const TableDecl *r) const {
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

  bool cmpClass(const ClassDecl *l, const ClassDecl *r) const {
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
    case TO_ROOT:
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
    case TO_INEXPR_ASSIGN:
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
    case TO_RESUME:
    case TO_CLONE:
    case TO_PAREN:
    case TO_DELETE:
      return cmpUnary((const UnExpr *)lhs, (const UnExpr *)rhs);
    case TO_LITERAL:
      return cmpLiterals((const LiteralExpr *)lhs, (const LiteralExpr *)rhs);
    case TO_INC:
      return cmpIncExpr((const IncExpr *)lhs, (const IncExpr *)rhs);
    case TO_DECL_EXPR:
      return cmpDeclExpr((const DeclExpr *)lhs, (const DeclExpr *)rhs);
    case TO_ARRAYEXPR:
      return cmpArrayExpr((const ArrayExpr *)lhs, (const ArrayExpr *)rhs);
    case TO_GETFIELD:
      return cmpGetField((const GetFieldExpr *)lhs, (const GetFieldExpr *)rhs);
    case TO_GETTABLE:
      return cmpGetTable((const GetTableExpr *)lhs, (const GetTableExpr *)rhs);
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
    case TO_DESTRUCT:
      return cmpDestructDecl((const DestructuringDecl *)lhs, (const DestructuringDecl *)rhs);
    case TO_FUNCTION:
    case TO_CONSTRUCTOR:
      return cmpFunction((const FunctionDecl *)lhs, (const FunctionDecl *)rhs);
    case TO_CLASS:
      return cmpClass((const ClassDecl *)lhs, (const ClassDecl *)rhs);
    case TO_ENUM:
      return cmpEnumDecl((const EnumDecl *)lhs, (const EnumDecl *)rhs);
    case TO_TABLE:
      return cmpTable((const TableDecl *)lhs, (const TableDecl *)rhs);
    case TO_SETFIELD:
    case TO_SETTABLE:
    default:
      assert(0);
      return false;
    }
  }
};

enum ReturnTypeBits
{
  RT_NOTHING = 1 << 0,
  RT_NULL = 1 << 1,
  RT_BOOL = 1 << 2,
  RT_NUMBER = 1 << 3,
  RT_STRING = 1 << 4,
  RT_TABLE = 1 << 5,
  RT_ARRAY = 1 << 6,
  RT_CLOSURE = 1 << 7,
  RT_FUNCTION_CALL = 1 << 8,
  RT_UNRECOGNIZED = 1 << 9,
  RT_THROW = 1 << 10,
  RT_CLASS = 1 << 11,
};

class FunctionReturnTypeEvaluator {

  void checkLiteral(const LiteralExpr *l);
  void checkDeclaration(const DeclExpr *de);
  void checkAddExpr(const BinExpr *expr);

  bool checkNode(const Statement *node);

  bool checkBlock(const Block *b);
  bool checkIf(const IfStatement *stmt);
  bool checkLoop(const LoopStatement *loop);
  bool checkSwitch(const SwitchStatement *swtch);
  bool checkReturn(const ReturnStatement *ret);
  bool checkTry(const TryStatement *trstmt);
  bool checkThrow(const ThrowStatement *thrw);

  const Statement *unwrapLastStatement(const Statement *stmt) {
    if (stmt->op() == TO_BLOCK) {
      const Block *b = stmt->asBlock();
      auto &stmts = b->statements();
      return stmts.empty() ? nullptr : unwrapLastStatement(stmts.back());
    }

    return stmt;
  }

public:

  unsigned flags;

  unsigned compute(const Statement *n, bool &r) {
    flags = 0;
    r = checkNode(n);
    if (!r) {
      flags |= RT_NOTHING;
    }

    return flags;
  }
};

bool FunctionReturnTypeEvaluator::checkNode(const Statement *n) {
  switch (n->op())
  {
  case TO_RETURN: return checkReturn(static_cast<const ReturnStatement *>(n));
  case TO_THROW: return checkThrow(static_cast<const ThrowStatement *>(n));
  case TO_FOR: case TO_FOREACH: case TO_WHILE: case TO_DOWHILE:
    return checkLoop(static_cast<const LoopStatement *>(n));
  case TO_IF: return checkIf(static_cast<const IfStatement *>(n));
  case TO_SWITCH: return checkSwitch(static_cast<const SwitchStatement *>(n));
  case TO_BLOCK: return checkBlock(static_cast<const Block *>(n));
  case TO_TRY: return checkTry(static_cast<const TryStatement *>(n));
  default:
    return false;
  }
}

void FunctionReturnTypeEvaluator::checkLiteral(const LiteralExpr *lit) {
  switch (lit->kind())
  {
  case LK_STRING: flags |= RT_STRING; break;
  case LK_NULL: flags |= RT_NULL; break;
  case LK_BOOL: flags |= RT_BOOL; break;
  default: flags |= RT_NUMBER; break;
  }
}

void FunctionReturnTypeEvaluator::checkAddExpr(const BinExpr *expr) {
  assert(expr->op() == TO_ADD);

  unsigned saved = flags;

  flags = 0;

  const Expr *lhs = deparen(expr->lhs());
  const Expr *rhs = deparen(expr->rhs());

  if (lhs->op() == TO_ADD) { // -V522
    checkAddExpr(lhs->asBinExpr());
  }

  if (rhs->op() == TO_ADD) { // -V522
    checkAddExpr(rhs->asBinExpr());
  }

  if (lhs->op() == TO_LITERAL) { // -V522
    checkLiteral(lhs->asLiteral());
  }

  if (rhs->op() == TO_LITERAL) { // -V522
    checkLiteral(rhs->asLiteral());
  }

  if (flags & RT_STRING) {
    // concat with string is casted to string
    flags &= ~(RT_NUMBER | RT_BOOL | RT_NULL);
  }
  else {
    flags |= RT_NUMBER;
  }

  flags |= saved;
}

void FunctionReturnTypeEvaluator::checkDeclaration(const DeclExpr *de) {
  const Decl *decl = de->declaration();

  switch (decl->op())
  {
  case TO_CLASS: flags |= RT_CLASS; break;
  case TO_FUNCTION: flags |= RT_CLOSURE; break;
  case TO_TABLE: flags |= RT_TABLE; break;
  default:
    break;
  }
}

bool FunctionReturnTypeEvaluator::checkReturn(const ReturnStatement *ret) {

  const Expr *arg = deparen(ret->argument());

  if (arg == nullptr) {
    flags |= RT_NOTHING;
    return true;
  }

  switch (arg->op())
  {
  case TO_LITERAL:
    checkLiteral(static_cast<const LiteralExpr *>(arg));
    break;
  case TO_OROR:
  case TO_ANDAND:
  case TO_NE:
  case TO_EQ:
  case TO_GE:
  case TO_GT:
  case TO_LE:
  case TO_LT:
  case TO_INSTANCEOF:
  case TO_IN:
  case TO_NOT:
    flags |= RT_BOOL;
    break;
  case TO_ADD:
    checkAddExpr(arg->asBinExpr());
    break;
  case TO_SUB:
  case TO_MUL:
  case TO_DIV:
  case TO_MOD:
  case TO_NEG:
  case TO_BNOT:
  case TO_3CMP:
  case TO_AND:
  case TO_OR:
  case TO_XOR:
  case TO_SHL:
  case TO_SHR:
  case TO_USHR:
  case TO_INC:
    flags |= RT_NUMBER;
    break;
  case TO_CALL:
    flags |= RT_FUNCTION_CALL;
    break;
  case TO_DECL_EXPR:
    checkDeclaration(static_cast<const DeclExpr *>(arg));
    break;
  case TO_ARRAYEXPR:
    flags |= RT_ARRAY;
    break;
  default:
    flags |= RT_UNRECOGNIZED;
    break;
  }

  return true;
}

bool FunctionReturnTypeEvaluator::checkThrow(const ThrowStatement *thrw) {
  flags |= RT_THROW;
  return true;
}

bool FunctionReturnTypeEvaluator::checkIf(const IfStatement *ifStmt) {
  bool retThen = checkNode(ifStmt->thenBranch());
  bool retElse = false;
  if (ifStmt->elseBranch()) {
    retElse = checkNode(ifStmt->elseBranch());
  }

  return retThen && retElse;
}

bool FunctionReturnTypeEvaluator::checkLoop(const LoopStatement *loop) {
  checkNode(loop->body());
  return false;
}

bool FunctionReturnTypeEvaluator::checkBlock(const Block *block) {
  bool allReturns = false;

  for (const Statement *stmt : block->statements()) {
    if (stmt->op() == TO_EMPTY)
      continue;
    allReturns = checkNode(stmt);
  }

  return allReturns;
}

bool FunctionReturnTypeEvaluator::checkSwitch(const SwitchStatement *swtch) {
  bool allReturns = true;

  for (auto &c : swtch->cases()) {
    bool caseR = checkNode(c.stmt);;

    if (!caseR) {
      const Statement *last = unwrapLastStatement(c.stmt);
      if (!last || last->op() != TO_BREAK) // fall through
        continue;
    }
    allReturns &= caseR;
  }

  if (swtch->defaultCase().stmt) {
    allReturns &= checkNode(swtch->defaultCase().stmt);
  }
  else {
    allReturns = false;
  }

  return allReturns;
}

bool FunctionReturnTypeEvaluator::checkTry(const TryStatement *stmt) {
  bool retTry = checkNode(stmt->tryStatement());
  bool retCatch = checkNode(stmt->catchStatement());
  return retTry && retCatch;
}

class PredicateCheckerVisitor : public Visitor {
  bool deepCheck;
  bool result;
  const Node *checkee;
protected:
  NodeEqualChecker equalChecker;

  PredicateCheckerVisitor(bool deep) : deepCheck(deep), result(false), checkee(nullptr) {}

  virtual bool doCheck(const Node *checkee, Node *n) const = 0;

public:
  void visitNode(Node *n) {
    if (doCheck(checkee, n)) {
      result = true;
      return;
    }

    if (deepCheck)
      n->visitChildren(this);
  }

  bool check(const Node *toCheck, Node *tree) {
    result = false;
    checkee = toCheck;
    tree->visit(this); // -V522
    return result;
  }
};

class CheckModificationVisitor : public PredicateCheckerVisitor {
protected:
  bool doCheck(const Node *checkee, Node *n) const {
    enum TreeOp op = n->op();

    if (op == TO_ASSIGN || op == TO_INEXPR_ASSIGN || (TO_PLUSEQ <= op && op <= TO_MODEQ)) {
      BinExpr *bin = static_cast<BinExpr *>(n);
      return equalChecker.check(checkee, bin->lhs());
    }

    if (op == TO_INC) {
      IncExpr *inc = static_cast<IncExpr *>(n);
      return equalChecker.check(checkee, inc->argument());
    }

    return false;
  }
public:
  CheckModificationVisitor() : PredicateCheckerVisitor(false) {}
};

class ExistsChecker : public PredicateCheckerVisitor {
protected:

  bool doCheck(const Node *checkee, Node *n) const {
    return equalChecker.check(checkee, n);
  }

public:

  ExistsChecker() : PredicateCheckerVisitor(true) {}
};

class ModificationChecker : public Visitor {
  bool result;
public:

  void visitNode(Node *n) {
    enum TreeOp op = n->op();

    switch (op)
    {
    case TO_INC:
    case TO_ASSIGN:
    case TO_INEXPR_ASSIGN:
    case TO_PLUSEQ:
    case TO_MINUSEQ:
    case TO_MULEQ:
    case TO_DIVEQ:
    case TO_MODEQ:
      result = true;
      return;
    default:
      n->visitChildren(this);
      break;
    }
  }

  bool check(Node *n) {
    result = false;
    n->visit(this);
    return result;
  }
};

static bool isBinaryArith(const Expr *expr) {
  return TO_OROR <= expr->op() && expr->op() <= TO_SUB;
}

static bool isAssignOp(const TreeOp op) {
  return op == TO_ASSIGN || op == TO_INEXPR_ASSIGN || (TO_PLUSEQ <= op && op <= TO_MODEQ);
}

static bool isAssignExpr(const Expr *expr) {
  return isAssignOp(expr->op());
}

class LoopTerminatorCollector : public Visitor {
  bool _firstLevel; // means not under some condition, if or switch
  bool _inSwitch;
  bool _inTry;

  void setTerminator(const Statement *t) {
    if (terminator == nullptr)
      terminator = t;
  }

public:

  bool hasCondBreak;
  bool hasCondContinue;
  bool hasCondReturn;
  bool hasCondThrow;

  bool hasUnconditionalTerm;
  bool hasUnconditionalContinue;

  const Statement *terminator;

  LoopTerminatorCollector(bool firstLevel)
    : _firstLevel(firstLevel)
    , _inSwitch(false)
    , _inTry(false)
    , hasCondBreak(false)
    , hasCondContinue(false)
    , hasCondReturn(false)
    , hasCondThrow(false)
    , hasUnconditionalTerm(false)
    , hasUnconditionalContinue(false)
    , terminator(nullptr) {}

  void visitReturnStatement(ReturnStatement *stmt) {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if (!hasUnconditionalTerm)
      hasCondReturn = true;
  }

  void visitThrowStatement(ThrowStatement *stmt) {
    if (_firstLevel && !_inTry) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if(!hasUnconditionalTerm)
      hasCondThrow = true;
  }

  void visitBreakStatement(BreakStatement *stmt) {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if (!_inSwitch && !hasUnconditionalTerm)
      hasCondBreak = true;
  }

  void visitContinueStatement(ContinueStatement *stmt) {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      hasUnconditionalContinue = true;
      setTerminator(stmt);
    }
    else if (!hasUnconditionalTerm)
      hasCondContinue = true;
  }

  void visitIfStatement(IfStatement *stmt) {
    bool old = _firstLevel;
    _firstLevel = false;
    Visitor::visitIfStatement(stmt);
    _firstLevel = old;
  }

  void visitLoopStatement(LoopStatement *loop) {
    // skip
  }

  void visitDecl(Decl *d) {
    // skip
  }

  void visitTryStatement(TryStatement *stmt) {
    bool old = _inTry;
    _inTry = true;
    stmt->tryStatement()->visit(this);
    _inTry = old;
    stmt->catchStatement()->visit(this);
  }

  void visitSwitchStatement(SwitchStatement *stmt) {
    bool oldL = _firstLevel;
    bool oldS = _inSwitch;
    _firstLevel = false;
    _inSwitch = true;
    Visitor::visitSwitchStatement(stmt);
    _inSwitch = oldS;
    _firstLevel = oldL;
  }

  bool hasUnconditionalTerminator() const {
    return (hasUnconditionalTerm && !hasCondContinue) || hasUnconditionalContinue;
  }
};


class AssignSeqTerminatorFinder : public Visitor {

  const Expr *assigne;
  bool foundUsage;
  bool foundInteruptor;

  NodeEqualChecker eqChecker;

public:
  AssignSeqTerminatorFinder(const Expr *asg) : assigne(asg), foundUsage(false), foundInteruptor(false), eqChecker() {}

  void visitNode(Node *n) {
    if (!foundInteruptor && !foundUsage)
      Visitor::visitNode(n);
  }

  void visitCallExpr(CallExpr *c) {
    foundInteruptor = true; // consider call as potenrial usage
  }

  void visitExpr(Expr *e) {
    Visitor::visitExpr(e);

    if (eqChecker.check(assigne, e))
      foundUsage = true;
  }

  void visitFunctionDecl(FunctionDecl *e) { /* skip */ }

  bool check(Node *tree) {

    tree->visit(this);

    return foundUsage || foundInteruptor;
  }
};

static bool terminateAssignSequence(const Expr *assignee, Node *tree) {
  AssignSeqTerminatorFinder finder(assignee);

  return finder.check(tree);
}


static bool isSuspiciousNeighborOfNullCoalescing(enum TreeOp op) {
  return (op == TO_3CMP || op == TO_ANDAND || op == TO_OROR || op == TO_IN || /*op == TO_NOTIN ||*/ op == TO_EQ || op == TO_NE || op == TO_LE ||
    op == TO_LT || op == TO_GT || op == TO_GE /*|| op == TO_NOT*/ || op == TO_BNOT || op == TO_AND || op == TO_OR ||
    op == TO_XOR || op == TO_DIV || op == TO_MOD || op == TO_INSTANCEOF || /*op == TO_QMARK ||*/ /*op == TO_NEG ||*/
    op == TO_ADD || op == TO_MUL || op == TO_SHL || op == TO_SHR || op == TO_USHR);
}

static bool isSuspiciousTernaryConditionOp(enum TreeOp op) {
  return op == TO_ADD || op == TO_SUB || op == TO_MUL || op == TO_DIV || op == TO_MOD ||
    op == TO_AND || op == TO_OR || op == TO_SHL || op == TO_SHR || op == TO_USHR || op == TO_3CMP || op == TO_NULLC;
}

static bool isSuspiciousSameOperandsBinaryOp(enum TreeOp op) {
  return op == TO_EQ || op == TO_LE || op == TO_LT || op == TO_GE || op == TO_GT || op == TO_NE ||
    op == TO_ANDAND || op == TO_OROR || op == TO_SUB || op == TO_3CMP || op == TO_DIV || op == TO_MOD ||
    op == TO_OR || op == TO_AND || op == TO_XOR || op == TO_SHL || op == TO_SHR || op == TO_USHR;
}

static bool isBlockTerminatorStatement(enum TreeOp op) {
  return op == TO_RETURN || op == TO_THROW || op == TO_BREAK || op == TO_CONTINUE;
}

static bool isBooleanResultOperator(enum TreeOp op) {
  return op == TO_OROR || op == TO_ANDAND || op == TO_NE || op == TO_EQ || (TO_GE <= op && op <= TO_IN) || op == TO_NOT;
}

static bool isArithOperator(enum TreeOp op) {
  return op == TO_OROR || op == TO_ANDAND
    || (TO_3CMP <= op && op <= TO_LT)
    || (TO_USHR <= op && op <= TO_SUB)
    || (TO_PLUSEQ <= op && op <= TO_MODEQ)
    || op == TO_BNOT || op == TO_NEG || op == TO_INC;
}

static bool isDivOperator(enum TreeOp op) {
  return op == TO_DIV || op == TO_MOD || op == TO_DIVEQ || op == TO_MODEQ;
}

bool isPureArithOperator(enum TreeOp op) {
  return TO_USHR <= op && op <= TO_SUB || TO_PLUSEQ <= op && op <= TO_MODEQ;
}

bool isRelationOperator(enum TreeOp op) {
  return TO_3CMP <= op && op <= TO_LT;
}

bool isBoolRelationOperator(enum TreeOp op) {
  return TO_GE <= op && op <= TO_LT;
}

bool isBitwiseOperator(enum TreeOp op) {
  return op == TO_OR || op == TO_AND || op == TO_XOR;
}

bool isBoolCompareOperator(enum TreeOp op) {
  return op == TO_NE || op == TO_EQ || isBoolRelationOperator(op);
}

bool isCompareOperator(enum TreeOp op) {
  return TO_NE <= op && op <= TO_LT;
}

bool isShiftOperator(enum TreeOp op) {
  return op == TO_SHL || op == TO_SHR || op == TO_USHR;
}

bool isHigherShiftPriority(enum TreeOp op) {
  return TO_MUL <= op && op <= TO_SUB;
}

bool looksLikeBooleanExpr(const Expr *e) {
  if (isBooleanResultOperator(e->op())) // -V522
    return true;

  if (e->op() == TO_LITERAL) { // -V522
    return e->asLiteral()->kind() == LK_BOOL; // -V522
  }

  if (e->op() == TO_NULLC) {
    // check for `x?.y ?? false`
    return looksLikeBooleanExpr(e->asBinExpr()->rhs());
  }

  return false;
}

static const char *terminatorOpToName(enum TreeOp op) {
  switch (op)
  {
  case TO_BREAK: return "break";
  case TO_CONTINUE: return "continue";
  case TO_RETURN: return "return";
  case TO_THROW: return "throw";
  default:
    assert(0);
    return "<unkown terminator>";
  }
}

static bool hasPrefix(const SQChar *str, const SQChar *prefix, unsigned &length) {
  unsigned i = 0;

  for (;;) {
    SQChar c = str[i];
    SQChar p = prefix[i];

    if (!p) {
      length = i;
      return true;
    }

    if (!c) {
      return false;
    }

    if (c != p)
      return false;

    ++i;
  }
}

static bool hasAnyPrefix(const SQChar *str, const std::vector<std::string> &prefixes) {
  for (auto &prefix : prefixes) {
    unsigned length = 0;
    if (hasPrefix(str, prefix.c_str(), length)) {
      SQChar c = str[length];
      if (!c || c == '_' || c != tolower(c)) {
        return true;
      }
    }
  }

  return false;
}

static bool nameLooksLikeResultMustBeBoolean(const SQChar *funcName) {
  if (!funcName)
    return false;

  return hasAnyPrefix(funcName, SQCompilationContext::function_should_return_bool_prefix);
}

static bool nameLooksLikeFunctionMustReturnResult(const SQChar *funcName) {
  if (!funcName)
    return false;

  bool nameInList = nameLooksLikeResultMustBeBoolean(funcName) ||
    hasAnyPrefix(funcName, SQCompilationContext::function_should_return_something_prefix);

  if (!nameInList)
    if ((strstr(funcName, "_ctor") || strstr(funcName, "Ctor")) && strstr(funcName, "set") != funcName)
      nameInList = true;

  return nameInList;
}

static bool nameLooksLikeResultMustBeUtilised(const SQChar *name) {
  return hasAnyPrefix(name, SQCompilationContext::function_result_must_be_utilized);
}

static bool nameLooksLikeResultMustBeString(const SQChar *name) {
  return hasAnyPrefix(name, SQCompilationContext::function_can_return_string);
}

static bool nameLooksLikeCallsLambdaInPlace(const SQChar *name) {
  return hasAnyPrefix(name, SQCompilationContext::function_calls_lambda_inplace);
}

static bool canFunctionReturnNull(const SQChar *n) {
  return hasAnyPrefix(n, SQCompilationContext::function_can_return_null);
}

static const SQChar rootName[] = "::";
static const SQChar baseName[] = "base";
static const SQChar thisName[] = "this";

enum ValueRefState {
  VRS_UNDEFINED,
  VRS_EXPRESSION,
  VRS_INITIALIZED,
  VRS_MULTIPLE,
  VRS_UNKNOWN,
  VRS_PARTIALLY,
  VRS_DECLARED,
  VRS_NUM_OF_STATES
};

enum SymbolKind {
  SK_EXCEPTION,
  SK_FUNCTION,
  SK_CLASS,
  SK_TABLE,
  SK_VAR,
  SK_BINDING,
  SK_CONST,
  SK_ENUM,
  SK_ENUM_CONST,
  SK_PARAM,
  SK_FOREACH
};

static const char *symbolContextName(enum SymbolKind k) {
  switch (k)
  {
  case SK_EXCEPTION: return "exception";
  case SK_FUNCTION: return "function";
  case SK_CLASS: return "class";
  case SK_TABLE: return "table";
  case SK_VAR: return "variable";
  case SK_BINDING: return "let";
  case SK_CONST: return "const";
  case SK_ENUM: return "enum";
  case SK_ENUM_CONST: return "enum const";
  case SK_PARAM: return "parameter";
  case SK_FOREACH: return "foreach var";
  default: return "<unknown>";
  }
}

enum ValueBoundKind {
  VBK_UNKNOWN,
  VBK_INTEGER,
  VRK_FLOAT
};

struct ValueBound {
  enum ValueBoundKind kind;

  union {
    SQInteger i;
    SQFloat f;
  } v;
};

struct FunctionInfo {

  FunctionInfo(const FunctionDecl *d, const FunctionDecl *o) : declaration(d), owner(o) {}

  ~FunctionInfo() = default;

  struct Modifiable {
    const FunctionDecl *owner;
    const SQChar *name;
  };

  const FunctionDecl *owner;
  std::vector<Modifiable> modifible;
  const FunctionDecl *declaration;
  std::vector<const SQChar *> parameters;

  void joinModifiable(const FunctionInfo *other);
  void addModifiable(const SQChar *name, const FunctionDecl *o);

};

void FunctionInfo::joinModifiable(const FunctionInfo *other) {
  for (auto &m : other->modifible) {
    if (owner == m.owner)
      continue;

    addModifiable(m.name, m.owner);
  }
}

void FunctionInfo::addModifiable(const SQChar *name, const FunctionDecl *o) {
  for (auto &m : modifible) {
    if (m.owner == o && strcmp(name, m.name) == 0)
      return;
  }

  modifible.push_back({ o, name });
}

struct VarScope;

static ValueRefState mergeMatrix[VRS_NUM_OF_STATES][VRS_NUM_OF_STATES] = {
 // VRS_UNDEFINED  VRS_EXPRESSION  VRS_INITIALIZED  VRS_MULTIPLE   VRS_UNKNOWN    VRS_PARTIALLY  VRS_DECLARED
  { VRS_UNDEFINED, VRS_PARTIALLY,  VRS_PARTIALLY,   VRS_PARTIALLY, VRS_PARTIALLY, VRS_PARTIALLY, VRS_PARTIALLY }, // VRS_UNDEFINED
  { VRS_PARTIALLY, VRS_EXPRESSION, VRS_MULTIPLE,    VRS_MULTIPLE,  VRS_MULTIPLE,  VRS_PARTIALLY, VRS_PARTIALLY }, // VRS_EXPRESSION
  { VRS_PARTIALLY, VRS_MULTIPLE,   VRS_INITIALIZED, VRS_MULTIPLE,  VRS_MULTIPLE,  VRS_PARTIALLY, VRS_PARTIALLY }, // VRS_INITIALIZED
  { VRS_PARTIALLY, VRS_MULTIPLE,   VRS_MULTIPLE,    VRS_MULTIPLE,  VRS_MULTIPLE,  VRS_PARTIALLY, VRS_MULTIPLE  }, // VRS_MULTIPLE
  { VRS_PARTIALLY, VRS_MULTIPLE,   VRS_UNKNOWN,     VRS_MULTIPLE,  VRS_UNKNOWN,   VRS_PARTIALLY, VRS_PARTIALLY }, // VRS_UNKNOWN
  { VRS_PARTIALLY, VRS_PARTIALLY,  VRS_PARTIALLY,   VRS_PARTIALLY, VRS_PARTIALLY, VRS_PARTIALLY, VRS_PARTIALLY }, // VRS_PARTIALLY
  { VRS_PARTIALLY, VRS_MULTIPLE,   VRS_INITIALIZED, VRS_MULTIPLE,  VRS_PARTIALLY, VRS_PARTIALLY, VRS_DECLARED  }  // VRS_DECLARED
};

struct SymbolInfo {
  union {
    const Id *x;
    const FunctionDecl *f;
    const ClassDecl *k;
    const TableMember *m;
    const VarDecl *v;
    const TableDecl *t;
    const ParamDecl *p;
    const EnumDecl *e;
    const ConstDecl *c;
    const EnumConst *ec;
  } declarator;

  enum SymbolKind kind;

  bool declared;
  bool used;
  bool usedAfterAssign;

  const struct VarScope *ownedScope;

  SymbolInfo(enum SymbolKind k) : kind(k), declarator() {
    declared = true;
    used = usedAfterAssign = false;
    ownedScope = nullptr;
  }

  bool isConstant() const {
    return kind != SK_VAR;
  }

  const Node *extractPointedNode() const {
    switch (kind)
    {
    case SK_EXCEPTION:
      return declarator.x;
    case SK_FUNCTION:
      return declarator.f;
    case SK_CLASS:
      return declarator.k;
    case SK_TABLE:
      return declarator.t;
    case SK_VAR:
    case SK_BINDING:
    case SK_FOREACH:
      return declarator.v;
    case SK_CONST:
      return declarator.c;
    case SK_ENUM:
      return declarator.e;
    case SK_ENUM_CONST:
      return declarator.ec->val;
    case SK_PARAM:
      return declarator.p;
    default:
      assert(0);
      return nullptr;
    }
  }

  const char *contextName() const {
    return symbolContextName(kind);
  }
};

struct ValueRef {

  SymbolInfo *info;
  enum ValueRefState state;
  const Expr *expression;

  /*
    used to track mixed assignments
    local x = 10 // eid = 1
    let y = x    // eid = 2
    x = 20       // eid = 3
  */

  int32_t evalIndex;

  ValueRef(SymbolInfo *i, int32_t eid) : info(i), evalIndex(eid), state(), expression(nullptr) {
    assigned = false;
    lastAssigneeScope = nullptr;
    lowerBound.kind = upperBound.kind = VBK_UNKNOWN;
    flagsPositive = flagsNegative = 0;
  }

  bool hasValue() const {
    return state == VRS_EXPRESSION || state == VRS_INITIALIZED;
  }

  bool isConstant() const {
    return info->isConstant();
  }

  bool assigned;
  const VarScope *lastAssigneeScope;

  ValueBound lowerBound, upperBound;
  unsigned flagsPositive, flagsNegative;

  void kill(enum ValueRefState k = VRS_UNKNOWN, bool clearFlags = true) {
    if (!isConstant()) {
      state = k;
      expression = nullptr;
    }
    if (clearFlags) {
      flagsPositive = 0;
      flagsNegative = 0;
    }
    lowerBound.kind = upperBound.kind = VBK_UNKNOWN;
  }

  void intersectValue(const ValueRef *other);

  void merge(const ValueRef *other);
};

class CheckerVisitor;

struct VarScope {

  VarScope(const FunctionDecl *o, struct VarScope *p = nullptr) : owner(o), parent(p), depth(p ? p->depth + 1 : 0), evalId(p ? p->evalId + 1 : 0) {}

  ~VarScope() {
    if (parent)
      parent->~VarScope();
    symbols.clear();
  }


  int32_t evalId;
  const int32_t depth;
  const FunctionDecl *owner;
  struct VarScope *parent;
  std::unordered_map<const SQChar *, ValueRef *, StringHasher, StringEqualer> symbols;

  void intersectScopes(const VarScope *other);

  void merge(const VarScope *other);
  VarScope *copy(Arena *a, bool forClosure = false) const;
  void copyFrom(const VarScope *other);

  void mergeUnbalanced(const VarScope *other);

  VarScope *findScope(const FunctionDecl *own);
  void checkUnusedSymbols(CheckerVisitor *v);
};

void ValueRef::intersectValue(const ValueRef *other) {
  assert(info == other->info);

  flagsPositive &= other->flagsPositive;
  flagsNegative &= other->flagsNegative;

  assert((flagsNegative & flagsPositive) == 0);

  assigned &= other->assigned;
  if (lastAssigneeScope) {
    if (other->lastAssigneeScope && other->lastAssigneeScope->depth > lastAssigneeScope->depth)
      lastAssigneeScope = other->lastAssigneeScope;
  }
  else {
    lastAssigneeScope = other->lastAssigneeScope;
  }

  // TODO: intersect bounds

  if (isConstant())
    return;

  if (!NodeEqualChecker().check(expression, other->expression)) {
    kill(VRS_MULTIPLE, false);
  }
}

void ValueRef::merge(const ValueRef *other) {

  assert(info == other->info);

  assigned &= other->assigned;
  if (lastAssigneeScope) {
    if (other->lastAssigneeScope && other->lastAssigneeScope->depth > lastAssigneeScope->depth)
      lastAssigneeScope = other->lastAssigneeScope;
  }
  else
  {
    lastAssigneeScope = other->lastAssigneeScope;
  }

  if (state != other->state) {
    enum ValueRefState k = mergeMatrix[other->state][state];
    kill(k);
    return;
  }

  if (isConstant()) {
    assert(other->isConstant());
    return;
  }

  if (!NodeEqualChecker().check(expression, other->expression)) {
    kill(VRS_MULTIPLE);
  }
}

void VarScope::copyFrom(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->owner == r->owner && "Scope corruption");

    auto &thisSymbols = l->symbols;
    auto &otherSymbols = r->symbols;
    auto it = otherSymbols.begin();
    auto ie = otherSymbols.end();

    while (it != ie) {
      thisSymbols[it->first] = it->second;
      ++it;
    }

    l = l->parent;
    r = r->parent;
  }
}

void VarScope::intersectScopes(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->owner == r->owner && "Scope corruption");

    auto &thisSymbols = l->symbols;
    auto &otherSymbols = r->symbols;
    auto it = otherSymbols.begin();
    auto ie = otherSymbols.end();
    auto te = thisSymbols.end();

    while (it != ie) {
      auto f = thisSymbols.find(it->first);
      if (f != te) {
        if (it->second->info == f->second->info)
          f->second->intersectValue(it->second);
      }
      ++it;
    }

    l = l->parent;
    r = r->parent;
  }
  evalId = std::max(evalId, other->evalId) + 1; // -V522
}


void VarScope::mergeUnbalanced(const VarScope *other) {
  VarScope *lhs = this;
  const VarScope *rhs = other;

  while (lhs->depth > rhs->depth) {
    lhs = lhs->parent;
  }

  while (rhs->depth > lhs->depth) {
    rhs = rhs->parent;
  }

  lhs->merge(rhs);
}

void VarScope::merge(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->depth == r->depth && "Scope corruption");
    assert(l->owner == r->owner && "Scope corruption");

    auto &thisSymbols = l->symbols;
    auto &otherSymbols = r->symbols;
    auto it = otherSymbols.begin();
    auto ie = otherSymbols.end();
    auto te = thisSymbols.end();

    while (it != ie) {
      auto f = thisSymbols.find(it->first);
      if (f != te) {
        if (it->second->info == f->second->info) // lambdas declared on the same line could have same names
          f->second->merge(it->second);
      }
      else {
        it->second->kill(VRS_PARTIALLY);
        thisSymbols[it->first] = it->second;
      }
      ++it;
    }

    l = l->parent;
    r = r->parent;
  }

  evalId = std::max(evalId, other->evalId) + 1;
}

VarScope *VarScope::findScope(const FunctionDecl *own) {
  VarScope *s = this;

  while (s) {
    if (s->owner == own) {
      return s;
    }
    s = s->parent;
  }

  return nullptr;
}

VarScope *VarScope::copy(Arena *a, bool forClosure) const {
  VarScope *parentCopy = parent ? parent->copy(a, forClosure) : nullptr;
  void *mem = a->allocate(sizeof(VarScope));
  VarScope *thisCopy = new(mem) VarScope(owner, parentCopy);

  for (auto &kv : symbols) {
    const SQChar *k = kv.first;
    ValueRef *v = kv.second;
    void *mem = a->allocate(sizeof(ValueRef));
    ValueRef *vcopy = new(mem) ValueRef(v->info, v->evalIndex);

    if (!v->isConstant() && forClosure) {
      // if we analyse closure we cannot rely on existed assignable values
      vcopy->state = VRS_UNKNOWN;
      vcopy->expression = nullptr;
      vcopy->flagsNegative = vcopy->flagsPositive = 0;
      vcopy->lowerBound.kind = vcopy->upperBound.kind = VBK_UNKNOWN;
      vcopy->assigned = v->assigned;
      vcopy->lastAssigneeScope = v->lastAssigneeScope;
    }
    else {
      memcpy(vcopy, v, sizeof(ValueRef));
      vcopy->assigned = false;
      vcopy->lastAssigneeScope = nullptr;
    }
    thisCopy->symbols[k] = vcopy;
  }

  return thisCopy;
}

enum BreakableScopeKind {
  BSK_LOOP,
  BSK_SWITCH
};

struct BreakableScope {
  enum BreakableScopeKind kind;

  struct BreakableScope *parent;

  union {
    const LoopStatement *loop;
    const SwitchStatement *swtch;
  } node;

  VarScope *loopScope;
  VarScope *exitScope;
  CheckerVisitor *visitor;

  BreakableScope(CheckerVisitor *v, const SwitchStatement *swtch);

  BreakableScope(CheckerVisitor *v, const LoopStatement *loop, VarScope *ls, VarScope *es);

  ~BreakableScope();

};

class CheckerVisitor : public Visitor {
  friend struct VarScope;
  friend struct BreakableScope;

  SQCompilationContext &_ctx;

  NodeEqualChecker _equalChecker;

  bool isUpperCaseIdentifier(const Expr *expr);

  void report(const Node *n, int32_t id, ...);

  void checkKeyNameMismatch(const Expr *key, const Expr *expr);

  void checkAlwaysTrueOrFalse(const Expr *expr);

  void checkForeachIteratorInClosure(const Id *id, const ValueRef *v);
  void checkIdUsed(const Id *id, const Node *p, ValueRef *v);

  void checkAndOrPriority(const BinExpr *expr);
  void checkBitwiseParenBool(const BinExpr *expr);
  void checkCoalescingPriority(const BinExpr *expr);
  void checkAssignmentToItself(const BinExpr *expr);
  void checkGlobalVarCreation(const BinExpr *expr);
  void checkSameOperands(const BinExpr *expr);
  void checkAlwaysTrueOrFalse(const BinExpr *expr);
  void checkDeclarationInArith(const BinExpr *expr);
  void checkIntDivByZero(const BinExpr *expr);
  void checkPotentiallyNullableOperands(const BinExpr *expr);
  void checkBitwiseToBool(const BinExpr *expr);
  void checkCompareWithBool(const BinExpr *expr);
  void checkRelativeCompareWithBool(const BinExpr *expr);
  void checkCopyOfExpression(const BinExpr *expr);
  void checkConstInBoolExpr(const BinExpr *expr);
  void checkShiftPriority(const BinExpr *expr);
  void checkCanReturnNull(const BinExpr *expr);
  void checkCompareWithContainer(const BinExpr *expr);
  void checkBoolToStrangePosition(const BinExpr *expr);
  void checkNewSlotNameMatch(const BinExpr *expr);
  void checkPlusString(const BinExpr *expr);
  void checkAlwaysTrueOrFalse(const TerExpr *expr);
  void checkTernaryPriority(const TerExpr *expr);
  void checkSameValues(const TerExpr *expr);
  void checkExtendToAppend(const CallExpr *callExpr);
  void checkAlreadyRequired(const CallExpr *callExpr);
  void checkCallNullable(const CallExpr *callExpr);
  void checkArguments(const CallExpr *callExpr);
  void checkBoolIndex(const GetTableExpr *expr);
  void checkNullableIndex(const GetTableExpr *expr);

  bool findIfWithTheSameCondition(const Expr * condition, const IfStatement * elseNode, const Expr *&duplicated) {
    if (_equalChecker.check(condition, elseNode->condition())) {
      duplicated = elseNode->condition();
      return true;
    }

    const Statement *elseB = unwrapSingleBlock(elseNode->elseBranch());

    if (elseB && elseB->op() == TO_IF) {
      return findIfWithTheSameCondition(condition, static_cast<const IfStatement *>(elseB), duplicated);
    }

    return false;
  }

  const SQChar *normalizeParamName(const SQChar *name, SQChar *buffer = nullptr);
  int32_t normalizeParamNameLength(const SQChar *n);

  void checkDuplicateSwitchCases(SwitchStatement *swtch);
  void checkDuplicateIfBranches(IfStatement *ifStmt);
  void checkDuplicateIfConditions(IfStatement *ifStmt);

  bool onlyEmptyStatements(int32_t start, const ArenaVector<Statement *> &statements) {
    for (int32_t i = start; i < statements.size(); ++i) {
      Statement *stmt = statements[i];
      if (stmt->op() != TO_EMPTY)
        return false;
    }

    return true;
  }

  bool existsInTree(const Expr *toSearch, Expr *tree) const {
    return ExistsChecker().check(toSearch, tree);
  }

  bool indexChangedInTree(Expr *index) const {
    return ModificationChecker().check(index);
  }

  bool checkModification(Expr *key, Node *tree) {
    return CheckModificationVisitor().check(key, tree);
  }

  bool isCallResultShouldBeUtilized(const SQChar *name, const CallExpr *call);

  void checkUnterminatedLoop(LoopStatement *loop);
  void checkVariableMismatchForLoop(ForStatement *loop);
  void checkEmptyWhileBody(WhileStatement *loop);
  void checkEmptyThenBody(IfStatement *stmt);
  void checkForgottenDo(const Block *block);
  void checkUnreachableCode(const Block *block);
  void checkAssignedTwice(const Block *b);
  void checkUnutilizedResult(const ExprStatement *b);
  void checkNullableContainer(const ForeachStatement *loop);
  void checkMissedBreak(const SwitchStatement *swtch);

  const SQChar *findSlotNameInStack(const Decl *);
  void checkFunctionReturns(FunctionDecl *func);

  void checkAccessNullable(const DestructuringDecl *d);
  void checkAccessNullable(const AccessExpr *acc);
  void checkEnumConstUsage(const GetFieldExpr *acc);

  enum StackSlotType {
    SST_NODE,
    SST_TABLE_MEMBER
  };

  struct StackSlot {
    enum StackSlotType sst;
    union {
      const Node *n;
      const struct TableMember *member;
    };
  };

  std::vector<StackSlot> nodeStack;

  struct VarScope *currentScope;
  struct BreakableScope *breakScope;

  FunctionInfo *makeFunctionInfo(const FunctionDecl *d, const FunctionDecl *o) {
    void *mem = arena->allocate(sizeof(FunctionInfo));
    return new(mem) FunctionInfo(d, o);
  }

  ValueRef *makeValueRef(SymbolInfo *info) {
    void *mem = arena->allocate(sizeof(ValueRef));
    return new (mem) ValueRef(info, currentScope->evalId);
  }

  SymbolInfo *makeSymbolInfo(enum SymbolKind kind) {
    void *mem = arena->allocate(sizeof(SymbolInfo));
    return new (mem) SymbolInfo(kind);
  }

  std::unordered_map<const FunctionDecl *, FunctionInfo *> functionInfoMap;

  std::unordered_set<const SQChar *, StringHasher, StringEqualer> requiredModules;

  Arena *arena;

  FunctionInfo *currentInfo;

  void declareSymbol(const SQChar *name, ValueRef *v);
  void pushFunctionScope(VarScope *functionScope, const FunctionDecl *decl);

  void applyCallToScope(const CallExpr *call);
  void applyBinaryToScope(const BinExpr *bin);
  void applyAssignmentToScope(const BinExpr *bin);
  void applyAssignEqToScope(const BinExpr *bin);

  int32_t computeNameLength(const GetFieldExpr *access);
  void computeNameRef(const GetFieldExpr *access, SQChar *b, int32_t &ptr, int32_t size);
  int32_t computeNameLength(const Expr *e);
  void computeNameRef(const Expr *lhs, SQChar *buffer, int32_t &ptr, int32_t size);
  const SQChar *computeNameRef(const Expr *lhs, SQChar *buffer, size_t bufferSize);

  ValueRef *findValueInScopes(const SQChar *ref);
  void applyKnownInvocationToScope(const ValueRef *ref);
  void applyUnknownInvocationToScope();

  const FunctionInfo *findFunctionInfo(const Expr *e, bool &isCtor);

  void setValueFlags(const Expr *lvalue, unsigned pf, unsigned nf);
  const ValueRef *findValueForExpr(const Expr *e);
  const Expr *maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited);
  const Expr *maybeEval(const Expr *e, int32_t &evalId);
  const Expr *maybeEval(const Expr *e);

  const SQChar *findFieldName(const Expr *e);

  bool isSafeAccess(const AccessExpr *acc);
  bool isPotentiallyNullable(const Expr *e);
  bool isPotentiallyNullable(const Expr *e, std::unordered_set<const Expr *> &visited);
  bool couldBeString(const Expr *e);

  BinExpr *wrapConditionIntoNC(Expr *e);
  void visitBinaryBranches(Expr *lhs, Expr *rhs, bool inv);
  void speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, bool inv = false);
  void speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, std::unordered_set<const Expr *> &visited, int32_t evalId, unsigned flags, bool inv);
  bool detectTypeOfPattern(const Expr *expr, const Expr *&r_checkee, const LiteralExpr *&r_lit);
  bool detectNullCPattern(enum TreeOp op, const Expr *expr, const Expr *&checkee);

  void checkAssertCall(const CallExpr *call);

  LiteralExpr trueValue, falseValue, nullValue;

  bool effectsOnly;

public:

  CheckerVisitor(SQCompilationContext &ctx)
    : _ctx(ctx)
    , arena(ctx.arena())
    , currentInfo(nullptr)
    , currentScope(nullptr)
    , breakScope(nullptr)
    , trueValue(true)
    , falseValue(false)
    , nullValue()
    , effectsOnly(false) {}

  ~CheckerVisitor();

  void visitNode(Node *n);


  void visitId(Id *id);
  void visitBinExpr(BinExpr *expr);
  void visitTerExpr(TerExpr *expr);
  void visitIncExpr(IncExpr *expr);
  void visitCallExpr(CallExpr *expr);
  void visitGetFieldExpr(GetFieldExpr *expr);
  void visitGetTableExpr(GetTableExpr *expr);

  void visitExprStatement(ExprStatement *stmt);

  void visitBlock(Block *b);
  void visitForStatement(ForStatement *loop);
  void visitForeachStatement(ForeachStatement *loop);
  void visitWhileStatement(WhileStatement *loop);
  void visitDoWhileStatement(DoWhileStatement *loop);
  void visitIfStatement(IfStatement *ifstmt);

  void visitBreakStatement(BreakStatement *breakStmt);
  void visitContinueStatement(ContinueStatement *continueStmt);

  void visitSwitchStatement(SwitchStatement *swtch);

  void visitTryStatement(TryStatement *tryStmt);

  void visitFunctionDecl(FunctionDecl *func);

  void visitTableDecl(TableDecl *table);
  void visitClassDecl(ClassDecl *klass);

  void visitParamDecl(ParamDecl *p);
  void visitVarDecl(VarDecl *decl);
  void visitConstDecl(ConstDecl *cnst);
  void visitEnumDecl(EnumDecl *enm);
  void visitDestructuringDecl(DestructuringDecl *decl);

  void analyse(RootBlock *root);
};

void VarScope::checkUnusedSymbols(CheckerVisitor *checker) {

  for (auto &s : symbols) {
    const SQChar *n = s.first;
    const ValueRef *v = s.second;

    if (strcmp(n, thisName) == 0) // skip 'this'
      continue;

    if (n[0] == '_')
      continue;

    SymbolInfo *info = v->info;

    if (info->kind == SK_ENUM_CONST)
      continue;

    if (!info->used) {
      checker->report(info->extractPointedNode(), DiagnosticsId::DI_DECLARED_NEVER_USED, info->contextName(), n);
      // TODO: add hint for param/exception name about silencing it with '_' prefix
    }
  }
}

BreakableScope::BreakableScope(CheckerVisitor *v, const SwitchStatement *swtch) : visitor(v), kind(BSK_SWITCH), loopScope(nullptr), exitScope(nullptr), parent(v->breakScope) {
  node.swtch = swtch;
  v->breakScope = this;
}

BreakableScope::BreakableScope(CheckerVisitor *v, const LoopStatement *loop, VarScope *ls, VarScope *es) : visitor(v), kind(BSK_LOOP), loopScope(ls), exitScope(es), parent(v->breakScope) {
  assert(ls/* && es*/);
  node.loop = loop;
  v->breakScope = this;
}

BreakableScope::~BreakableScope() {
  visitor->breakScope = parent;
}

CheckerVisitor::~CheckerVisitor() {
  for (auto &p : functionInfoMap) {
    if (p.second)
      p.second->~FunctionInfo();
  }
}

void CheckerVisitor::visitNode(Node *n) {
  nodeStack.push_back({ SST_NODE, n });
  Visitor::visitNode(n);
  nodeStack.pop_back();
}

void CheckerVisitor::report(const Node *n, int32_t id, ...) {
  if (effectsOnly)
    return;

  va_list vargs;
  va_start(vargs, id);

  _ctx.vreportDiagnostic((enum DiagnosticsId)id, n->lineStart(), n->columnStart(), n->columnEnd() - n->columnStart(), vargs); // -V522

  va_end(vargs);
}

bool CheckerVisitor::isUpperCaseIdentifier(const Expr *e) {

  const char *id = nullptr;

  if (e->op() == TO_GETFIELD) {
    id = e->asGetField()->fieldName();
  }

  if (e->op() == TO_ID) {
    id = e->asId()->id();
  }
  else if (e->op() == TO_LITERAL && e->asLiteral()->kind() == LK_STRING) {
    id = e->asLiteral()->s();
  }

  if (!id)
    return false;

  while (*id) {
    if (*id >= 'a' && *id <= 'z')
      return false;
    ++id;
  }

  return true;
}

void CheckerVisitor::checkForeachIteratorInClosure(const Id *id, const ValueRef *v) {
  if (effectsOnly)
    return;

  if (v->info->kind != SK_FOREACH)
    return;

  const FunctionDecl *thisScopeOwner = currentScope->owner;
  const FunctionDecl *vOwnerScope = v->info->ownedScope->owner;

  if (v->info->ownedScope->owner == currentScope->owner)
    return;

  int32_t i = nodeStack.size() - 1;
  assert(i > 0);

  int32_t thisId = -1;
  auto rit = nodeStack.rbegin();
  auto rie = nodeStack.rend();

  while (i >= 0) {
    auto &m = nodeStack[i];

    if (m.sst == SST_NODE) {
      if (m.n == thisScopeOwner) {
        thisId = i;
        break;
      }
    }
    --i;
  }

  assert(thisId > 0);

  if (i > 2) { // decl_expr + call + foreach

    auto &candidate = nodeStack[i - 2];

    if (candidate.sst == SST_NODE && candidate.n->op() == TO_CALL) {
      const CallExpr *call = candidate.n->asExpression()->asCallExpr();

      bool found = false;
      for (auto arg : call->arguments()) {
        if (arg->op() == TO_DECL_EXPR && arg->asDeclExpr()->declaration() == thisScopeOwner) {
          found = true;
          break;
        }
      }

      if (found) {// in call arguments
        const Expr *callee = call->callee();
        const SQChar *name = nullptr;

        if (callee->op() == TO_ID)
          name = callee->asId()->id();
        else if (callee->op() == TO_GETFIELD)
          name = callee->asGetField()->fieldName();

        if (name && nameLooksLikeCallsLambdaInPlace(name)) {
          return;
        }
      }
    }
  }

  report(id, DiagnosticsId::DI_ITER_IN_CLOSURE, id->id());
}

void CheckerVisitor::checkIdUsed(const Id *id, const Node *p, ValueRef *v) {
  const Expr *e = nullptr;
  if (p) {
    if (p->op() == TO_EXPR_STMT)
      e = static_cast<const ExprStatement *>(p)->expression();
    else if (p->isExpression())
      e = p->asExpression();
  }

  bool assigned = v->assigned;

  if (e && isAssignExpr(e)) {
    const BinExpr *bin = static_cast<const BinExpr *>(e);
    const Expr *lhs = bin->lhs();
    const Expr *rhs = bin->rhs();
    bool simpleAsgn = e->op() == TO_ASSIGN || e->op() == TO_INEXPR_ASSIGN;
    if (id == lhs) {
      bool used = v->info->usedAfterAssign || existsInTree(id, bin->rhs());
      //if (!used && assigned && simpleAsgn) {
      //  if (!v->lastAssigneeScope || currentScope->owner == v->lastAssigneeScope->owner)
      //    report(bin, DiagnosticsId::DI_REASSIGN_WITH_NO_USAGE);
      //}
      v->info->used |= used;
      v->assigned = true;
      v->lastAssigneeScope = currentScope;
      v->info->usedAfterAssign = false;
      if (simpleAsgn)
        return;
    }
  }

  // it's usage
  v->info->used = true;
  if (assigned) {
    v->info->usedAfterAssign = true;
    v->assigned = e ? TO_PLUSEQ <= e->op() && e->op() <= TO_MODEQ : false;
  }

  if (v->state == VRS_PARTIALLY) {
    report(id, DiagnosticsId::DI_POSSIBLE_GARBAGE, id->id());
  }
  else if (v->state == VRS_UNDEFINED) {
    report(id, DiagnosticsId::DI_UNINITIALIZED_VAR);
  }
}

void CheckerVisitor::checkAndOrPriority(const BinExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (expr->op() == TO_OROR) {
    if (l->op() == TO_ANDAND || r->op() == TO_ANDAND) {
      report(expr, DiagnosticsId::DI_AND_OR_PAREN);
    }
  }
}

void CheckerVisitor::checkBitwiseParenBool(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (expr->op() == TO_OROR || expr->op() == TO_ANDAND) {
    if (l->op() == TO_AND || l->op() == TO_OR || r->op() == TO_AND || r->op() == TO_OR) {
      report(expr, DiagnosticsId::DI_BITWISE_BOOL_PAREN);
    }
  }
}

void CheckerVisitor::checkCoalescingPriority(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (expr->op() == TO_NULLC) {
    if (isSuspiciousNeighborOfNullCoalescing(l->op())) {
      report(l, DiagnosticsId::DI_NULL_COALESCING_PRIOR, treeopStr(l->op()));
    }

    if (isSuspiciousNeighborOfNullCoalescing(r->op())) {
      report(r, DiagnosticsId::DI_NULL_COALESCING_PRIOR, treeopStr(r->op()));
    }
  }
}

void CheckerVisitor::checkAssignmentToItself(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (expr->op() == TO_ASSIGN) {
    auto *ll = deparen(l);
    auto *rr = deparen(r);

    if (_equalChecker.check(ll, rr)) {
      report(expr, DiagnosticsId::DI_ASG_TO_ITSELF);
    }
  }
}

void CheckerVisitor::checkGlobalVarCreation(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();

  if (expr->op() == TO_NEWSLOT) {
    if (l->op() == TO_ID) {
      report(expr, DiagnosticsId::DI_GLOBAL_VAR_CREATE);
    }
  }
}

void CheckerVisitor::checkSameOperands(const BinExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (isSuspiciousSameOperandsBinaryOp(expr->op())) {
    const Expr *ll = deparen(l);
    const Expr *rr = deparen(r);

    if (_equalChecker.check(ll, rr)) {
      if (ll->op() != TO_LITERAL || (ll->asLiteral()->kind() != LK_FLOAT && ll->asLiteral()->kind() != LK_INT)) { // -V522
        report(expr, DiagnosticsId::DI_SAME_OPERANDS, treeopStr(expr->op()));
      }
    }
  }
}

void CheckerVisitor::checkAlwaysTrueOrFalse(const Expr *n) {

  if (effectsOnly)
    return;

  const Node *cond = n; // maybeEval(n);

  if (cond->op() == TO_LITERAL) {
    const LiteralExpr *l = cond->asExpression()->asLiteral();
    report(n, DiagnosticsId::DI_ALWAYS_T_OR_F, l->raw() ? "true" : "false");
  }
  else if (cond->op() == TO_ARRAYEXPR || cond->op() == TO_DECL_EXPR || cond->isDeclaration()) {
    report(n, DiagnosticsId::DI_ALWAYS_T_OR_F, "true");
  }
}

void CheckerVisitor::checkAlwaysTrueOrFalse(const TerExpr *expr) {
  checkAlwaysTrueOrFalse(expr->a());
}

void CheckerVisitor::checkTernaryPriority(const TerExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *cond = expr->a();
  const Expr *thenExpr = expr->b();
  const Expr *elseExpr = expr->c();

  if (isSuspiciousTernaryConditionOp(cond->op())) {
    const BinExpr *binCond = static_cast<const BinExpr *>(cond);
    const Expr *l = binCond->lhs();
    const Expr *r = binCond->rhs();
    bool isIgnore = cond->op() == TO_AND
      && (isUpperCaseIdentifier(l) || isUpperCaseIdentifier(r)
        || (r->op() == TO_LITERAL && r->asLiteral()->kind() == LK_INT));

    if (!isIgnore) {
      report(cond, DiagnosticsId::DI_TERNARY_PRIOR, treeopStr(cond->op()));
    }
  }
}

void CheckerVisitor::checkSameValues(const TerExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *ifTrue = expr->b();
  const Expr *ifFalse = expr->c();

  if (_equalChecker.check(ifTrue, ifFalse)) {
    report(expr, DiagnosticsId::DI_TERNARY_SAME_VALUES);
  }
}

void CheckerVisitor::checkAlwaysTrueOrFalse(const BinExpr *bin) {

  if (effectsOnly)
    return;

  enum TreeOp op = bin->op();
  if (op != TO_ANDAND && op != TO_OROR)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  enum TreeOp cmpOp = lhs->op();
  if (cmpOp == rhs->op() && (cmpOp == TO_NE || cmpOp == TO_EQ)) {
    const char *constValue = nullptr;

    if (op == TO_ANDAND && cmpOp == TO_EQ)
      constValue = "false";

    if (op == TO_OROR && cmpOp == TO_NE)
      constValue = "true";

    if (!constValue)
      return;

    const BinExpr *lhsBin = static_cast<const BinExpr *>(lhs);
    const BinExpr *rhsBin = static_cast<const BinExpr *>(rhs);

    const Expr *lconst = lhsBin->rhs();
    const Expr *rconst = rhsBin->rhs();

    if (lconst->op() == TO_LITERAL || isUpperCaseIdentifier(lconst)) {
      if (rconst->op() == TO_LITERAL || isUpperCaseIdentifier(rconst)) {
        if (_equalChecker.check(lhsBin->lhs(), rhsBin->lhs())) {
          if (!_equalChecker.check(lconst, rconst)) {
            report(bin, DiagnosticsId::DI_ALWAYS_T_OR_F, constValue);
          }
        }
      }
    }
  }
  else if ((lhs->op() == TO_NOT || rhs->op() == TO_NOT) && (lhs->op() != rhs->op())) {
    const char *v = op == TO_OROR ? "true" : "false";
    if (lhs->op() == TO_NOT) {
      const UnExpr *u = static_cast<const UnExpr *>(lhs);
      if (_equalChecker.check(u->argument(), rhs)) {
        report(bin, DiagnosticsId::DI_ALWAYS_T_OR_F, v);
      }
    }
    if (rhs->op() == TO_NOT) {
      const UnExpr *u = static_cast<const UnExpr *>(rhs);
      if (_equalChecker.check(lhs, u->argument())) {
        report(bin, DiagnosticsId::DI_ALWAYS_T_OR_F, v);
      }
    }
  }
}

void CheckerVisitor::checkDeclarationInArith(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (isArithOperator(bin->op())) {
    const Expr *lhs = maybeEval(bin->lhs());
    const Expr *rhs = maybeEval(bin->rhs());

    if (lhs->op() == TO_DECL_EXPR || lhs->op() == TO_ARRAYEXPR) {
      report(bin->lhs(), DiagnosticsId::DI_DECL_IN_EXPR);
    }

    if (bin->op() != TO_OROR && bin->op() != TO_ANDAND) {
      if (rhs->op() == TO_DECL_EXPR || rhs->op() == TO_ARRAYEXPR) {
        report(bin->rhs(), DiagnosticsId::DI_DECL_IN_EXPR);
      }
    }
  }
}

void CheckerVisitor::checkIntDivByZero(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (isDivOperator(bin->op())) {
    const Expr *divided = maybeEval(bin->lhs());
    const Expr *divisor = maybeEval(bin->rhs());

    if (divisor->op() == TO_LITERAL) {
      const LiteralExpr *rhs = divisor->asLiteral();
      if (rhs->raw() == 0) {
        report(bin, DiagnosticsId::DI_DIV_BY_ZERO);
        return;
      }
      if (divided->op() == TO_LITERAL) {
        const LiteralExpr *lhs = divided->asLiteral();
        if (lhs->kind() == LK_INT && rhs->kind() == LK_INT) {
          if (rhs->i() == -1 && lhs->i() == MIN_SQ_INTEGER) {
            report(bin, DiagnosticsId::DI_INTEGER_OVERFLOW);
          }
          else if (lhs->i() % rhs->i() != 0) {
            report(bin, DiagnosticsId::DI_ROUND_TO_INT);
          }
        }
      }
    }
  }
}

void CheckerVisitor::checkPotentiallyNullableOperands(const BinExpr *bin) {

  if (effectsOnly)
    return;

  bool isRelOp = isRelationOperator(bin->op());
  bool isArithOp = isPureArithOperator(bin->op());
  bool isAssign = bin->op() == TO_ASSIGN || bin->op() == TO_INEXPR_ASSIGN || bin->op() == TO_NEWSLOT;

  if (!isRelOp && !isArithOp && !isAssign)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  const char *opType = isRelOp ? "Comparison operation" : "Arithmetic operation";

  if (isPotentiallyNullable(lhs)) {
    if (isAssign) {
      if (lhs->op() != TO_ID) { // -V522
        report(bin->lhs(), DiagnosticsId::DI_NULLABLE_ASSIGNMENT);
      }
    }
    else {
      report(bin->lhs(), DiagnosticsId::DI_NULLABLE_OPERANDS, opType);
    }
  }

  if (isPotentiallyNullable(rhs) && !isAssign) {
    report(bin->rhs(), DiagnosticsId::DI_NULLABLE_OPERANDS, opType);
  }
}

void CheckerVisitor::checkBitwiseToBool(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (!isBitwiseOperator(bin->op()))
    return;

  const Expr *lhs = maybeEval(bin->lhs());
  const Expr *rhs = maybeEval(bin->rhs());

  if (looksLikeBooleanExpr(lhs) || looksLikeBooleanExpr(rhs)) {
    report(bin, DiagnosticsId::DI_BITWISE_OP_TO_BOOL);
  }

}

void CheckerVisitor::checkRelativeCompareWithBool(const BinExpr *expr) {
  if (effectsOnly)
    return;

  if (!isBoolRelationOperator(expr->op()))
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  const Expr *dl = deparen(l);
  const Expr *dr = deparen(r);

  const Expr *el = maybeEval(dl);
  const Expr *er = maybeEval(dr);

  if (looksLikeBooleanExpr(l) || looksLikeBooleanExpr(r) ||
    looksLikeBooleanExpr(dl) || looksLikeBooleanExpr(dr) || // -V522
    looksLikeBooleanExpr(el) || looksLikeBooleanExpr(er)) { // -V522
    report(expr, DiagnosticsId::DI_RELATIVE_CMP_BOOL);
  }
}

void CheckerVisitor::checkCompareWithBool(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  enum TreeOp thisOp = expr->op();
  enum TreeOp lhsOp = l->op();
  enum TreeOp rhsOp = r->op();

  // if (a == b != c)
  if (thisOp == TO_EQ || thisOp == TO_NE) {
    if (lhsOp == TO_EQ || lhsOp == TO_NE || rhsOp == TO_EQ || rhsOp == TO_NE) {
      report(expr, DiagnosticsId::DI_EQ_PAREN_MISSED);
      return;
    }
  }

  if (isBoolCompareOperator(thisOp) && lhsOp != TO_PAREN && rhsOp != TO_PAREN) {
    const Expr *lhs = maybeEval(l);
    const Expr *rhs = maybeEval(r);

    if (isBoolCompareOperator(lhs->op()) || isBoolCompareOperator(rhs->op())) {
      bool warn = true;

      if (expr->op() == TO_EQ || expr->op() == TO_NE) {

        //function_should_return_bool_prefix

        if (nameLooksLikeResultMustBeBoolean(findFieldName(l)) ||
          nameLooksLikeResultMustBeBoolean(findFieldName(r)) ||
          nameLooksLikeResultMustBeBoolean(findFieldName(rhs)) ||
          nameLooksLikeResultMustBeBoolean(findFieldName(lhs))) {
          warn = false;
        }

        if (l->op() == TO_ID && r->op() == TO_ID) {
          warn = false;
        }
      }

      if (warn) {
        report(expr, DiagnosticsId::DI_COMPARE_WITH_BOOL);
      }
    }
  }
}

void CheckerVisitor::checkCopyOfExpression(const BinExpr *bin) {

  if (effectsOnly)
    return;

  enum TreeOp op = bin->op();
  if (op != TO_OROR && op != TO_ANDAND && op != TO_OR)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *cmp = bin->rhs();

  while (cmp->op() == op) {
    const BinExpr *binCmp = static_cast<const BinExpr *>(cmp);

    if (_equalChecker.check(lhs, binCmp->lhs()) || _equalChecker.check(lhs, binCmp->rhs())) {
      report(binCmp, DiagnosticsId::DI_COPY_OF_EXPR);
    }
    cmp = binCmp->rhs();
  }
}

void CheckerVisitor::checkConstInBoolExpr(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (bin->op() != TO_OROR && bin->op() != TO_ANDAND)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  bool leftIsConst = lhs->op() == TO_LITERAL || lhs->op() == TO_DECL_EXPR || lhs->op() == TO_ARRAYEXPR || isUpperCaseIdentifier(lhs); //-V522
  bool rightIsConst = rhs->op() == TO_LITERAL || rhs->op() == TO_DECL_EXPR || rhs->op() == TO_ARRAYEXPR || isUpperCaseIdentifier(rhs); // -V522

  if (rightIsConst && bin->op() == TO_OROR) {
    if (rhs->op() != TO_LITERAL || rhs->asLiteral()->kind() != LK_BOOL || rhs->asLiteral()->b() != true) {
      rightIsConst = false;
    }
  }

  if (leftIsConst || rightIsConst) {
    report(bin, DiagnosticsId::DI_CONST_IN_BOOL_EXPR);
  }
}

void CheckerVisitor::checkShiftPriority(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (isShiftOperator(bin->op())) {
    if (isHigherShiftPriority(bin->lhs()->op()) || isHigherShiftPriority(bin->rhs()->op())) {
      report(bin, DiagnosticsId::DI_SHIFT_PRIORITY);
    }
  }
}

void CheckerVisitor::checkCanReturnNull(const BinExpr *bin) {
  if (effectsOnly)
    return;


  if (!isPureArithOperator(bin->op()) && !isRelationOperator(bin->op())) {
    return;
  }

  const Expr *l = skipUnary(maybeEval(skipUnary(bin->lhs())));
  const Expr *r = skipUnary(maybeEval(skipUnary(bin->rhs())));
  const CallExpr *c = nullptr;

  if (l->op() == TO_CALL) // -V522
    c = static_cast<const CallExpr *>(l);
  else if (r->op() == TO_CALL) // -V522
    c = static_cast<const CallExpr *>(r);

  if (!c)
    return;

  const Expr *callee = c->callee();
  bool isCtor = false;
  const FunctionInfo *info = findFunctionInfo(callee, isCtor);

  if (isCtor)
    return;

  const SQChar *funcName = nullptr;

  if (info) {
    funcName = info->declaration->name();
  }
  else if (callee->op() == TO_ID) {
    funcName = callee->asId()->id();
  }
  else if (callee->op() == TO_GETFIELD) {
    funcName = callee->asGetField()->fieldName();
  }

  if (funcName) {
    if (canFunctionReturnNull(funcName)) {
      report(c, DiagnosticsId::DI_FUNC_CAN_RET_NULL, funcName);
    }
  }
}

void CheckerVisitor::checkCompareWithContainer(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (!isCompareOperator(bin->op()))
    return;

  const Expr *l = bin->lhs();
  const Expr *r = bin->rhs();

  if (l->op() == TO_ARRAYEXPR || r->op() == TO_ARRAYEXPR) {
    report(bin, DiagnosticsId::DI_CMP_WITH_CONTAINER, "array");
  }

  if (l->op() == TO_DECL_EXPR || r->op() == TO_DECL_EXPR) {
    report(bin, DiagnosticsId::DI_CMP_WITH_CONTAINER, "declaration");
  }
}

void CheckerVisitor::checkExtendToAppend(const CallExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *callee = expr->callee();
  const auto &args = expr->arguments();

  if (callee->op() == TO_GETFIELD) {
    if (args.size() > 0) {
      Expr *arg0 = args[0];
      if (arg0->op() == TO_ARRAYEXPR) {
        if (strcmp(callee->asGetField()->fieldName(), "extend") == 0) {
          report(expr, DiagnosticsId::DI_EXTEND_TO_APPEND);
        }
      }
    }
  }
}

void CheckerVisitor::checkBoolToStrangePosition(const BinExpr *bin) {
  if (effectsOnly)
    return;

  if (bin->op() != TO_IN && bin->op() != TO_INSTANCEOF)
    return;

  const Expr *lhs = maybeEval(bin->lhs());
  const Expr *rhs = maybeEval(bin->rhs());

  if (looksLikeBooleanExpr(lhs) && bin->op() == TO_IN) {
    report(bin, DiagnosticsId::DI_BOOL_PASSED_TO_STRANGE, "in");
  }

  if (looksLikeBooleanExpr(rhs)) {
    report(bin, DiagnosticsId::DI_BOOL_PASSED_TO_STRANGE, bin->op() == TO_IN ? "in" : "instanceof");
  }
}

const SQChar *tryExtractKeyName(const Expr *e) {

  if (e->op() == TO_GETFIELD)
    return e->asGetField()->fieldName();

  if (e->op() == TO_GETTABLE) {
    e = e->asGetTable()->key();
  }

  if (e->op() == TO_LITERAL) {
    if (e->asLiteral()->kind() == LK_STRING)
      return e->asLiteral()->s();
  }

  return nullptr;
}

static bool isValidId(const SQChar *id) {
  assert(id != nullptr);

  if (!isalpha(id[0]) && id[0] != '_')
    return false;

  for (int i = 1; id[i]; ++i) {
    SQChar c = id[i];
    if (!isalpha(c) && !isdigit(c) && c != '_')
      return false;
  }

  return true;
}

void CheckerVisitor::checkKeyNameMismatch(const Expr *key, const Expr *e) {

  /*
    let function bar() {}
    let tt = {
      "1foo" : function bar1() { .. }, // OK, not id
      "foo2" : function bar2() { .. }, // WARN
      "foo3" : function foo3() { .. }, // OK

      foo4 = function bar5() { ... }, // WARN
      foo6 = function foo6() { ... }, // OK

      ["foo7"] = function bar7() { ... }, // WARN
      ["8foo"] = function bar8() { ... } // OK, not id
    }
    tt.foo <- bar // OK
    tt.qux <- function fex() { .. } // WARN
    tt["fk"] <- function uyte() { .. } // WARN
    tt["f:g:h"] <- function fgh() { .. } // OK
  */

  if (effectsOnly)
    return;

  const SQChar *fieldName = tryExtractKeyName(key);

  if (!fieldName)
    return;

  if (e->op() != TO_DECL_EXPR)
    return;

  const Decl *decl = static_cast<const DeclExpr *>(e)->declaration();

  const SQChar *declName = nullptr;

  if (decl->op() == TO_FUNCTION) {
    const FunctionDecl *f = static_cast<const FunctionDecl *>(decl);
    if (!f->isLambda() && f->name()[0] != '(') {
      declName = f->name();
    }
  }

  if (!declName)
    return;

  if (!isValidId(fieldName))
    return;

  if (strcmp(fieldName, declName) != 0) {
    report(e, DiagnosticsId::DI_KEY_NAME_MISMATCH, fieldName, declName);
  }
}

void CheckerVisitor::checkNewSlotNameMatch(const BinExpr *bin) {
  if (effectsOnly)
    return;

  if (bin->op() != TO_NEWSLOT)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  checkKeyNameMismatch(lhs, rhs);
}

void CheckerVisitor::checkPlusString(const BinExpr *bin) {
  if (effectsOnly)
    return;

  if (bin->op() != TO_ADD && bin->op() != TO_PLUSEQ)
    return;

  const Expr *l = maybeEval(bin->lhs());
  const Expr *r = maybeEval(bin->rhs());

  bool sl = couldBeString(l);
  bool sr = couldBeString(r);

  if (bool(sl) != bool(sr)) {
    report(bin, DiagnosticsId::DI_PLUS_STRING);
  }
}

void CheckerVisitor::checkAlreadyRequired(const CallExpr *call) {
  if (effectsOnly)
    return;

  if (nodeStack.size() > 2)
    return; // do not consider require which is not on TL

  if (call->arguments().size() != 1)
    return;

  const Expr *arg = maybeEval(call->arguments()[0]);

  if (arg->op() != TO_LITERAL)
    return;

  const LiteralExpr *l = arg->asLiteral();
  if (l->kind() != LK_STRING)
    return;


  const Expr *callee = call->callee();
  bool isCtor = false;
  const FunctionInfo *info = findFunctionInfo(callee, isCtor);

  if (isCtor)
    return;

  const SQChar *name = nullptr;

  if (info) {
    name = info->declaration->name();
  }
  else if (callee->op() == TO_ID) {
    name = callee->asId()->id();
  }
  else if (callee->op() == TO_GETFIELD) {
    const GetFieldExpr *g = callee->asGetField();
    if (g->receiver()->op() == TO_ROOT) {
      name = g->fieldName();
    }
  }

  if (!name)
    return;

  if (strcmp(name, "require") != 0)
    return;

  const SQChar *moduleName = l->s();

  if (requiredModules.find(moduleName) != requiredModules.end()) {
    report(call, DiagnosticsId::DI_ALREADY_REQUIRED, moduleName);
  }
  else {
    requiredModules.insert(moduleName);
  }
}

void CheckerVisitor::checkCallNullable(const CallExpr *call) {
  if (effectsOnly)
    return;

  const Expr *c = call->callee();

  if (!call->isNullable() && isPotentiallyNullable(c)) {
    report(c, DiagnosticsId::DI_ACCESS_POT_NULLABLE, c->op() == TO_ID ? c->asId()->id() : "expression", "function");
  }
}

int32_t CheckerVisitor::normalizeParamNameLength(const SQChar *name) {
  int32_t r = 0;

  while (*name) {
    if (*name != '_')
      ++r;
    ++name;
  }

  return r;
}

const SQChar *CheckerVisitor::normalizeParamName(const SQChar *name, SQChar *buffer) {

  if (!buffer) {
    int32_t nl = normalizeParamNameLength(name);
    buffer = (SQChar *)arena->allocate(nl + 1);
  }

  int32_t i = 0, j = 0;
  while (name[i]) {
    SQChar c = name[i++];
    if (c != '_') {
      buffer[j++] = std::tolower(c);
    }
  }

  buffer[j] = '\0';

  return buffer;
}

void CheckerVisitor::checkArguments(const CallExpr *callExpr) {

  if (effectsOnly)
    return;

  bool dummy;
  const FunctionInfo *info = findFunctionInfo(callExpr->callee(), dummy);

  if (!info)
    return;

  const FunctionDecl *decl = info->declaration;
  const auto &params = decl->parameters();
  const auto &paramsNames = info->parameters;
  const auto &args = callExpr->arguments();

  const size_t effectiveParamSizeUP = decl->isVararg() ? paramsNames.size() - 2 : paramsNames.size() - 1;
  int32_t dpParameters = 0;

  for (auto &p : params) {
    if (p->hasDefaultValue())
      ++dpParameters;
  }

  const size_t effectiveParamSizeLB = effectiveParamSizeUP - dpParameters;
  const size_t maxParamSize = decl->isVararg() ? SIZE_MAX : effectiveParamSizeUP;

  if (!(effectiveParamSizeLB <= args.size() && args.size() <= maxParamSize)) {
    report(callExpr, DiagnosticsId::DI_PARAM_COUNT_MISMATCH, decl->name());
  }

  for (int i = 1; i < paramsNames.size(); ++i) {
    const SQChar *paramName = paramsNames[i];
    for (int j = 0; j < args.size(); ++j) {
      const Expr *arg = args[j];
      const SQChar *possibleArgName = nullptr;

      if (arg->op() == TO_ID)
        possibleArgName = arg->asId()->id();
      else if (arg->op() == TO_GETFIELD)
        possibleArgName = arg->asGetField()->fieldName();

      if (!possibleArgName)
        continue;

      int32_t argNL = normalizeParamNameLength(possibleArgName);
      SQChar *buffer = (SQChar *)sq_malloc(_ctx.allocContext(), argNL + 1);
      normalizeParamName(possibleArgName, buffer);

      if ((i - 1) != j) {
        if (strcmp("vargv", paramName) != 0 || (j != (paramsNames.size() - 1))) {
          if (strcmp(paramName, buffer) == 0) {  // -V575
            report(arg, DiagnosticsId::DI_PARAM_POSITION_MISMATCH, paramName);
          }
        }
      }

      sq_free(_ctx.allocContext(), buffer, argNL + 1);
    }
  }
}

void CheckerVisitor::checkAssertCall(const CallExpr *call) {

  // assert(x != null) or assert(x != null, "X should not be null")

  const Expr *callee = maybeEval(call->callee());

  if (callee->op() != TO_ID)
    return;

  if (strcmp("assert", callee->asId()->id()) != 0)
    return;


  const SQUnsignedInteger argSize = call->arguments().size();

  if (!argSize || argSize > 2)
    return;

  const Expr *cond = call->arguments()[0];

  speculateIfConditionHeuristics(cond, currentScope, nullptr);
}

BinExpr *CheckerVisitor::wrapConditionIntoNC(Expr *e) {
  Expr *b = new (arena) UnExpr(TO_PAREN, e);

  b->setLineStartPos(e->lineStart());
  b->setColumnStartPos(e->columnStart());
  b->setLineEndPos(e->lineEnd());
  b->setColumnEndPos(e->columnEnd());

  BinExpr *r = new (arena) BinExpr(TO_NE, b, &nullValue);

  r->setLineEndPos(b->lineEnd());
  r->setColumnEndPos(b->columnEnd());

  return r;
}

void CheckerVisitor::visitBinaryBranches(Expr *lhs, Expr *rhs, bool inv) {
  VarScope *trunkScope = currentScope;
  VarScope *branchScope = nullptr;

  lhs->visit(this);

  branchScope = trunkScope->copy(arena);

  if (inv) {
    speculateIfConditionHeuristics(lhs, nullptr, branchScope);
  }
  else {
    speculateIfConditionHeuristics(lhs, branchScope, nullptr);
  }

  currentScope = branchScope;
  rhs->visit(this);

  trunkScope->merge(branchScope);

  currentScope = trunkScope;
}

void CheckerVisitor::visitId(Id *id) {
  const StackSlot &ss = nodeStack.back();
  const Node *parentNode = nullptr;

  if (ss.sst == SST_NODE)
    parentNode = ss.n;

  ValueRef *v = findValueInScopes(id->id());

  if (!v)
    return;

  checkForeachIteratorInClosure(id, v);
  checkIdUsed(id, parentNode, v);
}

void CheckerVisitor::visitBinExpr(BinExpr *expr) {
  checkAndOrPriority(expr);
  checkBitwiseParenBool(expr);
  checkCoalescingPriority(expr);
  checkAssignmentToItself(expr);
  //checkGlobalVarCreation(expr); // seems this check is no longer has any sense
  checkSameOperands(expr);
  checkAlwaysTrueOrFalse(expr);
  checkDeclarationInArith(expr);
  checkIntDivByZero(expr);
  checkPotentiallyNullableOperands(expr);
  checkBitwiseToBool(expr);
  checkCompareWithBool(expr);
  checkRelativeCompareWithBool(expr);
  checkCopyOfExpression(expr);
  checkConstInBoolExpr(expr);
  checkShiftPriority(expr);
  //checkCanReturnNull(expr);
  checkCompareWithContainer(expr);
  checkBoolToStrangePosition(expr);
  checkNewSlotNameMatch(expr);
  checkPlusString(expr);

  Expr *lhs = expr->lhs();
  Expr *rhs = expr->rhs();

  switch (expr->op())
  {
  case TO_NULLC:
    lhs = wrapConditionIntoNC(lhs); // -V796
  case TO_OROR:
    visitBinaryBranches(lhs, rhs, true);
    break;
  case TO_ANDAND:
    visitBinaryBranches(lhs, rhs, false);
    break;
  case TO_PLUSEQ:
  case TO_MINUSEQ:
  case TO_MULEQ:
  case TO_DIVEQ:
  case TO_MODEQ:
  case TO_NEWSLOT:
  case TO_ASSIGN:
  case TO_INEXPR_ASSIGN:
    expr->rhs()->visit(this);
    expr->lhs()->visit(this);
    break;
  default:
    Visitor::visitBinExpr(expr);
    break;
  }

  applyBinaryToScope(expr);
}

void CheckerVisitor::visitTerExpr(TerExpr *expr) {
  checkTernaryPriority(expr);
  checkSameValues(expr);
  checkAlwaysTrueOrFalse(expr);

  nodeStack.push_back({ SST_NODE, expr });

  expr->a()->visit(this);

  VarScope *trunkScope = currentScope;

  VarScope *ifTrueScope = trunkScope->copy(arena);
  VarScope *ifFalseScope = trunkScope->copy(arena);

  speculateIfConditionHeuristics(expr->a(), ifTrueScope, ifFalseScope);


  currentScope = ifTrueScope;
  expr->b()->visit(this);

  currentScope = ifFalseScope;
  expr->c()->visit(this);

  nodeStack.pop_back();

  ifTrueScope->merge(ifFalseScope);

  trunkScope->copyFrom(ifTrueScope);

  ifTrueScope->~VarScope();
  ifFalseScope->~VarScope();

  currentScope = trunkScope;
}

void CheckerVisitor::visitIncExpr(IncExpr *expr) {

  SQChar buffer[64] = { 0 };
  const SQChar *name = computeNameRef(deparen(expr->argument()), buffer, sizeof buffer);
  if (name) {
    ValueRef *v = findValueInScopes(name);

    if (v) {
      if (currentInfo) {
        currentInfo->addModifiable(name, v->info->ownedScope->owner);
      }

      v->expression = nullptr;
      v->state = VRS_MULTIPLE;
      v->flagsNegative = v->flagsPositive = 0;
      v->lowerBound.kind = v->upperBound.kind = VBK_UNKNOWN;
    }
  }

  Visitor::visitIncExpr(expr);
}

void CheckerVisitor::visitCallExpr(CallExpr *expr) {
  checkExtendToAppend(expr);
  checkAlreadyRequired(expr);
  checkCallNullable(expr);

  applyCallToScope(expr);

  checkAssertCall(expr);

  Visitor::visitCallExpr(expr);

  checkArguments(expr);
}

void CheckerVisitor::checkBoolIndex(const GetTableExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *key = maybeEval(expr->key())->asExpression();

  if (isBooleanResultOperator(key->op())) {
    report(expr->key(), DiagnosticsId::DI_BOOL_AS_INDEX);
  }
}

void CheckerVisitor::checkNullableIndex(const GetTableExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *key = expr->key();

  if (!isSafeAccess(expr) && isPotentiallyNullable(key)) {
    report(expr->key(), DiagnosticsId::DI_POTENTIALLY_NULLABLE_INDEX);
  }
}

void CheckerVisitor::visitGetFieldExpr(GetFieldExpr *expr) {
  checkAccessNullable(expr);
  checkEnumConstUsage(expr);

  Visitor::visitGetFieldExpr(expr);
}

void CheckerVisitor::visitGetTableExpr(GetTableExpr *expr) {
  checkBoolIndex(expr);
  checkNullableIndex(expr);
  checkAccessNullable(expr);

  Visitor::visitGetTableExpr(expr);
}

void CheckerVisitor::checkDuplicateSwitchCases(SwitchStatement *swtch) {

  if (effectsOnly)
    return;

  const auto &cases = swtch->cases();

  for (int32_t i = 0; i < int32_t(cases.size()) - 1; ++i) {
    for (int32_t j = i + 1; j < cases.size(); ++j) {
      const auto &icase = cases[i];
      const auto &jcase = cases[j];

      if (_equalChecker.check(icase.val, jcase.val)) {
        report(jcase.val, DiagnosticsId::DI_DUPLICATE_CASE);
      }
    }
  }
}

void CheckerVisitor::checkMissedBreak(const SwitchStatement *swtch) {
  if (effectsOnly)
    return;

  auto &cases = swtch->cases();

  FunctionReturnTypeEvaluator rtEvaluator;

  const Statement *last = nullptr;
  for (auto &c : cases) {

    if (last) {
      report(last, DiagnosticsId::DI_MISSED_BREAK);
    }
    last = nullptr;

    const Statement *stmt = c.stmt;
    bool r = false;
    unsigned f = rtEvaluator.compute(stmt, r);
    if (!r) {
      const Statement *uw = unwrapBodyNonEmpty(stmt);

      if (!uw)
        continue; // empty case statement -> FT

      const enum TreeOp op = uw->op();
      if (op != TO_BLOCK) {
        if (op != TO_BREAK)
          last = unwrapBody(stmt);
      }
      else {
        const Block *b = uw->asBlock();
        int32_t dummy;
        const Statement *tmp = lastNonEmpty(b, dummy);
        if (tmp && tmp->op() != TO_BREAK) {
          last = unwrapBody(b);
        }
      }
    }
  }

  if (last && swtch->defaultCase().stmt) {
    /*
      switch (x) {
        case A:
          ...
          break;

        case B:
          ...   <-- missed break
        default:
          ...
      }
    */
    report(last, DiagnosticsId::DI_MISSED_BREAK);
  }
}

void CheckerVisitor::checkDuplicateIfBranches(IfStatement *ifStmt) {

  if (effectsOnly)
    return;

  if (_equalChecker.check(ifStmt->thenBranch(), ifStmt->elseBranch())) {
    report(ifStmt->elseBranch(), DiagnosticsId::DI_THEN_ELSE_EQUAL);
  }
}

void CheckerVisitor::checkDuplicateIfConditions(IfStatement *ifStmt) {

  if (effectsOnly)
    return;

  const Statement *elseB = unwrapSingleBlock(ifStmt->elseBranch());
  const Expr *duplicated = nullptr;

  if (elseB && elseB->op() == TO_IF) {
    if (findIfWithTheSameCondition(ifStmt->condition(), static_cast<const IfStatement *>(elseB), duplicated)) {
      // TODO: high-light both conditions, original and duplicated
      report(duplicated, DiagnosticsId::DI_DUPLICATE_IF_EXPR);
    }
  }
}

void CheckerVisitor::checkVariableMismatchForLoop(ForStatement *loop) {

  if (effectsOnly)
    return;

  const SQChar *varname = nullptr;
  Node *init = loop->initializer();
  Expr *cond = loop->condition();
  Expr *mod = loop->modifier();

  if (init) {
    if (init->op() == TO_ASSIGN) {
      Expr *l = static_cast<BinExpr *>(init)->lhs();
      if (l->op() == TO_ID)
        varname = l->asId()->id();
    }

    if (init->op() == TO_VAR) {
      VarDecl *decl = static_cast<VarDecl *>(init);
      varname = decl->name();
    }
  }

  if (varname && cond) {
    if (isBinaryArith(cond)) {
      BinExpr *bin = static_cast<BinExpr *>(cond);
      Expr *l = bin->lhs();
      Expr *r = bin->rhs();
      bool idUsed = false;

      if (l->op() == TO_ID) {
        if (strcmp(l->asId()->id(), varname) != 0) {
          if (r->op() != TO_ID || (strcmp(r->asId()->id(), varname) != 0)) {
            report(cond, DiagnosticsId::DI_MISMATCH_LOOP_VAR);
          }
        }
      }
    }
  }

  if (varname && mod) {
    if (isAssignExpr(mod)) {
      Expr *lhs = static_cast<BinExpr *>(mod)->lhs();
      if (lhs->op() == TO_ID) {
        if (strcmp(varname, lhs->asId()->id()) != 0) {
          report(mod, DiagnosticsId::DI_MISMATCH_LOOP_VAR);
        }
      }
    }

    if (mod->op() == TO_INC) {
      Expr *arg = static_cast<IncExpr *>(mod)->argument();
      if (arg->op() == TO_ID) {
        if (strcmp(varname, arg->asId()->id()) != 0) {
          report(mod, DiagnosticsId::DI_MISMATCH_LOOP_VAR);
        }
      }
    }
  }
}

void CheckerVisitor::checkUnterminatedLoop(LoopStatement *loop) {

  if (effectsOnly)
    return;

  LoopTerminatorCollector collector(true);
  Statement *body = loop->body();

  body->visit(&collector);

  if (collector.hasUnconditionalTerminator()) {
    const Statement *t = collector.terminator;
    assert(t);
    const char *type = nullptr;
    switch (t->op())
    {
    case TO_RETURN: type = "return"; break;
    case TO_THROW: type = "throw"; break;
    case TO_CONTINUE: type = "continue"; break;
    case TO_BREAK: type = "break"; break;
    default: assert(0); break;
    }

    assert(type);

    report(t, DiagnosticsId::DI_UNCOND_TERMINATED_LOOP, type);
  }
}

void CheckerVisitor::checkEmptyWhileBody(WhileStatement *loop) {
  if (effectsOnly)
    return;

  const Statement *body = unwrapBody(loop->body());

  if (body && body->op() == TO_EMPTY) {
    report(body, DiagnosticsId::DI_EMPTY_BODY, "while");
  }
}

void CheckerVisitor::checkEmptyThenBody(IfStatement *stmt) {
  if (effectsOnly)
    return;

  const Statement *thenStmt = unwrapBody(stmt->thenBranch());

  if (thenStmt && thenStmt->op() == TO_EMPTY) {
    report(thenStmt, DiagnosticsId::DI_EMPTY_BODY, "then");
  }
}

void CheckerVisitor::checkAssignedTwice(const Block *b) {
  if (effectsOnly)
    return;

  const auto &statements = b->statements();

  for (int32_t i = 0; i < int32_t(statements.size()) - 1; ++i) {
    const Expr *iexpr = unwrapExprStatement(statements[i]);

    if (iexpr && isAssignOp(iexpr->op())) {
      const BinExpr *iassgn = static_cast<const BinExpr *>(iexpr);
      const Expr *firstAssignee = iassgn->lhs();

      for (int32_t j = i + 1; j < statements.size(); ++j) {
        Statement *stmt = statements[j];
        Expr *jexpr = unwrapExprStatement(stmt);

        if (jexpr && jexpr->op() == TO_ASSIGN) {
          const BinExpr *jassgn = static_cast<const BinExpr *>(jexpr);

          if (_equalChecker.check(firstAssignee, jassgn->lhs())) {
            bool ignore = existsInTree(firstAssignee, jassgn->rhs());
            if (!ignore && firstAssignee->op() == TO_GETTABLE) {
              const GetTableExpr *getT = firstAssignee->asGetTable();
              ignore = indexChangedInTree(getT->key());
              if (!ignore) {
                for (int32_t m = i + 1; m < j; ++m) {
                  if (checkModification(getT->key(), statements[m])) {
                    ignore = true;
                    break;
                  }
                }
              }
            }

            if (!ignore) {
              report(jassgn, DiagnosticsId::DI_ASSIGNED_TWICE);
            }
          }
        }
        else if (terminateAssignSequence(firstAssignee, stmt)) {
          break;
        }
      }
    }
  }
}

bool CheckerVisitor::isCallResultShouldBeUtilized(const SQChar *name, const CallExpr *call) {
  if (!name)
    return false;

  if (nameLooksLikeResultMustBeUtilised(name)) {
    return true;
  }

  if (nameLooksLikeResultMustBeBoolean(name)) {
    const auto &args = call->arguments();

    if (args.size() != 1)
      return true;

    const Expr *arg = args[0];

    if (looksLikeBooleanExpr(arg)) {
      return false;
    }

    if (arg->op() == TO_ID) {
      if (nameLooksLikeResultMustBeBoolean(arg->asId()->id())) {
        return false;
      }
    }

    if (arg->op() == TO_CALL) {
      const CallExpr *acall = arg->asCallExpr();
      const Expr *callee = acall->callee();
      const SQChar *calleeName = nullptr;
      if (callee->op() == TO_ID) {
        calleeName = callee->asId()->id();
      }
      else if (callee->op() == TO_GETFIELD) {
        calleeName = callee->asGetField()->fieldName();
      }

      return isCallResultShouldBeUtilized(calleeName, acall) == false;
    }

    const Expr *evaled = maybeEval(arg);

    return looksLikeBooleanExpr(evaled) == false;
  }

  return false;
}

void CheckerVisitor::checkUnutilizedResult(const ExprStatement *s) {
  if (effectsOnly)
    return;

  const Expr *e = s->expression();

  if (e->op() == TO_CALL) {
    const CallExpr *c = static_cast<const CallExpr *>(e);
    const Expr *callee = c->callee();
    bool isCtor = false;
    const FunctionInfo *info = findFunctionInfo(callee, isCtor);
    // check only for functions because instances could have overrided () operator
    if (info) {
      const FunctionDecl *f = info->declaration;
      assert(f);

      if (f->op() == TO_CONSTRUCTOR) {
        report(e, DiagnosticsId::DI_NAME_LIKE_SHOULD_RETURN, "<constructor>");
      }
      else if (isCallResultShouldBeUtilized(f->name(), c)) {
        report(e, DiagnosticsId::DI_NAME_LIKE_SHOULD_RETURN, f->name());
      }
    }
    else if (isCtor) {
      report(e, DiagnosticsId::DI_NAME_LIKE_SHOULD_RETURN, "<constructor>");
    }

    return;
  }

  if (!isAssignExpr(e) && e->op() != TO_INC && e->op() != TO_NEWSLOT && e->op() != TO_DELETE) {
    report(s, DiagnosticsId::DI_UNUTILIZED_EXPRESSION);
  }
}

void CheckerVisitor::checkForgottenDo(const Block *b) {
  if (effectsOnly)
    return;

  const auto &statements = b->statements();

  for (int32_t i = 1; i < statements.size(); ++i) {
    Statement *prev = statements[i - 1];
    Statement *stmt = statements[i];

    if (stmt->op() == TO_WHILE && prev->op() == TO_BLOCK) {
      report(stmt, DiagnosticsId::DI_FORGOTTEN_DO);
    }
  }
}

void CheckerVisitor::checkUnreachableCode(const Block *block) {
  if (effectsOnly)
    return;

  const auto &statements = block->statements();

  for (int32_t i = 0; i < int32_t(statements.size()) - 1; ++i) {
    Statement *stmt = statements[i];
    Statement *next = statements[i + 1];
    if (isBlockTerminatorStatement(stmt->op())) {
      if (next->op() != TO_BREAK) {
        if (!onlyEmptyStatements(i + 1, statements)) {
          report(next, DiagnosticsId::DI_UNREACHABLE_CODE, terminatorOpToName(stmt->op()));
          break;
        }
      }
    }
  }
}

void CheckerVisitor::visitExprStatement(ExprStatement *stmt) {
  checkUnutilizedResult(stmt);

  Visitor::visitExprStatement(stmt);
}

void CheckerVisitor::visitBlock(Block *b) {
  checkForgottenDo(b);
  checkUnreachableCode(b);
  checkAssignedTwice(b);

  VarScope *thisScope = currentScope;
  VarScope blockScope(thisScope ? thisScope->owner : nullptr, thisScope);
  currentScope = &blockScope;

  nodeStack.push_back({ SST_NODE, b });

  for (Statement *s : b->statements()) {
    s->visit(this);
    blockScope.evalId += 1;
  }

  nodeStack.pop_back();

  blockScope.parent = nullptr;
  blockScope.checkUnusedSymbols(this);
  currentScope = thisScope;
}

void CheckerVisitor::visitForStatement(ForStatement *loop) {
  checkUnterminatedLoop(loop);
  checkVariableMismatchForLoop(loop);

  VarScope *trunkScope = currentScope;

  Node *init = loop->initializer();
  Expr *cond = loop->condition();
  Expr *mod = loop->modifier();

  VarScope *copyScope = trunkScope->copy(arena);
  VarScope loopScope(copyScope->owner, copyScope);
  currentScope = &loopScope;

  if (init) {
    init->visit(this);
  }

  if (cond) {
    cond->visit(this);
  }

  bool old = effectsOnly;
  effectsOnly = true;

  {
    VarScope *dummyScope = loopScope.copy(arena, false);
    BreakableScope bs(this, loop, dummyScope, nullptr); // null because we don't (??) interest in exit effect here
    currentScope = dummyScope;

    loop->body()->visit(this);

    if (mod)
      mod->visit(this);

    loopScope.merge(dummyScope);

    dummyScope->parent = nullptr;
    dummyScope->~VarScope();
  }

  effectsOnly = old;

  if (!old) {
    BreakableScope bs(this, loop, &loopScope, copyScope);
    currentScope = &loopScope;

    if (cond)
      speculateIfConditionHeuristics(cond, &loopScope, copyScope);

    loop->body()->visit(this);

    if (mod)
      mod->visit(this);
  }

  trunkScope->merge(copyScope);
  loopScope.checkUnusedSymbols(this);

  currentScope = trunkScope;
}

void CheckerVisitor::checkNullableContainer(const ForeachStatement *loop) {
  if (effectsOnly)
    return;

  if (isPotentiallyNullable(loop->container())) {
    report(loop->container(), DiagnosticsId::DI_POTENTIALLY_NULLABLE_CONTAINER);
  }
}

void CheckerVisitor::visitForeachStatement(ForeachStatement *loop) {
  checkUnterminatedLoop(loop);
  checkNullableContainer(loop);


  VarScope *trunkScope = currentScope;
  VarScope *copyScope = trunkScope->copy(arena);
  VarScope loopScope(trunkScope->owner, copyScope);
  currentScope = &loopScope;

  const VarDecl *idx = loop->idx();
  const VarDecl *val = loop->val();

  if (idx) {
    SymbolInfo *info = makeSymbolInfo(SK_FOREACH);
    ValueRef *v = makeValueRef(info);
    v->state = VRS_UNKNOWN;
    v->expression = nullptr;
    info->declarator.v = idx;
    info->ownedScope = &loopScope; //-V506
    declareSymbol(idx->name(), v);
  }

  if (val) {
    SymbolInfo *info = makeSymbolInfo(SK_FOREACH);
    ValueRef *v = makeValueRef(info);
    v->state = VRS_UNKNOWN;
    v->expression = nullptr;
    info->declarator.v = val;
    info->ownedScope = &loopScope; //-V506
    declareSymbol(val->name(), v);
  }

  StackSlot slot;
  slot.n = loop;
  slot.sst = SST_NODE;
  nodeStack.push_back(slot);

  loop->container()->visit(this);

  bool old = effectsOnly;
  effectsOnly = true;

  {
    VarScope *dummyScope = loopScope.copy(arena, false);
    BreakableScope bs(this, loop, dummyScope, nullptr); // null because we don't (??) interest in exit effect here
    currentScope = dummyScope;

    loop->body()->visit(this);

    loopScope.merge(dummyScope);

    dummyScope->parent = nullptr;
    dummyScope->~VarScope();
  }
  effectsOnly = old;

  if (!old) {
    BreakableScope bs(this, loop, &loopScope, copyScope);

    currentScope = &loopScope;
    loop->body()->visit(this);
  }

  nodeStack.pop_back();

  trunkScope->merge(copyScope);
  loopScope.checkUnusedSymbols(this);
  currentScope = trunkScope;
}

void CheckerVisitor::visitWhileStatement(WhileStatement *loop) {
  checkUnterminatedLoop(loop);
  checkEmptyWhileBody(loop);

  loop->condition()->visit(this);

  VarScope *trunkScope = currentScope;
  VarScope *loopScope = trunkScope->copy(arena);
  currentScope = loopScope;

  bool old = effectsOnly;
  effectsOnly = true;

  {
    BreakableScope bs(this, loop, loopScope, nullptr); // null because we don't (??) interest in exit effect here
    loop->body()->visit(this);
  }
  effectsOnly = old;

  if (!old) {
    BreakableScope bs(this, loop, loopScope, trunkScope);
    speculateIfConditionHeuristics(loop->condition(), loopScope, trunkScope);

    loop->body()->visit(this);
  }

  trunkScope->merge(loopScope);
  loopScope->~VarScope();

  currentScope = trunkScope;
}

void CheckerVisitor::visitDoWhileStatement(DoWhileStatement *loop) {
  checkUnterminatedLoop(loop);

  VarScope *trunkScope = currentScope;
  VarScope *loopScope = trunkScope->copy(arena);
  currentScope = loopScope;

  bool old = effectsOnly;
  effectsOnly = true;

  {
    BreakableScope bs(this, loop, loopScope, nullptr); // null because we don't (??) interest in exit effect here
    loop->body()->visit(this);
    loop->condition()->visit(this);
  }
  effectsOnly = old;

  if (!old) {
    BreakableScope bs(this, loop, loopScope, trunkScope);
    loop->body()->visit(this);
    loop->condition()->visit(this);
  }

  trunkScope->copyFrom(loopScope);
  loopScope->~VarScope();
  currentScope = trunkScope;
}

void CheckerVisitor::visitBreakStatement(BreakStatement *breakStmt) {
  VarScope *trunkScope = currentScope;
  BreakableScope *bs = breakScope;
  assert(bs);

  if (bs->kind != BSK_LOOP)
    return;

  if (bs->exitScope)
    bs->exitScope->mergeUnbalanced(trunkScope);
}

void CheckerVisitor::visitContinueStatement(ContinueStatement *continueStmt) {
  VarScope *trunkScope = currentScope;
  BreakableScope *bs = breakScope;
  assert(bs);

  while (bs->kind != BSK_LOOP)
    bs = bs->parent;

  assert(bs->loopScope);

  bs->loopScope->mergeUnbalanced(trunkScope);
}

const Id *extractReceiver(const Expr *e) {
  const Expr *last = e;

  for (;;) {
    e = deparen(e);

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

bool CheckerVisitor::detectTypeOfPattern(const Expr *expr, const Expr *&res_checkee, const LiteralExpr *&res_lit) {
  // Either of 'typeof checkee ==/!= "lit"' or 'type(checkee) ==/!= "lit"
  expr = deparen(expr);

  enum TreeOp op = expr->op(); // -V522

  if (op != TO_EQ && op != TO_NE)
    return false; // not typeof pattern

  const BinExpr *bin = expr->asBinExpr();

  const Expr *lhs = deparen(bin->lhs());
  const Expr *rhs = deparen(bin->rhs());

  const LiteralExpr *l_lit = lhs->op() == TO_LITERAL ? lhs->asLiteral() : nullptr; // -V522
  const LiteralExpr *r_lit = rhs->op() == TO_LITERAL ? rhs->asLiteral() : nullptr; // -V522

  if (!l_lit && !r_lit)
    return false;

  if (l_lit && r_lit)
    return false;

  const LiteralExpr *lit = l_lit ? l_lit : r_lit;

  if (lit->kind() != LK_STRING) // -V522
    return false;

  const Expr *checkExpr = lit == lhs ? rhs : lhs;

  if (checkExpr->op() == TO_TYPEOF) { // -V522
    const UnExpr *typeof = static_cast<const UnExpr *>(checkExpr);
    res_checkee = deparen(typeof->argument()); // -V522
    res_lit = lit;
    return true;
  }

  if (checkExpr->op() == TO_CALL) {
    // check for `type(check)` pattern

    const CallExpr *call = checkExpr->asCallExpr();
    const Expr *callee = maybeEval(call->callee());

    if (callee->op() != TO_ID || strcmp("type", callee->asId()->id()) != 0) {
      return false;
    }

    if (call->arguments().size() != 1)
      return false;

    res_checkee = deparen(call->arguments()[0]);
    res_lit = lit;
    return true;
  }

  return false;
}

bool CheckerVisitor::detectNullCPattern(enum TreeOp op, const Expr *cond, const Expr *&checkee) {

  // detect patterns like `(o?.x ?? D) <op> D` which implies o not null

  // (o?.f ?? D) != V -- then-branch implies `o` non-null
  // (o?.f ?? D) == V -- assume else-branch implies `o` non-null
  // (o?.f ?? D) > V -- then-b implies `o` non-null
  // (o?.f ?? D) < V -- then-b implies `o` non-null
  // (o?.f ?? D) >= V -- nothing could be said
  // (o?.f ?? D) <= V -- nothing

  if (op != TO_LT && op != TO_GT && op != TO_EQ && op != TO_NE) {
    return false;
  }

  const BinExpr *bin = cond->asBinExpr();

  const Expr *lhs = deparen(bin->lhs());
  const Expr *rhs = deparen(bin->rhs());

  if (lhs->op() != TO_NULLC && rhs->op() != TO_NULLC) // -V522
    return false;

  const BinExpr *nullc = lhs->op() == TO_NULLC ? lhs->asBinExpr() : rhs->asBinExpr();
  const Expr *V = lhs == nullc ? rhs : lhs;
  const Expr *D = deparen(nullc->rhs());
  const Expr *candidate = deparen(nullc->lhs());

  const Expr *reciever = extractReceiver(candidate);

  if (reciever == nullptr)
    return false;

  if (V->op() == TO_ID || _equalChecker.check(D, V)) { // -V522
    checkee = reciever;
    return true;
  }

  return false;
}

void CheckerVisitor::speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, bool inv) {

  std::unordered_set<const Expr *> visited;

  speculateIfConditionHeuristics(cond, thenScope, elseScope, visited, -1, 0, inv);
}

#define NULL_CHECK_F 1

void CheckerVisitor::speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, std::unordered_set<const Expr *> &visited, int32_t evalId, unsigned flags, bool inv) {
  cond = deparen(cond);

  if (visited.find(cond) != visited.end()) {
    return;
  }

  visited.emplace(cond);

  enum TreeOp op = cond->op();

  VarScope *thisScope = currentScope;

  if (op == TO_NOT) {
    const UnExpr *u = static_cast<const UnExpr *>(cond);
    return speculateIfConditionHeuristics(u->argument(), thenScope, elseScope, visited, evalId, flags, !inv);
  }

  bool invertLR = false;
  if (op == TO_NE) {
    op = TO_EQ;
    inv = !inv;
  } else if (op == TO_OROR) {
    // apply a || b <==> !(!a && !b)
    op = TO_ANDAND;
    invertLR = true;
    inv = !inv;
  }

  if (inv) {
    std::swap(thenScope, elseScope);
  }

  const Expr *nullcCheckee = nullptr;
  if (detectNullCPattern(op, cond, nullcCheckee)) {
    // (o?.f ?? D) == V -- assume else-branch implies `o` non-null
    // (o?.f ?? D) > V -- then-b implies `o` non-null
    // (o?.f ?? D) < V -- then-b implies `o` non-null

    if (op == TO_EQ) {
      if (elseScope) {
        currentScope = elseScope;
        setValueFlags(nullcCheckee, 0, RT_NULL);
      }
    }
    else {
      assert(op == TO_LT || op == TO_GT);
      if (thenScope) {
        currentScope = thenScope;
        setValueFlags(nullcCheckee, 0, RT_NULL);
      }
    }

    currentScope = thisScope; // -V519
    return;
  }

  if (evalId == -1 && op == TO_CALL && thenScope) {
    // if condition looks like `if (foo(x)) x.bar()` consider call as null check
    currentScope = thenScope;
    const CallExpr *call = cond->asCallExpr();
    for (auto parg : call->arguments()) {
      auto arg = deparen(parg);
      if (arg->op() == TO_ID) { // -V522
        setValueFlags(arg, 0, RT_NULL);
      }
    }

    const Expr *callee = extractReceiver(call->callee());
    if (callee) {
      setValueFlags(callee, 0, RT_NULL);
    }

    currentScope = thisScope; // -V519
    return;
  }

  if (op == TO_ID) {
    if (thenScope && evalId < 0) {
      // set iff it was explicit check like `if (o) { .. }`
      // otherwise there could be complexities, see intersected_assignment.nut
      currentScope = thenScope;
      setValueFlags(cond, 0, RT_NULL);
      currentScope = thisScope;
    }

    if (elseScope && evalId < 0 && (flags & NULL_CHECK_F)) {
      // set NULL iff it was explicit null check `if (o == null) { ... }` otherwise it could not be null, see w233_inc_in_for.nut
      currentScope = elseScope;
      setValueFlags(cond, RT_NULL, 0);
      currentScope = thisScope;
    }

    int32_t evalIndex = -1;
    const Expr *eval = maybeEval(cond, evalIndex);

    if (eval != cond) {
      // let cond = x != null
      // if (cond) { ... }
      speculateIfConditionHeuristics(eval, thenScope, elseScope, visited, evalIndex, flags, false);
    }
    return;
  }

  if (op == TO_GETTABLE || op == TO_GETFIELD) {
    // x?[y]
    const AccessExpr *acc = static_cast<const AccessExpr *>(cond);
    const Expr *reciever = extractReceiver(deparen(acc->receiver()));

    if (reciever && thenScope) {
      currentScope = thenScope;
      setValueFlags(reciever, 0, RT_NULL);
    }

    if (op == TO_GETTABLE) {
      const Expr *key = deparen(acc->asGetTable()->key());
      if (thenScope) {
        currentScope = thenScope;
        setValueFlags(key, 0, RT_NULL);
      }
    }

    currentScope = thisScope; // -V519

    return;
  }

  if (op == TO_EQ) {
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = deparen(bin->lhs());
    const Expr *rhs = deparen(bin->rhs());

    const Id *lhs_id = lhs->op() == TO_ID ? lhs->asId() : nullptr; // -V522
    const Id *rhs_id = rhs->op() == TO_ID ? rhs->asId() : nullptr; // -V522

    const LiteralExpr *lhs_lit = lhs->op() == TO_LITERAL ? lhs->asLiteral() : nullptr; // -V522
    const LiteralExpr *rhs_lit = rhs->op() == TO_LITERAL ? rhs->asLiteral() : nullptr; // -V522

    const LiteralExpr *lit = lhs_lit ? lhs_lit : rhs_lit;
    const Expr *testee = lit == lhs ? rhs : lhs;

    if (lit && lit->kind() == LK_NULL) { // -V522
      speculateIfConditionHeuristics(testee, elseScope, thenScope, visited, evalId, flags | NULL_CHECK_F, false);
      return;
    }
  }

  if (isRelationOperator(op)) {
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = deparen(bin->lhs());
    const Expr *rhs = deparen(bin->rhs());

    const Id *lhs_id = lhs->op() == TO_ID ? lhs->asId() : nullptr; // -V522
    const Id *rhs_id = rhs->op() == TO_ID ? rhs->asId() : nullptr; // -V522

    if (thenScope) {
      currentScope = thenScope;
      if (lhs_id) {
        setValueFlags(lhs_id, 0, RT_NULL);
      }
      if (rhs_id) {
        setValueFlags(rhs_id, 0, RT_NULL);
      }
      currentScope = thisScope;
    }
    return;
  }

  const LiteralExpr *typeLit = nullptr;
  const Expr *typeCheckee = nullptr;

  if (detectTypeOfPattern(cond, typeCheckee, typeLit)) {
    assert(typeCheckee && typeLit);

    const Id *checkeeId = extractReceiver(typeCheckee);
    if (!checkeeId)
      return;

    bool nullCheck = strcmp(typeLit->s(), _SC("null")) == 0; // todo: should it be more precise in case of avaliable names
    bool accessCheck = typeCheckee != checkeeId;

    // if it's null check we could know for sure that true-branch implies NULL and false-branch implies NON_NULL
    // if it's not a null check we could only guarantee that true-branch implies NON_NULL
    unsigned pt = 0, nt = 0, pe = 0, ne = 0;

    if (nullCheck) {
      if (accessCheck) {
        ne = RT_NULL;
      }
      else {
        pt = RT_NULL;
        ne = RT_NULL;
      }
    }
    else {
      nt = RT_NULL;
    }

    if (thenScope) {
      currentScope = thenScope;
      setValueFlags(checkeeId, pt, nt);
    }
    if (elseScope) {
      currentScope = elseScope;
      setValueFlags(checkeeId, pe, ne);
    }

    currentScope = thisScope; // -V519

    return;
  }

  if (op == TO_ANDAND) {
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = bin->lhs();
    const Expr *rhs = bin->rhs();

    VarScope *lhsEScope = nullptr;
    VarScope *rhsEScope = nullptr;

    // In '&&' operator all effects of the left-hand side work for the right side as well
    if (elseScope) {
      lhsEScope = elseScope->copy(arena);
      rhsEScope = elseScope->copy(arena);
    }

    speculateIfConditionHeuristics(lhs, thenScope, lhsEScope, visited, evalId, flags, invertLR);
    speculateIfConditionHeuristics(rhs, thenScope, rhsEScope, visited, evalId, flags, invertLR);

    if (elseScope) {
      // In contrast in `false`-branch there is no such integral effect, all we could say is something common of both side so
      // to compute that `common` part intersect lhs with rhs
      lhsEScope->intersectScopes(rhsEScope); // -V522
      elseScope->copyFrom(lhsEScope);

      lhsEScope->~VarScope();
      rhsEScope->~VarScope();
    }

    return;
  }

  if (op == TO_INSTANCEOF || op == TO_IN) {
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = bin->lhs();
    const Expr *rhs = bin->rhs();

    const Expr *checkee = extractReceiver(lhs);

    if (checkee && thenScope) {
      currentScope = thenScope;
      setValueFlags(checkee, 0, RT_NULL);
    }

    if (op == TO_IN) {
      checkee = extractReceiver(rhs);
      if (checkee && thenScope) {
        currentScope = thenScope;
        setValueFlags(checkee, 0, RT_NULL);
      }
    }

    currentScope = thisScope; // -V519

    return;
  }
}

static bool isFallThroughStatemtnt(const Statement *stmt) {
  enum TreeOp op = stmt->op();

  return op != TO_RETURN && op != TO_THROW && op != TO_BREAK && op != TO_CONTINUE;
}

static bool isFallThroughBranch(const Statement *stmt) {
  if (stmt->op() != TO_BLOCK)
    return isFallThroughStatemtnt(stmt);

  const Block *blk = stmt->asBlock();

  for (const Statement *s : blk->statements()) {
    if (!isFallThroughStatemtnt(s))
      return false;
  }

  return true;

}

void CheckerVisitor::visitIfStatement(IfStatement *ifstmt) {
  checkEmptyThenBody(ifstmt);
  checkDuplicateIfConditions(ifstmt);
  checkDuplicateIfBranches(ifstmt);
  checkAlwaysTrueOrFalse(ifstmt->condition());

  VarScope *trunkScope = currentScope;
  VarScope *thenScope = trunkScope->copy(arena);
  VarScope *dummyScope = trunkScope->copy(arena);
  VarScope *elseScope = ifstmt->elseBranch() ? trunkScope->copy(arena) : nullptr;

  nodeStack.push_back({ SST_NODE, ifstmt });

  currentScope = dummyScope;

  ifstmt->condition()->visit(this);

  currentScope = trunkScope;

  // pass fall-through branch to not lose effects
  speculateIfConditionHeuristics(ifstmt->condition(), thenScope, elseScope ? elseScope : trunkScope);

  currentScope = thenScope;

  ifstmt->thenBranch()->visit(this);

  bool isThenFT = isFallThroughBranch(ifstmt->thenBranch());
  bool isElseFT = true;

  if (ifstmt->elseBranch()) {
    currentScope = elseScope;
    ifstmt->elseBranch()->visit(this);
    isElseFT = isFallThroughBranch(ifstmt->elseBranch());
  }

  nodeStack.pop_back();

  if (isThenFT && isElseFT) {
    if (elseScope) {
      thenScope->merge(elseScope);
      trunkScope->copyFrom(thenScope);
    }
    else {
      trunkScope->merge(thenScope);
    }
  }
  else if (isThenFT) {
    trunkScope->copyFrom(thenScope);
  }
  else if (elseScope && isElseFT) {
    trunkScope->copyFrom(elseScope);
  }

  if (elseScope)
    elseScope->~VarScope();

  thenScope->~VarScope();
  currentScope = trunkScope;
}

void CheckerVisitor::visitSwitchStatement(SwitchStatement *swtch) {
  checkDuplicateSwitchCases(swtch);
  checkMissedBreak(swtch);

  Expr *expr = swtch->expression();
  expr->visit(this);

  auto &cases = swtch->cases();
  VarScope *trunkScope = currentScope;

  BreakableScope breakScope(this, swtch); // -V688

  std::vector<VarScope *> scopes;

  for (auto &c : cases) {
    c.val->visit(this);
    VarScope *caseScope = trunkScope->copy(arena);
    currentScope = caseScope;
    c.stmt->visit(this);
    scopes.push_back(caseScope);
  }

  if (swtch->defaultCase().stmt) {
    VarScope *defaultScope = trunkScope->copy(arena);
    currentScope = defaultScope;
    swtch->defaultCase().stmt->visit(this);
    scopes.push_back(defaultScope);
  }

  for (VarScope *s : scopes) {
    trunkScope->merge(s);
    s->~VarScope();
  }

  currentScope = trunkScope;
}

void CheckerVisitor::visitTryStatement(TryStatement *tryStmt) {

  Statement *t = tryStmt->tryStatement();
  Id *id = tryStmt->exceptionId();
  Statement *c = tryStmt->catchStatement();

  VarScope *trunkScope = currentScope;
  VarScope *tryScope = trunkScope->copy(arena);
  currentScope = tryScope;
  t->visit(this);

  VarScope *copyScope = trunkScope->copy(arena);
  VarScope catchScope(copyScope->owner, copyScope);
  currentScope = &catchScope;
  SymbolInfo *info = makeSymbolInfo(SK_EXCEPTION);
  ValueRef *v = makeValueRef(info);
  v->state = VRS_UNKNOWN;
  v->expression = nullptr;
  info->declarator.x = id;
  info->ownedScope = &catchScope; //-V506

  declareSymbol(id->id(), v);

  id->visit(this);
  c->visit(this);

  tryScope->merge(copyScope);
  trunkScope->copyFrom(tryScope);

  tryScope->~VarScope();
  currentScope = trunkScope;
}

const SQChar *CheckerVisitor::findSlotNameInStack(const Decl *decl) {
  auto it = nodeStack.rbegin();
  auto ie = nodeStack.rend();

  while (it != ie) {
    auto slot = *it;
    if (slot.sst == SST_NODE) {
      const Node *n = slot.n;
      if (n->op() == TO_NEWSLOT) {
        const BinExpr *bin = static_cast<const BinExpr *>(n);
        Expr *lhs = bin->lhs();
        Expr *rhs = bin->rhs();
        if (rhs->op() == TO_DECL_EXPR) {
          const DeclExpr *de = static_cast<const DeclExpr *>(rhs);
          if (de->declaration() == decl) {
            if (lhs->op() == TO_LITERAL) {
              if (lhs->asLiteral()->kind() == LK_STRING) {
                return lhs->asLiteral()->s();
              }
            }
          }
          return nullptr;
        }
      }
    }
    else {
      assert(slot.sst == SST_TABLE_MEMBER);
      Expr *lhs = slot.member->key;
      Expr *rhs = slot.member->value;
      if (rhs->op() == TO_DECL_EXPR) {
        const DeclExpr *de = static_cast<const DeclExpr *>(rhs);
        if (de->declaration() == decl) {
          if (lhs->op() == TO_LITERAL) {
            if (lhs->asLiteral()->kind() == LK_STRING) {
              return lhs->asLiteral()->s();
            }
          }
        }
        return nullptr;
      }
    }
    ++it;
  }

  return nullptr;
}

void CheckerVisitor::checkFunctionReturns(FunctionDecl *func) {
  if (effectsOnly)
    return;

  const SQChar *name = func->name();

  if (!name || name[0] == '(') {
    name = findSlotNameInStack(func);
  }

  bool dummy;
  unsigned returnFlags = FunctionReturnTypeEvaluator().compute(func->body(), dummy);

  bool reported = false;

  if (returnFlags & ~(RT_BOOL | RT_UNRECOGNIZED | RT_FUNCTION_CALL)) {
    if (nameLooksLikeResultMustBeBoolean(name)) {
      report(func, DiagnosticsId::DI_NAME_RET_BOOL, name);
      reported = true;
    }
  }

  if (!!(returnFlags & RT_NOTHING) &&
    !!(returnFlags & (RT_NUMBER | RT_STRING | RT_TABLE | RT_CLASS | RT_ARRAY | RT_CLOSURE | RT_UNRECOGNIZED | RT_THROW)))
  {
    if ((returnFlags & RT_THROW) == 0)
      report(func, DiagnosticsId::DI_NOT_ALL_PATH_RETURN_VALUE);
    reported = true;
  }
  else if (returnFlags)
  {
    unsigned flagsDiff = returnFlags & ~(RT_THROW | RT_NOTHING | RT_NULL | RT_UNRECOGNIZED | RT_FUNCTION_CALL);
    if (flagsDiff)
    {
      bool powerOfTwo = !(flagsDiff & (flagsDiff - 1));
      if (!powerOfTwo)
      {
        report(func, DiagnosticsId::DI_RETURNS_DIFFERENT_TYPES);
        reported = true;
      }
    }
  }

  if (!reported) {
    if (!!(returnFlags & RT_NOTHING) && nameLooksLikeFunctionMustReturnResult(name)) {
      report(func, DiagnosticsId::DI_NAME_EXPECTS_RETURN, name);
    }
  }
}

void CheckerVisitor::checkAccessNullable(const DestructuringDecl *dd) {
  if (effectsOnly)
    return;

  const Expr *i = dd->initiExpression();
  const Expr *initializer = i;

  if (isPotentiallyNullable(initializer)) {
    bool allHasDefault = true;

    for (auto d : dd->declarations()) {
      if (d->initializer() == nullptr) {
        allHasDefault = false;
        break;
      }
    }

    if (!allHasDefault) {
      report(dd, DiagnosticsId::DI_ACCESS_POT_NULLABLE, i->op() == TO_ID ? i->asId()->id() : "expression", "container");
    }
  }
}

void CheckerVisitor::checkAccessNullable(const AccessExpr *acc) {
  if (effectsOnly)
    return;

  const Expr *r = acc->receiver();

  if (!isSafeAccess(acc) && isPotentiallyNullable(r)) {
    report(r, DiagnosticsId::DI_ACCESS_POT_NULLABLE, "expression", "container");
  }
}

void CheckerVisitor::checkEnumConstUsage(const GetFieldExpr *acc) {
  if (effectsOnly)
    return;

  SQChar buffer[64] = { 0 };
  const SQChar *receiverName = computeNameRef(acc->receiver(), buffer, sizeof buffer);

  if (!receiverName)
    return;

  const ValueRef *enumV = findValueInScopes(receiverName);
  if (!enumV || enumV->info->kind != SK_ENUM)
    return;

  const SQChar *fqn = enumFqn(arena, receiverName, acc->fieldName());
  const ValueRef *constV = findValueInScopes(fqn);

  if (!constV) {
    // very suspeccious
    return;
  }

  constV->info->used = true;
}

void CheckerVisitor::visitTableDecl(TableDecl *table) {
  for (auto &member : table->members()) {
    StackSlot slot;
    slot.sst = SST_TABLE_MEMBER;
    slot.member = &member;

    checkKeyNameMismatch(member.key, member.value);

    nodeStack.push_back(slot);
    member.key->visit(this);
    member.value->visit(this);
    nodeStack.pop_back();
  }
}

void CheckerVisitor::visitClassDecl(ClassDecl *klass) {
  nodeStack.push_back({ SST_NODE, klass });

  if (klass->classKey())
    klass->classKey()->visit(this);

  if (klass->classBase())
    klass->classBase()->visit(this);

  visitTableDecl(klass);

  nodeStack.pop_back();
}

void CheckerVisitor::visitFunctionDecl(FunctionDecl *func) {
  VarScope *parentScope = currentScope;
  VarScope *copyScope = parentScope->copy(arena, true);
  VarScope functionScope(func, copyScope);

  SymbolInfo *info = makeSymbolInfo(SK_FUNCTION);
  ValueRef *v = makeValueRef(info);
  v->state = VRS_DECLARED;
  v->expression = nullptr;
  info->declarator.f = func;
  info->ownedScope = currentScope;
  info->used = true;

  declareSymbol(func->name(), v);

  pushFunctionScope(&functionScope, func);

  FunctionInfo *oldInfo = currentInfo;
  FunctionInfo *newInfo = functionInfoMap[func];

  currentInfo = newInfo;
  assert(newInfo);

  checkFunctionReturns(func);

  Visitor::visitFunctionDecl(func);

  if (oldInfo) {
    oldInfo->joinModifiable(newInfo);
  }

  functionScope.checkUnusedSymbols(this);

  currentInfo = oldInfo;
  currentScope = parentScope;
}

ValueRef *CheckerVisitor::findValueInScopes(const SQChar *ref) {
  if (!ref)
    return nullptr;

  VarScope *current = currentScope;
  VarScope *s = current;

  while (s) {
    auto &symbols = s->symbols;
    auto it = symbols.find(ref);
    if (it != symbols.end()) {
      return it->second;
    }

    s = s->parent;
  }

  return nullptr;
}

void CheckerVisitor::applyAssignmentToScope(const BinExpr *bin) {
  assert(bin->op() == TO_ASSIGN || bin->op() == TO_INEXPR_ASSIGN);

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  if (lhs->op() != TO_ID)
    return;

  const SQChar *name = lhs->asId()->id();
  ValueRef *v = findValueInScopes(name);

  if (!v) {
    // TODO: what if declarator == null
    SymbolInfo *info = makeSymbolInfo(SK_VAR);
    v = makeValueRef(info);
    currentScope->symbols[name] = v;
    info->ownedScope = currentScope;
    info->declarator.v = nullptr;
    if (currentInfo) {
      currentInfo->addModifiable(name, info->ownedScope->owner);
    }
  }

  v->expression = rhs;
  v->state = VRS_EXPRESSION;
  v->flagsNegative = v->flagsPositive = 0;
  v->lowerBound.kind = v->upperBound.kind = VBK_UNKNOWN;
  v->assigned = true;
  v->lastAssigneeScope = currentScope;
  v->evalIndex = currentScope->evalId;

  if (isPotentiallyNullable(rhs)) {
    v->flagsPositive |= RT_NULL;
  }

}

void CheckerVisitor::applyAssignEqToScope(const BinExpr *bin) {
  assert(TO_PLUSEQ <= bin->op() && bin->op() <= TO_MODEQ);

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  SQChar buffer[128] = { 0 };
  const SQChar *name = computeNameRef(lhs, buffer, sizeof buffer);
  if (!name)
    return;

  ValueRef *v = findValueInScopes(name);

  if (v) {
    if (currentInfo) {
      currentInfo->addModifiable(name, v->info->ownedScope->owner);
    }
    v->kill();
    v->evalIndex = currentScope->evalId;
  }
}

void CheckerVisitor::applyBinaryToScope(const BinExpr *bin) {
  switch (bin->op())
  {
  case TO_ASSIGN:
  case TO_INEXPR_ASSIGN:
    return applyAssignmentToScope(bin);
    //return applyNewslotToScope(bin);
  case TO_PLUSEQ:
  case TO_MINUSEQ:
  case TO_MULEQ:
  case TO_DIVEQ:
  case TO_MODEQ:
    return applyAssignEqToScope(bin);
  default:
    break;
  }
}

int32_t CheckerVisitor::computeNameLength(const Expr *e) {
  switch (e->op()) // -V522
  {
  case TO_GETFIELD: return computeNameLength(e->asGetField());
  //case TO_GETTABLE: return computeNameLength(e->asGetTable());
  case TO_ID: return strlen(e->asId()->id());
  case TO_ROOT: return sizeof rootName;
  case TO_BASE: return sizeof baseName;
    // TODO:
  default:
    return -1;
  }
}

void CheckerVisitor::computeNameRef(const Expr *e, SQChar *b, int32_t &ptr, int32_t size) {
  switch (e->op())
  {
  case TO_GETFIELD: return computeNameRef(e->asGetField(), b, ptr, size);
  //case TO_GETTABLE: return computeNameRef(lhs->asGetTable());
  case TO_ID: {
    int32_t l = snprintf(&b[ptr], size - ptr, "%s", e->asId()->id());
    ptr += l;
    break;
  }
  case TO_ROOT:
    snprintf(&b[ptr], size - ptr, "%s", rootName);
    ptr += sizeof rootName;
    break;
  case TO_BASE:
    snprintf(&b[ptr], size - ptr, "%s", baseName);
    ptr += sizeof baseName;
    break;
  default:
    assert(0);
  }
}

const SQChar *CheckerVisitor::computeNameRef(const Expr *lhs, SQChar *buffer, size_t bufferSize) {
  int32_t length = computeNameLength(lhs);
  if (length < 0)
    return nullptr;

  SQChar *result = buffer;

  if (!result || bufferSize < (length + 1)) {
    result = (SQChar *)arena->allocate(length + 1);
  }

  int32_t ptr = 0;
  computeNameRef(lhs, result, ptr, length + 1);
  assert(ptr == length);
  return result;
}

void CheckerVisitor::setValueFlags(const Expr *lvalue, unsigned pf, unsigned nf) {

  SQChar buffer[128] = { 0 };

  const SQChar *name = computeNameRef(lvalue, buffer, sizeof buffer);

  if (!name)
    return;

  ValueRef *v = findValueInScopes(name);

  if (v) {
    v->flagsPositive |= pf;
    v->flagsPositive &= ~nf;
    v->flagsNegative |= nf;
    v->flagsNegative &= ~pf;
  }
}

const ValueRef *CheckerVisitor::findValueForExpr(const Expr *e) {
  e = deparen(e);

  SQChar buffer[128] = { 0 };

  const SQChar *n = computeNameRef(e, buffer, sizeof buffer);

  if (!n) {
    return nullptr;
  }

  return findValueInScopes(n);
}

bool CheckerVisitor::isSafeAccess(const AccessExpr *acc) {
  if (acc->isNullable())
    return true;

  const Expr *recevier = acc->receiver();

  if (recevier->isAccessExpr()) {
    return isSafeAccess(recevier->asAccessExpr());
  }

  return false;
}

bool CheckerVisitor::isPotentiallyNullable(const Expr *e) {
  std::unordered_set<const Expr *> v;

  return isPotentiallyNullable(e, v);
}

bool CheckerVisitor::isPotentiallyNullable(const Expr *e, std::unordered_set<const Expr *> &visited) {

  e = deparen(e);

  auto existed = visited.find(e);
  if (existed != visited.end()) {
    return false;
  }

  visited.emplace(e);

  const ValueRef *v = findValueForExpr(e);

  if (v) {
    if (v->flagsPositive & RT_NULL)
      return true;

    if (v->flagsNegative & RT_NULL)
      return false;

    if (v->state == VRS_UNDEFINED || v->state == VRS_PARTIALLY)
      return true;
  }

  e = maybeEval(e);

  if (e->op() == TO_LITERAL) {
    const LiteralExpr *l = e->asLiteral();
    return l->kind() == LK_NULL;
  }

  if (e->isAccessExpr()) {
    if (e->asAccessExpr()->isNullable()) {
      return true;
    }
  }

  if (e->op() == TO_CALL) {
    const CallExpr *call = static_cast<const CallExpr *>(e);
    if (call->isNullable()) {
      return true;
    }

    const SQChar *funcName = nullptr;
    const Expr *callee = call->callee();

    if (callee->op() == TO_ID) {
      funcName = callee->asId()->id();
    }
    else if (callee->op() == TO_GETFIELD) {
      funcName = callee->asGetField()->fieldName();
    }

    if (funcName) {
      if (canFunctionReturnNull(funcName)) {
        return true;
      }
    }
  }

  if (e->op() == TO_NULLC) {
    return isPotentiallyNullable(static_cast<const BinExpr *>(e)->rhs(), visited);
  }

  v = findValueForExpr(e);

  if (v) {
    switch (v->state)
    {
    case VRS_EXPRESSION:
    case VRS_INITIALIZED:
      return isPotentiallyNullable(v->expression, visited);
    case VRS_UNDEFINED:
    case VRS_PARTIALLY:
      return true;
    default:
      return false;
    }
  }

  return false;
}

bool CheckerVisitor::couldBeString(const Expr *e) {
  if (!e)
    return false;

  if (e->op() == TO_LITERAL) {
    return e->asLiteral()->kind() == LK_STRING;
  }

  if (e->op() == TO_NULLC) {
    const BinExpr *b = static_cast<const BinExpr *>(e);
    if (b->rhs()->op() == TO_LITERAL && b->rhs()->asLiteral()->kind() == LK_STRING)
      return couldBeString(b->lhs());
  }

  if (e->op() == TO_CALL) {
    const SQChar *name = nullptr;
    const Expr *callee = static_cast<const CallExpr *>(e)->callee();

    if (callee->op() == TO_ID)
      name = callee->asId()->id();
    else if (callee->op() == TO_GETFIELD)
      name = callee->asGetField()->fieldName();

    if (name) {
      return nameLooksLikeResultMustBeString(name) || strcmp(name, "loc") == 0;
    }
  }

  return false;
}

const Expr *CheckerVisitor::maybeEval(const Expr *e) {
  int32_t dummy = -1;
  return maybeEval(e, dummy);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId) {
  std::unordered_set<const Expr *> visited;
  return maybeEval(e, evalId, visited);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited) {

  if (visited.find(e) != visited.end())
    return e;

  visited.emplace(e);

  e = deparen(e);
  const ValueRef *v = findValueForExpr(e);

  if (!v) {
    return e;
  }

  evalId = v->evalIndex;
  if (v->hasValue()) {
    return maybeEval(v->expression, evalId, visited);
  }
  else {
    return e;
  }
}

const SQChar *CheckerVisitor::findFieldName(const Expr *e) {
  if (e->op() == TO_ID)
    return e->asId()->id();

  if (e->op() == TO_GETFIELD)
    return e->asGetField()->fieldName();

  const ValueRef *v = findValueForExpr(e);

  if (v && v->expression && v->expression->op() == TO_DECL_EXPR) {
    const Decl *d = static_cast<const DeclExpr *>(v->expression)->declaration();
    if (d->op() == TO_FUNCTION) {
      return static_cast<const FunctionDecl *>(d)->name();
    }
  }

  return "";
}

int32_t CheckerVisitor::computeNameLength(const GetFieldExpr *acc) {
  int32_t size = computeNameLength(acc->receiver());

  if (size < 0) {
    return size;
  }

  size += 1;
  size += strlen(acc->fieldName());
  return size;
}

void CheckerVisitor::computeNameRef(const GetFieldExpr *access, SQChar *b, int32_t &ptr, int32_t size) {
  computeNameRef(access->receiver(), b, ptr, size);
  b[ptr++] = '.';
  int32_t l = snprintf(&b[ptr], size - ptr, "%s", access->fieldName());
  ptr += l;
}

const FunctionInfo *CheckerVisitor::findFunctionInfo(const Expr *e, bool &isCtor) {
  const Expr *ee = maybeEval(e);

  if (ee->op() == TO_DECL_EXPR) { //-V522
    const Decl *decl = ee->asDeclExpr()->declaration();
    if (decl->op() == TO_FUNCTION || decl->op() == TO_CLASS) {
      return functionInfoMap[static_cast<const FunctionDecl *>(decl)];
    }
  }

  SQChar buffer[128] = { 0 };

  const SQChar *name = computeNameRef(ee, buffer, sizeof buffer);

  if (!name)
    return nullptr;

  const ValueRef *v = findValueInScopes(name);

  if (!v || !v->hasValue())
    return nullptr;

  const Expr *expr = maybeEval(v->expression);

  if (expr->op() != TO_DECL_EXPR) // -V522
    return nullptr;

  const Decl *decl = static_cast<const DeclExpr *>(expr)->declaration();

  if (decl->op() == TO_FUNCTION || decl->op() == TO_CONSTRUCTOR) {
    return functionInfoMap[static_cast<const FunctionDecl *>(decl)];
  }
  else if (decl->op() == TO_CLASS) {
    const ClassDecl *klass = static_cast<const ClassDecl *>(decl);
    isCtor = true;
    for (auto &m : klass->members()) {
      const Expr *me = m.value;
      if (me->op() == TO_DECL_EXPR) {
        const Decl *de = static_cast<const DeclExpr *>(m.value)->declaration();
        if (de->op() == TO_CONSTRUCTOR) {
          return functionInfoMap[static_cast<const FunctionDecl *>(de)];
        }
      }
    }
  }

  return nullptr;
}

void CheckerVisitor::applyKnownInvocationToScope(const ValueRef *value) {
  const FunctionInfo *info = nullptr;

  if (value->hasValue()) {
    const Expr *expr = maybeEval(value->expression);
    assert(expr != nullptr);

    if (expr->op() == TO_DECL_EXPR) { //-V522
      const Decl *decl = static_cast<const DeclExpr *>(expr)->declaration();
      if (decl->op() == TO_FUNCTION || decl->op() == TO_CONSTRUCTOR) {
        info = functionInfoMap[static_cast<const FunctionDecl *>(decl)];
      }
      else if (decl->op() == TO_CLASS) {
        const ClassDecl *klass = static_cast<const ClassDecl *>(decl);
        for (auto &m : klass->members()) {
          const Expr *me = m.value;
          if (me->op() == TO_DECL_EXPR) {
            const Decl *de = static_cast<const DeclExpr *>(m.value)->declaration();
            if (de->op() == TO_CONSTRUCTOR) {
              info = functionInfoMap[static_cast<const FunctionDecl *>(de)];
              break;
            }
          }
        }
      }
      else {
        applyUnknownInvocationToScope();
        return;
      }
    }
  }

  if (!info) {
    // probably it is class constructor
    return;
  }

  for (auto s : info->modifible) {
    VarScope *scope = currentScope->findScope(s.owner);
    if (!scope)
      continue;

    auto it = scope->symbols.find(s.name);
    if (it != scope->symbols.end()) {
      if (it->second->isConstant())
        continue;

      if (currentInfo) {
        currentInfo->addModifiable(it->first, it->second->info->ownedScope->owner);
      }
      it->second->kill();
    }
  }
}

void CheckerVisitor::applyUnknownInvocationToScope() {
  VarScope *s = currentScope;

  while (s) {
    auto &symbols = s->symbols;
    for (auto &sym : symbols) {
      if (sym.second->isConstant())
        continue;
      if (currentInfo) {
        currentInfo->addModifiable(sym.first, sym.second->info->ownedScope->owner);
      }
      sym.second->kill();
    }
    s = s->parent;
  }
}

void CheckerVisitor::applyCallToScope(const CallExpr *call) {
  const Expr *callee = deparen(call->callee());

  if (callee->op() == TO_ID) { // -V522
    const Id *calleeId = callee->asId();
    //const NameRef ref(nullptr, calleeId->id());
    const ValueRef *value = findValueInScopes(calleeId->id());
    if (value) {
      applyKnownInvocationToScope(value);
    }
    else {
      // unknown invocation by pure Id points to some external
      // function which could not modify any scoped value
    }
  }
  else if (callee->op() == TO_GETFIELD) {
    SQChar buffer[128] = { 0 };
    const SQChar *ref = computeNameRef(callee, buffer, sizeof buffer);
    const ValueRef *value = findValueInScopes(ref);
    if (value) {
      applyKnownInvocationToScope(value);
    }
    else if (!ref || strncmp(ref, rootName, sizeof rootName) != 0) {
      // we don't know what exactly is being called so assume the most conservative case
      applyUnknownInvocationToScope();
    }
  }
  else {
    // unknown invocation so everything could be modified
    applyUnknownInvocationToScope();
  }
}

void CheckerVisitor::pushFunctionScope(VarScope *functionScope, const FunctionDecl *decl) {

  FunctionInfo *info = functionInfoMap[decl];

  if (!info) {
    info = makeFunctionInfo(decl, currentScope->owner);
    functionInfoMap[decl] = info;
  }

  currentScope = functionScope;
}

void CheckerVisitor::declareSymbol(const SQChar *nameRef, ValueRef *v) {
  currentScope->symbols[nameRef] = v;
}

void CheckerVisitor::visitParamDecl(ParamDecl *p) {
  Visitor::visitParamDecl(p);

  const Expr *dv = p->defaultValue();

  SymbolInfo *info = makeSymbolInfo(SK_PARAM);
  ValueRef *v = makeValueRef(info);
  v->state = VRS_UNKNOWN;
  v->expression = nullptr;
  info->declarator.p = p;
  info->ownedScope = currentScope;
  info->used = p->isVararg();

  if (dv && dv->op() == TO_LITERAL) {
    if (dv->asLiteral()->kind() == LK_NULL) {
      v->flagsPositive |= RT_NULL;
    }
  }

  declareSymbol(p->name(), v);

  assert(currentInfo);
  if (!effectsOnly)
    currentInfo->parameters.push_back(normalizeParamName(p->name()));
}

void CheckerVisitor::visitVarDecl(VarDecl *decl) {
  Visitor::visitVarDecl(decl);

  SymbolInfo *info = makeSymbolInfo(decl->isAssignable() ? SK_VAR : SK_BINDING);
  ValueRef *v = makeValueRef(info);
  const Expr *init = decl->initializer();
  if (decl->isDestructured()) {
    v->state = VRS_UNKNOWN;
    v->expression = nullptr;
  }
  else {
    v->state = VRS_INITIALIZED;
    if (init) {
      v->expression = init;
    }
    else {
      v->expression = &nullValue;
      v->flagsPositive |= RT_NULL;
    }
  }

  if (init) {
    if (init->op() == TO_LITERAL) {
      if (init->asLiteral()->kind() == LK_NULL) {
        v->flagsPositive |= RT_NULL;
      }
    }
    if (init->op() == TO_ID) {
      const ValueRef *vr = findValueForExpr(init);
      if (vr) {
        v->flagsPositive = vr->flagsPositive;
        v->flagsNegative = vr->flagsNegative;
      }
    }
  }

  info->declarator.v = decl;
  info->ownedScope = currentScope;

  declareSymbol(decl->name(), v);
}

void CheckerVisitor::visitConstDecl(ConstDecl *cnst) {

  SymbolInfo *info = makeSymbolInfo(SK_CONST);
  ValueRef *v = makeValueRef(info);
  v->state = VRS_INITIALIZED;
  v->expression = cnst->value();
  info->declarator.c = cnst;
  info->ownedScope = currentScope;
  info->used = cnst->isGlobal();

  declareSymbol(cnst->name(), v);

  Visitor::visitConstDecl(cnst);
}

void CheckerVisitor::visitEnumDecl(EnumDecl *enm) {

  SymbolInfo *info = makeSymbolInfo(SK_ENUM);
  ValueRef *ev = makeValueRef(info);
  ev->state = VRS_DECLARED;
  ev->expression = nullptr;
  info->declarator.e = enm;
  info->ownedScope = currentScope;
  info->used = enm->isGlobal();
  declareSymbol(enm->name(), ev);

  for (auto &c : enm->consts()) {
    SymbolInfo *constInfo = makeSymbolInfo(SK_ENUM_CONST);
    ValueRef *cv = makeValueRef(constInfo);
    cv->state = VRS_INITIALIZED;
    cv->expression = c.val;
    constInfo->declarator.ec = &c;
    constInfo->ownedScope = currentScope;
    constInfo->used = enm->isGlobal();

    const SQChar *fqn = enumFqn(arena, enm->name(), c.id);
    declareSymbol(fqn, cv);

    c.val->visit(this);
  }
}

void CheckerVisitor::visitDestructuringDecl(DestructuringDecl *d) {

  checkAccessNullable(d);

  Visitor::visitDestructuringDecl(d);
}

void CheckerVisitor::analyse(RootBlock *root) {
  root->visit(this);
}

class NameShadowingChecker : public Visitor {
  // TODO: merge this checker into main one. Find hiden symbols in a single pass
  SQCompilationContext _ctx;

  std::vector<const Node *> nodeStack;

  void report(const Node *n, int32_t id, ...) {
    va_list vargs;
    va_start(vargs, id);

    _ctx.vreportDiagnostic((enum DiagnosticsId)id, n->lineStart(), n->columnStart(), n->columnEnd() - n->columnStart(), vargs); //-V522

    va_end(vargs);
  }

  struct Scope;

  struct SymbolInfo {
    union {
      const Id *x;
      const FunctionDecl *f;
      const ClassDecl *k;
      const VarDecl *v;
      const TableDecl *t;
      const ParamDecl *p;
      const EnumDecl *e;
      const ConstDecl *c;
      const EnumConst *ec;
    } declaration;

    enum SymbolKind kind;

    const struct Scope *ownerScope;
    const SQChar *name;

    SymbolInfo(enum SymbolKind k) : kind(k), declaration(), name(nullptr), ownerScope(nullptr) {}
  };

  struct Scope {
    NameShadowingChecker *checker;

    Scope(NameShadowingChecker *chk, const Decl *o) : checker(chk), owner(o), symbols() {
      parent = checker->scope;
      checker->scope = this;
    }

    ~Scope() {
      checker->scope = parent;
    }

    struct Scope *parent;
    std::unordered_map<const SQChar *, SymbolInfo *, StringHasher, StringEqualer> symbols;
    const Decl *owner;

    SymbolInfo *findSymbol(const SQChar *name) const;
  };

  const Node *extractPointedNode(const SymbolInfo *info);

  SymbolInfo *newSymbolInfo(enum SymbolKind k) {
    void *mem = _ctx.arena()->allocate(sizeof(SymbolInfo));
    return new(mem) SymbolInfo(k);
  }

  struct Scope *scope;

  void declareVar(enum SymbolKind k, const VarDecl *v);
  void declareSymbol(const SQChar *name, SymbolInfo *info);

public:
  NameShadowingChecker(SQCompilationContext &ctx) : _ctx(ctx), scope(nullptr) {}

  void visitNode(Node *n);

  void visitVarDecl(VarDecl *var);
  void visitFunctionDecl(FunctionDecl *f);
  void visitParamDecl(ParamDecl *p);
  void visitConstDecl(ConstDecl *c);
  void visitEnumDecl(EnumDecl *e);
  void visitClassDecl(ClassDecl *k);
  void visitTableDecl(TableDecl *t);

  void visitBlock(Block *block);
  void visitTryStatement(TryStatement *stmt);
  void visitForStatement(ForStatement *stmt);
  void visitForeachStatement(ForeachStatement *stmt);

  void analyse(RootBlock *root) {
    root->visit(this);
  }
};

const Node *NameShadowingChecker::extractPointedNode(const SymbolInfo *info) {
  switch (info->kind)
  {
  case SK_EXCEPTION:
    return info->declaration.x;
  case SK_FUNCTION:
    return info->declaration.f;
  case SK_CLASS:
    return info->declaration.k;
  case SK_TABLE:
    return info->declaration.t;
  case SK_VAR:
  case SK_BINDING:
  case SK_FOREACH:
    return info->declaration.v;
  case SK_CONST:
    return info->declaration.c;
  case SK_ENUM:
    return info->declaration.e;
  case SK_ENUM_CONST:
    return info->declaration.ec->val;
  case SK_PARAM:
    return info->declaration.p;
  default:
    assert(0);
    return nullptr;
  }
}

void NameShadowingChecker::declareSymbol(const SQChar *name, SymbolInfo *info) {
  const SymbolInfo *existedInfo = scope->findSymbol(name);
  bool addSymbol = true;
  if (existedInfo) {
    bool warn = name[0] != '_';
    if (strcmp(name, "this") == 0) {
      warn = false;
    }

    if (existedInfo->ownerScope == info->ownerScope) { // something like `let foo = expr<function foo() { .. }>`
      if ((existedInfo->kind == SK_BINDING || existedInfo->kind == SK_VAR) && info->kind == SK_FUNCTION) {
        if (nodeStack.size() > 2) {
          const Node *ln = nodeStack[nodeStack.size() - 1];
          const Node *lln = nodeStack[nodeStack.size() - 2];
          if (ln->op() == TO_DECL_EXPR && static_cast<const DeclExpr *>(ln)->declaration() == info->declaration.f) {
            warn = false;
            // if it `let foo = function foo() {}` or `let function foo() {}` or `function foo() {}`
            addSymbol = existedInfo->declaration.v == lln;
          }
        }
      }
    }

    if (existedInfo->kind == SK_FUNCTION && info->kind == SK_FUNCTION) {
      const FunctionDecl *existed = existedInfo->declaration.f;
      const FunctionDecl *_new = info->declaration.f;
      if (existed->name()[0] == '(' && _new->name()[0] == '(') {
        warn = false;
      }
    }

    if (info->kind == SK_PARAM && existedInfo->kind == SK_PARAM && warn) {
      const ParamDecl *p1 = info->declaration.p;
      const ParamDecl *p2 = existedInfo->declaration.p;
      if (p1->isVararg() && p2->isVararg()) {
        warn = false;
      }
    }

    if (warn) {
      report(extractPointedNode(info), DiagnosticsId::DI_ID_HIDES_ID,
        symbolContextName(info->kind),
        info->name,
        symbolContextName(existedInfo->kind));
    }
  }

  if (addSymbol)
    scope->symbols[name] = info;
}

NameShadowingChecker::SymbolInfo *NameShadowingChecker::Scope::findSymbol(const SQChar *name) const {
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    return it->second;
  }

  return parent ? parent->findSymbol(name) : nullptr;
}

void NameShadowingChecker::visitNode(Node *n) {
  nodeStack.push_back(n);
  n->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::declareVar(enum SymbolKind k, const VarDecl *var) {
  SymbolInfo *info = newSymbolInfo(k);
  info->declaration.v = var;
  info->ownerScope = scope;
  info->name = var->name();
  declareSymbol(var->name(), info);
}

void NameShadowingChecker::visitVarDecl(VarDecl *var) {
  declareVar(var->isAssignable() ? SK_VAR : SK_BINDING, var);

  Visitor::visitVarDecl(var);
}

void NameShadowingChecker::visitParamDecl(ParamDecl *p) {
  SymbolInfo *info = newSymbolInfo(SK_PARAM);
  info->declaration.p = p;
  info->ownerScope = scope;
  info->name = p->name();
  declareSymbol(p->name(), info);

  Visitor::visitParamDecl(p);
}

void NameShadowingChecker::visitConstDecl(ConstDecl *c) {
  SymbolInfo *info = newSymbolInfo(SK_CONST);
  info->declaration.c = c;
  info->ownerScope = scope;
  info->name = c->name();
  declareSymbol(c->name(), info);

  Visitor::visitConstDecl(c);
}

void NameShadowingChecker::visitEnumDecl(EnumDecl *e) {
  SymbolInfo *info = newSymbolInfo(SK_ENUM);
  info->declaration.e = e;
  info->ownerScope = scope;
  info->name = e->name();
  declareSymbol(e->name(), info);

  for (auto &ec : e->consts()) {
    SymbolInfo *cinfo = newSymbolInfo(SK_ENUM_CONST);
    const SQChar *fqn = enumFqn(_ctx.arena(), e->name(), ec.id);

    info->declaration.ec = &ec;
    info->ownerScope = scope;
    info->name = fqn;

    declareSymbol(fqn, cinfo);
  }
}

void NameShadowingChecker::visitFunctionDecl(FunctionDecl *f) {
  Scope *p = scope;

  bool tableScope = p->owner && (p->owner->op() == TO_CLASS || p->owner->op() == TO_TABLE);

  if (!tableScope) {
    SymbolInfo *info = newSymbolInfo(SK_FUNCTION);
    info->declaration.f = f;
    info->ownerScope = p;
    info->name = f->name();
    declareSymbol(f->name(), info);
  }

  Scope funcScope(this, f);
  Visitor::visitFunctionDecl(f);
}

static bool isFunctionDecl(const Expr *e) {
  if (e->op() != TO_DECL_EXPR)
    return false;

  const Decl *d = static_cast<const DeclExpr *>(e)->declaration();

  return d->op() == TO_FUNCTION || d->op() == TO_CONSTRUCTOR;
}

void NameShadowingChecker::visitTableDecl(TableDecl *t) {
  Scope tableScope(this, t);

  nodeStack.push_back(t);
  t->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::visitClassDecl(ClassDecl *k) {
  Scope klassScope(this, k);

  nodeStack.push_back(k);
  k->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::visitBlock(Block *block) {
  Scope blockScope(this, scope ? scope->owner : nullptr);
  Visitor::visitBlock(block);
}

void NameShadowingChecker::visitTryStatement(TryStatement *stmt) {

  nodeStack.push_back(stmt);

  stmt->tryStatement()->visit(this);

  Scope catchScope(this, scope->owner);

  SymbolInfo *info = newSymbolInfo(SK_EXCEPTION);
  info->declaration.x = stmt->exceptionId();
  info->ownerScope = scope;
  info->name = stmt->exceptionId()->id();
  declareSymbol(stmt->exceptionId()->id(), info);

  stmt->catchStatement()->visit(this);

  nodeStack.pop_back();
}

void NameShadowingChecker::visitForStatement(ForStatement *stmt) {
  assert(scope);
  Scope forScope(this, scope->owner);

  Visitor::visitForStatement(stmt);
}

void NameShadowingChecker::visitForeachStatement(ForeachStatement *stmt) {

  assert(scope);
  Scope foreachScope(this, scope->owner);

  VarDecl *idx = stmt->idx();
  if (idx) {
    declareVar(SK_FOREACH, idx);
    assert(idx->initializer() == nullptr);
  }
  VarDecl *val = stmt->val();
  if (val) {
    declareVar(SK_FOREACH, val);
    assert(val->initializer() == nullptr);
  }

  nodeStack.push_back(stmt);

  stmt->container()->visit(this);
  stmt->body()->visit(this);

  nodeStack.pop_back();
}

void StaticAnalyser::runAnalysis(RootBlock *root)
{
  CheckerVisitor(_ctx).analyse(root);
  NameShadowingChecker(_ctx).analyse(root);
}

}
