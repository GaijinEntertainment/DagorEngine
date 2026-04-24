#include "sqmodules.h"

#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>
#include <sqstddebug.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>
#include <sqstdaux.h>

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <cstdarg>

#include "helpers.h"

#ifndef G_UNUSED
  #define G_UNUSED(x)    ((void)(x))
#endif

const char *SqModules::__main__ = "__main__";
const char *SqModules::__fn__ = nullptr;
const char *SqModules::__analysis__ = "__analysis__";


using namespace sqm;

static sqm::span<char> skip_bom(sqm::span<char> buf)
{
  static constexpr uint8_t utf8_bom[] = {0xef, 0xbb, 0xbf};

  bool isBom = (buf.size() >= sizeof(utf8_bom)) && memcmp(buf.data(), utf8_bom, sizeof(utf8_bom))==0;
  int utf8MarkOffset = isBom ? sizeof(utf8_bom) : 0;
  return sqm::make_span<char>(buf.data() + utf8MarkOffset, buf.size() - utf8MarkOffset);
}


static SQInteger persist_state(HSQUIRRELVM vm)
{
  HSQOBJECT hModuleThis, hSlot, hCtor, hMstate;
  sq_getstackobj(vm, 1, &hModuleThis);
  sq_getstackobj(vm, 2, &hSlot);
  sq_getstackobj(vm, 3, &hCtor);
  sq_getstackobj(vm, 4, &hMstate);

  Sqrat::Table mstate(hMstate, vm);
  SQRAT_ASSERTF(mstate.GetType() == OT_TABLE, "type = %X", mstate.GetType());

  Sqrat::Object slot(hSlot, vm);
  if (mstate.RawHasKey(slot))
  {
    Sqrat::Object curVal = mstate.GetSlot(slot);
    sq_pushobject(vm, curVal);
    return 1;
  }

  Sqrat::Object ctor(hCtor, vm);
  Sqrat::Function f(vm, Sqrat::Object(hModuleThis, vm), ctor);
  auto callRes = f.Eval<Sqrat::Object>();
  if (!callRes)
    return sq_throwerror(vm, "Failed to call initializer");
  Sqrat::Object &res = callRes.value();

  SQObjectType tp = res.GetType();
  bool isMutable = tp == OT_TABLE || tp == OT_ARRAY || tp == OT_USERDATA || tp == OT_CLASS || tp == OT_INSTANCE;
  if (!isMutable)
  {
    if (sq_isstring(hSlot))
      return sqstd_throwerrorf(vm, "Persist '%s' is not mutable, probably an error", sq_objtostring(&hSlot));
    else
      return sq_throwerror(vm, "Persist is not mutable, probably an error");
  }

  mstate.SetValue(slot, res);

  sq_pushobject(vm, res);
  return 1;
}


static SQInteger keepref(HSQUIRRELVM vm)
{
  // arguments: this, object, ref holder
  SQRAT_ASSERT(sq_gettype(vm, 3) == OT_ARRAY);
  sq_push(vm, 2);                                    // push object
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_arrayappend(vm, 3))); // append to ref holder

  sq_push(vm, 2); // push object again
  return 1;
}


SqModules::SqModules(HSQUIRRELVM vm, ISqModulesFileAccess *file_access)
  : sqvm(vm), fileAccess(file_access),
  beforeImportModuleCb(nullptr), up_data(nullptr), onAST_cb(nullptr), onBytecode_cb(nullptr)
{
  compilationOptions.raiseError = true;
  compilationOptions.doStaticAnalysis = false;

  sq_enablesyntaxwarnings(false);

  registerTypesLib();
  registerModulesLib();
}


SqModules::~SqModules()
{
  modules.swap(prevModules);
  modules.clear();

  callAndClearUnloadHandlers(true);

  prevModules.clear();

  fileAccess->destroy();
}


bool SqModules::checkCircularReferences(const string &resolved_fn, const char *requested_fn, string &out_err_msg)
{
  for (const string &scriptFn : runningScripts)
  {
    if (scriptFn == resolved_fn)
    {
      out_err_msg = format_string("Required script %s (%s) is currently being executed, possible circular require() call.\n"
        "require() stack:",
        requested_fn, resolved_fn.c_str());

      for (const string &stackItem : runningScripts)
        append_format(out_err_msg, "\n * %s", stackItem.c_str());

      return false;
    }
  }
  return true;
}


