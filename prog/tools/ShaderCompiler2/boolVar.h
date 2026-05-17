// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shlexterm.h"
#include "commonUtils.h"
#include "varMap.h"
#include <ska_hash_map/flat_hash_map2.hpp>

class BoolVarTable;

class BoolVar
{
  ShaderTerminal::bool_expr *mExpr = nullptr;
  int id = 0;

  friend class BoolVarTable;

  BoolVar(ShaderTerminal::bool_expr *expr) : mExpr(expr) {}

public:
  COPYABLE_TYPE(BoolVar)
};

class BoolVarTable
{
public:
  struct Record
  {
    BoolVar var;
    int declCount = 0;
  };

private:
  ska::flat_hash_map<int, Record> vars{};
  VarNameMap *nameMap;
  BoolVarTable *parent;
  int id = 0;

public:
  explicit BoolVarTable(VarNameMap &name_map, BoolVarTable *parent = nullptr) :
    nameMap{&name_map}, parent{parent}, id{parent ? parent->maxId() : 0}
  {
    if (parent)
      G_ASSERT(parent->nameMap == nameMap);
  }

  struct QueryResult
  {
    ShaderTerminal::bool_expr *expr = nullptr;
    bool hasBeenDeclaredMultipleTimes = false;

    operator bool() const { return expr != nullptr; }
  };

  void add(int nid, ShaderTerminal::bool_expr *expr, Parser &parser, Terminal *t, bool ignore_dup = false);
  void addAlias(int nid, int base_nid, Parser &parser, bool ignore_dup = false);
  QueryResult tryGetExpr(int nid) const;

  void add(ShaderTerminal::bool_decl &var, Parser &parser, bool ignore_dup = false)
  {
    add(nameMap->addVarId(var.name->text), var.expr, parser, var.name, ignore_dup);
  }
  void addAlias(const char *name, const char *base_name, Parser &parser, bool ignore_dup = false)
  {
    addAlias(nameMap->addVarId(name), nameMap->getVarId(base_name), parser, ignore_dup);
  }
  QueryResult tryGetExpr(ShaderTerminal::SHTOK_ident &ident) const
  {
    if (int nid = nameMap->getVarId(ident.text); nid >= 0)
      return tryGetExpr(nid);
    else
      return {};
  }

  const VarNameMap *getNameMap() const { return nameMap; }
  const BoolVarTable *getParent() const { return parent; }
  int maxId() const { return id; }

  template <class TCb>
  void iterateBoolVars(TCb &&cb) const
  {
    for (const auto &[nameId, rec] : vars)
      cb(nameId, rec.var);
  }
};
