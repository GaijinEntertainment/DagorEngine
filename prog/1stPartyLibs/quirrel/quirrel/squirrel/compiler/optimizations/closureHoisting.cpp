/*****

The basic algorithm:
* Walk the AST top-down
  * Calculate nesting depth
  * Collect locals in each scope
* For each closure, find the deepest scope it captures.
* If the closure can be hoisted, create a VarDecl `let $chN = <FunctionExpr>`
  and insert it into the highest scope reachable (the one where the deepest
  captured variable lives). Within that scope's block, place it right before
  the statement that (transitively) contains the closure to ensure correct
  ordering with respect to imports and other declarations.
* In Phase 2 (OriginalClosureReplacer) replace the original FunctionExpr
  with an Id reference to the generated name ($ch0, $ch1, ...)

*****/

#include "closureHoisting.h"

#include <algorithm>
#include <cstdio>

namespace SQCompilation {

ClosureHoistingOpt::ClosureHoistingOpt(SQSharedState *ss, Arena *astA)
  : _ss(ss)
  , astArena(astA)
  , arena(ss->_alloc_ctx, "ClosureHoistingOpt")
  , varIdx(0)
  , hoistedClosures(HoistedMap::Allocator(&arena))
  , funcLocals(FuncLocalsMap::Allocator(&arena))
{}


const char *ClosureHoistingOpt::generateName() {
  char buffer[32];
  int n = snprintf(buffer, sizeof(buffer), "$ch%d", varIdx++);
  char *result = (char *)astArena->allocate((n + 1) * sizeof(char));
  strcpy(result, buffer);
  return result;
}

//------------------------------------------------------------------------------
// Capture Analysis
//------------------------------------------------------------------------------

// Result of capture analysis for a single closure (internal to this file).
struct CaptureInfo {
  int maxCaptureDepth;          // Deepest captured variable's depth (-1 if none)
  bool capturesImmediateParent; // Does it capture from immediate parent? (blocks hoisting)
  int maxIndirectCaptureDepth;  // Max depth with indirect capture (-1 if none)
};

// Visitor that analyzes what a closure captures.
// Uses a scope stack (not a flat set) to correctly handle nested function
// parameter shadowing: a nested function's parameter only masks names
// within that nested function, not in the outer scope.
class CaptureAnalyzer : public Visitor {
  CaptureInfo &info;
  ScopeContext *funcScope;    // The function being analyzed
  const char *selfName;       // The function's name (for self-reference detection)
  FuncLocalsMap &funcLocals;  // Pre-computed locals per function

  // Stack of local name sets for nested scopes.
  // Bottom = the function being analyzed, top = innermost nested function.
  // Names are checked from top to bottom; popping correctly un-shadows.
  ArenaVector<const NameSet *> scopeStack;

  bool isLocal(const char *name) {
    for (int i = (int)scopeStack.size() - 1; i >= 0; i--)
      if (scopeStack[i]->find(name) != scopeStack[i]->end())
        return true;
    return false;
  }

  // Once the function captures from its immediate parent, it can't be hoisted
  // at all — stop traversing early.
  bool canHoist() const { return !info.capturesImmediateParent; }

public:
  CaptureAnalyzer(CaptureInfo &i, ScopeContext *fs, const char *sn,
                   FuncLocalsMap &fl, Arena *a)
      : info(i), funcScope(fs), selfName(sn), funcLocals(fl),
        scopeStack(a) {}

  void visitNode(Node *node) override {
    if (!canHoist()) return;
    node->visitChildren(this);
  }

  void visitId(Id *id) override {
    if (!canHoist()) return;

    const char *name = id->name();

    // Self-reference to the function is treated as a capture from immediate parent
    // (where the function name is declared). This blocks hoisting.
    if (selfName && strcmp(name, selfName) == 0) {
      if (funcScope->isInImmediateParent(name))
        info.capturesImmediateParent = true;
      return;
    }

    // Local to this function or a nested function at current nesting level
    if (isLocal(name))
      return;

    // Check if captured from the function's immediate parent (blocks hoisting)
    if (funcScope->isInImmediateParent(name))
      info.capturesImmediateParent = true;

    // Find capture depth relative to the function's parent
    int depth = funcScope->parent ? funcScope->parent->findNameDepth(name) : -1;
    if (depth >= 0) {
      info.maxCaptureDepth = std::max(info.maxCaptureDepth, depth);

      // Check if this capture is from a nested block (not directly in the scope's block).
      // Such captures prevent hoisting to that depth.
      if (funcScope->parent->isIndirectLocalAtDepth(name, depth)) { //-V1004
        info.maxIndirectCaptureDepth = std::max(info.maxIndirectCaptureDepth, depth);
      }
    }
  }

