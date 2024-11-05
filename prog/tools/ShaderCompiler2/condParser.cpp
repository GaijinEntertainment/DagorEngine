// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shVarBool.h"
#include "shsem.h"
#include "shLog.h"

static void patch_cond_tokens(const char *stage, ShaderTerminal::bool_expr &e);

bool_expr *parse_pp_condition(const char *stage, char *str, int len, const char *fname, int line)
{
  bool_expr *s = ShaderParser::parse_condition(str, len, fname, line);
  if (!s)
    return nullptr;

  if (stage)
    patch_cond_tokens(stage, *s);
  return s;
}

const char *mangle_bool_var(const char *stage, const char *name)
{
  String new_ident(0, "__hlsl_%s_%s", stage, name);
  return str_dup(new_ident, BaseParNamespace::symbolsmem);
}

static void patch_cond_tokens_value(const char *stage, ShaderTerminal::bool_value &e)
{
  if (e.bool_var)
    e.bool_var->text = mangle_bool_var(stage, e.bool_var->text);

  if (e.maybe_bool_var)
    e.maybe_bool_var->text = mangle_bool_var(stage, e.maybe_bool_var->text);
}
static void patch_cond_tokens_bool_not(const char *stage, ShaderTerminal::not_expr &e)
{
  if (e.value->expr)
    patch_cond_tokens(stage, *e.value->expr);
  else
    patch_cond_tokens_value(stage, *e.value);
}
static void patch_cond_tokens_bool_and(const char *stage, ShaderTerminal::and_expr &e)
{
  if (e.value)
  {
    patch_cond_tokens_bool_not(stage, *e.value);
    return;
  }

  patch_cond_tokens_bool_and(stage, *e.a);
  patch_cond_tokens_bool_not(stage, *e.b);
}

static void patch_cond_tokens(const char *stage, ShaderTerminal::bool_expr &e)
{
  if (e.value)
  {
    patch_cond_tokens_bool_and(stage, *e.value);
    return;
  }

  patch_cond_tokens(stage, *e.a);
  patch_cond_tokens_bool_and(stage, *e.b);
}
