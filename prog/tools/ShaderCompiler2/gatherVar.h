// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  gather variants
/************************************************************************/
#ifndef __GATHERVAR_H
#define __GATHERVAR_H

#include <EASTL/bitvector.h>

#include "shsem.h"
#include "shlexterm.h"
#include "shaderVariant.h"
#include "shShaderContext.h"
#include "shVarBool.h"
#include "nameMap.h"
#include "hlslStage.h"

/************************************************************************/
/* forwards
/************************************************************************/
class CodeSourceBlocks;


namespace ShaderParser
{
/*********************************
 *
 * class GatherVarShaderEvalCB
 *
 *********************************/
class GatherVarShaderEvalCB : public ShaderEvalCB, public ShaderBoolEvalCB
{
public:
  explicit GatherVarShaderEvalCB(shc::ShaderContext &ctx);

  void eval_interval_decl(interval &interv) override;

  void eval_static(static_var_decl &s) override;
  void eval_bool_decl(bool_decl &) override;
  void decl_bool_alias(const char *name, bool_expr &expr) override;

  void eval_channel_decl(channel_decl &s, int str_id = 0) override;
  void eval_state(state_stat &) override {}
  void eval_zbias_state(zbias_state_stat &) override {}
  void eval_external_block(external_state_block &) override;
  void eval_assume_stat(assume_stat &s) override;
  void eval_assume_if_not_assumed_stat(assume_if_not_assumed_stat &s) override;
  void eval_render_stage(render_stage_stat &) override {}
  void eval_command(shader_directive &) override {}
  void eval_supports(supports_stat &) override;

  void eval_shader_locdecl(local_var_decl &s) override {}

  int eval_if(bool_expr &e) override;

  void eval_else(bool_expr &) override;
  void eval_endif(bool_expr &) override;

  ShVarBool eval_expr(bool_expr &e) override;
  ShVarBool eval_bool_value(bool_value &e) override;
  int eval_interval_value(const char *ival_name) override;

  void eval_hlsl_compile(hlsl_compile_class &hlsl_compile) override {}
  void eval_hlsl_decl(hlsl_local_decl_class &hlsl_compile) override;
  void eval(immediate_const_block &) override;

private:
  shc::ShaderContext &ctx;

  // Cached refs from ctx
  shc::TypeTables &types;
  Parser &parser;

  // Internal pass state
  Tab<bool> dynStack;
  int dynCount;
  bool hasDynFlag;

  SCFastNameMap staticVars, dynamicVars;

  ShVarBool addInterval(const char *intervalName, Terminal *terminal, bool_value *e);

  ShVarBool addTextureInterval(const char *textureName, Terminal *terminal, bool_value &e);
};
} // namespace ShaderParser


#endif //__GATHERVAR_H
