// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  gather variants
/************************************************************************/
#ifndef __GATHERVAR_H
#define __GATHERVAR_H

#include <EASTL/bitvector.h>

#include "shsem.h"
#include "shaderVariant.h"
#include "shVarBool.h"
#include "nameMap.h"

/************************************************************************/
/* forwards
/************************************************************************/
struct ShHardwareOptions;
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
  ShaderVariant::TypeTable varTypes;
  ShaderVariant::TypeTable dynVarTypes;
  ShaderVariant::TypeTable allRefStaticVars;
  eastl::vector<eastl::pair<eastl::string, uint8_t>> assumedIntervals;
  IntervalList intervals;
  ShaderSyntaxParser &parser;
  const ShHardwareOptions &opt;

  OAHashNameMap<false> staticVars, dynamicVars;

  Terminal *shname_token;
  bool shaderDebugModeEnabled = false;


  GatherVarShaderEvalCB(ShaderSyntaxParser &_parser, const ShHardwareOptions &_opt, Terminal *shname, const char *hlsl_vs,
    const char *hlsl_hs, const char *hlsl_ds, const char *hlsl_gs, const char *hlsl_ps, const char *hlsl_cs, const char *hlsl_ms,
    const char *hlsl_as, bool &out_shaderDebugModeEnabled);

  virtual void eval_interval_decl(interval &interv);

  virtual void eval_static(static_var_decl &s);
  void eval_bool_decl(bool_decl &) override;
  void decl_bool_alias(const char *name, bool_expr &expr) override;
  int add_message(const char *message, bool file_name) override;
  int is_debug_mode_enabled() override { return shaderDebugModeEnabled; }
  bool is_filename_message(int id) const { return !nonFilenameMessages.test(id, false); }
  const SCFastNameMap &get_messages() const { return messages; }

  void eval_channel_decl(channel_decl &s, int str_id = 0);
  void eval_state(state_stat &) {}
  void eval_zbias_state(zbias_state_stat &) {}
  void eval_external_block(external_state_block &) override;
  void eval_assume_stat(assume_stat &s) override;
  void eval_render_stage(render_stage_stat &) {}
  void eval_command(shader_directive &) {}
  void eval_supports(supports_stat &);

  virtual void eval_shader_locdecl(local_var_decl &s) {}

  int eval_if(bool_expr &e);

  void eval_else(bool_expr &);
  void eval_endif(bool_expr &);

  virtual ShVarBool eval_expr(bool_expr &e);
  virtual ShVarBool eval_bool_value(bool_value &e);
  virtual int eval_interval_value(const char *ival_name);

  void error(const char *msg, Symbol *s);
  void warning(const char *msg, Symbol *s);

  virtual void eval_hlsl_compile(hlsl_compile_class &hlsl_compile) {}
  virtual void eval_hlsl_decl(hlsl_local_decl_class &hlsl_compile);
  virtual void eval(immediate_const_block &) override;

  bool end_eval(CodeSourceBlocks &vs, CodeSourceBlocks &hs, CodeSourceBlocks &ds, CodeSourceBlocks &gs, CodeSourceBlocks &ps,
    CodeSourceBlocks &cs, CodeSourceBlocks &ms, CodeSourceBlocks &as);

private:
  Tab<bool> hwStack; // check variables for hardware dependens
  int hwCount;
  bool hasHWFlag; // true, if last boolean expression has hardware-depended flag

  Tab<bool> dynStack;
  int dynCount;
  bool hasDynFlag;
  SCFastNameMap messages;
  eastl::bitvector<> nonFilenameMessages;

  String hlslPs, hlslVs, hlslHs, hlslDs, hlslGs, hlslCs, hlslMs, hlslAs;

  ShVarBool addInterval(const char *intervalName, Terminal *terminal, bool_value *e);

  ShVarBool addTextureInterval(const char *textureName, Terminal *terminal, bool_value &e);
};
} // namespace ShaderParser


#endif //__GATHERVAR_H
