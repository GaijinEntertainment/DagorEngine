// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameScripts.h"
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/dasEvents.h>
#include <ecs/scripts/scripts.h>
#include <ecs/scripts/dascripts.h>
#include <ecs/scripts/netBindSq.h>
#include <ecs/scripts/sqEntity.h>
#include <ecs/scripts/netsqevent.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_set.h>
#include "main/vromfs.h"
#include "game/team.h"
#include "game/gameEvents.h"
#include "game/player.h"

#include "net/dedicated.h"
#include "net/netBindSq.h"
#include "main/appProfile.h"

#include <sqrat.h>
#include <sqstdmath.h>
#include <sqstdstring.h>

#include <util/dag_watchdog.h>
#include <sqModules/sqModules.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_zstdIo.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/autoBind.h>
#include <quirrel/http/sqHttpClient.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/yupfile_parse/yupfile_parse.h>
#include <quirrel/sqConsole/sqConsole.h>
#include <quirrel/sqDacoll/sqDacoll.h>
#include <quirrel/clientLog/clientLog.h>
#include <quirrel/frp/dag_frp.h>
#include <quirrel/sqJwt/sqJwt.h>
#if _TARGET_C1 | _TARGET_C2

#endif
#include "dasModules/dasScriptsLoader.h"


#include <quirrel/base64/base64.h>
#include <quirrel/nestdb/nestdb.h>
#include <quirrel/sqStackChecker.h>
#include <sqstdaux.h>
#include <ecs/scripts/dasEs.h>
#include <daScript/daScript.h>
#include <daScript/ast/ast_serializer.h>
#include <dasModules/dasFsFileAccess.h>
#include <dasModules/aotDagorConsole.h>
#include <perfMon/dag_autoFuncProf.h>
#include "ui/overlay.h"

#include <eventLog/errorLog.h>
#include <quirrel/sqEventBus/sqEventBus.h>

#include "main/scriptDebug.h"
#include <startup/dag_globalSettings.h>
#include "main/main.h"
#include <osApiWrappers/dag_direct.h>
#include <dasModules/dasModulesCommon.h>

#include <memory/dag_memStat.h>
#include <quirrel/sqDataCache/datacache.h>
#include <folders/folders.h>

#include "main/ecsUtils.h"
#include "main/gameLoad.h"

SQInteger get_thread_id_func();


#if DAS_SMART_PTR_ID
uint64_t das::ptr_ref_count::ref_count_total = 0;
uint64_t das::ptr_ref_count::ref_count_track = 0;
das::das_set<uint64_t> das::ptr_ref_count::ref_count_ids;
das::mutex das::ptr_ref_count::ref_count_mutex;
#endif

#if DAS_SMART_PTR_TRACKER
das::atomic<uint64_t> das::g_smart_ptr_total{0};
#endif


extern bool NEED_DAS_AOT_COMPILE;
// For das
void os_debug_break()
{
#if DAGOR_DBGLEVEL > 0 && defined(_MSC_VER)
  if (is_debugger_present())
    __debugbreak();
#else
  debug("das: script break");
#endif
}

extern void pull_das();

namespace bind_dascript
{
extern bool debugSerialization;
extern bool enableSerialization;
extern bool serializationReading;
extern eastl::string deserializationFileName;
extern eastl::string serializationFileName;
extern das::unique_ptr<das::SerializationStorage> initSerializerStorage;
extern das::unique_ptr<das::SerializationStorage> initDeserializerStorage;
extern das::unique_ptr<das::AstSerializer> initSerializer;
extern das::unique_ptr<das::AstSerializer> initDeserializer;
extern bool load_entry_script_with_serialization(
  const char *entry_point_name, TInitDas init, LoadEntryScriptCtx ctx, uint32_t serialized_version);
} // namespace bind_dascript

namespace gamescripts
{
static bool auto_hot_reload = false;

struct SQCloser
{
  void operator()(SQVM *vm) { vm ? sq_close(vm) : (void)0; }
};

static eastl::unique_ptr<SQVM, SQCloser> sqvm;
static eastl::unique_ptr<SqModules> moduleMgr;
static eastl::unique_ptr<sqfrp::ObservablesGraph> frp_graph;

HSQUIRRELVM get_vm() { return sqvm.get(); }

SqModules *get_module_mgr() { return moduleMgr.get(); }


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

