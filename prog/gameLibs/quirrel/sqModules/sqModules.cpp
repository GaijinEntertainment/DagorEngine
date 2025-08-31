// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqModules/sqModules.h>
#include <sqStackChecker.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_basePath.h>
#include <utf8/utf8.h>
#include <memory/dag_framemem.h>
#include <util/dag_strUtil.h>

#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>
#include <sqstddebug.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>

#include <stdlib.h>
#include <limits.h>

#define MODULE_PARSE_STL eastl
#include <helpers/importParser/importParser.h>

using namespace sqimportparser;


const char *SqModules::__main__ = "__main__";
const char *SqModules::__fn__ = nullptr;
const char *SqModules::__analysis__ = "__analysis__";

#if _TARGET_PC
bool SqModules::tryOpenFilesFromRealFS = true; // by default we allow loading from fs on PC.
#elif DAGOR_DBGLEVEL > 0
bool SqModules::tryOpenFilesFromRealFS = false; // by default we disallow loading from fs on non-PC platforms.
#endif


static const char SCRIPT_MODULE_FILE_EXT[] = ".nut";


static dag::Span<char> skip_bom(dag::Span<char> buf)
{
  bool isBom = (buf.size() >= sizeof(utf8::bom)) && utf8::is_bom(buf.data());
  int utf8MarkOffset = isBom ? sizeof(utf8::bom) : 0;
  return dag::Span<char>(buf.data() + utf8MarkOffset, buf.size() - utf8MarkOffset);
}


static SQInteger persist_state(HSQUIRRELVM vm)
{
  if (sq_gettop(vm) != 4)
  {
    SQRAT_ASSERTF(0, "top = %d", sq_gettop(vm));
    sq_pushnull(vm);
    return 1;
  }

  HSQOBJECT hModuleThis, hSlot, hCtor, hMstate;
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 1, &hModuleThis)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &hSlot)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 3, &hCtor)));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 4, &hMstate)));

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
  Sqrat::Object res;
  if (!f.Evaluate(res))
    return sq_throwerror(vm, "Failed to call initializer");

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
  SQRAT_ASSERTF(sq_gettype(vm, 3) == OT_ARRAY, "type = %X", sq_gettype(vm, 3));
  sq_push(vm, 2);                                    // push object
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_arrayappend(vm, 3))); // append to ref holder

  sq_push(vm, 2); // push object again
  return 1;
}


SqModules::SqModules(HSQUIRRELVM vm, SqModulesConfigBits cfg) :
  sqvm(vm), configBits(cfg), beforeImportModuleCb(nullptr), up_data(nullptr), onAST_cb(nullptr), onBytecode_cb(nullptr)
{
  compilationOptions.raiseError = true;
  compilationOptions.debugInfo = true;
  compilationOptions.doStaticAnalysis = false;
  compilationOptions.useAbsolutePath = false;

  sq_enablesyntaxwarnings(false);

  registerModulesModule();
}

SqModules::~SqModules()
{
  modules.swap(prevModules);
  modules.clear();

  callAndClearUnloadHandlers(true);

  prevModules.clear();
}

#ifdef _TARGET_PC
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
#else  // _TARGET_PC
#define MAX_PATH_LENGTH 1
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  (void)resolved_fn;
  (void)buffer;
  (void)size;
  return nullptr;
}
#endif // _TARGET_PC

