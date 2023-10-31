
#ifndef __DAGOR_SHSEM_H
#define __DAGOR_SHSEM_H
#pragma once

#include "shsyn.h"
#include "shVarBool.h"
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

using namespace ShaderTerminal;

class DefineTable;

namespace ShaderParser
{

enum
{
  HLSL_PS = 1,
  HLSL_VS = 2,
  HLSL_CS = 4,
  HLSL_DS = 8,
  HLSL_HS = 16,
  HLSL_GS = 32,
  HLSL_MS = 64,
  HLSL_AS = 128,
  HLSL_ALL = 0xFFFF
};

class ShaderEvalCB
{
public:
  struct GsclStopProcessingException
  {};

  enum
  {
    IF_FALSE,
    IF_TRUE,
    IF_BOTH
  };

  inline virtual DefineTable *getDefines() { return NULL; };

  virtual void eval_static(static_var_decl &) = 0;
  virtual void eval_interval_decl(interval &interv) = 0;
  virtual void eval_bool_decl(bool_decl &) = 0;
  virtual void eval_error_stat(error_stat &) {}
  virtual void eval_channel_decl(channel_decl &, int stream_id = 0) = 0;
  virtual void eval_state(state_stat &) = 0;
  virtual void eval_zbias_state(zbias_state_stat &) = 0;
  virtual void eval_external_block(external_state_block &) = 0;
  virtual void eval(immediate_const_block &) = 0;
  virtual void eval_supports(supports_stat &) = 0;
  virtual void eval_render_stage(render_stage_stat &) = 0;
  virtual void eval_assume_stat(assume_stat &) = 0;
  virtual void eval_command(shader_directive &) = 0;

  virtual int eval_if(bool_expr &) = 0;
  virtual void eval_else(bool_expr &) = 0;
  virtual void eval_endif(bool_expr &) = 0;

  virtual void eval_shader_locdecl(local_var_decl &s) = 0;

  virtual void eval_hlsl_compile(hlsl_compile_class &hlsl_compile) = 0;
  virtual void eval_hlsl_decl(hlsl_local_decl_class &hlsl_compile) = 0;
};

class ShaderBoolEvalCB
{
public:
  virtual ShVarBool eval_expr(bool_expr &) = 0;
  virtual ShVarBool eval_bool_value(bool_value &) = 0;
  virtual int eval_interval_value(const char *ival_name) = 0;
  virtual void decl_bool_alias(const char *name, bool_expr &expr) {}
  virtual int add_message(const char *message, bool file_name) { return 0; }
};

void eval_shader(shader_decl &sh, ShaderEvalCB &cb);

ShVarBool eval_shader_bool(bool_expr &, ShaderBoolEvalCB &cb);

void add_shader(shader_decl *, ShaderSyntaxParser &);
void add_block(block_decl *, ShaderSyntaxParser &);

// add a new global variable to a global variable list
void add_global_var(global_var_decl *decl, ShaderSyntaxParser &parser);

// add a new global variable interval
void add_global_interval(ShaderTerminal::interval &interv, ShaderTerminal::ShaderSyntaxParser &parser);

// add a global assume for interval
void add_global_assume(ShaderTerminal::assume_stat &assume, ShaderTerminal::ShaderSyntaxParser &parser);

// add a global bool var
void add_global_bool(ShaderTerminal::bool_decl &bool_var, ShaderTerminal::ShaderSyntaxParser &parser);

void add_hlsl(hlsl_global_decl_class &, ShaderSyntaxParser &);

void build_bool_expr_string(bool_expr &e, String &out, bool clr_str = true);

void addSourceCode(String &text, SHTOK_hlsl_text *source_text, ShaderSyntaxParser &parser);
void addSourceCode(String &text, Symbol *term, const char *source_text, ShaderSyntaxParser &parser);

bool_expr *parse_condition(const char *cond_str, int cond_str_len, const char *file_name, int line);
shader_stat *parse_shader_stat(const char *stat_str, int stat_str_len, IMemAlloc *alloc = nullptr);

void reset();
}; // namespace ShaderParser

// report error
void sh_report_error(ShaderTerminal::shader_stat *s, ShaderTerminal::ShaderSyntaxParser &parser);


#endif
