// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/event.h>
#include <sqrat.h>
#include <sqstdblob.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_hwExcept.h>
#include <osApiWrappers/dag_symHlp.h>
#include <util/dag_delayedAction.h>
#include "main/hostedServerLauncher.h"
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <sqmodules/sqmodules.h>
#include <debug/dag_debug.h>
#include <mutex>
#if _TARGET_C2

#elif _TARGET_XBOX
#include "platform/gdk/dedSrv/fill_init_values.h"
#else
static void fill_platform_specific_init_values(DataBlock &) {}
#endif
#include <gameRes/dag_gameResProxyTable.h>

#if _TARGET_PC | _TARGET_XBOX
#include <supp/dag_dllexport.h>
#include <memory/dag_dbgMem.h>
DAG_DLL_EXPORT IMemAlloc *shared_stdmem;
DAG_DLL_EXPORT IMemAllocExtAPI *shared_stdmem_extapi;
IMemAlloc *shared_stdmem = nullptr;
IMemAllocExtAPI *shared_stdmem_extapi = nullptr;
static void export_memalloc_for_hosted_server() { shared_stdmem = defaultmem, shared_stdmem_extapi = stdmem_extapi; }
#else
static void export_memalloc_for_hosted_server() {}
#endif

ECS_REGISTER_EVENT(EventHostedInternalServerDidStart);
ECS_REGISTER_EVENT(EventHostedInternalServerDidStop);
ECS_REGISTER_EVENT(EventHostedInternalServerToStart);
ECS_REGISTER_EVENT(EventHostedInternalServerToStop);

static void(__cdecl *invoke_try_start_relay_and_subscribe)(void(__cdecl *)(bool)) = nullptr;
static const char *(__cdecl *get_local_server_connection_url)(eastl::string &) = nullptr;


static bool internal_server_did_start = false;

std::recursive_mutex server_state_lock;

class ServerLockGuard
{
public:
  explicit ServerLockGuard(bool lock_if_true)
  { // construct and lock
    lock = lock_if_true;
    if (lock)
      server_state_lock.lock();
  }

  ~ServerLockGuard() noexcept
  {
    if (lock)
      server_state_lock.unlock();
  }

private:
  bool lock;
};

#define SCOPED_STATE_LOCK()                 ServerLockGuard lock(true)
#define SCOPED_STATE_LOCK_IF_NOT(CONDITION) ServerLockGuard(!(CONDITION))

extern "C" const char *dedicated_server_dll_fn;

struct InternalDedicatedServerMainThread;
static eastl::unique_ptr<InternalDedicatedServerMainThread> current_running_internal_server;
static bool dedicated_server_dll_pdb_loaded = false;

void hosted_internal_server_init_shared_memory();
void hosted_internal_server_pass_shared_memory(DataBlock &startParams);
void hosted_internal_server_term_shared_memory();

typedef enum InternalServerState
{
  NONE,
  CREATED,
  ACTIVATING,
  RUNNING,
  TERMINATING,
  TERMINATED
} InternalServerState;
static InternalServerState current_state();

void schedule_new_internal_server_with_args(int external_argc, char **external_argv);
void schedule_new_internal_server_with_args_block(DataBlock &args);

// no need to lock scope, since the return value is useless if not taken already in a lock
bool is_hosting_api_available() { return current_state() > CREATED && current_state() < TERMINATING; }

// no need to lock scope, since the return value is useless if not taken already in a lock
bool is_ingame_api_available() { return current_state() == RUNNING; }


static void hosted_server_did_start()
{
  debug("hosted_server_did_start");
  internal_server_did_start = true;
  run_action_on_main_thread([&] { g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStart()); });
}
static void __cdecl on_internal_server_detected_memory_leak(const char *msg) { debug("%s: %s", __FUNCTION__, msg); }


static DataBlock new_server_start_params;
static bool is_new_server_scheduled() { return new_server_start_params.paramCount() > 0; }
static void remove_current_new_server_request() { new_server_start_params.reset(); }