void SqModules::resolveFileName(const char *requested_fn, String &res)
{
  clear_and_shrink(res);

  if (fileSystemOverride)
  {
    fileSystemOverride->resolveFileName(requested_fn, res);
    return;
  }

  bool isScriptFile = str_ends_with(requested_fn, SCRIPT_MODULE_FILE_EXT, strlen(requested_fn));
  bool modules_with_ext = (configBits & SqModulesConfigBits::PermissiveModuleNames) == SqModulesConfigBits::PermissiveModuleNames;
  // try relative path first
  if (!runningScripts.empty() && (isScriptFile || modules_with_ext))
  {
    const string &curFile = runningScripts.back();
    String scriptPath;
    scriptPath.resize(curFile.length() + 1);
    dd_get_fname_location(scriptPath, curFile.c_str());
    dd_append_slash_c(scriptPath);
    scriptPath.updateSz();
    scriptPath.append(requested_fn);
    dd_simplify_fname_c(scriptPath);
    scriptPath.updateSz();
    bool exists = dd_check_named_mount_in_path_valid(scriptPath);
    if (exists)
      exists = tryOpenFilesFromRealFS ? dd_file_exists(scriptPath) : vromfs_check_file_exists(scriptPath);
    if (!isScriptFile && exists && dd_dir_exists(scriptPath))
      exists = false;
    if (exists)
    {
      if (const char *real_fn = df_get_abs_fname(scriptPath))
        if (real_fn != scriptPath.data())
        {
          scriptPath = String(real_fn);
          real_fn = df_get_abs_fname(scriptPath); // do second check for case of mixed abs/rel base pathes
          if (real_fn != scriptPath.data())
            scriptPath = String(real_fn);
        }
      res.swap(scriptPath);
    }
  }

  if (res.empty() && dd_check_named_mount_in_path_valid(requested_fn))
  {
    res = requested_fn;
    dd_simplify_fname_c(res);
    res.updateSz();
    if (const char *real_fn = df_get_abs_fname(res))
      if (real_fn != res.data())
      {
        res = String(real_fn);
        real_fn = df_get_abs_fname(res); // do second check for case of mixed abs/rel base pathes
        if (real_fn != res.data())
          res = String(real_fn);
      }
  }

  if (compilationOptions.useAbsolutePath)
  {
    char buffer[MAX_PATH_LENGTH] = {0};
    const char *real_path = computeAbsolutePath(res.str(), buffer, sizeof buffer);
    if (real_path)
    {
      res = String(real_path);
    }
  }
}


bool SqModules::checkCircularReferences(const String &resolved_fn, const char *requested_fn)
{
  for (const string &scriptFn : runningScripts)
  {
    if (scriptFn == resolved_fn)
    {
      String requireStack;
      for (const string &stackItem : runningScripts)
        requireStack.aprintf(32, "\n * %s", stackItem.c_str());

      DAG_FATAL("Required script %s (%s) is currently being executed, possible circular require() call.\nrequire() stack:%s",
        requested_fn, resolved_fn.str(), requireStack.str());
      return false;
    }
  }
  return true;
}


bool SqModules::readFile(const String &resolved_fn, const char *requested_fn, Tab<char> &buf, String &out_err_msg)
{
  if (fileSystemOverride)
    return fileSystemOverride->readFile(resolved_fn, requested_fn, buf, out_err_msg);

  uint32_t fileFlags = DF_READ | DF_IGNORE_MISSING | (tryOpenFilesFromRealFS ? 0 : DF_VROM_ONLY);
  file_ptr_t f = df_open(resolved_fn, fileFlags);
  if (!f)
  {
    out_err_msg.printf(128, "Script file %s (%s) not found", requested_fn, resolved_fn.str());
    return false;
  }

  int len = df_length(f);
  buf.resize(len + 1);
  if (df_read(f, (void *)buf.data(), len) != len)
  {
    logerr("Failed to read script file %s (%s)", requested_fn, resolved_fn.str());
    out_err_msg.printf(128, "Failed to read script file %s (%s)", requested_fn, resolved_fn.str());
    return false;
  }
  buf[len] = 0;
  df_close(f);
  return true;
}


