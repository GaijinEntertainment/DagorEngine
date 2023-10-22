#include "sqmodules.h"
#include "path.h"

#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>
#include <sqstdaux.h>
#include <sqdirect.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#  define _access access
#endif

const char* SqModules::__main__ = "__main__";
const char* SqModules::__fn__ = nullptr;


static SQInteger persist_state(HSQUIRRELVM vm)
{
  // arguments:
  // 1 - module 'this', 2 - slot id, 3 - intitializer, 4 - state storage
  SQRAT_ASSERT(sq_gettype(vm, 4) == OT_TABLE);

  const SQChar *slotName;
  sq_getstring(vm, 2, &slotName);

  sq_push(vm, 2); // slot id @ 5
  if (SQ_SUCCEEDED(sq_get_noerr(vm, 4)))
    return 1;

  HSQOBJECT mState;
  sq_getstackobj(vm, 4, &mState);

  sq_push(vm, 3); // initializer
  sq_push(vm, 1); // module 'this'
  if (SQ_FAILED(sq_call(vm, 1, SQTrue, SQTrue)))
    return sq_throwerror(vm, _SC("Failed to call initializer"));

  SQObjectType tp = sq_gettype(vm, -1);
  bool isMutable = tp==OT_TABLE || tp==OT_ARRAY || tp==OT_USERDATA || tp==OT_CLASS || tp==OT_INSTANCE;
  if (!isMutable)
  {
    const SQChar *slotName = nullptr;
    if (sq_getstring(vm, 2, &slotName))
      return sqstd_throwerrorf(vm, _SC("Persist '%s' is not mutable, probably an error"), slotName);
    else
      return sq_throwerror(vm, _SC("Persist is not mutable, probably an error"));
  }

  HSQOBJECT res;
  sq_getstackobj(vm, -1, &res);
  sq_addref(vm, &res);
  sq_pop(vm, 1); // initializer closure

  sq_push(vm, 2); // slot id
  sq_pushobject(vm, res); // initializer result
  sq_newslot(vm, 4, false);

  sq_pushobject(vm, res);
  sq_release(vm, &res);

  return 1;
}


static SQInteger keepref(HSQUIRRELVM vm)
{
  // arguments: this, object, ref holder
  sq_push(vm, 2); // push object
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_arrayappend(vm, 3))); // append to ref holder

  sq_push(vm, 2); // push object again
  return 1;
}

void SqModules::resolveFileName(const char *requested_fn, string &res)
{
  res.clear();

  std::vector<char> scriptPathBuf;

  // try relative path first
  if (!runningScripts.empty())
  {
    const string &curFile = runningScripts.back();
    scriptPathBuf.resize(curFile.length()+2);
    dd_get_fname_location(&scriptPathBuf[0], curFile.c_str());
    dd_append_slash_c(&scriptPathBuf[0]);
    size_t locationLen = strlen(&scriptPathBuf[0]);
    size_t reqFnLen = strlen(requested_fn);
    scriptPathBuf.resize(locationLen + reqFnLen + 1);
    strcpy(&scriptPathBuf[locationLen], requested_fn);
    dd_simplify_fname_c(&scriptPathBuf[0]);
    scriptPathBuf.resize(strlen(&scriptPathBuf[0])+1);
    bool exists = _access(&scriptPathBuf[0], 0) == 0;
    if (exists)
      res.insert(res.end(), scriptPathBuf.begin(), scriptPathBuf.end());
  }

  if (res.empty())
  {
    scriptPathBuf.resize(strlen(requested_fn)+1);
    strcpy(&scriptPathBuf[0], requested_fn);
    dd_simplify_fname_c(&scriptPathBuf[0]);
    res.insert(res.end(), scriptPathBuf.begin(), scriptPathBuf.end());
  }
}


bool SqModules::checkCircularReferences(const char* resolved_fn, const char *)
{
  for (const string& scriptFn : runningScripts)
    if (scriptFn == resolved_fn)
      return false;
  return true;
}

enum FileAccessError {
  FAE_OK,
  FAE_NOT_FOUND,
  FAE_NO_READ
};

static enum FileAccessError readFileContent(const char *resolved_fn, std::vector<char> &buf)
{
  FILE* f = fopen(resolved_fn, "rb");
  if (!f)
  {
    return FAE_NOT_FOUND;
  }

#ifdef _WIN32
  long len = _filelength(_fileno(f));
#else
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  if (len < 0)
  {
    fclose(f);
    return FAE_NO_READ;
  }