bool SqModules::compileScriptImpl(const sqm::span<char> buf, const char *resolved_fn, Sqrat::Table &bindings,
  string &out_err_msg, SQCompilation::SqASTData **return_ast)
{
  out_err_msg.clear();

  if (onCompileFile_cb)
    onCompileFile_cb(sqvm, resolved_fn);

  SQCompilation::SqASTData *ast =
    sq_parsetoast(sqvm, buf.data(), buf.size(), resolved_fn, compilationOptions.doStaticAnalysis, compilationOptions.raiseError);

  if (return_ast)
    *return_ast = ast;

  if (!ast)
    return false;

  if (onAST_cb)
    onAST_cb(sqvm, ast, up_data);

  SQModuleImport *imports = nullptr;
  SQInteger numImports = 0;
  bool importsRes = SQ_SUCCEEDED(sq_getimports(sqvm, ast, &numImports, &imports));
  SQRAT_ASSERT(importsRes);
  G_UNUSED(importsRes);

  bool importRes = importModules(resolved_fn, bindings, imports, numImports, out_err_msg);
  sq_freeimports(sqvm, numImports, imports);

  if (!importRes)
  {
    sq_releaseASTData(sqvm, ast);
    if (return_ast)
      *return_ast = nullptr;
    return false;
  }

  HSQOBJECT hBindings = bindings.GetObject();
  hBindings._flags = SQOBJ_FLAG_IMMUTABLE;

  if (SQ_FAILED(sq_translateasttobytecode(sqvm, ast, &hBindings, &buf[0], buf.size(), compilationOptions.raiseError)))
  {
    sq_releaseASTData(sqvm, ast);
    if (return_ast)
      *return_ast = nullptr;
    return false;
  }

  if (compilationOptions.doStaticAnalysis && !return_ast)
  {
    sq_analyzeast(sqvm, ast, &hBindings, buf.data(), buf.size());
  }

  if (onBytecode_cb)
  {
    HSQOBJECT func;
    sq_getstackobj(sqvm, -1, &func);
    onBytecode_cb(sqvm, func, up_data);
  }

  if (!return_ast)
    sq_releaseASTData(sqvm, ast);
  return true;
}


bool SqModules::compileScript(const sqm::span<char> buf, const string &resolved_fn, const char *requested_fn, Sqrat::Table &bindings,
  Sqrat::Object &script_closure, string &out_err_msg, SQCompilation::SqASTData **return_ast)
{
  script_closure.Release();

  string compileErrMsg;
  if (!compileScriptImpl(buf, resolved_fn.c_str(), bindings, compileErrMsg, return_ast))
  {
    out_err_msg = format_string("Failed to compile file %s (%s)", requested_fn, resolved_fn.c_str());
    if (!compileErrMsg.empty())
      append_format(out_err_msg, ": %s", compileErrMsg.c_str());
    return false;
  }

  script_closure = Sqrat::Var<Sqrat::Object>(sqvm, -1).value;
  sq_pop(sqvm, 1);

  return true;
}


Sqrat::Table SqModules::setupStateStorage(const char *resolved_fn)
{
  SQRAT_ASSERT(sqvm);

  for (const Module &prevMod : prevModules)
    if (prevMod.fn == resolved_fn)
      return prevMod.stateStorage;

  return Sqrat::Table(sqvm);
}

SqModules::Module *SqModules::findModule(const char *resolved_fn)
{
  for (Module &m : modules)
    if (m.fn == resolved_fn)
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

  sq_pushstring(sqvm, "require", 7);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<true>, 1);
  sq_setnativeclosurename(sqvm, -1, "require");
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(sqvm, 2, ".s")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(sqvm, -3)));

  sq_pushstring(sqvm, "require_optional", 16);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<false>, 1);
  sq_setnativeclosurename(sqvm, -1, "require_optional");
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(sqvm, 2, ".s")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(sqvm, -3)));

  sq_poptop(sqvm); // bindings
}


