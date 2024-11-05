// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <lagCatcher/lagCatcher.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_logSys.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/fixed_string.h>
#include <stdlib.h>
#include <daGame/timers.h>
#include <util/dag_watchdog.h>
#include <util/dag_delayedAction.h>
#include <util/dag_finally.h>
#include "main/level.h"
#include "game/player.h"


static int lagcatcher_deadline_ms = 0;

static void lagcatcher_level_loaded_es_event_handler(const EventLevelLoaded &)
{
  Finally disableLCUpdate([] { delayed_call([] { g_entity_mgr->enableES("lagcatcher_update_es", false); }); });
  const char *lcchstr = dgs_get_argv("lagcatcher");
  if (!lcchstr)
    return;
#ifdef __SANITIZE_ADDRESS__ // backtrace() that used by lagcatcher is know to be unreliable in ASAN
  if (!*lcchstr || *lcchstr != '2')
  {
    logwarn(
      "Refuse to enable lagcatcher by default on ASAN (it's known to be unreliable). Specify '-lagcatcher 2' in order to force it");
    return;
  }
  else
#endif
    if (*lcchstr && *lcchstr != '1') // both empty str & 1 - enabled (otherwise it treated as random chance to enable it)
  {
    float urnd = (hash_int(get_time_msec()) & ((1 << 24) - 1)) / float(1 << 24);
    char *_1;
    if (urnd > eastl::min(strtof(lcchstr, &_1), 1.f))
      return;
  }
  const DataBlock &cfg = *dgs_get_settings()->getBlockByNameEx("lagcatcher");
  int default_deadline_ms = 100;
  int deadline = cfg.getInt("deadline_ms", default_deadline_ms);
  int sampling_interval_us = cfg.getInt("sampling_interval_us", 1000);
  if (lagcatcher::init(sampling_interval_us, cfg.getInt("mem_budget_kb", 256) << 10))
    debug("lagcatcher inited with deadline_ms=%d sampling_interval_us=%d", deadline, sampling_interval_us);
  else
  {
    logerr("lagcatcher init failed");
    return;
  }
  game::Timer th;
  game::g_timers_mgr->setTimer(
    th,
    [=] {
      lagcatcher_deadline_ms = deadline;
      debug("lagcatcher started");
    },
    dgs_get_settings()->getBlockByNameEx("lagcatcher")->getReal("initial_skip_time_sec", 60.f));
  th.release(); // one shot
  disableLCUpdate.release();
}

ECS_NO_ORDER
inline void lagcatcher_update_es(const ecs::UpdateStageInfoAct &)
{
  if (!lagcatcher_deadline_ms)
    return;
  if (game::get_num_connected_players())
    lagcatcher::start(lagcatcher_deadline_ms);
  else
    lagcatcher::stop();
}

static void lagcatcher_stop_on_clear_es(const ecs::EventEntityManagerBeforeClear &)
{
  if (lagcatcher_deadline_ms)
    lagcatcher::stop();
}

static void lagcatcher_on_scene_clear_es_event_handler(const ecs::EventEntityManagerAfterClear &)
{
  if (!lagcatcher_deadline_ms)
    return;
  eastl::fixed_string<char, 128, true> lagspath;
  lagspath.sprintf("%slags.txt", get_log_directory());
  debug("Start saving lagcatcher samples to '%s'...", lagspath.c_str());
  ScopeSetWatchdogTimeout wt(dgs_get_settings()->getBlockByNameEx("lagcatcher")->getReal("watchdog_timeout_sec", 120.0) * 1000);
  int timenow = get_time_msec();
  int nwr = lagcatcher::flush(lagspath.c_str());
  debug("Written %d lags samples to '%s' for %d ms", nwr, lagspath.c_str(), get_time_msec() - timenow);
  lagcatcher::shutdown();
  lagcatcher_deadline_ms = 0;
}
