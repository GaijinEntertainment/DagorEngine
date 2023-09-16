#include <startup/dag_globalSettings.h>
#include <memory/dag_mem.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <debug/dag_logSys.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <utf8/utf8.h>

#include <sqplus.h>
#include <sqModules/sqModules.h>

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel_json/quirrel_json.h>
#include <quirrel/http/sqHttpClient.h>
#if HAS_CHARSQ
#include <charServerClient/sqBindings.h>
#include <charServerClient/httpWrapperAsync/wrapper.h>
#endif
#include <quirrel/sqEventBus/sqEventBus.h>
#if HAS_YUP_PARSE
#include <quirrel/yupfile_parse/yupfile_parse.h>
#endif
#include <quirrel/frp/dag_frp.h>
#include <quirrel/sqJwt/sqJwt.h>
#include <quirrel/base64/base64.h>
#include <asyncHTTPClient/asyncHTTPClient.h>

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <ioSys/dag_dataBlock.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdblob.h>

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_delayedAction.h>
#include <gui/dag_visualLog.h>
#include <perfMon/dag_cpuFreq.h>
#include <helpers/keyValueFile.h>

#include <EASTL/unordered_map.h>

#define assert G_ASSERT // -V1059
#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>


#define MODULE_PARSE_STL eastl
#include <helpers/importParser/importParser.h>
#include <algorithm>

#include "fs_utils.h"

using namespace sqimportparser;


#define APP_VERSION "1.0.18"

namespace visuallog
{
// stub for dagor debug module
void logmsg(const char *, LogItemCBProc, void *, E3DCOLOR, int) {}
} // namespace visuallog


static void before_execute_module(HSQUIRRELVM vm, const char *requested_module_name, const char *resolved_module_name);
static void after_execute_module(HSQUIRRELVM vm, const char *requested_module_name, const char *resolved_module_name);

static bool syntax_check_only = false;
static bool has_errors = false;
static bool csq_time = false;
static bool csq_print_to_string = false;
static bool csq_show_native_modules = false;
static bool use_debug_file = false;
static bool check_fname_case = false;
static bool do_static_analysis = false;
static bool flip_diagnostic_state = false;
static String csq_output_string;
static String sqconfig_dir;

static HSQUIRRELVM sqvm = nullptr;
static SqModules *module_manager = nullptr;

static eastl::unique_ptr<sqfrp::ObservablesGraph> frp_graph;
static std::vector<std::string> allowed_native_modules;
static std::vector<std::string> char_server_urls;

static const char *print_sorted_root_code =
#include "print_sorted_root.nut.inl"
  ;


static void stderr_report_fatal_error(const char *, const char *msg, const char *call_stack)
{
  printf("Fatal error: %s\n%s", msg, call_stack);
  flush_debug_file();
  quit_game(1, false);
}


static void output_text(const char *msg)
{
  if (csq_print_to_string)
  {
    csq_output_string.reserve(65536);
    csq_output_string += msg;
  }
  else
  {
    printf("%s", msg);
    debug_("%s", msg);
  }
}


static void output_error_text(const char *msg)
{
  output_text(msg);
  has_errors = true;
}


static void report_error(const char *msg)
{
  output_error_text("\nERROR: ");
  output_error_text(msg);
  output_error_text("\n");
}

static void script_print_function(HSQUIRRELVM v, const SQChar *s, ...)
{
  G_UNUSED(v);
  const int maxlen = 65536;
  static char temp[maxlen];
  va_list vl;
  va_start(vl, s);
  vsnprintf(temp, maxlen - 1, s, vl);
  va_end(vl);

  temp[maxlen - 1] = 0;
  output_text(temp);
}

static void script_error_function(HSQUIRRELVM v, const SQChar *s, ...)
{
  G_UNUSED(v);
  const int maxlen = 65536;
  static char temp[maxlen];
  va_list vl;
  va_start(vl, s);
  vsnprintf(temp, maxlen - 1, s, vl);
  va_end(vl);

  temp[maxlen - 1] = 0;
  output_error_text(temp);
}


inline bool ends_with(const eastl::string_view &str, const char *suffix)
{
  const size_t suffixLen = strlen(suffix);
  return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix);
}

static bool is_module_allowed(SqModules *, const char *module_name)
{
  if (allowed_native_modules.empty() || ends_with(eastl::string_view(module_name), ".nut"))
    return true;

  return std::find(allowed_native_modules.begin(), allowed_native_modules.end(), std::string(module_name)) !=
         allowed_native_modules.end();
}

static void register_json(SqModules *module_mgr) { bindquirrel::bind_json_api(module_mgr); }

static void sq_error_handler(const Sqrat::string &e)
{
  sqstd_printcallstack(sqvm);
  report_error(String("Quirrel error: ") + e.c_str());
}

static bool is_path_absolute(const String &path);


static bool run_sq_code(const char *name, const char *code)
{
  Sqrat::Script script(sqvm);
  Sqrat::string errMsg;

  if (!script.CompileString(code, errMsg, name) || (!syntax_check_only && script.Run(errMsg)))
  {
    sq_error_handler(errMsg);
    return false;
  }
  return true;
}


typedef void (*UseModuleFunc)();

struct Module
{
  const char *name;
  const char *comment;
  UseModuleFunc func;
};

