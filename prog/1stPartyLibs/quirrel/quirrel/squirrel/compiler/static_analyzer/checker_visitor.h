#pragma once

#include "compiler/compilationcontext.h"
#include "node_equal_checker.h"
#include "modification_checker.h"
#include "ast_helpers.h"
#include "function_info.h"
#include "value_ref.h"
#include "external_value.h"

#include <unordered_set>


namespace SQCompilation
{

struct VarScope;
struct BreakableScope;


struct IdLocation {
  const char *filename;
  int32_t line, column;
  bool diagSilenced;
};


class PredicateCheckerVisitor : public Visitor
{
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


class CheckModificationVisitor : public PredicateCheckerVisitor
{
protected:
  bool doCheck(const Node *checkee, Node *n) const {
    TreeOp op = n->op();

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


class ExistsChecker : public PredicateCheckerVisitor
{
protected:
  bool doCheck(const Node *checkee, Node *n) const {
    return equalChecker.check(checkee, n);
  }

public:
  ExistsChecker() : PredicateCheckerVisitor(true) {}
};


class CheckerVisitor : public Visitor
{
  friend struct VarScope;
  friend struct BreakableScope;
  friend class FunctionReturnTypeEvaluator;

  const int32_t functionComplexityThreshold = 45; // get threshold complexity from command line args
  const int32_t statementComplexityThreshold = 23;
  const int32_t statementSimilarityThreshold = 33;

  SQCompilationContext &_ctx;

  NodeEqualChecker _equalChecker;

  void report(const Node *n, int32_t id, ...);
  void report(int line, int col, int width, int32_t id, ...);

  void checkKeyNameMismatch(const Expr *key, const Expr *expr);

  void checkAlwaysTrueOrFalse(const Expr *expr);

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
  void checkCompareWithContainer(const BinExpr *expr);
  void checkBoolToStrangePosition(const BinExpr *expr);
  void checkNewSlotNameMatch(const BinExpr *expr);
  void checkPlusString(const BinExpr *expr);
  void checkNewGlobalSlot(const BinExpr *);
  void checkUselessNullC(const BinExpr *);
  void checkCannotBeNull(const BinExpr *);
  void checkCanBeSimplified(const BinExpr *expr);
  void checkRangeCheck(const BinExpr *expr);
  void checkParamAssignInLambda(const BinExpr *expr);
  void checkAlwaysTrueOrFalse(const TerExpr *expr);
  void checkTernaryPriority(const TerExpr *expr);
  void checkSameValues(const TerExpr *expr);
  void checkCanBeSimplified(const TerExpr *expr);
  void checkExtendToAppend(const CallExpr *callExpr);
  void checkMergeEmptyTable(const CallExpr *callExpr);
  void checkEmptyArrayResize(const CallExpr *callExpr);
  void checkAlreadyRequired(const CallExpr *callExpr);
  void resolveRequire(const CallExpr *call, const char *moduleName);
  void noteRequiredModule(const CallExpr *call, const char *moduleName);
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
  void checkCallbackReturnValue(const CallExpr *expr);
  void checkCallbackShouldNotReturn(const CallExpr *expr);
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

  const char *normalizeParamName(const char *name, char *buffer = nullptr);
  int32_t normalizeParamNameLength(const char *n);

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

  bool shouldCallResultBeUtilized(const char *name, const CallExpr *call);

  void checkUnterminatedLoop(LoopStatement *loop);
  void checkVariableMismatchForLoop(ForStatement *loop);
  void checkEmptyWhileBody(WhileStatement *loop);
  void checkEmptyThenBody(IfStatement *stmt);
  void checkForgottenDo(const Block *block);
  void checkUnreachableCode(const Block *block);
  void checkAssignedTwice(const Block *b);

  void checkFunctionSimilarity(const Block *b);
  void checkFunctionSimilarity(const TableExpr *table);
  void checkFunctionPairSimilarity(const FunctionExpr *f1, const FunctionExpr *f2);

  void checkAssignExpressionSimilarity(const Block *b);
  void checkUnutilizedResult(const ExprStatement *b);
  void checkNullableContainer(const ForeachStatement *loop);
  void checkMissedBreak(const SwitchStatement *swtch);

  const char *findSlotNameInStack(const Node *);
  void checkFunctionReturns(FunctionExpr *func);

  void checkAccessNullable(const DestructuringDecl *d);
  void checkAccessNullable(const AccessExpr *acc);
  void checkEnumConstUsage(const GetFieldExpr *acc);

  enum StackSlotType {
    SST_NODE,
    SST_TABLE_MEMBER
  };

  struct StackSlot {
    StackSlotType sst;
    union {
      const Node *n;
      const TableMember *member;
    };
  };

  std::vector<StackSlot> nodeStack;

  VarScope *currentScope;
  BreakableScope *breakScope;

  FunctionInfo *makeFunctionInfo(const FunctionExpr *d, const FunctionExpr *o) {
    void *mem = arena->allocate(sizeof(FunctionInfo));
    return new(mem) FunctionInfo(d, o);
  }

  ValueRef *makeValueRef(SymbolInfo *info) {
    void *mem = arena->allocate(sizeof(ValueRef));
    return new (mem) ValueRef(info, currentScope->evalId);
  }

  SymbolInfo *makeSymbolInfo(SymbolKind kind) {
    void *mem = arena->allocate(sizeof(SymbolInfo));
    return new (mem) SymbolInfo(kind);
  }

  std::unordered_map<const FunctionExpr *, FunctionInfo *> functionInfoMap;

  std::unordered_set<const char *, StringHasher, StringEqualer> requiredModules;
  std::unordered_set<const char *, StringHasher, StringEqualer> persistedKeys;

  std::unordered_map<const Node *, ValueRef *> astValues;

  Arena *arena;

  ExternalValueTable externalValues; // Declared after arena and ctx for correct life time

  FunctionInfo *currentInfo;

  void declareSymbol(const char *name, ValueRef *v);
  void pushFunctionScope(VarScope *functionScope, const FunctionExpr *decl);

  void applyCallToScope(const CallExpr *call);
  void applyBinaryToScope(const BinExpr *bin);
  void applyAssignmentToScope(const BinExpr *bin);
  void applyAssignEqToScope(const BinExpr *bin);

  int32_t computeNameLength(const GetFieldExpr *access);
  void computeNameRef(const GetFieldExpr *access, char *b, int32_t &ptr, int32_t size);
  int32_t computeNameLength(const Expr *e);
  void computeNameRef(const Expr *lhs, char *buffer, int32_t &ptr, int32_t size);
  const char *computeNameRef(const Expr *lhs, char *buffer, size_t bufferSize);

  ValueRef *findValueInScopes(const char *ref);
  void applyKnownInvocationToScope(const ValueRef *ref);
  void applyUnknownInvocationToScope();

  const ExternalValue *findExternalValue(const Expr *e);
  const FunctionInfo *findFunctionInfo(const Expr *e, bool &isCtor);

  void setValueFlags(const Expr *lvalue, unsigned pf, unsigned nf);
  const ValueRef *findValueForExpr(const Expr *e);
  const Expr *maybeEval(const Expr *e, int32_t &evalId, std::unordered_set<const Expr *> &visited);
  const Expr *maybeEval(const Expr *e, int32_t &evalId);
  const Expr *maybeEval(const Expr *e);

  const char *findFieldName(const Expr *e);

  bool isSafeAccess(const AccessExpr *acc);
  bool isPotentiallyNullable(const Expr *e);
  bool isPotentiallyNullable(const Expr *e, std::unordered_set<const Expr *> &visited);
  bool couldBeString(const Expr *e);

  void visitBinaryBranches(Expr *lhs, Expr *rhs, bool inv);
  void speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, bool inv = false);
  void speculateIfConditionHeuristics(const Expr *cond, VarScope *thenScope, VarScope *elseScope, std::unordered_set<const Expr *> &visited, int32_t evalId, unsigned flags, bool inv);
  bool detectTypeOfPattern(const Expr *expr, const Expr *&r_checkee, const LiteralExpr *&r_lit);
  bool detectNullCPattern(TreeOp op, const Expr *expr, const Expr *&checkee);

  void checkAssertCall(const CallExpr *call);
  const char *extractFunctionName(const CallExpr *call);

  LiteralExpr trueValue, falseValue, nullValue;

  bool isEffectsGatheringPass;

  void putIntoGlobalNamesMap(std::unordered_map<std::string, std::vector<IdLocation>> &map, enum DiagnosticsId diag, const char *name, const Node *d);
  void storeGlobalDeclaration(const char *name, const Node *d);
  void storeGlobalUsage(const char *name, const Node *d);

public:

  CheckerVisitor(SQCompilationContext &ctx)
    : _ctx(ctx)
    , arena(ctx.arena())
    , externalValues(ctx.arena())
    , currentInfo(nullptr)
    , currentScope(nullptr)
    , breakScope(nullptr)
    , trueValue(SourceSpan::invalid(), true)
    , falseValue(SourceSpan::invalid(), false)
    , nullValue(SourceSpan::invalid())
    , isEffectsGatheringPass(false) {}

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

  void visitFunctionExpr(FunctionExpr *func);

  void visitTableExpr(TableExpr *table);
  void visitClassExpr(ClassExpr *klass);

  void visitParamDecl(ParamDecl *p);
  void visitVarDecl(VarDecl *decl);
  void visitConstDecl(ConstDecl *cnst);
  void visitEnumDecl(EnumDecl *enm);
  void visitDestructuringDecl(DestructuringDecl *decl);

  void visitImportStatement(ImportStmt *import);

  // Declare a host-provided binding into the current scope. Creates an
  // SK_IMPORT SymbolInfo with the name, attaches a fresh ExternalValue, and
  // returns the ValueRef. Only used by analyze() when ingesting `bindings`.
  ValueRef *declareHostBinding(const char *name, const SQObject &val, const Node *location);

  // Make an ExternalValue-only ValueRef for stashing into astValues so that
  // callers of findValueForExpr can see the value of a node without a real
  // declaration (synthetic require()/require_optional() result, GetField on
  // an external). The returned ValueRef has info == nullptr; see ValueRef::info docs.
  ValueRef *makeExternalValueRef(const SQObject &val, const Node *location);

  // Attach an ExternalValue (and mark VRS_INITIALIZED) to an existing ValueRef
  // - for destructuring slots that already have a SymbolInfo from visitVarDecl,
  // or any other site that already created a ValueRef and just needs the value.
  // The Node* form derives line/col/width from the node; the explicit-coord
  // form is for sites that have parser coordinates but no AST node (e.g. an
  // import-slot symbol that points at the `from X import name` line).
  void attachExternalValue(ValueRef *v, const SQObject &val, const Node *location);
  void attachExternalValue(ValueRef *v, const SQObject &val, int line, int col, int width);

  void checkDestructuredVarDefault(VarDecl *var);

  void analyze(RootBlock *root, const HSQOBJECT *bindings);
};


}
