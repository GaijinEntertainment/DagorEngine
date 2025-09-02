#pragma once

#include "../sqast.h"
#include "../arena.h"
#include "../sqstate.h"
#include <map>
#include <set>

namespace SQCompilation {

struct cmp_str
{
  bool operator()(char const *a, char const *b) const
  {
    return std::strcmp(a, b) < 0;
  }
};

class CounterState;

class ClosureHoistingOpt {
  friend class CounterState;
  enum ScopeKind {
    ROOT_SCOPE,
    FUNC_SCOPE,
    CLASS_SCOPE,
    METHOD_SCOPE
  };

  struct VarScope : public ArenaObj {

    typedef ArenaMap<const char *, Decl *, cmp_str> SymbolsMap;
    typedef ArenaMap<FunctionDecl *, int> CandidateMap;

    SymbolsMap symbols;
    CandidateMap candidates;

    struct VarScope *parent;

    enum ScopeKind kind;

    void declareSymbol(const char *s, Decl *d) {
      symbols[s] = d;
    }

    Decl *findDeclaration(const char *name);

    VarScope(Arena *arena, VarScope *p, enum ScopeKind k) : symbols(SymbolsMap::Allocator(arena)), parent(p), candidates(CandidateMap::Allocator(arena)), kind(k) {}
  };

  SQSharedState *_ss;

  Arena *astArena;

  typedef ArenaMap<Decl *, VarScope *> ScopeMap;
  typedef ArenaMap<FunctionDecl *, const char *> RelocateMap;
  typedef ArenaSet<DeclExpr *> RememberSet;

  Arena arena;
  ScopeMap scopeMap;
  RelocateMap relocMap;
  RememberSet relocatedSet;
  int varIdx;

  class ScopeComputeVisitor : public Visitor {
    ScopeMap &scopeMap;
    VarScope *currentScope;
    Arena *arena;

  public:
    size_t maxFrameSize;

    void visitValueDecl(ValueDecl *v);
    void visitTryStatement(TryStatement *stmt);
    void visitConstDecl(ConstDecl *c);

    void visitFunctionDecl(FunctionDecl *decl);
    void visitClassDecl(ClassDecl *klass);

    ScopeComputeVisitor(ScopeMap &map, Arena *a, VarScope *rootScope)
      : scopeMap(map), arena(a), currentScope(rootScope), maxFrameSize(0) {}
  };

  class CandidateCounterVisitor : public Visitor {
    friend class CounterState;

    ScopeMap &scopeMap;
    VarScope *currentScope;
    CounterState *state;

    Arena *arena;

    int curDepth;

    enum SymbolKind : int {
      SK_UNKNOWN = -1,
      SK_LOCAL = 0,
      SK_OUT_LOCAL = 1,
      SK_OUT_OUT
    };

    int getSymbolKind(const char *) const;

  public:
    CandidateCounterVisitor(ScopeMap &map, Arena *a) : scopeMap(map), numberOfCandidates(0), arena(a) {
      currentScope = scopeMap[NULL];
      curDepth = 0;
      state = NULL;
    }

    void visitId(Id *id);
    void visitFunctionDecl(FunctionDecl *f);

    int numberOfCandidates;
  };

  class HoistingVisitor : public Visitor {
    ScopeMap &scopeMap;
    Arena *astArena;
    RelocateMap &relocMap;
    RememberSet &relocSet;
    int &varIndex;

    VarScope *scope;

    int scopeDepth;

    struct RelocateState {
      struct RelocateState *prev;
      size_t index;
      size_t size;
      ArenaVector<Statement *> &statements;
      int depth;
      HoistingVisitor *visitor;

      RelocateState(ArenaVector<Statement *> &stmts, HoistingVisitor *visitor);
      ~RelocateState();
    };

    struct RelocateState *relocState;

    const char *generateName();

    VarDecl *createStubVariable(FunctionDecl *);
    void relocateCandidate(FunctionDecl *f, int depth);
    void doRelocation(RelocateState *s, VarDecl *wrap);

    RelocateState *findState(int depth);

  public:

    void visitBlock(Block *b);
    void visitFunctionDecl(FunctionDecl *f);

    HoistingVisitor(ScopeMap &map, RelocateMap &rmap, RememberSet &set, int &varIdx, Arena *astA)
      : scopeMap(map), relocMap(rmap), relocSet(set), varIndex(varIdx), astArena(astA), relocState(NULL) {
      scope = scopeMap[NULL];
      scopeDepth = 0;
    }
  };

  class RemapTransformer : public Transformer {
    ScopeMap &scopeMap;
    RelocateMap &relocMap;
    RememberSet &relocSet;
    Arena *astArena;

    VarScope *scope;

    Id *createId(const char *name);

  public:
    Node *transformDeclExpr(DeclExpr *expr);
    Node *transformFunctionDecl(FunctionDecl *f);
    Node *transformId(Id *id);

    RemapTransformer(ScopeMap &sm, RelocateMap &rm, RememberSet &set, Arena *astA) : scopeMap(sm), relocMap(rm), relocSet(set), astArena(astA) {
      scope = scopeMap[NULL];
    }
  };

  int computeScopes(RootBlock *);
  int findCandidates(RootBlock *);
  void hoistClosures(RootBlock *);
  void remapUsages(RootBlock *);

public:

  ClosureHoistingOpt(SQSharedState *ss, Arena *astArena);

  void run(RootBlock *root, const char *fname);
};


} // namespace SQCompilation