static Module modules[] = {
  {"register_dagor_fs", "module dagor.fs: scan_folder, file_exists, ...",
    [] { bindquirrel::register_dagor_fs_module(module_manager); }},
  {"register_dagor_fs_vrom", "module dagor.fs.vrom: scan_vrom_folder",
    [] { bindquirrel::register_dagor_fs_vrom_module(module_manager); }},
  {"sqstd_register_io_lib", "see quirrel doc", [] { module_manager->registerIoLib(); }},
  {"sqstd_register_base_libs", "math, string, iostream",
    [] {
      module_manager->registerMathLib();
      module_manager->registerStringLib();
      if (!module_manager->findNativeModule("io"))
        module_manager->registerIoStreamLib();
    }},
  {"sqstd_register_system_lib", "system lib", [] { module_manager->registerSystemLib(); }},
  {"sqstd_register_datetime_lib", "datetime lib", [] { module_manager->registerDateTimeLib(); }},
  {"sqrat_bind_dagor_logsys", "modules: dagor.debug, dagor.assertf, dagor.debug_dump_stack ...",
    [] { bindquirrel::sqrat_bind_dagor_logsys(module_manager, true); }},
  {"register_platform_module", "get_platform_string_id ...", [] { bindquirrel::register_platform_module(module_manager); }},
  {"register_utf8", "roottable: class utf8, utf8.strtr, utf8.charCount ...", [] { bindquirrel::register_utf8(module_manager); }},
  {"sqrat_bind_datablock_module", "'DataBlock' module", [] { bindquirrel::sqrat_bind_datablock(module_manager); }},
  {"register_json", "'json' module", [] { register_json(module_manager); }},
  {"sqrat_bind_dagor_math_module", "'dagor.math', 'dagor.random' and 'hash' modules: Point2, Point3, TMatrix, Color4 ...",
    [] {
      bindquirrel::sqrat_bind_dagor_math(module_manager);
      bindquirrel::register_random(module_manager);
      bindquirrel::register_hash(module_manager);
    }},
  {"register_dagor_http", "'dagor.http' module",
    [] {
      bindquirrel::bind_http_client(module_manager);
      bindquirrel::http_set_blocking_wait_mode(true);
    }},
#if HAS_CHARSQ
  {"register_char_server_client", "module char: request, requestEventBus ...",
    [] {
      struct UrlProvider final : charsq::IUrlProvider
      {
        void enumerateUrls(const char *service, const eastl::function<void(const char *)> &callback) const override
        {
          if (0 == strcmp(service, "char"))
          {
            for (const std::string &url : char_server_urls)
              callback(url.c_str());
          }
        }
      };
      static UrlProvider urlProvider;

      if (!cpujobs::is_inited())
        cpujobs::init();

      charsq::init("console", 0, spawn_http_wrapper_async(), module_manager);
      charsq::add_client(sqvm, "char", false, nullptr, &urlProvider);
    }},
#endif
  {"register_eventbus", "module eventbus: subscribe, subscribe_onehit",
    [] { sqeventbus::bind(module_manager, "console", sqeventbus::ProcessingMode::IMMEDIATE); }},
  {"register_reg_exp", "roottable: class regexp2, regexp2.match, regexp2.fullmatch, regexp2.replace",
    [] { bindquirrel::register_reg_exp(module_manager); }},
#if HAS_YUP_PARSE
  {"yupfile_parse lib", "get_str, get_int", [] { bindquirrel::register_yupfile_module(module_manager); }},
#endif
  {"register_dagor_time_module", "get_time_msec", [] { bindquirrel::bind_dagor_time(module_manager); }},
  {"register_dagor_shell_module", "shell_execute", [] { bindquirrel::register_dagor_shell(module_manager); }},
  {"register_dagor_iso8601_module", "module dagor.iso8601: parse_unix_time, format_unix_time ...",
    [] { bindquirrel::register_iso8601_time(module_manager); }},
  {"register_jwt", "JWT decode", [] { bindquirrel::bind_jwt(module_manager); }},
  {"register_base64", "base64 encode/decode", [] { bindquirrel::bind_base64_utils(module_manager); }},
  {"register_frp", "Watched/Computed",
    [] {
      sqfrp::bind_frp_classes(module_manager);
      frp_graph.reset(new sqfrp::ObservablesGraph(sqvm));
    }},
  {"register_localization", "doesLocTextExist, getLocTextForLang and so on for localizations",
    [] { bindquirrel::register_dagor_localization_module(module_manager); }},
  {"register_clipboard", "register_dagor_clipboard", [] { bindquirrel::register_dagor_clipboard(module_manager); }},

};


static SQInteger get_type_delegates(HSQUIRRELVM v)
{
  const SQChar *type_name;
  sq_getstring(v, 2, &type_name);

  SQObjectType t;
  if (!strcmp(type_name, "table"))
    t = OT_TABLE;
  else if (!strcmp(type_name, "array"))
    t = OT_ARRAY;
  else if (!strcmp(type_name, "string"))
    t = OT_STRING;
  else if (!strcmp(type_name, "integer"))
    t = OT_INTEGER;
  else if (!strcmp(type_name, "generator"))
    t = OT_GENERATOR;
  else if (!strcmp(type_name, "closure"))
    t = OT_CLOSURE;
  else if (!strcmp(type_name, "thread"))
    t = OT_THREAD;
  else if (!strcmp(type_name, "class"))
    t = OT_CLASS;
  else if (!strcmp(type_name, "instance"))
    t = OT_INSTANCE;
  else if (!strcmp(type_name, "weakref"))
    t = OT_WEAKREF;
  else
    return sq_throwerror(v, String(0, "no default delegate table for type %s or it is an unknown type", type_name));

  SQRESULT res = sq_getdefaultdelegate(v, t);
  if (SQ_FAILED(res))
    return SQ_ERROR;

  return 1;
}

static void split_string_lines(String s, Tab<String> &output, Tab<String> &output_name)
{
  clear_and_shrink(output);
  s.replaceAll("\r\n", "\n");

  String buf;
  for (int i = 0; i < s.length(); i++)
  {
    if (s[i] == '\n')
    {
      output.push_back(buf);
      int endOfName = max<int>(find_value_idx(buf, '|'), 0);
      String substr;
      substr.setSubStr(buf.str(), buf.str() + endOfName);
      output_name.push_back(substr);

      clear_and_shrink(buf);
      continue;
    }
    buf += s[i];
  }

  if (buf.length() > 0)
  {
    output.push_back(buf);
    int endOfName = max<int>(find_value_idx(buf, '|'), 0);
    String substr;
    substr.setSubStr(buf.str(), buf.str() + endOfName);
    output_name.push_back(substr);
  }
}


