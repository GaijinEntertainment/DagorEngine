// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shlexterm.h"
#include "commonUtils.h"
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
  ska::flat_hash_map<eastl::string, Record> vars{};
  int id = 0;

public:
  explicit BoolVarTable(int base_id = 0) : id{base_id} {}

  struct QueryResult
  {
    ShaderTerminal::bool_expr *expr = nullptr;
    bool hasBeenDeclaredMultipleTimes = false;

    operator bool() const { return expr != nullptr; }
  };

  void add(ShaderTerminal::bool_decl &var, Parser &parser, bool ignore_dup = false);
  QueryResult tryGetExpr(ShaderTerminal::SHTOK_ident &ident) const;
  int maxId() const { return id; }


  template <class TCb>
  void iterateBoolVars(TCb &&cb) const
  {
    for (const auto &[name, rec] : vars)
      cb(name.c_str(), rec.var);
  }
};
