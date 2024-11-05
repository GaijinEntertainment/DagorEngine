// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdarg.h>
#include <util/dag_string.h>
#include <stdio.h>

#include "shLog.h"
#include "shCompiler.h"
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_debug.h>

#include <libTools/util/atomicPrintf.h>
AtomicPrintfMutex AtomicPrintfMutex::inst;

ErrorCounter &ErrorCounter::curShader()
{
  static ErrorCounter ec("In current shader %s\n");
  return ec;
}


ErrorCounter &ErrorCounter::allShaders()
{
  static ErrorCounter ec("%s");
  return ec;
}


ErrorCounter &ErrorCounter::allCompilations()
{
  static ErrorCounter ec("Total %s\n");
  return ec;
}

void ErrorCounter::dumpInfo()
{
  String msg;
  msg.printf(2048, "%d error(s), %d warning(s)", err, warn);

  sh_debug(SHLOG_INFO, (char *)prefix, (char *)msg);
}


static ShLogMode realMode[__SHLOG_MODE_COUNT];
static WinCritSec atomicDebug;

void sh_enter_atomic_debug() { atomicDebug.lock(); }
void sh_leave_atomic_debug() { atomicDebug.unlock(); }

static class realMode_Constructor
{
public:
  realMode_Constructor()
  {
    for (int i = 0; i < __SHLOG_MODE_COUNT; i++)
      realMode[i] = (ShLogMode)i;
  }
} __realMode_Constructor;


void sh_change_mode(ShLogMode mode, ShLogMode mode_other) { realMode[mode] = mode_other; }


// limit of errors
static const int fatalErrorCount = 100;

static bool processError = true;
static IVariantInfoProvider *current_variant = NULL;
static IVariantInfoProvider *prev_variant = NULL;
static IVariantInfoProvider *current_dyn_variant = NULL;
static IVariantInfoProvider *prev_dyn_variant = NULL;
static const char *current_shader = NULL;
static bool usePrintf = false;
static bool quetConsole = false;
static bool printWarnings = false;
static bool lockShaderOutput = false;

// enable output to console
void enable_sh_debug_con(bool e) { usePrintf = e; }

String sh_get_compile_context()
{
  return String(0, "== S.variant: shader=%s %s\n = D.variant: %s", current_shader ? current_shader : "?",
    current_variant ? current_variant->getVarStringInfo().str() : "?",
    current_dyn_variant ? current_dyn_variant->getVarStringInfo().str() : "?");
}

// output to shader log
void sh_debug(ShLogMode mode, const char *fmt, const DagorSafeArg *arg, int anum)
{
  // change mode, if needed
  mode = realMode[mode];

  if (lockShaderOutput)
    return;

  bool dup_stdout = usePrintf && (!quetConsole || mode == SHLOG_FATAL || mode == SHLOG_ERROR || mode == SHLOG_NORMAL ||
                                   (mode == SHLOG_WARNING && printWarnings));

  bool st_var_changed = prev_variant != current_variant;
  if (st_var_changed)
  {
    prev_variant = current_variant;
    if (current_variant)
    {
      String s = current_variant->getVarStringInfo();
      debug("\n== S.variant: %s", (char *)s);
      if (dup_stdout && !lockShaderOutput)
        printf("\n== S.variant: shader=%s %s\n", current_shader, (char *)s);
    }
  }

  if (prev_dyn_variant != current_dyn_variant || st_var_changed)
  {
    prev_dyn_variant = current_dyn_variant;
    if (current_dyn_variant)
    {
      String s = current_dyn_variant->getVarStringInfo();
      debug(" = D.variant: %s", (char *)s);
      if (dup_stdout && !lockShaderOutput)
        printf(" = D.variant: %s\n", (char *)s);
    }
  }

  String st;

  switch (mode)
  {
    case SHLOG_NORMAL: st = fmt; break;
    case SHLOG_INFO: st = String("[INFO] ") + fmt; break;
    case SHLOG_WARNING: st = String("[WARN] ") + fmt; break;
    case SHLOG_SILENT_WARNING: return;
    case SHLOG_ERROR: st = String("[ERROR] ") + fmt; break;
    case SHLOG_FATAL: st = String("\n[FATAL ERROR] ") + fmt; break;
  }

  logmessage_fmt(LOGLEVEL_DEBUG, st, arg, anum);


  if (dup_stdout)
  {
    st += "\n";
    AtomicPrintfMutex::inst.lock();
    DagorSafeArg::fprint_fmt(stdout, st, arg, anum);
    AtomicPrintfMutex::inst.unlock(stdout);
  }


  if (processError)
  {
    bool needStop = false;
    bool dv = false;
    bool de = false;
    switch (mode)
    {
      case SHLOG_WARNING: dv = true; break;
      case SHLOG_FATAL: needStop = true;
      case SHLOG_ERROR: de = true; break;
    }

    if (dv)
    {
      ErrorCounter::curShader().warn++;
      ErrorCounter::allShaders().warn++;
      ErrorCounter::allCompilations().warn++;
    }

    if (de)
    {
      ErrorCounter::curShader().err++;
      ErrorCounter::allShaders().err++;
      ErrorCounter::allCompilations().err++;
    }

    if (needStop || (ErrorCounter::allShaders().err >= fatalErrorCount))
    {
      sh_process_errors();
      return;
    }
  }
}

// close shader debug file
void sh_close_debug()
{
  ErrorCounter::curShader().reset();
  ErrorCounter::allShaders().reset();
  ErrorCounter::allCompilations().reset();
}


void sh_dump_warn_info()
{
  ErrorCounter &esh = ErrorCounter::allShaders();
  ErrorCounter &ec = ErrorCounter::allCompilations();
  esh.dumpInfo();
  ec.dumpInfo();
}


// if has errors, terminate program
void sh_process_errors()
{
  if (ErrorCounter::allShaders().err != 0)
  {
    if (!atomicDebug.tryLock())
      return;
    processError = false;
    current_variant = prev_variant = NULL;
    current_dyn_variant = prev_dyn_variant = NULL;

    sh_dump_warn_info();
    sh_debug(SHLOG_FATAL, "Shader parsing interrupted after %d error(s)", ErrorCounter::allShaders().err);

    lockShaderOutput = true;
    processError = true;
    atomicDebug.unlock();

    if (!shc::try_enter_shutdown())
      return;

    shc::deinit_jobs();
    if (usePrintf)
    {
      printf("\n\nCompilation aborted after %d error(s)\n", ErrorCounter::allShaders().err);
      printf("See shaderlog for more info\n");
      quit_game(13);
    }
    else
    {
      DAG_FATAL("Shader parser (%d errors) - see ShaderLog.\n", ErrorCounter::allShaders().err);
    }
  }
}


// set current variant to output
void sh_set_current_shader(const char *shader) { current_shader = shader; }
void sh_set_current_variant(IVariantInfoProvider *var)
{
  prev_variant = current_variant;
  current_variant = var;
}
void sh_set_current_dyn_variant(IVariantInfoProvider *var)
{
  prev_dyn_variant = current_dyn_variant;
  current_dyn_variant = var;
}

void sh_console_quet_output(bool e) { quetConsole = e; }

void sh_console_print_warnings(bool e) { printWarnings = e; }