static void print_key(const String &full, const String &name, bool overwrites)
{
  printf(overwrites ? "> " : "  ");

  printf("%s", name.str());
  const char *params = strstr(full.str(), "|params:");
  if (params)
    printf("%s", params + sizeof("|params:") - 1);

  if (strstr(full.str(), "(class :"))
    printf(" // class");

  if (strstr(full.str(), "(array :"))
    printf(" // array");

  if (strstr(full.str(), "(table :"))
    printf(" // table");

  if (strstr(full.str(), "(null :"))
    printf(" // null");

  printf("\n");
}

static void after_execute(const char *code)
{
  static Tab<String> exeBase;
  static Tab<String> exeBaseName;

  clear_and_shrink(csq_output_string);
  if (!run_sq_code("print_sorted_root_code", print_sorted_root_code))
    quit_game(1, false);

  if (!code)
  {
    split_string_lines(csq_output_string, exeBase, exeBaseName);
  }
  else
  {
    printf("\n%s:\n", code);
    Tab<String> cur;
    Tab<String> curName;
    split_string_lines(csq_output_string, cur, curName);
    for (int i = 0; i < cur.size(); i++)
    {
      if (find_value_idx(exeBase, cur[i]) >= 0)
        continue;

      bool overwrites = (find_value_idx(exeBaseName, curName[i]) >= 0);
      print_key(cur[i], curName[i], overwrites);
    }

    exeBase = cur;
    exeBaseName = curName;
  }
}


static String read_file_ignoring_utf8bom(const char *filename)
{
  String script;

  file_ptr_t f = df_open(filename, DF_READ);
  if (!f)
  {
    printf("Error: cannot open file: '%s'\n", filename);
    quit_game(1, false);
  }

  int len = df_length(f);
  script.resize(len + 1);
  df_read(f, &script[0], len);
  script.back() = '\0';
  if (utf8::is_bom(script.str()))
    erase_items(script, 0, sizeof(utf8::bom));

  df_close(f);

  return script;
}


static void import_error_cb(void *filename, const char *message, int line, int column)
{
  printf("Error: %s\nat %s:%d:%d\n", message, (const char *)filename, line, column);
}


static bool remove_import_from_code(const char *code, const char *filename)
{
  const char *c = code;
  ImportParser importParser(import_error_cb, (void *)filename);
  int firstLineAfterImport = 1;
  int importEndCol = 0;
  eastl::vector<eastl::string> directives;
  eastl::vector<eastl::pair<const char *, const char *>> keepRanges;
  eastl::vector<ModuleImport> importModules;
  if (!importParser.parse(&c, firstLineAfterImport, importEndCol, importModules, &directives, &keepRanges))
    return false;

  (void)firstLineAfterImport;
  (void)importEndCol;
  (void)directives;

  ImportParser::replaceImportBySpaces((char *)code, (char *)c, keepRanges);

  return true;
}


static void check_syntax(SqModules *module_mgr, const char *filename)
{
  G_ASSERT(module_mgr->getVM() == sqvm);
  String source = read_file_ignoring_utf8bom(filename);

  Sqrat::Table bindings(sqvm);
  HSQOBJECT hBindings = bindings;
  Sqrat::Table stateStorage(sqvm);
  Sqrat::Array refHolder(sqvm);
  module_mgr->bindModuleApi(hBindings, stateStorage, refHolder, SqModules::__main__, filename);
  module_mgr->bindRequireApi(hBindings);
  module_mgr->bindBaseLib(hBindings);

  sq_setcompilecheckmode(sqvm, SQTrue);

  if (!remove_import_from_code(source.str(), filename))
    quit_game(1, false);

  bool success = false;

  if (module_manager->compilationOptions.useAST)
    success = SQ_SUCCEEDED(sq_compilewithast(sqvm, source.c_str(), source.length(), filename, true, true, &hBindings));
  else
    success = SQ_SUCCEEDED(sq_compileonepass(sqvm, source.c_str(), source.length(), filename, true, true, &hBindings));

  if (!success)
    quit_game(1, false);
}


struct ModuleStackRecord
{
  eastl::string requestedModuleName;
  eastl::string resolvedModuleName;
  eastl::string absoluteFileName;
};

static eastl::vector<ModuleStackRecord> modules_list;

// index in modules_list
static eastl::vector<int> module_stack;

// identifiler -> index in modules_list
static eastl::unordered_map<eastl::string, int> root_identifiers;
static eastl::unordered_map<eastl::string, int> const_identifiers;
static bool first_execute = true;

static eastl::vector<eastl::string> file_names_in_order_of_execution;

static void dump_export_item(Sqrat::Object &key, Sqrat::Object &value, eastl::string *result = nullptr)
{
  if (key.GetType() != OT_STRING)
    return;

  eastl::string str;

  const Sqrat::Var<const char *> keyVar = key.GetVar<const char *>();
  str.append(keyVar.value);

  if (value.GetType() == OT_CLOSURE)
    str.append(" function");
  else if (value.GetType() == OT_NATIVECLOSURE)
    str.append(" native");
  else if (value.GetType() == OT_TABLE)
    str.append(" table");
  else if (value.GetType() == OT_ARRAY)
    str.append(" array");
  else if (value.GetType() == OT_CLASS)
    str.append(" class");
  else if (value.GetType() == OT_INSTANCE)
    str.append(" instance");
  else if (value.GetType() == OT_NULL)
    str.append(" null");
  else if (value.GetType() == OT_INTEGER)
    str.append(" integer");
  else if (value.GetType() == OT_FLOAT)
    str.append(" float");
  else if (value.GetType() == OT_BOOL)
    str.append(" bool");
  else if (value.GetType() == OT_STRING)
    str.append(" string");
  else
    str.append(" other");

  if (value.GetType() == OT_CLOSURE)
  {
    HSQOBJECT &obj = value.GetObject();
    SQFunctionProto *func = obj._unVal.pClosure->_function;
    int maxParams = max(int(func->_nparameters) - 1, 0);
    int minParams = max(int(maxParams - func->_ndefaultparams), 0);

    eastl::vector<eastl::string> paramNames;
    for (int i = 0; i < func->_nparameters; i++)
    {
      SQObjectPtr &param = func->_parameters[i];
      if (param._type == OT_STRING)
        paramNames.push_back((const char *)param._unVal.pString->_val);
    }

    if (!paramNames.empty() && func->_varparams)
    {
      paramNames.pop_back(); // remove 'vargv'
      minParams = max(minParams - 1, 0);
      maxParams = 99;
    }

    str.append_sprintf(" %d %d", minParams, maxParams);

    for (const eastl::string &paramName : paramNames)
      if (paramName != "this")
        str.append_sprintf(" %s", paramName.c_str());
  }

  if (value.GetType() == OT_NATIVECLOSURE)
  {
    HSQOBJECT &obj = value.GetObject();
    SQNativeClosure *func = obj._unVal.pNativeClosure;
    if (func->_nparamscheck < 0)
      str.append_sprintf(" %d %d", abs(int(func->_nparamscheck)) - 1, 99);
    else
    {
      int params = max(int(func->_nparamscheck) - 1, 0);
      str.append_sprintf(" %d %d", params, params);
    }
  }

  if (value.GetType() == OT_TABLE || value.GetType() == OT_CLASS || value.GetType() == OT_INSTANCE)
  {
    Sqrat::Table::iterator iter;
    while (value.Next(iter))
    {
      Sqrat::Object sqKey(iter.getKey(), sqvm);
      if (sqKey.GetType() == OT_STRING)
        str.append_sprintf(" %s", sqKey.GetVar<const char *>().value);
    }
  }

  if (!result)
    printf("%s\n", str.c_str());
  else
    eastl::swap(str, *result);
}