static String get_valid_dll_fn()
{
  if (!dedicated_server_dll_fn)
  {
    logerr("client is built without hosted-internal-server support, try building with -sUseHostedInternalServer=yes");
    return String();
  }
  String dll_fn;
#if _TARGET_PC
  dll_fn.setStrCat3(dgs_argv[0], "/../", dedicated_server_dll_fn);
  simplify_fname(dll_fn);
#else
  dll_fn = df_get_real_name(dedicated_server_dll_fn);
#endif
  if (dll_fn.empty())
  {
    logerr("dll_fn is empty, with dedicated_server_dll_fn=%s", dedicated_server_dll_fn);
    return String();
  }
  if (!dd_file_exists(dll_fn))
  {
    logerr("dll_fn=%s but file doesn't exist", dll_fn);
    return String();
  }
  return dll_fn;
}

static void create_start_params(DataBlock &start_params, int argc, char **argv)
{
  start_params.reset();
  start_params.addStr("arg", get_valid_dll_fn());
  for (int i = 0; i < argc; i++)
  {
    start_params.addStr("arg", argv[i]);
    debug(argv[i]);
  }
}

static void set_server_request(DataBlock &args)
{
  remove_current_new_server_request();
  new_server_start_params = eastl::move(args);
}


struct InternalDedicatedServerMainThread final : public DaThread
{

private:
  void *dllHandle = nullptr;
  DataBlock startParams;
  GameResProxyTable gameres_proxy_table;
  bool shutdownRequested = false;
  bool isWaitingShutdown = false;
  void(__cdecl *exit_internal_server)(const char *exit_reason) = nullptr;
  bool(__cdecl *is_internal_server_terminating)() = nullptr;
  InternalServerState state = CREATED;

  void send_exit_signal_to_server()
  {
    SCOPED_STATE_LOCK();
    if (state == TERMINATED)
      return;
    debug("%s", __FUNCTION__);
    if (exit_internal_server)
    {
      debug("%s - call exit", __FUNCTION__);
      exit_internal_server("kill_internal_server");
    }
    state = TERMINATING;
    shutdownRequested = true;
  }

public:
  InternalDedicatedServerMainThread(DataBlock &args) : DaThread("InternalDedicatedServerMain", 4 << 20)
  {
    state = ACTIVATING;
    startParams = args;
    hosted_internal_server_pass_shared_memory(startParams);
    fill_platform_specific_init_values(startParams);

    fill_gameres_proxy_table(gameres_proxy_table);
    startParams.setInt64("gameResProxyTablePtr", (intptr_t)(void *)&gameres_proxy_table);
    startParams.setInt64("gameResProxyTableSz", sizeof(gameres_proxy_table));

    internal_server_did_start = false;
  }

  InternalServerState get_state()
  {
    SCOPED_STATE_LOCK();
    if (state == RUNNING && (is_internal_server_terminating() || shutdownRequested))
      state = TERMINATING;
    return state;
  }

  void kill_server(bool wait)
  {
    debug("%s", __FUNCTION__);
    SCOPED_STATE_LOCK();
    send_exit_signal_to_server();
    if (wait)
    {
      debug("%s - wait", __FUNCTION__);
      // prevents attempts to lock mutex by the thread while shutting down - otherwise terminating thread would cause deadlock
      isWaitingShutdown = true;
      DaThread::terminate(true, -1);
      current_running_internal_server.reset();
    }
  }

