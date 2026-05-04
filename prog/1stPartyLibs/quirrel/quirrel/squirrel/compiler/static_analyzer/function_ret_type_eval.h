#pragma once

#include "compiler/ast.h"
#include "analyzer_internal.h"


namespace SQCompilation
{

class CheckerVisitor;


struct StmtEvalResult
{
  unsigned flags;
  bool allPathsReturn;

  StmtEvalResult(unsigned f, bool r) : flags(f), allPathsReturn(r) {}

  static StmtEvalResult returns(unsigned f) { return StmtEvalResult(f, true); }
  static StmtEvalResult fallsThrough(unsigned f) { return StmtEvalResult(f, false); }
  static StmtEvalResult fallsThrough() { return StmtEvalResult(0, false); }
};


class FunctionReturnTypeEvaluator
{
  unsigned evalLiteral(const LiteralExpr *l);
  unsigned evalAddExpr(const BinExpr *expr);
  unsigned evalExpr(const Expr *expr);
  unsigned evalCall(const CallExpr *expr);
  unsigned evalId(const Id *id);
  unsigned evalGetField(const GetFieldExpr *gf);
  unsigned evalCompoundBin(const BinExpr *expr);

  StmtEvalResult evalStmt(const Statement *node);

  StmtEvalResult evalBlock(const Block *b);
  StmtEvalResult evalIf(const IfStatement *stmt);
  StmtEvalResult evalLoop(const LoopStatement *loop);
  StmtEvalResult evalSwitch(const SwitchStatement *swtch);
  StmtEvalResult evalReturn(const ReturnStatement *ret);
  StmtEvalResult evalTry(const TryStatement *trstmt);
  StmtEvalResult evalThrow(const ThrowStatement *thrw);

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

  FunctionReturnTypeEvaluator(CheckerVisitor *c) : checker(c) {}

  unsigned compute(const Statement *n, bool &allPathsReturn) {
    StmtEvalResult result = evalStmt(n);
    allPathsReturn = result.allPathsReturn;

    unsigned flags = result.flags;
    if (!allPathsReturn) {
      flags |= RT_NOTHING;
    }

    return flags;
  }
};

}