  String str;
  str.cvprintf(128, s, vl);

  va_end(vl);

  logerr("[SQ] %s", str.str());
}

static void compile_error_handler(HSQUIRRELVM /*v*/,
  SQMessageSeverity severity,
  const SQChar *desc,
  const SQChar *source,
  SQInteger line,
  SQInteger column,
  const SQChar *)
{
  const SQChar *sevName = "error";
  if (severity == SEV_HINT)
    sevName = "hint";
  else if (severity == SEV_WARNING)
    sevName = "warning";
  String str;
  str.printf(128, "Squirrel compile %s %s (%d:%d): %s", sevName, source, line, column, desc);
  logerr("[SQ] %s", str.str());

  event_log::ErrorLogSendParams params;
  params.attach_game_log = true;
  params.meta["vm"] = "game";
  if (sceneload::is_user_game_mod())
    params.collection = dedicated::is_dedicated() ? "mods_dedicated_errorlog" : "mods_client_errorlog";
  else
    params.collection = dedicated::is_dedicated() ? "dedicated_sqassert" : "sqassert";
  event_log::send_error_log(str.str(), params);
}


static SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  G_ASSERT(sq_gettop(v) == 2);

  const char *errMsg;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  LOGERR_CTX("[SQ] %s", errMsg);

  eastl::string netErrorStr = errMsg;

  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(v)))
  {
    const char *callstack = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, -1, &callstack)));

    LOGERR_CTX("[SQ] %s", callstack);

    netErrorStr.append("\n");
    netErrorStr.append(callstack);
    sq_pop(v, 1);
  }

#if DAGOR_DBGLEVEL == 0
  event_log::ErrorLogSendParams params;
  params.attach_game_log = true;
  params.meta["vm"] = "game";
  event_log::send_error_log(netErrorStr.c_str(), params);
#endif

  return sq_suspendvm(v);
}

static const char *get_das_path_setting(const char *path)
{
  const char *fn = ::dgs_get_settings()->getStr(path, nullptr);
  if (!fn)
    logerr("%s:t= is not set. Add empty %s:t=\"\" if you really don't need it", path, path);
  return (fn && !*fn) ? nullptr : fn;
}

static int get_das_int_setting(const char *path, int def)
{
  if (::dgs_get_settings()->findParam(path) >= 0)
    return ::dgs_get_settings()->getInt(path, def);
  logerr("%s:i= is not set. Add empty %s:i=%@ if you really don't need it", path, path, def);
  return def;
}

static const char *get_das_init_entry_point() { return get_das_path_setting("game_das_script_init"); }
#if DAGOR_DBGLEVEL > 0
static const char *get_das_thread_init_script() { return get_das_path_setting("game_das_script_thread_init"); }
#endif
static const char *get_das_entry_point() { return get_das_path_setting("game_das_script"); }
static const char *get_das_project() { return get_das_path_setting("game_das_project"); }
static const char *get_das_sandbox_project() { return get_das_path_setting("game_das_sandbox_project"); }
static int get_game_das_loading_threads() { return get_das_int_setting("game_das_loading_threads", 1); }
int get_game_das_reload_threads() { return get_das_int_setting("game_das_reload_threads", 0); }