  void execute() override
  {
    static const char *func_label = "InternalDedicatedServerMainThread::execute";
    debug("%s: enter", func_label);
    String dllPath = get_valid_dll_fn();
    if (dllPath.empty())
    {
      state = TERMINATED;
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      current_running_internal_server.reset();
      return;
    }
    dllHandle = os_dll_load_deep_bind(dllPath);
    debug("%s: load_dll(%s = %s) -> %p", func_label, dedicated_server_dll_fn, dllPath, dllHandle);
    if (!dllHandle)
    {
      state = TERMINATED;
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      current_running_internal_server.reset();
      return;
    }

    auto start_internal_server =
      (bool(__cdecl *)(const DataBlock &, void(__cdecl *)(const char *)))os_dll_get_symbol(dllHandle, "start_internal_server");

    auto hosted_server_on_loaded = (void(__cdecl *)(void *))os_dll_get_symbol(dllHandle, "hosted_server_on_loaded");
    {
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      exit_internal_server = (void(__cdecl *)(const char *exit_reason))os_dll_get_symbol(dllHandle, "exit_game_exported");
      is_internal_server_terminating = (bool(__cdecl *)())os_dll_get_symbol(dllHandle, "dng_is_app_terminating_exported");
      invoke_try_start_relay_and_subscribe =
        (void(__cdecl *)(void(__cdecl *)(bool)))os_dll_get_symbol(dllHandle, "try_start_relay_and_subscribe");
      get_local_server_connection_url =
        (const char *(__cdecl *)(eastl::string &))os_dll_get_symbol(dllHandle, "local_server_connection_url");
    }
    debug_flush(false);
    if (start_internal_server)
    {
      debug("%s: found mandatory exports", func_label);
      if (!dedicated_server_dll_pdb_loaded)
        ::symhlp_load(dllPath);
      dedicated_server_dll_pdb_loaded = true;
      if (hosted_server_on_loaded)
        hosted_server_on_loaded((void *)(&hosted_server_did_start));
      state = RUNNING;
      if (shutdownRequested)
        send_exit_signal_to_server();
      start_internal_server(startParams, on_internal_server_detected_memory_leak);
      debug("%s: internal server main finished", func_label);
    }
    else
      logerr("%s: missing exports: %s=%p %s=%p %s=%p %s=%p %s=%p", func_label,         //
        "start_internal_server", (void *)start_internal_server,                        //
        "hosted_server_on_loaded", (void *)hosted_server_on_loaded,                    //
        "try_start_relay_and_subscribe", (void *)invoke_try_start_relay_and_subscribe, //
        "exit_game_exported", (void *)exit_internal_server,                            //
        "local_server_connection_url", (void *)get_local_server_connection_url);
    debug_flush(false);
    {
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      state = TERMINATED;
      internal_server_did_start = false;
      exit_internal_server = nullptr;
      invoke_try_start_relay_and_subscribe = nullptr;
      is_internal_server_terminating = nullptr;
      get_local_server_connection_url = nullptr;
    }

    debug("%s: unload dll=%p {%s}", func_label, dllHandle, dllPath);
    bool result = os_dll_close(dllHandle);
    debug("%s: unload dll result=%s", func_label, result ? "SUCCESS" : "FAIL");
#if DAGOR_DBGLEVEL <= 0 // skip unloading symbols in non-release build to report memory leaks properly
    ::symhlp_unload(dllPath);
    dedicated_server_dll_pdb_loaded = false;
#endif
    dllHandle = nullptr;
    SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
    if (!isWaitingShutdown)
      current_running_internal_server.reset();
    return run_action_on_main_thread([] {
      g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStop());
      if (is_new_server_scheduled())
      {
        schedule_new_internal_server_with_args_block(new_server_start_params);
      }
    });
  }
};

InternalServerState current_state()
{
  SCOPED_STATE_LOCK();
  if (!current_running_internal_server)
    return NONE;
  return current_running_internal_server->get_state();
}

void kill_internal_server(bool wait)
{
  SCOPED_STATE_LOCK();
  if (current_running_internal_server)
    current_running_internal_server->kill_server(wait);
}

static void make_sure_no_current_server_running()
{
  SCOPED_STATE_LOCK();
  debug("%s: before terminate %p", __FUNCTION__, current_running_internal_server.get());
  kill_internal_server(true);
  debug("%s: before reset", __FUNCTION__);
}


static void launch_internal_server_with_args_block(DataBlock args)
{
  SCOPED_STATE_LOCK();
  make_sure_no_current_server_running();
  export_memalloc_for_hosted_server();
  hosted_internal_server_init_shared_memory();
  remove_current_new_server_request();
  if (!get_valid_dll_fn().empty())
  {
    current_running_internal_server.reset(new InternalDedicatedServerMainThread(args));
    current_running_internal_server->start();
  }
}

static void kill_current_internal_server_and_relaunch_after(DataBlock &args)
{
  SCOPED_STATE_LOCK();
  debug("%s", __FUNCTION__);

  // first of all we clear out existing schedule if it exists
  remove_current_new_server_request();

  if (current_running_internal_server) // if there's current server schedule a new server to run after
  {
    set_server_request(args);
    kill_internal_server(false);
  }
  else
  {
    launch_internal_server_with_args_block(args);
  }
}

