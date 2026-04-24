#include "compiler/ast.h"
#include "compiler/compilationcontext.h"
#include "name_shadowing_checker.h"
#include "symbol_info.h"

#include "sqtable.h"
#include "sqarray.h"
#include "sqclass.h"
#include "sqfuncproto.h"
#include "sqclosure.h"


namespace SQCompilation
{


void NameShadowingChecker::loadBindings(const HSQOBJECT *bindings) {
  if (bindings && sq_istable(*bindings)) {
    SQTable *table = _table(*bindings);

    SQInteger idx = 0;
    SQObjectPtr pos(idx), key, val;

    while ((idx = table->Next(false, pos, key, val)) >= 0) {
      if (sq_isstring(key)) {
        const char *s = _string(key)->_val;
        SymbolInfo *info = newSymbolInfo(SK_EXTERNAL_BINDING);
        declareSymbol(s, info);
      }
      pos._unVal.nInteger = idx;
    }
  }
}

const Node *NameShadowingChecker::extractPointedNode(const SymbolInfo *info) {
  switch (info->kind)
  {
  case SK_EXCEPTION:
    return info->declaration.x;
  case SK_FUNCTION:
    return info->declaration.f;
  case SK_CLASS:
    return info->declaration.k;
  case SK_TABLE:
    return info->declaration.t;
  case SK_VAR:
  case SK_BINDING:
  case SK_FOREACH:
    return info->declaration.v;
  case SK_CONST:
    return info->declaration.c;
  case SK_ENUM:
    return info->declaration.e;
  case SK_ENUM_CONST:
    return info->declaration.ec->val;
  case SK_PARAM:
    return info->declaration.p;
  case SK_EXTERNAL_BINDING:
    return &rootPointerNode;
  default:
    assert(0);
    return nullptr;
  }
}

void NameShadowingChecker::declareSymbol(const char *name, SymbolInfo *info) {
  const SymbolInfo *existedInfo = scope->findSymbol(name);
  if (existedInfo) {
    bool warn = name[0] != '_' && name[0] != '@';
    if (strcmp(name, "this") == 0) {
      warn = false;
    }

    if (existedInfo->ownerScope == info->ownerScope) { // something like `let foo = expr<function foo() { .. }>`
      if ((info->kind == SK_BINDING || info->kind == SK_VAR) && existedInfo->kind == SK_FUNCTION) {
        const VarDecl *vardecl = info->declaration.v;
        const FunctionExpr *funcdecl = existedInfo->declaration.f;
        const Expr *varinit = vardecl->initializer();

        if (varinit == funcdecl) {
          warn = false;
        }
      }
    }

    if (existedInfo->kind == SK_FUNCTION && info->kind == SK_FUNCTION) {
      const FunctionExpr *existed = existedInfo->declaration.f;
      const FunctionExpr *_new = info->declaration.f;
      if (existed->name()[0] == '(' && _new->name()[0] == '(') {
        warn = false;
      }
    }

    if (info->kind == SK_PARAM && existedInfo->kind == SK_PARAM && warn) {
      const ParamDecl *p1 = info->declaration.p;
      const ParamDecl *p2 = existedInfo->declaration.p;
      if (p1->isVararg() && p2->isVararg()) {
        warn = false;
      }
    }

    if (warn) {
      report(extractPointedNode(info), DiagnosticsId::DI_ID_HIDES_ID,
        symbolContextName(info->kind),
        info->name,
        symbolContextName(existedInfo->kind));
    }
  }

  scope->symbols[name] = info;
}

NameShadowingChecker::SymbolInfo *NameShadowingChecker::Scope::findSymbol(const char *name) const {
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    return it->second;
  }

  return parent ? parent->findSymbol(name) : nullptr;
}