void SqModules::bindModuleApi(HSQOBJECT bindings, Sqrat::Table &state_storage, Sqrat::Array &ref_holder, const char *__name__,
  const char *__filename__)
{
  SQRAT_ASSERT(__name__);
  SQRAT_ASSERT(__filename__);

  HSQUIRRELVM vm = sqvm;

  sq_pushobject(vm, bindings);

  sq_pushstring(vm, "persist", 7);
  sq_pushobject(vm, state_storage.GetObject());
  sq_newclosure(vm, persist_state, 1);
  sq_setnativeclosurename(vm, -1, "persist");
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 3, ".sc")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pushstring(vm, "keepref", 7);
  sq_pushobject(vm, ref_holder.GetObject());
  sq_newclosure(vm, keepref, 1);
  sq_setnativeclosurename(vm, -1, "keepref");
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pushstring(vm, "__name__", 8);
  sq_pushstring(vm, __name__, -1);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pushstring(vm, "__filename__", 12);
  sq_pushstring(vm, __filename__, -1);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pushstring(vm, "__static_analysis__", 19);
  sq_pushbool(vm, compilationOptions.doStaticAnalysis);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pop(vm, 1); // bindings table
}


bool SqModules::mergeBindings(Sqrat::Table &target, Sqrat::Object &key, Sqrat::Object &value, string &out_err_msg)
{
  if (target.RawHasKey(key))
  {
    string keyStr = key.Cast<string>();
    Sqrat::Object current = target.RawGetSlot(key);
    if (value.IsEqual(current))
      return true;
    else
    {
      out_err_msg = format_string("Duplicate field '%s' in imported bindings", keyStr.c_str());
      return false;
    }
  }
  else
  {
    target.SetValue(key, value);
    return true;
  }
}


bool SqModules::mergeBindings(Sqrat::Table &target, Sqrat::Table &upd, string &out_err_msg)
{
  Sqrat::Object::iterator it;

  while (upd.Next(it))
  {
    Sqrat::Object key(it.getKey(), sqvm);
    Sqrat::Object val(it.getValue(), sqvm);
    if (!mergeBindings(target, key, val, out_err_msg))
      return false;
  }

  return true;
}

bool SqModules::importModules(const char *resolved_fn, Sqrat::Table &bindings_dest, const SQModuleImport *imports, int num_imports,
  string &out_err_msg)
{
  if (!num_imports)
    return true;

  size_t rsIdx = runningScripts.size();
  runningScripts.emplace_back(resolved_fn);

  bool success = true;
  string mergeErrorMsg, requireErrorMsg;

  for (int iImp = 0; iImp < num_imports; ++iImp)
  {
    const SQModuleImport &import = imports[iImp];
    Sqrat::Object exports;
    string moduleFnStr(import.name);

    if (beforeImportModuleCb && !beforeImportModuleCb(this, moduleFnStr.c_str()))
    {
      out_err_msg = format_string("%s:%d:%d: Module '%s' is forbidden", resolved_fn, import.line, import.nameColumn, moduleFnStr.c_str());
      success = false;
      break;
    }

    auto nativeIt = nativeModules.find(import.name);
    if (nativeIt != nativeModules.end())
      exports = nativeIt->second;
    else if (!requireModule(moduleFnStr.c_str(), true, __fn__, exports, requireErrorMsg))
    {
      out_err_msg = format_string("Import error at %s:%d:%d: %s", resolved_fn, import.line, import.nameColumn, requireErrorMsg.c_str());
      success = false;
      break;
    }

    if (import.numSlots == 0) // import module as slot
    {
      Sqrat::Object key(import.alias ? import.alias : import.name, sqvm);
      if (!mergeBindings(bindings_dest, key, exports, mergeErrorMsg))
      {
        success = false;
        out_err_msg = format_string("%s:%d:%d: %s", resolved_fn, import.line, import.nameColumn, mergeErrorMsg.c_str());
      }
    }
    else // import fields
    {
      if (exports.GetType() != OT_TABLE && exports.GetType() != OT_CLASS)
      {
        out_err_msg = format_string("%s:%d:%d: module '%s' export is not a table or class", resolved_fn, import.line, import.nameColumn,
          moduleFnStr.c_str());
        success = false;
        break;
      }

      Sqrat::Table exportsTbl(exports);
      for (int iSlot = 0; iSlot < import.numSlots; ++iSlot)
      {
        const SQModuleImportSlot &slot = import.slots[iSlot];
        if (strcmp(slot.name, "*") == 0) // import all fields
        {
          if (!mergeBindings(bindings_dest, exportsTbl, mergeErrorMsg))
          {
            out_err_msg = format_string("%s:%d: %s", resolved_fn, import.line, mergeErrorMsg.c_str());
            success = false;
            break;
          }
        }
        else // import single field
        {
          Sqrat::Object srcKey(slot.name, sqvm);

          if (!exportsTbl.RawHasKey(srcKey))
          {
            out_err_msg = format_string("%s:%d:%d no field '%s' in module '%s' exports", resolved_fn, slot.line, slot.column,
              sq_objtostring(&srcKey.GetObject()), moduleFnStr.c_str());
            success = false;
            break;
          }
          else
          {
            Sqrat::Object destKey(slot.alias ? slot.alias : slot.name, sqvm);
            Sqrat::Object val = exports.GetSlot(srcKey);
            if (!mergeBindings(bindings_dest, destKey, val, mergeErrorMsg))
            {
              out_err_msg = format_string("%s:%d:%d: %s", resolved_fn, slot.line, slot.column, mergeErrorMsg.c_str());
              success = false;
              break;
            }
          }
        }
      }
    }
    if (!success)
      break;
  }

  G_UNUSED(rsIdx);
  SQRAT_ASSERT(runningScripts.size() == rsIdx + 1);
  runningScripts.pop_back();

  return success;
}


