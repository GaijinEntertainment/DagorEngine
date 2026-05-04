#include "function_ret_type_eval.h"
#include "naming.h"
#include "checker_visitor.h"


namespace SQCompilation
{


StmtEvalResult FunctionReturnTypeEvaluator::evalStmt(const Statement *n) {
  switch (n->op())
  {
    case TO_RETURN: return evalReturn(static_cast<const ReturnStatement *>(n));
    case TO_THROW: return evalThrow(static_cast<const ThrowStatement *>(n));
    case TO_FOR: case TO_FOREACH: case TO_WHILE: case TO_DOWHILE:
      return evalLoop(static_cast<const LoopStatement *>(n));
    case TO_IF: return evalIf(static_cast<const IfStatement *>(n));
    case TO_SWITCH: return evalSwitch(static_cast<const SwitchStatement *>(n));
    case TO_BLOCK: return evalBlock(static_cast<const Block *>(n));
    case TO_TRY: return evalTry(static_cast<const TryStatement *>(n));
    case TO_BREAK:
    case TO_CONTINUE:
      return StmtEvalResult::fallsThrough(); // Control flow, but no return
    default:
      return StmtEvalResult::fallsThrough(); // Expression statements, declarations, etc.
  }
}

unsigned FunctionReturnTypeEvaluator::evalLiteral(const LiteralExpr *lit) {
  switch (lit->kind())
  {
    case LK_STRING: return RT_STRING;
    case LK_NULL: return RT_NULL;
    case LK_BOOL: return RT_BOOL;
    default: return RT_NUMBER;
  }
}

unsigned FunctionReturnTypeEvaluator::evalExpr(const Expr *expr) {
  switch (expr->op())
  {
    case TO_LITERAL:
      return evalLiteral(static_cast<const LiteralExpr *>(expr));
    case TO_OROR:
    case TO_ANDAND:
    case TO_NULLC:
      return evalCompoundBin(static_cast<const BinExpr *>(expr));
    case TO_NE:
    case TO_EQ:
    case TO_GE:
    case TO_GT:
    case TO_LE:
    case TO_LT:
    case TO_INSTANCEOF:
    case TO_IN:
    case TO_NOT:
      return RT_BOOL;
    case TO_ADD:
      return evalAddExpr(static_cast<const BinExpr *>(expr));
    case TO_MOD: {
      const BinExpr *b = static_cast<const BinExpr *>(expr);
      if (canBeString(b->rhs())) // this special pattern 'o % "something"'
        return RT_UNRECOGNIZED;
    } // -V796 fall through
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
      return RT_NUMBER;
    case TO_CALL:
      return evalCall(expr->asCallExpr());
    case TO_TABLE:
      return RT_TABLE;
    case TO_CLASS:
      return RT_CLASS;
    case TO_FUNCTION:
      return RT_CLOSURE;
    case TO_ARRAY:
      return RT_ARRAY;
    case TO_PAREN:
      return evalExpr(static_cast<const UnExpr *>(expr)->argument());
    case TO_TERNARY: {
        const TerExpr *ter = static_cast<const TerExpr *>(expr);
        return evalExpr(ter->b()) | evalExpr(ter->c());
    }
    case TO_ID:
        return evalId(expr->asId());
    case TO_GETFIELD: {
        unsigned flags = evalGetField(expr->asGetField());
        if (expr->asAccessExpr()->isNullable())
          flags |= RT_NULL;
        return flags;
    }
    case TO_GETSLOT:
        if (expr->asAccessExpr()->isNullable())
          return RT_NULL;
        return 0;
    case TO_ASSIGN:
        return RT_NOTHING;
    default:
      if (canBeString(expr))
        return RT_STRING;
      else
        return RT_UNRECOGNIZED;
  }
}

unsigned FunctionReturnTypeEvaluator::evalAddExpr(const BinExpr *expr) {
  assert(expr->op() == TO_ADD);

  unsigned lhsFlags = evalExpr(expr->lhs());
  unsigned rhsFlags = evalExpr(expr->rhs());

  if ((lhsFlags & RT_STRING) || (rhsFlags & RT_STRING)) {
    // concat with string is casted to string
    return RT_STRING;
  }
  else if ((lhsFlags | rhsFlags) & RT_NUMBER) {
    return RT_NUMBER;
  }
  else {
    return lhsFlags | rhsFlags;
  }
}

StmtEvalResult FunctionReturnTypeEvaluator::evalReturn(const ReturnStatement *ret) {
  const Expr *arg = ret->argument();

  if (arg == nullptr) {
    return StmtEvalResult::returns(RT_NOTHING);
  }

  return StmtEvalResult::returns(evalExpr(arg));
}

StmtEvalResult FunctionReturnTypeEvaluator::evalThrow(const ThrowStatement *thrw) {
  return StmtEvalResult::returns(RT_THROW);
}

StmtEvalResult FunctionReturnTypeEvaluator::evalIf(const IfStatement *ifStmt) {
  StmtEvalResult thenResult = evalStmt(ifStmt->thenBranch());

  if (!ifStmt->elseBranch()) {
    return StmtEvalResult::fallsThrough(thenResult.flags);
  }

  StmtEvalResult elseResult = evalStmt(ifStmt->elseBranch());

  unsigned combinedFlags = thenResult.flags | elseResult.flags;
  bool allReturn = thenResult.allPathsReturn && elseResult.allPathsReturn;

  return StmtEvalResult(combinedFlags, allReturn);
}

StmtEvalResult FunctionReturnTypeEvaluator::evalLoop(const LoopStatement *loop) {
  StmtEvalResult bodyResult = evalStmt(loop->body());
  // Loop may not execute at all, so it never guarantees return
  return StmtEvalResult::fallsThrough(bodyResult.flags);
}

StmtEvalResult FunctionReturnTypeEvaluator::evalBlock(const Block *block) {
  unsigned accumulatedFlags = 0;
  bool lastReturns = false;

  for (const Statement *stmt : block->statements()) {
    if (stmt->op() == TO_EMPTY)
      continue;
    StmtEvalResult result = evalStmt(stmt);
    accumulatedFlags |= result.flags;
    lastReturns = result.allPathsReturn;
  }

  return StmtEvalResult(accumulatedFlags, lastReturns);
}

StmtEvalResult FunctionReturnTypeEvaluator::evalSwitch(const SwitchStatement *swtch) {
  unsigned accumulatedFlags = 0;
  bool allCasesReturn = true;

  for (auto &c : swtch->cases()) {
    StmtEvalResult caseResult = evalStmt(c.stmt);
    accumulatedFlags |= caseResult.flags;

    if (!caseResult.allPathsReturn) {
      const Statement *last = unwrapLastStatement(c.stmt);
      if (!last || last->op() != TO_BREAK) // fall through
        continue;
    }
    allCasesReturn &= caseResult.allPathsReturn;
  }

  if (swtch->defaultCase().stmt) {
    StmtEvalResult defaultResult = evalStmt(swtch->defaultCase().stmt);
    accumulatedFlags |= defaultResult.flags;
    allCasesReturn &= defaultResult.allPathsReturn;
  }
  else {
    allCasesReturn = false;
  }

  return StmtEvalResult(accumulatedFlags, allCasesReturn);
}

StmtEvalResult FunctionReturnTypeEvaluator::evalTry(const TryStatement *stmt) {
  StmtEvalResult tryResult = evalStmt(stmt->tryStatement());
  StmtEvalResult catchResult = evalStmt(stmt->catchStatement());

  unsigned combinedFlags = tryResult.flags | catchResult.flags;
  bool allReturn = tryResult.allPathsReturn && catchResult.allPathsReturn;

  return StmtEvalResult(combinedFlags, allReturn);
}

unsigned FunctionReturnTypeEvaluator::evalCompoundBin(const BinExpr *expr) {
  TreeOp op = expr->op();

  assert(op == TO_ANDAND || op == TO_OROR || op == TO_NULLC);

  unsigned lhsFlags = evalExpr(expr->lhs());
  unsigned rhsFlags = evalExpr(expr->rhs());

  lhsFlags &= ~RT_UNRECOGNIZED;
  rhsFlags &= ~RT_UNRECOGNIZED;

  bool hasFuncCall = (lhsFlags | rhsFlags) & RT_FUNCTION_CALL;

  lhsFlags &= ~RT_FUNCTION_CALL;
  rhsFlags &= ~RT_FUNCTION_CALL;

  unsigned resultFlags = 0;

  if (op != TO_NULLC && (((lhsFlags & RT_BOOL) && (rhsFlags & RT_NUMBER)) || ((rhsFlags & RT_BOOL) && (lhsFlags & RT_NUMBER))))
    resultFlags = RT_BOOL;
  else if (rhsFlags & RT_NULL)
    resultFlags = RT_NULL;
  else if ((lhsFlags | rhsFlags) & RT_STRING)
    resultFlags = RT_STRING;
  else
    resultFlags = rhsFlags;

  if (hasFuncCall)
    resultFlags |= RT_FUNCTION_CALL;

  return resultFlags;
}

unsigned FunctionReturnTypeEvaluator::evalId(const Id *id) {
  if (nameLooksLikeResultMustBeBoolean(id->name()))
    return RT_BOOL;
  else
    return RT_UNRECOGNIZED;
}

unsigned FunctionReturnTypeEvaluator::evalGetField(const GetFieldExpr *gf) {
  if (nameLooksLikeResultMustBeBoolean(gf->fieldName()))
    return RT_BOOL;
  else
    return RT_UNRECOGNIZED;
}


unsigned FunctionReturnTypeEvaluator::evalCall(const CallExpr *call) {
  unsigned flags = 0;

  if (canBeString(call)) {
    flags |= RT_STRING;
  }

  const char *fn = checker->extractFunctionName(call);

  if (nameLooksLikeResultMustBeBoolean(fn)) {
    flags |= RT_BOOL;
  }

  if (call->isNullable())
    flags |= RT_NULL;

  flags |= RT_FUNCTION_CALL;

  return flags;
}

bool FunctionReturnTypeEvaluator::canBeString(const Expr *e) {
  return checker->couldBeString(e);
}

}
