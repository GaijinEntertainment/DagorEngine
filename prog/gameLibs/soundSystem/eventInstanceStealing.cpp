// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <fmod_studio.hpp>
#include <soundSystem/events.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/eventInstanceStealing.h>
#include "internal/fmodCompatibility.h"
#include "internal/delayed.h"
#include "internal/events.h"
#include "internal/debug.h"

namespace sndsys
{
static constexpr int max_target_instances = 16;
static constexpr float min_fade_in_out_speed = 1.f / 8.f;

struct EventInstanceStealingGroup
{
  using instances_t = eastl::fixed_vector<EventHandle, max_target_instances, false>;

  instances_t instances;

  const eastl::fixed_string<char, 32> name;
  const int targetInstances;
  const float fadeInOutSpeed;

  EventInstanceStealingGroup() = delete;
  EventInstanceStealingGroup(const char *name, int target_instances, float fade_in_out_speed) :
    name(name), targetInstances(target_instances), fadeInOutSpeed(fade_in_out_speed)
  {}
};

static inline Point3 get_pos(const FMOD::Studio::EventInstance *event_instance)
{
  Attributes3D attributes;
  SOUND_VERIFY(event_instance->get3DAttributes(&attributes));
  return as_point3(attributes.position);
}

static inline bool is_playing(const FMOD::Studio::EventInstance *event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
  SOUND_VERIFY(event_instance->getPlaybackState(&playbackState));
  return playbackState != FMOD_STUDIO_PLAYBACK_STOPPED;
}

static void update(EventHandle handle, EventInstanceStealingGroup &grp, float dt)
{
#if DAGOR_DBGLEVEL > 0
  if (delayed::is_delayed(handle))
  {
    logerr("May not append delayed event to stealing group");
    return;
  }
#endif

  FMOD::Studio::EventInstance *instance = fmodapi::get_instance(handle);

  if (!instance)
    return;

  bool shouldPlay = true;

  auto it = eastl::find(grp.instances.begin(), grp.instances.end(), handle);
  if (it == grp.instances.end())
  {
    if (grp.instances.size() < grp.targetInstances)
      grp.instances.push_back(handle);
    else
    {
      static constexpr float race_avoid_dist_mul = 1.1f;
      const Point3 listener = get_3d_listener_pos();
      const float distSq = lengthSq((get_pos(instance) - listener) * race_avoid_dist_mul);

      EventHandle *stealable = nullptr;
      float furthestDistSq = 0.f;
      for (EventHandle &other : grp.instances)
      {
        const FMOD::Studio::EventInstance *otherInstance = fmodapi::get_instance(other);

        if (!otherInstance)
        {
          stealable = &other;
          break;
        }

        const float otherDistSq = lengthSq(get_pos(otherInstance) - listener);
        if (otherDistSq > furthestDistSq && distSq < otherDistSq)
        {
          furthestDistSq = otherDistSq;
          stealable = &other;
        }
      }

      if (stealable)
        *stealable = handle;
      else
        shouldPlay = false;
    }
  }

  const bool volumeControlEnabled = grp.fadeInOutSpeed > 0.f;

  if (shouldPlay)
  {
    if (!is_playing(instance))
      SOUND_VERIFY(instance->start());
    if (volumeControlEnabled)
      adjust_stealing_volume(handle, dt * grp.fadeInOutSpeed);
  }
  else if (is_playing(instance))
  {
    if (!volumeControlEnabled || adjust_stealing_volume(handle, -dt * grp.fadeInOutSpeed) == 0.f)
      SOUND_VERIFY(instance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
  }
}

static eastl::vector<EventInstanceStealingGroup> g_all_groups;

static WinCritSec g_stealing_cs;
#define SNDSYS_STEALING_BLOCK WinAutoLock stealingLock(g_stealing_cs);

int create_event_instance_stealing_group(const char *group_name, int target_instances, float fade_in_out_speed)
{
  if (!group_name || !*group_name || target_instances <= 0)
    return -1;

  target_instances = min(target_instances, max_target_instances);
  if (fade_in_out_speed != 0.f)
    fade_in_out_speed = max(fade_in_out_speed, min_fade_in_out_speed);

  SNDSYS_STEALING_BLOCK;
  auto pred = [group_name](const auto &it) { return it.name == group_name; };
  auto it = eastl::find_if(g_all_groups.begin(), g_all_groups.end(), pred);
  const int groupIdx = it - g_all_groups.begin();
  if (it == g_all_groups.end())
    g_all_groups.emplace_back(group_name, target_instances, fade_in_out_speed);
  return groupIdx;
}

void update_event_instance_stealing(EventHandle handle, int group_idx, float dt)
{
  SNDSYS_STEALING_BLOCK;
  G_ASSERT_RETURN(size_t(group_idx) < g_all_groups.size(), );
  update(handle, g_all_groups[group_idx], dt);
}
} // namespace sndsys