static bool is_internal_server_running_or_activating()
{
  SCOPED_STATE_LOCK();
  if ((current_state() > NONE && current_state() < TERMINATING) || is_new_server_scheduled())
    // if exit_internal_server is NULL we have requested exit at this point
    return true;
  return false;
}

void schedule_new_internal_server_with_args(int external_argc, char **external_argv)
{
  SCOPED_STATE_LOCK();
  debug("%s", __FUNCTION__);
  DataBlock args;
  create_start_params(args, external_argc, external_argv);
  kill_current_internal_server_and_relaunch_after(args);
}
void schedule_new_internal_server_with_args_block(DataBlock &args)
{
  SCOPED_STATE_LOCK();
  debug("%s", __FUNCTION__);
  kill_current_internal_server_and_relaunch_after(args);
}


eastl::string cached_internal_server_url;
static const char *get_internal_server_url()
{
  SCOPED_STATE_LOCK();
  if (!current_running_internal_server)
    return NULL;
  if (current_state() < RUNNING || is_new_server_scheduled())
  {
    return "-NOT-READY-";
  }

  if (current_state() > RUNNING)
    return "-TERMINATING-";
  const char *str = get_local_server_connection_url(cached_internal_server_url);
  debug("local server connection url is %s", str);
  return str;
}

static SQRESULT launch_internal_server(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Table> params_var(vm, 2);
  const Sqrat::Table &params = params_var.value;

  Sqrat::Array cmd_args = params["cmd_args"];

  int cmd_args_count = (cmd_args.GetType() == OT_ARRAY) ? cmd_args.Length() : 0;
  int argc = cmd_args_count;
  char **argv = new char *[argc];

  for (int i = 0; i < cmd_args_count; i++)
  {
    char *cmd_str = (char *)cmd_args[SQInteger(i)].GetVar<const char *>().value;
    argv[i] = str_dup(cmd_str, strmem);
  }
  schedule_new_internal_server_with_args(argc, argv);
  for (int i = 0; i < cmd_args_count; i++)
  {
    delete argv[i];
  }
  delete[] argv;
  return SQ_OK;
}

static void kill_internal_server_from_script() { kill_internal_server(false); }

void __cdecl on_relay_try_start_result(bool success)
{
  debug("client: receive confirmation of relay %s", success ? "succesfully established" : "failed to be established");
}


static void manual_start_relay()
{
  SCOPED_STATE_LOCK();
  debug("manual_start_relay");
  if (invoke_try_start_relay_and_subscribe)
    invoke_try_start_relay_and_subscribe(on_relay_try_start_result);
}


void bind_hosting_internal_server(Sqrat::Table &ns)
{
  ns.SquirrelFunc("launch_internal_server", launch_internal_server, 2, ".t");
  ns.Func("kill_internal_server", kill_internal_server_from_script);
  ns.Func("is_internal_server_running_or_activating", is_internal_server_running_or_activating);
  ns.Func("get_internal_server_url", get_internal_server_url);
  ns.Func("manual_relay_start", manual_start_relay);
}


void shutdown_internal_server_on_host_exit()
{
  SCOPED_STATE_LOCK();
  remove_current_new_server_request();
  make_sure_no_current_server_running();
  hosted_internal_server_term_shared_memory();
}


static bool debug_host_mode_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("host_mode", "launch_internal_server", 0, 40)
  {
    for (int i = 0; i < argc; i++)
      debug(argv[i]);
    schedule_new_internal_server_with_args(argc - 1, (char **)(&argv[1]));
  }

  CONSOLE_CHECK_NAME("host_mode", "kill_internal_server", 0, 0) { kill_internal_server(false); }
  return found;
}

REGISTER_CONSOLE_HANDLER(debug_host_mode_console_handler);

void prelaunch_internal_server_if_needed()
{
  if (!dgs_get_argv("prelaunch_host_arg"))
    return;
  eastl::vector<char *> cmdsCopy(0);
  int it = 1;
  while (const char *ap = ::dgs_get_argv("prelaunch_host_arg", it))
  {
    cmdsCopy.push_back((char *)ap);
    debug("%s: arg %s", __FUNCTION__, ap);
  }
  char **argv = (char **)cmdsCopy.data();
  int argc = cmdsCopy.size();
  debug("%s: starting with %d args", __FUNCTION__, argc);
  schedule_new_internal_server_with_args(argc, argv);
}