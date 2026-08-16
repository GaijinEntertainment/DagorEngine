// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include "net/dedicated/matching_state_data.h"
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
#include <memory/dag_framemem.h>
#include <atomic>

static std::atomic<void (*)()> on_server_loaded_callback{nullptr};
static std::atomic<bool> auto_fire_server_ready{true};

typedef void(__cdecl *hosted_server_log_forwarder_t)(int level, const char *message, const char *filename, int code_line);
static std::atomic<hosted_server_log_forwarder_t> log_forwarder{nullptr};
static debug_log_callback_t prev_hosted_debug_log_cb = nullptr;
static bool hosted_debug_log_installed = false;

static int hosted_server_debug_log_cb(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (hosted_server_log_forwarder_t fwd = log_forwarder.load(std::memory_order_acquire))
  {
    // Host sink owns console display for the HIS lifetime; do not chain to the
    // previous callback (would double-emit ERR into logsBuff via console_output_listener).
    String buf(framemem_ptr());
    buf.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);
    fwd(lev_tag, buf.c_str(), ctx_file ? ctx_file : "", ctx_line);
    return 1;
  }
  if (prev_hosted_debug_log_cb)
    return prev_hosted_debug_log_cb(lev_tag, fmt, arg, anum, ctx_file, ctx_line);
  return 1;
}

void hosted_server_install_debug_log_forward()
{
  if (hosted_debug_log_installed)
    return;
  hosted_debug_log_installed = true;
  prev_hosted_debug_log_cb = debug_set_log_callback(hosted_server_debug_log_cb);
}

void hosted_server_uninstall_debug_log_forward()
{
  if (!hosted_debug_log_installed)
    return;
  hosted_debug_log_installed = false;
  debug_set_log_callback(prev_hosted_debug_log_cb);
  prev_hosted_debug_log_cb = nullptr;
}

DAG_DLL_EXPORT void hosted_server_set_log_forwarder(hosted_server_log_forwarder_t cb)
{
  log_forwarder.store(cb, std::memory_order_release);
}

DAG_DLL_EXPORT hosted_server_log_forwarder_t hosted_server_get_log_forwarder()
{
  return log_forwarder.load(std::memory_order_acquire);
}

static void fire_server_loaded_once()
{
  if (void (*cb)() = on_server_loaded_callback.exchange(nullptr, std::memory_order_acq_rel))
    cb();
}

DAG_DLL_EXPORT
const char *local_server_connection_url(eastl::string &str)
{
  int tmpIt = 1;
  return dedicated::get_host_url(str, tmpIt);
}

DAG_DLL_EXPORT
void hosted_server_on_loaded(void *callback) { on_server_loaded_callback.store((void (*)())callback, std::memory_order_release); }

void hosted_server_signal_ready() { fire_server_loaded_once(); }

void hosted_server_disable_auto_ready() { auto_fire_server_ready.store(false, std::memory_order_release); }

DAG_DLL_EXPORT
void try_start_relay_and_subscribe(void(__cdecl *relay_status_subscribe)(bool enabled))
{
  if (dedicated_matching::state_data::try_start_relay_and_subscribe)
    dedicated_matching::state_data::try_start_relay_and_subscribe(relay_status_subscribe);
  else if (relay_status_subscribe)
  {
    logwarn("%s not set", __FUNCTION__);
    relay_status_subscribe(false);
  }
}

static void hosted_server_tracking_loaded_local_scene_entities_es(const ecs::Event &__restrict, const ecs::QueryView &__restrict)
{
  debug("hosted_server_tracking_loaded_local_scene_entities: on_server_loaded_callback=%p, auto_fire=%d",
    (void *)on_server_loaded_callback.load(std::memory_order_acquire), (int)auto_fire_server_ready.load(std::memory_order_acquire));
  if (!auto_fire_server_ready.load(std::memory_order_acquire))
    return;
  if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("hostedServerManualReady", false))
    return;
  fire_server_loaded_once();
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
