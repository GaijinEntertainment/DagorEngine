// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ctype.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <sqrat.h>

#include <webui/sqDebuggerPlugin.h>
#include <sqDebugger/sqDebugger.h>
#include <sqDebugger/scriptProfiler.h>
#include <quirrel/sqStackChecker.h>

#include <squirrel/sqpcheader.h>
#include <sqstdaux.h>
#include <sqastio.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>


using namespace webui;

#define SQ_DEBUGGER_DISCONNECT_TIME_MSEC 6000

#if DAGOR_DBGLEVEL > 0
#define ENABLE_SQ_DEBUGGER 1
#else
#define ENABLE_SQ_DEBUGGER 0
#endif


#if ENABLE_SQ_DEBUGGER

static String break_sourcename;
static int break_line = 0;

static String sq_output;
static int sq_output_limit = 14000;
static bool sq_output_overflow = false;


static int sort_by_alph(const String *lhs, const String *rhs) { return strcmp(lhs->str(), rhs->str()); }

static void sq_debug_printf(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  if (sq_output.length() > sq_output_limit)
  {
    if (!sq_output_overflow)
    {
      sq_output_overflow = true;
      sq_output.append("...\n");
    }
    return;
  }
  else
    sq_output_overflow = false;

  va_list vl;
  va_start(vl, s);
  static char tempBuf[8192];
  _vsnprintf(tempBuf, sizeof(tempBuf), s, vl);
  tempBuf[sizeof(tempBuf) - 1] = 0;
  sq_output.append(tempBuf);
  if (sq_output.length() > 0 && sq_output[sq_output.length() - 1] != '\r' && sq_output[sq_output.length() - 1] != '\n')
    sq_output.append("\n");
  va_end(vl);
}

static SQInteger sq_debug_print_to_output_str(HSQUIRRELVM vm)
{
  if (SQ_SUCCEEDED(sq_tostring(vm, 2)))
  {
    const SQChar *str;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &str)))
    {
      sq_debug_printf(vm, "%s", str);
      return 0;
    }
  }
  return SQ_ERROR;
}


static Sqrat::Object make_func_obj(HSQUIRRELVM vm, SQFUNCTION func, const char *name, int paramscheck = 0,
  const char *typemask = nullptr)
{
  SqStackChecker stackCheck(vm);
  sq_newclosure(vm, func, 0);
  if (name)
    sq_setnativeclosurename(vm, -1, name);
  if (paramscheck)
    sq_setparamscheck(vm, paramscheck, typemask);

  HSQOBJECT obj;
  sq_getstackobj(vm, -1, &obj);
  Sqrat::Object res(obj, vm);
  sq_pop(vm, 1);
  return res;
}


static SQInteger stub_debug_table_data(HSQUIRRELVM v)
{
  if (SQ_SUCCEEDED(sq_tostring(v, 2)))
  {
    const SQChar *s = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, -1, &s)));

    const char *quotes = (sq_gettype(v, 2) == OT_STRING) ? "\"" : "";
    sq_output.printf(0, "%s%s%s\n\nWARNING: Object printing function is not defined for this VM (use setObjPrintFunc)", quotes, s,
      quotes);
  }
  else
    sq_output.setStr("[ERROR]");

  return 0;
}


static SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  G_ASSERT(sq_gettop(v) == 2);
  SqStackChecker stackCheck(v);

  const char *errMsg = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  sq_output.setStr(errMsg);
  return 0;
}

static void compile_error_handler(HSQUIRRELVM /*vm*/, SQMessageSeverity /*severity*/, const SQChar *desc, const SQChar * /*source*/,
  SQInteger /*line*/, SQInteger /*column*/, const SQChar *)
{
  sq_output.setStr(desc);
}


static SQInteger tracevar(HSQUIRRELVM v)
{
  HSQOBJECT container;
  HSQOBJECT key;
  sq_getstackobj(v, 2, &container);
  sq_getstackobj(v, 3, &key);
  char buf[4000];
  buf[0] = '\0';
  sq_tracevar(v, &container, &key, buf, sizeof(buf));
  buf[sizeof(buf) - 1] = 0;
  sq_pushstring(v, buf, -1);
  return 1;
}


