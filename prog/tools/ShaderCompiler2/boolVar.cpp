// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "boolVar.h"
#include "globVarSem.h"
#include "commonUtils.h"
#include "shErrorReporting.h"
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

static const BoolVarTable::Record *find_bool_var(const char *name, const auto &table)
{
  auto found = table.find(name);
  if (found != table.end())
    return &found->second;
  return nullptr;
}

void BoolVarTable::add(ShaderTerminal::bool_decl &var, Parser &parser, bool ignore_dup)
{
  G_ASSERT(var.expr);
  auto [it, inserted] = vars.emplace(var.name->text, Record{BoolVar(var.expr), 1});
  if (!inserted)
  {
    if (ignore_dup)
    {
      report_debug_message(parser, *var.name,
        "Variant-dependent redeclaration of bool var '%s'. If this is not your intention, please refactor the usage, as some passes "
        "are required to evaluate such bool vars as depending on dynamic variants.",
        var.name->text);
      ++it->second.declCount;
      return;
    }
    auto message = string_f("Bool variable '%s' already declared in ", var.name->text);
    message += parser.get_lexer().get_symbol_location(it->second.var.id, SymbolType::BOOL_VARIABLE);
    report_error(parser, var.name, message.c_str());
    return;
  }

  it->second.var.id = id++;
  parser.get_lexer().register_symbol(it->second.var.id, SymbolType::BOOL_VARIABLE, var.name);
}

BoolVarTable::QueryResult BoolVarTable::tryGetExpr(ShaderTerminal::SHTOK_ident &ident) const
{
  auto found = find_bool_var(ident.text, vars);
  return found ? QueryResult{found->var.mExpr, found->declCount > 1} : QueryResult{};
}