// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/hostedServerLauncher.h"
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <atomic>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/event.h>
#include <debug/dag_debug.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <mutex>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_cpuFreq.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <sqstdblob.h>
#include <startup/dag_globalSettings.h>
#include <stdlib.h>
#include <string.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#if _TARGET_C2

#else
static void fill_platform_specific_init_values(DataBlock &) {}
#endif
#include <gameRes/dag_gameResProxyTable.h>

#if _TARGET_PC | _TARGET_XBOX
#include <memory/dag_dbgMem.h>
#include <supp/dag_dllexport.h>
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

static void(__cdecl *invoke_try_start_relay)() = nullptr;
static const char *(__cdecl *get_local_server_connection_url)(eastl::string &) = nullptr;

void set_mirror_hosted_dedic_logs(bool on)
{
  const_cast<DataBlock *>(::dgs_get_settings())->addBlock("debug")->setBool("mirrorHostedDedicLogs", on);
}

bool get_mirror_hosted_dedic_logs()
{
  return ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("mirrorHostedDedicLogs", false);
}

void stamp_test_log_uid_if_present()
{
  if (const char *uid = ::dgs_get_argv(TEST_LOG_UID_ARG))
    debug("%s%s", TEST_LOG_UID_STAMP_PREFIX, uid);
}

bool is_host_identity_uid_argv(const char *arg)
{
  if (!arg || arg[0] != '-')
    return false;
  auto match_name = [](const char *a, const char *name) {
    const size_t n = strlen(name);
    if (strncmp(a + 1, name, n) != 0)
      return false;
    const char d = a[1 + n];
    return d == ':' || d == '=' || d == '\0';
  };
  return match_name(arg, TEST_LOG_UID_ARG) || match_name(arg, TEST_SERVER_LOG_UID_ARG);
}

void append_relayed_test_log_uid_arg(DataBlock &start_params)
{
  if (const char *sid = ::dgs_get_argv(TEST_SERVER_LOG_UID_ARG))
  {
    String a(0, "%s%s", TEST_LOG_UID_STAMP_PREFIX, sid);
    start_params.addStr("arg", a.str());
  }
}

static void dll_set_log_forwarder(void *dll, void *cb, bool dll_side_override_expect_later)
{
  using Fn = void(__cdecl *)(void *, bool);
  if (Fn fn = (Fn)os_dll_get_symbol(dll, "hosted_server_set_log_forwarder"))
    fn(cb, dll_side_override_expect_later);
}

static void dll_set_work_status_callback(void *dll, void *cb)
{
  using Fn = void(__cdecl *)(void *);
  if (Fn fn = (Fn)os_dll_get_symbol(dll, "hosted_server_set_work_status_callback"))
    fn(cb);
}

static std::atomic<void *> hosted_log_mirror_override{nullptr};
static std::atomic<bool> hosted_log_mirror_dll_override{false};

void __cdecl default_hosted_server_log_forwarder(int level, const char *message, const char *filename, int code_line)
{
  G_UNUSED(filename);
  G_UNUSED(code_line);
  if (!message)
    return;
  String line;
  format_hosted_dedic_log_line(line, message);
  logmessage(level, "%s", line.c_str());
}

void set_hosted_log_mirror(void *fn, bool dll_side_override_expect_later)
{
  hosted_log_mirror_dll_override.store(fn ? dll_side_override_expect_later : false, std::memory_order_release);
  hosted_log_mirror_override.store(fn, std::memory_order_release);
}

void *get_hosted_log_mirror()
{
  if (void *fn = hosted_log_mirror_override.load(std::memory_order_acquire))
    return fn;
  return (void *)&default_hosted_server_log_forwarder;
}

bool hosted_log_mirror_expects_dll_override() { return hosted_log_mirror_dll_override.load(std::memory_order_acquire); }

// Dedic/DLL symbols provided by dedicated_dll__exports.cpp when HIS.
void hosted_server_arm_log_mirror_callback() {}

static bool internal_server_did_start = false;

static constexpr int HOSTED_READY_TIMEOUT_DEFAULT_MS = 2 * 60 * 1000;
static int get_hosted_ready_timeout_ms()
{
  int t = dgs_get_settings()->getBlockByNameEx("debug")->getInt("internalServerLaunchTimeout", HOSTED_READY_TIMEOUT_DEFAULT_MS);
  return t > 0 ? t : HOSTED_READY_TIMEOUT_DEFAULT_MS;
}

std::recursive_mutex server_state_lock;

static volatile int hosted_ready_watchdog_armed = 0;

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