static void collect_root_and_const_names(HSQUIRRELVM vm, eastl::vector<eastl::string> &root_names,
  eastl::vector<eastl::string> &const_names)
{
  {
    Sqrat::RootTable exports(vm);
    Sqrat::Table::iterator iter;
    while (exports.Next(iter))
    {
      Sqrat::Object sqKey(iter.getKey(), vm);
      Sqrat::Object sqValue(iter.getValue(), vm);
      eastl::string tmp;
      dump_export_item(sqKey, sqValue, &tmp);
      root_names.emplace_back(tmp);
    }
  }

  {
    Sqrat::ConstTable exports(vm);
    Sqrat::Table::iterator iter;
    while (exports.Next(iter))
    {
      Sqrat::Object sqKey(iter.getKey(), vm);
      Sqrat::Object sqValue(iter.getValue(), vm);
      eastl::string tmp;
      dump_export_item(sqKey, sqValue, &tmp);
      const_names.emplace_back(tmp);
    }
  }
}


static void before_execute_module(HSQUIRRELVM vm, const char *requested_module_name, const char *resolved_module_name)
{
  if (first_execute) // dump identifiers from root and consttable that a present by default
  {
    first_execute = false;
    modules_list.push_back({"", "", ""});
    module_stack.push_back(0);
    eastl::vector<eastl::string> root_names;
    eastl::vector<eastl::string> const_names;
    collect_root_and_const_names(vm, root_names, const_names);

    for (const eastl::string &name : root_names)
      if (root_identifiers.find(name) == root_identifiers.end())
        root_identifiers[name] = 0;

    for (const eastl::string &name : const_names)
      if (const_identifiers.find(name) == const_identifiers.end())
        const_identifiers[name] = 0;
  }

  if (!resolved_module_name || !resolved_module_name[0])
    resolved_module_name = requested_module_name;

  modules_list.push_back({requested_module_name, resolved_module_name, resolve_absolute_file_name(resolved_module_name)});
  module_stack.push_back(modules_list.size() - 1);
}

static void after_execute_module(HSQUIRRELVM vm, const char *requested_module_name, const char *resolved_module_name)
{
  G_UNUSED(resolved_module_name);
  G_UNUSED(requested_module_name);
  int moduleIndex = module_stack.back();
  G_ASSERT(modules_list[moduleIndex].requestedModuleName == requested_module_name);

  eastl::vector<eastl::string> root_names;
  eastl::vector<eastl::string> const_names;
  collect_root_and_const_names(vm, root_names, const_names);

  for (const eastl::string &name : root_names)
    if (root_identifiers.find(name) == root_identifiers.end())
      root_identifiers[name] = moduleIndex;

  for (const eastl::string &name : const_names)
    if (const_identifiers.find(name) == const_identifiers.end())
      const_identifiers[name] = moduleIndex;

  module_stack.pop_back();

  file_names_in_order_of_execution.push_back(resolve_absolute_file_name(resolved_module_name));
}

static int find_module_index_by_resolved_name(const char *resolved_name)
{
  for (int i = 0; i < modules_list.size(); i++)
    if (modules_list[i].resolvedModuleName == resolved_name)
      return i;
  // printf("#Internal error: cannot find module by resolved name '%s'\n", resolved_name);
  return -1;
}

