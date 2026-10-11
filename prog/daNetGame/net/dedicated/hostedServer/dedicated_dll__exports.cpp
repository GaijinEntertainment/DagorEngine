// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include "net/dedicated/matching_state_data.h"
#include "net/netEvents.h"
#include "net/net.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/scene/scene.h>
#include <supp/dag_dllexport.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <debug/dag_log.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <memory/dag_memBase.h>
#include <atomic>
#include <EASTL/string.h>
#include "main/hostedServerLauncher.h"
#include "main/workStatus.h"

static std::atomic<void (*)()> on_server_loaded_callback{nullptr};
// DNG default: scene entities alone. Eden sets Scripts (+ Entities for main-thread net).
static std::atomic<uint32_t> hosted_ready_required{uint32_t(HostedReadyFlags::Entities)};
static std::atomic<uint32_t> hosted_ready_got{0};

static std::atomic<void *> log_forwarder{nullptr};
static std::atomic<bool> expect_dll_override{false};
static eastl::string dll_his_uid;

void hosted_server_store_instance_uid(const char *uid) { dll_his_uid = uid ? uid : ""; }

DAG_DLL_EXPORT const char *hosted_server_get_uid()
{
  if (const char *s = ::dgs_get_argv(TEST_LOG_UID_ARG))
    return s;
  return dll_his_uid.c_str();
}

// Same pattern as hosted_server_set_log_forwarder: Scope in HIS calls this host callback.
DAG_DLL_EXPORT void hosted_server_set_work_status_callback(void *cb) { workstatus::set_callback((workstatus::Callback)cb); }
static debug_log_callback_t prev_log_callback = nullptr;
static std::atomic<bool> log_callback_armed{false};

void *hosted_server_get_log_forwarder() { return log_forwarder.load(std::memory_order_acquire); }

// Default HIS path: chain prev, then host forwarder (default signature, no Eden).
static int hosted_server_log_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  const int r = prev_log_callback ? prev_log_callback(lev_tag, fmt, arg, anum, ctx_file, ctx_line) : 1;
  if (r > 0)
  {
    // Logging can arrive from threads without framemem; strmem is always safe.
    String buf(strmem);
    format_hosted_log_message(buf, fmt, arg, anum);
    if (HostedServerLogForwarder fwd = (HostedServerLogForwarder)log_forwarder.load(std::memory_order_acquire))
      fwd(lev_tag, buf.c_str(), ctx_file, ctx_line);
  }
  return r;
}

static void arm_default_log_callback()
{
  if (log_callback_armed.exchange(true, std::memory_order_acq_rel))
    return;
  prev_log_callback = debug_set_log_callback(&hosted_server_log_callback);
}

static void disarm_log_callback()
{
  if (!log_callback_armed.exchange(false, std::memory_order_acq_rel))
    return;
  debug_set_log_callback(prev_log_callback);
  prev_log_callback = nullptr;
}

void hosted_server_arm_log_mirror_callback()
{
  if (!log_forwarder.load(std::memory_order_acquire))
    return;
  // Eden (expect_later): skip default arm; dedic Eden forwards from its own on_log.
  if (expect_dll_override.load(std::memory_order_acquire))
    return;
  arm_default_log_callback();
}

DAG_DLL_EXPORT void hosted_server_set_log_forwarder(void *cb, bool dll_side_override_expect_later)
{
  // Host must clear (nullptr) before os_dll_close so the callback cannot outlive the DLL.
  // Store only; default arm is hosted_server_arm_log_mirror_callback after visual_err_log_setup.
  if (cb)
  {
    expect_dll_override.store(dll_side_override_expect_later, std::memory_order_release);
    log_forwarder.store(cb, std::memory_order_release);
  }
  else
  {
    disarm_log_callback();
    log_forwarder.store(nullptr, std::memory_order_release);
    expect_dll_override.store(false, std::memory_order_release);
  }
}


static void fire_server_loaded_once()
{
  if (void (*cb)() = on_server_loaded_callback.exchange(nullptr, std::memory_order_acq_rel))
    cb();
}

static void hosted_ready_try_fire()
{
  const uint32_t req = hosted_ready_required.load(std::memory_order_acquire);
  const uint32_t got = hosted_ready_got.load(std::memory_order_acquire);
  if (req == 0 || (got & req) != req)
    return;
  debug("hosted_server: ready latch satisfied got=0x%x required=0x%x (callback=%p)", got, req,
    (void *)on_server_loaded_callback.load(std::memory_order_acquire));
  fire_server_loaded_once();
}

void hosted_server_set_ready_required(HostedReadyFlags mask)
{
  hosted_ready_required.store(uint32_t(mask), std::memory_order_release);
  hosted_ready_try_fire();
}

void hosted_server_signal_ready(HostedReadyFlags flags)
{
  hosted_ready_got.fetch_or(uint32_t(flags), std::memory_order_acq_rel);
  hosted_ready_try_fire();
}

DAG_DLL_EXPORT
const char *local_server_connection_url(eastl::string &str)
{
  int tmpIt = 1;
  return dedicated::get_host_url(str, tmpIt);
}

DAG_DLL_EXPORT
void hosted_server_on_loaded(void *callback)
{
  hosted_ready_got.store(0, std::memory_order_release);
  on_server_loaded_callback.store((void (*)())callback, std::memory_order_release);
  hosted_ready_try_fire();
}

DAG_DLL_EXPORT
void try_start_relay()
{
  if (dedicated_matching::state_data::try_start_relay)
    dedicated_matching::state_data::try_start_relay();
  else
    logwarn("%s not set", __FUNCTION__);
}

static void hosted_server_tracking_loaded_local_scene_entities_es(const ecs::Event &__restrict, const ecs::QueryView &__restrict)
{
  debug("hosted_server: signal Entities (EventOnLocalSceneEntitiesCreated)");
  hosted_server_signal_ready(HostedReadyFlags::Entities);
}
static ecs::EntitySystemDesc hosted_server_tracking_loaded_local_scene_entities_es_es_desc(
  "hosted_server_tracking_loaded_local_scene_entities_es",
  "prog/daNetGame/net/dedicated/dedicated_dll__exports.inc.cpp",
  ecs::EntitySystemOps(nullptr, hosted_server_tracking_loaded_local_scene_entities_es),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventOnLocalSceneEntitiesCreated>::build(),
  0);

static void hosted_server_ready_on_scripts_initialized_es(const ecs::Event &__restrict, const ecs::QueryView &__restrict)
{
  debug("hosted_server: signal Scripts (OnEdenScriptsInitialized)");
  hosted_server_signal_ready(HostedReadyFlags::Scripts);
}
static ecs::EntitySystemDesc hosted_server_ready_on_scripts_initialized_es_es_desc("hosted_server_ready_on_scripts_initialized_es",
  "prog/daNetGame/net/dedicated/dedicated_dll__exports.inc.cpp",
  ecs::EntitySystemOps(nullptr, hosted_server_ready_on_scripts_initialized_es),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnEdenScriptsInitialized>::build(),
  0);
