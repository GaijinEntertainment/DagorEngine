#pragma once

#include "../ast.h"
#include "../arena.h"
#include "../sqstate.h"

namespace SQCompilation {

// TODO: Unify this with functions from static analyzer
struct str_hash {
  size_t operator()(const char *s) const {
    size_t h = 0;
    for (; *s; ++s)
      h = h * 31 + (unsigned char)*s;
    return h;
  }
};
struct str_eq {
  bool operator()(const char *a, const char *b) const {
    return std::strcmp(a, b) == 0;
  }
};

typedef ArenaUnorderedSet<const char *, str_hash, str_eq> NameSet;

// Represents a closure that CAN be hoisted
struct HoistCandidate {
  FunctionExpr *func;
  int hoistDepth;              // How many levels to hoist
  const char *generatedName;   // $ch0, $ch1, etc. (assigned when hoisting)
  int insertionPoint;          // Index in target block where to insert (before this statement)
};

// Function/class-level scope context (NOT per-block). Created for: root scope,
// each FunctionExpr, and each ClassExpr. Block-level constructs (for, foreach,
// try, if, while) do NOT create their own ScopeContext — their variables are
// tracked in the enclosing function's localNames. The directLocalNames subset
// distinguishes top-level declarations from those inside nested blocks.
// The localNames/directLocalNames sets are arena-allocated and survive scope exit,
// allowing CaptureAnalyzer to share them via pointer (zero-copy).
struct ScopeContext {
  ScopeContext *parent;        // null for root scope
  bool isClassScope;           // true only for class body (not methods inside it)
  int depth;                   // Nesting depth (class scopes don't increment)
  Block *block;                // The block where hoisted decls go
  int currentStatementIndex;   // Index of statement currently being visited in this scope's block

  // ALL locals declared in this function scope (params, vars, for-loop inits,
  // foreach vars, try exception vars). Used for capture depth calculation.
  // Arena-allocated pointer: survives scope exit for CaptureAnalyzer reuse.
  NameSet *localNames;

  // Subset of localNames: only locals declared as direct children of this
  // scope's block (params + top-level let/const/destructuring). Excludes
  // for-loop init vars, foreach vars, try exception vars, etc.
  // Used by isIndirectLocalAtDepth() to detect captures from nested blocks.
  NameSet *directLocalNames;

  // Candidates found in child scopes that should hoist to THIS level
  ArenaVector<HoistCandidate> pendingHoists;

  static NameSet *makeNameSet(Arena *arena) {
    void *mem = arena->allocate(sizeof(NameSet));
    return new (mem) NameSet(NameSet::Allocator(arena));
  }

  // Constructor computes depth, skipping class scopes
  ScopeContext(Arena *arena, ScopeContext *p, bool isClass, Block *b)
      : parent(p),
        isClassScope(isClass),
        depth(p ? p->depth + (p->isClassScope ? 0 : 1) : 0),
        block(b),
        currentStatementIndex(0),
        localNames(makeNameSet(arena)),
        directLocalNames(makeNameSet(arena)),
        pendingHoists(arena) {}

  int findNameDepth(const char *name) const {
    if (localNames->find(name) != localNames->end()) return depth;
    if (parent) return parent->findNameDepth(name);
    return -1;  // Not found (global/builtin)
  }

  bool isInImmediateParent(const char *name) const {
    if (!parent) return false;
    return parent->localNames->find(name) != parent->localNames->end();
  }

  // Check if a name at a given depth is in a nested block (not directly in that scope's block).
  // Returns true if the name is a local at that depth but NOT a direct block declaration.
  bool isIndirectLocalAtDepth(const char *name, int targetDepth) const {
    // Walk up to scope at targetDepth
    const ScopeContext *scope = this;
    while (scope && scope->depth > targetDepth) {
      scope = scope->parent;
    }
    if (!scope || scope->depth != targetDepth) {
      return false;  // Couldn't find scope at this depth
    }
    // Check if name is in scope's localNames but NOT in directLocalNames
    if (scope->localNames->find(name) != scope->localNames->end()) {
      return scope->directLocalNames->find(name) == scope->directLocalNames->end();
    }
    return false;
  }

