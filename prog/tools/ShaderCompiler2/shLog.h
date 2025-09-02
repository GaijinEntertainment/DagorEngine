// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader's log-file
/************************************************************************/
#ifndef __SHLOG_H
#define __SHLOG_H

#include <util/dag_string.h>
#include <cstdio>

/************************************************************************/
/* forwards
/************************************************************************/
class IVariantInfoProvider
{
public:
  IVariantInfoProvider() {}
  virtual ~IVariantInfoProvider() {}

  // return string info about variant
  virtual String getVarStringInfo() const = 0;
};

/************************************************************************/
/* shader message mode
/************************************************************************/
enum ShLogMode
{
  SHLOG_NORMAL,
  SHLOG_INFO,
  SHLOG_WARNING,
  SHLOG_ERROR,
  SHLOG_FATAL,
  __SHLOG_MODE_COUNT,
};

struct ErrorCounter
{
  int err;
  int warn;

  inline ErrorCounter(const char *_prefix) : prefix(_prefix) { reset(); };
  inline void reset() { err = warn = 0; };
  inline bool isEmpty() const { return (err == 0) && (warn == 0); };
  void dumpInfo();

  static ErrorCounter &curShader();
  static ErrorCounter &allShaders();
  static ErrorCounter &allCompilations();

private:
  const char *prefix;
};

// Function for correct output
#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
DECLARE_DSA_OVERLOADS_FAMILY(static inline void sh_printf, void sh_printf, sh_printf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL FILE *file,
#define DSA_OVERLOADS_PARAM_PASS file,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void sh_fprintf, void sh_fprintf, sh_fprintf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

// output to shader log
#define DSA_OVERLOADS_PARAM_DECL ShLogMode mode,
#define DSA_OVERLOADS_PARAM_PASS mode,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void sh_debug, void sh_debug, sh_debug);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

// close shader debug file
void sh_close_debug();

// if has errors, terminate program
void sh_process_errors();

// set current variant to output
void sh_set_current_shader(const char *shader);
void sh_set_current_variant(const IVariantInfoProvider *var);
void sh_set_current_dyn_variant(const IVariantInfoProvider *var);
String sh_get_compile_context();

// enable output to console
void enable_sh_debug_con(bool e);
void sh_console_quet_output(bool e);
void sh_console_print_warnings(bool e);

void sh_dump_warn_info();
void sh_change_mode(ShLogMode mode, ShLogMode mode_other);

void sh_enter_atomic_debug();
void sh_leave_atomic_debug();

#endif //__SHLOG_H