void NameShadowingChecker::visitNode(Node *n) {
  nodeStack.push_back(n);
  n->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::declareVar(enum SymbolKind k, const VarDecl *var) {
  const FunctionExpr *f = nullptr;
  if (k == SK_BINDING) {
    const Expr *init = var->initializer();
    if (init && init->op() == TO_FUNCTION) {
      k = SK_FUNCTION;
      f = init->asFunctionExpr();
    }
  }


  SymbolInfo *info = newSymbolInfo(k);
  if (f) {
    info->declaration.f = f;
  }
  else {
    info->declaration.v = var;
  }
  info->ownerScope = scope;
  info->name = var->name();
  declareSymbol(var->name(), info);
}

void NameShadowingChecker::visitVarDecl(VarDecl *var) {
  Visitor::visitVarDecl(var);

  declareVar(var->isAssignable() ? SK_VAR : SK_BINDING, var);
}

void NameShadowingChecker::visitParamDecl(ParamDecl *p) {
  Visitor::visitParamDecl(p);

  SymbolInfo *info = newSymbolInfo(SK_PARAM);
  info->declaration.p = p;
  info->ownerScope = scope;
  info->name = p->name();
  declareSymbol(p->name(), info);
}

void NameShadowingChecker::visitConstDecl(ConstDecl *c) {
  SymbolInfo *info = newSymbolInfo(SK_CONST);
  info->declaration.c = c;
  info->ownerScope = scope;
  info->name = c->name();
  declareSymbol(c->name(), info);

  Visitor::visitConstDecl(c);
}

void NameShadowingChecker::visitEnumDecl(EnumDecl *e) {
  SymbolInfo *einfo = newSymbolInfo(SK_ENUM);
  einfo->declaration.e = e;
  einfo->ownerScope = scope;
  einfo->name = e->name();
  declareSymbol(e->name(), einfo);

  for (auto &ec : e->consts()) {
    SymbolInfo *cinfo = newSymbolInfo(SK_ENUM_CONST);
    // Are these fully qualified names ever used?
    const char *fqn = enumFqn(_ctx.arena(), e->name(), ec.id);

    cinfo->declaration.ec = &ec;
    cinfo->ownerScope = scope;
    cinfo->name = fqn;

    declareSymbol(fqn, cinfo);
  }
}

void NameShadowingChecker::visitFunctionExpr(FunctionExpr *f) {
  Scope *p = scope;

  Scope funcScope(this, f);

  bool tableScope = p->owner && (p->owner->op() == TO_CLASS || p->owner->op() == TO_TABLE);

  if (!tableScope) {
    SymbolInfo * info = newSymbolInfo(SK_FUNCTION);
    info->declaration.f = f;
    info->ownerScope = p;
    info->name = f->name();
    declareSymbol(f->name(), info);
  }

  Visitor::visitFunctionExpr(f);
}

void NameShadowingChecker::visitTableExpr(TableExpr *t) {
  Scope tableScope(this, t);

  nodeStack.push_back(t);
  t->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::visitClassExpr(ClassExpr *k) {
  Scope klassScope(this, k);

  nodeStack.push_back(k);
  k->visitChildren(this);
  nodeStack.pop_back();
}

void NameShadowingChecker::visitBlock(Block *block) {
  Scope blockScope(this, scope->owner);
  Visitor::visitBlock(block);
}

void NameShadowingChecker::visitTryStatement(TryStatement *stmt) {

  nodeStack.push_back(stmt);

  stmt->tryStatement()->visit(this);

  Scope catchScope(this, scope->owner);

  SymbolInfo *info = newSymbolInfo(SK_EXCEPTION);
  info->declaration.x = stmt->exceptionId();
  info->ownerScope = scope;
  info->name = stmt->exceptionId()->name();
  declareSymbol(stmt->exceptionId()->name(), info);

  stmt->catchStatement()->visit(this);

  nodeStack.pop_back();
}

void NameShadowingChecker::visitForStatement(ForStatement *stmt) {
  assert(scope);
  Scope forScope(this, scope->owner);

  Visitor::visitForStatement(stmt);
}

void NameShadowingChecker::visitForeachStatement(ForeachStatement *stmt) {

  assert(scope);
  Scope foreachScope(this, scope->owner);

  VarDecl *idx = stmt->idx();
  if (idx) {
    declareVar(SK_FOREACH, idx);
    assert(idx->initializer() == nullptr);
  }
  VarDecl *val = stmt->val();
  if (val) {
    declareVar(SK_FOREACH, val);
    assert(val->initializer() == nullptr);
  }

  nodeStack.push_back(stmt);

  stmt->container()->visit(this);
  stmt->body()->visit(this);

  nodeStack.pop_back();
}


}
