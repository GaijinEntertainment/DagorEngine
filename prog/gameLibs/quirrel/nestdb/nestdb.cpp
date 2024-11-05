// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nestdb/nestdb.h>
#include <sqModules/sqModules.h>
#include <sqCrossCall/sqCrossCall.h>
#include <sqStackChecker.h>
#include <sqstdaux.h>
#include <sqstdblob.h>
#include <sqrat.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <util/dag_string.h>


namespace nestdb
{

static HSQUIRRELVM sqvm = nullptr;
static Sqrat::Table storage;


static void script_print_func(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(_MAKE4C('SQRL'), s, vl);
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(/*_MAKE4C('SQRL')*/ LOGLEVEL_ERR, s, vl);
  va_end(vl);
}


static void compile_error_handler(HSQUIRRELVM /*v*/, SQMessageSeverity severity, const SQChar *desc, const SQChar *source,
  SQInteger line, SQInteger column, const SQChar *)
{
  if (severity == SEV_HINT)
    debug("[STORAGE VM] Squirrel compile hint %s (%d:%d): %s", source, line, column, desc);
  else if (severity == SEV_WARNING)
    logwarn("[STORAGE VM] Squirrel compile warning %s (%d:%d): %s", source, line, column, desc);
  else
    logerr("[STORAGE VM] Squirrel compile error %s (%d:%d): %s", source, line, column, desc);
}


static SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  G_ASSERT(sq_gettop(v) == 2);

  const char *errMsg = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  logerr("[STORAGE VM] %s", errMsg);

  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(v)))
  {
    const char *callstack = nullptr;
    sq_getstring(v, -1, &callstack);
    LOGERR_CTX("[STORAGE VM] %s", callstack);
    sq_pop(v, 1);
  }

  return 0;
}


void init()
{
  G_ASSERT_RETURN(!sqvm, );

  sqvm = sq_open(1024);
  G_ASSERT(sqvm);

  SqStackChecker chk(sqvm);

  sq_setprintfunc(sqvm, script_print_func, script_err_print_func);
  sq_setcompilererrorhandler(sqvm, compile_error_handler);

  sq_newclosure(sqvm, runtime_error_handler, 0);
  sq_seterrorhandler(sqvm);

  sq_newtable(sqvm); // dummy bound class holder
  G_VERIFY(SQ_SUCCEEDED(sqstd_register_bloblib(sqvm)));
  sq_pop(sqvm, 1); // dummy bound class holder

  storage = Sqrat::Table(sqvm);
}


void shutdown()
{
  G_ASSERT_RETURN(sqvm, );
  storage.Release();
  sq_close(sqvm);
  sqvm = nullptr;
}


static bool parse_path(HSQUIRRELVM vm, SQInteger idx, Tab<const char *> &path)
{
  path.clear();

  SQObjectType argType = sq_gettype(vm, idx);
  if (argType == OT_STRING)
  {
    path.resize(1);
    SQInteger strLen = -1;
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, idx, &path[0], &strLen)));
    if (strLen < 1)
    {
      sq_throwerror(vm, "Path must not be empty");
      return false;
    }
    return true;
  }
  else if (argType == OT_ARRAY)
  {
    SQInteger prevTop = sq_gettop(vm);
    SQInteger len = sq_getsize(vm, idx);
    if (len < 1)
    {
      sq_throwerror(vm, "path array must not be empty");
      return false;
    }
    path.resize(len);
    for (int iElem = 0; iElem < len; ++iElem)
    {
      sq_pushinteger(vm, iElem);
      sq_rawget_noerr(vm, idx);
      SQObjectType elemType = sq_gettype(vm, -1);
      if (elemType != OT_STRING)
      {
        sqstd_throwerrorf(vm, "Elem #%d of 'path' type: %s, must be string", iElem, sq_objtypestr(elemType));
        return false;
      }
      SQInteger strLen = -1;
      G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, -1, &path[iElem], &strLen)));
      if (strLen < 1)
      {
        sqstd_throwerrorf(vm, "path element %d is empty", iElem);
        return false;
      }
      sq_pop(vm, 1);
    }
    G_UNUSED(prevTop);
    G_ASSERT(sq_gettop(vm) == prevTop);
    return true;
  }
  else
  {
    sqstd_throwerrorf(vm, "Unexpected 'path' type: %d, must be string or array of strings", sq_objtypestr(argType));
    return false;
  }
}


static bool verified_write_op(SQRESULT res, HSQUIRRELVM vm_req, const char *key)
{
  if (SQ_SUCCEEDED(res))
    return true;

  sq_getlasterror(sqvm);
  const char *errMsg = "Unknown error";
  if (SQ_SUCCEEDED(sq_tostring(sqvm, -1)))
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(sqvm, -1, &errMsg)));
  sqstd_throwerrorf(vm_req, "Error '%s' while writing at path '%s'", errMsg, key);
  return false;
}


