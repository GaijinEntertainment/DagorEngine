// Copyright (C) Gaijin Games KFT.  All rights reserved.

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
#include <sqastio.h>

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_delayedAction.h>
#include <gui/dag_visualLog.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
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
#include <string>
#include <set>

#include "fs_utils.h"
#include "scriptapi.h"


using namespace sqimportparser;


#define APP_VERSION "1.0.25"

// Stubs

namespace visuallog
{
// stub for dagor debug module
void logmsg(const char *, LogItemCBProc, void *, E3DCOLOR, int) {}
} // namespace visuallog

void ShaderElement::invalidate_cached_state_block() {}


extern int __argc;
extern char **__argv;
static int argc_limit = -1; // index of '--' argument

static bool syntax_check_only = false;
static bool has_errors = false;
static bool csq_time = false;
static bool csq_show_native_modules = false;
static bool csq_show_native_modules_content = false;
static bool use_debug_file = false;
static bool check_fname_case = false;
static bool do_static_analysis = false;
static bool flip_diagnostic_state = false;
static bool dump_bytecode = false;
static bool dump_ast = false;
static String sqconfig_dir;

static HSQUIRRELVM sqvm = nullptr;
static SqModules *module_manager = nullptr;

static eastl::unique_ptr<sqfrp::ObservablesGraph> frp_graph;
static std::vector<std::string> allowed_native_modules;
static std::vector<std::string> char_server_urls;


static void stderr_report_fatal_error(const char *, const char *msg, const char *call_stack)
{
  if (sqvm)
  {
    sqstd_printcallstack(sqvm);
    printf("\n");
  }
  printf("Fatal error: %s\n%s", msg, call_stack);
  flush_debug_file();
  quit_game(1, false);
}


static void output_text(const char *msg)
{
  printf("%s", msg);
  debug_("%s", msg);
}


static void output_error_text(const char *msg) { output_text(msg); }


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
  {"sqstd_register_base_libs", "math, string, iostream, debug",
    [] {
      module_manager->registerMathLib();
      module_manager->registerStringLib();
      if (!module_manager->findNativeModule("debug"))
        module_manager->registerDebugLib();
      if (!module_manager->findNativeModule("io"))
        module_manager->registerIoStreamLib();
    }},
  {"sqstd_register_system_lib", "system lib", [] { module_manager->registerSystemLib(); }},
  {"sqstd_register_datetime_lib", "datetime lib", [] { module_manager->registerDateTimeLib(); }},
  {"sqstd_register_debug_lib", "debug lib",
    [] {
      if (!module_manager->findNativeModule("debug"))
        module_manager->registerDebugLib();
    }},
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
      frp_graph.reset(new sqfrp::ObservablesGraph(sqvm, "csq"));
    }},
  {"register_localization", "doesLocTextExist, getLocTextForLang and so on for localizations",
    [] { bindquirrel::register_dagor_localization_module(module_manager); }},
  {"register_clipboard", "register_dagor_clipboard", [] { bindquirrel::register_dagor_clipboard(module_manager); }},

};


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

  if (!remove_import_from_code(source.str(), filename))
    quit_game(1, false);

  SQCompilation::SqASTData *ast =
    sq_parsetoast(sqvm, source.str(), source.length(), filename, /*static analysis*/ false, /* raise error*/ true);

  if (!ast)
    quit_game(1, false);

  sq_releaseASTData(sqvm, ast);
}


static FILE *diag_json_file = nullptr;

static void escape_json(const char *input, eastl::string &output)
{
  output.clear();
  output.reserve(64);

  int i = 0;
  while (input[i])
  {
    switch (input[i])
    {
      case '"': output += "\\\""; break;
      case '/': output += "\\/"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      case '\\': output += "\\\\"; break;
      default: output += input[i]; break;
    }

    i++;
  }
}


static void compiler_diag_cb(HSQUIRRELVM, const SQCompilerMessage *msg)
{
  G_ASSERT_RETURN(diag_json_file, );

  eastl::string escapedMsg;
  eastl::string escapedFile;
  escape_json(msg->message, escapedMsg);
  escape_json(msg->fileName, escapedFile);

  fprintf(diag_json_file,
    "\n{\"line\":%d,\"col\":%d,\"len\":%d,\"file\":\"%s\",\"intId\":%d,\"textId\":\"%s\",\"message\":\"%s\",\"isError\":%s}, ",
    msg->line, msg->column + 1, max(1, msg->columnsWidth), escapedFile.c_str(), msg->intId, msg->textId, escapedMsg.c_str(),
    msg->isError ? "true" : "false");
}