void global_init_das()
{
  // das::ptr_ref_count::ref_count_track = <insert idx of leaked smart ptr>
#if _TARGET_PC && DAGOR_DBGLEVEL > 0 // only allow auto hot reload in dev mode
  const DataBlock *debugBlk = dgs_get_settings()->getBlockByNameEx("debug");
  bool useAddonVromSrc = debugBlk->getBool("useAddonVromSrc", false);
  auto_hot_reload = debugBlk->getBool("daScriptHotReload", useAddonVromSrc);
#endif
  debug("auto_hot_reload=%d", auto_hot_reload);
  bind_dascript::enableSerialization = !das::is_in_aot() && dgs_get_settings()->getBool("game_das_enable_serialization", false);
#if defined(DAGOR_THREAD_SANITIZER)
  bind_dascript::enableSerialization = false;
#endif
  debug("game_das_enable_serialization=%d", bind_dascript::enableSerialization);

  bind_dascript::set_das_root("."); // use current dir as root path
  bind_dascript::set_command_line_arguments(dgs_argc, dgs_argv);

  const bool enableAot = NEED_DAS_AOT_COMPILE || dgs_get_settings()->getBlockByNameEx("debug")->getBool("das_aot", false);
  const bool enableAotErrorsLog = dgs_get_settings()->getBlockByNameEx("debug")->getBool("das_log_aot_errors", DAGOR_DBGLEVEL > 0);
  bind_dascript::init_das(enableAot ? bind_dascript::AotMode::AOT : bind_dascript::AotMode::NO_AOT,
    auto_hot_reload ? bind_dascript::HotReload::ENABLED : bind_dascript::HotReload::DISABLED,
    enableAotErrorsLog ? bind_dascript::LogAotErrors::YES : bind_dascript::LogAotErrors::NO);

  const Tab<const char *> ecsTags = ecs_get_global_tags_context();
  const auto devTagPos = eastl::find_if(ecsTags.begin(), ecsTags.end(), [](const char *tag) { return strcmp(tag, "dev") == 0; });
  const bool loadDebugCode = devTagPos != ecsTags.end();
  bind_dascript::init_scripts(auto_hot_reload ? bind_dascript::HotReload::ENABLED : bind_dascript::HotReload::DISABLED,
    loadDebugCode ? bind_dascript::LoadDebugCode::YES : bind_dascript::LoadDebugCode::NO, get_das_project());
  bind_dascript::init_sandbox(get_das_sandbox_project());

#if DAGOR_DBGLEVEL > 0
  // thread init script is only used in dev mode (for linter), remove this check if you need it in release too
  bind_dascript::set_thread_init_script(get_das_thread_init_script());
#endif

  set_load_threads_num(get_game_das_loading_threads());
}

void init_das()
{
  bind_dascript::pull_das();
  pull_das();
}

HSQUIRRELVM init()
{
  if (sqvm)
    return sqvm.get();

  sqvm.reset(sq_open(1024));
  G_ASSERT(sqvm);
  HSQUIRRELVM vm = sqvm.get();
  sq_limitthreadaccess(vm, ::get_current_thread_id()); // InitialLoadingThread

  moduleMgr.reset(new SqModules(vm));

  bindquirrel::apply_compiler_options_from_game_settings(moduleMgr.get());

  const DataBlock *debugBlk = dgs_get_settings()->getBlockByNameEx("debug");
  bool useAddonVromSrc = debugBlk->getBool("useAddonVromSrc", false);

  // we allow loading from real fs if useAddonVromSrc is on, otherwise it won't even work
  SqModules::tryOpenFilesFromRealFS = useAddonVromSrc;
#if DAGOR_DBGLEVEL > 0 || _TARGET_PC
  // on PC or in in dev mode on non-PC platforms we allow scripts 'modding'.
  // we also allow loading from real fs if useAddonVromSrc is on
  SqModules::tryOpenFilesFromRealFS |= debugBlk->getBool("allowScriptsModding", false);
#endif


#if DAGOR_DBGLEVEL > 0
  sq_enabledebuginfo(vm, true);
  sq_enablevartrace(vm, false);
  sq_set_thread_id_function(vm, get_thread_id_func);
  dag_sq_debuggers.initDebugger(SQ_DEBUGGER_GAME_SCRIPTS, moduleMgr.get(), "DaNetGame In-game Scripts");
  scriptprofile::register_profiler_module(vm, moduleMgr.get());
#endif

  SqStackChecker stackCheck(vm);

  sq_setprintfunc(vm, script_print_func, script_err_print_func);
  sq_setcompilererrorhandler(vm, compile_error_handler);

  sq_newclosure(vm, runtime_error_handler, 0);
  sq_seterrorhandler(vm);

  stackCheck.check();

  sqeventbus::bind(moduleMgr.get(), "game", sqeventbus::ProcessingMode::IMMEDIATE);
  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerIoStreamLib();
  moduleMgr->registerIoLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();
  moduleMgr->registerDebugLib();
  bindquirrel::register_reg_exp(moduleMgr.get());
  bindquirrel::register_utf8(moduleMgr.get());

  bindquirrel::bind_datacache(moduleMgr.get());
  bindquirrel::sqrat_bind_dagor_math(moduleMgr.get());
  bindquirrel::sqrat_bind_dagor_logsys(moduleMgr.get());
  bindquirrel::register_dagor_system(moduleMgr.get());
  bindquirrel::bind_dagor_workcycle(moduleMgr.get(), true, "gamescripts");
  bindquirrel::bind_dagor_time(moduleMgr.get());
  bindquirrel::register_iso8601_time(moduleMgr.get());
  bindquirrel::register_platform_module(moduleMgr.get());

  bindquirrel::register_dagor_fs_module(moduleMgr.get());
  bindquirrel::register_dagor_localization_module(moduleMgr.get());
  bindquirrel::register_dagor_clipboard(moduleMgr.get());
  bindquirrel::sqrat_bind_datablock(moduleMgr.get());
  bindquirrel::bind_http_client(moduleMgr.get());
  bindquirrel::bind_json_api(moduleMgr.get());

  bindquirrel::register_random(moduleMgr.get());
  bindquirrel::register_hash(moduleMgr.get());

  bindquirrel::register_yupfile_module(moduleMgr.get());

  bindquirrel::sqrat_bind_dacoll_trace_lib(moduleMgr.get());
  bindquirrel::bind_jwt(moduleMgr.get());
  clientlog::bind_script(moduleMgr.get(), "game");
#if _TARGET_C1 | _TARGET_C2

#endif
  bindquirrel::bind_base64_utils(moduleMgr.get());

  sqfrp::bind_frp_classes(moduleMgr.get());
  frp_graph.reset(new sqfrp::ObservablesGraph(vm, 16));

  create_script_console_processor(moduleMgr.get(), "sqgame.exec");
  nestdb::bind_api(moduleMgr.get());

  // bind core events before binding game events (it depends on binding of base 'ecs::Event')
  ecs_register_sq_binding(moduleMgr.get(), /*createSys*/ true, /*createFactories*/ true);

  sq::auto_bind_native_api(moduleMgr.get(), sq::VM_GAME);
  register_net_sqevent(moduleMgr.get());
  net::register_dasevents(moduleMgr.get());
  ecs::sq::bind_net(moduleMgr.get());
  net::bind_danetgame_net(moduleMgr.get());

  global_init_das();
  init_das();

  return vm;
}