// no need to lock scope, since the return value is useless if not taken already
// in a lock
bool is_hosting_api_available() { return current_state() > CREATED && current_state() < TERMINATING; }

// no need to lock scope, since the return value is useless if not taken already
// in a lock
bool is_ingame_api_available() { return current_state() == RUNNING; }

static void hosted_server_did_start();
static void __cdecl on_internal_server_detected_memory_leak(const char *msg) { debug("%s: %s", __FUNCTION__, msg); }

static DataBlock new_server_start_params;
static bool is_new_server_scheduled() { return new_server_start_params.paramCount() > 0; }
static void remove_current_new_server_request() { new_server_start_params.reset(); }

static bool hosted_server_start_pending = false;
static HostedServerUid hosted_server_pending_uid = HOSTED_SERVER_UID_NONE;
static HostedServerUid hosted_server_preset_start_uid = HOSTED_SERVER_UID_NONE;
static std::atomic<uint64_t> hosted_server_uid_seq{1};

static bool parse_hosted_server_uid(const char *val, HostedServerUid &out)
{
  if (!val || !*val)
    return false;
  char *end = nullptr;
  const unsigned long long v = strtoull(val, &end, 10);
  if (*end || v == 0)
    return false;
  out = (HostedServerUid)v;
  return true;
}

static bool extract_test_log_uid_value(const char *arg, HostedServerUid &out)
{
  if (!arg || arg[0] != '-')
    return false;
  auto match_name = [](const char *a, const char *name) -> const char * {
    const size_t n = strlen(name);
    if (strncmp(a + 1, name, n) != 0)
      return nullptr;
    const char d = a[1 + n];
    if (d != ':' && d != '=')
      return nullptr;
    return a + 2 + n;
  };
  const char *val = match_name(arg, TEST_SERVER_LOG_UID_ARG);
  if (!val)
    val = match_name(arg, TEST_LOG_UID_ARG);
  return parse_hosted_server_uid(val, out);
}

static HostedServerUid allocate_hosted_server_uid_unlocked()
{
  return (HostedServerUid)hosted_server_uid_seq.fetch_add(1, std::memory_order_relaxed);
}

// Keep the monotonic sequence above any caller-supplied id so the next allocate cannot collide.
// Callers hold server_state_lock.
static void reserve_hosted_server_uid_unlocked(HostedServerUid uid)
{
  if (uid == HOSTED_SERVER_UID_NONE)
    return;
  const uint64_t next = (uint64_t)uid + 1;
  if (hosted_server_uid_seq.load(std::memory_order_relaxed) < next)
    hosted_server_uid_seq.store(next, std::memory_order_relaxed);
}

HostedServerUid allocate_hosted_server_uid()
{
  SCOPED_STATE_LOCK();
  return allocate_hosted_server_uid_unlocked();
}

void set_hosted_server_start_uid(HostedServerUid uid)
{
  SCOPED_STATE_LOCK();
  hosted_server_preset_start_uid = uid;
  reserve_hosted_server_uid_unlocked(uid);
}

HostedServerUid resolve_hosted_server_start_uid(HostedServerUid passed)
{
  SCOPED_STATE_LOCK();
  if (passed != HOSTED_SERVER_UID_NONE)
  {
    hosted_server_preset_start_uid = HOSTED_SERVER_UID_NONE;
    reserve_hosted_server_uid_unlocked(passed);
    return passed;
  }
  if (hosted_server_preset_start_uid != HOSTED_SERVER_UID_NONE)
  {
    const HostedServerUid uid = hosted_server_preset_start_uid;
    hosted_server_preset_start_uid = HOSTED_SERVER_UID_NONE;
    reserve_hosted_server_uid_unlocked(uid);
    return uid;
  }
  return allocate_hosted_server_uid_unlocked();
}

