// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include "net/dedicated/matching_state_data.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/scene/scene.h>
#include <supp/dag_dllexport.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <atomic>

static std::atomic<void (*)()> on_server_loaded_callback{nullptr};
static std::atomic<bool> auto_fire_server_ready{true};

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

extern "C" void hosted_server_signal_ready() { fire_server_loaded_once(); }

extern "C" void hosted_server_disable_auto_ready() { auto_fire_server_ready.store(false, std::memory_order_release); }

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