const char *get_serialized_data_filename(char buf[DAGOR_MAX_PATH])
{
#if defined(__x86_64__) || defined(_M_X64)
  const char *arch = "x64";
#elif defined(__i386__) || defined(_M_IX86)
  const char *arch = "x32";
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
  const char *arch = "arm";
#elif defined(__e2k__)
  const char *arch = "e2k";
#endif

  const char *build_type = (DAGOR_DBGLEVEL == 0) ? "rel" : "dev";

#if defined(DAGOR_ADDRESS_SANITIZER)
  const char *sanitizer = "_asan";
#elif defined(DAGOR_THREAD_SANITIZER)
  const char *sanitizer = "_tsan";
#else
  const char *sanitizer = "";
#endif

  const char *_ded = dedicated::is_dedicated() ? "_ded" : "";

  snprintf(buf, DAGOR_MAX_PATH, "das_ast_%s_%s%s%s.bin", arch, build_type, _ded, sanitizer);

  return buf;
}

static eastl::string get_serialized_data_filepath(const char *name, bool temp = false)
{
  return eastl::string(eastl::string::CtorSprintf(), "%s/%s%s", folders::get_downloads_dir().c_str(), name, temp ? "XXXXXX" : "");
}

void init_serializer(const char *name)
{
  if (bind_dascript::enableSerialization)
  {
    bind_dascript::serializationFileName = get_serialized_data_filepath(name, /*temp*/ true);
    if (auto file = df_mkstemp(&bind_dascript::serializationFileName[0]))
    {
      debug("das: serialize: writing serialized data to %s", bind_dascript::serializationFileName.c_str());
      bind_dascript::initSerializerStorage.reset(
        bind_dascript::create_file_write_serialization_storage(file, bind_dascript::serializationFileName));
      bind_dascript::initSerializer = das::make_unique<das::AstSerializer>(bind_dascript::initSerializerStorage.get(), true);
    }
    else
      logerr("das: serialize: could not write serialized data to file '%s'", bind_dascript::serializationFileName.c_str());
  }
}

