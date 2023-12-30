#include "boolVar.h"
#include "globVarSem.h"
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

static eastl::unordered_map<eastl::string, BoolVar> g_global_bool_vars;
static eastl::unordered_map<eastl::string, BoolVar> g_local_bool_vars;
static int g_id = 0;

static BoolVar *find_bool_var(const char *name)
{
  auto localFound = g_local_bool_vars.find(name);
  if (localFound != g_local_bool_vars.end())
    return &localFound->second;
  auto globalFound = g_global_bool_vars.find(name);
  if (globalFound != g_global_bool_vars.end())
    return &globalFound->second;
  return nullptr;
}

void BoolVar::add(bool is_local, ShaderTerminal::bool_decl &var, ShaderTerminal::ShaderSyntaxParser &parser, bool ignore_dup)
{
  G_ASSERT(var.expr);
  auto &map = is_local ? g_local_bool_vars : g_global_bool_vars;
  auto [it, inserted] = map.emplace(var.name->text, BoolVar(var.expr));
  if (!inserted)
  {
    if (ignore_dup)
      return;
    eastl::string message(eastl::string::CtorSprintf{}, "Bool variable '%s' already declared in ", var.name->text);
    message += parser.get_lex_parser().get_symbol_location(it->second.id, SymbolType::BOOL_VARIABLE);
    ShaderParser::error(message.c_str(), var.name, parser);
    return;
  }

  it->second.id = g_id++;
  parser.get_lex_parser().register_symbol(it->second.id, SymbolType::BOOL_VARIABLE, var.name);
}

ShaderTerminal::bool_expr *BoolVar::get_expr(ShaderTerminal::SHTOK_ident &ident, ShaderTerminal::ShaderSyntaxParser &parser)
{
  auto found = find_bool_var(ident.text);
  if (!found)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "Bool variable '%s' not found", ident.text);
    ShaderParser::error(message.c_str(), &ident, parser);
    return nullptr;
  }
  return found->mExpr;
}

ShaderTerminal::bool_expr *BoolVar::maybe(ShaderTerminal::SHTOK_ident &ident)
{
  auto found = find_bool_var(ident.text);
  if (!found)
    return nullptr;
  return found->mExpr;
}

void BoolVar::clear(bool clear_only_local)
{
  g_local_bool_vars.clear();
  if (!clear_only_local)
  {
    g_global_bool_vars.clear();
    g_id = 0;
  }
}
