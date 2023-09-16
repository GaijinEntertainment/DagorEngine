#include "boolVar.h"
#include "globVarSem.h"
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

static eastl::unordered_map<eastl::string, BoolVar> g_bool_vars;
static int g_id = 0;

eastl::string build_name(const char *name_space, const char *name)
{
  return eastl::string(eastl::string::CtorSprintf{}, "%s/%s", name_space ? name_space : "", name);
}

static BoolVar *find(const char *name_space, const char *name)
{
  eastl::string n = build_name(name_space, name);
  auto found = g_bool_vars.find(n);
  if (found != g_bool_vars.end())
    return &found->second;

  if (name_space)
    return find(nullptr, name);
  return nullptr;
}

void BoolVar::add(const char *name_space, ShaderTerminal::bool_decl &var, ShaderTerminal::ShaderSyntaxParser &parser, bool ignore_dup)
{
  G_ASSERT(var.expr);
  eastl::string n = build_name(name_space, var.name->text);
  auto [it, inserted] = g_bool_vars.emplace(n, BoolVar(var.expr));
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

ShaderTerminal::bool_expr *BoolVar::get_expr(const char *name_space, ShaderTerminal::SHTOK_ident &ident,
  ShaderTerminal::ShaderSyntaxParser &parser)
{
  auto found = find(name_space, ident.text);
  if (!found)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "Bool variable '%s' not found", ident.text);
    ShaderParser::error(message.c_str(), &ident, parser);
    return nullptr;
  }
  return found->mExpr;
}

ShaderTerminal::bool_expr *BoolVar::maybe(const char *name_space, ShaderTerminal::SHTOK_ident &ident)
{
  auto found = find(name_space, ident.text);
  if (!found)
    return nullptr;
  return found->mExpr;
}

void BoolVar::clear()
{
  g_bool_vars.clear();
  g_id = 0;
}
