#include <assert.h>
#include <squirrel.h>
#include <limits.h>

#include "checker_visitor.h"
#include "node_complexity_counter.h"
#include "node_diff_computer.h"
#include "node_equal_checker.h"
#include "function_ret_type_eval.h"
#include "modification_checker.h"
#include "loop_terminator_collector.h"
#include "ast_helpers.h"
#include "analyzer_internal.h"
#include "operator_classification.h"
#include "assign_seq_terminator.h"
#include "var_scope.h"
#include "value_ref.h"
#include "symbol_info.h"
#include "naming.h"
#include "config.h"
#include "global_state.h"
#include "breakable_scope.h"

#include "sqtable.h"
#include "sqarray.h"
#include "sqclass.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include <sq_char_class.h>



namespace SQCompilation
{

static bool isBinaryArith(const Expr *expr) {
  return TO_OROR <= expr->op() && expr->op() <= TO_SUB;
}

static bool isAssignExpr(const Expr *expr) {
  return isAssignOp(expr->op());
}

static bool looksLikeBooleanExpr(const Expr *e) {
  TreeOp op = e->op(); // -V522
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


void CheckerVisitor::putIntoGlobalNamesMap(std::unordered_map<std::string, std::vector<IdLocation>> &map, enum DiagnosticsId diag, const char *name, const Node *d) {
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

void CheckerVisitor::storeGlobalDeclaration(const char *name, const Node *d) {
  putIntoGlobalNamesMap(declaredGlobals, DiagnosticsId::DI_GLOBAL_NAME_REDEF, name, d);
}

void CheckerVisitor::storeGlobalUsage(const char *name, const Node *d) {
  putIntoGlobalNamesMap(usedGlobals, DiagnosticsId::DI_UNDEFINED_GLOBAL, name, d);
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
  if (isEffectsGatheringPass)
    return;

  va_list vargs;
  va_start(vargs, id);

  _ctx.vreportDiagnostic((enum DiagnosticsId)id, n->lineStart(), n->columnStart(), n->textWidth(), vargs); // -V522

  va_end(vargs);
}

void CheckerVisitor::report(int line, int col, int width, int32_t id, ...) {
  if (isEffectsGatheringPass)
    return;

  va_list vargs;
  va_start(vargs, id);

  _ctx.vreportDiagnostic((enum DiagnosticsId)id, line, col, width, vargs);

  va_end(vargs);
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
}

void CheckerVisitor::checkAccessFromStatic(const GetFieldExpr *acc) {
  if (isEffectsGatheringPass)
    return;

  const Expr *r = acc->receiver();

  if (r->op() != TO_ID || strcmp(r->asId()->name(), "this") != 0)
    return;

  const TableMember *m = nullptr;
  const ClassExpr *klass = nullptr;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst == SST_TABLE_MEMBER) {
      m = it->member;
      ++it;

      if (it != nodeStack.rend() && it->sst == SST_NODE && it->n->op() == TO_CLASS) {
        klass = static_cast<const ClassExpr *>(it->n);
      }

      break;
    }
  }

  if (!m || !m->isStatic() || !klass)
    return;

  const auto &members = klass->members();
  const char *memberName = acc->fieldName();

  for (const auto &m : members) {
    if (m.key->op() == TO_LITERAL && m.key->asLiteral()->kind() == LK_STRING) {
      const char *klassMemberName = m.key->asLiteral()->s();
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
  if (isEffectsGatheringPass)
    return;

  const ExternalValue *ev = findExternalValue(acc->receiver());
  if (!ev)
    return;

  const SQObjectPtr &container = ev->value;
  if (sq_isinstance(container))
    return;

  SQObjectPtr key(_ctx.getVm(), acc->fieldName());
  SQObject rawVal;
  if (!SQ_SUCCEEDED(sq_obj_get(_ctx.getVm(), &container, &key, &rawVal, false))) {
    if (!acc->isNullable() && !hasDynamicContent(container)) {
      report(acc, DI_MISSING_FIELD, acc->fieldName(), GetTypeName(container));
      char buf[128];
      snprintf(buf, sizeof(buf), "source of %s", GetTypeName(container));
      report(ev->line, ev->column, ev->width, DI_SEE_OTHER, buf);
    }
    return;
  }
  sq_poptop(_ctx.getVm()); // pop the stack copy pushed by sq_obj_get

  astValues[acc] = makeExternalValueRef(rawVal, acc);
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
  case TO_ARRAY: case TO_TABLE: case TO_CLASS: case TO_FUNCTION:
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


static bool stringLooksLikeFormatTemplate(const char *s) {
  const char *bracePtr = strchr(s, '{');
  if (bracePtr && (sq_isalpha(bracePtr[1]) || bracePtr[1] == '_'))
  {
    // check for strings specific to Dagor DataBlock objects
    bool isDagorBlk = (strstr(s, ":i=") || strstr(s, ":r=") || strstr(s, ":t=") || strstr(s, ":p2=") || strstr(s, ":p3=") || strstr(s, ":tm="));
    return !isDagorBlk && bracePtr[1] && strchr(bracePtr + 2, '}');
  }

  return false;
}

void CheckerVisitor::checkForgotSubst(const LiteralExpr *l) {
  if (isEffectsGatheringPass)
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

  const char *s = l->s();
  if (!stringLooksLikeFormatTemplate(s)) {
    return;
  }

  bool ok = false;

  if (candidate) {
    const Expr *callee = deparenStatic(candidate->callee());
    const auto &arguments = candidate->arguments();
    if (callee->op() == TO_GETFIELD) { // -V522
      const GetFieldExpr *f = callee->asGetField();
      const char *funcName = f->fieldName();
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
  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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


void CheckerVisitor::checkParamAssignInLambda(const BinExpr *expr) {
  if (isEffectsGatheringPass)
    return;

  if (expr->op() != TO_ASSIGN)
    return;

  const Expr *lhs = expr->lhs();
  if (lhs->op() != TO_ID)
    return;

  if (!currentInfo || !currentInfo->declaration)
    return;

  const FunctionExpr *func = currentInfo->declaration;

  // Only warn in expression lambdas (@(x) expr), not in block-body functions
  if (!func->isLambda())
    return;

  const char *name = lhs->asId()->name();
  ValueRef *v = findValueInScopes(name);

  if (!v || !v->info || v->info->kind != SK_PARAM)
    return;

  // Only warn if the parameter belongs to the current (innermost) function
  if (v->info->ownedScope->owner != func)
    return;

  report(expr, DiagnosticsId::DI_PARAM_ASSIGNMENT_IN_LAMBDA, name);
}

void CheckerVisitor::checkSameOperands(const BinExpr *expr) {

  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
    return;

  const Node *cond = n; // maybeEval(n);

  if (cond->op() == TO_LITERAL) {
    const LiteralExpr *l = cond->asExpression()->asLiteral();
    report(n, DiagnosticsId::DI_ALWAYS_T_OR_F, l->raw() ? "true" : "false");
  }
  else if (cond->op() == TO_ARRAY || cond->op() == TO_TABLE || cond->op() == TO_CLASS ||
           cond->op() == TO_FUNCTION || cond->isDeclaration()) {
    report(n, DiagnosticsId::DI_ALWAYS_T_OR_F, "true");
  }
}

void CheckerVisitor::checkAlwaysTrueOrFalse(const TerExpr *expr) {
  checkAlwaysTrueOrFalse(expr->a());
}

void CheckerVisitor::checkTernaryPriority(const TerExpr *expr) {
  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
    return;

  const Expr *ifTrue = expr->b();
  const Expr *ifFalse = expr->c();

  if (_equalChecker.check(ifTrue, ifFalse)) {
    report(expr, DiagnosticsId::DI_TERNARY_SAME_VALUES);
  }
}

void CheckerVisitor::checkCanBeSimplified(const TerExpr *expr) {
  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
    return;

  TreeOp op = bin->op();
  if (op != TO_ANDAND && op != TO_OROR)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  TreeOp cmpOp = lhs->op();
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

  if (isEffectsGatheringPass)
    return;

  if (isArithOperator(bin->op())) {
    const Expr *lhs = maybeEval(bin->lhs());
    const Expr *rhs = maybeEval(bin->rhs());

    if (lhs->op() == TO_TABLE || lhs->op() == TO_CLASS || lhs->op() == TO_FUNCTION ||
        lhs->op() == TO_ARRAY) {
      report(bin->lhs(), DiagnosticsId::DI_DECL_IN_EXPR);
    }

    if (bin->op() != TO_OROR && bin->op() != TO_ANDAND) {
      if (rhs->op() == TO_TABLE || rhs->op() == TO_CLASS || rhs->op() == TO_FUNCTION ||
          rhs->op() == TO_ARRAY) {
        report(bin->rhs(), DiagnosticsId::DI_DECL_IN_EXPR);
      }
    }
  }
}

void CheckerVisitor::checkIntDivByZero(const BinExpr *bin) {

  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
    return;

  TreeOp op = bin->op();

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

  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
    return;

  const Expr *l = expr->lhs();
  const Expr *r = expr->rhs();

  TreeOp thisOp = expr->op();
  TreeOp lhsOp = l->op();
  TreeOp rhsOp = r->op();

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

  if (isEffectsGatheringPass)
    return;

  TreeOp op = bin->op();
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

  if (isEffectsGatheringPass)
    return;

  if (bin->op() != TO_OROR && bin->op() != TO_ANDAND)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  bool leftIsConst = lhs->op() == TO_LITERAL || lhs->op() == TO_TABLE || lhs->op() == TO_CLASS ||
                     lhs->op() == TO_FUNCTION || lhs->op() == TO_ARRAY || isUpperCaseIdentifier(lhs); //-V522
  bool rightIsConst = rhs->op() == TO_LITERAL || rhs->op() == TO_TABLE || rhs->op() == TO_CLASS ||
                      rhs->op() == TO_FUNCTION || rhs->op() == TO_ARRAY || isUpperCaseIdentifier(rhs); // -V522

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

  if (isEffectsGatheringPass)
    return;

  if (isShiftOperator(bin->op())) {
    if (isHigherShiftPriority(bin->lhs()->op()) || isHigherShiftPriority(bin->rhs()->op())) {
      report(bin, DiagnosticsId::DI_SHIFT_PRIORITY);
    }
  }
}


void CheckerVisitor::checkCompareWithContainer(const BinExpr *bin) {

  if (isEffectsGatheringPass)
    return;

  if (!isCompareOperator(bin->op()))
    return;

  const Expr *l = bin->lhs();
  const Expr *r = bin->rhs();

  if (l->op() == TO_ARRAY || r->op() == TO_ARRAY) {
    report(bin, DiagnosticsId::DI_CMP_WITH_CONTAINER, "array");
  }

  auto isDeclLikeExpr = [](TreeOp op) {
    return op == TO_TABLE || op == TO_CLASS || op == TO_FUNCTION;
  };
  if (isDeclLikeExpr(l->op()) || isDeclLikeExpr(r->op())) {
    report(bin, DiagnosticsId::DI_CMP_WITH_CONTAINER, "declaration");
  }
}

void CheckerVisitor::checkExtendToAppend(const CallExpr *expr) {
  if (isEffectsGatheringPass)
    return;

  const Expr *callee = expr->callee();
  const auto &args = expr->arguments();

  if (callee->op() == TO_GETFIELD) {
    if (args.size() > 0) {
      Expr *arg0 = args[0];
      if (arg0->op() == TO_ARRAY) {
        if (strcmp(callee->asGetField()->fieldName(), "extend") == 0) {
          report(expr, DiagnosticsId::DI_EXTEND_TO_APPEND);
        }
      }
    }
  }
}

void CheckerVisitor::checkMergeEmptyTable(const CallExpr *expr) {
  if (isEffectsGatheringPass)
    return;

  const Expr *callee = expr->callee();
  const auto &args = expr->arguments();

  if (callee->op() == TO_GETFIELD) {
    if (args.size() == 1) {
      Expr *arg0 = args[0];
      if (arg0->op() == TO_TABLE && arg0->asTableExpr()->members().size() == 0) {
        if (strcmp(callee->asGetField()->fieldName(), "__merge") == 0) {
          report(expr, DiagnosticsId::DI_MERGE_EMPTY_TABLE);
        }
      }
    }
  }
}

void CheckerVisitor::checkEmptyArrayResize(const CallExpr *expr) {
  if (isEffectsGatheringPass)
    return;

  const Expr *callee = expr->callee();

  if (callee->op() == TO_GETFIELD) {
    const GetFieldExpr *gf = callee->asGetField();
    if (strcmp(gf->fieldName(), "resize") == 0) {
      const Expr *receiver = gf->receiver();
      if (receiver->op() == TO_ARRAY && static_cast<const ArrayExpr *>(receiver)->initializers().size() == 0) {
        report(expr, DiagnosticsId::DI_EMPTY_ARRAY_RESIZE);
      }
    }
  }
}

void CheckerVisitor::checkBoolToStrangePosition(const BinExpr *bin) {
  if (isEffectsGatheringPass)
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

static const char *tryExtractKeyName(const Expr *e) {

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

  if (isEffectsGatheringPass)
    return;

  const char *fieldName = tryExtractKeyName(key);

  if (!fieldName)
    return;

  if (e->op() != TO_FUNCTION)
    return;

  const FunctionExpr *f = e->asFunctionExpr();
  const char *declName = nullptr;

  if (!f->isLambda() && f->name()[0] != '(') {
    declName = f->name();
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
  if (isEffectsGatheringPass)
    return;

  if (bin->op() != TO_NEWSLOT)
    return;

  const Expr *lhs = bin->lhs();
  const Expr *rhs = bin->rhs();

  checkKeyNameMismatch(lhs, rhs);
}

void CheckerVisitor::checkPlusString(const BinExpr *bin) {
  if (isEffectsGatheringPass)
    return;

  if (bin->op() != TO_ADD && bin->op() != TO_PLUSEQ)
    return;

  const Expr *l = maybeEval(bin->lhs());
  const Expr *r = maybeEval(bin->rhs());

  if (couldBeString(l) || couldBeString(r)) {
    report(bin, DiagnosticsId::DI_PLUS_STRING);
  }
}

void CheckerVisitor::checkNewGlobalSlot(const BinExpr *bin) {

  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
    return;

  if (bin->op() != TO_NULLC)
    return;

  const Expr *rhs = maybeEval(bin->rhs());

  if (rhs->op() == TO_LITERAL && rhs->asLiteral()->kind() == LK_NULL)
    report(bin, DiagnosticsId::DI_USELESS_NULLC);
}

void CheckerVisitor::checkCannotBeNull(const BinExpr *bin) {
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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


void CheckerVisitor::checkRangeCheck(const BinExpr *expr) {
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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

  const char *name = nullptr;

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

  const char *moduleName = l->s();

  resolveRequire(call, moduleName);
  noteRequiredModule(call, moduleName);
}

// Effectful: ask the host for the module's value (by calling require_optional
// from the analyzer) and attach the result as an ExternalValue on the CallExpr.
// Skipped when the user disabled it locally with `// -skip-require`.
void CheckerVisitor::resolveRequire(const CallExpr *call, const char *moduleName) {
  if (_ctx.isRequireDisabled(call->lineStart(), call->columnStart()))
    return;

  auto fv = findValueInScopes("require_optional");
  if (!fv || !fv->externalValue)
    return;

  auto vm = _ctx.getVm();
  SQInteger top = sq_gettop(vm);

  sq_pushobject(vm, fv->externalValue->value);
  sq_pushnull(vm);
  sq_pushstring(vm, moduleName, -1);

  SQObject ret;
  bool ok = SQ_SUCCEEDED(sq_call(vm, 2, true, false))
         && SQ_SUCCEEDED(sq_getstackobj(vm, -1, &ret))
         && !sq_isnull(ret);

  if (ok) {
    astValues[call] = makeExternalValueRef(ret, call);
  }
  else if (do_report_missing_modules) {
    report(call, DI_MISSING_MODULE, moduleName);
    SQObjectPtr empty;
    astValues[call] = makeExternalValueRef(empty, call);
  }

  sq_settop(vm, top);
}

// Structural: report DI_ALREADY_REQUIRED when the same top-level require/
// require_optional appears twice in this file.
void CheckerVisitor::noteRequiredModule(const CallExpr *call, const char *moduleName) {
  if (nodeStack.size() > 2)
    return; // do not consider require which is not on the top level

  if (requiredModules.find(moduleName) != requiredModules.end()) {
    report(call, DiagnosticsId::DI_ALREADY_REQUIRED, moduleName);
  }
  else {
    requiredModules.insert(moduleName);
  }
}

void CheckerVisitor::checkCallNullable(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const Expr *c = call->callee();

  if (!call->isNullable() && isPotentiallyNullable(c)) {
    report(c, DiagnosticsId::DI_ACCESS_POT_NULLABLE, c->op() == TO_ID ? c->asId()->name() : "expression", "function");
  }
}

void CheckerVisitor::checkPersistCall(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *calleeName = extractFunctionName(call);

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

  const char *key = l->s();

  auto r = persistedKeys.emplace(key);

  if (!r.second) {
    report(keyExpr, DiagnosticsId::DI_DUPLICATE_PERSIST_ID, key);
  }
}

void CheckerVisitor::checkForbiddenCall(const CallExpr *call) {

  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (isForbiddenFunctionName(fn)) {
    report(call, DiagnosticsId::DI_FORBIDDEN_FUNC, fn);
  }
}

void CheckerVisitor::checkCallFromRoot(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeMustBeCalledFromRoot(fn)) {
    return;
  }

  bool do_report = false;

  for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
    if (it->sst == SST_TABLE_MEMBER)
      continue;

    TreeOp op = it->n->op();

    if (op == TO_FUNCTION || op == TO_CLASS) {
      do_report = true;
      break;
    }
  }

  if (do_report) {
    report(call, DiagnosticsId::DI_CALL_FROM_ROOT, fn);
  }
}

void CheckerVisitor::checkForbiddenParentDir(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

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

  const char *path = arg->asLiteral()->s();

  const char * p = strstr(path, "..");
  if (p && (p[2] == '/' || p[2] == '\\')) {
    report(call, DiagnosticsId::DI_FORBIDDEN_PARENT_DIR);
  }
}

void CheckerVisitor::checkFormatArguments(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const auto &arguments = call->arguments();

  for (int32_t i = 0; i < arguments.size(); ++i) {
    const Expr *arg = deparenStatic(arguments[i]);
    if (arg->op() == TO_LITERAL && arg->asLiteral()->kind() == LK_STRING) { // -V522
      int32_t formatsCount = 0;
      for (const char *s = arg->asLiteral()->s(); *s; ++s) {
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
        const char *name = extractFunctionName(call);
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

int32_t CheckerVisitor::normalizeParamNameLength(const char *name) {
  int32_t r = 0;

  while (*name) {
    if (*name != '_')
      ++r;
    ++name;
  }

  return r;
}

const char *CheckerVisitor::normalizeParamName(const char *name, char *buffer) {

  if (!buffer) {
    int32_t nl = normalizeParamNameLength(name);
    buffer = (char *)arena->allocate(nl + 1);
  }

  int32_t i = 0, j = 0;
  while (name[i]) {
    char c = name[i++];
    if (c != '_') {
      buffer[j++] = std::tolower(c);
    }
  }

  buffer[j] = '\0';

  return buffer;
}

void CheckerVisitor::checkArguments(const CallExpr *callExpr) {

  if (isEffectsGatheringPass)
    return;

  bool dummy;
  const FunctionInfo *info = findFunctionInfo(callExpr->callee(), dummy);
  const ExternalValue *ev = nullptr;
  const SQFunctionProto *proto = nullptr;
  const SQNativeClosure *nclosure = nullptr;

  const char *funcName;
  int numParams;
  int dpParameters;
  bool isVararg;

  if (info) {
    const FunctionExpr *decl = info->declaration;
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
    ev = findExternalValue(callExpr->callee());
    if (!ev)
      return;

    const SQObject& v = ev->value;
    if (sq_isclosure(v)) {
      proto = _closure(v)->_function;
      funcName = sq_isstring(proto->_name) ? _stringval(proto->_name) : "unknown";
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
      funcName = sq_isstring(nclosure->_name) ? _stringval(nclosure->_name) : "unknown native";
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
    if (ev)
      report(ev->line, ev->column, ev->width, DI_SEE_OTHER, "the function");
    else
      report(maybeEval(callExpr->callee()), DI_SEE_OTHER, "the function");
  }

  for (int i = 0; i < numParams; ++i) {
    const char *paramName;
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
      const char *possibleArgName = nullptr;

      if (arg->op() == TO_ID)
        possibleArgName = arg->asId()->name();
      else if (arg->op() == TO_GETFIELD)
        possibleArgName = arg->asGetField()->fieldName();

      if (!possibleArgName)
        continue;

      int32_t argNL = normalizeParamNameLength(possibleArgName);
      char *buffer = (char *)sq_malloc(_ctx.allocContext(), argNL + 1);
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
  if (isEffectsGatheringPass)
    return;

  const char *name = extractFunctionName(call);

  if (!name)
    return;

  if (!nameLooksLikeModifiesObject(name))
    return;

  const Expr *callee = call->callee();

  if (!callee->isAccessExpr())
    return;

  reportModifyIfContainer(callee->asAccessExpr()->receiver(), call);
}

// Detect ambiguous expressions that may modify either a temporary or a persistent object
static bool hasMixedLifetime(const Expr *e) {
  e = deparenStatic(e);

  const Expr *expr0 = nullptr, *expr1 = nullptr;

  if (e->op() == TO_NULLC) { //-V522
    const BinExpr *nullc = static_cast<const BinExpr *>(e);
    expr0 = deparenStatic(nullc->lhs()); //-V522
    expr1 = deparenStatic(nullc->rhs()); //-V522
  }
  else if (e->op() == TO_TERNARY) { //-V522
    const TerExpr *ter = static_cast<const TerExpr *>(e);
    expr0 = deparenStatic(ter->b()); //-V522
    expr1 = deparenStatic(ter->c()); //-V522
  }
  else
    return false;

  auto isDeclLikeExpr = [](TreeOp op) {
    return op == TO_ARRAY || op == TO_TABLE || op == TO_CLASS || op == TO_FUNCTION;
  };
  bool isLiteral0 = isDeclLikeExpr(expr0->op()); //-V522
  bool isLiteral1 = isDeclLikeExpr(expr1->op()); //-V522

  return (isLiteral0 != isLiteral1);
}

void CheckerVisitor::checkUnwantedModification(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *name = extractFunctionName(call);

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

  if (hasMixedLifetime(callee->asAccessExpr()->receiver()))
    report(call, DiagnosticsId::DI_UNWANTED_MODIFICATION, name);
}

void CheckerVisitor::checkCannotBeNull(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  if (!call->isNullable())
    return;

  const Expr *callee = call->callee();

  reportIfCannotBeNull(maybeEval(callee), callee, "call");
}

void CheckerVisitor::checkBooleanLambda(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeFunctionTakeBooleanLambda(fn))
    return;

  if (call->arguments().size() < 1)
    return;

  const Expr *arg = deparenStatic(call->arguments()[0]);

  if (arg->op() != TO_FUNCTION) // -V522
    return;

  const FunctionExpr *f = arg->asFunctionExpr();

  FunctionReturnTypeEvaluator rte(this);

  bool r = false;
  unsigned typesMask = rte.compute(f->body(), r);

  typesMask &= ~(RT_UNRECOGNIZED | RT_FUNCTION_CALL | RT_BOOL | RT_NUMBER | RT_NULL);

  if (typesMask != 0) {
    report(arg, DiagnosticsId::DI_BOOL_LAMBDA_REQUIRED, fn);
  }
}

void CheckerVisitor::checkCallbackReturnValue(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeRequiresResultFromCallback(fn))
    return;

  if (call->arguments().size() < 1)
    return;

  const Expr *arg = maybeEval(call->arguments()[0]);

  if (arg->op() != TO_FUNCTION) // -V522
    return;

  const FunctionExpr *f = arg->asFunctionExpr();

  FunctionReturnTypeEvaluator rte(this);

  bool allPathsReturn = false;
  unsigned typesMask = rte.compute(f->body(), allPathsReturn);

  if (typesMask & RT_NOTHING) {
    report(arg, DiagnosticsId::DI_CALLBACK_SHOULD_RETURN_VALUE, fn);
  }
}

void CheckerVisitor::checkCallbackShouldNotReturn(const CallExpr *call) {
  if (isEffectsGatheringPass)
    return;

  const char *fn = extractFunctionName(call);

  if (!fn)
    return;

  if (!nameLooksLikeIgnoresCallbackResult(fn))
    return;

  if (call->arguments().size() < 1)
    return;

  const Expr *arg = maybeEval(call->arguments()[0]);

  if (arg->op() != TO_FUNCTION) // -V522
    return;

  const FunctionExpr *f = arg->asFunctionExpr();

  // Lambda with a single call expression like @(x) arr.append(x) is likely
  // a side-effect callback where the return value is incidental.
  if (f->isLambda()) {
    const Block *body = f->body()->asBlock();
    const auto &stmts = body->statements();
    if (stmts.size() == 1 && stmts[0]->op() == TO_RETURN) {
      const ReturnStatement *ret = static_cast<const ReturnStatement *>(stmts[0]);
      if (ret->argument() && ret->argument()->op() == TO_CALL)
        return;
    }
  }

  FunctionReturnTypeEvaluator rte(this);

  bool allPathsReturn = false;
  unsigned typesMask = rte.compute(f->body(), allPathsReturn);

  unsigned valueMask = typesMask & ~(RT_NOTHING | RT_THROW | RT_FUNCTION_CALL | RT_UNRECOGNIZED);
  if (valueMask != 0) {
    report(arg, DiagnosticsId::DI_CALLBACK_SHOULD_NOT_RETURN, fn);
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

const char *CheckerVisitor::extractFunctionName(const CallExpr *call) {
  const Expr *c = maybeEval(call->callee());

  const char *calleeName = nullptr;
  if (c->op() == TO_ID)
    calleeName = c->asId()->name();
  else if (c->op() == TO_FUNCTION) {
    calleeName = c->asFunctionExpr()->name();
  }
  else if (c->op() == TO_GETFIELD)
    calleeName = c->asGetField()->fieldName();

  return calleeName;
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
  checkParamAssignInLambda(expr);
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
  const char *name = computeNameRef(deparenStatic(expr->argument()), nullptr, 0);
  if (name) {
    ValueRef *v = findValueInScopes(name);

    if (v) {
      if (currentInfo) {
        currentInfo->addModifiable(name, v->info->ownedScope->owner);
      }

      v->expression = nullptr;
      v->state = VRS_MULTIPLE;
      v->flagsNegative = v->flagsPositive = 0;
    }
  }

  Visitor::visitIncExpr(expr);
}

void CheckerVisitor::visitCallExpr(CallExpr *expr) {
  checkExtendToAppend(expr);
  checkMergeEmptyTable(expr);
  checkEmptyArrayResize(expr);
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
  checkCallbackReturnValue(expr);
  checkCallbackShouldNotReturn(expr);

  applyCallToScope(expr);

  checkAssertCall(expr);

  Visitor::visitCallExpr(expr);

  checkArguments(expr);
}

void CheckerVisitor::checkBoolIndex(const GetSlotExpr *expr) {

  if (isEffectsGatheringPass)
    return;

  const Expr *key = maybeEval(expr->key())->asExpression();

  if (isBooleanResultOperator(key->op())) {
    report(expr->key(), DiagnosticsId::DI_BOOL_AS_INDEX);
  }
}

void CheckerVisitor::checkNullableIndex(const GetSlotExpr *expr) {

  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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

      const TreeOp op = uw->op();
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

  if (isEffectsGatheringPass)
    return;

  if (_equalChecker.check(ifStmt->thenBranch(), ifStmt->elseBranch())) {
    report(ifStmt->elseBranch(), DiagnosticsId::DI_THEN_ELSE_EQUAL);
  }
}

void CheckerVisitor::checkDuplicateIfConditions(IfStatement *ifStmt) {

  if (isEffectsGatheringPass)
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

  if (isEffectsGatheringPass)
    return;

  const char *varname = nullptr;
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

  if (isEffectsGatheringPass)
    return;

  LoopTerminatorCollector collector(true);
  Statement *body = loop->body();

  body->visit(&collector);

  if (collector.hasUnconditionalTerminator()) {
    const Statement *t = collector.terminator;
    assert(t);
    const char *type = terminatorOpToName(t->op());

    report(t, DiagnosticsId::DI_UNCOND_TERMINATED_LOOP, type);
  }
}

void CheckerVisitor::checkEmptyWhileBody(WhileStatement *loop) {
  if (isEffectsGatheringPass)
    return;

  const Statement *body = unwrapBody(loop->body());

  if (body && body->op() == TO_EMPTY) {
    report(body, DiagnosticsId::DI_EMPTY_BODY, "'while' loop");
  }
}

void CheckerVisitor::checkEmptyThenBody(IfStatement *stmt) {
  if (isEffectsGatheringPass)
    return;

  const Statement *thenStmt = unwrapBody(stmt->thenBranch());

  if (thenStmt && thenStmt->op() == TO_EMPTY) {
    report(thenStmt, DiagnosticsId::DI_EMPTY_BODY, "'then' branch of 'if'");
  }
}

static bool terminateAssignSequence(const Expr *assignee, Node *tree) {
  AssignSeqTerminatorFinder finder(assignee);

  return finder.check(tree);
}

void CheckerVisitor::checkAssignedTwice(const Block *b) {
  if (isEffectsGatheringPass)
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


void CheckerVisitor::checkFunctionPairSimilarity(const FunctionExpr *f1, const FunctionExpr *f2) {
  int32_t diff = NodeDiffComputer::compute(f1->body(), f2->body(), 4);
  if (diff < 4) {
    const char *name1 = f1->name();
    const char *name2 = f2->name();

    if (diff == 0)
      report(f2, DiagnosticsId::DI_DUPLICATE_FUNC, name1, name2);
    else {
      int32_t complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold * 3);
      if (diff <= complexity / functionComplexityThreshold)
        report(f2, DiagnosticsId::DI_SIMILAR_FUNC, name1, name2);
    }
  }
}

void CheckerVisitor::checkFunctionSimilarity(const Block *b) {
  if (isEffectsGatheringPass)
    return;

  const auto &statements = b->statements();

  for (int32_t i = 0; i < int32_t(statements.size()); ++i) {
    const FunctionExpr *f1 = extractFunction(statements[i]);
    if (!f1)
      continue;

    int32_t complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold);

    if (complexity >= functionComplexityThreshold) {
      for (int32_t j = i + 1; j < int32_t(statements.size()); ++j) {
        if (const FunctionExpr *f2 = extractFunction(statements[j])) {
          checkFunctionPairSimilarity(f1, f2);
        }
      }
    }
  }
}

void CheckerVisitor::checkFunctionSimilarity(const TableExpr *table) {
  if (isEffectsGatheringPass)
    return;

  const auto &members = table->members();

  for (int32_t i = 0; i < int32_t(members.size()); ++i) {
    const FunctionExpr *f1 = extractFunction(members[i].value);
    if (!f1)
      continue;

    int32_t complexity = NodeComplexityComputer::compute(f1->body(), functionComplexityThreshold);
    if (complexity >= functionComplexityThreshold) {
      for (int32_t j = i + 1; j < int32_t(members.size()); ++j) {
        if (const FunctionExpr *f2 = extractFunction(members[j].value))
          checkFunctionPairSimilarity(f1, f2);
      }
    }
  }
}

void CheckerVisitor::checkAssignExpressionSimilarity(const Block *b) {
  if (isEffectsGatheringPass)
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

// Determines if a function call's result should be utilized based on its name.
// Returns true if the result SHOULD be utilized (warn if discarded).
// Returns false if it's OK to discard the result (no warning).
//
// Special case: Boolean-named functions (is*, has*, etc.) with a single
// "valuable" argument are treated as potential setters, not getters.
// Example: isEnabled(true) might SET enabled state, not query it.
// In this case we suppress the warning.
bool CheckerVisitor::shouldCallResultBeUtilized(const char *name, const CallExpr *call) {
  if (!name)
    return false;

  // Functions like __merge, findvalue, filter - always should utilize result
  if (nameLooksLikeResultMustBeUtilized(name))
    return true;

  // Boolean-named functions (is*, has*, get*, etc.)
  if (nameLooksLikeResultMustBeBoolean(name)) {
    const auto &args = call->arguments();

    // Multiple or zero arguments - likely a getter, should utilize result
    if (args.size() != 1)
      return true;

    const Expr *arg = args[0];

    // Single boolean expression argument (true, false, x > 0, etc.)
    // Looks like: isEnabled(true) - setting state to true
    if (looksLikeBooleanExpr(arg))
      return false;

    // Single identifier with boolean-like name (isValid, hasItems, etc.)
    // Looks like: isEnabled(isCurrentlyValid) - setting state from another boolean
    if (arg->op() == TO_ID) {
      if (nameLooksLikeResultMustBeBoolean(arg->asId()->name()))
        return false;
    }

    // Single call to a function whose result should be utilized
    // Looks like: isEnabled(getValue()) - passing a meaningful value
    // If the inner call returns something "valuable", we're using it as input
    // to potentially set state, so suppress warning for the outer call
    if (arg->op() == TO_CALL) {
      const CallExpr *innerCall = arg->asCallExpr();
      const Expr *callee = innerCall->callee();
      const char *calleeName = nullptr;
      if (callee->op() == TO_ID)
        calleeName = callee->asId()->name();
      else if (callee->op() == TO_GETFIELD)
        calleeName = callee->asGetField()->fieldName();

      // Recursive check: if inner call's result should be utilized,
      // then we're passing a "valuable" result to outer function,
      // which suggests outer function might be a setter
      if (shouldCallResultBeUtilized(calleeName, innerCall))
        return false;
    }

    // Try to evaluate the argument - it might resolve to a boolean
    // Example: let x = true; isEnabled(x) - x evaluates to true
    const Expr *evaled = maybeEval(arg);
    if (looksLikeBooleanExpr(evaled))
      return false;

    // Single non-boolean argument - likely a getter, should utilize result
    return true;
  }

  return false;
}

void CheckerVisitor::checkUnutilizedResult(const ExprStatement *s) {
  if (isEffectsGatheringPass)
    return;

  const Expr *e = s->expression();

  if (e->op() == TO_CALL) {
    const CallExpr *c = static_cast<const CallExpr *>(e);
    const Expr *callee = c->callee();

    const char *calleeName = nullptr;
    if (callee->op() == TO_ID) {
      calleeName = callee->asId()->name();
    }
    else if (callee->op() == TO_GETFIELD) {
      calleeName = callee->asGetField()->fieldName();
    }

    if (calleeName && shouldCallResultBeUtilized(calleeName, c)) {
      report(e, DiagnosticsId::DI_NAME_LIKE_SHOULD_RETURN, calleeName);
    }
  }
  else if (!isAssignExpr(e) && e->op() != TO_INC && e->op() != TO_NEWSLOT && e->op() != TO_DELETE) {
    report(s, DiagnosticsId::DI_UNUTILIZED_EXPRESSION);
  }
}

void CheckerVisitor::checkForgottenDo(const Block *b) {
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
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

  bool wasGatheringEffects = isEffectsGatheringPass;
  isEffectsGatheringPass = true;

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

  isEffectsGatheringPass = wasGatheringEffects;

  if (!isEffectsGatheringPass) {
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
  if (isEffectsGatheringPass)
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

  bool wasGatheringEffects = isEffectsGatheringPass;
  isEffectsGatheringPass = true;

  {
    VarScope *dummyScope = loopScope.copy(arena, false);
    BreakableScope bs(this, loop, dummyScope, nullptr); // null because we don't (??) interest in exit effect here
    currentScope = dummyScope;

    loop->body()->visit(this);

    loopScope.merge(dummyScope);

    dummyScope->parent = nullptr;
    dummyScope->~VarScope();
  }
  isEffectsGatheringPass = wasGatheringEffects;

  if (!isEffectsGatheringPass) {
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

  bool wasGatheringEffects = isEffectsGatheringPass;
  isEffectsGatheringPass = true;

  {
    BreakableScope bs(this, loop, loopScope, nullptr); // null because we don't (??) interest in exit effect here
    loop->body()->visit(this);
  }
  isEffectsGatheringPass = wasGatheringEffects;

  if (!isEffectsGatheringPass) {
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

  bool wasGatheringEffects = isEffectsGatheringPass;
  isEffectsGatheringPass = true;

  {
    BreakableScope bs(this, loop, loopScope, nullptr); // null because we don't (??) interest in exit effect here
    loop->body()->visit(this);
    loop->condition()->visit(this);
  }
  isEffectsGatheringPass = wasGatheringEffects;

  if (!isEffectsGatheringPass) {
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


bool CheckerVisitor::detectTypeOfPattern(const Expr *expr, const Expr *&res_checkee, const LiteralExpr *&res_lit) {
  // Either of 'typeof checkee ==/!= "lit"' or 'type(checkee) ==/!= "lit"
  expr = deparenStatic(expr);

  TreeOp op = expr->op(); // -V522

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

bool CheckerVisitor::detectNullCPattern(TreeOp op, const Expr *cond, const Expr *&checkee) {

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

  TreeOp op = cond->op();

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

    bool nullCheck = strcmp(typeLit->s(), "null") == 0; // todo: should it be more precise in case of avaliable names
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

      lhsEScope->~VarScope(); // -V522
      rhsEScope->~VarScope(); // -V522
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

const char *CheckerVisitor::findSlotNameInStack(const Node *decl) {
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
        if (rhs == decl) {
          if (lhs->op() == TO_LITERAL) {
            if (lhs->asLiteral()->kind() == LK_STRING) {
              return lhs->asLiteral()->s();
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
      if (rhs == decl) {
        if (lhs->op() == TO_LITERAL) {
            if (lhs->asLiteral()->kind() == LK_STRING) {
              return lhs->asLiteral()->s();
            }
          }
        return nullptr;
      }
    }
    ++it;
  }

  return nullptr;
}

void CheckerVisitor::checkFunctionReturns(FunctionExpr *func) {
  if (isEffectsGatheringPass)
    return;

  const char *name = func->name();

  if (!name || name[0] == '(') {
    name = findSlotNameInStack(func);
  }

  bool dummy;
  unsigned returnFlags = FunctionReturnTypeEvaluator(this).compute(func->body(), dummy);

  bool reported = false;

  if (returnFlags & ~(RT_BOOL | RT_UNRECOGNIZED | RT_FUNCTION_CALL | RT_NULL)) { // Null is castable to boolean
    if (nameLooksLikeResultMustBeBoolean(name)) {
      Node *reportNode = func->nameId();
      if (!reportNode) // just for safety
        reportNode = func;
      report(reportNode, DiagnosticsId::DI_NAME_RET_BOOL, name);
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
  if (isEffectsGatheringPass)
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
  if (isEffectsGatheringPass)
    return;

  const Expr *r = acc->receiver();

  if (!isSafeAccess(acc) && isPotentiallyNullable(r)) {
    report(r, DiagnosticsId::DI_ACCESS_POT_NULLABLE, "expression", "container");
  }
}

void CheckerVisitor::checkEnumConstUsage(const GetFieldExpr *acc) {
  if (isEffectsGatheringPass)
    return;

  char buffer[64] = { 0 };
  const char *receiverName = computeNameRef(acc->receiver(), buffer, sizeof buffer);

  if (!receiverName)
    return;

  const ValueRef *enumV = findValueInScopes(receiverName);
  if (!enumV || enumV->info->kind != SK_ENUM)
    return;

  const char *fqn = enumFqn(arena, receiverName, acc->fieldName());
  const ValueRef *constV = findValueInScopes(fqn);

  if (!constV) {
    // very suspicious
    return;
  }

  constV->info->used = true;
}

void CheckerVisitor::visitTableExpr(TableExpr *table) {
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

void CheckerVisitor::visitClassExpr(ClassExpr *klass) {
  nodeStack.push_back({ SST_NODE, klass });

  if (klass->classKey())
    klass->classKey()->visit(this);

  if (klass->classBase())
    klass->classBase()->visit(this);

  visitTableExpr(klass);

  nodeStack.pop_back();
}

void CheckerVisitor::visitFunctionExpr(FunctionExpr *func) {
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

  Visitor::visitFunctionExpr(func);

  if (oldInfo) {
    oldInfo->joinModifiable(newInfo);
  }

  functionScope.checkUnusedSymbols(this);

  currentInfo = oldInfo;
  currentScope = parentScope;
}

ValueRef *CheckerVisitor::findValueInScopes(const char *ref) {
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

  const char *name = lhs->asId()->name();
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
  v->externalValue = findExternalValue(rhs);
  v->flagsNegative = v->flagsPositive = 0;
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
  const char *name = computeNameRef(lhs, nullptr, 0);
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

static const char double_colon_str[] = "::";
static const char base_str[] = "base";

int32_t CheckerVisitor::computeNameLength(const Expr *e) {
  switch (e->op()) // -V522
  {
  case TO_GETFIELD: return computeNameLength(e->asGetField());
  //case TO_GETSLOT: return computeNameLength(e->asGetSlot());
  case TO_ID: return strlen(e->asId()->name());
  case TO_ROOT_TABLE_ACCESS: return strlen(double_colon_str);
  case TO_BASE: return strlen(base_str);
  default:
    return -1;
  }
}

void CheckerVisitor::computeNameRef(const Expr *e, char *b, int32_t &ptr, int32_t size) {
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
    ptr += strlen(double_colon_str);
    break;
  case TO_BASE:
    snprintf(&b[ptr], size - ptr, "%s", base_str);
    ptr += strlen(base_str);
    break;
  default:
    assert(0);
  }
}

const char *CheckerVisitor::computeNameRef(const Expr *lhs, char *buffer, size_t bufferSize) {
  int32_t length = computeNameLength(lhs);
  if (length < 0)
    return nullptr;

  char *result = buffer;

  if (!result || bufferSize < (length + 1)) {
    result = (char *)arena->allocate(length + 1);
  }

  int32_t ptr = 0;
  computeNameRef(lhs, result, ptr, length + 1);
  assert(ptr == length);
  return result;
}

void CheckerVisitor::setValueFlags(const Expr *lvalue, unsigned pf, unsigned nf) {

  char buffer[128] = { 0 };

  const char *name = computeNameRef(lvalue, buffer, sizeof buffer);

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

  char buffer[128] = { 0 };

  const char *n = computeNameRef(e, buffer, sizeof buffer);

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

    // Check aliased source variable's current flags if it wasn't reassigned.
    // This handles: let y = x; if (!x) return; y.field
    // (y is non-null via x)
    if (v->origin && v->origin->evalIndex == v->originEvalIndex) {
      if (v->origin->flagsNegative & RT_NULL)
        return false;
    }
  }

  e = maybeEval(e);
  if (!e) return false;

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

    const char *funcName = nullptr;
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
    const char *name = nullptr;
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

const Expr *CheckerVisitor::maybeEval(const Expr *e) {
  int32_t dummy = -1;
  return maybeEval(e, dummy);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId) {
  std::unordered_set<const Expr *> visited;
  return maybeEval(e, evalId, visited);
}

const Expr *CheckerVisitor::maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited) {
  if (!e)
    return e;

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
    // Stop at host-bound symbols: their value lives outside the AST in an
    // ExternalValue. Callers that need the value use findExternalValue directly.
    // Also stop when there is no AST expression to chase further.
    if (v->externalValue || !v->expression)
      return e;
    // If this variable was initialized from another variable (aliased) and that source
    // variable has been reassigned since, stop evaluating the expression chain.
    // Example: let a = null; let b = a; a = 123; return b ?? 2
    if (v->origin && v->expression->op() == TO_ID) {
      if (v->origin->evalIndex != v->originEvalIndex) {
        return e;
      }
    }
    return maybeEval(v->expression, evalId, visited);
  }
  else {
    return e;
  }
}

const char *CheckerVisitor::findFieldName(const Expr *e) {
  if (e->op() == TO_ID)
    return e->asId()->name();

  if (e->op() == TO_GETFIELD)
    return e->asGetField()->fieldName();

  if (e->op() == TO_CALL)
    return findFieldName(e->asCallExpr()->callee());

  const ValueRef *v = findValueForExpr(e);

  if (v && v->expression && v->expression->op() == TO_FUNCTION) {
    return v->expression->asFunctionExpr()->name();
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

void CheckerVisitor::computeNameRef(const GetFieldExpr *access, char *b, int32_t &ptr, int32_t size) {
  computeNameRef(access->receiver(), b, ptr, size);
  b[ptr++] = '.';
  int32_t l = snprintf(&b[ptr], size - ptr, "%s", access->fieldName());
  ptr += l;
}

const ExternalValue *CheckerVisitor::findExternalValue(const Expr *e) {
  e = deparenStatic(e);
  if (!e) return nullptr;

  if (const ValueRef *v = findValueForExpr(e); v && v->externalValue)
    return v->externalValue;

  return nullptr;
}


const FunctionInfo *CheckerVisitor::findFunctionInfo(const Expr *e, bool &isCtor) {
  const Expr *ee = maybeEval(e);

  if (ee->op() == TO_FUNCTION) {
    return functionInfoMap[ee->asFunctionExpr()];
  }

  char buffer[128] = { 0 };

  const char *name = computeNameRef(ee, buffer, sizeof buffer);

  if (!name)
    return nullptr;

  const ValueRef *v = findValueInScopes(name);

  if (!v || !v->hasValue() || !v->expression)
    return nullptr;

  const Expr *expr = maybeEval(v->expression);

  if (expr->op() == TO_FUNCTION) {
    return functionInfoMap[expr->asFunctionExpr()];
  }
  else if (expr->op() == TO_CLASS) {
    const ClassExpr *klass = expr->asClassExpr();
    isCtor = true;
    if (FunctionExpr *ctor = klass->findConstructor())
      return functionInfoMap[ctor];
  }

  return nullptr;
}

void CheckerVisitor::applyKnownInvocationToScope(const ValueRef *value) {
  const FunctionInfo *info = nullptr;

  if (value->externalValue) {
    applyUnknownInvocationToScope();
    return;
  }

  if (value->hasValue()) {
    const Expr *expr = maybeEval(value->expression);
    assert(expr != nullptr);

    if (expr->op() == TO_FUNCTION) {
      info = functionInfoMap[expr->asFunctionExpr()];
    }
    else if (expr->op() == TO_CLASS) {
      const ClassExpr *klass = expr->asClassExpr();
      if (FunctionExpr *ctor = klass->findConstructor())
        info = functionInfoMap[ctor];
    }
    else if (expr->op() == TO_TABLE) {
      // Tables don't have function info
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
    char buffer[128] = { 0 };
    const char *ref = computeNameRef(callee, buffer, sizeof buffer);
    const ValueRef *value = findValueInScopes(ref);
    if (value) {
      applyKnownInvocationToScope(value);
    }
    else if (!ref || strcmp(ref, double_colon_str) != 0) {
      // we don't know what exactly is being called so assume the most conservative case
      applyUnknownInvocationToScope();
    }
  }
  else {
    // unknown invocation so everything could be modified
    applyUnknownInvocationToScope();
  }
}

void CheckerVisitor::pushFunctionScope(VarScope *functionScope, const FunctionExpr *decl) {

  FunctionInfo *info = functionInfoMap[decl];

  if (!info) {
    info = makeFunctionInfo(decl, currentScope->owner);
    functionInfoMap[decl] = info;
  }

  currentScope = functionScope;
}

void CheckerVisitor::declareSymbol(const char *nameRef, ValueRef *v) {
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
  if (!isEffectsGatheringPass)
    currentInfo->parameters.push_back(normalizeParamName(p->name()));
}

void CheckerVisitor::visitVarDecl(VarDecl *decl) {
  Visitor::visitVarDecl(decl);

  SymbolInfo *info = makeSymbolInfo(decl->isAssignable() ? SK_VAR : SK_BINDING);
  ValueRef *v = makeValueRef(info);
  const Expr *init = decl->initializer();
  const Expr *origInit = deparenStatic(init);

  // Carry the external value across an alias declaration: `let mod = require(...)`,
  // `let foo = mod.foo`, `let f = imported_fn`. Resolved against the original
  // initializer (not maybeEval'd), so we hit astValues entries left by the
  // child visitors above and direct name lookups for imported symbols.
  if (origInit)
    v->externalValue = findExternalValue(origInit);

  init = maybeEval(init);
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
  }

  // Track aliasing: if initialized from a variable, store reference for dynamic flag lookup.
  // This allows checking the source's current flags even if they change after this declaration.
  if (origInit && origInit->op() == TO_ID) {
    const ValueRef *vr = findValueForExpr(origInit);
    if (vr) {
      v->flagsPositive = vr->flagsPositive;
      v->flagsNegative = vr->flagsNegative;
      v->origin = vr;
      v->originEvalIndex = vr->evalIndex;
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

    const char *fqn = enumFqn(arena, enm->name(), c.id);
    declareSymbol(fqn, cv);

    c.val->visit(this);
  }
}

void CheckerVisitor::checkDestructuredVarDefault(VarDecl *var) {
  const Expr *def = var->initializer();
  def = maybeEval(def);
  if (def) {
    auto vv = astValues[var];
    assert(vv);
    vv->state = VRS_INITIALIZED;
    vv->expression = def;
    // No need to set vv->externalValue here -- visitVarDecl already populated
    // it from decl->initializer() (the default expression) via
    // findExternalValue(origInit) before this destructuring pass ran.
  }
  else {
    report(var, DiagnosticsId::DI_MISSING_DESTRUCTURED_VALUE, var->name());
  }
}

void CheckerVisitor::visitDestructuringDecl(DestructuringDecl *d) {

  checkAccessNullable(d);

  Visitor::visitDestructuringDecl(d);

  if (auto v = findValueForExpr(d->initExpression()); v && v->hasValue()) {
    if (v->externalValue) {
      const SQObjectPtr &init = v->externalValue->value;
      if (sq_isnull(init)) {
        for (auto var : d->declarations()) {
          checkDestructuredVarDefault(var);
        }
      }
      else {
        auto applyDestructure = [&](DestructuringType expected, const char *typeName,
                                    auto &&lookup) {
          if (d->type() != expected) {
            report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, typeName);
            return;
          }
          SQInteger index = 0;
          for (auto var : d->declarations()) {
            SQObjectPtr val;
            if (lookup(var, index, val)) {
              auto vv = astValues[var];
              assert(vv);
              attachExternalValue(vv, val, var);
            }
            else {
              checkDestructuredVarDefault(var);
            }
            index++;
          }
        };

        if (sq_istable(init)) {
          applyDestructure(DT_TABLE, "table", [&](VarDecl *var, SQInteger, SQObjectPtr &val) {
            return _table(init)->GetStr(var->name(), strlen(var->name()), val);
          });
        }
        else if (sq_isclass(init)) {
          applyDestructure(DT_TABLE, "class", [&](VarDecl *var, SQInteger, SQObjectPtr &val) {
            SQObjectPtr key(_ctx.getVm(), var->name());
            return _class(init)->Get(key, val);
          });
        }
        else if (sq_isinstance(init)) {
          applyDestructure(DT_TABLE, "instance", [&](VarDecl *var, SQInteger, SQObjectPtr &val) {
            SQObjectPtr key(_ctx.getVm(), var->name());
            return _instance(init)->Get(key, val);
          });
        }
        else if (sq_isarray(init)) {
          applyDestructure(DT_ARRAY, "array", [&](VarDecl *, SQInteger idx, SQObjectPtr &val) {
            return _array(init)->Get(idx, val);
          });
        }
        else {
          report(d, DiagnosticsId::DI_DESTRUCTURED_TYPE_MISMATCH, GetTypeName(init));
        }
      }
    }
    else if (v->expression && v->expression->op() == TO_TABLE) {
      // TODO: check table destructuring
    }
    else if (v->expression && v->expression->op() == TO_ARRAY) {
      // TODO: check array destructuring
    }
  }
}

void CheckerVisitor::visitImportStatement(ImportStmt *import) {
  // For each name brought into scope by the import, build an SK_IMPORT symbol
  // pointing at the ExternalValue already collected by analyze() under the same
  // name (the host populates `bindings` before analyze() runs). The role
  // (import slot, with its own source location) and the external value (from
  // the binding) live in independent channels on ValueRef.
  auto declareSlot = [&](int line, int col, const char *name) {
    ValueRef *existing = findValueInScopes(name);
    assert(existing && existing->externalValue);

    ImportInfo *importInfo = (ImportInfo *)arena->allocate(sizeof(ImportInfo));
    importInfo->line = line;
    importInfo->column = col;
    importInfo->name = name;

    SymbolInfo *info = makeSymbolInfo(SK_IMPORT);
    info->declarator.imp = importInfo;
    info->ownedScope = currentScope;

    ValueRef *v = makeValueRef(info);
    // Same value, but coords pointing at the import statement itself rather
    // than at the host-bindings entry's synthetic location -- so DI_SEE_OTHER
    // hints land on the import line. Going through attachExternalValue also
    // preserves RT_NULL when the host binding's value happens to be null.
    attachExternalValue(v, existing->externalValue->value, line, col, (int)strlen(name)); //-V522

    declareSymbol(name, v);
  };

  if (import->slots.empty()) {
    declareSlot(import->lineStart(),
                import->moduleAlias ? import->aliasCol : import->nameCol,
                import->moduleAlias ? import->moduleAlias : import->moduleName);
  }
  else {
    for (const SQModuleImportSlot &slot : import->slots) {
      if (strcmp(slot.name, "*") == 0)
        continue;
      declareSlot(slot.line, slot.column, slot.alias ? slot.alias : slot.name);
    }
  }

  Visitor::visitImportStatement(import);
}

void CheckerVisitor::attachExternalValue(ValueRef *v, const SQObject &val, int line, int col, int width) {
  v->state = VRS_INITIALIZED;
  v->externalValue = externalValues.make(val, line, col, width);
  if (sq_isnull(val))
    v->flagsPositive |= RT_NULL;
}

void CheckerVisitor::attachExternalValue(ValueRef *v, const SQObject &val, const Node *location) {
  assert(location);
  attachExternalValue(v, val, location->lineStart(), location->columnStart(), location->textWidth());
}

ValueRef *CheckerVisitor::makeExternalValueRef(const SQObject &val, const Node *location) {
  // SK_SYNTHETIC: this ValueRef is not a declaration, just an external-value
  // carrier for astValues. Synthetic refs never enter scope walks.
  SymbolInfo *info = makeSymbolInfo(SK_SYNTHETIC);
  ValueRef *v = makeValueRef(info);
  attachExternalValue(v, val, location);
  return v;
}

ValueRef *CheckerVisitor::declareHostBinding(const char *name, const SQObject &val, const Node *location) {
  // Host bindings carry no ImportInfo: source coords live on the ExternalValue,
  // name is the symbol-table key. checkUnusedSymbols branches on declarator.imp
  // to distinguish host bindings from real `from X import Y` slots.
  SymbolInfo *info = makeSymbolInfo(SK_IMPORT);
  info->declarator.imp = nullptr;

  ValueRef *v = makeValueRef(info);
  attachExternalValue(v, val, location);
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
        const char *name = _string(key)->_val;
        declareSymbol(name, declareHostBinding(name, val, root));
      }
      pos._unVal.nInteger = idx;
    }
  }

  root->visit(this);

  rootScope.parent = nullptr;
  currentScope = nullptr;
}


}