bool SqModules::requireModule(const char *requested_fn, bool must_exist, const char *__name__, Sqrat::Object &exports,
  string &out_err_msg)
{
  out_err_msg.clear();
  exports.Release();

  string resolvedFn;
  fileAccess->resolveFileName(requested_fn, runningScripts.empty() ? nullptr : runningScripts.back().c_str(), resolvedFn);

  if (!checkCircularReferences(resolvedFn, requested_fn, out_err_msg))
    return false;

  if (beforeImportModuleCb && !beforeImportModuleCb(this, requested_fn))
  {
    out_err_msg = format_string("Module '%s' is forbidden", requested_fn);
    return false;
  }

  if (Module *found = findModule(resolvedFn.c_str()))
  {
    exports = found->exports;
    return true;
  }

  vector<char> fileContents;
  bool readOk = fileAccess->readFile(resolvedFn, requested_fn, fileContents, out_err_msg);
  if (!readOk)
    return !must_exist;

  HSQUIRRELVM vm = sqvm;
  SQInteger prevTop = sq_gettop(sqvm);
  G_UNUSED(prevTop);

  SQRAT_ASSERT(__fn__ == nullptr);
  if (__name__ == __fn__)
    __name__ = resolvedFn.c_str();

  Sqrat::Table stateStorage = setupStateStorage(resolvedFn.c_str());
  Sqrat::Array refHolder(vm);

  Sqrat::Table bindingsTbl(vm);
  HSQOBJECT hBindings = bindingsTbl.GetObject();

  bindBaseLib(hBindings);
  bindModuleApi(hBindings, stateStorage, refHolder, __name__, resolvedFn.c_str());
  bindRequireApi(hBindings);

  SQRAT_ASSERT(sq_gettop(vm) == prevTop);

  sqm::span<char> sourceCode = skip_bom(sqm::make_span(fileContents));

  if (compilationOptions.doStaticAnalysis)
    sq_checktrailingspaces(vm, resolvedFn.c_str(), sourceCode.data(), sourceCode.size());

  SQCompilation::SqASTData *astData = nullptr;

  Sqrat::Object scriptClosure;
  bool compileRes = compileScript(sourceCode, resolvedFn, requested_fn, bindingsTbl, scriptClosure, out_err_msg,
    compilationOptions.doStaticAnalysis ? &astData : nullptr);

  if (!compileRes)
  {
    sq_releaseASTData(vm, astData);
    SQRAT_ASSERT(sq_gettop(vm) == prevTop);
    return false;
  }

  size_t rsIdx = runningScripts.size();
  G_UNUSED(rsIdx);

  runningScripts.emplace_back(resolvedFn.c_str());


  sq_pushobject(vm, scriptClosure.GetObject());
  sq_pushnull(vm); // module 'this'

  runningScriptClosuresStack.push_back(scriptClosure);
  SQRESULT callRes = sq_call(vm, 1, true, true);
  runningScriptClosuresStack.pop_back();

  if (SQ_FAILED(callRes))
  {
    SQRAT_ASSERT(runningScripts.size() == rsIdx + 1);
    runningScripts.pop_back();

    out_err_msg = format_string("Failed to run script %s (%s)", requested_fn, resolvedFn.c_str());
    sq_pop(vm, 1); // clojure, no return value on error
    sq_releaseASTData(vm, astData);
    SQRAT_ASSERT(sq_gettop(vm) == prevTop);
    return false;
  }

  HSQOBJECT hExports;
  sq_getstackobj(vm, -1, &hExports);
  exports = Sqrat::Object(hExports, vm);

  sq_pop(vm, 2); // retval + closure

  SQRAT_ASSERT(sq_gettop(vm) == prevTop);

  Module module;
  module.exports = exports;
  module.fn = resolvedFn;
  module.stateStorage = stateStorage;
  module.refHolder = refHolder;
  module.scriptClosure = scriptClosure;
  module.__name__ = __name__;

  modules.push_back(module);

  if (compilationOptions.doStaticAnalysis && astData)
    sq_analyzeast(vm, astData, &hBindings, sourceCode.data(), sourceCode.size());

  SQRAT_ASSERT(runningScripts.size() == rsIdx + 1);
  runningScripts.pop_back();

  sq_releaseASTData(vm, astData);
  return true;
}