  // Descend into nested functions: push their pre-computed locals, visit, pop.
  // This correctly handles parameter shadowing — a nested function's param
  // only masks names within that function's body.
  void visitFunctionExpr(FunctionExpr *f) override {
    if (!canHoist()) return;

    // Visit parameter defaults FIRST, before pushing this function's locals.
    // Defaults are evaluated in the enclosing scope, not the function body scope,
    // so body locals must not shadow outer names during default analysis.
    for (auto param : f->parameters()) {
      if (!canHoist()) break;
      if (param->defaultValue())
        param->defaultValue()->visit(this);
      if (param->getDestructuring()) {
        for (auto decl : param->getDestructuring()->declarations()) {
          if (decl->expression())
            decl->expression()->visit(this);
        }
      }
    }

    auto it = funcLocals.find(f);
    if (it != funcLocals.end()) {
      scopeStack.push_back(it->second);
    }

    if (canHoist())
      f->body()->visit(this);

    if (it != funcLocals.end())
      scopeStack.pop_back();
  }
};

//------------------------------------------------------------------------------
// Insert Hoisted Declarations
//------------------------------------------------------------------------------

void ClosureHoistingOpt::insertHoistedDecls(Block *block,
                                             ArenaVector<HoistCandidate> &candidates) {
  if (candidates.empty() || !block)
    return;

  // Sort candidates by insertion point to handle them in order
  std::stable_sort(candidates.begin(), candidates.end(),
    [](const HoistCandidate &a, const HoistCandidate &b) {
      return a.insertionPoint < b.insertionPoint;
    });

  // Track how many we've inserted so far (to adjust subsequent indices)
  int insertionsBeforePoint = 0;

  for (auto &candidate : candidates) {
    // Generate unique name ($ch0, $ch1, etc.)
    candidate.generatedName = generateName();

    // Record in hoistedClosures map (FunctionExpr* -> name)
    hoistedClosures[candidate.func] = candidate.generatedName;

    // Update hoisting level on FunctionExpr (for codegen)
    candidate.func->hoistBy(candidate.hoistDepth);

    // Create VarDecl with the same FunctionExpr* as initializer.
    // The original location still references this FunctionExpr —
    // OriginalClosureReplacer (Phase 2) will replace it with an Id.
    //
    // Use the insertion point statement's source position for the VarDecl,
    // not the original lambda position. This ensures the _OP_CLOSURE
    // instruction gets the correct line number for stack traces.
    int actualIdx = candidate.insertionPoint + insertionsBeforePoint;
    SourceLoc insertLoc = candidate.func->sourceSpan().start;
    if (actualIdx < (int)block->statements().size()) {
      Statement *atStmt = block->statements()[actualIdx];
      // When inserting before a bare Block statement, use the first child
      // statement's location instead of the block's opening brace location.
      // The '{' brace doesn't generate instructions, so using its line would
      // create a spurious line entry that changes stack traces.
      if (atStmt->op() == TO_BLOCK) {
        Block *blk = static_cast<Block *>(atStmt);
        if (!blk->statements().empty())
          insertLoc = blk->statements()[0]->sourceSpan().start;
        else
          insertLoc = atStmt->sourceSpan().start;
      } else {
        insertLoc = atStmt->sourceSpan().start;
      }
    }

    Id *nameId = new (astArena) Id({insertLoc, insertLoc},
                                    candidate.generatedName);
    VarDecl *hoistedDecl = new (astArena) VarDecl(
        insertLoc, nameId, candidate.func, false);

    // Insert into block at the correct position
    block->statements().insert(actualIdx, hoistedDecl);
    insertionsBeforePoint++;
  }
}

//------------------------------------------------------------------------------
// Typed Default Parameter Safety Check
//------------------------------------------------------------------------------

// Check if a function has any parameter with both a type annotation and a
// default value. The type check for such defaults happens at closure creation
// time and can throw. Hoisting would move that throw to a different scope
// (e.g. out of a try/catch), changing program behavior.
static bool hasTypedDefaults(FunctionExpr *f) {
  for (auto param : f->parameters()) {
    if (param->getTypeMask() != ~0u && param->hasDefaultValue())
      return true;
  }
  return false;
}

//------------------------------------------------------------------------------
// HoistingVisitor Helpers
//------------------------------------------------------------------------------

// Pre-compute directLocalNames by scanning the block's direct child statements.
// A local is "direct" if its declaration is a top-level statement of the block,
// as opposed to being inside a for-loop init, if-body, try-body, etc.
void ClosureHoistingOpt::collectDirectLocals(Block *block, ScopeContext &scope) {
  if (!block) return;
  for (auto stmt : block->statements()) {
    switch (stmt->op()) {
      case TO_VAR:
        scope.directLocalNames->insert(static_cast<VarDecl *>(stmt)->name());
        break;
      case TO_CONST:
        scope.directLocalNames->insert(static_cast<ConstDecl *>(stmt)->name());
        break;
      case TO_DECL_GROUP:
      case TO_DESTRUCTURE:
        for (auto decl : static_cast<DeclGroup *>(stmt)->declarations())
          scope.directLocalNames->insert(decl->name());
        break;
      default:
        break;
    }
  }
}

void ClosureHoistingOpt::HoistingVisitor::registerFunctionParams(
    FunctionExpr *f, ScopeContext &scope) {
  for (auto param : f->parameters()) {
    scope.localNames->insert(param->name());
    scope.directLocalNames->insert(param->name());
    if (param->getDestructuring()) {
      for (auto decl : param->getDestructuring()->declarations()) {
        scope.localNames->insert(decl->name());
        scope.directLocalNames->insert(decl->name());
      }
    }
  }
}

void ClosureHoistingOpt::HoistingVisitor::tryHoistFunction(
    FunctionExpr *f, ScopeContext &funcScope) {
  // Don't hoist top-level functions or class methods
  ScopeContext *parentScope = funcScope.parent;
  if (!parentScope || !parentScope->parent)
    return;
  if (funcScope.isClassMethod())
    return;
  // Don't hoist const functions - they are compile-time constants
  if (insideConstDecl)
    return;

  // Don't hoist functions with typed default parameters.
  // The type check for defaults happens at closure creation and can throw.
  // Hoisting moves the closure creation to a different scope, which can
  // change error handling behavior (e.g. moving it out of a try/catch).
  if (hasTypedDefaults(f))
    return;

  // Analyze what this closure captures
  CaptureInfo captures = {-1, false, -1};
  CaptureAnalyzer analyzer(captures, &funcScope, f->name(), owner->funcLocals, &owner->arena);
  analyzer.visitFunctionExpr(f);

  if (captures.capturesImmediateParent)
    return;

  // Compute target depth based on captures
  int targetDepth;
  if (captures.maxCaptureDepth < 0) {
    targetDepth = 0;  // No captures — hoist all the way to root
  } else {
    // Can't hoist above the deepest capture.
    // Indirect captures (in nested blocks) require staying one level deeper.
    targetDepth = captures.maxCaptureDepth;
    if (captures.maxIndirectCaptureDepth >= 0)
      targetDepth = std::max(targetDepth, captures.maxIndirectCaptureDepth + 1);
  }

  if (targetDepth >= funcScope.depth)
    return;

  // Walk up to the target scope
  ScopeContext *target = parentScope;
  while (target && target->depth > targetDepth)
    target = target->parent;

  if (target && target->localCount() < ScopeContext::MAX_LOCALS_FOR_HOISTING) {
    int hoistDepth = funcScope.depth - targetDepth;
    target->pendingHoists.push_back({f, hoistDepth, nullptr, target->currentStatementIndex});
  }
}

//------------------------------------------------------------------------------
// HoistingVisitor Implementation
//------------------------------------------------------------------------------

void ClosureHoistingOpt::HoistingVisitor::visitBlock(Block *b) {
  bool isOwnBlock = currentScope && currentScope->block == b;
  int savedIndex = currentScope ? currentScope->currentStatementIndex : 0;

  for (int i = 0; i < (int)b->statements().size(); ++i) {
    if (isOwnBlock)
      currentScope->currentStatementIndex = i;
    b->statements()[i]->visit(this);
  }

  if (isOwnBlock)
    currentScope->currentStatementIndex = savedIndex;
}

void ClosureHoistingOpt::HoistingVisitor::visitFunctionExpr(FunctionExpr *f) {
  ScopeContext newScope(&owner->arena, currentScope, /*isClassScope=*/false, f->body());
  registerFunctionParams(f, newScope);
  collectDirectLocals(f->body(), newScope);

  ScopeContext *prevScope = currentScope;
  currentScope = &newScope;
  for (auto param : f->parameters())
    param->visit(this);
  f->body()->visit(this);
  currentScope = prevScope;

  // Save locals for CaptureAnalyzer (zero-copy pointer sharing)
  owner->funcLocals[f] = newScope.localNames;

  newScope.enforceLocalsLimit();
  owner->insertHoistedDecls(f->body(), newScope.pendingHoists);
  tryHoistFunction(f, newScope);
}

void ClosureHoistingOpt::HoistingVisitor::visitClassExpr(ClassExpr *c) {
  if (c->classBase())
    c->classBase()->visit(this);

  ScopeContext newScope(&owner->arena, currentScope, /*isClassScope=*/true, nullptr);

  ScopeContext *prevScope = currentScope;
  currentScope = &newScope;

  if (c->classKey() && c->classKey()->op() != TO_ID)
    c->classKey()->visit(this);

  for (auto &member : c->members()) {
    if (member.key) member.key->visit(this);
    if (member.value) member.value->visit(this);
  }

  currentScope = prevScope;
}

void ClosureHoistingOpt::HoistingVisitor::visitVarDecl(VarDecl *v) {
  currentScope->localNames->insert(v->name());
  if (v->expression())
    v->expression()->visit(this);
}

void ClosureHoistingOpt::HoistingVisitor::visitParamDecl(ParamDecl *p) {
  if (p->defaultValue())
    p->defaultValue()->visit(this);
  if (p->getDestructuring()) {
    for (auto decl : p->getDestructuring()->declarations()) {
      if (decl->expression())
        decl->expression()->visit(this);
    }
  }
}

void ClosureHoistingOpt::HoistingVisitor::visitConstDecl(ConstDecl *c) {
  currentScope->localNames->insert(c->name());
  if (c->value()) {
    bool wasInConstDecl = insideConstDecl;
    insideConstDecl = true;
    c->value()->visit(this);
    insideConstDecl = wasInConstDecl;
  }
}

void ClosureHoistingOpt::HoistingVisitor::visitTryStatement(TryStatement *stmt) {
  stmt->tryStatement()->visit(this);
  currentScope->localNames->insert(stmt->exceptionId()->name());
  stmt->catchStatement()->visit(this);
}

void ClosureHoistingOpt::HoistingVisitor::visitForeachStatement(ForeachStatement *fe) {
  if (fe->idx()) currentScope->localNames->insert(fe->idx()->name());
  if (fe->val()) currentScope->localNames->insert(fe->val()->name());
  fe->container()->visit(this);
  fe->body()->visit(this);
}

//------------------------------------------------------------------------------
// OriginalClosureReplacer Implementation
//------------------------------------------------------------------------------

Node *ClosureHoistingOpt::OriginalClosureReplacer::transformFunctionExpr(FunctionExpr *f) {
  auto it = hoistedClosures.find(f);
  if (it != hoistedClosures.end()) {
    // First encounter is the hoisted VarDecl initializer — keep it.
    // Second encounter is the original location — replace with Id.
    if (resolved.find(f) == resolved.end()) {
      resolved.insert(f);
      f->transformChildren(this);
      return f;
    }
    const char *hoistedName = it->second;
    return new (astArena) Id(f->sourceSpan(), hoistedName);
  }

  f->transformChildren(this);
  return f;
}

//------------------------------------------------------------------------------
// Main Entry Point
//------------------------------------------------------------------------------

void ClosureHoistingOpt::run(RootBlock *root) {
  if (!_ss->checkCompilationOption(CompilationOptions::CO_CLOSURE_HOISTING_OPT))
    return;

  // Phase 1: Traverse AST, collect scope info, and insert hoisted VarDecls.
  // After this, each hoisted FunctionExpr is referenced from two places:
  // the new VarDecl (at the target scope) and the original location.
  ScopeContext rootScope(&arena, nullptr, /*isClassScope=*/false, root);
  collectDirectLocals(root, rootScope);
  HoistingVisitor visitor(this, &rootScope);
  root->visit(&visitor);

  rootScope.enforceLocalsLimit();
  insertHoistedDecls(root, rootScope.pendingHoists);

  // Phase 2: Resolve the duplication — replace original FunctionExpr
  // occurrences with Id references to the hoisted variable names.
  if (!hoistedClosures.empty()) {
    OriginalClosureReplacer transformer(hoistedClosures, &arena, astArena);
    root->transform(&transformer);
  }
}

} // namespace SQCompilation
