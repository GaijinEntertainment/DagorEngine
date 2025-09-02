#include "closureHoisting.h"

#include <algorithm>
#include <stdio.h>

namespace SQCompilation {

ClosureHoistingOpt::ClosureHoistingOpt(SQSharedState *ss, Arena *astA)
  : _ss(ss)
  , astArena(astA)
  , arena(ss->_alloc_ctx, "ClosureHoistingOpt Arena")
  , scopeMap(ScopeMap::Allocator(&arena))
  , relocMap(RelocateMap::Allocator(&arena))
  , relocatedSet(RememberSet::Allocator(&arena))
  , varIdx(0) {

}

template<typename N>
static N *copyCoordinates(const Node *f, N *t) {
  t->setLineStartPos(f->lineStart());
  t->setColumnStartPos(f->columnStart());
  t->setLineEndPos(f->lineEnd());
  t->setColumnEndPos(f->columnEnd());

  return t;
}

void ClosureHoistingOpt::ScopeComputeVisitor::visitFunctionDecl(FunctionDecl *f) {

  if (f->name()) {
    currentScope->declareSymbol(f->name(), f);
  }

  VarScope *newScope = new(arena) VarScope(arena, currentScope,
    currentScope->kind == CLASS_SCOPE ? METHOD_SCOPE : FUNC_SCOPE);
  currentScope = newScope;
  scopeMap[f] = newScope;

  f->visitChildren(this);

  maxFrameSize = std::max(newScope->symbols.size(), maxFrameSize);

  currentScope = newScope->parent;
}

void ClosureHoistingOpt::ScopeComputeVisitor::visitClassDecl(ClassDecl *klass) {
  Expr *key = klass->classKey();
  if (key && key->op() == TO_ID) {
    currentScope->declareSymbol(key->asId()->name(), klass);
  }
  else if(key) {
    key->visit(this);
  }

  if (klass->classBase()) {
    klass->classBase()->visit(this);
  }

  VarScope *newScope = new(arena) VarScope(arena, currentScope, CLASS_SCOPE);
  currentScope = newScope;
  scopeMap[klass] = newScope;

  klass->TableDecl::visitChildren(this);

  currentScope = newScope->parent;
}

void ClosureHoistingOpt::ScopeComputeVisitor::visitValueDecl(ValueDecl *v) {
  currentScope->declareSymbol(v->name(), v);
  v->visitChildren(this);
}

void ClosureHoistingOpt::ScopeComputeVisitor::visitConstDecl(ConstDecl *c) {
  currentScope->declareSymbol(c->name(), c);
}

void ClosureHoistingOpt::ScopeComputeVisitor::visitTryStatement(TryStatement *stmt) {
  stmt->tryStatement()->visit(this);
  currentScope->declareSymbol(stmt->exceptionId()->name(), NULL);
  stmt->catchStatement()->visit(this);
}

int ClosureHoistingOpt::computeScopes(RootBlock *root) {
  scopeMap.clear();

  VarScope *rootScope = new (&arena) VarScope(&arena, NULL, ROOT_SCOPE);

  scopeMap[NULL] = rootScope;

  ScopeComputeVisitor scopeComputer(scopeMap, &arena, rootScope);

  root->visit(&scopeComputer);

  return std::max(scopeComputer.maxFrameSize, rootScope->symbols.size());
}

int ClosureHoistingOpt::findCandidates(RootBlock *root) {
  CandidateCounterVisitor visitor(scopeMap, &arena);

  root->visit(&visitor);

  return visitor.numberOfCandidates;
}

class CounterState {
  ClosureHoistingOpt::CandidateCounterVisitor *visitor;
  CounterState *prev;

  typedef StdArenaAllocator<const char *> UseSetAllocator;
  typedef std::set<const char *, cmp_str, UseSetAllocator> UseSet;

  UseSet useSet;

  void mergeUsedNames(UseSet &prevSet) {
    ClosureHoistingOpt::VarScope *scope = visitor->currentScope;

    for (auto name : useSet) {
      auto it = scope->symbols.find(name);
      if (it == scope->symbols.end()) { // name is not local name so add it into set
        prevSet.insert(name);
      }
    }
  }

public:

  void useName(const char *name) {
    useSet.insert(name);
  }

