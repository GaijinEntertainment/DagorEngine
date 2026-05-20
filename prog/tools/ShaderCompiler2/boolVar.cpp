// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "boolVar.h"
#include "globVarSem.h"
#include "commonUtils.h"
#include "shErrorReporting.h"
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

static const BoolVarTable::Record *find_bool_var(int nid, const auto &table)
{
  auto found = table.find(nid);
  if (found != table.end())
    return &found->second;
  return nullptr;
}

void BoolVarTable::add(int nid, ShaderTerminal::bool_expr *expr, Parser &parser, Terminal *t, bool ignore_dup)
{
  G_ASSERT(nid >= 0);
  G_ASSERT(expr);
  auto [it, inserted] = vars.emplace(nid, Record{BoolVar(expr), 1});
  if (!inserted)
  {
    if (ignore_dup)
    {
      report_debug_message(parser, *t,
        "Variant-dependent redeclaration of bool var '%s'. If this is not your intention, please refactor the usage, as some passes "
        "are required to evaluate such bool vars as depending on dynamic variants.",
        nameMap->getName(nid));
      ++it->second.declCount;
      return;
    }
    auto message = string_f("Bool variable '%s' already declared in ", nameMap->getName(nid));
    message += parser.get_lexer().get_symbol_location(it->second.var.id, SymbolType::BOOL_VARIABLE);
    report_error(parser, t, message.c_str());
    return;
  }

  it->second.var.id = id++;

  // @TODO: I don't like this excessive map touching. Better slap it in the structure.
  parser.get_lexer().register_symbol(it->second.var.id, SymbolType::BOOL_VARIABLE, t);
}

void BoolVarTable::addAlias(int nid, int base_nid, Parser &parser, bool ignore_dup)
{
  ShaderTerminal::bool_expr *e = nullptr;
  int declCount = 0;

  {
    auto baseIt = vars.find(base_nid);
    if (baseIt == vars.end())
    {
      if (parent && (baseIt = parent->vars.find(base_nid)) != parent->vars.end())
      {
        ;
      }
      else
      {
        report_error(parser, nullptr, "Bool variable '%s' not found for alias '%s'", nameMap->getName(base_nid),
          nameMap->getName(nid));
        return;
      }
    }
    e = baseIt->second.var.mExpr;
    declCount = baseIt->second.declCount; // if the base var has been redeclared, the alias should also carry it's ambiguity.
  }

  auto [it, inserted] = vars.emplace(nid, Record{BoolVar(e), declCount});
  if (!inserted)
  {
    if (ignore_dup)
    {
      report_debug_message(parser, {},
        "Variant-dependent redeclaration of bool alias '%s'. If this is not your intention, please refactor the usage, as some passes "
        "are required to evaluate such bool vars as depending on dynamic variants.",
        nameMap->getName(nid));
      ++it->second.declCount;
      return;
    }

    report_error(parser, nullptr, "Bool variable '%s' already declared in ", nameMap->getName(nid));
    return;
  }
}

BoolVarTable::QueryResult BoolVarTable::tryGetExpr(int nid) const
{
  auto *found = find_bool_var(nid, vars);
  return found ? QueryResult{found->var.mExpr, found->declCount > 1} : QueryResult{};
}
