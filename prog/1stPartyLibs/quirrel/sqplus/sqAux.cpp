#include "dag_sqAux.h"
#include <squirrel/sqpcheader.h>
#include <squirrel.h>
#include <assert.h>
#include <string.h> // strncmp
#include <sqstdaux.h>

#include <squirrel/sqvm.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <cstring> //memset


#define SECRET_PREFIX "no_dump_"
#define ERRBUF_MAX_SIZE (12*1024)
#define CALLSTACK_STRING_LIMIT 1000
#define CALLSTACK_SHORT_STRING_LIMIT 100


#ifndef _PRINT_INT_FMT
#include <util/dag_inttypes.h>
#define _PRINT_INT_FMT _SC("%" PRId64)
#endif

static SQInteger sqdagor_printerror(HSQUIRRELVM v);
static SQInteger sqdagor_imprint_error(HSQUIRRELVM v, SQChar *buf, SQInteger max_size);

static const char ERROR_HANDLER_KEY[]     = "__error_handler";
static const char ERROR_BUFFER_KEY[]      = "__error_buffer";
static const char ERROR_BUFFER_SIZE_KEY[] = "__error_buffer_size";


static SQFunctionProto * _get_function(const SQVM::CallInfo &ci)
{
  if (sq_type(ci._closure) != OT_CLOSURE)
    return NULL;
  return _closure(ci._closure)->_function;
}

void sqdagor_get_exception_source_and_line(HSQUIRRELVM v,
                                          const char ** exception_source_out, int &exception_line_out)
{
  SQInteger cssize = v->_callsstacksize;
  SQVM::CallInfo &ci = v->_callsstack[cssize - 2]; //0 level is _dag_handle_err_wrapper
  SQFunctionProto *func = _get_function(ci);

  if (func && sq_isstring(func->_sourcename))
    *exception_source_out = _stringval(func->_sourcename);
  else
    *exception_source_out = "unknown";
  exception_line_out = func ? func->GetLine(ci._ip) : -1;
}


static dag_sq_error_handler_t _get_error_handler(SQVM *vm)
{
  SQUserPointer errorHandler = NULL;
  sq_pushregistrytable(vm);
  sq_pushstring(vm, ERROR_HANDLER_KEY, sizeof(ERROR_HANDLER_KEY)-1);
  if (SQ_SUCCEEDED(sq_get(vm, -2)))
  {
    G_VERIFY(SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &errorHandler)));
    sq_pop(vm, 2);
  }
  else
    sq_pop(vm, 1);
  return (dag_sq_error_handler_t)errorHandler;
}


static SQChar * _get_error_buffer(SQVM *vm)
{
  SQUserPointer errorBuffer = NULL;
  sq_pushregistrytable(vm);
  sq_pushstring(vm, ERROR_BUFFER_KEY, sizeof(ERROR_BUFFER_KEY)-1);
  sq_get(vm, -2);
  G_VERIFY(SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &errorBuffer)));
  sq_pop(vm, 2);

  return (SQChar *)errorBuffer;
}


static size_t _get_error_buffer_size(SQVM *vm)
{
  SQInteger errorBufferSize = 0;
  sq_pushregistrytable(vm);
  sq_pushstring(vm, ERROR_BUFFER_SIZE_KEY, sizeof(ERROR_BUFFER_SIZE_KEY)-1);
  sq_get(vm, -2);
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, -1, &errorBufferSize)));
  sq_pop(vm, 2);

  return (size_t)errorBufferSize;
}


static SQInteger _dag_handle_err_wrapper(SQVM *vm)
{
  sqdagor_printerror(vm);
  dag_sq_error_handler_t errorHandler = _get_error_handler(vm);
  if (!errorHandler)
    return 0;

  SQChar * errorBuffer = _get_error_buffer(vm);
  size_t buffferSize = _get_error_buffer_size(vm);
  sqdagor_imprint_error(vm, errorBuffer, buffferSize);

  if (vm->_debughook_native == NULL) // don't show any message windows while debugger connected
  {
    const char * exceptionSrc = NULL;
    int exceptionLine = -1;
    sqdagor_get_exception_source_and_line(vm, &exceptionSrc, exceptionLine);
    errorHandler(vm, errorBuffer, exceptionSrc, exceptionLine);
  }

  return 0;
}