static void dump_export_content()
{
  printf("\n### BEGIN EXPORT CONTENT\n");
  HSQUIRRELVM vm = module_manager->getVM();
  for (SqModules::Module &module : module_manager->modules)
  {
    int index = find_module_index_by_resolved_name(module.fn.c_str());
    printf("### SCRIPT MODULE %s %s %s\n", modules_list[index].requestedModuleName.c_str(), module.fn.c_str(),
      modules_list[index].absoluteFileName.c_str());

    Sqrat::Object &exports = module.exports;
    if (exports.GetType() == OT_TABLE || exports.GetType() == OT_CLASS)
    {
      Sqrat::Table::iterator iter;
      while (exports.Next(iter))
      {
        Sqrat::Object sqKey(iter.getKey(), vm);
        Sqrat::Object sqValue(iter.getValue(), vm);
        dump_export_item(sqKey, sqValue);
      }
    }
    else
    {
      Sqrat::Object key("=", vm);
      dump_export_item(key, exports);
    }

    printf("### MODULE EXPORT TO ROOT\n");
    for (auto &item : root_identifiers)
      if (item.second == index)
        printf("%s\n", item.first.c_str());

    printf("### MODULE EXPORT TO CONST\n");
    for (auto &item : const_identifiers)
      if (item.second == index)
        printf("%s\n", item.first.c_str());
  }

  // native modules
  for (auto &module : module_manager->nativeModules)
  {
    int index = find_module_index_by_resolved_name(module.first.c_str());

    printf("### NATIVE MODULE %s\n", module.first.c_str());
    Sqrat::Object exports = module.second;
    if (exports.GetType() == OT_TABLE || exports.GetType() == OT_CLASS)
    {
      Sqrat::Table::iterator iter;
      while (exports.Next(iter))
      {
        Sqrat::Object sqKey(iter.getKey(), vm);
        Sqrat::Object sqValue(iter.getValue(), vm);
        dump_export_item(sqKey, sqValue);
      }
    }
    else
    {
      Sqrat::Object key("=", vm);
      dump_export_item(key, exports);
    }

    printf("### MODULE EXPORT TO ROOT\n");
    for (auto &item : root_identifiers)
      if (item.second == index)
        printf("%s\n", item.first.c_str());

    printf("### MODULE EXPORT TO CONST\n");
    for (auto &item : const_identifiers)
      if (item.second == index)
        printf("%s\n", item.first.c_str());
  }

  // initial
  {
    printf("### INITIAL ROOT\n");
    for (auto &item : root_identifiers)
      if (item.second == 0)
        printf("%s\n", item.first.c_str());

    printf("### INITIAL CONST\n");
    for (auto &item : const_identifiers)
      if (item.second == 0)
        printf("%s\n", item.first.c_str());
  }

  // default type deligates
  {
    const char *types[] = {"table", "array", "string", "integer", "generator", "closure", "thread", "class", "instance", "weakref"};
    for (const char *type : types)
    {
      printf("### NATIVE MODULE '%s' default deligate\n", type);
      sq_pushroottable(vm);
      sq_pushstring(vm, (SQChar *)type, -1);
      get_type_delegates(vm);
      HSQOBJECT hExports;
      SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hExports)));
      Sqrat::Object exports(hExports, vm);
      Sqrat::Table::iterator iter;
      while (exports.Next(iter))
      {
        Sqrat::Object sqKey(iter.getKey(), vm);
        Sqrat::Object sqValue(iter.getValue(), vm);
        dump_export_item(sqKey, sqValue);
      }
      sq_pop(vm, 3);
    }
  }

  for (eastl::string name : file_names_in_order_of_execution)
    if (::dd_file_exists(name.c_str()))
      printf("### PROCESS FILE %s\n", name.c_str());

  printf("### END EXPORT CONTENT\n\n");
}