uint32_t initialize_deserializer_(const char *name)
{
  bind_dascript::deserializationFileName = get_serialized_data_filepath(name);
  debug("das: serialize: reading cache file '%s'", bind_dascript::deserializationFileName.c_str());
  if (dgs_get_settings()->getBool("game_das_serialization_remove_cache", false))
  {
    dd_erase(bind_dascript::deserializationFileName.c_str());
  }
  const file_ptr_t file = df_open(bind_dascript::deserializationFileName.c_str(), DF_READ);
  if (!file)
  {
    debug("das: serialize: could not open serialized data file");
    return -1;
  }

  das::unique_ptr<das::SerializationStorage> buffer(
    bind_dascript::create_file_read_serialization_storage(file, bind_dascript::deserializationFileName));
  bind_dascript::initDeserializerStorage = das::move(buffer);
  bind_dascript::initDeserializer = das::make_unique<das::AstSerializer>(bind_dascript::initDeserializerStorage.get(), false);

  uint32_t serializedVersion = -1, ptrsize = 0, protocolVersion = 0;
  const bool ok = bind_dascript::initDeserializer->trySerialize(
    [&](das::AstSerializer &ser) { ser << serializedVersion << protocolVersion << ptrsize; });

  if (!ok)
  {
    logwarn("das: serialize: could not read version from serialized data");
    return -1;
  }
  if (bind_dascript::initDeserializer->getVersion() != protocolVersion)
  {
    logwarn("das: serialize: protocol version changed. Expected %d, got %d", bind_dascript::initDeserializer->getVersion(),
      protocolVersion);
    return -1;
  }

  if (ptrsize != sizeof(void *))
  {
    logwarn("das: serialize: serialized pointer size mismatch, expected %@ bytes, got %@", sizeof(void *), ptrsize);
    return -1; // TODO: more portable
  }

  return serializedVersion;
}


uint32_t initialize_deserializer(const char *name)
{
  init_serializer(name);
  return initialize_deserializer_(name);
}


void load_sq_module(const char *fn, bool is_entrypoint)
{
  Sqrat::Object exports;
  String errMsg;
  const char *name = is_entrypoint ? SqModules::__main__ : SqModules::__fn__;
  if (!moduleMgr->requireModule(fn, false, name, exports, errMsg))
    logerr("Failed to run script '%s': %s", fn, errMsg.c_str());
}


static const char *sq_get_game_main_script_fn()
{
  const char *fn = ::dgs_get_settings()->getStr("game_sq_script", nullptr);
  if (!fn)
    logerr("game_sq_script:t= not set, SQ game module will not be started");
  return (fn && !*fn) ? nullptr : fn;
}


void reload_squirrel(size_t memUsed)
{
  memUsed = dagor_memory_stat::get_memory_allocated(true) - memUsed;
  debug("daScript: mem usage after init load %dK\n", memUsed / 1024);

  if (const char *main_script_fn = sq_get_game_main_script_fn())
  {
    const updater::Version wasVersion = get_sq_version();
    const updater::Version curVersion = max(wasVersion, get_updated_game_version());
    set_sq_version(curVersion);
    debug("Reload SQ: %s -> %s", wasVersion.to_string(), curVersion.to_string());

    load_sq_module(main_script_fn, /*entrypoint*/ true);
  }
  // end loading sq modules
}


bool run_with_serialization()
{
  G_ASSERT(sqvm);

  Sqrat::Object exports;
  String errMsg;

  start_es_loading(); // prevents calls to es_reset_order during es registrations

  size_t memUsed = dagor_memory_stat::get_memory_allocated(true);

  const uint32_t serializedVersion = [] {
    char buf[DAGOR_MAX_PATH];
    return initialize_deserializer(get_serialized_data_filename(buf));
  }();
  debug("das: serialize: Checking against serialized AST:", serializedVersion);
  debug("das: serialize: - Serialized version: %lu - %s", serializedVersion, serializedVersion == -1 ? "missing (corrupted)" : "ok");

  bind_dascript::enableSerialization = false;
  reload_das_init();
  reload_squirrel(memUsed);

  bool success = false;

  if (const char *dasEntryPoint = get_das_entry_point())
  {
    if (dd_file_exist(dasEntryPoint))
    {
      const updater::Version wasVersion = get_das_version();
      const updater::Version curVersion = max(wasVersion, get_updated_game_version());
      set_das_version(curVersion);
      debug("Reload Das: %s -> %s", wasVersion.to_string(), curVersion.to_string());
      success = load_entry_script_with_serialization(dasEntryPoint, &init_das, bind_dascript::LoadEntryScriptCtx{int64_t(memUsed)},
        serializedVersion);
    }
    else
    {
      logerr("daScript entry point <%s> can't be found", dasEntryPoint);
    }
  }
  else
  {
    debug("dascript: entry point is empty, skip starting");
  }

  end_es_loading(); // call es_reset_order during es registrations

  bind_dascript::free_serializer_buffer(success);

  return true;
}