  bool check(int &depth) {
    for (auto name : useSet) {
      int k = visitor->getSymbolKind(name);
      depth = std::min(depth, k);
      if (k == ClosureHoistingOpt::CandidateCounterVisitor::SymbolKind::SK_OUT_LOCAL) {
        return false;
      }
    }
    return true;
  }

  CounterState(ClosureHoistingOpt::CandidateCounterVisitor *v) : visitor(v), useSet(UseSetAllocator(v->arena)) {
    prev = v->state;
    v->state = this;
  }

  ~CounterState() {
    if (prev)
      mergeUsedNames(prev->useSet);
    visitor->state = prev;
  }
};

int ClosureHoistingOpt::CandidateCounterVisitor::getSymbolKind(const char *n) const {

  VarScope *scope = currentScope;

  int depth = 0;

  auto it = scope->symbols.find(n);

  if (it != scope->symbols.end()) {
    return SymbolKind::SK_LOCAL;
  }

  scope = scope->parent;
  if (!scope) {
    return SymbolKind::SK_UNKNOWN;
  }

  depth = 1;

  if (scope->symbols.find(n) != scope->symbols.end()) {
    return SymbolKind::SK_OUT_LOCAL;
  }

  scope = scope->parent;

  while (scope) {
    if (scope->kind != CLASS_SCOPE) {
      ++depth;
    }
    if (scope->symbols.find(n) != scope->symbols.end()) {
      return depth;
    }
    scope = scope->parent;
  }

  return SymbolKind::SK_UNKNOWN;
}

void ClosureHoistingOpt::CandidateCounterVisitor::visitId(Id *id) {
  int depth = getSymbolKind(id->name());

  if (depth != SymbolKind::SK_LOCAL && depth != SymbolKind::SK_UNKNOWN) {
    if (state) {
      state->useName(id->name());
    }
  }
}

void ClosureHoistingOpt::CandidateCounterVisitor::visitFunctionDecl(FunctionDecl *f) {

  CounterState cs(this);
  VarScope *oldScope = currentScope;

  VarScope * funcScope = currentScope = scopeMap[f];
  assert(funcScope);

  for (auto param : f->parameters())
    param->visit(this);

  ++curDepth;
  f->body()->visit(this);

  int depth = curDepth;

  --curDepth;

  if (oldScope->kind != ROOT_SCOPE // skip top-level functions
      && funcScope->kind != METHOD_SCOPE// skip class methods
      && cs.check(depth)
    ) {
    ++numberOfCandidates;
    oldScope->candidates[f] = depth;
  }

  currentScope = oldScope;
}

const char *ClosureHoistingOpt::HoistingVisitor::generateName() {
  char buffer[1024] = { 0 };
  size_t n = snprintf(buffer, sizeof buffer, "$ch%d", varIndex++);
  char *result = (char *)astArena->allocate(n * sizeof(char));
  strcpy(result, buffer);
  return result;
}

VarDecl *ClosureHoistingOpt::HoistingVisitor::createStubVariable(FunctionDecl *f) {
  DeclExpr *dexpr = copyCoordinates(f, new (astArena) DeclExpr(f));
  const char *name = generateName();

  relocMap[f] = name;

  relocSet.insert(dexpr);

  return copyCoordinates(f, new (astArena) VarDecl(name, dexpr, false));
}

ClosureHoistingOpt::HoistingVisitor::RelocateState *ClosureHoistingOpt::HoistingVisitor::findState(int depth) {
  RelocateState *s = relocState;

  while (s) {
    if (s->depth == depth) return s;
    s = s->prev;
  }

  assert(0);
  return NULL;
}

void ClosureHoistingOpt::HoistingVisitor::doRelocation(RelocateState *s, VarDecl *wrap) {
  s->statements.insert(s->index++, wrap);
  s->size += 1;
}

void ClosureHoistingOpt::HoistingVisitor::relocateCandidate(FunctionDecl *f, int depth) {
  RelocateState *state = relocState;
  int depthToFind = state->depth - depth  + 1;
  assert(depthToFind >= 0);

  RelocateState *putState = findState(depthToFind);
  VarDecl *wrapDecl = createStubVariable(f);

  f->hoistBy(depth);

  doRelocation(putState, wrapDecl);
}

ClosureHoistingOpt::HoistingVisitor::RelocateState::RelocateState(ArenaVector<Statement *> &stmts, HoistingVisitor *v)
  : statements(stmts), index(0), size(stmts.size()), prev(v->relocState), visitor(v), depth(v->scopeDepth) {
  v->relocState = this;
}

ClosureHoistingOpt::HoistingVisitor::RelocateState::~RelocateState() {
  visitor->relocState = prev;
}

void ClosureHoistingOpt::HoistingVisitor::visitBlock(Block *b) {
  auto &statements = b->statements();

  RelocateState state(statements, this);

  for (; state.index < state.size; ++state.index) {
    Statement *stmt = statements[state.index];
    stmt->visit(this);
  }
}

void ClosureHoistingOpt::HoistingVisitor::visitFunctionDecl(FunctionDecl *f) {
  ++scopeDepth;

  VarScope *oldScope = scope;

  scope = scopeMap[f];
  assert(scope);

  f->visitChildren(this);

  scope = oldScope;

  auto it = oldScope->candidates.find(f);

  if (it != oldScope->candidates.end()) {
    relocateCandidate(it->first, it->second);
  }

  --scopeDepth;
}

void ClosureHoistingOpt::hoistClosures(RootBlock *root) {
  relocatedSet.clear();
  relocMap.clear();
  HoistingVisitor visitor(scopeMap, relocMap, relocatedSet, varIdx, astArena);
  root->visit(&visitor);
}

Id *ClosureHoistingOpt::RemapTransformer::createId(const char *name) {
  return new (astArena) Id(name);
}

Node *ClosureHoistingOpt::RemapTransformer::transformDeclExpr(DeclExpr *e) {

  Decl *d = e->declaration();
  auto it = relocSet.find(e);
  if (it == relocSet.end()) {
    if (d->op() == TO_FUNCTION) {
      FunctionDecl *f = (FunctionDecl *)d;
      auto it = relocMap.find(f);
      if (it != relocMap.end()) {
        return copyCoordinates(e, createId(it->second));
      }
    }
  }

  d->transformChildren(this);
  return e;
}

Decl *ClosureHoistingOpt::VarScope::findDeclaration(const char *name) {
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    return it->second;
  }

