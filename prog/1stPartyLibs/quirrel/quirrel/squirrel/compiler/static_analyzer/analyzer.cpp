#include "analyzer.h"
#include <stdarg.h>
#include <cctype>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <climits>
#include "../sqtable.h"
#include "../sqarray.h"
#include "../sqclass.h"
#include "../sqfuncproto.h"
#include "../sqclosure.h"
#include <sqdirect.h>
// #include "../sqast_tree_dump.h"

namespace SQCompilation {

static const Expr *deparen(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_PAREN)
    return deparen(static_cast<const UnExpr *>(e)->argument());
  return e;
}

static const Expr *deparenStatic(const Expr *e) {
  if (!e) return nullptr;

  if (e->op() == TO_PAREN || e->op() == TO_STATIC_MEMO)
    return deparen(static_cast<const UnExpr *>(e)->argument());
  return e;
}

static const Expr *skipUnary(const Expr *e) {
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

StaticAnalyzer::StaticAnalyzer(SQCompilationContext &ctx)
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

    return cmpLiterals(l->value(), r->value());
  }

  bool cmpDeclGroup(const DeclGroup *l, const DeclGroup *r) const {
    return cmpNodeVector(l->declarations(), r->declarations());
  }

  bool cmpDestructDecl(const DestructuringDecl *l, const DestructuringDecl *r) const {
    return l->type() == r->type() && check(l->initExpression(), r->initExpression());
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
    case TO_SETSLOT:
    default:
      assert(0);
      return false;
    }
  }
};

class NodeComplexityComputer : public Visitor {

  int32_t complexity;
  const int32_t limit;

  NodeComplexityComputer(int32_t l) : limit(l), complexity(0) {}
public:


  void visitNode(Node *n) {
    if (complexity > limit)
      return;

    Visitor::visitNode(n);
  }

  // Expressions

  void visitId(Id *id) {
    complexity += 1;
  }

  void visitLiteralExpr(LiteralExpr *l) {
  }

  void visitUnExpr(UnExpr *u) {
    if (u->op() != TO_PAREN)
      complexity += 1;
    Visitor::visitUnExpr(u);
  }

  void visitBinExpr(BinExpr *b) {
    complexity += 3;
    Visitor::visitBinExpr(b);
  }

  void visitTerExpr(TerExpr *t) {
    complexity += 2;
    Visitor::visitTerExpr(t);
  }

  void visitGetFieldExpr(GetFieldExpr *f) {
    complexity += 1;
    Visitor::visitGetFieldExpr(f);
  }

  void visitGetSlotExpr(GetSlotExpr *t) {
    complexity += 2;
    Visitor::visitGetSlotExpr(t);
  }

  void visitCallExpr(CallExpr *call) {
    complexity += (call->arguments().size() - 1);
    Visitor::visitCallExpr(call);
  }

  void visitIncExpr(IncExpr *inc) {
    complexity += 1;
    Visitor::visitIncExpr(inc);
  }

  void visitArrayExpr(ArrayExpr *arr) {
    complexity += arr->initializers().size();
    Visitor::visitArrayExpr(arr);
  }

  void visitCommaExpr(CommaExpr *comma) {
    complexity += comma->expressions().size();
    Visitor::visitCommaExpr(comma);
  }

  // Statements

  void visitBlock(Block *b) {
    complexity += b->statements().size();
    Visitor::visitBlock(b);
  }

  void visitIfStatement(IfStatement *ifstmt) {
    complexity += 2;
    if (ifstmt->elseBranch())
      complexity += 1;

    Visitor::visitIfStatement(ifstmt);
  }

  void visitSwitchStatement(SwitchStatement *swtch) {
    complexity += 1; // switch expr
    complexity += (swtch->cases().size() * 2); // 2 due to label + body;
    if (swtch->defaultCase().stmt)
      complexity += 1;

    Visitor::visitSwitchStatement(swtch);
  }

  void visitTryStatement(TryStatement *trystmt) {
    complexity += 3; // try + ex + catch
    Visitor::visitTryStatement(trystmt);
  }

  void visitTerminateStatement(TerminateStatement *t) {
    if (t->argument())
      complexity += 1;

    Visitor::visitTerminateStatement(t);
  }

  void visitLoopStatement(LoopStatement *l) {
    complexity += 1;
    Visitor::visitLoopStatement(l);
  }

  void visitWhileStatement(WhileStatement *w) {
    complexity += 1;

    Visitor::visitWhileStatement(w);
  }

  void visitDoWhileStatement(DoWhileStatement *dw) {
    complexity += 1;

    Visitor::visitDoWhileStatement(dw);
  }

  void visitForStatement(ForStatement *f) {
    if (f->initializer())
      complexity += 1;

    if (f->condition())
      complexity += 1;

    if (f->modifier())
      complexity += 1;

    Visitor::visitForStatement(f);
  }

  void visitForeachStatement(ForeachStatement *fe) {
    if (fe->idx())
      complexity += 1;

    if (fe->val())
      complexity += 1;

    complexity += 1;

    Visitor::visitForeachStatement(fe);
  }

  // Declarations

  void visitValueDecl(ValueDecl *v) {
    complexity += 1;

    if (v->expression())
      complexity += 1;

    Visitor::visitValueDecl(v);
  }

  void visitTableDecl(TableDecl *t) {
    complexity += (t->members().size() * 2); // key + value

    Visitor::visitTableDecl(t);
  }

  void visitClassDecl(ClassDecl *c) {
    if (c->classBase())
      complexity += 1;

    if (c->classKey())
      complexity += 1;

    Visitor::visitClassDecl(c);
  }

  void visitFunctionDecl(FunctionDecl *f) {
    complexity += f->parameters().size();

    Visitor::visitFunctionDecl(f);
  }

  void visitEnumDecl(EnumDecl *e) {
    complexity += 1; //name
    complexity += (e->consts().size() * 2);
    Visitor::visitEnumDecl(e);
  }

  void visitConstDecl(ConstDecl *c) {
    complexity += 2;
    Visitor::visitConstDecl(c);
  }

  void visitDeclGroup(DeclGroup *g) {
    complexity += g->declarations().size();
    Visitor::visitDeclGroup(g);
  }

  void visitDestructuringDecl(DestructuringDecl *dd) {
    complexity += 1;

    Visitor::visitDestructuringDecl(dd);
  }

  static int32_t compute(const Node *n, int32_t limit) {
    NodeComplexityComputer c(limit);
    const_cast<Node *>(n)->visit(&c);
    return c.complexity;
  }
};

class NodeDiffComputer {


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

