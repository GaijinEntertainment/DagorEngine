/************************************************************************
  global variables - semantic
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GLOBVARSEM_H
#define __GLOBVARSEM_H

#include "varTypes.h"
#include "parser/base_par.h"

class IntervalList;

namespace ShaderTerminal
{
struct global_var_decl;
struct default_shader_decl;
struct interval;
struct assume_stat;
struct bool_decl;

class ShaderSyntaxParser;
}; // namespace ShaderTerminal

namespace ShaderParser
{
void error(const char *msg, BaseParNamespace::Terminal *t, ShaderTerminal::ShaderSyntaxParser &parser);

// add a new global variable to a global variable list
void add_global_var(ShaderTerminal::global_var_decl *decl, ShaderTerminal::ShaderSyntaxParser &parser);

// add a new global variable interval
void add_global_interval(ShaderTerminal::interval &interv, ShaderTerminal::ShaderSyntaxParser &parser);

// add a new interval to intervals
void add_interval(IntervalList &intervals, ShaderTerminal::interval &interv, ShaderVariant::VarType type,
  ShaderTerminal::ShaderSyntaxParser &parser);

// add a global assume for interval
void add_global_assume(ShaderTerminal::assume_stat &assume, ShaderTerminal::ShaderSyntaxParser &parser);

// add a assume for interval to blk
void add_assume_to_blk(const char *block_name, const IntervalList &intervals, ShaderTerminal::assume_stat &assume,
  ShaderTerminal::ShaderSyntaxParser &parser);
}; // namespace ShaderParser


#endif //__GLOBVARSEM_H