bool SqModules::reloadModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, string &out_err_msg)
{
  SQRAT_ASSERT(prevModules.empty());

  modules.swap(prevModules);
  modules.clear(); // just in case

  callAndClearUnloadHandlers(false);

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

  callAndClearUnloadHandlers(false);

  bool res = true;
  Sqrat::Object exportsTmp;
  string errMsg;
  for (const Module &prev : prevModules)
  {
    if (!requireModule(prev.fn.c_str(), true, prev.__name__.c_str(), exportsTmp, errMsg))
    {
      res = false;
      append_format(full_err_msg, "%s: %s\n", prev.fn.c_str(), errMsg.c_str());
    }
  }

  prevModules.clear();

  return res;
}


bool SqModules::addNativeModule(const char *module_name, const Sqrat::Object &exports, const char *module_doc_string)
{
  if (!module_name || !*module_name)
  {
    SQRAT_ASSERT(0);
    return false;
  }

  auto ins = nativeModules.insert({string(module_name), exports});
  SQRAT_ASSERT(ins.second && "Module registered twice");

  if (module_doc_string && ins.second)
    sq_setobjectdocstring(exports.GetVM(), &const_cast<Sqrat::Object &>(exports).GetObject(), module_doc_string);

  return ins.second; // false if already registered
}


Sqrat::Array SqModules::getNativeModuleNames()
{
  Sqrat::Array list(sqvm, nativeModules.size());
  SQInteger idx = 0;
  for (auto &nm : nativeModules)
  {
    list.SetValue(idx, nm.first.data());
    ++idx;
  }
  return list;
}

void SqModules::getLoadedModules(vector<string> &file_names)
{
  file_names.clear();
  file_names.reserve(modules.size());
  for (const Module &m : modules)
    file_names.push_back(m.fn);
}


template <bool must_exist>
SQInteger SqModules::sqRequire(HSQUIRRELVM vm) // arguments: fileName, mustExist=true
{
  SQUserPointer selfPtr = nullptr;
  if (SQ_FAILED(sq_getuserpointer(vm, -1, &selfPtr)) || !selfPtr)
    return sq_throwerror(vm, "No module manager");

  SqModules *self = reinterpret_cast<SqModules *>(selfPtr);
  SQRAT_ASSERT(self->sqvm == vm);

  const char *fileName = nullptr;
  SQInteger fileNameLen = -1;
  sq_getstringandsize(vm, 2, &fileName, &fileNameLen);

  if (strcmp(fileName, "quirrel.native_modules") == 0)
  {
    Sqrat::Array list = self->getNativeModuleNames();
    sq_pushobject(vm, list);
    return 1;
  }

  if (self->beforeImportModuleCb && !self->beforeImportModuleCb(self, fileName))
    return sqstd_throwerrorf(vm, "Module '%s' is forbidden", fileName);

  bool searchNative = true, searchScript = true;
  self->fileAccess->getSearchTargets(fileName, searchNative, searchScript);

  auto nativeIt = searchNative ? self->nativeModules.find(fileName) : self->nativeModules.end();
  if (nativeIt != self->nativeModules.end())
  {
    sq_pushobject(vm, nativeIt->second);
    return 1;
  }
  if (!searchScript)
  {
    if (!must_exist)
    {
      sq_pushnull(vm);
      return 1;
    }
    else
      return sqstd_throwerrorf(vm, "Script module '%s' not found", fileName);
  }

  Sqrat::Object exports(vm);
  string errMsg;
  if (!self->requireModule(fileName, must_exist, __fn__, exports, errMsg))
    return sq_throwerror(vm, errMsg.c_str());

  sq_pushobject(vm, exports.GetObject());
  return 1;
}


