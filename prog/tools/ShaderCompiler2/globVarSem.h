// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  global variables - semantic
/************************************************************************/

#include "varTypes.h"
#include "shTargetContext.h"
#include "shShaderContext.h"
#include "parser/base_par.h"

class IntervalList;

namespace ShaderTerminal
{
struct global_var_decl;
struct default_shader_decl;
struct interval;
struct assume_stat;
struct assume_if_not_assumed_stat;
struct sampler_decl;
}; // namespace ShaderTerminal

struct Parser;

namespace ShaderParser
{
// add a new global variable to a global variable list
void add_global_var(ShaderTerminal::global_var_decl *decl, Parser &parser, shc::TargetContext &ctx);

// add a new sampler object
void add_sampler(ShaderTerminal::sampler_decl *decl, Parser &parser, shc::TargetContext &ctx);

// add a new global variable interval
void add_global_interval(ShaderTerminal::interval &interv, Parser &parser, shc::TargetContext &ctx);

// add a new interval to intervals
void add_interval(IntervalList &intervals, ShaderTerminal::interval &interv, ShaderVariant::VarType type, Parser &parser,
  shc::TargetContext &ctx);

// add a global assume for interval
void add_global_assume(ShaderTerminal::assume_stat &assume, Parser &parser, shc::TargetContext &ctx);

// add a global assume for interval if it was not already assumed
void add_global_assume_if_not_assumed(ShaderTerminal::assume_if_not_assumed_stat &assume, Parser &parser, shc::TargetContext &ctx);

// add a assume for interval to blk
void add_shader_assume(ShaderTerminal::assume_stat &assume, Parser &parser, shc::ShaderContext &ctx);
// add a assume for interval to blk if it was not already assumed
void add_shader_assume_if_not_assumed(ShaderTerminal::assume_if_not_assumed_stat &assume, Parser &parser, shc::ShaderContext &ctx);
}; // namespace ShaderParser
