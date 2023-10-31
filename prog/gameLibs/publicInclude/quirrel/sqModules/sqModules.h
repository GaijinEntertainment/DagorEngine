//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_string.h>
#include <generic/dag_enumBitMask.h>
#include <sqrat.h>
#include <EASTL/vector_map.h>

#if defined(SQRAT_HAS_EASTL)
#include <EASTL/vector.h>
#else
#include <vector>
#endif


namespace sqimportparser
{
struct ModuleImport;
}

enum class SqModulesConfigBits
{
  None = 0,
  PermissiveModuleNames = 1, // Do not check native|script modules file extension (otherwise script modules should have ".nut"
                             // extension, while native shouldn't)
  Default = None
};
DAGOR_ENABLE_ENUM_BITMASK(SqModulesConfigBits);

namespace sqmodulesinternal
{
struct Import;
}

class SqModules
{
public:
  template <class T>
  using vector = SQRAT_STD::vector<T>;
  using string = Sqrat::string;

  typedef bool (*BeforeImportModuleCallback)(SqModules *sq_modules, const char *module_name);
  typedef void (*ModuleExeCallback)(HSQUIRRELVM vm, const char *requested_module_name, const char *resolved_module_name);

  struct IFileSystemOverride
  {
    virtual void resolveFileName(const char *requested_fn, String &res) = 0;
    // when file can't be read, return false and fill out_err_msg
    virtual bool readFile(const String &resolved_fn, const char *requested_fn, Tab<char> &buf, String &out_err_msg) = 0;
  };

  struct Module
  {
    string fn;
    Sqrat::Object exports;
    Sqrat::Object stateStorage;
    Sqrat::Array refHolder;
    Sqrat::Object moduleThis; //< needed to add refcount
    string __name__;
  };

  SqModules(HSQUIRRELVM vm, SqModulesConfigBits cfg = SqModulesConfigBits::Default);
  ~SqModules();

  HSQUIRRELVM getVM() { return sqvm; }

  // File name may be:
  // 1) relative to currently running script
  // 2) relative to base path
  // Note: accessing files outside of current base paths (via ../../)
  // can mess things up (breaking load module once rule)
  // __name__ is put to module this. Use __fn__ constant to use resolved filename or custom string to override
  bool requireModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, String &out_err_msg);
  // This can also be used for initial module execution
  bool reloadModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, String &out_err_msg);

  bool reloadAll(String &err_msg);

  //
  // 'module_name' shall point to to null-terminated statically allocated string
  //
  bool addNativeModule(const char *module_name, const Sqrat::Object &exports);

  void registerMathLib();
  void registerStringLib();
  void registerSystemLib();
  void registerIoStreamLib();
  void registerIoLib();
  void registerDateTimeLib();
  void registerDebugLib();

  template <typename F>
  void forEachNativeModule(const F &cb); // 'cb' called as cb(const char *module_name, const Sqrat::Object &module)
  Sqrat::Object *findNativeModule(const char *module_name);

  // Script API
  //   require(file_name, must_exist=true)
  template <bool must_exist>
  static SQInteger sqRequire(HSQUIRRELVM vm);

  static void setModuleExecutionCallbacks(ModuleExeCallback before_execution_cb, ModuleExeCallback after_execution_cb)
  {
    beforeExecutionCb = before_execution_cb;
    afterExecutionCb = after_execution_cb;
  }

  void setModuleImportCallback(BeforeImportModuleCallback before_import_module_callback)
  {
    beforeImportModuleCb = before_import_module_callback;
  }

  void setFileSystemOverride(IFileSystemOverride *fso) { fileSystemOverride = fso; }

  void bindRequireApi(HSQOBJECT bindings);
  void bindModuleApi(HSQOBJECT bindings, Sqrat::Table &state_storage, Sqrat::Array &ref_holder, const char *__name__,
    const char *__filename__);
  void bindBaseLib(HSQOBJECT bindings);

private:
  void resolveFileName(const char *fn, String &res);
  bool checkCircularReferences(const String &resolved_fn, const char *orig_fn);
  bool readFile(const String &resolved_fn, const char *requested_fn, Tab<char> &buf, String &out_err_msg);
  bool compileScript(dag::ConstSpan<char> buf, const String &resolved_fn, const char *orig_fn, const HSQOBJECT *bindings,
    Sqrat::Object &script_closure, String &out_err_msg);
  bool compileScriptImpl(const dag::ConstSpan<char> &buf, const char *resolved_fn, const HSQOBJECT *bindings);
  Sqrat::Table setupStateStorage(const char *resolved_fn);
  Module *findModule(const char *resolved_fn);

  typedef SQInteger (*RegFunc)(HSQUIRRELVM);
  void registerStdLibNativeModule(const char *name, RegFunc);

  void registerModulesModule();
  Sqrat::Array getNativeModuleNames();

  static ModuleExeCallback beforeExecutionCb;
  static ModuleExeCallback afterExecutionCb;
  BeforeImportModuleCallback beforeImportModuleCb;

  bool mergeBindings(Sqrat::Table &target, Sqrat::Object &key, Sqrat::Object &value, String &out_err_msg);
  bool mergeBindings(Sqrat::Table &target, Sqrat::Table &upd, String &out_err_msg);
  bool importModules(const char *resolved_fn, Sqrat::Table &bindings_dest, const vector<sqimportparser::ModuleImport> &imports,
    String &out_err_msg);

  static SQInteger register_on_module_unload(HSQUIRRELVM vm);
  void callAndClearUnloadHandlers(bool is_closing);

public:
  SqModulesConfigBits configBits;
  vector<Module> modules;
  vector<Module> prevModules; //< for hot reload

  eastl::vector_map<eastl::string, Sqrat::Object> nativeModules;

  static const char *__main__, *__fn__;

  struct
  {
    bool raiseError;
    bool debugInfo;
    bool doStaticAnalysis;
    bool useAbsolutePath;
  } compilationOptions;

  void *up_data;
  void (*onAST_cb)(HSQUIRRELVM, SQCompilation::SqASTData *, void *);
  void (*onBytecode_cb)(HSQUIRRELVM, HSQOBJECT, void *);

  // if this flag is false we won't try to open files from real file system if there are missing in vromfs.
  // this is to allow modding on PC and useAddonVromfs in dev build
  static bool tryOpenFilesFromRealFS;

  IFileSystemOverride *fileSystemOverride = nullptr;

private:
  vector<string> runningScripts;
  vector<Sqrat::Object> onModuleUnload;
  HSQUIRRELVM sqvm = nullptr;
};

template <typename F>
inline void SqModules::forEachNativeModule(const F &cb)
{
  for (auto &nm : nativeModules)
    cb(nm.first.data(), nm.second);
}

inline Sqrat::Object *SqModules::findNativeModule(const char *module_name)
{
  auto it = nativeModules.find(module_name);
  return (it == nativeModules.end()) ? nullptr : &it->second;
}
