#pragma once

#include "compiler/compilationcontext.h"
#include "compiler/ast.h"
#include "node_equal_checker.h"


namespace SQCompilation
{

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

  void visitFunctionExpr(FunctionExpr *e) { /* skip */ }

  bool check(Node *tree) {
    tree->visit(this);
    return foundUsage || foundInterruptor;
  }
};

}