static bool process_file(const char *filename, const char *code, const KeyValueFile &config_blk, int moduleIndex,
  String &out_module_name, bool &is_execute)
{
  bool success = false;
  int indexCounter = 0;
  out_module_name = "<unknown>";

  SquirrelVM::Init(SquirrelVM::SF_IGNORE_DEFAULT_LIBRARIES);
  sqvm = SquirrelVM::GetVMPtr();
  sq_forbidglobalconstrewrite(sqvm, true);
  sq_setprintfunc(sqvm, ::script_print_function, ::script_error_function);

  bool useLibsByDefault = config_blk.getBool("use_libs_by_default", true);

  module_manager = new SqModules(sqvm, SqModulesConfigBits::PermissiveModuleNames); // allow stub native modules with script files

  int argIdx = 1;
  while (const char *optVal = ::dgs_get_argv("compiler", argIdx))
  {
    if (strcmp(optVal, "ast") == 0)
    {
      module_manager->compilationOptions.useAST = true;
      module_manager->compilationOptions.doStaticAnalysis = do_static_analysis;
    }
    else if (strcmp(optVal, "noast") == 0)
      module_manager->compilationOptions.useAST = false;
    else
      fprintf(stderr, "Unknown compiler option: '%s'\n", optVal);
  }

  if (::dgs_get_argv("absolute-path"))
  {
    module_manager->compilationOptions.useAbsolutePath = true;
  }

  if (do_static_analysis && !module_manager->compilationOptions.useAST)
  {
    do_static_analysis = false;
    fprintf(stderr, "Static Analysis won't be performed, it requires AST compiler (--compiler:ast)");
  }

  if (do_static_analysis)
  {
    sq_resetanalyserconfig();
    sq_loadanalyserconfigblk(config_blk);
  }

  module_manager->setModuleImportCallback(is_module_allowed);

  SqModules::setModuleExecutionCallbacks(before_execute_module, after_execute_module);

  allowed_native_modules = config_blk.getValuesList("allowed_native_modules");
  char_server_urls = config_blk.getValuesList("char_server_urls");

  {
    extern int dgs_argc;
    extern char **dgs_argv;
    sqstd_register_command_line_args(sqvm, dgs_argc, dgs_argv);
  }

  {
    Sqrat::RootTable rootTbl(sqvm);
    rootTbl.SquirrelFunc("__get_type_delegates", get_type_delegates, 2, ".s");
  }

  if (sq_isvartracesupported())
    sq_enablevartrace(sqvm, true);


  for (Module &m : modules)
  {
    if (config_blk.getBool(m.name, useLibsByDefault))
    {
      indexCounter++;
      if (moduleIndex < 0 || moduleIndex == indexCounter)
      {
        out_module_name.setStr(m.name);
        debug("using '%s'", m.name);
        m.func();
      }
    }
  }

  if (moduleIndex >= 0 && moduleIndex > indexCounter)
    for (Module &m : modules)
      if (config_blk.getBool(m.name, useLibsByDefault))
        m.func();


  bindquirrel::register_dagor_system(module_manager);

  if (csq_show_native_modules)
  {
    module_manager->forEachNativeModule([](const char *mn, const Sqrat::Object &) { printf("%s\n", mn); });
    quit_game(0, false);
  }

  if (moduleIndex >= 0)
  {
    Tab<String> nativeModules;
    module_manager->forEachNativeModule([&nativeModules](const char *mn, const Sqrat::Object &) { nativeModules.push_back() = mn; });
    for (int i = 0; i < nativeModules.size(); i++)
    {

      String assignCode(0, "::getroottable()[\"native_module__%s\"] <- ::require(\"%s\")", nativeModules[i].str(),
        nativeModules[i].str());
      run_sq_code(assignCode, assignCode);
    }
  }

  indexCounter++;
  if (moduleIndex < 0 || moduleIndex == indexCounter)
  {
    if (moduleIndex >= 0)
      after_execute(nullptr);

    is_execute = true;

    int executeCount = 0;
    for (int i = 0; i < config_blk.count(); i++)
    {
      const char *paramName = config_blk.keyValue(i).key.c_str();

      bool exeString = !strcmp(paramName, "execute");
      bool exeFile = !strcmp(paramName, "executeFile") || !strcmp(paramName, "execute_file");
      if (exeString || exeFile)
      {
        executeCount++;
        const char *s = config_blk.keyValue(i).value.c_str();
        if (!s[0])
          continue;

        bool saved = csq_print_to_string;
        csq_print_to_string = false;

        if (exeString)
        {
          out_module_name.printf(0, "execute \"%s\"", s);

          Sqrat::string errMsg;
          Sqrat::Script script(sqvm);
          String srcName = String(0, "(execute #%d)", executeCount);

          Sqrat::Table bindings(sqvm);
          module_manager->bindRequireApi(bindings.GetObject());
          Sqrat::Object nullThis(sqvm);

          String exportName;
          exportName.printf(0, "%s/execute:#%d", resolve_absolute_file_name(config_blk.getFileName().c_str()).c_str(), executeCount);

          before_execute_module(module_manager->getVM(), exportName.c_str(), exportName.c_str());

          if (!script.CompileString(s, errMsg, srcName.c_str(), &bindings.GetObject()) ||
              (!syntax_check_only && !script.Run(errMsg, &nullThis)))
          {
            sq_error_handler(errMsg);
            printf("In script: %s\n\n", s);
            quit_game(1, false);
          }

          after_execute_module(module_manager->getVM(), exportName.c_str(), exportName.c_str());
        }
        else
        {
          out_module_name.printf(0, "execute_file \"%s\"", s);
          String nutName = String(0, "%s%s", sqconfig_dir.str(), s);
          Sqrat::Object exports;
          String errMsg;
          if (!module_manager->requireModule(nutName, true, SqModules::__fn__, exports, errMsg))
          {
            sq_error_handler(Sqrat::string(errMsg));
            printf("In script: %s\n\n", s);
            quit_game(1, false);
          }
        }

        csq_print_to_string = saved;

        if (moduleIndex >= 0)
          after_execute(out_module_name);
      }
    }
  }


  int t0 = get_time_msec();

  if (code)
  {
    if (run_sq_code(filename, code))
      success = true;
    else
      quit_game(1, false);
  }
  else
  {
    Sqrat::Object ret;
    String errorMsg;
    success = true;

    if (syntax_check_only)
    {
      check_syntax(module_manager, filename);
    }
    else if (!module_manager->requireModule(filename, true, SqModules::__main__, ret, errorMsg))
    {
      success = false;
      report_error(errorMsg);
    }
  }

  int t1 = get_time_msec();
  if (csq_time)
    printf("Execution time: %d ms\n", t1 - t0);

  if (success && ::dgs_get_argv("export-modules-content"))
    dump_export_content();

  if (frp_graph)
    frp_graph->notifyObservablesOnShutdown(true);
  del_it(module_manager);

  perform_delayed_actions(); // explicitly flush delayed actions that might hold sq callbacks

  bindquirrel::clear_logerr_interceptors(sqvm);
  bindquirrel::http_client_on_vm_shutdown(sqvm);
  sqeventbus::unbind(sqvm);
#if HAS_CHARSQ
  charsq::shutdown();
#endif
  SquirrelVM::Shutdown(false);
  if (cpujobs::is_inited())
    cpujobs::term(true);

  if (frp_graph)
  {
    G_ASSERT(frp_graph->allObservables.empty());
    frp_graph.reset();
  }

  if (moduleIndex >= 0 && indexCounter < moduleIndex)
    success = false;

  return success;
}


static void show_modules_content(const KeyValueFile &config_blk)
{
  Tab<String> base;
  Tab<String> baseName;
  Tab<String> accum;
  Tab<String> accumName;
  Tab<String> nativeModules;

  csq_print_to_string = true;

  clear_and_shrink(csq_output_string);
  String tmpS;
  bool tmpB;
  process_file("print_sorted_root", print_sorted_root_code, config_blk, 0, tmpS, tmpB);
  split_string_lines(csq_output_string, base, baseName);


  for (int moduleIndex = 1;; moduleIndex++)
  {
    String givenName;
    bool inExecute = false;
    clear_and_shrink(csq_output_string);
    if (process_file("print_sorted_root", print_sorted_root_code, config_blk, moduleIndex, givenName, inExecute))
    {
      Tab<String> cur;
      Tab<String> curName;
      split_string_lines(csq_output_string, cur, curName);

      if (!inExecute)
      {
        printf("\n%s:\n", givenName.str());
        for (int i = 0; i < cur.size(); i++)
        {
          if (find_value_idx(baseName, curName[i]) >= 0)
            continue;

          bool overwrites = (find_value_idx(accumName, curName[i]) >= 0);
          print_key(cur[i], curName[i], overwrites);

          accum.push_back(cur[i]);
          accumName.push_back(curName[i]);
        }
      }
    }
    else
    {
      csq_print_to_string = false;
      return;
    }
  }
}