bool SqModules::compileScriptImpl(const dag::ConstSpan<char> &buf, const char *resolved_fn, const HSQOBJECT *bindings,
  SQCompilation::SqASTData **return_ast)
{
  if (onCompileFile_cb)
    onCompileFile_cb(sqvm, resolved_fn);

  SQCompilation::SqASTData *ast =
    sq_parsetoast(sqvm, buf.data(), buf.size(), resolved_fn, compilationOptions.doStaticAnalysis, compilationOptions.raiseError);

  if (return_ast)
    *return_ast = ast;

  if (!ast)
  {
    return false;
  }
  else
  {
    if (onAST_cb)
      onAST_cb(sqvm, ast, up_data);
  }

  if (SQ_FAILED(sq_translateasttobytecode(sqvm, ast, bindings, &buf[0], buf.size(), compilationOptions.raiseError,
        compilationOptions.debugInfo)))
  {
    sq_releaseASTData(sqvm, ast);
    if (return_ast)
      *return_ast = nullptr;
    return false;
  }

  if (compilationOptions.doStaticAnalysis && !return_ast)
  {
    sq_analyzeast(sqvm, ast, bindings, &buf[0], buf.size());
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


bool SqModules::compileScript(dag::ConstSpan<char> buf, const String &resolved_fn, const char *requested_fn, const HSQOBJECT *bindings,
  Sqrat::Object &script_closure, String &out_err_msg, SQCompilation::SqASTData **return_ast)
{
  SqStackChecker stackCheck(sqvm);

  script_closure.Release();

  const char *filePath = resolved_fn.str();
  char buffer[MAX_PATH_LENGTH] = {0};

  if (compilationOptions.useAbsolutePath)
  {
    if (!fileSystemOverride)
    {
      const char *real_path = computeAbsolutePath(filePath, buffer, sizeof buffer);
      if (real_path)
      {
        filePath = real_path;
      }
    }
  }

  if (!compileScriptImpl(buf, filePath, bindings, return_ast))
  {
    out_err_msg.printf(128, "Failed to compile file %s (%s)", requested_fn, resolved_fn.str());
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
    if (dd_fname_equal(prevMod.fn.c_str(), resolved_fn))
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
  SqStackChecker stackCheck(sqvm);
  sq_pushobject(sqvm, bindings);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_registerbaselib(sqvm)));
  sq_poptop(sqvm);
}


void SqModules::bindRequireApi(HSQOBJECT bindings)
{
  SqStackChecker stackCheck(sqvm);

  sq_pushobject(sqvm, bindings);

  sq_pushstring(sqvm, "require", 7);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<true>, 1);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(sqvm, 2, ".s")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(sqvm, -3)));

  stackCheck.check(1); // bindings

  sq_pushstring(sqvm, "require_optional", 16);
  sq_pushuserpointer(sqvm, this);
  sq_newclosure(sqvm, sqRequire<false>, 1);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(sqvm, 2, ".s")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(sqvm, -3)));

  stackCheck.check(1); // bindings
  stackCheck.restore();
}


void SqModules::bindModuleApi(HSQOBJECT bindings, Sqrat::Table &state_storage, Sqrat::Array &ref_holder, const char *__name__,
  const char *__filename__)
{
  SQRAT_ASSERT(__name__);
  SQRAT_ASSERT(__filename__);

  HSQUIRRELVM vm = sqvm;
  SqStackChecker stackCheck(sqvm);

  sq_pushobject(vm, bindings);
  stackCheck.check(1); // bindings table

  sq_pushstring(vm, "persist", 7);
  sq_pushobject(vm, state_storage);
  sq_newclosure(vm, persist_state, 1);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 3, ".sc")));
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

  sq_pushstring(vm, "keepref", 7);
  sq_pushobject(vm, ref_holder);
  sq_newclosure(vm, keepref, 1);
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

  stackCheck.check(1); // bindings table
  stackCheck.restore();
}


bool SqModules::mergeBindings(Sqrat::Table &target, Sqrat::Object &key, Sqrat::Object &value, String &out_err_msg)
{
  if (target.RawHasKey(key))
  {
    string keyStr = key.Cast<string>();
    Sqrat::Object current = target.RawGetSlot(key);
    if (value.IsEqual(current))
      return true;
    else
    {
      out_err_msg.printf(0, "Duplicate field '%s' in imported bindings", keyStr.c_str());
      return false;
    }
  }
  else
  {
    target.SetValue(key, value);
    return true;
  }
}


bool SqModules::mergeBindings(Sqrat::Table &target, Sqrat::Table &upd, String &out_err_msg)
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

