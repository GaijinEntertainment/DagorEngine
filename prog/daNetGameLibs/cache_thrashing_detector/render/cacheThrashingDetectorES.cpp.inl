// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <perfMon/dag_cpuFreq.h>

ECS_DECLARE_BOXED_TYPE(CallbackToken);
ECS_REGISTER_BOXED_TYPE(CallbackToken, nullptr);

static int logerr_at = 100000;
static int multiplier = 2;
static int start_time = 0;
static int downgradeRes_calls = 0;

ECS_TAG(render, dev)
ECS_ON_EVENT(on_appear)
static void cache_thrashing_detector_subscribe_es(const ecs::Event &,
  int cache_thrashing_detector__logerr_at,
  int cache_thrashing_detector__multiplier,
  CallbackToken &cache_thrashing_detector__callback_token)
{
  logerr_at = cache_thrashing_detector__logerr_at;
  multiplier = cache_thrashing_detector__multiplier;
  start_time = get_time_msec();
  downgradeRes_calls = 0;
  cache_thrashing_detector__callback_token = unitedvdata::riUnitedVdata.on_mesh_relems_updated.subscribe(
    [](const RenderableInstanceLodsResource *, bool deleted, int upper_lod) {
      if (deleted && upper_lod < INT_MAX)
      {
        downgradeRes_calls++;
        if (downgradeRes_calls >= logerr_at)
        {
          float timeSeconds = (get_time_msec() - start_time) / 1000.f;
          logerr("riUnitedVdata.downgradeRes has been called %d times since the start of the level in %f seconds!\n"
                 "That's %f downgrades per second! Increasing the reserved unitedVdata.rendInst buffer is recommended!",
            downgradeRes_calls, timeSeconds, downgradeRes_calls / timeSeconds);
          logerr_at *= multiplier;
        }
      }
    });
}