  int32_t diffDeclExpr(const DeclExpr *lhs, const DeclExpr *rhs) {
    return diffNodes(lhs->declaration(), rhs->declaration());
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
    int32_t valueDiff = diffLiterals(lhs->value(), rhs->value());

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

  const SQChar *realFunctionName(const FunctionDecl *f) const {
    const SQChar *name = f->name();
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

  int32_t diffFunction(const FunctionDecl *lhs, const FunctionDecl *rhs) {
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

  int32_t diffTable(const TableDecl *lhs, const TableDecl *rhs) {
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

  int32_t diffClass(const ClassDecl *lhs, const ClassDecl *rhs) {
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
    case TO_RESUME:
    case TO_CLONE:
    case TO_PAREN:
    case TO_DELETE:
      return diffUnary((const UnExpr *)lhs, (const UnExpr *)rhs);
    case TO_LITERAL:
      return diffLiterals((const LiteralExpr *)lhs, (const LiteralExpr *)rhs);
    case TO_INC:
      return diffIncExpr((const IncExpr *)lhs, (const IncExpr *)rhs);
    case TO_DECL_EXPR:
      return diffDeclExpr((const DeclExpr *)lhs, (const DeclExpr *)rhs);
    case TO_ARRAYEXPR:
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
    case TO_FUNCTION:
    case TO_CONSTRUCTOR:
      return diffFunction((const FunctionDecl *)lhs, (const FunctionDecl *)rhs);
    case TO_CLASS:
      return diffClass((const ClassDecl *)lhs, (const ClassDecl *)rhs);
    case TO_ENUM:
      return diffEnumDecl((const EnumDecl *)lhs, (const EnumDecl *)rhs);
    case TO_TABLE:
      return diffTable((const TableDecl *)lhs, (const TableDecl *)rhs);
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

class CheckerVisitor;

class FunctionReturnTypeEvaluator {

  void checkLiteral(const LiteralExpr *l);
  void checkDeclaration(const DeclExpr *de);
  void checkAddExpr(const BinExpr *expr);
  void checkExpr(const Expr *expr);
  void checkCall(const CallExpr *expr);
  void checkId(const Id *id);
  void checkGetField(const GetFieldExpr *gf);
  void checkCompoundBin(const BinExpr *expr);

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

  CheckerVisitor *checker;

  bool canBeString(const Expr *e);

public:

  FunctionReturnTypeEvaluator(CheckerVisitor *c) : checker(c), flags(0U) {}

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

void FunctionReturnTypeEvaluator::checkExpr(const Expr *expr) {

  switch (expr->op())
  {
  case TO_LITERAL:
    checkLiteral(static_cast<const LiteralExpr *>(expr));
    break;
  case TO_OROR:
  case TO_ANDAND:
  case TO_NULLC:
    checkCompoundBin(static_cast<const BinExpr *>(expr));
    break;
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
    checkAddExpr(static_cast<const BinExpr *>(expr));
    break;
  case TO_MOD: {
    const BinExpr *b = static_cast<const BinExpr *>(expr);
    if (canBeString(b->rhs())) { // this special pattern 'o % "something"'
      flags |= RT_UNRECOGNIZED;
      break;
    }
  } // -V796
  case TO_SUB:
  case TO_MUL:
  case TO_DIV:
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
    checkCall(expr->asCallExpr());
    break;
  case TO_DECL_EXPR:
    checkDeclaration(static_cast<const DeclExpr *>(expr));
    break;
  case TO_ARRAYEXPR:
    flags |= RT_ARRAY;
    break;
  case TO_PAREN:
    checkExpr(static_cast<const UnExpr *>(expr)->argument());
    break;
  case TO_TERNARY: {
      const TerExpr *ter = static_cast<const TerExpr *>(expr);
      checkExpr(ter->b());
      checkExpr(ter->c());
      break;
  }
  case TO_ID:
      checkId(expr->asId());
      break;
  case TO_GETFIELD:
      checkGetField(expr->asGetField());
      // Fall Through
  case TO_GETSLOT:
      if (expr->asAccessExpr()->isNullable())
        flags |= RT_NULL;
      break;
  case TO_ASSIGN:
      flags |= RT_NOTHING;
      break;
  default:
    if (canBeString(expr))
      flags |= RT_STRING;
    else
      flags |= RT_UNRECOGNIZED;
    break;
  }
}

void FunctionReturnTypeEvaluator::checkAddExpr(const BinExpr *expr) {
  assert(expr->op() == TO_ADD);

  unsigned saved = flags;

  flags = 0;

  checkExpr(expr->lhs());

  volatile unsigned lhsFlags = flags; // No idea WTF but in VC++ in dev build this code works incorrect
  flags = 0;

  checkExpr(expr->rhs());
  unsigned rhsFlags = flags;

  flags = saved;

  if ((lhsFlags & RT_STRING) || (rhsFlags & RT_STRING)) {
    // concat with string is casted to string
    flags &= ~(RT_NUMBER | RT_BOOL | RT_NULL);
    flags |= RT_STRING;
  }
  else if ((lhsFlags | rhsFlags) & RT_NUMBER) {
    flags |= RT_NUMBER;
  }
  else {
    flags |= (lhsFlags | rhsFlags);
  }
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

  const Expr *arg = ret->argument();

  if (arg == nullptr) {
    flags |= RT_NOTHING;
    return true;
  }

  checkExpr(arg);

  return true;
}

bool FunctionReturnTypeEvaluator::checkThrow(const ThrowStatement *thrw) {
  flags |= RT_THROW;
  return true;
}

bool FunctionReturnTypeEvaluator::checkIf(const IfStatement *ifStmt) {
  bool retThen = checkNode(ifStmt->thenBranch());
  unsigned thenFlags = flags;
  flags = 0;
  bool retElse = false;
  if (ifStmt->elseBranch()) {
    retElse = checkNode(ifStmt->elseBranch());
  }
  flags |= thenFlags;

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

    if (op == TO_ASSIGN || (TO_PLUSEQ <= op && op <= TO_MODEQ)) {
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
  return op == TO_ASSIGN || (TO_PLUSEQ <= op && op <= TO_MODEQ);
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
    if (hasCondContinue)
      return false;
    return hasUnconditionalTerm;
  }
};


class AssignSeqTerminatorFinder : public Visitor {

  const Expr *assignee;
  bool foundUsage;
  bool foundInterruptor;

  NodeEqualChecker eqChecker;

public:
  AssignSeqTerminatorFinder(const Expr *asg) : assignee(asg), foundUsage(false), foundInterruptor(false), eqChecker() {}

  void visitNode(Node *n) {
    if (!foundInterruptor && !foundUsage)
      Visitor::visitNode(n);
  }

  void visitCallExpr(CallExpr *c) {
    foundInterruptor = true; // consider call as potential usage
  }

  void visitExpr(Expr *e) {
    Visitor::visitExpr(e);

    if (eqChecker.check(assignee, e))
      foundUsage = true;
  }

  void visitFunctionDecl(FunctionDecl *e) { /* skip */ }

  bool check(Node *tree) {

    tree->visit(this);

    return foundUsage || foundInterruptor;
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

static bool isBooleanBinaryResultOperator(enum TreeOp op) {
  return op == TO_NE || op == TO_EQ || (TO_GE <= op && op <= TO_IN);
}

static bool isBooleanBinaryOrLogicalOpeartor(enum TreeOp op) {
  return isBooleanBinaryResultOperator(op) || op == TO_OROR || op == TO_ANDAND;
}

static bool isBooleanResultOperator(enum TreeOp op) {
  return isBooleanBinaryResultOperator(op) || op == TO_NOT;
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

static bool isPureArithOperator(enum TreeOp op) {
  return (TO_USHR <= op && op <= TO_SUB) || (TO_PLUSEQ <= op && op <= TO_MODEQ);
}

static bool isRelationOperator(enum TreeOp op) {
  return TO_3CMP <= op && op <= TO_LT;
}

static bool isBoolRelationOperator(enum TreeOp op) {
  return TO_GE <= op && op <= TO_LT;
}

static bool isBitwiseOperator(enum TreeOp op) {
  return op == TO_OR || op == TO_AND || op == TO_XOR;
}

static bool isBoolCompareOperator(enum TreeOp op) {
  return op == TO_NE || op == TO_EQ || isBoolRelationOperator(op);
}

static bool isCompareOperator(enum TreeOp op) {
  return TO_NE <= op && op <= TO_LT;
}

static bool isShiftOperator(enum TreeOp op) {
  return op == TO_SHL || op == TO_SHR || op == TO_USHR;
}

static bool isHigherShiftPriority(enum TreeOp op) {
  return TO_MUL <= op && op <= TO_SUB;
}

static bool looksLikeBooleanExpr(const Expr *e) {
  enum TreeOp op = e->op(); // -V522
  if (isBooleanResultOperator(op))
    return true;

  if (op == TO_LITERAL) {
    return e->asLiteral()->kind() == LK_BOOL; // -V522
  }

  if (op == TO_NULLC || op == TO_ANDAND || op == TO_OROR) {
    // check for `x?.y {??, ||, &&} false`
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

static bool hasAnyEqual(const SQChar *str, const std::vector<std::string> &candidates) {
  for (auto &candidate : candidates) {
    if (strcmp(str, candidate.c_str()) == 0) {
      return true;
    }
  }

  return false;
}

static bool hasAnySubstring(const SQChar *str, const std::vector<std::string> &candidates) {
  for (auto &candidate : candidates) {
    if (strstr(str, candidate.c_str())) {
      return true;
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

static bool nameLooksLikeResultMustBeUtilized(const SQChar *name) {
  return hasAnyEqual(name, SQCompilationContext::function_result_must_be_utilized);
}

static bool nameLooksLikeResultMustBeString(const SQChar *name) {
  return hasAnyEqual(name, SQCompilationContext::function_can_return_string);
}

static bool nameLooksLikeCallsLambdaInPlace(const SQChar *name) {
  return hasAnyEqual(name, SQCompilationContext::function_calls_lambda_inplace);
}

static bool canFunctionReturnNull(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_can_return_null);
}

static bool isForbiddenFunctionName(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_forbidden);
}

static bool nameLooksLikeMustBeCalledFromRoot(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_must_be_called_from_root);
}

static bool nameLooksLikeForbiddenParentDir(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_forbidden_parent_dir);
}

static bool nameLooksLikeFormatFunction(const SQChar *n) {
  std::string transformed(n);
  std::transform(transformed.begin(), transformed.end(), transformed.begin(), ::tolower);

  return hasAnySubstring(transformed.c_str(), SQCompilationContext::format_function_name);
}

static bool nameLooksLikeModifiesObject(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_modifies_object);
}

static bool nameLooksLikeFunctionTakeBooleanLambda(const SQChar *n) {
  return hasAnyEqual(n, SQCompilationContext::function_takes_boolean_lambda);
}

static const char double_colon_str[] = "::";
static const char base_str[] = "base";
static const char this_str[] = "this";

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
  SK_FOREACH,
  SK_EXTERNAL_BINDING
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
  case SK_EXTERNAL_BINDING: return "external binding";
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
  std::vector<Modifiable> modifiable;
  const FunctionDecl *declaration;
  std::vector<const SQChar *> parameters;

  void joinModifiable(const FunctionInfo *other);
  void addModifiable(const SQChar *name, const FunctionDecl *o);

};

void FunctionInfo::joinModifiable(const FunctionInfo *other) {
  for (auto &m : other->modifiable) {
    if (owner == m.owner)
      continue;

    addModifiable(m.name, m.owner);
  }
}

void FunctionInfo::addModifiable(const SQChar *name, const FunctionDecl *o) {
  for (auto &m : modifiable) {
    if (m.owner == o && strcmp(name, m.name) == 0)
      return;
  }

  modifiable.push_back({ o, name });
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
    const ExternalValueExpr *ev;
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
    case SK_EXTERNAL_BINDING:
      return declarator.ev;
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
      // if we analyze closure we cannot rely on existed assignable values
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

struct IdLocation {
  const char *filename;
  int32_t line, column;
  bool diagSilenced;
};

static std::unordered_set<std::string> knownBindings;
static std::set<std::string> fileNames;
static std::map<std::string, std::vector<IdLocation>> declaredGlobals;
static std::map<std::string, std::vector<IdLocation>> usedGlobals;

class CheckerVisitor : public Visitor {
  friend struct VarScope;
  friend struct BreakableScope;
  friend class FunctionReturnTypeEvaluator;

  const int32_t functionComplexityThreshold = 45; // get threshold complexity from command line args
  const int32_t statementComplexityThreshold = 23;
  const int32_t statementSimilarityThreshold = 33;

  SQCompilationContext &_ctx;

  NodeEqualChecker _equalChecker;

  bool isUpperCaseIdentifier(const Expr *expr);

  void report(const Node *n, int32_t id, ...);

  void checkKeyNameMismatch(const Expr *key, const Expr *expr);

  void checkAlwaysTrueOrFalse(const Expr *expr);

  void checkForeachIteratorCapturedByClosure(const Id *id, const ValueRef *v);
  void checkIdUsed(const Id *id, const Node *p, ValueRef *v);

  void reportIfCannotBeNull(const Expr *checkee, const Expr *n, const char *loc);
  void reportModifyIfContainer(const Expr *e, const Expr *mod);
  void checkForgotSubst(const LiteralExpr *l);
  void checkContainerModification(const UnExpr *expr);
  void checkAndOrPriority(const BinExpr *expr);
  void checkBitwiseParenBool(const BinExpr *expr);
  void checkNullCoalescingPriority(const BinExpr *expr);
  void checkAssignmentToItself(const BinExpr *expr);
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
  void checkNewGlobalSlot(const BinExpr *);
  void checkUselessNullC(const BinExpr *);
  void checkCannotBeNull(const BinExpr *);
  void checkCanBeSimplified(const BinExpr *expr);
  void checkRangeCheck(const BinExpr *expr);
  void checkAlwaysTrueOrFalse(const TerExpr *expr);
  void checkTernaryPriority(const TerExpr *expr);
  void checkSameValues(const TerExpr *expr);
  void checkCanBeSimplified(const TerExpr *expr);
  void checkExtendToAppend(const CallExpr *callExpr);
  void checkAlreadyRequired(const CallExpr *callExpr);
  void checkCallNullable(const CallExpr *callExpr);
  void checkPersistCall(const CallExpr *callExpr);
  void checkForbiddenCall(const CallExpr *callExpr);
  void checkCallFromRoot(const CallExpr *callExpr);
  void checkForbiddenParentDir(const CallExpr *callExpr);
  void checkFormatArguments(const CallExpr *callExpr);
  void checkArguments(const CallExpr *callExpr);
  void checkContainerModification(const CallExpr *expr);
  void checkUnwantedModification(const CallExpr *expr);
  void checkCannotBeNull(const CallExpr *expr);
  void checkBooleanLambda(const CallExpr *expr);
  void checkBoolIndex(const GetSlotExpr *expr);
  void checkNullableIndex(const GetSlotExpr *expr);
  void checkGlobalAccess(const GetFieldExpr *expr);
  void checkAccessFromStatic(const GetFieldExpr *expr);
  void checkExternalField(const GetFieldExpr *expr);

  bool hasDynamicContent(const SQObject &container);

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
  void checkSuspiciousFormatting(const Statement *body, const Statement *stmt);
  void checkSuspiciousFormattingOfStetementSequence(const Statement* prev, const Statement* cur);

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
  void checkFunctionSimilarity(const Block *b);
  void checkAssignExpressionSimilarity(const Block *b);
  void checkUnutilizedResult(const ExprStatement *b);
  void checkNullableContainer(const ForeachStatement *loop);
  void checkMissedBreak(const SwitchStatement *swtch);

  const SQChar *findSlotNameInStack(const Decl *);
  void checkFunctionReturns(FunctionDecl *func);
  void checkFunctionSimilarity(const TableDecl *table);

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
  std::unordered_set<const SQChar *, StringHasher, StringEqualer> persistedKeys;

  std::unordered_map<const Node *, ValueRef *> astValues;
  std::vector<ExternalValueExpr *> externalValues;

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

  const ExternalValueExpr *findExternalValue(const Expr *e);
  const FunctionInfo *findFunctionInfo(const Expr *e, bool &isCtor);

  void setValueFlags(const Expr *lvalue, unsigned pf, unsigned nf);
  const ValueRef *findValueForExpr(const Expr *e);
  const Expr *maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited, bool allow_external = false);
  const Expr *maybeEval(const Expr *e, int32_t &evalId, bool allow_external = false);
  const Expr *maybeEval(const Expr *e, bool allow_external = false);

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
  const SQChar *extractFunctionName(const CallExpr *call);

  LiteralExpr trueValue, falseValue, nullValue;

  bool effectsOnly;

  void putIntoGlobalNamesMap(std::map<std::string, std::vector<IdLocation>> &map, enum DiagnosticsId diag, const SQChar *name, const Node *d);
  void storeGlobalDeclaration(const SQChar *name, const Node *d);
  void storeGlobalUsage(const SQChar *name, const Node *d);

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

  void visitLiteralExpr(LiteralExpr *l);
  void visitId(Id *id);
  void visitUnExpr(UnExpr *expr);
  void visitBinExpr(BinExpr *expr);
  void visitTerExpr(TerExpr *expr);
  void visitIncExpr(IncExpr *expr);
  void visitCallExpr(CallExpr *expr);
  void visitGetFieldExpr(GetFieldExpr *expr);
  void visitGetSlotExpr(GetSlotExpr *expr);

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

  ValueRef* addExternalValue(const SQObject &val, const Node *location);
  void checkDestructuredVarDefault(VarDecl *var);

  void analyze(RootBlock *root, const HSQOBJECT *bindings);
};

void VarScope::checkUnusedSymbols(CheckerVisitor *checker) {

  for (auto &s : symbols) {
    const SQChar *n = s.first;
    const ValueRef *v = s.second;

    if (strcmp(n, this_str) == 0) // skip 'this'
      continue;


    SymbolInfo *info = v->info;

    if (info->kind == SK_ENUM_CONST)
      continue;

    if (!info->used && n[0] != '_') {
      checker->report(info->extractPointedNode(), DiagnosticsId::DI_DECLARED_NEVER_USED, info->contextName(), n);
      // TODO: add hint for param/exception name about silencing it with '_' prefix
    }
    else if (info->used && n[0] == '_') {
      if (info->kind == SK_PARAM || info->kind == SK_FOREACH)
        checker->report(info->extractPointedNode(), DiagnosticsId::DI_INVALID_UNDERSCORE, info->contextName(), n);
    }
  }
}

void FunctionReturnTypeEvaluator::checkCompoundBin(const BinExpr *expr) {
  enum TreeOp op = expr->op();

  assert(op == TO_ANDAND || op == TO_OROR || op == TO_NULLC);

  unsigned old = flags;
  flags = 0;

  checkExpr(expr->lhs());

  unsigned lhsFlags = flags;
  flags = 0;

  checkExpr(expr->rhs());

  unsigned rhsFlags = flags;
  flags = old;

  lhsFlags &= ~RT_UNRECOGNIZED;
  rhsFlags &= ~RT_UNRECOGNIZED;

  bool hasFuncCall = (lhsFlags | rhsFlags) & RT_FUNCTION_CALL;

  lhsFlags &= ~RT_FUNCTION_CALL;
  rhsFlags &= ~RT_FUNCTION_CALL;

  if (op != TO_NULLC && (((lhsFlags & RT_BOOL) && (rhsFlags & RT_NUMBER)) || ((rhsFlags & RT_BOOL) && (lhsFlags & RT_NUMBER))))
    flags |= RT_BOOL;
  else if (rhsFlags & RT_NULL)
    flags |= RT_NULL;
  else if ((lhsFlags | rhsFlags) & RT_STRING)
    flags |= RT_STRING;
  else
    flags |= rhsFlags;

  if (hasFuncCall)
    flags |= RT_FUNCTION_CALL;
}

void FunctionReturnTypeEvaluator::checkId(const Id *id) {
  if (nameLooksLikeResultMustBeBoolean(id->name()))
    flags |= RT_BOOL;
  else
    flags |= RT_UNRECOGNIZED;
}

void FunctionReturnTypeEvaluator::checkGetField(const GetFieldExpr *gf) {
  if (nameLooksLikeResultMustBeBoolean(gf->fieldName()))
    flags |= RT_BOOL;
  else
    flags |= RT_UNRECOGNIZED;
}

void FunctionReturnTypeEvaluator::checkCall(const CallExpr *call) {
  if (canBeString(call)) {
    flags |= RT_STRING;
  }

  const SQChar *fn = checker->extractFunctionName(call);

  if (nameLooksLikeResultMustBeBoolean(fn)) {
    flags |= RT_BOOL;
  }

  if (call->isNullable())
    flags |= RT_NULL;

  flags |= RT_FUNCTION_CALL;
}

void CheckerVisitor::putIntoGlobalNamesMap(std::map<std::string, std::vector<IdLocation>> &map, enum DiagnosticsId diag, const SQChar *name, const Node *d) {

  std::string sourcenameCache(_ctx.sourceName());

  auto fnIt = fileNames.find(sourcenameCache);
  IdLocation loc;

  if (fnIt == fileNames.end()) {
    auto it2 = fileNames.insert(sourcenameCache);
    loc.filename = it2.first->c_str();
  }
  else {
    loc.filename = fnIt->c_str();
  }

  loc.line = d->lineStart();
  loc.column = d->columnStart();
  loc.diagSilenced = _ctx.isDisabled(diag, loc.line, loc.column);

  std::string key(name);

  auto it = map.find(key);

  if (it != map.end()) {
    it->second.push_back(loc);
  }
  else {
    auto it2 = map.insert({ key, std::vector<IdLocation>() });
    it2.first->second.push_back(loc);
  }
}

void CheckerVisitor::storeGlobalDeclaration(const SQChar *name, const Node *d) {
  putIntoGlobalNamesMap(declaredGlobals, DiagnosticsId::DI_GLOBAL_NAME_REDEF, name, d);
}

void CheckerVisitor::storeGlobalUsage(const SQChar *name, const Node *d) {
  putIntoGlobalNamesMap(usedGlobals, DiagnosticsId::DI_UNDEFINED_GLOBAL, name, d);
}

bool FunctionReturnTypeEvaluator::canBeString(const Expr *e) { return checker->couldBeString(e); }

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
  for (auto ev : externalValues)
    ev->~ExternalValueExpr();
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
    id = e->asId()->name();
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

void CheckerVisitor::checkForeachIteratorCapturedByClosure(const Id *id, const ValueRef *v) {
  if (effectsOnly)
    return;

  if (v->info->kind != SK_FOREACH)
    return;

  const FunctionDecl *thisScopeOwner = currentScope->owner;

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
          name = callee->asId()->name();
        else if (callee->op() == TO_GETFIELD)
          name = callee->asGetField()->fieldName();

        if (name && nameLooksLikeCallsLambdaInPlace(name)) {
          return;
        }
      }
    }
  }

  report(id, DiagnosticsId::DI_ITER_IN_CLOSURE, id->name());
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
    Expr *rhs = bin->rhs();
    bool simpleAsgn = e->op() == TO_ASSIGN;
    if (id == lhs) {
      bool used = v->info->usedAfterAssign || existsInTree(id, rhs);
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
    report(id, DiagnosticsId::DI_POSSIBLE_GARBAGE, id->name());
  }
  else if (v->state == VRS_UNDEFINED) {
    report(id, DiagnosticsId::DI_UNINITIALIZED_VAR);
  }
}

void CheckerVisitor::checkAccessFromStatic(const GetFieldExpr *acc) {
  if (effectsOnly)
    return;

  const Expr *r = acc->receiver();

  if (r->op() != TO_ID || strcmp(r->asId()->name(), this_str) != 0)
    return;

  const TableMember *m = nullptr;
  const ClassDecl *klass = nullptr;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst == SST_TABLE_MEMBER) {
      m = it->member;
      ++it;

      if (it != nodeStack.rend() && it->sst == SST_NODE && it->n->op() == TO_CLASS) {
        klass = static_cast<const ClassDecl *>(it->n);
      }

      break;
    }
  }

  if (!m || !m->isStatic() || !klass)
    return;

  const auto &members = klass->members();
  const SQChar *memberName = acc->fieldName();

  for (const auto &m : members) {
    if (m.key->op() == TO_LITERAL && m.key->asLiteral()->kind() == LK_STRING) {
      const SQChar *klassMemberName = m.key->asLiteral()->s();
      if (strcmp(memberName, klassMemberName) == 0) {
        if (!m.isStatic())
          report(acc, DiagnosticsId::DI_USED_FROM_STATIC, memberName, "static member");
        return;
      }
    }
  }

  report(acc, DiagnosticsId::DI_USED_FROM_STATIC, memberName, "static member");
}

bool CheckerVisitor::hasDynamicContent(const SQObject &container) {
  if (!sq_istable(container))
    return false;
  SQObjectPtr key(_ctx.getVm(), "__dynamic_content__");
  SQObjectPtr val;
  return _table(container)->Get(key, val);
}

void CheckerVisitor::checkExternalField(const GetFieldExpr *acc) {
  if (effectsOnly)
    return;

  const Expr *r = acc->receiver();
  r = maybeEval(r, true);
  if (r->op() != TO_EXTERNAL_VALUE)
    return;

  const auto &container = r->asExternal()->value();
  if (sq_isinstance(container))
    return;

  SQObjectPtr key(_ctx.getVm(), acc->fieldName());
  SQObjectPtr val;
  if (!SQ_SUCCEEDED(sq_direct_get(_ctx.getVm(), &container, &key, &val, false))) {
    if (!acc->isNullable() && !hasDynamicContent(container)) {
      report(acc, DI_MISSING_FIELD, acc->fieldName(), GetTypeName(container));
      char buf[128];
      snprintf(buf, sizeof(buf), "source of %s", GetTypeName(container));
      report(r, DI_SEE_OTHER, buf);
    }
    return;
  }

  __AddRef(val._type, val._unVal);
  astValues[acc] = addExternalValue(val, acc);
}

static bool cannotBeNull(const Expr *e) {
  switch (e->op())
  {
  case TO_INC:
  case TO_NEG: case TO_NOT: case TO_BNOT:
  case TO_3CMP:
  case TO_XOR: case TO_OR: case TO_AND:
  case TO_EQ: case TO_NE: case TO_LE: case TO_LT: case TO_GE: case TO_GT:
  case TO_IN: case TO_INSTANCEOF:
  case TO_USHR: case TO_SHL: case TO_SHR:
  case TO_ADD: case TO_SUB:  case TO_MUL: case TO_DIV: /*case TO_MOD:*/
  case TO_TYPEOF: case TO_RESUME:
  case TO_BASE: case TO_ROOT_TABLE_ACCESS:
  case TO_ARRAYEXPR: case TO_DECL_EXPR:
    return true;
  case TO_LITERAL:
    return e->asLiteral()->kind() != LK_NULL;
  default:
    return false;
  }
}

void CheckerVisitor::reportIfCannotBeNull(const Expr *checkee, const Expr *n, const char *loc) {
  assert(n);

  if (checkee->op() == TO_NULLC) {
    checkee = maybeEval(static_cast<const BinExpr *>(checkee)->rhs());
  }

  if (checkee->op() == TO_TERNARY) {
    const TerExpr *ter = static_cast<const TerExpr *>(checkee);
    const Expr *ifTrue = maybeEval(ter->b());
    const Expr *ifFalse = maybeEval(ter->c());

    if (cannotBeNull(ifTrue) && cannotBeNull(ifFalse)) {
      report(n, DiagnosticsId::DI_EXPR_NOT_NULL, loc);
    }
    return;
  }

  if (cannotBeNull(checkee))
    report(n, DiagnosticsId::DI_EXPR_NOT_NULL, loc);
}

void CheckerVisitor::reportModifyIfContainer(const Expr *e, const Expr *mod) {
  bool found = false;
  const ForeachStatement *f = nullptr;
  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst != SST_NODE)
      continue;

    const Node *n = it->n;
    if (n->op() == TO_FOREACH) {
      f = static_cast<const ForeachStatement *>(n);

      if (_equalChecker.check(f->container(), e)) {
        found = true;
        break;
      }
    }
  }

  if (!found)
    return;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst != SST_NODE)
      continue;

    if (it->n == f)
      break;

    const Node *n = it->n;

    if (n->op() == TO_BLOCK) {
      const auto &stmts = static_cast<const Block *>(n)->statements();
      assert(stmts.size() > 0);
      const Statement *stmt = stmts.back();
      if (stmt->op() == TO_RETURN || stmt->op() == TO_THROW || stmt->op() == TO_BREAK)
        return;
    }
  }