bool run()
{
  if (bind_dascript::enableSerialization)
    return run_with_serialization();

  G_ASSERT(sqvm);

  Sqrat::Object exports;
  String errMsg;

  start_es_loading(); // prevents calls to es_reset_order during es registrations

  size_t memUsed = dagor_memory_stat::get_memory_allocated(true);
  reload_das_init();
  memUsed = dagor_memory_stat::get_memory_allocated(true) - memUsed;
  debug("daScript: mem usage after init load %dK\n", memUsed / 1024);

  if (const char *main_script_fn = sq_get_game_main_script_fn())
  {
    const updater::Version wasVersion = get_sq_version();
    const updater::Version curVersion = max(wasVersion, get_updated_game_version());
    set_sq_version(curVersion);
    debug("Reload SQ: %s -> %s", wasVersion.to_string(), curVersion.to_string());

    load_sq_module(main_script_fn, /*entrypoint*/ true);
  }
  // end loading sq modules

  if (const char *dasEntryPoint = get_das_entry_point())
  {
    if (dd_file_exist(dasEntryPoint))
    {
      const updater::Version wasVersion = get_das_version();
      const updater::Version curVersion = max(wasVersion, get_updated_game_version());
      set_das_version(curVersion);
      debug("Reload Das: %s -> %s", wasVersion.to_string(), curVersion.to_string());

      bind_dascript::start_multiple_scripts_loading();
      bind_dascript::load_entry_script(dasEntryPoint, &init_das, bind_dascript::LoadEntryScriptCtx{int64_t(memUsed)});
      bind_dascript::drop_multiple_scripts_loading(); // unset das_is_in_init_phase, es_reset_order calls in the end of function
    }
    else
    {
      logerr("daScript entry point <%s> can't be found", dasEntryPoint);
    }
  }
  else
  {
    debug("dascript: entry point is empty, skip starting");
  }

  end_es_loading(); // call es_reset_order during es registrations

  return true;
}

bool run_user_game_mode_entry_scripts(const char *das_entry_fn, const char *sq_entry_fn)
{
  bind_dascript::set_sandbox(true);
  // const char *ugm_das_script_module = "project_ugm.das_project";
  if (das_entry_fn && dd_file_exist(das_entry_fn))
  {
    debug("daScript: load UGM script '%s'", das_entry_fn);
    bind_dascript::start_multiple_scripts_loading();
    bind_dascript::load_entry_script(das_entry_fn, &init_das);
    bind_dascript::end_multiple_scripts_loading();
    bind_dascript::main_thread_post_load();
  }
  if (sq_entry_fn && dd_file_exist(sq_entry_fn))
  {
    debug("SQ: load UGM script '%s'", sq_entry_fn);
    start_es_loading();
    load_sq_module(sq_entry_fn, /*entrypoint*/ true);
    end_es_loading();
  }
  return true;
}
void unload_user_game_mode_scripts(const char *fn_prefix)
{
  debug("unload_user_game_mode_scripts: %s", fn_prefix);
  bind_dascript::start_multiple_scripts_loading();
  bind_dascript::set_sandbox(false);
  bind_dascript::end_multiple_scripts_loading();
  //== unload SQ modules here
}

bool run_game_mode_entry_scripts(const char *das_entry_fn)
{
  bind_dascript::set_game_mode(true);
  if (das_entry_fn && dd_file_exist(das_entry_fn))
  {
    debug("daScript: load game mode script '%s'", das_entry_fn);
    bind_dascript::start_multiple_scripts_loading();
    bind_dascript::load_das_script(das_entry_fn); // sync loading, redo to async if need it
    bind_dascript::end_multiple_scripts_loading();
    bind_dascript::dump_statistics();
  }
  return true;
}