void sqdagor_override_error_handler(SQVM *vm, dag_sq_error_handler_t handler, char *buf, size_t buf_size)
{
  G_ASSERT(handler && buf && buf_size);

  sq_pushregistrytable(vm);

  sq_pushstring(vm, ERROR_HANDLER_KEY, sizeof(ERROR_HANDLER_KEY)-1);
  G_STATIC_ASSERT(sizeof(SQUserPointer) == sizeof(handler));
  sq_pushuserpointer(vm, (SQUserPointer)handler);
  sq_newslot(vm, -3, false);

  sq_pushstring(vm, ERROR_BUFFER_KEY, sizeof(ERROR_BUFFER_KEY)-1);
  sq_pushuserpointer(vm, buf);
  sq_newslot(vm, -3, false);

  sq_pushstring(vm, ERROR_BUFFER_SIZE_KEY, sizeof(ERROR_BUFFER_SIZE_KEY)-1);
  sq_pushinteger(vm, buf_size);
  sq_newslot(vm, -3, false);

  sq_pop(vm, 1);

  sqdagor_seterrorhandlers(vm);
}

bool dag_sq_is_errhandler_overriden(HSQUIRRELVM v)
{
  return _get_error_handler(v) != NULL;
}


#define CAN_IMPRINT (total < max_size)