void debug_plugin_break_cb(DagSqDebugger &debugger, const char *sourcename, int line)
{
  break_sourcename = sourcename;
  break_line = line;

  while (debugger.suspended && !debugger.executingImmediate)
  {
    sleep_msec(10);
    watchdog_kick();
    webui::update();

    if (debugger.checkForUITimeout)
      if (get_time_msec() - debugger.lastActiveUITime > SQ_DEBUGGER_DISCONNECT_TIME_MSEC)
      {
        debugger.enable(false);
        debug("Quirrel Debugger stopped due to UI timeout");
      }
  }
}


void on_sqdebug_internal(int debugger_index, RequestInfo *params)
{
  DagSqDebugger &debugger = dag_sq_debuggers.get(debugger_index);

  debugger.checkForUITimeout = true;
  debugger.lastActiveUITime = get_time_msec();

  if (!params->params)
  {
    String inlinedSqDebugger(tmpmem);
    inlinedSqDebugger.setStr(
#include "quirrelDebugger.html.inl"
    );
    inlinedSqDebugger.replaceAll("__PLUGIN_NAME__", params->plugin->name);
    inlinedSqDebugger.replaceAll("__PLUGIN_TITLE__", dag_sq_debuggers.getDescription(debugger_index));
    html_response_raw(params->conn, inlinedSqDebugger);
    return;
  }


  if (!strcmp(params->params[0], "file") && params->params[1] && params->params[1][0])
  {
    html_response_file(params->conn, params->params[1]);
  }
  else if (!strcmp(params->params[0], "attach"))
  {
    debugger.enable(true);
    debugger.setBreakCb(debug_plugin_break_cb);
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "detach"))
  {
    debugger.enable(false);
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "state"))
  {
    if (!debugger.suspended)
      html_response_raw(params->conn, "0");
    else
    {
      String s(128, "%d%%%s%%%d", (debugger.runtimeErrorState ? 2 : 1), break_sourcename.c_str(), break_line);
      if (debugger.runtimeErrorState)
      {
        Base64 errorBase64;
        errorBase64.encode((uint8_t *)debugger.lastRuntimeError.str(), debugger.lastRuntimeError.length());
        s.aprintf(128, "%%%s", errorBase64.c_str());
      }
      html_response_raw(params->conn, s.str());
    }
  }
  else if (!strcmp(params->params[0], "sources"))
  {
    Tab<String> src;
    String s;
    debugger.getSourceList(src);
    sort(src, &sort_by_alph);
    for (int i = 0; i < src.size(); i++)
      s.aprintf(128, "%s%%", src[i].str());
    if (!s.empty())
      s[s.length() - 1] = 0;
    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "breaks"))
  {
    String s;
    const char *sourcename = params->params[1];
    for (int i = 0; i < debugger.breakpoints.size(); i++)
      if (!strcmp(sourcename, debugger.breakpoints[i].sourcename.str()))
        s.aprintf(32, "%d%%", debugger.breakpoints[i].lineNum);
    if (!s.empty())
      s[s.length() - 1] = 0;
    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "breakpoint_set") && params->params[1] && params->params[2] && params->params[3])
  {
    const char *sourcename = params->params[1];
    int linenum = atoi(params->params[3]);
    debugger.setBreakpoint(sourcename, linenum, "");
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "breakpoint_remove") && params->params[1] && params->params[2] && params->params[3])
  {
    const char *sourcename = params->params[1];
    int linenum = atoi(params->params[3]);
    debugger.removeBreakpoint(sourcename, linenum);
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "breakpoint_remove_all"))
  {
    debugger.breakpoints.clear();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "run"))
  {
    debugger.run();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "step_into"))
  {
    debugger.stepInto();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "step_out"))
  {
    debugger.stepOut();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "step_over"))
  {
    debugger.stepOver();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "pause"))
  {
    debugger.pause();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "stack"))
  {
    String s;
    SQStackInfos si;
    SQInteger level = 0;
    HSQUIRRELVM vm = debugger.vm;

    if (!vm)
      html_response_raw(params->conn, "");
    else
    {
      SqStackChecker stackCheck(vm);

      while (sq_stackinfos(vm, level, &si) >= 0)
      {
        const char *fn = "unknown";
        const char *src = "unknown";
        if (si.funcname)
          fn = si.funcname;
        if (si.source)
          src = si.source;
        s.aprintf(256, "%s%%%s%%%d%%", fn, src, (int)si.line);
        level++;
      }
      if (!s.empty())
        s[s.length() - 1] = 0;
      html_response_raw(params->conn, s.str());
    }
  }
  else if (!strcmp(params->params[0], "locals"))
  {
    HSQUIRRELVM vm = debugger.vm;

    if (!vm)
      html_response_raw(params->conn, "");
    else
    {
      SqStackChecker stackCheck(vm);
      const char *locals = "No stack";
      const char *callstack = nullptr;

      if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
      {
        G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &callstack)));
        locals = strstr(callstack, "\nLOCALS\n");
        if (locals)
          locals += 8;
        else
          locals = callstack;
      }

      html_response_raw(params->conn, locals);

      if (callstack)
        sq_pop(vm, 1);
    }
  }
  else if (!strcmp(params->params[0], "immediate") && params->params[1] && params->params[2] && params->params[3])
  {
    Base64 codeBase64(params->params[1]);
    String decodedCode;
    codeBase64.decode(decodedCode);
    Base64 commandBase64(params->params[3]);
    String decodedCommand;
    commandBase64.decode(decodedCommand);
    String code(128, decodedCommand.str(), decodedCode.str());

    HSQUIRRELVM vm = debugger.vm;

    if (!vm)
      html_response_raw(params->conn, "");
    else
    {
      SqStackChecker stackCheck(vm);

      bool paused = debugger.suspended;
      debugger.suspended = false;
      bool immediate = debugger.executingImmediate;
      debugger.executingImmediate = true;

      sq_output.clear();


      Sqrat::Function debugTableDataSq = debugger.objPrintFunc;
      if (debugTableDataSq.IsNull())
      {
        debug("SqDebugger: object printing function not found for this VM, 'tostring()' will be used instead");
        Sqrat::Object stubFunc = make_func_obj(vm, stub_debug_table_data, "stubDebugTableData", -2, nullptr);
        debugTableDataSq = Sqrat::Function(vm, Sqrat::Object(vm), stubFunc);
      }

      stackCheck.check();

      Sqrat::Object *thisObj = NULL;
      Sqrat::Object context;

      Sqrat::Table objFormatSettings(vm);
      objFormatSettings.SetValue("silentMode", true);
      objFormatSettings.SetValue("printFn", make_func_obj(vm, sq_debug_print_to_output_str, "debugger_print_collect", 2));

      Sqrat::Table locals(vm);

      stackCheck.check();

      bool foundInLocals = false;
      for (int level = 0; !foundInLocals && level < 10; level++)
      {
        int seq = 0;
        while (const SQChar *name = sq_getlocal(vm, level, seq))
        {
          Sqrat::Var<Sqrat::Object> obj(vm, -1);

          if (!locals.HasKey(name))
            locals.SetValue(name, obj.value);

          if (!thisObj && name && !strcmp(name, "this"))
          {
            context = obj.value;
            thisObj = &context;
          }

          if (name && *name && !strcmp(name, decodedCode.str()))
          {
            sq_output.clear();
            foundInLocals = true;

            if (!debugTableDataSq.IsNull())
              debugTableDataSq.Execute(obj.value, objFormatSettings);

            sq_pop(vm, 1); // pop current local
            break;
          }
          seq++;
          sq_pop(vm, 1); // pop current local
        }
      }

      stackCheck.check();

      if (!foundInLocals && thisObj)
      {
        String scriptSrc(0, "return (@() (\n\
          %s\n\
        ))()",
          code.c_str());

        stackCheck.check();

        Sqrat::Table bindings(vm);
        bindings.SetValue("debugTableData", Sqrat::Object(debugTableDataSq.GetFunc(), vm));
        bindings.SetValue("tracevar", make_func_obj(vm, tracevar, "tracevar", 3));

        // merge local variable into evaluated closure bindings
        Sqrat::Object::iterator it;
        while (locals.Next(it))
          bindings.SetValue(Sqrat::Object(it.getKey(), vm), Sqrat::Object(it.getValue(), vm));

        stackCheck.check();

        SQCOMPILERERROR prevCompileErrorHandler = sq_getcompilererrorhandler(vm);
        sq_setcompilererrorhandler(vm, compile_error_handler);

        stackCheck.check();

        Sqrat::Object prevRuntimeErrorHandler(sq_geterrorhandler(vm), vm);
        sq_newclosure(vm, runtime_error_handler, 0);
        sq_seterrorhandler(vm);

        stackCheck.check();

        bool old = sq_checkcompilationoption(vm, CompilationOptions::CO_CLOSURE_HOISTING_OPT);
        sq_setcompilationoption(vm, CompilationOptions::CO_CLOSURE_HOISTING_OPT, false);

        HSQOBJECT hBindings = bindings.GetObject();
        if (SQ_SUCCEEDED(sq_compile(vm, scriptSrc.c_str(), scriptSrc.length(), "interactive", true, &hBindings)))
        {
          sq_setcompilationoption(vm, CompilationOptions::CO_CLOSURE_HOISTING_OPT, old);
          HSQOBJECT scriptClosure;
          sq_getstackobj(vm, -1, &scriptClosure);
          Sqrat::Function func(vm, *thisObj, scriptClosure);
          Sqrat::Object result;
          if (func.Evaluate(result))
          {
            if (!debugTableDataSq.IsNull())
              debugTableDataSq.Execute(result, objFormatSettings);
          }
          sq_pop(vm, 1); // script closure
        }
        sq_setcompilationoption(vm, CompilationOptions::CO_CLOSURE_HOISTING_OPT, old);

        stackCheck.check();

        sq_setcompilererrorhandler(vm, prevCompileErrorHandler);
        sq_pushobject(vm, prevRuntimeErrorHandler);
        sq_seterrorhandler(vm);

        stackCheck.check();

        //  locals.Reset();
      }

      stackCheck.check();

      html_response_raw(params->conn, sq_output.str());
      sq_output.clear();

      debugger.executingImmediate = immediate;
      debugger.suspended = paused;
    }
  }
  else if (!strcmp(params->params[0], "disasm"))
  {
    HSQUIRRELVM vm = debugger.vm;

    if (!vm)
      html_response_raw(params->conn, "");
    else if (!debugger.suspended)
      html_response_raw(params->conn, "Program must be suspened for disasm of the current function");
    else
    {
      sq_output.clear();
      SQVM::CallInfo &ci = vm->_callsstack[0];
      if (sq_type(ci._closure) == OT_CLOSURE)
      {
        SQClosure *c = _closure(ci._closure);
        SQFunctionProto *func = c->_function;
        int instructionIndex = int(ci._ip - func->_instructions);

        MemoryOutputStream mem;
        sq_dumpbytecode(vm, ci._closure, &mem, instructionIndex);

        sq_output.setStr((const char *)mem.buffer(), mem.pos());
      }

      html_response_raw(params->conn, sq_output.str());
      sq_output.clear();
    }
  }
  else if (!strcmp(params->params[0], "clear_coverage"))
  {
    debugger.clearSourceCoverage();
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "break_on_event") && params->params[1])
  {
    debugger.breakOnEvent = (!strcmp(params->params[1], "1"));
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "get_coverage") && params->params[1])
  {
    Tab<char> res;
    if (debugger.getSourceCoverage(params->params[1], res))
    {
      String s(1024, "");
      int i = 1;

      while (i < res.size())
      {
        int prev = i;

        for (int t = 0; t < 3; t++)
        {
          int cnt = 0;
          while (i < res.size() && res[i] == t)
          {
            cnt++;
            i++;
          }
          s.aprintf(64, "%d%%", cnt);
        }

        if (i <= prev)
          break;
      }

      if (!s.empty())
        s[s.length() - 1] = 0;

      html_response_raw(params->conn, s.str());
    }
    else
      html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "profiler_start"))
  {
    scriptprofile::initProfile(debugger.vm);
    scriptprofile::start(debugger.vm);
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "profiler_stop"))
  {
    if (scriptprofile::isStarted())
    {
      scriptprofile::stop(debugger.vm);
      String profileOutput;
      scriptprofile::getProfileResult(profileOutput);
      html_response_raw(params->conn, profileOutput);
    }
    else
      html_response_raw(params->conn, "");
  }
}


#else // ENABLE_SQ_DEBUGGER

void debug_plugin_break_cb(DagSqDebugger & /*debugger*/, const char * /*sourcename*/, int /*line*/) {}


void on_sqdebug_internal(int /*debugger_index*/, RequestInfo * /*params*/) {}


#endif // ENABLE_SQ_DEBUGGER