  bool isClassMethod() const {
    return parent && parent->isClassScope;
  }

  // Count of locals including pending hoists (for frame size limit)
  size_t localCount() const {
    return localNames->size() + pendingHoists.size();
  }

  // Leave some room for temporary variables
  static const size_t MAX_LOCALS_FOR_HOISTING = 200;

  // Trim pending hoists if adding them would exceed the locals limit
  void enforceLocalsLimit() {
    int maxHoists = std::max(0, (int)MAX_LOCALS_FOR_HOISTING - (int)localNames->size());
    while ((int)pendingHoists.size() > maxHoists) {
      pendingHoists.pop_back();
    }
  }
};

// Pre-computed locals per function: FunctionExpr* -> its localNames set.
// Populated during HoistingVisitor, consumed by CaptureAnalyzer.
// Shares the same arena-allocated NameSet pointers from ScopeContext.
typedef ArenaUnorderedMap<FunctionExpr *, const NameSet *> FuncLocalsMap;

class ClosureHoistingOpt {
public:
  ClosureHoistingOpt(SQSharedState *ss, Arena *astArena);
  void run(RootBlock *root);

private:
  SQSharedState *_ss;
  Arena *astArena;             // For AST node allocation
  Arena arena;                 // Internal arena for temporary data structures
  int varIdx;                  // Counter for generating $ch0, $ch1, etc.

  // FunctionExpr* -> generated name mapping
  typedef ArenaUnorderedMap<FunctionExpr *, const char *> HoistedMap;
  HoistedMap hoistedClosures;

  FuncLocalsMap funcLocals;

  // Pre-compute which locals are declared directly in a block (not in nested
  // for/if/try/while blocks).
  static void collectDirectLocals(Block *block, ScopeContext &scope);

  // Single-pass visitor that collects local names, evaluates hoisting,
  // and inserts hoisted VarDecls at scope boundaries
  class HoistingVisitor : public Visitor {
    ClosureHoistingOpt *owner;
    ScopeContext *currentScope;
    bool insideConstDecl = false;

    void registerFunctionParams(FunctionExpr *f, ScopeContext &scope);
    void tryHoistFunction(FunctionExpr *f, ScopeContext &funcScope);

  public:
    HoistingVisitor(ClosureHoistingOpt *o, ScopeContext *root)
        : owner(o), currentScope(root) {}

    void visitBlock(Block *b) override;
    void visitFunctionExpr(FunctionExpr *f) override;
    void visitClassExpr(ClassExpr *c) override;
    void visitVarDecl(VarDecl *v) override;
    void visitParamDecl(ParamDecl *p) override;
    void visitConstDecl(ConstDecl *c) override;
    void visitTryStatement(TryStatement *stmt) override;
    void visitForeachStatement(ForeachStatement *fe) override;
  };

  // After Phase 1, each hoisted FunctionExpr is referenced from two places:
  // the new VarDecl (at the target scope) and the original location.
  // This transformer resolves the duplication by replacing the original
  // occurrence with an Id reference to the hoisted variable.
  class OriginalClosureReplacer : public Transformer {
    HoistedMap &hoistedClosures;
    ArenaUnorderedSet<FunctionExpr *> resolved;  // Hoisted FunctionExprs already resolved (kept at VarDecl)
    Arena *astArena;

  public:
    OriginalClosureReplacer(HoistedMap &map, Arena *arena, Arena *astA)
        : hoistedClosures(map),
          resolved(decltype(resolved)::Allocator(arena)),
          astArena(astA) {}

    Node *transformFunctionExpr(FunctionExpr *f) override;
  };

  // Helper methods
  const char *generateName();
  void insertHoistedDecls(Block *block, ArenaVector<HoistCandidate> &candidates);
};

} // namespace SQCompilation