static void compiler_error_cb(HSQUIRRELVM v, SQMessageSeverity severity, const SQChar *sErr, const SQChar *sSource, SQInteger line,
  SQInteger column, const SQChar *extra)
{
  if (severity > SEV_HINT)
    has_errors = true;
  script_print_function(v, _SC("%s\n"), sErr);
  script_print_function(v, _SC("%s:%d:%d\n"), sSource, (int)line, (int)column);
  if (extra)
    script_print_function(v, _SC("%s\n"), extra);
}


static void dump_ast_callback(HSQUIRRELVM vm, SQCompilation::SqASTData *ast_data, void *opts)
{
  G_UNUSED(opts);
  if (!ast_data)
    return;
  FileOutputStream fos(stdout);
  sq_dumpast(vm, ast_data, &fos);
}


static void dump_bytecode_callback(HSQUIRRELVM vm, HSQOBJECT obj, void *opts)
{
  G_UNUSED(opts);
  FileOutputStream fos(stdout);
  sq_dumpbytecode(vm, obj, &fos);
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
  sq_setcompilererrorhandler(sqvm, ::compiler_error_cb);

  bool useLibsByDefault = config_blk.getBool("use_libs_by_default", true);

  module_manager = new SqModules(sqvm, SqModulesConfigBits::PermissiveModuleNames); // allow stub native modules with script files

  if (::dgs_get_argv("absolute-path"))
  {
    module_manager->compilationOptions.useAbsolutePath = true;
  }

  if (do_static_analysis)
  {
    sq_enablesyntaxwarnings(true);
  }

  if (dump_ast)
  {
    module_manager->onAST_cb = &dump_ast_callback;
  }

  if (dump_bytecode)
  {
    module_manager->onBytecode_cb = &dump_bytecode_callback;
  }

  int iterator = 0;
  while (const char *diagNum = ::dgs_get_argv("W", iterator))
  {
    int id = atoi(diagNum);
    sq_setdiagnosticstatebyid(id, false);
  }

  iterator = 0;
  while (const char *diagName = ::dgs_get_argv("D", iterator))
  {
    sq_setdiagnosticstatebyname(diagName, false);
  }

  if (do_static_analysis)
  {
    module_manager->compilationOptions.doStaticAnalysis = true;
    sq_resetanalyzerconfig();
    sq_loadanalyzerconfigblk(config_blk);
    sq_setcompilationoption(sqvm, CompilationOptions::CO_CLOSURE_HOISTING_OPT, false);
  }

  module_manager->setModuleImportCallback(is_module_allowed);

  allowed_native_modules = config_blk.getValuesList("allowed_native_modules");
  char_server_urls = config_blk.getValuesList("char_server_urls");

  sqstd_register_command_line_args(sqvm, __argc, __argv);
  register_csq_module(module_manager);
  bindquirrel::register_dagor_system(module_manager);

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


  if (csq_show_native_modules)
  {
    module_manager->forEachNativeModule([](const char *mn, const Sqrat::Object &) { printf("%s\n", mn); });
    quit_game(0, false);
  }

  if (csq_show_native_modules_content)
  {
    Sqrat::string errMsg;
    Sqrat::Script script(sqvm);

    Sqrat::Table bindings(sqvm);
    module_manager->bindRequireApi(bindings.GetObject());
    module_manager->bindBaseLib(bindings.GetObject());
    Sqrat::Object nullThis(sqvm);

    module_manager->forEachNativeModule([&](const char *mn, const Sqrat::Object &) {
      printf("\nModule: %s\n", mn);
      String code(0,
        "local a = []\n"
        "foreach (k, v in require(\"%s\")) {\n"
        "  a.append($\"  {k} - {typeof v}\")\n"
        "  if (typeof v == \"class\") {\n"
        "    local a2 = []\n"
        "    foreach (k2, v2 in v) a2.append($\"    {k}.{k2} - {typeof v2}\")\n"
        "    a[a.len() - 1] += $\"\\n{\"\\n\".join(a2.sort())}\"\n"
        "  }\n"
        "}\n"
        "foreach (_, s in a.sort()) println(s);",
        mn);

      if (!script.CompileString(code.c_str(), errMsg, code.c_str(), &bindings.GetObject()) || !script.Run(errMsg, &nullThis))
      {
        sq_error_handler(errMsg);
        printf("In script: %s\n\n", code.c_str());
        quit_game(1, false);
      }
    });
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

          if (!script.CompileString(s, errMsg, srcName.c_str(), &bindings.GetObject()) ||
              (!syntax_check_only && !script.Run(errMsg, &nullThis)))
          {
            sq_error_handler(errMsg);
            printf("In script: %s\n\n", s);
            quit_game(1, false);
          }

          if (do_static_analysis)
          {
            sq_mergeglobalnames(&bindings.GetObject());
          }
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

    if (const char *diagOutFn = ::dgs_get_argv("message-output-file"))
    {
      diag_json_file = fopen(diagOutFn, "wb");
      if (!diag_json_file)
      {
        String errMsg(0, "Error: cannot open file '%s' for writing", diagOutFn);
        report_error(errMsg);
        quit_game(1, false);
      }
      fputs("{\"messages\":[  ", diag_json_file);
      sq_setcompilerdiaghandler(sqvm, compiler_diag_cb);
    }

    if (syntax_check_only)
    {
      check_syntax(module_manager, filename);
    }
    else if (!module_manager->requireModule(filename, true, do_static_analysis ? SqModules::__analysis__ : SqModules::__main__, ret,
               errorMsg))
    {
      success = false;
      report_error(errorMsg);
    }
  }

  if (do_static_analysis)
  {
    sq_checkglobalnames(sqvm);
  }

  if (diag_json_file)
  {
    fseek(diag_json_file, -2, SEEK_CUR);
    fputs("]}\n", diag_json_file);
    fclose(diag_json_file);
    diag_json_file = nullptr;
  }

  if (const char *file_list_name = ::dgs_get_argv("visited-files-list"))
  {
    std::set<std::string> existed_files;
    FILE *files_list_file = fopen(file_list_name, "wb");
    if (files_list_file)
    {
      auto &visitedModules = module_manager->modules;

      for (auto &m : visitedModules)
      {
        const char *resolved_fn = m.fn.c_str();

        auto r = existed_files.emplace(resolved_fn);
        if (r.second)
        {
          fprintf(files_list_file, "%s\n", resolved_fn);
        }
      }

      fclose(files_list_file);
    }
  }

  int t1 = get_time_msec();
  if (csq_time)
    printf("Execution time: %d ms\n", t1 - t0);

  if (frp_graph)
    frp_graph->shutdown(true);
  del_it(module_manager);

  perform_delayed_actions(); // explicitly flush delayed actions that might hold sq callbacks

  bindquirrel::clear_logerr_interceptors(sqvm);
  bindquirrel::http_client_on_vm_shutdown(sqvm);
  sqeventbus::unbind(sqvm);
#if HAS_CHARSQ
  charsq::shutdown();
#endif
  sqvm = nullptr;
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


static void print_usage()
{
  printf("\nUsage: csq [options] <script.nut>\n");
  printf("\nApp can be preconfigured to use some modules with .sqconfig found in folder of script on on any upper folders\n\n");
  printf("  --time - measure execution time\n");
  printf("  --debugfile - create 'debug' file\n");
  printf("  --config-path - show path to found .sqconfig\n");
  printf("  --show-basepaths - show all base paths\n");
  printf("  --set-sqconfig:<file-name> - specialize .sqconfig file\n");
  printf("  --sample-config - print reference .sqconfig in output\n");
  printf("  --native-modules - show list of available native modules\n");
  printf("  --native-modules-content - show list of available native modules and content of modules\n");
  printf("  --mount-vromfs:<vrom> - all vroms will be mounted\n");
  printf("  --mount:<mount>=<path> - add mount point (--mount:darg=../../prog/daRg)\n");
  printf("  --show-mounts - print mount points\n");
  printf("  --check-fname-case - check file name case on Windows\n");
  printf("  --set-blk-root - will set blk root path (relative to current dir) for '#/' includes\n");
  printf("  --add-basepath - will add basepath (relative to current dir)\n");
  printf("  --syntax-check - don't execute, just check syntax\n");
  printf("  --final-message:<message> - print message at the end of execution without fatal errors\n");
  printf("  --static-analysis - perform static analysis of compiling .nut files\n");
  printf("  --message-output-file:<file-name> - print compiler messages to file (JSON)\n");
  printf("  --warnings-list - print all warnings and exit\n");
  printf("  --absolute-path - use absolute path in diagnsotic render\n");
  printf("  --visited-files-list:<file_name> - dump all visited scripts file names into specified file (appends if existed)\n");
  printf("  --W:<id> - disable diagnostic by numeric id\n");
  printf("  --D:<diagnostic_id> - disable diagnostic by text id\n");
  printf("  --inverse-warnings - flip warning diagnostic state (enabled -> disabled and vice versa)\n");
  printf("  --dump-bytecode - dump bytecode and line infos of compiled script\n");
  printf("  --dump-ast - dump AST of compiled script\n");
  printf("  --sqversion - print version of quirrel\n");
  printf("  --version - print version of csq\n");
  printf("  --ignore-unknown-args - ignore unknown command line arguments\n");
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


static void sq_exit_func(int exit_code)
{
  flush_debug_file();
  quit_game(exit_code, false);
}


static void on_file_open(const char *fname, void * /*file_handle*/, int flags)
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


static int check_unused_args()
{
  if (::dgs_get_argv("ignore-unknown-args"))
    return has_errors;

  for (int i = 1; i < argc_limit; i++)
    if (!dgs_is_arg_used(i))
    {
      has_errors = true;
      printf("\nERROR: Invalid argument '%s'\n", __argv[i]);
    }

  return has_errors ? 1 : 0;
}


int DagorWinMain(bool debugmode)
{
  G_UNUSED(debugmode);
  argc_limit = __argc;
  dgs_report_fatal_error = stderr_report_fatal_error;
  debug_set_log_callback(log_err_callback);
  bindquirrel::set_sq_exit_function_ptr(sq_exit_func);

  dag_on_file_open = on_file_open;

  if (::dgs_get_argv("help"))
  {
    print_usage();
    return check_unused_args();
  }

  Tab<String> inputFiles(tmpmem);

  for (int i = 1; i < __argc; i++)
  {
    const char *s = __argv[i];
    if (*s == '-')
    {
      if (!strcmp(s, "--"))
      {
        argc_limit = i;
        break;
      }

      if (!strcmp(s, "--time"))
      {
        csq_time = true;
        dgs_set_arg_used(i);
      }

      if (!strcmp(s, "--debugfile"))
      {
        dgs_set_arg_used(i);
        if (!use_debug_file)
        {
          use_debug_file = true;
          start_classic_debug_system("debug");
        }
      }
    }
    else
    {
      dgs_set_arg_used(i);
      inputFiles.push_back(String(s));
    }
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
    return check_unused_args();
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

    return check_unused_args();
  }

  if (::dgs_get_argv("sqversion"))
  {
    printf("%s\n", SQUIRREL_VERSION);
    return check_unused_args();
  }

  if (::dgs_get_argv("version"))
  {
    printf("%s\n", APP_VERSION);
    return check_unused_args();
  }

  if (::dgs_get_argv("warnings-list"))
  {
    sq_printwarningslist(stdout);
    return check_unused_args();
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

  if (::dgs_get_argv("dump-bytecode"))
  {
    dump_bytecode = true;
  }

  if (::dgs_get_argv("dump-ast"))
  {
    dump_ast = true;
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
      printf("ERROR: failed to load vromfs: %s\n", fn);
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

  if (::dgs_get_argv("show-basepathes") || ::dgs_get_argv("show-basepaths"))
  {
    printf("basepaths:\n");
    for (int i = 0;;)
      if (const char *p = df_next_base_path(&i))
        printf("%s\n", p);
      else
        break;

    printf("\n");
    return check_unused_args();
  }

  syntax_check_only = ::dgs_get_argv("syntax-check");

  csq_show_native_modules = ::dgs_get_argv("native-modules");
  csq_show_native_modules_content = ::dgs_get_argv("native-modules-content");
  if ((csq_show_native_modules || csq_show_native_modules_content) && inputFiles.size() == 0)
    inputFiles.push_back(String(""));

  if (inputFiles.size() == 0)
  {
    print_usage();
    return 1;
  }

  if (flip_diagnostic_state)
  {
    sq_invertwarningsstate();
  }

  d3d::init_driver();

  for (String &fileName : inputFiles)
  {
    String tmpS;
    bool tmpB;
    if (!process_file(fileName, nullptr, configBlk, -1, tmpS, tmpB))
      has_errors = true;
  }

  httprequests::shutdown_async();
  d3d::release_driver();

  if (const char *finalMsg = ::dgs_get_argv("final-message"))
    printf("%s", finalMsg);

  return check_unused_args();
}
