// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <daECS/core/componentType.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <ecs/sound/soundGroup.h>
#include <animChar/dag_animCharacter2.h>
#include <soundSystem/events.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <ecs/delayedAct/actInThread.h>

static WinCritSec g_group_cs;
#define SOUND_GROUP_BLOCK WinAutoLock groupLock(g_group_cs);

ECS_REGISTER_RELOCATABLE_TYPE(SoundEventGroup, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SoundEventGroup, "sound_event_group", nullptr, 0);

using namespace sndsys;

static sndsys::event_id_t g_empty_id = SND_HASH("");

static inline bool test_id(const SoundEventGroup::Sound &snd, sound_handle_t id) { return id == snd.id && id != g_empty_id; }

EventHandle get_sound(const SoundEventGroup &group, event_id_t id)
{
  SOUND_GROUP_BLOCK;
  for (const SoundEventGroup::Sound &snd : group.sounds)
    if (test_id(snd, id))
      return snd.handle;
  return {};
}

bool has_sound(const SoundEventGroup &group, event_id_t id)
{
  SOUND_GROUP_BLOCK;
  for (const SoundEventGroup::Sound &snd : group.sounds)
    if (test_id(snd, id))
      return true;
  return false;
}

bool has_sound(const SoundEventGroup &group, const char *name, const char *path)
{
  SOUND_GROUP_BLOCK;
  auto desc = sndsys::fmodapi::get_description(name, path);
  auto pred = [desc](const SoundEventGroup::Sound &sound) { return sndsys::fmodapi::get_description(sound.handle) == desc; };
  return eastl::find_if(group.sounds.begin(), group.sounds.end(), pred) != group.sounds.end();
}

void reject_sound(SoundEventGroup &group, event_id_t id, bool stop)
{
  SOUND_GROUP_BLOCK;
  for (SoundEventGroup::Sound &snd : group.sounds)
    if (snd.id == id)
    {
      if (stop || !is_oneshot(snd.handle))
        ::stop(snd.handle, true, true);
      snd.id = g_empty_id;
    }
}

void abandon_sound(SoundEventGroup &group, sndsys::event_id_t id)
{
  SOUND_GROUP_BLOCK;
  for (SoundEventGroup::Sound &snd : group.sounds)
    if (snd.id == id)
    {
      abandon(snd.handle);
      snd.id = g_empty_id;
    }
}

void abandon_all_sounds(SoundEventGroup &group)
{
  SOUND_GROUP_BLOCK;
  for (SoundEventGroup::Sound &snd : group.sounds)
    abandon(snd.handle);
  group.sounds.clear();
}

void release_all_sounds(SoundEventGroup &group)
{
  SOUND_GROUP_BLOCK;
  for (SoundEventGroup::Sound &snd : group.sounds)
    release(snd.handle);
  group.sounds.clear();
}

template <typename Release>
static __forceinline void validate_sounds(SoundEventGroup &group, Release release)
{
  auto it = eastl::remove_if(group.sounds.begin(), group.sounds.end(), release);
  group.sounds.erase(it, group.sounds.end());
}

static const auto def_release_sound = [](SoundEventGroup::Sound &snd) {
  if (!is_playing(snd.handle))
  {
    ::release(snd.handle);
    return true;
  }
  return false;
};

void release_sound(SoundEventGroup &group, event_id_t id)
{
  SOUND_GROUP_BLOCK;
  validate_sounds(group, [id](SoundEventGroup::Sound &snd) {
    if (snd.id != id)
      return false;
    ::release(snd.handle);
    return true;
  });
}

void remove_sound(SoundEventGroup &group, const sndsys::EventHandle &handle)
{
  SOUND_GROUP_BLOCK;
  validate_sounds(group, [&handle](SoundEventGroup::Sound &snd) {
    if (snd.handle != handle)
      return false;
    snd.handle.reset();
    return true;
  });
}

static inline void update_sounds_impl(SoundEventGroup &group, const Animchar &animchar)
{
  validate_sounds(group, def_release_sound);

  TMatrix wtm;
  animchar.getTm(wtm);
  for (SoundEventGroup::Sound &snd : group.sounds)
  {
    if (auto id = get_node_id(snd.handle))
      set_3d_attr(snd.handle, animchar.getNodeTree().getNodeWposScalar(id));
    else
      set_3d_attr(snd.handle, wtm * snd.localPos);
  }
}
static void inline update_sounds_impl(SoundEventGroup &group, const TMatrix &transform)
{
  validate_sounds(group, def_release_sound);

  for (SoundEventGroup::Sound &snd : group.sounds)
    set_3d_attr(snd.handle, transform * snd.localPos);
}

void update_sounds(SoundEventGroup &group, const Animchar &animchar)
{
  SOUND_GROUP_BLOCK;
  update_sounds_impl(group, animchar);
}

void update_sounds(SoundEventGroup &group, const TMatrix &transform)
{
  SOUND_GROUP_BLOCK;
  update_sounds_impl(group, transform);
}

void update_sounds(SoundEventGroup &group)
{
  SOUND_GROUP_BLOCK;
  validate_sounds(group, def_release_sound);
}

