#pragma once

#include "analyzer_internal.h"
#include "symbol_info.h"


namespace SQCompilation
{

class NameShadowingChecker : public Visitor {
  SQCompilationContext _ctx;

  std::vector<const Node *> nodeStack;

  void report(const Node *n, int32_t id, ...) {
    va_list vargs;
    va_start(vargs, id);

    _ctx.vreportDiagnostic((enum DiagnosticsId)id, n->lineStart(), n->columnStart(), n->textWidth(), vargs); //-V522

    va_end(vargs);
  }

  struct Scope;

  struct SymbolInfo {
    union {
      const Id *x;
      const FunctionExpr *f;
      const ClassExpr *k;
      const VarDecl *v;
      const TableExpr *t;
      const ParamDecl *p;
      const EnumDecl *e;
      const ConstDecl *c;
      const EnumConst *ec;
    } declaration;

    SymbolKind kind;

    const struct Scope *ownerScope;
    const char *name;

    SymbolInfo(SymbolKind k) : kind(k), declaration(), name(nullptr), ownerScope(nullptr) {}
  };

  struct Scope {
    NameShadowingChecker *checker;

    Scope(NameShadowingChecker *chk, const Node *o) : checker(chk), owner(o), symbols() {
      parent = checker->scope;
      checker->scope = this;
    }

    ~Scope() {
      checker->scope = parent;
    }

    struct Scope *parent;
    std::unordered_map<const char *, SymbolInfo *, StringHasher, StringEqualer> symbols;
    const Node *owner;

    SymbolInfo *findSymbol(const char *name) const;
  };

  const Node *extractPointedNode(const SymbolInfo *info);

  SymbolInfo *newSymbolInfo(SymbolKind k) {
    void *mem = _ctx.arena()->allocate(sizeof(SymbolInfo));
    return new(mem) SymbolInfo(k);
  }

  struct Scope rootScope;

  struct Scope *scope;

  void loadBindings(const HSQOBJECT *bindings);

  void declareVar(SymbolKind k, const VarDecl *v);
  void declareSymbol(const char *name, SymbolInfo *info);

  Id rootPointerNode;
public:
  NameShadowingChecker(SQCompilationContext &ctx, const HSQOBJECT *bindings)
    : _ctx(ctx)
    , rootScope(this, nullptr)
    , scope(&rootScope)
    , rootPointerNode(SourceSpan::invalid(), "<root>") {
    rootScope.parent = nullptr;
    loadBindings(bindings);
  }

  void visitNode(Node *n);

  void visitVarDecl(VarDecl *var);
  void visitFunctionExpr(FunctionExpr *f);
  void visitParamDecl(ParamDecl *p);
  void visitConstDecl(ConstDecl *c);
  void visitEnumDecl(EnumDecl *e);
  void visitClassExpr(ClassExpr *k);
  void visitTableExpr(TableExpr *t);

  void visitBlock(Block *block);
  void visitTryStatement(TryStatement *stmt);
  void visitForStatement(ForStatement *stmt);
  void visitForeachStatement(ForeachStatement *stmt);

  void analyze(RootBlock *root) {
    root->visit(this);
  }
};

}