static String get_valid_dll_fn()
{
  if (!dedicated_server_dll_fn)
  {
    logerr("client is built without hosted-internal-server support, try "
           "building with -sUseHostedInternalServer=yes");
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
  HostedServerUid hisUid = HOSTED_SERVER_UID_NONE;
  String relayedStamp;
  for (int i = 0; i < argc; i++)
  {
    // Host identity args stay on the host; the child gets -test_log_uid only.
    if (is_host_identity_uid_argv(argv[i]))
    {
      if (extract_test_log_uid_value(argv[i], hisUid))
        relayedStamp.printf(0, "%s%llu", TEST_LOG_UID_STAMP_PREFIX, (unsigned long long)hisUid);
      continue;
    }
    start_params.addStr("arg", argv[i]);
    debug("%s", argv[i]);
  }
  if (!relayedStamp.empty())
    start_params.addStr("arg", relayedStamp.str());
  else
    append_relayed_test_log_uid_arg(start_params);
  if (hisUid != HOSTED_SERVER_UID_NONE)
    start_params.setInt64(TEST_LOG_UID_ARG, (int64_t)hisUid);
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
  int hostedReadyDeadlineMs = 0;
  HostedServerUid hisUid = HOSTED_SERVER_UID_NONE;

  void setHostedReadyDeadline(int deadline_ms)
  {
    hostedReadyDeadlineMs = deadline_ms;
    interlocked_relaxed_store(hosted_ready_watchdog_armed, deadline_ms != 0 ? 1 : 0);
  }

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
    setHostedReadyDeadline(0);
  }

public:
  void disarm_hosted_ready_watchdog()
  {
    SCOPED_STATE_LOCK();
    setHostedReadyDeadline(0);
  }

  bool consume_hosted_ready_watchdog_expiry(int now_ms)
  {
    SCOPED_STATE_LOCK();
    if (hostedReadyDeadlineMs == 0 || now_ms < hostedReadyDeadlineMs)
      return false;
    setHostedReadyDeadline(0);
    return true;
  }

  InternalDedicatedServerMainThread(DataBlock &args) : DaThread("InternalDedicatedServerMain", 4 << 20)
  {
    state = ACTIVATING;
    startParams = args;
    hosted_internal_server_pass_shared_memory(startParams);
    fill_platform_specific_init_values(startParams);

    fill_gameres_proxy_table(gameres_proxy_table);
    startParams.setInt64("gameResProxyTablePtr", (intptr_t)(void *)&gameres_proxy_table);
    startParams.setInt64("gameResProxyTableSz", sizeof(gameres_proxy_table));

    hisUid = (HostedServerUid)startParams.getInt64(TEST_LOG_UID_ARG, 0);
    internal_server_did_start = false;
  }

  HostedServerUid query_uid() const
  {
    // Host stamped this at launch; do not call into the dedic DLL.
    return hisUid;
  }

  InternalServerState get_state()
  {
    SCOPED_STATE_LOCK();
    if (state == RUNNING && (shutdownRequested || (is_internal_server_terminating && is_internal_server_terminating())))
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
      // prevents attempts to lock mutex by the thread while shutting down -
      // otherwise terminating thread would cause deadlock
      isWaitingShutdown = true;
      DaThread::terminate(true, -1);
      current_running_internal_server.reset();
    }
  }

  void execute() override
  {
    static const char *func_label = "InternalDedicatedServerMainThread::execute";
    debug("[LIFECYCLE] %s: enter (launcher thread)", func_label);
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
    dll_set_work_status_callback(dllHandle, (void *)&default_hosted_work_status_callback);

    auto start_internal_server =
      (bool(__cdecl *)(const DataBlock &, void(__cdecl *)(const char *)))os_dll_get_symbol(dllHandle, "start_internal_server");

    auto hosted_server_on_loaded = (void(__cdecl *)(void *))os_dll_get_symbol(dllHandle, "hosted_server_on_loaded");
    {
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      exit_internal_server = (void(__cdecl *)(const char *exit_reason))os_dll_get_symbol(dllHandle, "exit_game_exported");
      is_internal_server_terminating = (bool(__cdecl *)())os_dll_get_symbol(dllHandle, "dng_is_app_terminating_exported");
      invoke_try_start_relay = (void(__cdecl *)())os_dll_get_symbol(dllHandle, "try_start_relay");
      get_local_server_connection_url =
        (const char *(__cdecl *)(eastl::string &))os_dll_get_symbol(dllHandle, "local_server_connection_url");
    }
    // Optional: re-emit dedic logs on the host. Default: DLL arms after
    // visual_err_log_setup. Eden override (expect_later): DLL skips default
    // arm; Eden dedic forwards from its on_log.
    if (get_mirror_hosted_dedic_logs())
      dll_set_log_forwarder(dllHandle, get_hosted_log_mirror(), hosted_log_mirror_expects_dll_override());
    debug_flush(false);
    if (start_internal_server)
    {
      debug("%s: found mandatory exports", func_label);
      if (!dedicated_server_dll_pdb_loaded)
        ::symhlp_load(dllPath);
      dedicated_server_dll_pdb_loaded = true;
      if (hosted_server_on_loaded)
        hosted_server_on_loaded((void *)(&hosted_server_did_start));
      {
        SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
        {
          const int timeoutMs = get_hosted_ready_timeout_ms();
          setHostedReadyDeadline(get_time_msec() + timeoutMs);
          debug("%s: hosted-ready watchdog armed, deadline in %d ms", func_label, timeoutMs);
        }
      }
      state = RUNNING;
      if (shutdownRequested)
        send_exit_signal_to_server();
      debug("[LIFECYCLE] %s: calling start_internal_server (blocks until "
            "server shuts down)",
        func_label);
      start_internal_server(startParams, on_internal_server_detected_memory_leak);
      debug("[LIFECYCLE] %s: internal server main finished, returned from DLL "
            "entry",
        func_label);
    }
    else
      logerr("%s: missing exports: %s=%p %s=%p %s=%p %s=%p %s=%p",
        func_label,                                                 //
        "start_internal_server", (void *)start_internal_server,     //
        "hosted_server_on_loaded", (void *)hosted_server_on_loaded, //
        "try_start_relay", (void *)invoke_try_start_relay,          //
        "exit_game_exported", (void *)exit_internal_server,         //
        "local_server_connection_url", (void *)get_local_server_connection_url);
    // Start used debug_flush(false) so HIS mirror spam does not fsync every
    // line. Leave it off after stop and "hosted internal server stopped" stays
    // in the host buffer; log-tail tests sit until the next fill or process
    // exit.
    debug_flush(true);
    {
      SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
      state = TERMINATED;
      internal_server_did_start = false;
      setHostedReadyDeadline(0);
      exit_internal_server = nullptr;
      invoke_try_start_relay = nullptr;
      is_internal_server_terminating = nullptr;
      get_local_server_connection_url = nullptr;
    }

    debug("%s: unload dll=%p {%s}", func_label, dllHandle, dllPath);
    // Clear dedic forwarder before unmap so a late dedic log cannot call into
    // the host after close.
    dll_set_log_forwarder(dllHandle, nullptr, false);
    dll_set_work_status_callback(dllHandle, nullptr);
    bool result = os_dll_close(dllHandle);
    debug("%s: unload dll result=%s", func_label, result ? "SUCCESS" : "FAIL");
#if DAGOR_DBGLEVEL <= 0 // skip unloading symbols in non-release build to report
                        // memory leaks properly
    ::symhlp_unload(dllPath);
    dedicated_server_dll_pdb_loaded = false;
#endif
    dllHandle = nullptr;
    // Capture before reset: DidStop is queued and must name this instance, not
    // a rehost.
    const HostedServerUid stoppedUid = hisUid;
    SCOPED_STATE_LOCK_IF_NOT(isWaitingShutdown);
    if (!isWaitingShutdown)
      current_running_internal_server.reset();
    return run_action_on_main_thread([stoppedUid] {
      g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStop{stoppedUid});
      debug_flush(true);
      if (is_new_server_scheduled())
      {
        schedule_new_internal_server_with_args_block(new_server_start_params);
      }
    });
  }
};

