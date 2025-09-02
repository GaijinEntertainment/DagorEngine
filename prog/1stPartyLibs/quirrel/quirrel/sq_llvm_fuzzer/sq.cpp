/*  see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>
#include <sqmodules.h>
#include <sqastio.h>
#include <sqstddebug.h>
#include <squirrel/sqtypeparser.h>

#include <startup/dag_globalSettings.h>

#define scvprintf vfprintf


void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
  va_list vl;
  va_start(vl, s);
  scvprintf(stdout, s, vl);
  va_end(vl);
}

static FILE *errorStream = stderr;

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
  va_list vl;
  va_start(vl, s);
  scvprintf(errorStream, s, vl);
  va_end(vl);
}


static int fill_stack(HSQUIRRELVM v)
{
  for (int i = 0; i < 8; i++)
    sq_pushinteger(v, i + 1000000);

  return sq_gettop(v);
}

static bool check_stack(HSQUIRRELVM v, int expected_top)
{
  int top = sq_gettop(v);
  if (top != expected_top)
  {
    fprintf(errorStream, "ERROR: Stack top is %d, expected %d\n", top, expected_top);
    abort();
  }

  for (int i = 0; i < 8; i++)
  {
    SQInteger val = -1;
    if (SQ_FAILED(sq_getinteger(v, -8 + i, &val)) || val != i + 1000000)
    {
      fprintf(errorStream, "ERROR: Stack value at %d is %d, expected %d\n", -8 + i, int(val), i + 1000000);
      abort();
    }
  }

  return true;
}


void run(HSQUIRRELVM v, const char * buffer, int length)
{
  int stackTop = fill_stack(v);

  HSQOBJECT bindingsTable;
  sq_newtable(v);
  sq_getstackobj(v, -1, &bindingsTable);
  sq_addref(v, &bindingsTable);

  sq_registerbaselib(v);

  sq_pop(v, 1); // bindingsTable

  SQInteger oldtop = sq_gettop(v);
  if (SQ_SUCCEEDED(sq_compile(v, buffer, length, _SC("buffer"), SQTrue, &bindingsTable))) {
    sq_pushroottable(v);
    SQInteger retval = 0;
    sq_call(v, 1, retval, SQTrue);
  }
  sq_settop(v,oldtop);

  sq_release(v, &bindingsTable);
  check_stack(v, stackTop);
}


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

  HSQUIRRELVM v;

  v = sq_open(64);
  //sq_setprintfunc(v, printfunc, errorfunc);
  sq_pushroottable(v);

  sqstd_register_bloblib(v);
  sqstd_register_iolib(v);
  sqstd_register_systemlib(v);
  sqstd_register_datetimelib(v);
  sqstd_register_mathlib(v);
  sqstd_register_stringlib(v);
  sqstd_register_debuglib(v);
  sqstd_seterrorhandlers(v);

  run(v, (const char *)(data), int(size));

  sq_close(v);

  return 0;
}