static __forceinline bool can_append(const SoundEventGroup &group) { return group.sounds.size() < group.sounds.static_size; }

template <typename Pred>
static void steal_sound(SoundEventGroup &group, Pred pred)
{
  SoundEventGroup::Sound *best = nullptr;
  for (SoundEventGroup::Sound &snd : group.sounds)
    if (!best || pred(*best, snd))
      best = &snd;
  if (!best)
    return;
  release(best->handle);
  update_sounds(group);
}

static __forceinline int get_num_instances(SoundEventGroup &group, EventHandle handle)
{
  int count = 0;
  for (const SoundEventGroup::Sound &snd : group.sounds)
    if (are_of_the_same_desc(handle, snd.handle))
      ++count;
  return count;
}

void add_sound(SoundEventGroup &group, event_id_t id, const Point3 &local_pos, EventHandle handle, int max_instances)
{
  if (!handle)
    return;

  SOUND_GROUP_BLOCK;

  if (max_instances > 0 && get_num_instances(group, handle) >= max_instances)
    steal_sound(group, [&](const SoundEventGroup::Sound &best, const SoundEventGroup::Sound &better) {
      const bool isSameDesc = are_of_the_same_desc(handle, best.handle);
      const bool betterIsSameDesc = are_of_the_same_desc(handle, better.handle);
      if (isSameDesc != betterIsSameDesc)
        return betterIsSameDesc;

      const bool isSameId = best.id == id;
      const bool betterIsSameId = better.id == id;
      if (isSameId != betterIsSameId)
        return betterIsSameId;

      const bool isEmpty = best.id == g_empty_id;
      const bool betterIsEmpty = better.id == g_empty_id;
      if (isEmpty != betterIsEmpty)
        return betterIsEmpty;

      return &better < &best;
    });

  if (!can_append(group))
  {
    sndsys::debug_trace_warn("Sound group capacity exceeded(%d)", group.sounds.size());
    steal_sound(group, [&](const SoundEventGroup::Sound &best, const SoundEventGroup::Sound &better) {
      const bool isPlaying = is_playing(best.handle) && !is_stopping(best.handle);
      const bool betterIsPlaying = is_playing(better.handle) && !is_stopping(better.handle);
      if (isPlaying != betterIsPlaying)
        return !betterIsPlaying;

      const bool isSameId = best.id == id;
      const bool betterIsSameId = better.id == id;
      if (isSameId != betterIsSameId)
        return betterIsSameId;

      const bool isEmpty = best.id == g_empty_id;
      const bool betterIsEmpty = better.id == g_empty_id;
      if (isEmpty != betterIsEmpty)
        return betterIsEmpty;

      const bool isOneshot = is_oneshot(best.handle);
      const bool betterIsOneshot = is_oneshot(better.handle);
      if (isOneshot != betterIsOneshot)
        return betterIsOneshot;

      const bool isSameDesc = are_of_the_same_desc(handle, best.handle);
      const bool betterIsSameDesc = are_of_the_same_desc(handle, better.handle);
      if (isSameDesc != betterIsSameDesc)
        return betterIsSameDesc;

      return &better < &best;
    });
  }

  if (can_append(group))
    group.sounds.push_back(SoundEventGroup::Sound(id, local_pos, handle));
  else
  {
    abandon(handle);
    logerr("Sound group capacity exceeded(%d)", group.sounds.size());
  }
}

int get_num_sounds(const SoundEventGroup &group, sndsys::event_id_t id)
{
  SOUND_GROUP_BLOCK;
  int count = 0;
  for (const SoundEventGroup::Sound &snd : group.sounds)
    if (snd.id == id)
      ++count;
  return count;
}

int get_num_sounds(const SoundEventGroup &group)
{
  SOUND_GROUP_BLOCK;
  return group.sounds.size();
}

SoundEventGroup::~SoundEventGroup() { release_all_sounds(*this); }


template <typename Callable>
static inline void sound_group_with_animchar_ecs_query(Callable c);

template <typename Callable>
static inline void sound_group_with_transform_ecs_query(Callable c);


ECS_TAG(sound)
static inline void update_sound_group_using_animchar_es(const ParallelUpdateFrameDelayed &)
{
  SOUND_GROUP_BLOCK;
  sound_group_with_animchar_ecs_query([](SoundEventGroup &sound_event_group,
                                        const AnimV20::AnimcharBaseComponent &animchar ECS_REQUIRE(
                                          ecs::Tag sound_group_with_animchar)) { update_sounds_impl(sound_event_group, animchar); });
}

ECS_TAG(sound)
static inline void update_sound_group_using_transform_es(const ParallelUpdateFrameDelayed &)
{
  SOUND_GROUP_BLOCK;
  sound_group_with_transform_ecs_query(
    [](SoundEventGroup &sound_event_group, const TMatrix &transform ECS_REQUIRE_NOT(ecs::Tag sound_group_with_animchar)) {
      update_sounds_impl(sound_event_group, transform);
    });
}


ECS_TAG(sound)
ECS_ON_EVENT(on_disappear)
static inline void destroy_sound_group_es_event_handler(const ecs::Event &, SoundEventGroup &sound_event_group)
{
  release_all_sounds(sound_event_group);
}