  fseek(f, 0, SEEK_SET);
#endif

  buf.resize(len + 1);
  fread(&buf[0], 1, len, f);
  buf[len] = 0;
  fclose(f);

  return FAE_OK;
}

struct ASTDataGuard
{
  SQCompilation::SqASTData *data;
  HSQUIRRELVM vm;

  ASTDataGuard(HSQUIRRELVM vm_, SQCompilation::SqASTData *astData) : vm(vm_), data(astData) {}

  ~ASTDataGuard()
  {
    sq_releaseASTData(vm, data);
  }
};

bool SqModules::compileScriptImpl(const std::vector<char> &buf, const char *sourcename, const HSQOBJECT *bindings)
{
  if (compilationOptions.doStaticAnalysis)
  {
    sq_checktrailingspaces(sqvm, sourcename, &buf[0], buf.size());
  }

  auto *ast = sq_parsetoast(sqvm, &buf[0], buf.size() - 1, sourcename, compilationOptions.doStaticAnalysis, compilationOptions.raiseError);
  if (!ast)
  {
    return true;
  }
  else
  {
    if (onAST_cb)
      onAST_cb(sqvm, ast, up_data);
  }

  ASTDataGuard g(sqvm, ast);

  if (SQ_FAILED(sq_translateasttobytecode(sqvm, ast, bindings, &buf[0], buf.size(), compilationOptions.raiseError, compilationOptions.debugInfo)))
  {
    return true;
  }

  if (compilationOptions.doStaticAnalysis)
  {
    sq_analyseast(sqvm, ast, bindings, &buf[0], buf.size());
  }

  if (onBytecode_cb) {
    HSQOBJECT func;
    sq_getstackobj(sqvm, -1, &func);
    onBytecode_cb(sqvm, func, up_data);
  }

  return false;
}

#ifdef _WIN32
#define MAX_PATH_LENGTH _MAX_PATH
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  return _fullpath(buffer, resolved_fn, size);
}
#else // _WIN32
#define MAX_PATH_LENGTH PATH_MAX
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  (void)size;
  return realpath(resolved_fn, buffer);
}
#endif // _WIN32

SqModules::CompileScriptResult SqModules::compileScript(const char *resolved_fn, const char *requested_fn,
                                                        const HSQOBJECT *bindings,
                                                        Sqrat::Object &script_closure, string &out_err_msg)
{
  script_closure.release();
  std::vector<char> buf;

  auto r = readFileContent(resolved_fn, buf);
  if (r != FAE_OK) {
    out_err_msg = string(r == FAE_NOT_FOUND ? "Script file not found: " : "Cannot read script file: ") + requested_fn + " / " + resolved_fn;
    return CompileScriptResult::FileNotFound;
  }

  const char *filePath = resolved_fn;
  char buffer[MAX_PATH_LENGTH] = {0};

  if (compilationOptions.useAbsolutePath) {
    const char *r = computeAbsolutePath(resolved_fn, buffer, MAX_PATH_LENGTH);
    if (r)
      filePath = r;
  }

  if (compileScriptImpl(buf, filePath, bindings))
  {
    out_err_msg = string("Failed to compile file: ") + requested_fn + " / " + resolved_fn;
    return CompileScriptResult::CompilationFailed;
  }

  script_closure.attachToStack(sqvm, -1);
  sq_pop(sqvm, 1);

  return CompileScriptResult::Ok;
}


Sqrat::Object SqModules::setupStateStorage(const char* resolved_fn)
{
  SQRAT_ASSERT(sqvm);

  for (const Module &prevMod : prevModules)
    if (dd_fname_equal(prevMod.fn.c_str(), resolved_fn))
      return prevMod.stateStorage;

  HSQOBJECT ho;
  sq_newtable(sqvm);
  sq_getstackobj(sqvm, -1, &ho);
  Sqrat::Object res(sqvm, ho);
  sq_poptop(sqvm);
  return res;
}

SqModules::Module * SqModules::findModule(const char * resolved_fn)
{
  for (Module &m : modules)
    if (dd_fname_equal(resolved_fn, m.fn.c_str()))
      return &m;

  return nullptr;
}


void SqModules::bindBaseLib(HSQOBJECT bindings)
{
  sq_pushobject(sqvm, bindings);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_registerbaselib(sqvm)));
  sq_poptop(sqvm);
}