#define IMPRINT_BUF(fmt, ...) \
do \
if (CAN_IMPRINT) \
{ \
  size_t avail = max_size - total; \
  printed = scsprintf(pos, avail, (fmt), ##__VA_ARGS__); \
  if ((size_t)printed < avail) { total += printed; pos += printed; } \
  else total = max_size; \
} \
while (0)

SQInteger sqdagor_formatcallstack(HSQUIRRELVM v, SQChar *buf, SQInteger max_size, bool is_short_form)
{
  G_ASSERT(buf);
  SQStackInfos si;
  SQInteger i;
  SQFloat f;
  SQBool bval;
  const SQChar *s;
  SQInteger level = 0;
  const SQChar *name=0;
  SQInteger seq=0;
  SQInteger printed = 0;
  SQInteger total = 0;
  SQChar *pos = buf;

  IMPRINT_BUF(_SC("CALLSTACK:\n"));

  while (CAN_IMPRINT && SQ_SUCCEEDED(sq_stackinfos(v,level,&si)))
  {
    if (si.line < 0 && level == 0) // skip top native function
    {
      ++level;
      continue;
    }

    const SQChar *fn=_SC("unknown");
    const SQChar *src=_SC("unknown");
    if (si.funcname)
      fn=si.funcname;
    if (si.source)
      src=si.source;

    if (is_short_form)
    {
      const char * lastSlash = strrchr(src, '/');
      if (lastSlash && level > 1)
        src = lastSlash + 1;

      size_t srcLen = strlen(src);
      const char *ext = strstr(src, ".nut");
      if (ext == src + srcLen - 4 && level > 1)
        srcLen -= 4;

      size_t fnLen = strlen(fn);
      const char *brackets = strstr(fn, "()");
      if (brackets == fn + fnLen - 2)
        fnLen -= 2;

      if (level < 12)
        IMPRINT_BUF(_SC("%.*s %.*s:%d\n"), (int) fnLen, fn, (int) srcLen, src, (int)si.line);
      else
        IMPRINT_BUF(_SC("%.*s ?:%d\n"), (int) fnLen, fn, (int)si.line);

      if (level > 22)
      {
        IMPRINT_BUF(_SC("?\n"));
        break;
      }
    }
    else
      IMPRINT_BUF(_SC("*FUNCTION [%s()] %s line [%d]\n"), fn, src, (int)si.line);

    level++;
  }
  level=0;
  IMPRINT_BUF(_SC("\nLOCALS\n"));

  for (level = 0; CAN_IMPRINT && level < 10; ++level)
  {
    seq=0;
    while (CAN_IMPRINT && (name = sq_getlocal(v,level,seq)) != NULL)
    {
      seq++;
#if DAGOR_DBGLEVEL <= 0
      if (strncmp(name, SECRET_PREFIX, sizeof(SECRET_PREFIX) - 1) == 0)
      {
        sq_pop(v, 1); // Restore stack after successful sq_getlocal()
        continue; // you saw nothing!
      }
#endif
      const SQObjectPtr& obj = stack_get(v, -1);
      switch(sq_gettype(v,-1))
      {
        case OT_NULL:
          IMPRINT_BUF(_SC(is_short_form ? "%s=null\n" : "[%s] NULL\n"), name);
          break;
        case OT_INTEGER:
          sq_getinteger(v,-1,&i);
          IMPRINT_BUF(_SC(is_short_form ? "%s=" _PRINT_INT_FMT "\n" : "[%s] " _PRINT_INT_FMT "\n"), name, i);
          break;
        case OT_FLOAT:
          sq_getfloat(v,-1,&f);
          IMPRINT_BUF(_SC(is_short_form ? "%s=%g\n" : "[%s] %.14g\n"), name, f);
          break;
        case OT_USERPOINTER:
          IMPRINT_BUF(_SC(is_short_form ? "%s=ptr %p\n" : "[%s] USERPOINTER 0x%p\n"),name,obj._unVal.pUserPointer);
          break;
        case OT_STRING:
          {
            sq_getstring(v,-1,&s);
            int limit = is_short_form ? CALLSTACK_SHORT_STRING_LIMIT : CALLSTACK_STRING_LIMIT;
            int len = s ? int(strlen(s)) : 0;
            bool ellipsis = (s && strlen(s) > limit + 6);
            IMPRINT_BUF(_SC(is_short_form ? "%s=\"%.*s%s" : "[%s] \"%.*s%s"), name, limit, s ? s : _SC(""),
              ellipsis ? _SC("") : _SC("\""));

            if (ellipsis)
              IMPRINT_BUF(_SC("...(%d)\n"), len);
            else
              IMPRINT_BUF(_SC("\n"));
          }
          break;
        case OT_TABLE:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:{} (%d)\n"), name, int(sq_getsize(v, -1)));
          else
            IMPRINT_BUF(_SC("[%s] TABLE 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_ARRAY:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:[] (%d)\n"), name, int(sq_getsize(v, -1)));
          else
            IMPRINT_BUF(_SC("[%s] ARRAY 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_CLOSURE:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:fn\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] CLOSURE 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_NATIVECLOSURE:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:C++\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] NATIVECLOSURE 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_GENERATOR:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:gen\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] GENERATOR 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_USERDATA:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:userdata\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] USERDATA 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_THREAD:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:thread\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] THREAD 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_CLASS:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:class\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] CLASS 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_INSTANCE:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:inst\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] INSTANCE 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_WEAKREF:
          if (is_short_form)
            IMPRINT_BUF(_SC("%s:ref\n"), name);
          else
            IMPRINT_BUF(_SC("[%s] WEAKREF 0x%p\n"), name, obj._unVal.pUserPointer);
          break;
        case OT_BOOL:
          {
            sq_getbool(v,-1,&bval);
            IMPRINT_BUF(_SC(is_short_form ? "%s=%s\n" : "[%s] %s\n"),name,bval == SQTrue ? _SC("true"):_SC("false"));
          }
          break;
        default: assert(0); break;
      }
      sq_pop(v,1);
    }
  }
  return (total < max_size && printed > 0) ? total : max_size - 1;
}

#undef CAN_IMPRINT
#undef IMPRINT_BUF

SQInteger sqdagor_imprint_error(HSQUIRRELVM v, SQChar *buf, SQInteger max_size)
{
  const SQChar *sErr = 0;
  SQChar *pos = buf;
  SQInteger printed = 0;

  if (sq_gettop(v)>=1)
  {
    if (SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))
      printed = scsprintf(buf, max_size, _SC("AN ERROR HAS OCCURED [%s]\n"), sErr);
    else
      printed = scsprintf(buf, max_size, _SC("AN ERROR HAS OCCURED [unknown]\n"));

    if (printed < 0 || printed >= max_size)
    {
      buf[max_size - 1] = _SC('\0');
      return max_size - 1;
    }

    pos += printed;
    SQInteger space_left = max_size - printed;
    printed = sqdagor_formatcallstack(v, pos, space_left);
    if (printed < 0 || printed >= space_left)
    {
      buf[max_size - 1] = _SC('\0');
      return max_size - 1;
    }

    pos += printed;
  }

  SQInteger total = pos - buf;
  buf[total] = _SC('\0');

  return total;
}


void sqdagor_printcallstack(HSQUIRRELVM v, bool is_short_form)
{
  if (SQPRINTFUNCTION pf = sq_getprintfunc(v))
  {
    SQChar buf[ERRBUF_MAX_SIZE];
    SQInteger len = sqdagor_formatcallstack(v, buf, ERRBUF_MAX_SIZE, is_short_form);
    buf[len] = _SC('\0');
    pf(v, _SC("%s"), buf);
  }
}

static SQInteger sqdagor_printerror(HSQUIRRELVM v)
{
  SQPRINTFUNCTION pf = sq_getprintfunc(v);
  if (pf)
  {
    SQChar buf[ERRBUF_MAX_SIZE];
    sqdagor_imprint_error(v, buf, ERRBUF_MAX_SIZE);
    pf(v, _SC("%s"), buf);
  }
  return 0;
}

static void _sqdagor_compiler_error(HSQUIRRELVM v,const SQChar *sErr,const SQChar *sSource,SQInteger line,SQInteger column, const SQChar *extra)
{
  SQPRINTFUNCTION pf = sq_geterrorfunc(v);
  if (pf) {
    pf(v, _SC("%s line = (%d) column = (%d) : error %s\n"), sSource, (int)line, (int)column, sErr);
    if (extra)
      pf(v, _SC("%s\n"), extra);
  }
}

void sqdagor_seterrorhandlers(HSQUIRRELVM v)
{
  sq_setcompilererrorhandler(v,_sqdagor_compiler_error);
  sq_newclosure(v,_dag_handle_err_wrapper,0);
  sq_seterrorhandler(v);
}