void print_usage()
{
  printf("\nUsage: csq [options] <script.nut>\n");
  printf("\nApp can be preconfigured to use some modules with .sqconfig found in folder of script on on any upper folders\n\n");
  printf("  --time - measure execution time\n");
  printf("  --debugfile - create 'debug' file\n");
  printf("  --config-path - show path to found .sqconfig\n");
  printf("  --show-basepaths - show all base paths\n");
  printf("  --set-sqconfig:<file-name> - specialize .sqconfig file\n");
  printf("  --sample-config - print reference .sqconfig in output\n");
  printf("  --modules-content - show modules content ('>' - overwrites previous)\n");
  printf("  --export-modules-content - show content of mudules (used by sq3_sa)\n");
  printf("  --native-modules - show list of available native modules\n");
  printf("  --mount-vromfs:<vrom> - all vroms will be mounted\n");
  printf("  --mount:<mount>=<path> - add mount point (--mount:darg=../../prog/daRg)\n");
  printf("  --show-mounts - print mount points\n");
  printf("  --check-fname-case - check file name case on Windows\n");
  printf("  --set-blk-root - will set blk root path (relative to current dir) for '#/' includes\n");
  printf("  --add-basepath - will add basepath (relative to current dir)\n");
  printf("  --syntax-check - don't execute, just check syntax\n");
  printf("  --final-message:<message> - print message at the end of execution without fatal errors\n");
  printf("  --compiler:<option> - set compiler option\n");
  printf("    Available options:\n");
  printf("      ast - use AST compiler\n");
  printf("      noast - use one-pass compiler\n");
  printf("  --static-analysis - perform static analysis of compiling .nut files (implies '--compiler:ast')\n");
  printf("  --inverse-warnings - flip warning diagnostic state (enbaled -> disabled and vice versa)\n");
  printf("  --warnings-list - print all warnings and exit");
  printf("  --absolute-path - use absolute path in diagnsotic render");
  printf("  --W:<id> - disable diagnostic by numeric id");
  printf("  --D:<diagnostic_id> - disable diagnostic by text id");
  printf("  --sqversion - print version of quirrel\n");
  printf("  --version - print version of csq\n");
  printf("  -- - ignore arguments after '--', but these arguments are still available in '__argv' array\n");
  printf("\n");
}

static void append_final_slash(String &dir)
{
  if (!dir.empty() && dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
    dir += "/";
}

static bool is_path_absolute(const String &path)
{
  if (path.empty())
    return false;

  if (path[0] == '/')
    return true;

  return !!strchr(path, ':');
}

static String find_sqconfig_path(String &sq_config_dir)
{
  String dir = sq_config_dir;
  int dirLenAvailable = _MAX_DIR - dir.size() - 4;
  for (int i = 0; i < dirLenAvailable / 3; i++)
  {
    String cur = dir + ".sqconfig";
    if (dd_file_exists(cur))
    {
      sq_config_dir = dir;
      return cur;
    }

    cur = dir;
    dir += "../";
    dd_simplify_fname_c(dir.data());
    int newLen = i_strlen(dir.str());
    if (newLen < dir.length())
      dir.resize(newLen + 1);
    if (cur == dir)
      break;
    if (!dir.empty() && !dd_dir_exists(dir))
      break;
  }

  return String("");
}


static int log_err_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (lev_tag != LOGLEVEL_ERR)
    return use_debug_file ? 1 : 0;

  has_errors = true;

  String s;
  if (ctx_file)
    s.printf(0, "[E] %s,%d: ", ctx_file, ctx_line);
  else
    s.printf(0, "[E] ");
  s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);

  printf("LOG ERROR: %s\n", s.str());
  return 1;
}


void sq_exit_func(int exit_code)
{
  flush_debug_file();
  quit_game(exit_code, false);
}


void on_file_open(const char *fname, void * /*file_handle*/, int flags)
{
  if (!check_fname_case)
    return;

  if (flags & DF_READ)
    if (dd_file_exists(fname))
      if (!is_file_case_valid(fname))
      {
        printf("\nERROR: Invalid file case\nTrying to open file '%s'\n", fname);
        has_errors = true;
      }
}


