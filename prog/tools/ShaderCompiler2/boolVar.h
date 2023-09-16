#pragma once

#include "shsyn.h"

class BoolVar
{
  ShaderTerminal::bool_expr *mExpr = nullptr;
  int id = 0;

  BoolVar(ShaderTerminal::bool_expr *expr) : mExpr(expr) {}

public:
  BoolVar &operator=(const BoolVar &) = default;
  BoolVar(const BoolVar &) = default;

  static void add(const char *name_space, ShaderTerminal::bool_decl &var, ShaderTerminal::ShaderSyntaxParser &parser,
    bool ignore_dup = false);
  static ShaderTerminal::bool_expr *get_expr(const char *name_space, ShaderTerminal::SHTOK_ident &ident,
    ShaderTerminal::ShaderSyntaxParser &parser);
  static ShaderTerminal::bool_expr *maybe(const char *name_space, ShaderTerminal::SHTOK_ident &ident);
  static void clear();
};
