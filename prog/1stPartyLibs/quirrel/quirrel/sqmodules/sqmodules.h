#pragma once

#include <squirrel.h>
#include <sqrat.h>

#if defined(SQRAT_HAS_EASTL)
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#else
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#endif

#include "span.h"


namespace SQCompilation
{
struct SqASTData;
}

struct ISqModulesFileAccess
{
  template <class T>
  using vector = SQRAT_STD::vector<T>;
  using string = Sqrat::string;

  virtual ~ISqModulesFileAccess() {}

  virtual void destroy() = 0;

  virtual void resolveFileName(const char *requested_fn, const char *running_script, string &res) = 0;
  // when file can't be read, return false and fill out_err_msg
  virtual bool readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf, string &out_err_msg) = 0;

  virtual void getSearchTargets(const char * /*fn*/, bool &search_native, bool &search_script)
  {
    search_native = true;
    search_script = true;
  }
};

struct DefSqModulesFileAccess : public ISqModulesFileAccess
{
  virtual void destroy() override;

  virtual void resolveFileName(const char *requested_fn, const char *running_script, string &res) override;
  // when file can't be read, return false and fill out_err_msg
  virtual bool readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf, string &out_err_msg) override;

  string makeRelativeFilename(const char *cur_file, const char *requested_fn);

  bool useAbsolutePath = false;
  vector<char> scriptPathBuf; // cache
  bool needToDeleteSelf = false;
};


class SqModules
{
private:
  SqModules(const SqModules &) = delete;
  SqModules(SqModules &&) = delete;
public:
  template <class T>
  using vector = SQRAT_STD::vector<T>;
  using string = Sqrat::string;

  typedef bool (*BeforeImportModuleCallback)(SqModules *sq_modules, const char *module_name);
  typedef void (*SQOnCompileFileCb)(HSQUIRRELVM, const char *);

  struct Module
  {
    string fn; // filename resolved to the full path
    Sqrat::Object exports;
    Sqrat::Object stateStorage;
    Sqrat::Array refHolder;
    Sqrat::Object scriptClosure;
    string __name__;
  };

  SqModules(HSQUIRRELVM vm, ISqModulesFileAccess *file_access);
  ~SqModules();

  HSQUIRRELVM getVM() { return sqvm; }

  // File name may be:
  // 1) relative to currently running script
  // 2) relative to base path
  // Note: accessing files outside of current base paths (via ../../)
  // can mess things up (breaking load module once rule)
  // __name__ is put to module this. Use __fn__ constant to use resolved filename or custom string to override
  bool requireModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, string &out_err_msg);
  // This can also be used for initial module execution
  bool reloadModule(const char *fn, bool must_exist, const char *__name__, Sqrat::Object &exports, string &out_err_msg);

  bool reloadAll(string &err_msg);

  void resetStaticMemos();

  bool addNativeModule(const char *module_name, const Sqrat::Object &exports, const char *module_doc_string = nullptr);

  void registerTypesLib();
  void registerMathLib();
  void registerStringLib();
  void registerSystemLib();
  void registerIoStreamLib();
  void registerIoLib();
  void registerDateTimeLib();
  void registerDebugLib();
  void registerModulesLib();

  template <typename F>
  void forEachNativeModule(const F &cb); // 'cb' called as cb(const char *module_name, const Sqrat::Object &module)
  Sqrat::Object *findNativeModule(const char *module_name);
  void getLoadedModules(vector<string> &file_names);

  void setModuleImportCallback(BeforeImportModuleCallback before_import_module_callback)
  {
    beforeImportModuleCb = before_import_module_callback;
  }

  void bindRequireApi(HSQOBJECT bindings);
  void bindModuleApi(HSQOBJECT bindings, Sqrat::Table &state_storage, Sqrat::Array &ref_holder, const char *__name__,
    const char *__filename__);
  void bindBaseLib(HSQOBJECT bindings);

private:
  // Script API
  //   require(file_name, must_exist=true)
  template <bool must_exist>
  static SQInteger sqRequire(HSQUIRRELVM vm);

  void resolveFileName(const char *fn, string &res);
  bool checkCircularReferences(const string &resolved_fn, const char *orig_fn, string &out_err_msg);
  bool readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf, string &out_err_msg);
  bool compileScript(const sqm::span<char> buf, const string &resolved_fn, const char *orig_fn, Sqrat::Table &bindings,
                                    Sqrat::Object &script_closure, string &out_err_msg,
                                    SQCompilation::SqASTData **return_ast = nullptr);
  bool compileScriptImpl(const sqm::span<char> buf, const char *resolved_fn, Sqrat::Table &bindings, string &out_err_msg,
                         SQCompilation::SqASTData **return_ast = nullptr);

  Sqrat::Table setupStateStorage(const char *resolved_fn);

  Module *findModule(const char *resolved_fn);

  typedef SQInteger (*RegFunc)(HSQUIRRELVM);
  void registerStdLibNativeModule(const char *name, RegFunc);

  Sqrat::Array getNativeModuleNames();

  BeforeImportModuleCallback beforeImportModuleCb;

  bool mergeBindings(Sqrat::Table &target, Sqrat::Object &key, Sqrat::Object &value, string &out_err_msg);
  bool mergeBindings(Sqrat::Table &target, Sqrat::Table &upd, string &out_err_msg);
  bool importModules(const char *resolved_fn, Sqrat::Table &bindings_dest, const SQModuleImport *imports, int num_imports,
    string &out_err_msg);

  static SQInteger register_on_module_unload(HSQUIRRELVM vm);
  void callAndClearUnloadHandlers(bool is_closing);

public:
  static const char *__main__, *__fn__, *__analysis__;

  struct
  {
    bool raiseError;
    bool doStaticAnalysis;
  } compilationOptions;

  void *up_data;
  void (*onAST_cb)(HSQUIRRELVM, SQCompilation::SqASTData *, void *);
  void (*onBytecode_cb)(HSQUIRRELVM, HSQOBJECT, void *);
  SQOnCompileFileCb onCompileFile_cb = nullptr;

private:
  vector<Module> modules;
  vector<Module> prevModules; //< for hot reload
  vector<Sqrat::Object> runningScriptClosuresStack;

#if defined(SQRAT_HAS_EASTL)
  using NativeModulesMap = eastl::vector_map<string, Sqrat::Object>;
#else
  using NativeModulesMap = std::unordered_map<string, Sqrat::Object>;
#endif

  NativeModulesMap nativeModules;

  vector<string> runningScripts;
  vector<Sqrat::Object> onModuleUnload;
  HSQUIRRELVM sqvm = nullptr;

  ISqModulesFileAccess *fileAccess = nullptr;
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