static void hosted_server_did_start()
{
  debug("[LIFECYCLE] hosted_server_did_start: invoked by dedic DLL "
        "(engine-owned ready ES)");
  HostedServerUid startedUid = HOSTED_SERVER_UID_NONE;
  {
    SCOPED_STATE_LOCK();
    if (!current_running_internal_server)
    {
      debug("[LIFECYCLE] hosted_server_did_start: no active server instance, "
            "discarding late hosted-ready signal");
      return;
    }
    const InternalServerState st = current_running_internal_server->get_state();
    if (st >= TERMINATING)
    {
      debug("[LIFECYCLE] hosted_server_did_start: server already in state=%d "
            "(terminating/terminated), discarding late "
            "hosted-ready signal",
        (int)st);
      return;
    }
    startedUid = current_running_internal_server->query_uid();
    internal_server_did_start = true;
    current_running_internal_server->disarm_hosted_ready_watchdog();
    debug("[LIFECYCLE] hosted_server_did_start: hosted-ready watchdog disarmed");
  }
  run_action_on_main_thread([startedUid] {
    debug("[LIFECYCLE] hosted_server_did_start: broadcasting "
          "EventHostedInternalServerDidStart uid=%llu on main thread",
      (unsigned long long)startedUid);
    g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStart{startedUid});
  });
}

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

