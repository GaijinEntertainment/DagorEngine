//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
#include <sqstdaux.h>
#include <stdio.h>
#include <util/dag_string.h>
#include <quirrel/sqStackChecker.h>

#define SQPRINTCOLL_REGISTRY_KEY "dagor:print_collector"

struct SQPrintCollector
{
  SQPrintCollector(HSQUIRRELVM vm)
  {
    sqVm = vm;
    prevPrintFunc = sq_getprintfunc(vm);
    prevErrorFunc = sq_geterrorfunc(vm);
    sq_setprintfunc(sqVm, &SQPrintCollector::sq_printf, &SQPrintCollector::sq_printf);

    savedCompilerHandler = sq_getcompilererrorhandler(sqVm);
    sq_setcompilererrorhandler(sqVm, &SQPrintCollector::sq_compile_error_handler);

    savedRuntimeErrorHandler = sq_geterrorhandler(sqVm);
    sq_addref(sqVm, &savedRuntimeErrorHandler);

    sq_newclosure(sqVm, sq_runtime_error_handler, 0);
    sq_seterrorhandler(sqVm);

    setToVm(vm);
  }

  ~SQPrintCollector()
  {
    removeFromVm(sqVm);

    sq_setprintfunc(sqVm, prevPrintFunc, prevErrorFunc);
    sq_setcompilererrorhandler(sqVm, savedCompilerHandler);

    sq_pushobject(sqVm, savedRuntimeErrorHandler);
    sq_seterrorhandler(sqVm);
    sq_release(sqVm, &savedRuntimeErrorHandler);
  }

  void setToVm(HSQUIRRELVM vm)
  {
    SqStackChecker stackCheck(vm);
    sq_pushregistrytable(vm);
    sq_pushstring(vm, SQPRINTCOLL_REGISTRY_KEY, -1);
    sq_pushuserpointer(vm, this);
    G_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
    sq_poptop(vm);
  }

  void removeFromVm(HSQUIRRELVM vm)
  {
    SqStackChecker stackCheck(vm);
    sq_pushregistrytable(vm);
    sq_pushstring(vm, SQPRINTCOLL_REGISTRY_KEY, -1);
    G_VERIFY(SQ_SUCCEEDED(sq_deleteslot(vm, -2, false)));
    sq_poptop(vm);
  }

  static SQPrintCollector *getFromVm(HSQUIRRELVM vm)
  {
    SQUserPointer ptr;
    SqStackChecker stackCheck(vm);
    sq_pushregistrytable(vm);
    sq_pushstring(vm, SQPRINTCOLL_REGISTRY_KEY, -1);
    G_VERIFY(SQ_SUCCEEDED(sq_rawget(vm, -2)));
    sq_getuserpointer(vm, -1, &ptr);
    sq_pop(vm, 2);
    G_ASSERT(ptr);
    return (SQPrintCollector *)ptr;
  }

  void appendBuf(const char *str)
  {
    if (output.length() > sq_output_limit)
    {
      if (!overflow)
      {
        overflow = true;
        output.append("...\n");
      }
      return;
    }
    else
      overflow = false;

    output.append(str);
    if (output.length() > 0 && output[output.length() - 1] != '\r' && output[output.length() - 1] != '\n')
      output.append("\n");
  }

  static void sq_printf(HSQUIRRELVM vm, const SQChar *s, ...)
  {
    va_list vl;
    va_start(vl, s);
    char tempBuf[8192];
    _vsnprintf(tempBuf, sizeof(tempBuf), s, vl);
    va_end(vl);
    tempBuf[sizeof(tempBuf) - 1] = 0;
    getFromVm(vm)->appendBuf(tempBuf);
  }

  static void sq_compile_error_handler(HSQUIRRELVM vm, SQMessageSeverity severity, const SQChar *desc, const SQChar *source,
    SQInteger line, SQInteger column, const SQChar *extra)
  {
    const SQChar *sevName = "error";
    if (severity == SEV_HINT)
      sevName = "hint";
    else if (severity == SEV_WARNING)
      sevName = "warning";
    String str;
    str.printf(128, "Compile %s %s (%d:%d): %s", sevName, source, line, column, desc);
    if (extra)
      str.printf(128, "\n%s", extra);
    getFromVm(vm)->appendBuf(str);
  }

  static SQInteger sq_runtime_error_handler(HSQUIRRELVM vm)
  {
    G_ASSERT(sq_gettop(vm) == 2);

    SQPrintCollector *self = getFromVm(vm);

    const char *errMsg = "";
    if (SQ_FAILED(sq_getstring(vm, 2, &errMsg)))
      errMsg = "Unknown error";

    self->appendBuf(String(0, "\n%s\n", errMsg));

    if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
    {
      const char *callstack = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &callstack)));
      G_ASSERT(callstack);
      self->appendBuf("\n");
      self->appendBuf(callstack);
      sq_pop(vm, 1);
    }

    return 0;
  }


  HSQUIRRELVM sqVm;
  SQPRINTFUNCTION prevPrintFunc, prevErrorFunc;
  SQCOMPILERERROR savedCompilerHandler;
  HSQOBJECT savedRuntimeErrorHandler;

  String output;
  const int sq_output_limit = 14000;
  bool overflow = false;
};

#undef SQPRINTCOLL_REGISTRY_KEY