  report(mod, DiagnosticsId::DI_MODIFIED_CONTAINER);
}


static bool stringLooksLikeFormatTemplate(const SQChar *s) {
  const SQChar *bracePtr = strchr(s, '{');
  if (bracePtr && (isalpha(bracePtr[1]) || bracePtr[1] == '_'))
  {
    // check for strings specific to Dagor DataBlock objects
    bool isDagorBlk = (strstr(s, ":i=") || strstr(s, ":r=") || strstr(s, ":t=") || strstr(s, ":p2=") || strstr(s, ":p3=") || strstr(s, ":tm="));
    return !isDagorBlk && bracePtr[1] && strchr(bracePtr + 2, '}');
  }

  return false;
}

void CheckerVisitor::checkForgotSubst(const LiteralExpr *l) {
  if (effectsOnly)
    return;

  if (l->kind() != LK_STRING)
    return;

  const CallExpr *candidate = nullptr;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst != SST_NODE)
      continue;

    const Node *n = it->n;

    if (n->op() == TO_CALL) {
      candidate = static_cast<const CallExpr *>(n);
      break;
    }
  }

  const SQChar *s = l->s();
  if (!stringLooksLikeFormatTemplate(s)) {
    return;
  }

  bool ok = false;

  if (candidate) {
    const Expr *callee = deparenStatic(candidate->callee());
    const auto &arguments = candidate->arguments();
    if (callee->op() == TO_GETFIELD) { // -V522
      const GetFieldExpr *f = callee->asGetField();
      const SQChar *funcName = f->fieldName();
      if (deparenStatic(f->receiver()) == l) {
        ok = strcmp(funcName, "subst") == 0;
      } else if (strcmp(funcName, "split") == 0) {
        ok = arguments.size() >= 1 && deparenStatic(arguments[0]) == l;;
      }
    }
    else if (callee->op() == TO_ID) { // -V522
      if (strcmp("loc", callee->asId()->name()) == 0) {
        ok = arguments.size() >= 2 && deparenStatic(arguments[1]) == l;
      }
    }
  }

  if (!ok)
    report(l, DiagnosticsId::DI_FORGOT_SUBST);
}