static SQInteger write(HSQUIRRELVM vm_req)
{
  Tab<const char *> path(framemem_ptr());
  if (!parse_path(vm_req, 2, path))
    return SQ_ERROR;

  SqStackChecker stackCheck(sqvm);

  sq_pushobject(sqvm, storage);

  for (int i = 0, n = path.size(); i < n - 1 /*Not including last */; ++i)
  {
    sq_pushstring(sqvm, path[i], -1);
    if (SQ_SUCCEEDED(sq_rawget_noerr(sqvm, -2)) && sq_gettype(sqvm, -1) != OT_NULL)
    {
      // replace target container on stack
      sq_remove(sqvm, -2);
    }
    else // non-existing key
    {
      sq_pushstring(sqvm, path[i], -1);
      sq_newtable(sqvm);
      HSQOBJECT tbl;
      sq_getstackobj(sqvm, -1, &tbl);
      if (!verified_write_op(sq_rawset(sqvm, -3), vm_req, path[i]))
      {
        stackCheck.restore();
        return SQ_ERROR;
      }
      sq_pop(sqvm, 1); // prev containeer
      sq_pushobject(sqvm, tbl);
    }
  }

  sq_pushstring(sqvm, path[path.size() - 1], -1);
  stackCheck.check(2); // contaner and key from path

  String errMsg;
  if (!sq_cross_push(vm_req, 3, sqvm, &errMsg))
  {
    stackCheck.restore();
    return sq_throwerror(vm_req, errMsg);
  }

  if (!verified_write_op(sq_rawset(sqvm, -3), vm_req, path[path.size() - 1]))
  {
    stackCheck.restore();
    return SQ_ERROR;
  }

  stackCheck.restore();

  return 0;
}


static SQInteger read_impl(HSQUIRRELVM vm_req, bool error_on_missing)
{
  Tab<const char *> path(framemem_ptr());
  if (!parse_path(vm_req, 2, path))
    return SQ_ERROR;

  SqStackChecker stackCheck(sqvm);

  sq_pushobject(sqvm, storage);

  for (int i = 0, n = path.size(); i < n; ++i)
  {
    const char *pathElem = path[i];
    sq_pushstring(sqvm, pathElem, -1);
    if (SQ_FAILED(sq_rawget_noerr(sqvm, -2))) // non-existing key
    {
      stackCheck.restore();
      if (error_on_missing)
      {
        stackCheck.restore();
        if (i == 0)
          return sqstd_throwerrorf(vm_req, "Key '%s' does not exist", pathElem);
        else
        {
          String traversedPath(framemem_ptr());
          for (int j = 0; j < i; ++j)
          {
            if (j)
              traversedPath.append("/", 1);
            traversedPath.append(path[j]);
          }
          return sqstd_throwerrorf(vm_req, "Key '%s' does not exist in '%s'", pathElem, traversedPath.c_str());
        }
      }
      else
      {
        stackCheck.restore();
        return 0;
      }
    }
    sq_remove(sqvm, -2);
  }


  String errMsg;
  bool ok = sq_cross_push(sqvm, -1, vm_req, &errMsg);
  sq_pop(sqvm, 1);

  if (!ok)
    return sq_throwerror(vm_req, errMsg);
  return 1;
}

static SQInteger read(HSQUIRRELVM vm_req) { return read_impl(vm_req, true); }

static SQInteger try_read(HSQUIRRELVM vm_req) { return read_impl(vm_req, false); }

static SQInteger exists(HSQUIRRELVM vm_req)
{
  Tab<const char *> path(framemem_ptr());
  if (!parse_path(vm_req, 2, path))
    return SQ_ERROR;

  SqStackChecker stackCheck(sqvm);

  sq_pushobject(sqvm, storage);

  for (const char *pathElem : path)
  {
    sq_pushstring(sqvm, pathElem, -1);
    if (SQ_FAILED(sq_rawget_noerr(sqvm, -2))) // non-existing key
    {
      stackCheck.restore();
      sq_pushbool(vm_req, SQFalse);
      return 1;
    }

    sq_remove(sqvm, -2);
  }

  stackCheck.restore();
  sq_pushbool(vm_req, SQTrue);
  return 1;
}


static SQInteger del(HSQUIRRELVM vm_req)
{
  Tab<const char *> path(framemem_ptr());
  if (!parse_path(vm_req, 2, path))
    return SQ_ERROR;

  SqStackChecker stackCheck(sqvm);

  sq_pushobject(sqvm, storage);

  for (int i = 0, n = path.size(); i < n; ++i)
  {
    const char *pathElem = path[i];

    sq_pushstring(sqvm, pathElem, -1);

    if (i == n - 1)
    {
      bool ok = SQ_SUCCEEDED(sq_deleteslot(sqvm, -2, SQFalse));
      stackCheck.restore();
      sq_pushbool(vm_req, ok);
      return 1;
    }
    else
    {
      if (SQ_FAILED(sq_rawget_noerr(sqvm, -2))) // non-existing key
      {
        stackCheck.restore();
        sq_pushbool(vm_req, SQFalse);
        return 1;
      }

      sq_remove(sqvm, -2);
    }
  }

  // should not get here
  stackCheck.restore();
  return sq_throwerror(vm_req, "Internal error");
}

///@module nestdb
void bind_api(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  exports //
    .SquirrelFunc("ndbWrite", write, 3, ". s|a .")
    .SquirrelFunc("ndbRead", read, 2, ". s|a")
    .SquirrelFunc("ndbTryRead", try_read, 2, ". s|a")
    .SquirrelFunc("ndbExists", exists, 2, ". s|a")
    .SquirrelFunc("ndbDelete", del, 2, ". s|a")
    /**/;
  module_mgr->addNativeModule("nestdb", exports);
}

} // namespace nestdb