int DagorWinMain(bool debugmode)
{
  extern int __argc;
  extern char **__argv;

  G_UNUSED(debugmode);
  dgs_report_fatal_error = stderr_report_fatal_error;
  debug_set_log_callback(log_err_callback);
  bindquirrel::set_sq_exit_function_ptr(sq_exit_func);
  // SqModules::setModuleImportCallback(before_import_module_cb);
  // SqModules::setModuleExecutionCallbacks(before_exec_module, after_exec_module);

  dag_on_file_open = on_file_open;

  if (::dgs_get_argv("help"))
  {
    print_usage();
    return 0;
  }

  Tab<String> inputFiles(tmpmem);

  for (int i = 1; i < __argc; i++)
  {
    const char *s = __argv[i];
    if (*s == '-')
    {
      if (!strcmp(s, "--"))
        break;

      if (!strcmp(s, "--time"))
        csq_time = true;

      if (!strcmp(s, "--debugfile") && !use_debug_file)
      {
        use_debug_file = true;
        start_classic_debug_system("debug");
      }
    }
    else
      inputFiles.push_back(String(s));
  }

  if (inputFiles.size() > 0 && inputFiles[0].length() < 512)
  {
    char buf[1024] = {0};
    dd_get_fname_location(buf, inputFiles[0]);

    dd_simplify_fname_c(buf);
    if (!buf[0])
      dd_append_slash_c(buf);

    sqconfig_dir.setStr(buf);
  }

  String sqconfigPath = find_sqconfig_path(sqconfig_dir);

  if (const char *conf = ::dgs_get_argv("set-sqconfig"))
  {
    sqconfigPath.setStr(conf);
    String s = sqconfigPath;
    char *p1 = strrchr(s.str(), '\\');
    char *p2 = strrchr(s.str(), '/');
    p1 = max(p1, p2);
    if (p1)
      p1[1] = 0;
    char buf[1024] = {0};
    dd_get_fname_location(buf, s);

    dd_simplify_fname_c(buf);
    if (!buf[0])
      dd_append_slash_c(buf);
    sqconfig_dir.setStr(buf);
  }


  append_final_slash(sqconfig_dir);

  int it = 1;
  for (const char *ap; (ap = ::dgs_get_argv("add-basepath", it)) != NULL;)
    if (*ap)
    {
      String buf(ap);
      append_final_slash(buf);
      dd_append_slash_c(buf);
      dd_simplify_fname_c(buf);
      dd_add_base_path(buf);
    }

  KeyValueFile configBlk;
  configBlk.printErrorFunc = output_error_text;

  if (::dgs_get_argv("config-path"))
  {
    printf("%s\n", sqconfigPath.str());
    return 0;
  }

  if (::dgs_get_argv("sample-config"))
  {
    printf("; base_path = prog/scripts/scripts/  ; adds basepaths (relative to this .sqconfig)");
    printf("\n");
    printf("; mount = alias=prog/scripts/mount/  ; adds mount paths (relative to this .sqconfig)");
    printf("\n");
    printf("; allowed_native_modules = dagor.debug DataBlock");
    printf("\n");
    printf("\n; execute = print(\"hello\\n\")  ; execute some quirrel script, line by line of \"execute\"");
    printf("\n");
    printf("\n; execute_file = stub.nut  ; execute some .nut file (ralative to this .sqconfig)");
    printf("\n");
    printf("\nuse_libs_by_default = yes  ; what to do if library not found in this config");

    for (Module &m : modules)
      printf("\n%s = yes  ; %s", m.name, m.comment);

    printf("\n");

    return 0;
  }

  if (::dgs_get_argv("sqversion"))
  {
    printf("%s", SQUIRREL_VERSION);
    return 0;
  }

  if (::dgs_get_argv("version"))
  {
    printf("%s", APP_VERSION);
    return 0;
  }

  if (::dgs_get_argv("warnings-list"))
  {
    sq_printwarningslist(stdout);
    return 0;
  }

  if (::dgs_get_argv("static-analysis"))
  {
    do_static_analysis = true;
  }

  if (::dgs_get_argv("inverse-warnings"))
  {
    flip_diagnostic_state = true;
  }

  if (::dgs_get_argv("check-fname-case"))
  {
    check_fname_case = true;
  }

  if (const char *dir = ::dgs_get_argv("set-blk-root"))
  {
    DataBlock::setRootIncludeResolver(dir);
    printf("using BLK root resolver: %s\n", dir);
  }

  if (!sqconfigPath.empty())
  {
    bool configOk = false;
    file_ptr_t f = df_open(sqconfigPath.str(), DF_READ);
    if (f)
    {
      String data;
      int len = df_length(f);
      data.resize(len + 1);
      df_read(f, &data[0], len);
      data.back() = '\0';
      df_close(f);
      configOk = configBlk.loadFromString(data.str(), sqconfigPath.str());
    }


    if (!configOk)
    {
      printf("'%s' loaded with errors\n", sqconfigPath.str());
      return 1;
    }

    for (int i = 0; i < configBlk.count(); i++)
    {
      const char *key = configBlk.keyValue(i).key.c_str();

      if (!strcmp("base_path", key))
      {
        String path(configBlk.keyValue(i).value.c_str());
        if (!is_path_absolute(path))
          path = sqconfig_dir + path;

        append_final_slash(path);
        dd_add_base_path(path);
      }
    }

    for (int i = 0; i < configBlk.count(); i++)
    {
      const char *key = configBlk.keyValue(i).key.c_str();

      if (!strcmp("mount", key))
      {
        String s(configBlk.keyValue(i).value.c_str());
        char *eq = strchr(s.str(), '=');
        if (!eq)
        {
          printf("ERROR: expected '=' at mount: '%s'  at file '%s'\n", s.str(), sqconfigPath.str());
          return 1;
        }
        *eq = 0;
        String fullPath(0, "%s%s", sqconfig_dir.str(), eq + 1);
        dd_set_named_mount_path(s.str(), fullPath.str());
        if (::dgs_get_argv("show-mounts"))
          printf("mount: '%s' = '%s'\n", s.str(), fullPath.str());
      }
    }
  }

  int iterator = 0;
  while (const char *fn = ::dgs_get_argv("mount-vromfs", iterator))
  {
    const char *mount_at = strchr(fn, '@');
    String fn_stor;
    if (mount_at)
    {
      fn_stor.printf(0, "%.*s", mount_at - fn, fn);
      fn = fn_stor;
      mount_at++;
    }

    VirtualRomFsData *d = load_vromfs_dump(fn, inimem);
    if (!d)
    {
      printf("ERR: failed to load vromfs: %s\n", fn);
      return 1;
    }
    add_vromfs(d, false, mount_at ? str_dup(String(0, "%s/", mount_at), inimem) : NULL);
    mount_at = get_vromfs_mount_path(d);
    printf("mounted %s  at %s, %d files\n", fn, mount_at ? mount_at : "<root>", int(d->files.map.size()));
  }

  iterator = 0;
  while (const char *fn = ::dgs_get_argv("mount", iterator))
  {
    String s(fn);
    char *eq = strchr(s.str(), '=');
    if (!eq)
    {
      printf("ERR: expected '=' at --mount: '%s'\n", s.str());
      return 1;
    }
    *eq = 0;
    dd_set_named_mount_path(s.str(), eq + 1);
  }

  iterator = 0;
  while (const char *diagNum = ::dgs_get_argv("W", iterator))
  {
    int id = atoi(diagNum);
    sq_switchdiagnosticstate_i(id, false);
  }

  iterator = 0;
  while (const char *diagName = ::dgs_get_argv("D", iterator))
  {
    sq_switchdiagnosticstate_t(diagName, false);
  }

  if (::dgs_get_argv("show-basepathes") || ::dgs_get_argv("show-basepaths"))
  {
    printf("basepaths:\n");
    for (int i = 0;;)
      if (const char *p = df_next_base_path(&i))
        printf("%s\n", p);
      else
        break;

    printf("\n");
    return 0;
  }

  if (::dgs_get_argv("modules-content"))
  {
    show_modules_content(configBlk);
    return 0;
  }

  syntax_check_only = ::dgs_get_argv("syntax-check");

  csq_show_native_modules = ::dgs_get_argv("native-modules");
  if (csq_show_native_modules && inputFiles.size() == 0)
    inputFiles.push_back(String(""));

  if (inputFiles.size() != 1)
  {
    print_usage();
    return 1;
  }

  if (flip_diagnostic_state)
  {
    sq_invertwarningsstate();
  }

  String tmpS;
  bool tmpB;
  if (!process_file(inputFiles[0], nullptr, configBlk, -1, tmpS, tmpB))
    has_errors = true;

  httprequests::shutdown_async();

  if (const char *finalMsg = ::dgs_get_argv("final-message"))
    printf("%s", finalMsg);

  return has_errors ? 1 : 0;
}
