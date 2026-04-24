#pragma once

namespace SQCompilation
{


class NodeComplexityComputer : public Visitor
{
  int32_t complexity;
  const int32_t limit;

  NodeComplexityComputer(int32_t l) : limit(l), complexity(0) {}

public:
  void visitNode(Node *n) override {
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

  void visitTableExpr(TableExpr *t) {
    complexity += (t->members().size() * 2); // key + value

    Visitor::visitTableExpr(t);
  }

  void visitClassExpr(ClassExpr *c) {
    if (c->classBase())
      complexity += 1;

    if (c->classKey())
      complexity += 1;

    Visitor::visitClassExpr(c);
  }

  void visitFunctionExpr(FunctionExpr *f) {
    complexity += f->parameters().size();

    Visitor::visitFunctionExpr(f);
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

}