bool SqModules::importModules(const char *resolved_fn, Sqrat::Table &bindings_dest, const eastl::vector<ModuleImport> &imports,
  String &out_err_msg)
{
  if (imports.empty())
    return true;

  size_t rsIdx = runningScripts.size();
  runningScripts.emplace_back(resolved_fn);

  bool success = true;
  String mergeErrorMsg, requireErrorMsg;

  // printf("@#@ %d imports\n", int(imports.size()));
  for (const ModuleImport &import : imports)
  {
    Sqrat::Object exports;
    string moduleFnStr(import.moduleName);

    if (beforeImportModuleCb && !beforeImportModuleCb(this, moduleFnStr.c_str()))
    {
      out_err_msg.printf(0, "%s:%d:%d: Module '%s' is forbidden", resolved_fn, import.line, import.column, moduleFnStr.c_str());
      success = false;
      break;
    }

    auto nativeIt = nativeModules.find(import.moduleName);
    if (nativeIt != nativeModules.end())
      exports = nativeIt->second;
    else if (!requireModule(moduleFnStr.c_str(), true, __fn__, exports, requireErrorMsg))
    {
      out_err_msg.printf(0, "Import error at %s:%d:%d: %s", resolved_fn, import.line, import.column, requireErrorMsg.c_str());
      success = false;
      break;
    }

    if (import.slots.empty()) // import module as slot
    {
      // printf("* Module %s as %s\n", string(import.moduleFn).c_str(), string(import.moduleIdentifier).c_str());
      Sqrat::Object key(import.moduleIdentifier.c_str(), sqvm);
      if (!mergeBindings(bindings_dest, key, exports, mergeErrorMsg))
      {
        success = false;
        out_err_msg.printf(0, "%s:%d:%d: %s", resolved_fn, import.line, import.column, mergeErrorMsg.c_str());
      }
    }
    else if (import.slots.size() == 1 && import.slots[0].path.size() == 1 && import.slots[0].path[0] == "*") // import all fields
    {
      // printf("* %s.*\n", string(import.moduleFn).c_str());
      if (exports.GetType() != OT_TABLE)
      {
        out_err_msg.printf(0, "%s:%d:%d: module '%s' export is not a table", resolved_fn, import.line, import.column,
          moduleFnStr.c_str());
        success = false;
      }
      else
      {
        Sqrat::Table exportsTbl(exports);
        if (!mergeBindings(bindings_dest, exportsTbl, mergeErrorMsg))
        {
          success = false;
          out_err_msg.printf(0, "%s:%d: %s", resolved_fn, import.line, mergeErrorMsg.c_str());
        }
      }
    }
    else // import specific fields
    {
      // printf("* Specific field (%d):\n", int(import.fieldImports.size()));
      //  for (Import::FieldImport &field : import.fieldImports)
      //    printf("  * %s as %s\n", string(field.fieldName).c_str(), string(field.importAsName).c_str());
      Sqrat::Table exportsTbl(exports);
      for (const ModuleImport::Slot &slot : import.slots)
      {
        if (slot.path.size() != 1)
        {
          out_err_msg = slot.path.empty() ? "Empty field path" : "Nested fields import is not supported yet";
          success = false;
        }
        // printf("  * %s as %s\n", string(field.fieldName).c_str(), string(field.importAsName).c_str());
        Sqrat::Object srcKey(slot.path[0].c_str(), sqvm);

        if (!exportsTbl.RawHasKey(srcKey))
        {
          out_err_msg.printf(0, "%s:%d:%d no field '%s' in module '%s' exports", resolved_fn, slot.line, slot.column,
            slot.path[0].c_str(), moduleFnStr.c_str());
          success = false;
          break;
        }
        else
        {
          Sqrat::Object destKey(slot.importAsIdentifier.c_str(), sqvm);
          Sqrat::Object val = exports.GetSlot(srcKey);
          if (!mergeBindings(bindings_dest, destKey, val, mergeErrorMsg))
          {
            success = false;
            out_err_msg.printf(0, "%s:%d:%d: %s", resolved_fn, slot.line, slot.column, mergeErrorMsg.c_str());
            break;
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


static void import_parser_error_cb(void *user_pointer, const char *message, int line, int column)
{
  Sqrat::string *targetStr = reinterpret_cast<Sqrat::string *>(user_pointer);
  if (!targetStr->empty())
    targetStr->append("\n");
  targetStr->append(Sqrat::string(Sqrat::string::CtorSprintf{}, "line %d, col %d: %s", line, column, message));
}


bool SqModules::requireModule(const char *requested_fn, bool must_exist, const char *__name__, Sqrat::Object &exports,
  String &out_err_msg)
{
  out_err_msg.clear();
  exports.Release();

  String resolvedFn;
  resolveFileName(requested_fn, resolvedFn);

  if (!checkCircularReferences(resolvedFn, requested_fn))
  {
    out_err_msg = "Circular references error";
    // ^ will not be needed because of fatal() but keep it for the case of future changes
    return false;
  }

  if (beforeImportModuleCb && !beforeImportModuleCb(this, requested_fn))
  {
    out_err_msg.printf(0, "Module '%s' is forbidden", requested_fn);
    return false;
  }

  if (Module *found = findModule(resolvedFn))
  {
    exports = found->exports;
    return true;
  }

  Tab<char> fileContents(framemem_ptr());
  bool readOk = readFile(resolvedFn, requested_fn, fileContents, out_err_msg);
  if (!readOk)
    return !must_exist;

  string importErrMsg;
  ImportParser importParser(import_parser_error_cb, &importErrMsg);
  const char *importPtr = fileContents.data();
  int importEndLine = 0, importEndCol = 0;
  eastl::vector<ModuleImport> imports;
  eastl::vector<string> directives;
  eastl::vector<eastl::pair<const char *, const char *>> keepRanges;
  if (!importParser.parse(&importPtr, importEndLine, importEndCol, imports, &directives, &keepRanges))
  {
    out_err_msg.printf(0, "%s: %s", requested_fn, importErrMsg.c_str());
    return false;
  }

  HSQUIRRELVM vm = sqvm;

  SQRAT_ASSERT(__fn__ == nullptr);
  if (__name__ == __fn__)
    __name__ = resolvedFn.c_str();

  SqStackChecker stackCheck(sqvm);

  Sqrat::Table stateStorage = setupStateStorage(resolvedFn);
  Sqrat::Array refHolder(vm);

  Sqrat::Table bindingsTbl(vm);
  HSQOBJECT hBindings = bindingsTbl.GetObject();
  bindBaseLib(hBindings);
  bindModuleApi(hBindings, stateStorage, refHolder, __name__, resolvedFn.c_str());
  bindRequireApi(hBindings);

  if (compilationOptions.doStaticAnalysis)
    sq_checktrailingspaces(vm, resolvedFn, fileContents.data(), fileContents.size());

  if (!importModules(resolvedFn, bindingsTbl, imports, out_err_msg))
    return false;

  importParser.replaceImportBySpaces((char *)fileContents.data(), (char *)importPtr, keepRanges);

  Sqrat::Object scriptClosure;
  dag::Span<char> sourceCode = skip_bom(make_span(fileContents));

  SQCompilation::SqASTData *astData = nullptr;

  hBindings._flags = SQOBJ_FLAG_IMMUTABLE;
  bool compileRes = compileScript(sourceCode, resolvedFn, requested_fn, &hBindings, scriptClosure, out_err_msg,
    compilationOptions.doStaticAnalysis ? &astData : nullptr);

  if (!compileRes)
  {
    sq_releaseASTData(vm, astData);
    return false;
  }

  stackCheck.check();

  size_t rsIdx = runningScripts.size();
  runningScripts.emplace_back(resolvedFn.c_str());


  sq_pushobject(vm, scriptClosure.GetObject());
  sq_pushnull(vm); // module 'this'

  runningScriptClosuresStack.push_back(scriptClosure);
  SQRESULT callRes = sq_call(vm, 1, true, true);
  runningScriptClosuresStack.pop_back();

  G_UNUSED(rsIdx);

  if (SQ_FAILED(callRes))
  {
    SQRAT_ASSERT(runningScripts.size() == rsIdx + 1);
    runningScripts.pop_back();

    out_err_msg.printf(128, "Failed to run script %s (%s)", requested_fn, resolvedFn.c_str());
    sq_pop(vm, 1); // clojure, no return value on error
    sq_releaseASTData(vm, astData);
    return false;
  }

  HSQOBJECT hExports;
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hExports)));
  exports = Sqrat::Object(hExports, vm);

  sq_pop(vm, 2); // retval + closure

  stackCheck.check();

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


bool SqModules::reloadModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, String &out_err_msg)
{
  SQRAT_ASSERT(prevModules.empty());

  modules.swap(prevModules);
  modules.clear(); // just in case

  callAndClearUnloadHandlers(false);

  String errMsg;
  bool res = requireModule(fn, must_exist, __name__, exports, out_err_msg);

  prevModules.clear();

  return res;
}


bool SqModules::reloadAll(String &full_err_msg)
{
  SQRAT_ASSERT(prevModules.empty());
  full_err_msg.clear();

  modules.swap(prevModules);
  modules.clear(); // just in case

  callAndClearUnloadHandlers(false);

  bool res = true;
  Sqrat::Object exportsTmp;
  String errMsg;
  for (const Module &prev : prevModules)
  {
    if (!requireModule(prev.fn.c_str(), true, prev.__name__.c_str(), exportsTmp, errMsg))
    {
      res = false;
      full_err_msg.aprintf(0, "%s: %s\n", prev.fn.c_str(), errMsg.c_str());
    }
  }

  prevModules.clear();

  return res;
}


bool SqModules::addNativeModule(const char *name, const Sqrat::Object &exports, const char *module_doc_string)
{
  eastl::string module_name(name);
  if (nativeModules.find(module_name) != nativeModules.end())
  {
    G_ASSERTF_RETURN(0, false, "Native module '%s' is already registered", name);
  }

  if (module_doc_string)
    sq_setobjectdocstring(exports.GetVM(), &const_cast<Sqrat::Object &>(exports).GetObject(), module_doc_string);

  nativeModules[module_name] = exports;
  return true;
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

  if (strcmp(fileName, "squirrel.native_modules") == 0)
  {
    Sqrat::Array list = self->getNativeModuleNames();
    sq_pushobject(vm, list);
    return 1;
  }

  if (self->beforeImportModuleCb && !self->beforeImportModuleCb(self, fileName))
    return sqstd_throwerrorf(vm, "Module '%s' is forbidden", fileName);

  bool searchNative = true, searchScript = true;
  if ((self->configBits & SqModulesConfigBits::PermissiveModuleNames) == SqModulesConfigBits::None)
  {
    searchScript = str_ends_with(fileName, SCRIPT_MODULE_FILE_EXT, (int)fileNameLen);
    searchNative = !searchScript;
  }

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
  String errMsg(framemem_ptr());
  if (!self->requireModule(fileName, must_exist, __fn__, exports, errMsg))
    return sq_throwerror(vm, errMsg.c_str());

  sq_pushobject(vm, exports.GetObject());
  return 1;
}


void SqModules::registerStdLibNativeModule(const char *name, RegFunc reg_func)
{
  HSQOBJECT hModule;
  sq_resetobject(&hModule);
  sq_newtable(sqvm);
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(sqvm, -1, &hModule)));
  reg_func(sqvm);
  bool regRes = addNativeModule(name, Sqrat::Object(hModule, sqvm));
  G_UNUSED(regRes);
  SQRAT_ASSERTF(regRes, "Failed to init '%s' library with module manager", name);
  sq_pop(sqvm, 1);
}


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
    G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(sqvm, -1, &nparams, &nfreevars)));
    sq_pop(sqvm, 1);

    Sqrat::Function func(sqvm, Sqrat::Object(), f.GetObject());
    if (nparams == 1)
      func();
    else if (nparams == 2)
      func(is_closing);
    else
      G_ASSERT(0);
  }
  onModuleUnload.clear();
}


SQInteger SqModules::register_on_module_unload(HSQUIRRELVM vm)
{
  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  if (nparams != 1 && nparams != 2)
    return sqstd_throwerrorf(vm, "Function accepts 'this' + optional argument, but provided function requires %d args", nparams);

  SqModules *self = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getuserpointer(vm, 3, (SQUserPointer *)&self)));

  HSQOBJECT hFunc;
  sq_getstackobj(vm, 2, &hFunc);

  auto it = eastl::find_if(self->onModuleUnload.begin(), self->onModuleUnload.end(),
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


void SqModules::registerModulesModule()
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
