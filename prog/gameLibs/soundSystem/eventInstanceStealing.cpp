#include <EASTL/sort.h>
#include <fmod_studio.hpp>
#include <memory/dag_framemem.h>
#include <soundSystem/events.h>
#include <soundSystem/eventInstanceStealing.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include "internal/fmodCompatibility.h"
#include "internal/delayed.h"
#include "internal/debug.h"

namespace sndsys
{
void EventInstanceStealingGroup::append(EventHandle event_handle)
{
#if DAGOR_DBGLEVEL > 0
  if (delayed::is_delayed(event_handle))
  {
    logerr("Should not append delayed event to stealing group");
  }
#endif
  Instance inst(event_handle);
  auto ins = instances.insert(eastl::move(inst));
  if (ins.second) // inserted
    nextUpdateTime = 0.f;
}

static inline Point3 get_pos(FMOD::Studio::EventInstance *event_instance)
{
  Attributes3D attributes;
  SOUND_VERIFY(event_instance->get3DAttributes(&attributes));
  return as_point3(attributes.position);
}

static inline float get_volume(FMOD::Studio::EventInstance *event_instance)
{
  float volume = 0.f;
  SOUND_VERIFY(event_instance->getVolume(&volume));
  return volume;
}

static inline bool is_playing(FMOD::Studio::EventInstance *event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
  SOUND_VERIFY(event_instance->getPlaybackState(&playbackState));
  return playbackState != FMOD_STUDIO_PLAYBACK_STOPPED;
}

void EventInstanceStealingGroup::update(float cur_time, float dt, float update_interval, float fade_in_out_speed, int target_instances)
{
  if (cur_time >= nextUpdateTime)
  {
    nextUpdateTime = cur_time + update_interval;
    for (intptr_t idx = 0; idx < instances.size(); ++idx)
    {
      instances[idx].eventInstance = fmodapi::get_instance(instances[idx].handle);
      if (!instances[idx].eventInstance)
      {
        size_t numValidInstances = idx;
        for (++idx; idx < instances.size(); ++idx)
          if (FMOD::Studio::EventInstance *eventInstance = fmodapi::get_instance(instances[idx].handle))
          {
            instances[idx].eventInstance = eventInstance;
            instances[numValidInstances++] = eastl::move(instances[idx]);
          }
        instances.erase(instances.begin() + numValidInstances, instances.end());
        break;
      }
    }

    const Point3 listenerPos = get_3d_listener_pos();

    eastl::vector<Instance *, framemem_allocator> list;
    list.reserve(instances.size());

    for (Instance &inst : instances)
    {
      list.push_back(&inst);
      inst.distSq = lengthSq(get_pos(inst.eventInstance) - listenerPos);
    }

    eastl::sort(list.begin(), list.end(), [](const auto &a, const auto &b) { return a->distSq < b->distSq; });

    intptr_t idx = 0;
    for (Instance *inst : list)
    {
      inst->shouldPlay = idx < target_instances;
      ++idx;
    }
  }

  for (Instance &inst : instances)
  {
    if (!inst.eventInstance || !inst.eventInstance->isValid())
      continue;

    if (!inst.isInited)
    {
      inst.isInited = true;
      inst.volume = inst.shouldPlay ? 1.f : 0.f;
      SOUND_VERIFY(inst.eventInstance->setVolume(inst.volume));
      if (is_playing(inst.eventInstance) != inst.shouldPlay)
      {
        if (inst.shouldPlay)
        {
          SOUND_VERIFY(inst.eventInstance->start());
        }
        else
        {
          SOUND_VERIFY(inst.eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
        }
      }
    }
    else if (inst.shouldPlay && inst.volume < 1.f)
    {
      if (!inst.volume)
        SOUND_VERIFY(inst.eventInstance->start());
      inst.volume = min(inst.volume + dt * fade_in_out_speed, 1.f);
      SOUND_VERIFY(inst.eventInstance->setVolume(inst.volume));
    }
    else if (!inst.shouldPlay && inst.volume > 0.f)
    {
      inst.volume = max(0.f, inst.volume - dt * fade_in_out_speed);
      SOUND_VERIFY(inst.eventInstance->setVolume(inst.volume));
      if (!inst.volume)
        SOUND_VERIFY(inst.eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
    }
  }
}
} // namespace sndsys