void SqModules::bindRequireApi(HSQOBJECT bindings)
{
  sq_pushobject(sqvm, bindings);

  sq_pushstring(sqvm, _SC("require"), 7);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<true>, 1);
  sq_setparamscheck(sqvm, 2, _SC(".s"));
  sq_rawset(sqvm, -3);

  sq_pushstring(sqvm, _SC("require_optional"), 16);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<false>, 1);
  sq_setparamscheck(sqvm, 2, _SC(".s"));
  sq_rawset(sqvm, -3);

  sq_poptop(sqvm); // bindings
}


bool SqModules::requireModule(const char *requested_fn, bool must_exist, const char *__name__,
                              Sqrat::Object &exports, string &out_err_msg)
{
  out_err_msg.clear();
  exports.release();

  string resolvedFn;
  resolveFileName(requested_fn, resolvedFn);

  if (!checkCircularReferences(resolvedFn.c_str(), requested_fn))
  {
    out_err_msg = "Circular references error";
    // ^ will not be needed because of fatal() but keep it for the case of future changes
    return false;
  }

  if (Module * found = findModule(resolvedFn.c_str()))
  {
    exports = found->exports;
    return true;
  }

  HSQUIRRELVM vm = sqvm;
  SQInteger prevTop = sq_gettop(sqvm);
  (void)prevTop;


  sq_newtable(vm);
  HSQOBJECT hBindings;
  sq_getstackobj(vm, -1, &hBindings);
  Sqrat::Object bindingsTbl(vm, hBindings); // add ref
  Sqrat::Object stateStorage = setupStateStorage(resolvedFn.c_str());

  SQRAT_ASSERT(sq_gettop(vm) == prevTop+1); // bindings table

  sq_pushstring(vm, "persist", 7);
  sq_pushobject(vm, stateStorage.o);
  sq_newclosure(vm, persist_state, 1);
  sq_setparamscheck(vm, 3, ".sc");
  sq_rawset(vm, -3);

  sq_newarray(vm, 0);
  Sqrat::Object refHolder;
  refHolder.attachToStack(vm, -1);
  sq_poptop(vm);

  sq_pushstring(vm, "keepref", 7);
  sq_pushobject(vm, refHolder.o);
  sq_newclosure(vm, keepref, 1);
  sq_rawset(vm, -3);


  sq_pushstring(vm, "__name__", 8);
  sq_pushstring(vm, __name__, -1);
  sq_rawset(vm, -3);

  SQRAT_ASSERT(sq_gettop(vm) == prevTop+1); // bindings table

  bindBaseLib(hBindings);
  bindRequireApi(hBindings);

  SQRAT_ASSERT(sq_gettop(vm) == prevTop+1); // bindings table
  sq_poptop(vm); //bindings table


  Sqrat::Object scriptClosure;
  CompileScriptResult res = compileScript(resolvedFn.c_str(), requested_fn, &hBindings, scriptClosure, out_err_msg);
  if (!must_exist && res == CompileScriptResult::FileNotFound)
  {
    exports.release();
    return true;
  }
  if (res != CompileScriptResult::Ok)
    return false;

  if (__name__ == __fn__)
    __name__ = resolvedFn.c_str();

  size_t rsIdx = runningScripts.size();
  runningScripts.emplace_back(resolvedFn.c_str());


  sq_pushobject(vm, scriptClosure.o);
  sq_newtable(vm);

  SQRAT_ASSERT(sq_gettype(vm, -1) == OT_TABLE);
  Sqrat::Object objThis;
  objThis.attachToStack(vm, -1);


  SQRESULT callRes = sq_call(vm, 1, true, true);

  (void)rsIdx;
  SQRAT_ASSERT(runningScripts.size() == rsIdx+1);
  runningScripts.pop_back();

  if (SQ_FAILED(callRes))
  {
    out_err_msg = string("Failed to run script ") + requested_fn + " / " + resolvedFn;
    sq_pop(vm, 1); // clojure, no return value on error
    return false;
  }

  HSQOBJECT hExports;
  sq_getstackobj(vm, -1, &hExports);
  exports = Sqrat::Object(vm, hExports);

  sq_pop(vm, 2); // retval + closure

  SQRAT_ASSERT(sq_gettop(vm) == prevTop);

  Module module;
  module.exports = exports;
  module.fn = resolvedFn;
  module.stateStorage = stateStorage;
  module.moduleThis = objThis;
  module.refHolder = refHolder;
  module.__name__ = __name__;

  modules.push_back(module);

  return true;
}


