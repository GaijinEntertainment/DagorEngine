#pragma once

#include "compiler/ast.h"


namespace SQCompilation
{

struct ImportInfo {
  int line;
  int column;
  const char *name;
};

enum SymbolKind {
  SK_EXCEPTION,
  SK_FUNCTION,
  SK_CLASS,
  SK_TABLE,
  SK_VAR,
  SK_BINDING,
  SK_CONST,
  SK_ENUM,
  SK_ENUM_CONST,
  SK_PARAM,
  SK_FOREACH,
  SK_EXTERNAL_BINDING,
  SK_IMPORT
};

const char *symbolContextName(SymbolKind k);


struct SymbolInfo {
  union {
    const Id *x;
    const FunctionExpr *f;
    const ClassExpr *k;
    const TableMember *m;
    const VarDecl *v;
    const TableExpr *t;
    const ParamDecl *p;
    const EnumDecl *e;
    const ConstDecl *c;
    const EnumConst *ec;
    const ExternalValueExpr *ev;
    const ImportInfo *imp;
  } declarator;

  SymbolKind kind;

  bool declared;
  bool used;
  bool usedAfterAssign;

  const struct VarScope *ownedScope;

  SymbolInfo(SymbolKind k) : kind(k), declarator() {
    declared = true;
    used = usedAfterAssign = false;
    ownedScope = nullptr;
  }

  bool isConstant() const {
    return kind != SK_VAR;
  }

  const Node *extractPointedNode() const {
    switch (kind)
    {
    case SK_EXCEPTION:
      return declarator.x;
    case SK_FUNCTION:
      // Return the Id node if available (points to identifier for diagnostics)
      if (declarator.f && declarator.f->nameId())
        return declarator.f->nameId();
      return declarator.f;
    case SK_CLASS:
      return declarator.k;
    case SK_TABLE:
      return declarator.t;
    case SK_VAR:
    case SK_BINDING:
    case SK_FOREACH:
      // Return the Id node if available (points to identifier for diagnostics)
      if (declarator.v && declarator.v->nameId())
        return declarator.v->nameId();
      return declarator.v;
    case SK_CONST:
      return declarator.c;
    case SK_ENUM:
      return declarator.e;
    case SK_ENUM_CONST:
      return declarator.ec->val;
    case SK_PARAM:
      return declarator.p;
    case SK_EXTERNAL_BINDING:
      return declarator.ev;
    case SK_IMPORT:
      return nullptr; // Import slots don't have a Node representation
    default:
      assert(0);
      return nullptr;
    }
  }

  const char *contextName() const {
    return symbolContextName(kind);
  }
};

}