void kill_internal_server_uid(HostedServerUid uid, bool wait)
{
  SCOPED_STATE_LOCK();
  if (uid == HOSTED_SERVER_UID_NONE)
    return;
  if (hosted_server_start_pending && hosted_server_pending_uid == uid)
  {
    hosted_server_start_pending = false;
    hosted_server_pending_uid = HOSTED_SERVER_UID_NONE;
  }
  if (is_new_server_scheduled())
  {
    if ((HostedServerUid)new_server_start_params.getInt64(TEST_LOG_UID_ARG, 0) == uid)
      remove_current_new_server_request();
  }
  if (!current_running_internal_server)
    return;
  // Already exiting/exited: exit export may be cleared while the thread still
  // owns current_running until unload finishes (scene reload races ToStop).
  if (current_running_internal_server->get_state() >= TERMINATING)
  {
    debug("[LIFECYCLE] kill_internal_server_uid: already stopping uid=%llu", (unsigned long long)uid);
    return;
  }
  const HostedServerUid current = current_running_internal_server->query_uid();
  if (current != uid)
  {
    debug("[LIFECYCLE] kill_internal_server_uid: skip uid=%llu current=%llu", (unsigned long long)uid, (unsigned long long)current);
    return;
  }
  current_running_internal_server->kill_server(wait);
}

HostedServerUid get_hosted_internal_server_uid()
{
  SCOPED_STATE_LOCK();
  // Pending/scheduled win over a still-current dying HIS so rehost callers
  // (ownership checks, end_lobby_session) see the claimed uid, not the old one.
  if (hosted_server_start_pending)
    return hosted_server_pending_uid;
  if (is_new_server_scheduled())
    return (HostedServerUid)new_server_start_params.getInt64(TEST_LOG_UID_ARG, 0);
  if (current_running_internal_server)
    return current_running_internal_server->query_uid();
  return HOSTED_SERVER_UID_NONE;
}

void cancel_scheduled_internal_server_start()
{
  SCOPED_STATE_LOCK();
  if (is_new_server_scheduled())
  {
    debug("[LIFECYCLE] cancel pending relaunch");
    remove_current_new_server_request();
  }
}

bool is_hosted_internal_server_active()
{
  SCOPED_STATE_LOCK();
  return (current_state() > NONE && current_state() < TERMINATED) || is_new_server_scheduled() || hosted_server_start_pending;
}

bool try_begin_hosted_server_start(HostedServerUid uid)
{
  SCOPED_STATE_LOCK();
  // A live or dying HIS is fine: EventHostedInternalServerToStart -> schedule
  // kills and relaunches. Only refuse a second start while one is already
  // pending.
  if (hosted_server_start_pending)
  {
    debug("[LIFECYCLE] try_begin_hosted_server_start: busy pending uid=%llu "
          "(asked=%llu)",
      (unsigned long long)hosted_server_pending_uid, (unsigned long long)uid);
    return false;
  }
  hosted_server_start_pending = true;
  hosted_server_pending_uid = uid;
  debug("[LIFECYCLE] try_begin_hosted_server_start uid=%llu", (unsigned long long)hosted_server_pending_uid);
  return true;
}

void clear_hosted_server_start_pending()
{
  SCOPED_STATE_LOCK();
  hosted_server_start_pending = false;
  hosted_server_pending_uid = HOSTED_SERVER_UID_NONE;
}

bool is_hosted_server_start_pending()
{
  SCOPED_STATE_LOCK();
  return hosted_server_start_pending;
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

  DataBlock argsCopy(args);

  // first of all we clear out existing schedule if it exists
  remove_current_new_server_request();

  if (current_running_internal_server) // if there's current server schedule a
                                       // new server to run after
  {
    set_server_request(argsCopy);
    kill_internal_server(false);
  }
  else
  {
    launch_internal_server_with_args_block(argsCopy);
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
  if (current_state() < RUNNING || is_new_server_scheduled() || !internal_server_did_start)
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

static void manual_start_relay()
{
  SCOPED_STATE_LOCK();
  debug("manual_start_relay");
  if (invoke_try_start_relay)
    invoke_try_start_relay();
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
      debug("%s", argv[i]);
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

bool poll_hosted_ready_watchdog()
{
  if (!interlocked_relaxed_load(hosted_ready_watchdog_armed))
    return false;
  SCOPED_STATE_LOCK();
  if (!current_running_internal_server)
    return false;
  return current_running_internal_server->consume_hosted_ready_watchdog_expiry(get_time_msec());
}

void hosted_internal_server_management_update()
{
  if (poll_hosted_ready_watchdog())
  {
    logwarn("hosted_server_ready_watchdog: server did not report ready in time "
            "-- killing internal server");
    kill_internal_server(/*wait*/ false);
  }
}
