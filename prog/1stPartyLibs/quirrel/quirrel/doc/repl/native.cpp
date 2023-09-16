#include "../../include/squirrel.h"
#include "../../include/sqstdaux.h"
#include "../../sqmodules/sqmodules.h"
#include <emscripten/bind.h>
#include <cstdio>
#include <cstdarg>

static std::string output;


static const std::string vlformat(const char * const fmt, va_list va)
{
  va_list vaCopy;
  va_copy(vaCopy, va);
  const int iLen = std::vsnprintf(nullptr, 0, fmt, vaCopy);
  va_end(vaCopy);

  std::vector<char> zc(iLen + 1);
  std::vsnprintf(zc.data(), zc.size(), fmt, va);

  return std::string(zc.data(), iLen);
}

static const std::string vaformat(const char * const fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);
  std::string res = vlformat(fmt, vl);
  va_end(vl);
  return res;
}

static void script_print_func(HSQUIRRELVM /*v*/, const SQChar* s,...)
{
  va_list vl;
  va_start(vl, s);
  output += vlformat(s, vl);
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM /*v*/, const SQChar* s,...)
{
  va_list vl;
  va_start(vl, s);
  output += vlformat(s, vl);
  va_end(vl);
}


static void compile_error_handler(HSQUIRRELVM /*v*/, const SQChar *desc, const SQChar *source,
                        SQInteger line, SQInteger column, const SQChar *)
{
  output += vaformat("Squirrel compile error %s (%d:%d): %s", source, int(line), int(column), desc);
}


static SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  assert(sq_gettop(v) == 2);

  const char *errMsg = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  output.append(errMsg);

  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(v)))
  {
    const char* callstack = nullptr;
    sq_getstring(v, -1, &callstack);

    output.append("\n");
    output.append(callstack);

    sq_pop(v, 1);
  }
  else
    output += "No stack";

  return 0;
}


static std::string runScript(const std::string &source_text)
{
  output.clear();

  HSQUIRRELVM vm = sq_open(1024);
  assert(vm);

  std::unique_ptr<SqModules> moduleMgr;
  moduleMgr.reset(new SqModules(vm));

  sq_setprintfunc(vm, script_print_func, script_err_print_func);
  sq_setcompilererrorhandler(vm, compile_error_handler);

  sq_newclosure(vm, runtime_error_handler, 0);
  sq_seterrorhandler(vm);

  sq_enabledebuginfo(vm, true);
  sq_enablevartrace(vm, false);

  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerIoStreamLib();
  //moduleMgr->registerIoLib(); // not needed in browser version
  moduleMgr->registerDateTimeLib();

  sq_newtable(vm);
  HSQOBJECT hBindings;
  sq_getstackobj(vm, -1, &hBindings);
  moduleMgr->bindBaseLib(hBindings);
  moduleMgr->bindRequireApi(hBindings);

  if (SQ_SUCCEEDED(sq_compilebuffer(vm, source_text.c_str(), source_text.length(), "console", true, &hBindings)))
  {
    sq_pushnull(vm); //environment

    if (SQ_FAILED(sq_call(vm, 1, SQTrue, SQTrue)))
      output += "\nScript execution failed";
    else
    {
      if (sq_gettype(vm, -1) != OT_NULL)
      {
        if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
        {
          const SQChar *resStr = nullptr;
          sq_getstring(vm, -1, &resStr);
          output += "\n";
          output += resStr;
          sq_pop(vm, 1);
        }
      }
    }

    sq_pop(vm, 1); // script closure
  }

  sq_pop(vm, 1); // bindings table

  std::string result = output;

  output.clear();

  moduleMgr.reset();
  sq_close(vm);

  return result;
}

EMSCRIPTEN_BINDINGS(native) { emscripten::function("runScript", &runScript); }