void CheckerVisitor::checkContainerModification(const UnExpr *u) {
  if (effectsOnly)
    return;

  if (u->op() != TO_DELETE)
    return;

  const Expr *arg = u->argument();

  if (!arg->isAccessExpr())
    return;

  const Expr *receiver = arg->asAccessExpr()->receiver();

  reportModifyIfContainer(receiver, u);
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

void CheckerVisitor::checkNullCoalescingPriority(const BinExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  if (expr->op() == TO_NULLC) {
    if (isSuspiciousNeighborOfNullCoalescing(l->op())) {
      report(l, DiagnosticsId::DI_NULL_COALESCING_PRIORITY, treeopStr(l->op()));
    }

    if (isSuspiciousNeighborOfNullCoalescing(r->op())) {
      report(r, DiagnosticsId::DI_NULL_COALESCING_PRIORITY, treeopStr(r->op()));
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

void CheckerVisitor::checkCanBeSimplified(const TerExpr *expr) {
  if (effectsOnly)
    return;

  const Expr *cond = expr->a();
  const Expr *checkee = nullptr;


  if (cond->op() == TO_NE) {
    const BinExpr *b = static_cast<const BinExpr *>(cond);

    if (b->lhs()->op() == TO_LITERAL && b->lhs()->asLiteral()->kind() == LK_NULL)
      checkee = b->rhs();
    else if (b->rhs()->op() == TO_LITERAL && b->rhs()->asLiteral()->kind() == LK_NULL)
      checkee = b->lhs();
  }

  if (!checkee)
    return;

  if (expr->c()->op() != TO_LITERAL || expr->c()->asLiteral()->kind() != LK_NULL)
    return;

  checkee = maybeEval(checkee);
  const Expr *t = maybeEval(expr->b());

  if (_equalChecker.check(checkee, t)) {
    report(expr, DiagnosticsId::DI_CAN_BE_SIMPLIFIED);
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

  enum TreeOp op = bin->op();

  bool isRelOp = isBoolRelationOperator(op);
  bool isArithOp = isPureArithOperator(op);
  bool isAssign = op == TO_ASSIGN || op == TO_NEWSLOT;

  if (!isRelOp && !isArithOp && !isAssign)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  const char *opType = isRelOp ? "Comparison operation" : "Arithmetic operation";


  bool lhsNullable = isPotentiallyNullable(lhs);
  bool rhsNullable = isPotentiallyNullable(rhs);

  if (!lhsNullable && !rhsNullable)
    return;

  // string concat with nullable is OK
  if (couldBeString(lhs) || couldBeString(rhs))
    return;

  if (lhsNullable) {
    if (isAssign) {
      if (lhs->op() != TO_ID) { // -V522
        report(bin->lhs(), DiagnosticsId::DI_NULLABLE_ASSIGNMENT);
      }
    }
    else {
      report(bin->lhs(), DiagnosticsId::DI_NULLABLE_OPERANDS, opType);
    }
  }

  if (rhsNullable && !isAssign) {
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

  const Expr *dl = deparenStatic(l);
  const Expr *dr = deparenStatic(r);

  const Expr *el = maybeEval(dl);
  const Expr *er = maybeEval(dr);

  bool l_b = looksLikeBooleanExpr(l);
  bool dl_b = looksLikeBooleanExpr(dl);
  bool el_b = looksLikeBooleanExpr(el);

  bool r_b = looksLikeBooleanExpr(r);
  bool dr_b = looksLikeBooleanExpr(dr);
  bool er_b = looksLikeBooleanExpr(er);

  bool left_cb = l_b || dl_b || el_b;
  bool right_cb = r_b || dr_b || er_b;

  if (left_cb != right_cb) { // -V522
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

/*  if (rightIsConst && bin->op() == TO_OROR) {
    if (rhs->op() != TO_LITERAL || rhs->asLiteral()->kind() != LK_BOOL || rhs->asLiteral()->b() != true) {
      rightIsConst = false;
    }
  } */

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
    funcName = callee->asId()->name();
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

static const SQChar *tryExtractKeyName(const Expr *e) {

  if (e->op() == TO_GETFIELD)
    return e->asGetField()->fieldName();

  if (e->op() == TO_GETSLOT) {
    e = e->asGetSlot()->key();
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
    function bar() {}
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

void CheckerVisitor::checkNewGlobalSlot(const BinExpr *bin) {

  if (effectsOnly)
    return;

  if (bin->op() != TO_NEWSLOT)
    return;

  const Expr *lhs = bin->lhs();

  if (lhs->op() != TO_GETFIELD)
    return;

  const GetFieldExpr *gf = lhs->asGetField();

  if (gf->receiver()->op() != TO_ROOT_TABLE_ACCESS)
    return;

  storeGlobalDeclaration(gf->fieldName(), bin);
}

void CheckerVisitor::checkUselessNullC(const BinExpr *bin) {
  if (effectsOnly)
    return;

  if (bin->op() != TO_NULLC)
    return;

  const Expr *rhs = maybeEval(bin->rhs());

  if (rhs->op() == TO_LITERAL && rhs->asLiteral()->kind() == LK_NULL)
    report(bin, DiagnosticsId::DI_USELESS_NULLC);
}

void CheckerVisitor::checkCannotBeNull(const BinExpr *bin) {
  if (effectsOnly)
    return;

  const char *loc = nullptr;
  const Expr *reportee = nullptr;
  const Expr *checkee = nullptr;
  if (bin->op() == TO_NULLC) {
    reportee = bin->lhs();
    checkee = maybeEval(reportee);
    loc = "null coalescing";
  }
  else if (bin->op() == TO_NE || bin->op() == TO_EQ) {
    const Expr *le = maybeEval(bin->lhs());
    const Expr *re = maybeEval(bin->rhs());

    loc = "equal check";

    if (le->op() == TO_LITERAL && le->asLiteral()->kind() == LK_NULL) {
      checkee = re;
      reportee = bin->rhs();
    }
    else if (re->op() == TO_LITERAL && re->asLiteral()->kind() == LK_NULL) {
      checkee = le;
      reportee = bin->lhs();
    }
  }

  if (checkee)
    reportIfCannotBeNull(checkee, reportee, loc);
}

void CheckerVisitor::checkCanBeSimplified(const BinExpr *bin) {
  if (effectsOnly)
    return;

  if (bin->op() != TO_ANDAND && bin->op() != TO_OROR)
    return;

  const Expr *lhs = maybeEval(bin->lhs());
  const Expr *rhs = maybeEval(bin->rhs());

  if (isBoolRelationOperator(lhs->op()) && isBoolRelationOperator(rhs->op())) {
    const BinExpr *binL = static_cast<const BinExpr *>(lhs);
    const BinExpr *binR = static_cast<const BinExpr *>(rhs);

    if (_equalChecker.check(binL->lhs(), binR->lhs())) {
      int leftCmp = (binL->op() == TO_GE) || (binL->op() == TO_GT) ? 1 : -1;
      int rightCmp = (binR->op() == TO_GE) || (binR->op() == TO_GT) ? 1 : -1;

      if (leftCmp == rightCmp) {
        const Expr *l = maybeEval(binL->rhs());
        const Expr *r = maybeEval(binR->rhs());

        bool lConst = l->op() == TO_LITERAL || isUpperCaseIdentifier(l);
        bool rConst = r->op() == TO_LITERAL || isUpperCaseIdentifier(r);

        if (lConst && rConst) {
          report(bin, DiagnosticsId::DI_CAN_BE_SIMPLIFIED);
        }
      }
    }
  }
}


static bool looksLikeElementCount(const Expr *e) {
  const SQChar *checkee = nullptr;

  if (e->op() == TO_ID)
    checkee = e->asId()->name();
  else if (e->op() == TO_GETFIELD) {
    checkee = e->asGetField()->fieldName();
  }
  else if (e->op() == TO_GETSLOT) {
    return looksLikeElementCount(deparenStatic(e->asGetSlot()->key()));
  }
  else if (e->op() == TO_CALL) {
    return looksLikeElementCount(deparenStatic(e->asCallExpr()->callee()));
  }

  if (!checkee)
    return false;

  static const char * markers[] = { "len", "Len", "length", "Length", "count", "Count", "cnt", "Cnt", "size", "Size", "Num", "Number" };

  for (const char *m : markers) {
    const char *p = strstr(checkee, m);
    if (!p)
      continue;

    if (p == checkee || p[-1] == '_' || (islower(p[-1]) && isupper(p[0]))) {
      char next = p[strlen(m)];
      if (!next || next == '_' || isupper(next))
        return true;
    }
  }

  return false;
}

void CheckerVisitor::checkRangeCheck(const BinExpr *expr) {
  if (effectsOnly)
    return;

  if (expr->op() != TO_OROR && expr->op() != TO_ANDAND)
    return;

  const Expr *lhs = maybeEval(expr->lhs());
  const Expr *rhs = maybeEval(expr->rhs());

  if (isRelationOperator(lhs->op()) && isRelationOperator(rhs->op())) {
    const Expr *cmpZero = nullptr, *cmpCount = nullptr;
    int cmpDir = 1;

    const BinExpr *l = static_cast<const BinExpr *>(lhs);
    const BinExpr *r = static_cast<const BinExpr *>(rhs);

    if (_equalChecker.check(l->lhs(), r->lhs())) {
      cmpZero = l->rhs();
      cmpCount = r->rhs();
    }
    else if (_equalChecker.check(l->rhs(), r->lhs())) {
      cmpDir = -1;
      cmpZero = l->lhs();
      cmpCount = r->rhs();
    }
    else if (_equalChecker.check(l->lhs(), r->rhs())) {
      cmpZero = l->rhs();
      cmpCount = r->lhs();
    }

    if (!cmpZero)
      return;

    assert(cmpCount);

    if (cmpZero->op() != TO_LITERAL || cmpZero->asLiteral()->kind() != LK_INT || cmpZero->asLiteral()->i() != 0)
      return;

    if (looksLikeElementCount(maybeEval(cmpCount))) {
      int leftCmp = (l->op() == TO_GE) || (l->op() == TO_GT) ? 1 : -1;
      int rightCmp = (r->op() == TO_GE) || (r->op() == TO_GT) ? 1 : -1;
      bool leftEq = (l->op() == TO_GE) || (l->op() == TO_LE);
      bool rightEq = (r->op() == TO_GE) || (r->op() == TO_LE);

      if (leftCmp * cmpDir != rightCmp && leftEq == rightEq)
        report(expr, DiagnosticsId::DI_RANGE_CHECK);
    }
  }
}

void CheckerVisitor::checkAlreadyRequired(const CallExpr *call) {
  if (effectsOnly)
    return;

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
    name = callee->asId()->name();
  }
  else if (callee->op() == TO_GETFIELD) {
    const GetFieldExpr *g = callee->asGetField();
    if (g->receiver()->op() == TO_ROOT_TABLE_ACCESS) {
      name = g->fieldName();
    }
  }

  if (!name)
    return;

  if (strcmp(name, "require") != 0 && strcmp(name, "require_optional") != 0)
    return;

  const SQChar *moduleName = l->s();

  if (!_ctx.isRequireDisabled(call->lineStart(), call->columnStart()))
    if (auto fv = findValueInScopes("require_optional"); fv && fv->expression && fv->expression->op() == TO_EXTERNAL_VALUE) {
      auto vm = _ctx.getVm();
      SQInteger top = sq_gettop(vm);

      sq_pushobject(vm, fv->expression->asExternal()->value());
      sq_pushnull(vm);
      sq_pushstring(vm, moduleName, -1);

      SQRESULT result = sq_call(vm, 2, true, false);
      if (SQ_SUCCEEDED(result)) {
        SQObject ret;
        if (SQ_SUCCEEDED(sq_getstackobj(vm, -1, &ret)) && !sq_isnull(ret)) {
          astValues[call] = addExternalValue(ret, call);
        }
        else if (SQCompilationContext::do_report_missing_modules) {
          report(call, DI_MISSING_MODULE, moduleName);
          SQObjectPtr empty;
          astValues[call] = addExternalValue(empty, call);
        }
      }
      else if (SQCompilationContext::do_report_missing_modules) {
        report(call, DI_MISSING_MODULE, moduleName);
        SQObjectPtr empty;
        astValues[call] = addExternalValue(empty, call);
      }

      sq_settop(vm, top);
    }

  if (nodeStack.size() > 2)
    return; // do not consider require which is not on TL

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
    report(c, DiagnosticsId::DI_ACCESS_POT_NULLABLE, c->op() == TO_ID ? c->asId()->name() : "expression", "function");
  }
}

void CheckerVisitor::checkPersistCall(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *calleeName = extractFunctionName(call);

  if (!calleeName)
    return;

  const auto &arguments = call->arguments();
  const Expr *keyExpr = nullptr;

  if (strcmp("persist", calleeName) == 0) {
    if (arguments.size() < 2)
      return;
    keyExpr = arguments[0];
  }
  else if (strcmp("mkWatched", calleeName) == 0) {
    if (arguments.size() < 2)
      return;

    const Expr *persistsFunc = maybeEval(arguments[0]);
    if (persistsFunc->op() != TO_ID || strcmp("persist", persistsFunc->asId()->name()) != 0)
      return;

    keyExpr = arguments[1];
  }

  if (!keyExpr)
    return;

  const Expr *evalKeyExpr = maybeEval(keyExpr);

  if (evalKeyExpr->op() != TO_LITERAL)
    return;

  const LiteralExpr *l = evalKeyExpr->asLiteral();

  if (l->kind() != LK_STRING)
    return;

  const SQChar *key = l->s();

  auto r = persistedKeys.emplace(key);

  if (!r.second) {
    report(keyExpr, DiagnosticsId::DI_DUPLICATE_PERSIST_ID, key);
  }
}

void CheckerVisitor::checkForbiddenCall(const CallExpr *call) {

  if (effectsOnly)
    return;

  const SQChar *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (isForbiddenFunctionName(fn)) {
    report(call, DiagnosticsId::DI_FORBIDDEN_FUNC, fn);
  }
}

void CheckerVisitor::checkCallFromRoot(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeMustBeCalledFromRoot(fn)) {
    return;
  }

  bool do_report = false;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst == SST_TABLE_MEMBER)
      continue;

    enum TreeOp op = it->n->op();

    if (op == TO_FUNCTION || op == TO_CLASS || op == TO_CONSTRUCTOR) {
      do_report = true;
      break;
    }
  }

  if (do_report) {
    report(call, DiagnosticsId::DI_CALL_FROM_ROOT, fn);
  }
}

void CheckerVisitor::checkForbiddenParentDir(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeForbiddenParentDir(fn)) {
    return;
  }

  const auto &arguments = call->arguments();

  if (arguments.size() < 1)
    return;

  const Expr *arg = maybeEval(arguments[0]);

  if (arg->op() != TO_LITERAL || arg->asLiteral()->kind() != LK_STRING)
    return;

  const SQChar *path = arg->asLiteral()->s();

  const char * p = strstr(path, "..");
  if (p && (p[2] == '/' || p[2] == '\\')) {
    report(call, DiagnosticsId::DI_FORBIDDEN_PARENT_DIR);
  }
}

void CheckerVisitor::checkFormatArguments(const CallExpr *call) {
  if (effectsOnly)
    return;

  const auto &arguments = call->arguments();

  for (int32_t i = 0; i < arguments.size(); ++i) {
    const Expr *arg = deparenStatic(arguments[i]);
    if (arg->op() == TO_LITERAL && arg->asLiteral()->kind() == LK_STRING) { // -V522
      int32_t formatsCount = 0;
      for (const SQChar *s = arg->asLiteral()->s(); *s; ++s) {
        if (*s == '%') {
          if (*(s + 1) == '%') {
            s++;
          }
          else {
            formatsCount++;
          }
        }
      }

      if (formatsCount && formatsCount != (arguments.size() - i - 1)) {
        const SQChar *name = extractFunctionName(call);
        if (!name)
          return;

        if (nameLooksLikeFormatFunction(name)) {
          report(arguments[i], DiagnosticsId::DI_FORMAT_ARGUMENTS_COUNT);
        }
      }

      return;
    }
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
  const SQFunctionProto *proto = nullptr;
  const SQNativeClosure *nclosure = nullptr;

  const SQChar *funcName;
  int numParams;
  int dpParameters;
  bool isVararg;

  if (info) {
    const FunctionDecl *decl = info->declaration;
    funcName = decl->name();
    numParams = info->parameters.size();
    isVararg = decl->isVararg();

    dpParameters = 0;
    for (auto &p : decl->parameters()) {
      if (p->hasDefaultValue())
        ++dpParameters;
    }
  }
  else {
    const ExternalValueExpr *ev = findExternalValue(callExpr->callee());
    if (!ev)
      return;

    const SQObject& v = ev->value();
    if (sq_isclosure(v)) {
      proto = _closure(v)->_function;
      funcName = sq_isstring(proto->_name) ? _stringval(proto->_name) : _SC("unknown");
      numParams = proto->_nparameters - 1; // not counting 'this'
      isVararg = proto->_varparams;
      dpParameters = proto->_ndefaultparams;
    }
    else if (sq_isfunction(v)) {
      assert(!"Encountered function proto - internal structure, should not happen");
      return;
    }
    else if (sq_isnativeclosure(v)) {
      nclosure = _nativeclosure(v);
      if (nclosure->_nparamscheck == 0)
        return;
      funcName = sq_isstring(nclosure->_name) ? _stringval(nclosure->_name) : _SC("unknown native");
      numParams = std::abs(nclosure->_nparamscheck)-1; // not counting 'this'
      isVararg = nclosure->_nparamscheck < 0;
      dpParameters = 0;
    }
    else
      return;
  }

  if (numParams < 0) numParams = 0;

  const int effectiveParamSizeUP = std::max<int>(isVararg ? numParams - 1 : numParams, 0);
  const int effectiveParamSizeLB = effectiveParamSizeUP - dpParameters;
  const int maxParamSize = isVararg ? INT_MAX : effectiveParamSizeUP;

  const auto &args = callExpr->arguments();

  if (!(effectiveParamSizeLB <= args.size() && args.size() <= maxParamSize)) {
    report(callExpr, DiagnosticsId::DI_PARAM_COUNT_MISMATCH, funcName,
      effectiveParamSizeLB, maxParamSize, args.size());
    report(maybeEval(callExpr->callee(), true), DI_SEE_OTHER, "the function");
  }

  for (int i = 0; i < numParams; ++i) {
    const SQChar *paramName;
    if (info) paramName = info->parameters[i];
    else if (proto) {
      if (!sq_isstring(proto->_parameters[i + 1])) continue;
      paramName = _stringval(proto->_parameters[i + 1]);
    }
    else {
      if (!nclosure)
        assert(0);
      continue;
    }

    for (int j = 0; j < args.size(); ++j) {
      const Expr *arg = args[j];
      const SQChar *possibleArgName = nullptr;

      if (arg->op() == TO_ID)
        possibleArgName = arg->asId()->name();
      else if (arg->op() == TO_GETFIELD)
        possibleArgName = arg->asGetField()->fieldName();

      if (!possibleArgName)
        continue;

      int32_t argNL = normalizeParamNameLength(possibleArgName);
      SQChar *buffer = (SQChar *)sq_malloc(_ctx.allocContext(), argNL + 1);
      normalizeParamName(possibleArgName, buffer);

      if (i != j) {
        if (strcmp("vargv", paramName) != 0 || (j != numParams)) {
          if (strcmp(paramName, buffer) == 0) {  // -V575
            report(arg, DiagnosticsId::DI_PARAM_POSITION_MISMATCH, paramName);
          }
        }
      }

      sq_free(_ctx.allocContext(), buffer, argNL + 1);
    }
  }
}

void CheckerVisitor::checkContainerModification(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *name = extractFunctionName(call);

  if (!name)
    return;

  if (!nameLooksLikeModifiesObject(name))
    return;

  const Expr *callee = call->callee();

  if (!callee->isAccessExpr())
    return;

  reportModifyIfContainer(callee->asAccessExpr()->receiver(), call);
}

// rename it to `isTemporaryObject` ?
static bool isIndeterminated(const Expr *e) {
  e = deparenStatic(e);

  if (e->op() == TO_NULLC) { // -V522
    const Expr *rhs = static_cast<const BinExpr *>(e)->rhs(); // -V522
    return rhs->op() == TO_ARRAYEXPR || rhs->op() == TO_DECL_EXPR;
  }

  if (e->op() == TO_TERNARY) {  // -V522
    const TerExpr *ter = static_cast<const TerExpr *>(e);
    const Expr *thenE = ter->b(); // -V522
    const Expr *elseE = ter->c(); // -V522

    return (thenE->op() == TO_ARRAYEXPR || thenE->op() == TO_DECL_EXPR) || (elseE->op() == TO_ARRAYEXPR || elseE->op() == TO_DECL_EXPR);
  }

  return false;
}

void CheckerVisitor::checkUnwantedModification(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *name = extractFunctionName(call);

  if (!name)
    return;

  if (!nameLooksLikeModifiesObject(name))
    return;

  const Expr *callee = call->callee();

  if (!callee->isAccessExpr())
    return;

  if (nodeStack.back().sst == SST_NODE) {
    const Node *p = nodeStack.back().n;
    if (p->op() == TO_NEWSLOT)
      return;
  }

  if (isIndeterminated(callee->asAccessExpr()->receiver()))
    report(call, DiagnosticsId::DI_UNWANTED_MODIFICATION, name);
}

void CheckerVisitor::checkCannotBeNull(const CallExpr *call) {
  if (effectsOnly)
    return;

  if (!call->isNullable())
    return;

  const Expr *callee = call->callee();

  reportIfCannotBeNull(maybeEval(callee), callee, "call");
}

void CheckerVisitor::checkBooleanLambda(const CallExpr *call) {
  if (effectsOnly)
    return;

  const SQChar *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeFunctionTakeBooleanLambda(fn))
    return;

  if (call->arguments().size() < 1)
    return;

  const Expr *arg = deparenStatic(call->arguments()[0]);

  if (arg->op() != TO_DECL_EXPR) // -V522
    return;

  const Decl *decl = arg->asDeclExpr()->declaration();

  if (decl->op() != TO_FUNCTION)
    return;

  const FunctionDecl *f = static_cast<const FunctionDecl *>(decl);

  FunctionReturnTypeEvaluator rte(this);

  bool r = false;
  unsigned typesMask = rte.compute(f->body(), r);

  typesMask &= ~(RT_UNRECOGNIZED | RT_FUNCTION_CALL | RT_BOOL | RT_NUMBER | RT_NULL);

  if (typesMask != 0) {
    report(arg, DiagnosticsId::DI_BOOL_LAMBDA_REQUIRED, fn);
  }
}

void CheckerVisitor::checkAssertCall(const CallExpr *call) {

  // assert(x != null) or assert(x != null, "X should not be null")

  const Expr *callee = maybeEval(call->callee());

  if (callee->op() != TO_ID)
    return;

  if (strcmp("assert", callee->asId()->name()) != 0)
    return;


  const SQUnsignedInteger argSize = call->arguments().size();

  if (!argSize || argSize > 2)
    return;

  const Expr *cond = call->arguments()[0];

  speculateIfConditionHeuristics(cond, currentScope, nullptr);
}

const SQChar *CheckerVisitor::extractFunctionName(const CallExpr *call) {
  const Expr *c = maybeEval(call->callee());

  const SQChar *calleeName = nullptr;
  if (c->op() == TO_ID)
    calleeName = c->asId()->name();
  else if (c->op() == TO_DECL_EXPR) {
    const Decl *decl = c->asDeclExpr()->declaration();
    if (decl->op() != TO_FUNCTION)
      return nullptr;

    calleeName = static_cast<const FunctionDecl *>(decl)->name();
  }
  else if (c->op() == TO_GETFIELD)
    calleeName = c->asGetField()->fieldName();

  return calleeName;
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

void CheckerVisitor::visitLiteralExpr(LiteralExpr *l) {
  checkForgotSubst(l);
}

void CheckerVisitor::visitId(Id *id) {
  const StackSlot &ss = nodeStack.back();
  const Node *parentNode = nullptr;

  if (ss.sst == SST_NODE)
    parentNode = ss.n;

  ValueRef *v = findValueInScopes(id->name());

  if (!v)
    return;

  checkForeachIteratorCapturedByClosure(id, v);
  checkIdUsed(id, parentNode, v);
}

void CheckerVisitor::visitUnExpr(UnExpr *expr) {
  checkContainerModification(expr);

  Visitor::visitUnExpr(expr);
}

void CheckerVisitor::visitBinExpr(BinExpr *expr) {
  checkAndOrPriority(expr);
  checkBitwiseParenBool(expr);
  checkNullCoalescingPriority(expr);
  checkAssignmentToItself(expr);
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
  checkNewGlobalSlot(expr);
  checkUselessNullC(expr);
  checkCannotBeNull(expr);
  checkCanBeSimplified(expr);
  checkRangeCheck(expr);

  Expr *lhs = expr->lhs();
  Expr *rhs = expr->rhs();

  switch (expr->op())
  {
  case TO_NULLC:
  case TO_OROR:
    nodeStack.push_back({ SST_NODE, expr });
    visitBinaryBranches(lhs, rhs, true);
    nodeStack.pop_back();
    break;
  case TO_ANDAND:
    nodeStack.push_back({ SST_NODE, expr });
    visitBinaryBranches(lhs, rhs, false);
    nodeStack.pop_back();
    break;
  case TO_PLUSEQ:
  case TO_MINUSEQ:
  case TO_MULEQ:
  case TO_DIVEQ:
  case TO_MODEQ:
  case TO_NEWSLOT:
  case TO_ASSIGN:
    nodeStack.push_back({ SST_NODE, expr });
    expr->rhs()->visit(this);
    expr->lhs()->visit(this);
    nodeStack.pop_back();
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
  checkCanBeSimplified(expr);

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
  const SQChar *name = computeNameRef(deparenStatic(expr->argument()), buffer, sizeof buffer);
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
  checkPersistCall(expr);
  checkForbiddenCall(expr);
  checkCallFromRoot(expr);
  checkForbiddenParentDir(expr);
  checkFormatArguments(expr);
  checkContainerModification(expr);
  checkUnwantedModification(expr);
  checkCannotBeNull(expr);
  checkBooleanLambda(expr);

  applyCallToScope(expr);

  checkAssertCall(expr);

  Visitor::visitCallExpr(expr);

  checkArguments(expr);
}

void CheckerVisitor::checkBoolIndex(const GetSlotExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *key = maybeEval(expr->key())->asExpression();

  if (isBooleanResultOperator(key->op())) {
    report(expr->key(), DiagnosticsId::DI_BOOL_AS_INDEX);
  }
}

void CheckerVisitor::checkNullableIndex(const GetSlotExpr *expr) {

  if (effectsOnly)
    return;

  const Expr *key = expr->key();

  if (!isSafeAccess(expr) && isPotentiallyNullable(key)) {
    report(expr->key(), DiagnosticsId::DI_POTENTIALLY_NULLABLE_INDEX);
  }
}

void CheckerVisitor::checkGlobalAccess(const GetFieldExpr *expr) {
  if (expr->receiver()->op() != TO_ROOT_TABLE_ACCESS)
    return;

  const BinExpr *newslot = nullptr;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst != SST_NODE)
      continue;

    const Node *n = it->n;

    if (n->op() != TO_NEWSLOT)
      continue;

    newslot = static_cast<const BinExpr *>(n);
    break;
  }

  if (newslot) {
    const Expr *lhs = deparen(newslot->lhs());
    if (lhs == expr)
      return;
  }

  storeGlobalUsage(expr->fieldName(), expr);
}

void CheckerVisitor::visitGetFieldExpr(GetFieldExpr *expr) {
  checkAccessNullable(expr);
  checkEnumConstUsage(expr);
  checkGlobalAccess(expr);
  checkAccessFromStatic(expr);

  Visitor::visitGetFieldExpr(expr);

  checkExternalField(expr);
}

void CheckerVisitor::visitGetSlotExpr(GetSlotExpr *expr) {
  checkBoolIndex(expr);
  checkNullableIndex(expr);
  checkAccessNullable(expr);

  Visitor::visitGetSlotExpr(expr);
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

  FunctionReturnTypeEvaluator rtEvaluator(this);

  const Statement *last = nullptr;
  for (auto &c : cases) {

    if (last) {
      report(last, DiagnosticsId::DI_MISSED_BREAK);
    }
    last = nullptr;

    const Statement *stmt = c.stmt;
    bool r = false;
    rtEvaluator.compute(stmt, r);
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

static bool wrappedBody(const Statement *stmt) {
  if (stmt->op() != TO_BLOCK)
    return false;

  const Block *b = stmt->asBlock();

  if (b->statements().size() != 1)
    return false;

  const Statement *wp = b->statements().back();

  return stmt->lineStart() == wp->lineStart()
    && stmt->lineEnd() == wp->lineEnd()
    && stmt->columnStart() == wp->columnStart()
    && stmt->columnEnd() == wp->columnEnd();
}

void CheckerVisitor::checkSuspiciousFormattingOfStetementSequence(const Statement *prev, const Statement *cur)
{
  if (prev && cur && prev->lineStart() != cur->lineStart() && prev->columnStart() != cur->columnStart())
    report(cur, DiagnosticsId::DI_INVALID_INDENTATION, std::to_string(prev->lineStart()).c_str(), std::to_string(cur->lineStart()).c_str());
}

void CheckerVisitor::checkSuspiciousFormatting(const Statement *body, const Statement *stmt) {
  if (wrappedBody(body)) {
    if (stmt->lineStart() != body->lineStart() && stmt->columnStart() >= body->columnStart()) {

      // check if it is not `else if (...) ... ` pattern
      const IfStatement *elif = nullptr;
      const size_t nssize = nodeStack.size();
      if (nssize >= 2) {
        for (int i = 1; i <= 2; ++i) {
          auto &ss = nodeStack[nssize - i];
          if (ss.sst == SST_NODE && ss.n->op() == TO_IF) {
            elif = static_cast<const IfStatement *>(ss.n);
            break;
          }
        }
      }

      if (elif)
        return;

      report(body, DiagnosticsId::DI_SUSPICIOUS_FMT);
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
        varname = l->asId()->name();
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

      if (l->op() == TO_ID) {
        if (strcmp(l->asId()->name(), varname) != 0) {
          if (r->op() != TO_ID || (strcmp(r->asId()->name(), varname) != 0)) {
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
        if (strcmp(varname, lhs->asId()->name()) != 0) {
          report(mod, DiagnosticsId::DI_MISMATCH_LOOP_VAR);
        }
      }
    }

    if (mod->op() == TO_INC) {
      Expr *arg = static_cast<IncExpr *>(mod)->argument();
      if (arg->op() == TO_ID) {
        if (strcmp(varname, arg->asId()->name()) != 0) {
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
            if (!ignore && firstAssignee->op() == TO_GETSLOT) {
              const GetSlotExpr *getT = firstAssignee->asGetSlot();
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

static const FunctionDecl *extractFunction(const Node *n) {

  if (!n)
    return nullptr;

  if (n->op() == TO_FUNCTION || n->op() == TO_CONSTRUCTOR)
    return static_cast<const FunctionDecl *>(n);

  if (n->op() == TO_VAR)
    return extractFunction(n->asDeclaration()->asVarDecl()->initializer());

  if (n->op() == TO_DECL_EXPR)
    return extractFunction(n->asExpression()->asDeclExpr()->declaration());

  return nullptr;
}

void CheckerVisitor::checkFunctionSimilarity(const Block *b) {
  if (effectsOnly)
    return;

  const auto &statements = b->statements();

  for (int32_t i = 0; i < int32_t(statements.size()); ++i) {
    const FunctionDecl *f1 = extractFunction(statements[i]);

    if (!f1)
      continue;

    int32_t f1Complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold);

    if (f1Complexity >= functionComplexityThreshold) {

      for (int32_t j = i + 1; j < int32_t(statements.size()); ++j) {
        const FunctionDecl *f2 = extractFunction(statements[j]);

        if (!f2)
          continue;

        int32_t diff = NodeDiffComputer::compute(f1->body(), f2->body(), 4);
        if (diff < 4) {
          const SQChar *name1 = f1->name();
          const SQChar *name2 = f2->name();

          if (diff == 0)
            report(f2, DiagnosticsId::DI_DUPLICATE_FUNC, name1, name2);
          else {
            f1Complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold * 3);
            if (diff <= f1Complexity / functionComplexityThreshold)
              report(f2, DiagnosticsId::DI_SIMILAR_FUNC, name1, name2);
          }
        }
      }
    }
  }
}

static const Expr *extractAssignedExpression(const Node *n) {
  if (n->op() == TO_ASSIGN || n->op() == TO_NEWSLOT)
    return static_cast<const BinExpr *>(n)->rhs();

  if (n->op() == TO_VAR)
    return static_cast<const VarDecl *>(n)->initializer();

  return nullptr;
}

void CheckerVisitor::checkAssignExpressionSimilarity(const Block *b) {
  if (effectsOnly)
    return;

  const auto &statements = b->statements();

  for (int32_t i = 0; i < int32_t(statements.size()); ++i) {

    const Expr *assigned1 = extractAssignedExpression(statements[i]);

    if (!assigned1)
      continue;

    int32_t complexity = NodeComplexityComputer::compute(assigned1, statementComplexityThreshold);

    if (complexity >= statementComplexityThreshold) {

      for (int32_t j = i + 1; j < int32_t(statements.size()); ++j) {
        const Expr *assigned2 = extractAssignedExpression(statements[j]);

        if (!assigned2)
          continue;

        int32_t diff = NodeDiffComputer::compute(assigned1, assigned2, 3);
        if (diff < 3) {
          if (diff == 0)
            report(assigned2, DiagnosticsId::DI_DUPLICATE_ASSIGN_EXPR);
          else {
            complexity = NodeComplexityComputer::compute(assigned1, statementSimilarityThreshold * 2);
            if (diff <= complexity / statementSimilarityThreshold)
              report(assigned2, DiagnosticsId::DI_SIMILAR_ASSIGN_EXPR);
          }
        }
      }
    }
  }
}

bool CheckerVisitor::isCallResultShouldBeUtilized(const SQChar *name, const CallExpr *call) {
  if (!name)
    return false;

  if (nameLooksLikeResultMustBeUtilized(name)) {
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
      if (nameLooksLikeResultMustBeBoolean(arg->asId()->name())) {
        return false;
      }
    }

    if (arg->op() == TO_CALL) {
      const CallExpr *acall = arg->asCallExpr();
      const Expr *callee = acall->callee();
      const SQChar *calleeName = nullptr;
      if (callee->op() == TO_ID) {
        calleeName = callee->asId()->name();
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
  checkFunctionSimilarity(b);
  checkAssignExpressionSimilarity(b);

  VarScope *thisScope = currentScope;
  VarScope blockScope(thisScope ? thisScope->owner : nullptr, thisScope);
  currentScope = &blockScope;

  nodeStack.push_back({ SST_NODE, b });

  Statement* prevStatement = nullptr;

  for (Statement *s : b->statements()) {
    s->visit(this);
    blockScope.evalId += 1;

    if (s->op() != TO_EMPTY) {
      checkSuspiciousFormattingOfStetementSequence(prevStatement, s);
      prevStatement = s;
    }
  }

  nodeStack.pop_back();

  blockScope.parent = nullptr;
  blockScope.checkUnusedSymbols(this);
  currentScope = thisScope;
}

void CheckerVisitor::visitForStatement(ForStatement *loop) {
  checkUnterminatedLoop(loop);
  checkVariableMismatchForLoop(loop);
  checkSuspiciousFormatting(loop->body(), loop);

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
  checkSuspiciousFormatting(loop->body(), loop);


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
  checkSuspiciousFormatting(loop->body(), loop);

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

static const Id *extractReceiver(const Expr *e) {
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

bool CheckerVisitor::detectTypeOfPattern(const Expr *expr, const Expr *&res_checkee, const LiteralExpr *&res_lit) {
  // Either of 'typeof checkee ==/!= "lit"' or 'type(checkee) ==/!= "lit"
  expr = deparenStatic(expr);

  enum TreeOp op = expr->op(); // -V522

  if (op != TO_EQ && op != TO_NE)
    return false; // not typeof pattern

  const BinExpr *bin = expr->asBinExpr();

  const Expr *lhs = deparenStatic(bin->lhs());
  const Expr *rhs = deparenStatic(bin->rhs());

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
    const UnExpr *typeOf = static_cast<const UnExpr *>(checkExpr);
    res_checkee = deparenStatic(typeOf->argument()); // -V522
    res_lit = lit;
    return true;
  }

  if (checkExpr->op() == TO_CALL) {
    // check for `type(check)` pattern

    const CallExpr *call = checkExpr->asCallExpr();
    const Expr *callee = maybeEval(call->callee());

    if (callee->op() != TO_ID || strcmp("type", callee->asId()->name()) != 0) {
      return false;
    }

    if (call->arguments().size() != 1)
      return false;

    res_checkee = deparenStatic(call->arguments()[0]);
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
  // (o?.f ?? D) >= V -- else-branch implies `o` non-null
  // (o?.f ?? D) <= V -- else-branch implies `o` non-null

  if (op != TO_LT && op != TO_GT && op != TO_EQ && op != TO_NE && op != TO_LE && op != TO_GE) {
    return false;
  }

  const BinExpr *bin = cond->asBinExpr();

  const Expr *lhs = deparenStatic(bin->lhs());
  const Expr *rhs = deparenStatic(bin->rhs());

  if (lhs->op() != TO_NULLC && rhs->op() != TO_NULLC) // -V522
    return false;

  const BinExpr *nullc = lhs->op() == TO_NULLC ? lhs->asBinExpr() : rhs->asBinExpr();
  const Expr *V = lhs == nullc ? rhs : lhs;
  const Expr *D = deparenStatic(nullc->rhs());
  const Expr *candidate = deparenStatic(nullc->lhs());

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
  cond = deparenStatic(cond);

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

    if (op == TO_LE || op == TO_GE) {
      if (elseScope) {
        currentScope = elseScope;
        setValueFlags(nullcCheckee, 0, RT_NULL);
      }
    } else if (op == TO_EQ) {
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

  if (op == TO_NULLC) {
    // o?.f ?? false
    // o?.f ?? true
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = deparenStatic(bin->lhs());
    const Expr *rhs = deparenStatic(bin->rhs());

    if (rhs->op() != TO_LITERAL) { // -V522
      return;
    }

    const LiteralExpr *lit = rhs->asLiteral();
    bool nullCond;
    switch (lit->kind())
    {
    case LK_BOOL:
      nullCond = rhs->asLiteral()->b();
      break;
    case LK_NULL:
      nullCond = false;
      break;
    case LK_INT:
      nullCond = lit->i() != 0;
      break;
    case LK_FLOAT:
      nullCond = lit->f() != 0.0f;
      break;
    case LK_STRING:
      nullCond = true;
      break;
    default:
      assert(0 && "unknown literal kind");
      break;
    }

    unsigned pf, nf;

    if (nullCond) {
      pf = RT_NULL;
      nf = 0;
    }
    else {
      pf = 0;
      nf = RT_NULL;
    }

    const Expr *receiver = extractReceiver(lhs);

    if (!receiver)
      return;

    if (thenScope && !nullCond) {
      currentScope = thenScope;
      setValueFlags(receiver, pf, nf);
    }

    if (elseScope && nullCond) {
      currentScope = elseScope;
      setValueFlags(receiver, nf, pf); // -V764
    }

    currentScope = thisScope; // -V519

    return;
  }

  if (evalId == -1 && op == TO_CALL && thenScope) {
    // if condition looks like `if (foo(x)) x.bar()` consider call as null check
    currentScope = thenScope;
    const CallExpr *call = cond->asCallExpr();
    for (auto parg : call->arguments()) {
      auto arg = deparenStatic(parg);
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
    int32_t evalIndex = -1;
    const Expr *eval = maybeEval(cond, evalIndex);

    bool notOverriden = evalId == -1 || evalIndex < evalId;

    if (thenScope && notOverriden) {
      // set iff it was explicit check like `if (o) { .. }`
      // otherwise there could be complexities, see intersected_assignment.nut
      currentScope = thenScope;
      setValueFlags(cond, 0, RT_NULL);
      currentScope = thisScope;
    }

    if (elseScope && notOverriden && (flags & NULL_CHECK_F)) {
      // set NULL iff it was explicit null check `if (o == null) { ... }` otherwise it could not be null, see w233_inc_in_for.nut
      currentScope = elseScope;
      setValueFlags(cond, RT_NULL, 0);
      currentScope = thisScope;
    }

    if (eval != cond) {
      // let cond = x != null
      // if (cond) { ... }
      speculateIfConditionHeuristics(eval, thenScope, elseScope, visited, evalIndex, flags, false);
    }
    return;
  }

  if (op == TO_GETSLOT || op == TO_GETFIELD) {
    // x?[y]
    const AccessExpr *acc = static_cast<const AccessExpr *>(cond);
    const Expr *reciever = extractReceiver(deparenStatic(acc->receiver()));

    if (reciever && thenScope) {
      currentScope = thenScope;
      setValueFlags(reciever, 0, RT_NULL);
    }

    if (op == TO_GETSLOT) {
      const Expr *key = deparenStatic(acc->asGetSlot()->key());
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
    const Expr *lhs = deparenStatic(bin->lhs());
    const Expr *rhs = deparenStatic(bin->rhs());

    const LiteralExpr *lhs_lit = lhs->op() == TO_LITERAL ? lhs->asLiteral() : nullptr; // -V522
    const LiteralExpr *rhs_lit = rhs->op() == TO_LITERAL ? rhs->asLiteral() : nullptr; // -V522

    const LiteralExpr *lit = lhs_lit ? lhs_lit : rhs_lit;
    const Expr *testee = lit == lhs ? rhs : lhs;

    if (!lit)
      return;

    if (lit->kind() == LK_NULL) {
      speculateIfConditionHeuristics(testee, elseScope, thenScope, visited, evalId, flags | NULL_CHECK_F, false);
      return;
    }

    const Expr *receiver = extractReceiver(testee);

    if (receiver && thenScope) {
      currentScope = thenScope;
      setValueFlags(receiver, 0, RT_NULL);
      currentScope = thisScope;
    }

  }

  if (isRelationOperator(op)) {
    const BinExpr *bin = cond->asBinExpr();
    const Expr *lhs = deparenStatic(bin->lhs());
    const Expr *rhs = deparenStatic(bin->rhs());

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
  checkSuspiciousFormatting(ifstmt->thenBranch(), ifstmt);

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

  declareSymbol(id->name(), v);

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
  unsigned returnFlags = FunctionReturnTypeEvaluator(this).compute(func->body(), dummy);

  bool reported = false;

  if (returnFlags & ~(RT_BOOL | RT_UNRECOGNIZED | RT_FUNCTION_CALL | RT_NULL)) { // Null is castable to boolean
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

void CheckerVisitor::checkFunctionSimilarity(const TableDecl *table) {
  if (effectsOnly)
    return;

  const auto &members = table->members();

  for (int32_t i = 0; i < int32_t(members.size()); ++i) {
    const auto &m1 = members[i];
    const FunctionDecl *f1 = extractFunction(m1.value);

    if (!f1)
      continue;

    int32_t complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold);
    if (complexity >= functionComplexityThreshold) {
      for (int32_t j = i + 1; j < members.size(); ++j) {
        const auto &m2 = members[j];
        const FunctionDecl *f2 = extractFunction(m2.value);

        if (!f2)
          continue;

        int32_t diff = NodeDiffComputer::compute(f1->body(), f2->body(), 4);
        if (diff < 4) {
          const SQChar *name1 = f1->name();
          const SQChar *name2 = f2->name();

          if (diff == 0)
            report(f2, DiagnosticsId::DI_DUPLICATE_FUNC, name1, name2);
          else {
            complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold * 3);
            if (diff <= complexity / functionComplexityThreshold)
              report(f2, DiagnosticsId::DI_SIMILAR_FUNC, name1, name2);
          }
        }
      }
    }
  }
}

void CheckerVisitor::checkAccessNullable(const DestructuringDecl *dd) {
  if (effectsOnly)
    return;

  const Expr *i = dd->initExpression();
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
      report(dd, DiagnosticsId::DI_ACCESS_POT_NULLABLE, i->op() == TO_ID ? i->asId()->name() : "expression", "container");
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
  checkFunctionSimilarity(table);

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
  assert(bin->op() == TO_ASSIGN);

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  if (lhs->op() != TO_ID)
    return;

  const SQChar *name = lhs->asId()->name();
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
  //case TO_GETSLOT: return computeNameLength(e->asGetSlot());
  case TO_ID: return strlen(e->asId()->name());
  case TO_ROOT_TABLE_ACCESS: return sizeof double_colon_str;
  case TO_BASE: return sizeof base_str;
    // TODO:
  default:
    return -1;
  }
}

void CheckerVisitor::computeNameRef(const Expr *e, SQChar *b, int32_t &ptr, int32_t size) {
  switch (e->op())
  {
  case TO_GETFIELD: return computeNameRef(e->asGetField(), b, ptr, size);
  //case TO_GETSLOT: return computeNameRef(lhs->asGetSlot());
  case TO_ID: {
    int32_t l = snprintf(&b[ptr], size - ptr, "%s", e->asId()->name());
    ptr += l;
    break;
  }
  case TO_ROOT_TABLE_ACCESS:
    snprintf(&b[ptr], size - ptr, "%s", double_colon_str);
    ptr += sizeof double_colon_str;
    break;
  case TO_BASE:
    snprintf(&b[ptr], size - ptr, "%s", base_str);
    ptr += sizeof base_str;
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
  e = deparenStatic(e);
  if (!e) return nullptr;

  if (auto it = astValues.find(e); it != astValues.end())
    return it->second;

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

  e = deparenStatic(e);

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
      funcName = callee->asId()->name();
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

  if (e->op() == TO_TERNARY) {
    const TerExpr *t = static_cast<const TerExpr *>(e);
    return isPotentiallyNullable(t->b(), visited) || isPotentiallyNullable(t->c(), visited);
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

  e = deparenStatic(e);

  if (e->op() == TO_LITERAL) { // -V522
    return e->asLiteral()->kind() == LK_STRING;
  }

  if (e->op() == TO_NULLC) {
    const BinExpr *b = static_cast<const BinExpr *>(e);
    if (b->rhs()->op() == TO_LITERAL && b->rhs()->asLiteral()->kind() == LK_STRING) // -V522
      return couldBeString(b->lhs());
  }

  if (e->op() == TO_CALL) { // -V522
    const SQChar *name = nullptr;
    const Expr *callee = static_cast<const CallExpr *>(e)->callee();

    if (callee->op() == TO_ID)
      name = callee->asId()->name();
    else if (callee->op() == TO_GETFIELD)
      name = callee->asGetField()->fieldName();

    if (name) {
      return nameLooksLikeResultMustBeString(name) || strcmp(name, "loc") == 0;
    }
  }

  if (e->op() == TO_ADD) { // -V522
    // check for string concat
    const BinExpr *b = e->asBinExpr();
    return couldBeString(b->lhs()) || couldBeString(b->rhs());
  }

  const Expr *evaled = maybeEval(e);

  if (evaled != e && evaled->op() == TO_LITERAL) // do not go too deep
    return couldBeString(evaled);

  return false;
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, bool allow_external) {
  int32_t dummy = -1;
  return maybeEval(e, dummy, allow_external);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId, bool allow_external) {
  std::unordered_set<const Expr *> visited;
  return maybeEval(e, evalId, visited, allow_external);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited, bool allow_external) {

  if (visited.find(e) != visited.end())
    return e;

  visited.emplace(e);

  e = deparenStatic(e);
  const ValueRef *v = findValueForExpr(e);

  if (!v) {
    return e;
  }

  evalId = v->evalIndex;
  if (v->hasValue()) {
    if (!allow_external && v->expression && v->expression->op() == TO_EXTERNAL_VALUE)
      return e;
    return maybeEval(v->expression, evalId, visited, allow_external);
  }
  else {
    return e;
  }
}

const SQChar *CheckerVisitor::findFieldName(const Expr *e) {
  if (e->op() == TO_ID)
    return e->asId()->name();

  if (e->op() == TO_GETFIELD)
    return e->asGetField()->fieldName();

  if (e->op() == TO_CALL)
    return findFieldName(e->asCallExpr()->callee());

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

const ExternalValueExpr *CheckerVisitor::findExternalValue(const Expr *e) {
  const Expr *ee = maybeEval(e, true);

  if (ee->op() == TO_EXTERNAL_VALUE)
    return static_cast<const ExternalValueExpr *>(ee);

  SQChar buffer[128] = { 0 };
  const SQChar *name = computeNameRef(ee, buffer, sizeof buffer);
  if (!name)
    return nullptr;

  const ValueRef *v = findValueInScopes(name);

  if (!v || !v->hasValue())
    return nullptr;

  const Expr *expr = maybeEval(v->expression, true);

  if (expr->op() == TO_EXTERNAL_VALUE)
    return static_cast<const ExternalValueExpr *>(expr);

  return nullptr;
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
    else if (expr->op() == TO_EXTERNAL_VALUE) {
      applyUnknownInvocationToScope();
      return;
    }
  }

  if (!info) {
    // probably it is class constructor
    return;
  }

  for (auto s : info->modifiable) {
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
  const Expr *callee = deparenStatic(call->callee());

  if (callee->op() == TO_ID) { // -V522
    const Id *calleeId = callee->asId();
    //const NameRef ref(nullptr, calleeId->name());
    const ValueRef *value = findValueInScopes(calleeId->name());
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
    else if (!ref || strncmp(ref, double_colon_str, sizeof double_colon_str) != 0) {
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
  init = maybeEval(init, true);
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
  astValues[decl] = v;
}

void CheckerVisitor::visitConstDecl(ConstDecl *cnst) {

  if (cnst->isGlobal()) {
    storeGlobalDeclaration(cnst->name(), cnst);
  }

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

  if (enm->isGlobal()) {
    storeGlobalDeclaration(enm->name(), enm);
  }

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

void CheckerVisitor::checkDestructuredVarDefault(VarDecl *var) {
  const Expr *def = var->initializer();
  def = maybeEval(def, true);
  if (def) {
    auto vv = astValues[var];
    assert(vv);
    vv->state = VRS_INITIALIZED;
    vv->expression = def;
  }
  else {
    report(var, DiagnosticsId::DI_MISSING_DESTRUCTURED_VALUE, var->name());
  }
}

void CheckerVisitor::visitDestructuringDecl(DestructuringDecl *d) {

  checkAccessNullable(d);

  Visitor::visitDestructuringDecl(d);

  if (auto v = findValueForExpr(d->initExpression()); v && v->hasValue() && v->expression) {
    if (v->expression->op() == TO_EXTERNAL_VALUE) {
      const auto &init = v->expression->asExternal()->value();
      if (sq_isnull(init)) {
        for (auto var : d->declarations()) {
          checkDestructuredVarDefault(var);
        }
      }
      else if (sq_istable(init)) {
        if (d->type() == DT_TABLE) {
          auto table = _table(init);
          for (auto var : d->declarations()) {
            SQObjectPtr val;
            if (table->GetStr(var->name(), strlen(var->name()), val)) {
              auto vv = astValues[var];
              assert(vv);
              vv->state = VRS_INITIALIZED;
              vv->expression = addExternalValue(val, var)->expression;
            }
            else {
              checkDestructuredVarDefault(var);
            }
          }
        }
        else {
          report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, "table");
        }
      }
      else if (sq_isclass(init)) {
        if (d->type() == DT_TABLE) {
          auto klass = _class(init);
          for (auto var : d->declarations()) {
            SQObjectPtr key(_ctx.getVm(), var->name());
            SQObjectPtr val;
            if (klass->Get(key, val)) {
              auto vv = astValues[var];
              assert(vv);
              vv->state = VRS_INITIALIZED;
              vv->expression = addExternalValue(val, var)->expression;
            }
            else {
              checkDestructuredVarDefault(var);
            }
          }
        }
        else {
          report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, "class");
        }
      }
      else if (sq_isinstance(init)) {
        if (d->type() == DT_TABLE) {
          auto inst = _instance(init);
          for (auto var : d->declarations()) {
            SQObjectPtr key(_ctx.getVm(), var->name());
            SQObjectPtr val;
            if (inst->Get(key, val)) {
              auto vv = astValues[var];
              assert(vv);
              vv->state = VRS_INITIALIZED;
              vv->expression = addExternalValue(val, var)->expression;
            }
            else {
              checkDestructuredVarDefault(var);
            }
          }
        }
        else {
          report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, "instance");
        }
      }
      else if (sq_isarray(init)) {
        if (d->type() == DT_ARRAY) {
          auto array = _array(init);
          SQInteger index = 0;
          for (auto var : d->declarations()) {
            SQObjectPtr val;
            if (array->Get(index, val)) {
              auto vv = astValues[var];
              assert(vv);
              vv->state = VRS_INITIALIZED;
              vv->expression = addExternalValue(val, var)->expression;
            }
            else {
              checkDestructuredVarDefault(var);
            }
            index++;
          }
        }
        else {
          report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, "array");
        }
      }
      else {
        report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, GetTypeName(init));
      }
    }
    else if (v->expression->op() == TO_TABLE) {
      // TODO: check table destructuring
    }
    else if (v->expression->op() == TO_ARRAYEXPR) {
      // TODO: check array destructuring
    }
  }
}

ValueRef* CheckerVisitor::addExternalValue(const SQObject &val, const Node *location) {
  SymbolInfo *info = makeSymbolInfo(SK_EXTERNAL_BINDING);
  ValueRef *v = makeValueRef(info);
  ExternalValueExpr *ev = new(arena) ExternalValueExpr(val);
  if (location)
    ev->copyLocationFrom(*location);
  externalValues.push_back(ev);

  info->declarator.ev = ev;
  v->state = VRS_INITIALIZED;
  v->expression = ev;
  if (sq_isnull(val)) v->flagsPositive |= RT_NULL;
  // TODO: could create LiteralExpr for some values

  return v;
}

void CheckerVisitor::analyze(RootBlock *root, const HSQOBJECT *bindings) {
  assert(currentScope == nullptr);
  VarScope rootScope(nullptr, nullptr);
  currentScope = &rootScope;

  if (bindings && sq_istable(*bindings)) {
    SQTable *table = _table(*bindings);

    SQInteger idx = 0;
    SQObjectPtr pos(idx), key, val;

    while ((idx = table->Next(false, pos, key, val)) >= 0) {
      if (sq_isstring(key)) {
        const SQChar *name = _string(key)->_val;
        declareSymbol(name, addExternalValue(val, root));
      }
      pos._unVal.nInteger = idx;
    }
  }

  root->visit(this);

  rootScope.parent = nullptr;
  currentScope = nullptr;
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

  struct Scope rootScope;

  struct Scope *scope;

  void loadBindings(const HSQOBJECT *bindings);

  void declareVar(enum SymbolKind k, const VarDecl *v);
  void declareSymbol(const SQChar *name, SymbolInfo *info);

  Id rootPointerNode;
public:
  NameShadowingChecker(SQCompilationContext &ctx, const HSQOBJECT *bindings)
    : _ctx(ctx)
    , rootScope(this, nullptr)
    , scope(&rootScope)
    , rootPointerNode("<root>") {
    rootScope.parent = nullptr;
    loadBindings(bindings);
  }

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

  void analyze(RootBlock *root) {
    root->visit(this);
  }
};

void NameShadowingChecker::loadBindings(const HSQOBJECT *bindings) {
  if (bindings && sq_istable(*bindings)) {
    SQTable *table = _table(*bindings);

    SQInteger idx = 0;
    SQObjectPtr pos(idx), key, val;

    while ((idx = table->Next(false, pos, key, val)) >= 0) {
      if (sq_isstring(key)) {
        const SQChar *s = _string(key)->_val;
        SymbolInfo *info = newSymbolInfo(SK_EXTERNAL_BINDING);
        declareSymbol(s, info);
      }
      pos._unVal.nInteger = idx;
    }
  }
}

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
  case SK_EXTERNAL_BINDING:
    return &rootPointerNode;
  default:
    assert(0);
    return nullptr;
  }
}

void NameShadowingChecker::declareSymbol(const SQChar *name, SymbolInfo *info) {
  const SymbolInfo *existedInfo = scope->findSymbol(name);
  if (existedInfo) {
    bool warn = name[0] != '_';
    if (strcmp(name, "this") == 0) {
      warn = false;
    }

    if (existedInfo->ownerScope == info->ownerScope) { // something like `let foo = expr<function foo() { .. }>`
      if ((info->kind == SK_BINDING || info->kind == SK_VAR) && existedInfo->kind == SK_FUNCTION) {
        const VarDecl *vardecl = info->declaration.v;
        const FunctionDecl *funcdecl = existedInfo->declaration.f;
        const Expr *varinit = vardecl->initializer();

        if (varinit && varinit->op() == TO_DECL_EXPR && varinit->asDeclExpr()->declaration() == funcdecl) {
          warn = false;
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
  const FunctionDecl *f = nullptr;
  if (k == SK_BINDING) {
    const Expr *init = var->initializer();
    if (init && init->op() == TO_DECL_EXPR) {
      const Decl *d = init->asDeclExpr()->declaration();
      if (d->op() == TO_FUNCTION) {
        k = SK_FUNCTION;
        f = static_cast<const FunctionDecl *>(d);
      }
    }
  }


  SymbolInfo *info = newSymbolInfo(k);
  if (f) {
    info->declaration.f = f;
  }
  else {
    info->declaration.v = var;
  }
  info->ownerScope = scope;
  info->name = var->name();
  declareSymbol(var->name(), info);
}

void NameShadowingChecker::visitVarDecl(VarDecl *var) {
  Visitor::visitVarDecl(var);

  declareVar(var->isAssignable() ? SK_VAR : SK_BINDING, var);
}

void NameShadowingChecker::visitParamDecl(ParamDecl *p) {
  Visitor::visitParamDecl(p);

  SymbolInfo *info = newSymbolInfo(SK_PARAM);
  info->declaration.p = p;
  info->ownerScope = scope;
  info->name = p->name();
  declareSymbol(p->name(), info);
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

  Scope funcScope(this, f);

  bool tableScope = p->owner && (p->owner->op() == TO_CLASS || p->owner->op() == TO_TABLE);

  if (!tableScope) {
    SymbolInfo * info = newSymbolInfo(SK_FUNCTION);
    info->declaration.f = f;
    info->ownerScope = p;
    info->name = f->name();
    declareSymbol(f->name(), info);
  }

  Visitor::visitFunctionDecl(f);
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
  Scope blockScope(this, scope->owner);
  Visitor::visitBlock(block);
}

void NameShadowingChecker::visitTryStatement(TryStatement *stmt) {

  nodeStack.push_back(stmt);

  stmt->tryStatement()->visit(this);

  Scope catchScope(this, scope->owner);

  SymbolInfo *info = newSymbolInfo(SK_EXCEPTION);
  info->declaration.x = stmt->exceptionId();
  info->ownerScope = scope;
  info->name = stmt->exceptionId()->name();
  declareSymbol(stmt->exceptionId()->name(), info);

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

void StaticAnalyzer::reportGlobalNamesWarnings(HSQUIRRELVM vm) {
  auto errorFunc = _ss(vm)->_compilererrorhandler;

  if (!errorFunc)
    return;

  // 1. Check multiple declarations

  std::string message;

  for (auto it = declaredGlobals.begin(); it != declaredGlobals.end(); ++it) {
    const SQChar *name = it->first.c_str();
    const auto &declarations = it->second;

    if (declarations.size() == 1)
      continue;

    for (int32_t i = 0; i < declarations.size(); ++i) {
      const IdLocation &loc = declarations[i];
      if (loc.diagSilenced)
        continue;

      message.clear();
      SQCompilationContext::renderDiagnosticHeader(DiagnosticsId::DI_GLOBAL_NAME_REDEF, &message, name);
      errorFunc(vm, SEV_WARNING, message.c_str(), loc.filename, loc.line, loc.column, "\n");
    }
  }

  // 2. Check undefined usages

  for (auto it = usedGlobals.begin(); it != usedGlobals.end(); ++it) {
    const std::string &id = it->first;

    bool isKnownBinding = knownBindings.find(id) != knownBindings.end();
    if (isKnownBinding)
      continue;

    if (declaredGlobals.find(id) != declaredGlobals.end())
      continue;

    const auto &usages = it->second;

    for (int32_t i = 0; i < usages.size(); ++i) {
      const IdLocation &loc = usages[i];
      if (loc.diagSilenced)
        continue;

      message.clear();
      SQCompilationContext::renderDiagnosticHeader(DiagnosticsId::DI_UNDEFINED_GLOBAL, &message, id.c_str());
      errorFunc(vm, SEV_WARNING, message.c_str(), loc.filename, loc.line, loc.column, "\n");
    }
  }
}

static bool isSpaceOrTab(SQChar c) { return c == '\t' || c == ' '; }

void StaticAnalyzer::checkTrailingWhitespaces(HSQUIRRELVM vm, const SQChar *sourceName, const SQChar *code, size_t codeSize) {
  Arena arena(_ss(vm)->_alloc_ctx, "tmp");
  SQCompilationContext ctx(vm, &arena, sourceName, code, codeSize, nullptr, true);

  int32_t line = 1;
  int32_t column = 1;

  for (int32_t idx = 0; idx < codeSize - 1; ++idx, ++column) {
    if (isSpaceOrTab(code[idx])) {
      int next = code[idx + 1];
      if (!next || next == '\n' || next == '\r') {
        ctx.reportDiagnostic(DiagnosticsId::DI_SPACE_AT_EOL, line, column - 1, 1);
      }
    }
    else if (code[idx] == '\n') {
      column = 0;
      line++;
    }
  }
}

void StaticAnalyzer::mergeKnownBindings(const HSQOBJECT *bindings) {
  if (bindings && sq_istable(*bindings)) {
    SQTable *table = _table(*bindings);

    SQInteger idx = 0;
    SQObjectPtr pos(idx), key, val;

    while ((idx = table->Next(false, pos, key, val)) >= 0) {
      if (sq_isstring(key)) {
        SQInteger len = _string(key)->_len;
        const SQChar *s = _string(key)->_val;
        knownBindings.emplace(std::string(s, s+len));
      }
      pos._unVal.nInteger = idx;
    }
  }
}

void StaticAnalyzer::runAnalysis(RootBlock *root, const HSQOBJECT *bindings)
{
  mergeKnownBindings(bindings);
  CheckerVisitor(_ctx).analyze(root, bindings);
  NameShadowingChecker(_ctx, bindings).analyze(root);
}

}