  if (parent) return parent->findDeclaration(name);

  return NULL;
}

Node *ClosureHoistingOpt::RemapTransformer::transformId(Id *id) {
  Decl *d = scope->findDeclaration(id->name());
  if (d && d->op() == TO_FUNCTION) {
    FunctionDecl *f = (FunctionDecl *)d;
    auto it = relocMap.find(f);
    if (it != relocMap.end()) {
      return copyCoordinates(id, createId(it->second));
    }
  }

  return id;
}

Node *ClosureHoistingOpt::RemapTransformer::transformFunctionDecl(FunctionDecl *f) {
  VarScope *old = scope;

  scope = scopeMap[f];

  f->transformChildren(this);

  scope = old;

  auto it = relocMap.find(f);
  if (it != relocMap.end()) {
    return new(astArena) EmptyStatement();
  }

  return f;
}

void ClosureHoistingOpt::remapUsages(RootBlock *root) {
  RemapTransformer transformer(scopeMap, relocMap, relocatedSet, astArena);
  root->transform(&transformer);
}

void ClosureHoistingOpt::run(RootBlock *root, const char *fname) {

  if (!_ss->checkCompilationOption(CompilationOptions::CO_CLOSURE_HOISTING_OPT))
    return;

  for (;;) {
    // 1. First we need information about name and scopes where that names are declared so compute it
    int maxFrameSize = computeScopes(root);

    // 2. Second once we have scope info we could figure out what closures could be hoistied. Do that.
    int numOfCandidates = findCandidates(root);

    // 2.1 If no candidates found - finish
    // 2.2 If after hoisting the total number of locals exceeds 256 skip optimization since that code become uncompilable
    if (numOfCandidates == 0 || (maxFrameSize + numOfCandidates) > 0xFF) break;

    // 3. Hoist candidates to appropriate places
    hoistClosures(root);

    // 4. Link hoisted closures with their usages
    remapUsages(root);

    // 5. Do once again if we could hoist something else
  }
}

} // namespace SQCompilation
