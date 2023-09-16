//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector_set.h>
#include <math/dag_Point3.h>
#include <soundSystem/handle.h>

namespace FMOD
{
namespace Studio
{
class EventInstance;
}
}; // namespace FMOD

namespace sndsys
{
// seems like stolen or virtual instances in 3d are not being updated by fmod.
// stolen or virtual instance will/may not wake when listener arrives near it.
// stolen stopped instance will/may never be started.
// currently there is no solution how to make fmod handle this properly.
//
// sort instances by distance and stop instances with index outside of max_instances count.
// made to manage non-oneshot instances of different types and big hearing range.
// fmod event stealing supposed to be set to 'virtualize' and max instances should be greater than target_instances.
struct EventInstanceStealingGroup
{
private:
  struct Instance
  {
    EventHandle handle;
    FMOD::Studio::EventInstance *eventInstance = nullptr;
    float distSq = 0.f;
    float volume = 0.f;
    bool shouldPlay = false;
    bool isInited = false;

    Instance() = default;
    Instance(EventHandle handle) : handle(handle) {}

    __forceinline bool operator<(const Instance &other) const { return (sound_handle_t)handle < (sound_handle_t)other.handle; }
  };

  eastl::vector_set<Instance> instances;
  float nextUpdateTime = 0.f;

public:
  void append(EventHandle event_handle);
  void update(float cur_time, float dt, float update_interval, float fade_in_out_speed, int target_instances);
};
} // namespace sndsys
