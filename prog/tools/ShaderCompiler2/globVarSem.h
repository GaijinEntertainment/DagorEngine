// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  global variables - semantic
/************************************************************************/

#include "varTypes.h"
#include "parser/base_par.h"

class IntervalList;

namespace ShaderTerminal
{
struct global_var_decl;
struct default_shader_decl;
struct interval;
struct assume_stat;
struct sampler_decl;

class ShaderSyntaxParser;
}; // namespace ShaderTerminal

namespace ShaderParser
{
void error(const char *msg, BaseParNamespace::Symbol *s, ShaderTerminal::ShaderSyntaxParser &parser);

// add a new global variable to a global variable list
void add_global_var(ShaderTerminal::global_var_decl *decl, ShaderTerminal::ShaderSyntaxParser &parser);

// add a new sampler object
void add_sampler(ShaderTerminal::sampler_decl *decl, ShaderTerminal::ShaderSyntaxParser &parser);

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
