// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <sqModules/sqModules.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include "scriptBindings.h"


static void script_print_func(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(_MAKE4C('SQRL'), s, vl);
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM v, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(LOGLEVEL_ERR, s, vl);
  va_end(vl);
}


static void compile_error_handler(HSQUIRRELVM v, SQMessageSeverity sev, const SQChar *desc, const SQChar *source, SQInteger line,
  SQInteger column, const SQChar *)
{
  const SQChar *sevName = "error";
  if (sev == SEV_HINT)
    sevName = "hint";
  else if (sev == SEV_WARNING)
    sevName = "warning";
  String str;
  str.printf(128, "Squirrel compile %s %s (%d:%d): %s", sevName, source, line, column, desc);
  if (sev == SEV_HINT)
    clogmessage(LOGLEVEL_DEBUG, "%s", str.c_str());
  else
    DAG_FATAL(str);
}


static SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  G_ASSERT(sq_gettop(v) == 2);
  const char *errMsg = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  sqstd_printcallstack(v);
  DAG_FATAL(errMsg);

  return sq_suspendvm(v);
}

void run_init_script(const char *scriptKey)
{
  const char *scriptFn = ::dgs_get_settings()->getStr(scriptKey, nullptr);
  if (!scriptFn)
  {
    debug("No initialization script %s", scriptKey);
    return;
  }

  HSQUIRRELVM sqvm = sq_open(1024);
  G_ASSERT(sqvm);


  {
    SqModules moduleMgr(sqvm);
    bindquirrel::apply_compiler_options_from_game_settings(&moduleMgr);

    sq_setprintfunc(sqvm, script_print_func, script_err_print_func);
    sq_setcompilererrorhandler(sqvm, compile_error_handler);

    sq_newclosure(sqvm, runtime_error_handler, 0);
    sq_seterrorhandler(sqvm);

    bind_dargbox_script_api(&moduleMgr);

    String errMsg;
    Sqrat::Object initScriptExports;
    if (!moduleMgr.reloadModule(scriptFn, true, SqModules::__main__, initScriptExports, errMsg))
    {
      String msg;
      msg.printf(256, "Failed to load script module %s: %s", scriptFn, errMsg.c_str());
      DAG_FATAL(msg);
    }
    unbind_dargbox_script_api(sqvm);
  }

  sq_close(sqvm);
}