bool SqModules::reloadModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, string &out_err_msg)
{
  SQRAT_ASSERT(prevModules.empty());

  modules.swap(prevModules);
  modules.clear(); // just in case

  string errMsg;
  bool res = requireModule(fn, must_exist, __name__, exports, out_err_msg);

  prevModules.clear();

  return res;
}


bool SqModules::reloadAll(string &full_err_msg)
{
  SQRAT_ASSERT(prevModules.empty());
  full_err_msg.clear();

  modules.swap(prevModules);
  modules.clear(); // just in case

  bool res = true;
  Sqrat::Object exportsTmp;
  string errMsg;
  for (const Module &prev : prevModules)
  {
    if (!requireModule(prev.fn.c_str(), true, prev.__name__.c_str(), exportsTmp, errMsg))
    {
      res = false;
      full_err_msg += prev.fn + ": " + errMsg + "\n";
    }
  }

  prevModules.clear();

  return res;
}


bool SqModules::addNativeModule(const char *module_name, const Sqrat::Object &exports)
{
  if (!module_name || !*module_name)
    return false;
  auto ins = nativeModules.insert({string(module_name), exports});
  return ins.second; // false if already registered
}


template<bool must_exist> SQInteger SqModules::sqRequire(HSQUIRRELVM vm)
{
  SQUserPointer selfPtr = nullptr;
  if (SQ_FAILED(sq_getuserpointer(vm, -1, &selfPtr)) || !selfPtr)
    return sq_throwerror(vm, "No module manager");

  SqModules *self = reinterpret_cast<SqModules *>(selfPtr);
  SQRAT_ASSERT(self->sqvm == vm);

  const char *fileName = nullptr;
  SQInteger fileNameLen = -1;
  sq_getstringandsize(vm, 2, &fileName, &fileNameLen);

  if (strcmp(fileName, "quirrel.native_modules")==0)
  {
    sq_newarray(vm, self->nativeModules.size());
    SQInteger idx = 0;
    for (auto &nm : self->nativeModules)
    {
      sq_pushinteger(vm, idx);
      sq_pushstring(vm, nm.first.data(), -1);
      sq_set(vm, -3);
      ++idx;
    }
    return 1;
  }

  auto nativeIt = self->nativeModules.find(fileName);
  if (nativeIt != self->nativeModules.end())
  {
    sq_pushobject(vm, nativeIt->second.o);
    return 1;
  }

  Sqrat::Object exports;
  string errMsg;
  if (!self->requireModule(fileName, must_exist, __fn__, exports, errMsg))
    return sq_throwerror(vm, errMsg.c_str());

  sq_pushobject(vm, exports.o);
  return 1;
}


void SqModules::registerStdLibNativeModule(const char *name, RegFunc reg_func)
{
  HSQOBJECT hModule;
  sq_newtable(sqvm);
  sq_getstackobj(sqvm, -1, &hModule);
  reg_func(sqvm);
  bool regRes = addNativeModule(name, Sqrat::Object(sqvm, hModule));
  SQRAT_ASSERT(regRes);
  sq_pop(sqvm, 1);
}


void SqModules::registerMathLib()
{
  registerStdLibNativeModule("math", sqstd_register_mathlib);
}

void SqModules::registerStringLib()
{
  registerStdLibNativeModule("string", sqstd_register_stringlib);
}

void SqModules::registerSystemLib()
{
  registerStdLibNativeModule("system", sqstd_register_systemlib);
}

void SqModules::registerDateTimeLib()
{
  registerStdLibNativeModule("datetime", sqstd_register_datetimelib);
}

void SqModules::registerIoStreamLib()
{
  HSQOBJECT hModule;
  sq_resetobject(&hModule);
  sq_newtable(sqvm);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(sqvm, -1, &hModule)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sqstd_init_streamclass(sqvm)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sqstd_register_bloblib(sqvm)));
  bool regRes = addNativeModule("iostream", Sqrat::Object(sqvm, hModule));
  (void)(regRes);
  SQRAT_ASSERTF(regRes, "Failed to init 'iostream' library with module manager");
  sq_pop(sqvm, 1);
}

void SqModules::registerIoLib()
{
  if (!findNativeModule("iostream"))
  {
    // register 'iostream' module containing basestream class and blob
    registerIoStreamLib();
  }

  registerStdLibNativeModule("io", sqstd_register_iolib);
}
