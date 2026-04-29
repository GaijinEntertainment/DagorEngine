#include <assert.h>
#include <vector>
#include <algorithm>

#include "var_scope.h"
#include "value_ref.h"
#include "symbol_info.h"
#include "naming.h"
#include "checker_visitor.h"
#include "compiler/compilationcontext.h"
#include "compiler/sourceloc.h"

namespace SQCompilation
{


void VarScope::copyFrom(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->owner == r->owner && "Scope corruption");

    auto &thisSymbols = l->symbols;
    auto &otherSymbols = r->symbols;
    auto it = otherSymbols.begin();
    auto ie = otherSymbols.end();

    while (it != ie) {
      thisSymbols[it->first] = it->second;
      ++it;
    }

    l = l->parent;
    r = r->parent;
  }
}

void VarScope::intersectScopes(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->owner == r->owner && "Scope corruption");

    auto &thisSymbols = l->symbols;
    auto &otherSymbols = r->symbols;
    auto it = otherSymbols.begin();
    auto ie = otherSymbols.end();
    auto te = thisSymbols.end();

    while (it != ie) {
      auto f = thisSymbols.find(it->first);
      if (f != te) {
        if (it->second->info == f->second->info)
          f->second->intersectValue(it->second);
      }
      ++it;
    }

    l = l->parent;
    r = r->parent;
  }
  evalId = std::max(evalId, other->evalId) + 1; // -V522
}


void VarScope::mergeUnbalanced(const VarScope *other) {
  VarScope *lhs = this;
  const VarScope *rhs = other;

  while (lhs->depth > rhs->depth) {
    lhs = lhs->parent;
  }

  while (rhs->depth > lhs->depth) {
    rhs = rhs->parent;
  }

  lhs->merge(rhs);
}

void VarScope::merge(const VarScope *other) {
  VarScope *l = this;
  const VarScope *r = other;

  while (l) {
    assert(l->depth == r->depth && "Scope corruption");
    assert(l->owner == r->owner && "Scope corruption");

    auto &originSymbols = l->symbols;
    const auto &branchSymbols = r->symbols;

    for (const auto &kv : branchSymbols) {
      auto it = originSymbols.find(kv.first);
      if (it != originSymbols.end()) {
        // Lambdas declared on the same line could have same names
        if (it->second->info == kv.second->info) {
          it->second->merge(kv.second);
        }
      }
      // Otherwise (would be under `else`) the symbol only exists in branch
      // (e.g., lambda created in ternary) - it's local to that branch,
      // don't propagate to origin scope
    }

    l = l->parent;
    r = r->parent;
  }

  evalId = std::max(evalId, other->evalId) + 1;
}

VarScope *VarScope::findScope(const FunctionExpr *own) {
  VarScope *s = this;

  while (s) {
    if (s->owner == own) {
      return s;
    }
    s = s->parent;
  }

  return nullptr;
}

VarScope *VarScope::copy(Arena *a, bool forClosure) const {
  VarScope *parentCopy = parent ? parent->copy(a, forClosure) : nullptr;
  void *mem = a->allocate(sizeof(VarScope));
  VarScope *thisCopy = new(mem) VarScope(owner, parentCopy);

  for (auto &kv : symbols) {
    const char *k = kv.first;
    ValueRef *v = kv.second;
    void *mem = a->allocate(sizeof(ValueRef));
    ValueRef *vcopy = new(mem) ValueRef(v->info, v->evalIndex);

    if (!v->isConstant() && forClosure) {
      // if we analyze closure we cannot rely on existed assignable values
      vcopy->state = VRS_UNKNOWN;
      vcopy->expression = nullptr;
      vcopy->flagsNegative = vcopy->flagsPositive = 0;
      vcopy->assigned = v->assigned;
      vcopy->lastAssigneeScope = v->lastAssigneeScope;
    }
    else {
      memcpy(vcopy, v, sizeof(ValueRef));
      vcopy->assigned = false;
      vcopy->lastAssigneeScope = nullptr;
    }
    thisCopy->symbols[k] = vcopy;
  }

  return thisCopy;
}


static SourceLoc getSymbolLocation(const SymbolInfo *info) {
  if (info->kind == SK_IMPORT) {
    const ImportInfo *imp = info->declarator.imp;
    return {imp->line, imp->column};
  }
  const Node *node = info->extractPointedNode();
  return {node->lineStart(), node->columnStart()}; //-V522
}

void VarScope::checkUnusedSymbols(CheckerVisitor *checker) {
  std::vector<std::pair<const char *, ValueRef *>> sorted(symbols.begin(), symbols.end());
  std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
    SourceLoc locA = getSymbolLocation(a.second->info);
    SourceLoc locB = getSymbolLocation(b.second->info);
    if (locA.line != locB.line)
      return locA.line < locB.line;
    return locA.column < locB.column;
  });

  for (auto &s : sorted) {
    const char *n = s.first;
    const ValueRef *v = s.second;

    if (strcmp(n, "this") == 0 || n[0] == '@')
      continue;

    SymbolInfo *info = v->info;

    if (info->kind == SK_ENUM_CONST)
      continue;

    if (!info->used && n[0] != '_') {
      if (info->kind == SK_IMPORT) {
        const ImportInfo *import = info->declarator.imp;
        checker->reportImportSlot(import->line, import->column, import->name);
      }
      else {
        checker->report(info->extractPointedNode(), DiagnosticsId::DI_DECLARED_NEVER_USED, info->contextName(), n);
        // TODO: add hint for param/exception name about silencing it with '_' prefix
      }
    }
    else if (info->used && n[0] == '_') {
      if (info->kind == SK_PARAM || info->kind == SK_FOREACH)
        checker->report(info->extractPointedNode(), DiagnosticsId::DI_INVALID_UNDERSCORE, info->contextName(), n);
    }
  }
}

}
