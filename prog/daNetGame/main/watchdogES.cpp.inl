// Copyright (C) Gaijin Games KFT.  All rights reserved.

///@module watchdog
#include "main/watchdog.h"
#include "main/level.h"
#include "main/gameLoad.h"
#include "ui/overlay.h"

#include <daGame/timers.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/random/dag_random.h>
#include "render/animatedSplashScreen.h"
#include <ecs/core/entityManager.h>

#include <sqModules/sqModules.h>
#include <sqrat.h>


namespace watchdog
{

#if EA_ASAN_ENABLED || defined(__SANITIZE_ADDRESS__)
static constexpr uint32_t THRESHOLDS_MULTIPLIER = 3;
#else
static constexpr uint32_t THRESHOLDS_MULTIPLIER = 1;
#endif

#if (DAGOR_DBGLEVEL > 0 || _TARGET_PC) && !DAGOR_THREAD_SANITIZER
static constexpr bool ENABLED_BY_DEFAULT = true;
#else
static constexpr bool ENABLED_BY_DEFAULT = false;
#endif


static constexpr uint32_t DEF_TRIGGER_THRESHOLD = 30000;
static constexpr uint32_t DEF_CALLSTACKS_THRESHOLD = 10000;
static constexpr uint32_t DEF_SLEEP_TIME = 500;


struct Mode
{
  uint32_t trigger = 0;
  uint32_t callstacks = 0;
  uint32_t sleep = 0;
};


static Mode modes[WatchdogMode::COUNT];
static const char *mode_names[] = {"loading", "game_session", "menu"};
static WatchdogMode current_mode = WatchdogMode::COUNT;


static void initialize_watchdog(int sleep, int trigger, int callstacks, bool fatal)
{
  WatchdogConfig wcfg;
  wcfg.triggering_threshold_ms = trigger;
  wcfg.dump_threads_threshold_ms = callstacks;
  wcfg.sleep_time_ms = sleep;

  if (!fatal)
    wcfg.flags |= WATCHDOG_NO_FATAL;

  if (wcfg.triggering_threshold_ms)
  {
    watchdog_init(&wcfg);
    animated_splash_screen_allow_watchdog_kick(true);
  }
}


static void enable_from_config(const DataBlock *blk)
{
  bool fatalOnTimeout = blk->getBool("fatal", true);
  uint32_t gTriggerThreshold = blk->getInt("trigger", DEF_TRIGGER_THRESHOLD);
  uint32_t gCallstacksThreshold = blk->getInt("callstacks", DEF_CALLSTACKS_THRESHOLD);
  uint32_t gSleepTime = blk->getInt("sleep", DEF_SLEEP_TIME);

  for (uint8_t i = 0; i < WatchdogMode::COUNT; ++i)
  {
    Mode &curMode = modes[i];
    const char *curModeName = mode_names[i];
    const DataBlock *modeBlk = blk->getBlockByNameEx(curModeName);
    curMode.trigger = modeBlk->getInt("trigger", gTriggerThreshold);
    curMode.callstacks = modeBlk->getInt("callstacks", gCallstacksThreshold);
    curMode.sleep = modeBlk->getInt("sleep", gSleepTime);

    uint16_t chance = modeBlk->getInt("chance", 0);
    G_ASSERT_CONTINUE(chance <= 1000);

    int dice = grnd() % 1000;
    curMode.trigger = (dice < chance) ? curMode.trigger : gTriggerThreshold;
    curMode.callstacks = (dice < chance) ? curMode.callstacks : gCallstacksThreshold;
    curMode.sleep = (dice < chance) ? curMode.sleep : gSleepTime;

    curMode.trigger *= THRESHOLDS_MULTIPLIER;
    curMode.callstacks *= THRESHOLDS_MULTIPLIER;
  }

  gTriggerThreshold *= THRESHOLDS_MULTIPLIER;
  gCallstacksThreshold *= THRESHOLDS_MULTIPLIER;

  initialize_watchdog(gSleepTime, gTriggerThreshold, gCallstacksThreshold, fatalOnTimeout);
}


WatchdogMode change_mode(WatchdogMode mode)
{
  G_ASSERT_RETURN(mode < WatchdogMode::COUNT, current_mode);
  Mode &newMode = modes[mode];
  const char *curModeName = (current_mode != WatchdogMode::COUNT) ? mode_names[current_mode] : "none";
  const char *newModeName = mode_names[mode];
  debug("Watchdog: change mode <%s> -> <%s> { trigger: %u, callstacks: %u, sleep: %u }", curModeName, newModeName, newMode.trigger,
    newMode.callstacks, newMode.sleep);
  watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, newMode.trigger);
  watchdog_set_option(WATCHDOG_OPTION_CALLSTACKS_THRESHOLD, newMode.callstacks);
  watchdog_set_option(WATCHDOG_OPTION_SLEEP, newMode.sleep);
  eastl::swap(mode, current_mode);
  return mode;
}


void bind_sq(SqModules *mgr)
{
#define ENUM_VAL(EV) .SetValue(#EV, (int)watchdog::EV)
  Sqrat::Table ns(mgr->getVM());
  ns.Func("change_mode", change_mode) ENUM_VAL(LOADING) ENUM_VAL(GAME_SESSION) ENUM_VAL(LOBBY);
#undef ENUM_VAL
  mgr->addNativeModule("watchdog", ns);
}


void init()
{
  const DataBlock *blk = dgs_get_settings()->getBlockByNameEx("debug")->getBlockByNameEx("watchdog");
  if (blk->getBool("enabled", ENABLED_BY_DEFAULT))
    enable_from_config(blk);
  else
    debug("Watchdog disabled");
}


void shutdown()
{
  animated_splash_screen_allow_watchdog_kick(false);
  watchdog_shutdown();
}

} // namespace watchdog


static inline void watchdog_es(const ecs::UpdateStageInfoAct &act)
{
  static watchdog::WatchdogMode watchdog_mode = watchdog::COUNT;
  static float watchdog_end_load_time = 0.f;

  if (sceneload::is_load_in_progress())
    watchdog_end_load_time = act.curTime + 5.f; // if we are loading
  const watchdog::WatchdogMode mode =
    act.curTime < watchdog_end_load_time ? watchdog::LOADING : watchdog::LOBBY /*<<< This is most probably wrong*/;
  if (DAGOR_UNLIKELY(watchdog_mode != mode))
    change_mode(watchdog_mode = mode);
}
