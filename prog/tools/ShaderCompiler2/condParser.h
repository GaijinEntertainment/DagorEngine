// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shVarBool.h"

namespace ShaderTerminal
{
struct bool_expr;
struct bool_value;
} // namespace ShaderTerminal


ShaderTerminal::bool_expr *parse_pp_condition(const char *stage, char *str, int len, const char *fname, int line);
const char *mangle_bool_var(const char *stage, const char *name);
