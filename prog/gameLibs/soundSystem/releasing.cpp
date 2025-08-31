// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <osApiWrappers/dag_critSec.h>
#include <EASTL/fixed_vector.h>
#include <fmod_studio.hpp>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include "internal/events.h"
#include "internal/releasing.h"
#include "internal/debug.h"

static WinCritSec g_releasing_cs;
#define SNDSYS_RELEASING_BLOCK WinAutoLock releasingLock(g_releasing_cs);

namespace sndsys
{
namespace releasing
{
struct Event
{
  FMOD::Studio::EventInstance *instance;
  float deadlineAt;
  float checkAt;
};

static eastl::fixed_vector<Event, 32> g_events;
static float g_cur_time = 0.f;

static bool is_playing(FMOD::Studio::EventInstance *instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
  if (FMOD_OK == instance->getPlaybackState(&playbackState))
    return playbackState != FMOD_STUDIO_PLAYBACK_STOPPED;
  return false;
}

void track(FMOD::Studio::EventInstance *instance)
{
  SNDSYS_RELEASING_BLOCK;
  G_ASSERT_RETURN(instance != nullptr, );
  auto pred = [&](const Event &it) { return it.instance == instance; };
  if (eastl::find_if(g_events.begin(), g_events.end(), pred) == g_events.end())
    g_events.push_back(Event{instance, g_cur_time + g_max_duration, g_cur_time + g_check_interval});
}

void update(float cur_time)
{
  SNDSYS_RELEASING_BLOCK;
  g_cur_time = cur_time;

  Event *ptr = g_events.begin();
  Event *end = g_events.end();
  for (; ptr < end;)
  {
    if (g_cur_time >= ptr->checkAt)
    {
      if (!is_playing(ptr->instance))
      {
        release_immediate_impl(*ptr->instance);
        *ptr = *(--end);
        continue;
      }
      ptr->checkAt = g_cur_time + g_check_interval;
    }
    if (g_cur_time >= ptr->deadlineAt)
    {
      debug_warn_about_deadline(*ptr->instance);

      release_immediate_impl(*ptr->instance);

      *ptr = *(--end);
      continue;
    }
    ++ptr;
  }
  g_events.erase(end, g_events.end());
}

void close()
{
  SNDSYS_RELEASING_BLOCK;
  for (Event &it : g_events)
    release_immediate_impl(*it.instance);
  g_events.clear();
}

int debug_get_num_events()
{
  SNDSYS_RELEASING_BLOCK;
  return g_events.size();
}

void release_immediate_impl(FMOD::Studio::EventInstance &instance)
{
  instance.stop(FMOD_STUDIO_STOP_IMMEDIATE);
  instance.release();
}

void debug_warn_about_deadline(const FMOD::Studio::EventInstance &instance)
{
#if DAGOR_DBGLEVEL > 0
  FMOD::Studio::EventDescription *description = nullptr;
  FrameStr path;
  if (FMOD_OK == instance.getDescription(&description) && description != nullptr)
    path = get_debug_name(*description);
  debug_trace_warn_once("abandoned instance '%s' plays too long", path.c_str());
#else
  G_UNREFERENCED(instance);
#endif
}

} // namespace releasing
} // namespace sndsys