bool unload_game_mode_scripts()
{
  debug("daScript unload game mode scripts");
  bind_dascript::start_multiple_scripts_loading();
  bind_dascript::set_game_mode(false);
  bind_dascript::end_multiple_scripts_loading();
  return true;
}

void unload_all_das_scripts_without_debug_agents() { bind_dascript::unload_all_das_scripts_without_debug_agents(); }

void das_load_ecs_templates() { bind_dascript::das_load_ecs_templates(); }

void collect_garbage()
{
  // Note: this need to be logerr but we have to implement diagnostics first (see sq_resurrectunreachable)
  if (int numRefCycles = sq_collectgarbage(get_vm()))
    logwarn("%d ref cycles collected in game sq vm", numRefCycles);
}

void update_deferred()
{
  if (frp_graph)
  {
    String errMsg;
    frp_graph->updateDeferred(errMsg);
  }
}

static unsigned frame = 0;
bool reload_changed()
{
  if (!auto_hot_reload)
    return false;
  if (((++frame) & 31) != 0)
    return false;
  TIME_PROFILE(reload_changed_scripts);
#if DAGOR_DBGLEVEL > 0
  if (das::hasDebugAgentContext("breakpointsHook", nullptr, nullptr))
    das::tickSpecificDebugAgent("breakpointsHook");
#endif
  const bool reloaded = bind_dascript::reload_all_changed() == bind_dascript::RELOADED;
  if (reloaded)
  {
    bind_dascript::bind_das_events(moduleMgr.get());
    bind_dascript::bind_das(moduleMgr.get());
  }
  return reloaded;
}

void collect_das_garbage()
{
  if (dgs_get_settings()->getBool("game_das_enable_gc", true))
  {
    bind_dascript::ValidateDasGC validateGc = bind_dascript::ValidateDasGC::NO;
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
    validateGc = bind_dascript::ValidateDasGC::YES;
#endif
    bind_dascript::collect_heap(/*scale threshold*/ 0.8f, validateGc);
  }
}

static updater::Version current_sq_version;
static updater::Version current_das_version;

void set_sq_version(updater::Version version) { current_sq_version = version; }

void set_das_version(updater::Version version) { current_das_version = version; }

updater::Version get_sq_version()
{
  if (!current_sq_version)
    current_sq_version = get_game_build_version();

  return current_sq_version;
}

updater::Version get_das_version()
{
  if (!current_das_version)
    current_das_version = get_game_build_version();

  return current_das_version;
}

bool main_thread_post_load()
{
  G_ASSERT(::get_current_thread_id() == ::get_main_thread_id());
  sq_limitthreadaccess(get_vm(), ::get_current_thread_id());

  bind_dascript::das_add_con_proc_dagor_console();

  const bool res = bind_dascript::main_thread_post_load();
  bind_dascript::bind_das_events(moduleMgr.get());
  bind_dascript::bind_das(moduleMgr.get());
  return res;
}

bool reload_sq_modules(ReloadMode mode)
{
  AutoFuncProfT<AFP_MSEC> _prof("[RELOAD] gamescripts::reload_sq_modules() %d msec");

  const updater::Version wasVersion = get_sq_version();
  const updater::Version curVersion = max(wasVersion, get_updated_game_version());
  set_sq_version(curVersion);
  debug("Reload SQ: %s -> %s", wasVersion.to_string(), curVersion.to_string());

  Sqrat::Object exports;
  String errMsg;
  // library reload may be added if needed
  start_es_loading(); // prevents calls to es_reset_order during es registrations

  HSQUIRRELVM vm = moduleMgr->getVM();
  clear_script_console_processor(vm);
  bindquirrel::clear_workcycle_actions(vm);
  sqeventbus::clear_on_reload(vm);
  frp_graph->shutdown(false);

  bool res = true;
  if (const char *main_script_fn = sq_get_game_main_script_fn())
  {
    res = moduleMgr->reloadModule(main_script_fn, true, SqModules::__main__, exports, errMsg);
    if (!res)
      logerr("Failed to run script '%s': %s", main_script_fn, errMsg.c_str());
  }

  end_es_loading(); // call es_reset_order during es registrations

  if (mode != ReloadMode::SILENT_FULL_RELOAD && res)
  {
    g_entity_mgr->broadcastEvent(EventScriptReloaded());
    ecs::EntityId hero = game::get_controlled_hero();
    debug("%s Emit hero changed event %d", __FUNCTION__, ecs::entity_id_t{hero});
    g_entity_mgr->broadcastEvent(EventHeroChanged(hero));
  }

  return res;
}

