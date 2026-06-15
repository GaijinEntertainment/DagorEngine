// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <quirrel/bindQuirrelEx/sqModulesDagor.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include "scriptBindings.h"
#include <memory/dag_framemem.h>


static void script_print_func(HSQUIRRELVM /*v*/, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(_MAKE4C('SQRL'), "%s", msg.c_str());
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM /*v*/, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(LOGLEVEL_ERR, "%s", msg.c_str());
  va_end(vl);
}


static void compile_error_handler(HSQUIRRELVM v, SQMessageSeverity sev, const char *desc, const char *source, SQInteger line,
  SQInteger column, const char *)
{
  const char *sevName = "error";
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
  // (this, error, trace): async faults carry the captured trace as a third arg.
  G_ASSERT(sq_gettop(v) == 3);
  sqstd_aux_error_to_string(v, 2);
  const char *errMsg = "Unknown error";
  sq_getstring(v, -1, &errMsg);

  sqstd_printcallstack(v);
  DAG_FATAL(errMsg);

  sq_poptop(v);

  return 0;
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
    SqModulesDagorFileAccess fileAccess(false);
    SqModules moduleMgr(sqvm, &fileAccess);
    bindquirrel::apply_compiler_options_from_game_settings(&moduleMgr);

    sq_setprintfunc(sqvm, script_print_func, script_err_print_func);
    sq_setcompilererrorhandler(sqvm, compile_error_handler);

    sq_newclosure(sqvm, runtime_error_handler, 0);
    sq_setparamscheck(sqvm, 3, nullptr);
    sq_seterrorhandler(sqvm);

    bind_dargbox_script_api(&moduleMgr);

    Sqrat::string errMsg;
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