void SqModules::registerStdLibNativeModule(const char *name, RegFunc reg_func)
{
  HSQOBJECT hModule;
  sq_newtable(sqvm);
  sq_getstackobj(sqvm, -1, &hModule);
  reg_func(sqvm);
  bool regRes = addNativeModule(name, Sqrat::Object(hModule, sqvm));
  G_UNUSED(regRes);
  SQRAT_ASSERT(regRes);
  sq_pop(sqvm, 1);
}


void SqModules::registerTypesLib() { registerStdLibNativeModule("types", sq_registertypeslib); }

void SqModules::registerMathLib() { registerStdLibNativeModule("math", sqstd_register_mathlib); }

void SqModules::registerStringLib() { registerStdLibNativeModule("string", sqstd_register_stringlib); }

void SqModules::registerSystemLib() { registerStdLibNativeModule("system", sqstd_register_systemlib); }

void SqModules::registerDateTimeLib() { registerStdLibNativeModule("datetime", sqstd_register_datetimelib); }

void SqModules::registerDebugLib() { registerStdLibNativeModule("debug", sqstd_register_debuglib); }

void SqModules::registerIoStreamLib()
{
  HSQOBJECT hModule;
  sq_resetobject(&hModule);
  sq_newtable(sqvm);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(sqvm, -1, &hModule)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sqstd_init_streamclass(sqvm)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sqstd_register_bloblib(sqvm)));
  bool regRes = addNativeModule("iostream", Sqrat::Object(hModule, sqvm));
  G_UNUSED(regRes);
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


void SqModules::callAndClearUnloadHandlers(bool is_closing)
{
  resetStaticMemos();

  for (const Sqrat::Object &f : onModuleUnload)
  {
    SQInteger nparams = 0, nfreevars = 0;
    sq_pushobject(sqvm, f.GetObject());
    SQRAT_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(sqvm, -1, &nparams, &nfreevars)));
    sq_pop(sqvm, 1);

    Sqrat::Function func(sqvm, Sqrat::Object(), f.GetObject());
    if (nparams == 1)
      func();
    else if (nparams == 2)
      func(is_closing);
    else
      SQRAT_ASSERT(0);
  }
  onModuleUnload.clear();
}


SQInteger SqModules::register_on_module_unload(HSQUIRRELVM vm)
{
  SQInteger nparams = 0, nfreevars = 0;
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  if (nparams != 1 && nparams != 2)
    return sqstd_throwerrorf(vm, "Function accepts 'this' + optional argument, but provided function requires %d args", nparams);

  SqModules *self = nullptr;
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getuserpointer(vm, 3, (SQUserPointer *)&self)));

  HSQOBJECT hFunc;
  sq_getstackobj(vm, 2, &hFunc);

  auto it = SQRAT_STD::find_if(self->onModuleUnload.begin(), self->onModuleUnload.end(),
    [hFunc](const Sqrat::Object &f) { return f.GetType() == sq_type(hFunc) && f.GetObject()._unVal.raw == hFunc._unVal.raw; });
  if (it == self->onModuleUnload.end())
    self->onModuleUnload.push_back(Sqrat::Object(hFunc, vm));

  return 0;
}

void SqModules::resetStaticMemos()
{
  for (Module &module : modules)
    sq_reset_static_memos(sqvm, module.scriptClosure.GetObject());

  for (Module &module : prevModules)
    sq_reset_static_memos(sqvm, module.scriptClosure.GetObject());

  for (Sqrat::Object &f : runningScriptClosuresStack)
    sq_reset_static_memos(sqvm, f.GetObject());
}


void SqModules::registerModulesLib()
{
  Sqrat::Table exports(sqvm);

  sq_pushuserpointer(sqvm, this);
  Sqrat::Var<Sqrat::Object> selfObj(sqvm, -1);
  sq_pop(sqvm, 1);

  exports
    .Func(
      "get_native_module_names", [this]() { return this->getNativeModuleNames(); },
      "Returns an array with a list of names of native modules.")
    .Func(
      "reset_static_memos", [this]() { return this->resetStaticMemos(); }, "Reset static memo expressions cache.")
    .SquirrelFunc("on_module_unload", register_on_module_unload, 2, ".c",
      "Register module unload callback. "
      "Example: on_module_unload( function(is_app_closing) {println(is_app_closing ? \"Closing\" : \"Soft reloading\")} )",
      1, &selfObj.value);

  addNativeModule("modules", exports, "Contains functions to work with modules, like on_module_unload(), get_native_module_names()");
}