// NB: this function doesn't update es order
void reload_das_init()
{
  bind_dascript::load_event_files(true);
  if (const char *dasInitPoint = get_das_init_entry_point())
  {
    if (dd_file_exist(dasInitPoint))
    {
      debug("dascript: load init script '%s'", dasInitPoint);
      bind_dascript::start_multiple_scripts_loading();
      bind_dascript::load_das_script(dasInitPoint);
      bind_dascript::drop_multiple_scripts_loading(); // unset das_is_in_init_phase, es_reset_order should be called later
      bind_dascript::bind_das_events(moduleMgr.get());
      bind_dascript::bind_das(moduleMgr.get());
    }
    else
    {
      logerr("dascript: no init script '%s'", dasInitPoint);
    }
  }
  else
  {
    debug("dascript: init script is empty, skip initialization");
  }
  bind_dascript::load_event_files(false);
}

// NB: reload init script first gamescripts::reload_das_init();
bool reload_das_modules()
{
  AutoFuncProfT<AFP_MSEC> _prof("[RELOAD] gamescripts::reload_das_modules %d msec");
  if (const char *dasEntryPoint = get_das_entry_point())
  {
    const updater::Version wasVersion = get_das_version();
    const updater::Version curVersion = max(wasVersion, get_updated_game_version());
    set_das_version(curVersion);
    debug("Reload Das: %s -> %s", wasVersion.to_string(), curVersion.to_string());

    bind_dascript::start_multiple_scripts_loading();
    const bool success = bind_dascript::load_entry_script(dasEntryPoint, &init_das);
    bind_dascript::end_multiple_scripts_loading();

    bind_dascript::free_serializer_buffer(success);

    return success;
  }
  return false;
}

void set_das_aot(bool value) { bind_dascript::set_das_aot(value ? bind_dascript::AotMode::AOT : bind_dascript::AotMode::NO_AOT); }

bool get_das_aot() { return bind_dascript::get_das_aot() == bind_dascript::AotMode::AOT; }

void set_das_log_aot_errors(bool value)
{
  bind_dascript::set_das_log_aot_errors(value ? bind_dascript::LogAotErrors::YES : bind_dascript::LogAotErrors::NO);
}

bool get_das_log_aot_errors() { return bind_dascript::get_das_log_aot_errors() == bind_dascript::LogAotErrors::YES; }

int set_load_threads_num(int num) { return bind_dascript::set_load_threads_num(num); }

int get_load_threads_num() { return bind_dascript::get_load_threads_num(); }

void shutdown_before_ecs()
{
  if (dedicated::is_dedicated())
  {
    static constexpr int HTTP_CLIENT_TIMEOUT_MS = 60000;
    ScopeSetWatchdogTimeout _wd(HTTP_CLIENT_TIMEOUT_MS + 1000);
    bindquirrel::http_client_wait_active_requests(HTTP_CLIENT_TIMEOUT_MS);
  }
}

void shutdown()
{
  if (!sqvm)
    return;

  g_entity_mgr->broadcastEventImmediate(EventOnGameScriptsShutdown());
  bindquirrel::shutdown_datacache();
  bindquirrel::http_client_on_vm_shutdown(sqvm.get());
  bindquirrel::cleanup_dagor_workcycle_module(sqvm.get());
  bindquirrel::clear_logerr_interceptors(sqvm.get());
  sqeventbus::unbind(sqvm.get());

  frp_graph->shutdown(true);

  shutdown_ecs_sq_script(sqvm.get());
  bind_dascript::shutdown_systems();

  destroy_script_console_processor(sqvm.get());

  scriptprofile::shutdown(sqvm.get());

#if DAGOR_DBGLEVEL > 0
  dag_sq_debuggers.shutdownDebugger(SQ_DEBUGGER_GAME_SCRIPTS);
#endif

  moduleMgr.reset();

  sq_collectgarbage(sqvm.get());
  sqvm.reset();

  G_ASSERT(frp_graph->allObservables.empty());
  frp_graph.reset();
}


} // namespace gamescripts
