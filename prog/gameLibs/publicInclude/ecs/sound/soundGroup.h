//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_staticTab.h>
#include <soundSystem/eventId.h>
#include <soundSystem/handle.h>
#include <daECS/core/component.h>
#include <math/dag_Point3.h>

struct SoundEventGroup
{
  using time_t = uint32_t;
  struct Sound
  {
    sndsys::event_id_t id = SND_ID("");
    sndsys::EventHandle handle;
    Point3 localPos = {};

    Sound() = default;
    Sound(sndsys::event_id_t id, const Point3 &local_pos, sndsys::EventHandle handle) : id(id), localPos(local_pos), handle(handle) {}
  };
  static constexpr int capacity = 24;
  static constexpr int def_max_instances = 4;
  StaticTab<Sound, capacity> sounds;
  SoundEventGroup() = default;
  SoundEventGroup(SoundEventGroup &&) = default;
  SoundEventGroup &operator=(SoundEventGroup &&) = default;
  ~SoundEventGroup();
};

ECS_DECLARE_RELOCATABLE_TYPE(SoundEventGroup);

namespace AnimV20
{
class AnimcharBaseComponent;
}
using Animchar = AnimV20::AnimcharBaseComponent;

sndsys::EventHandle get_sound(const SoundEventGroup &group, sndsys::event_id_t id);
bool has_sound(const SoundEventGroup &group, sndsys::event_id_t id);
bool has_sound(const SoundEventGroup &group, const char *name, const char *path);

// keyoff/fadeout each sound with specific id using stop(allow_fadeout=true) and reset its id to make it inaccessible from the outside.
// keep and update sound in group and then automatically release and remove from group on play finished
void reject_sound(SoundEventGroup &group, sndsys::event_id_t id, bool stop = false);

void abandon_sound(SoundEventGroup &group, sndsys::event_id_t id);
void release_sound(SoundEventGroup &group, sndsys::event_id_t id);
void abandon_all_sounds(SoundEventGroup &group);
void release_all_sounds(SoundEventGroup &group);
void update_sounds(SoundEventGroup &group, const TMatrix &transform);
void update_sounds(SoundEventGroup &group, const Animchar &animchar);
void update_sounds(SoundEventGroup &group);

void add_sound(SoundEventGroup &group, sndsys::event_id_t id, const Point3 &local_pos, sndsys::EventHandle handle,
  int max_instances = SoundEventGroup::def_max_instances);

void remove_sound(SoundEventGroup &group, const sndsys::EventHandle &handle);
int get_num_sounds(const SoundEventGroup &group, sndsys::event_id_t id);
int get_num_sounds(const SoundEventGroup &group);
inline int get_max_capacity(const SoundEventGroup &) { return SoundEventGroup::capacity; }
