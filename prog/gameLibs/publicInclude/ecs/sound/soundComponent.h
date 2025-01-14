//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/events.h>
#include <soundSystem/streams.h>
#include <soundSystem/varId.h>
#include <daECS/core/componentType.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/vector.h>
#include <generic/dag_span.h>
#include <memory/dag_framemem.h>

template <typename Handle>
struct SoundComponent
{
  SoundComponent() = default;
  SoundComponent(SoundComponent &&) = default;
  ~SoundComponent() { reset(); }

  inline void reset(Handle new_value = {})
  {
    if (handle != new_value && (bool)handle)
    {
      if (abandonOnReset)
        sndsys::abandon(handle);
      else
        sndsys::release(handle);
    }
    handle = new_value;
  }

  Handle handle;
  bool abandonOnReset = false;
  // game code specific(tag), useful to store event state
  bool enabled = false;
};

struct SoundEventsPreload
{
  ~SoundEventsPreload() { unload(); }

  SoundEventsPreload() = default;
  SoundEventsPreload(const SoundEventsPreload &) = delete;
  SoundEventsPreload(SoundEventsPreload &&) = default;
  SoundEventsPreload &operator=(const SoundEventsPreload &) = delete;
  SoundEventsPreload &operator=(SoundEventsPreload &&) = default;

  void unload()
  {
    for (const sndsys::FMODGUID &id : loaded)
      sndsys::unload_sample_data(id);
    loaded.clear();
  }

  void load(const dag::Span<const char *> &paths)
  {
    eastl::vector<sndsys::FMODGUID, framemem_allocator> input;

    input.reserve(paths.size());

    for (const char *path : paths)
    {
      sndsys::FMODGUID id;

      if (!path || !*path || !sndsys::get_event_id(path, "", false, id, false))
        continue;

      if (eastl::find(input.begin(), input.end(), id) != input.end())
        continue; // prevent, skip duplicates

      if (eastl::find(loaded.begin(), loaded.end(), id) == loaded.end())
      {
        if (!sndsys::load_sample_data(id))
          continue;
      }

      input.push_back(id);
    }

    for (const sndsys::FMODGUID &id : loaded)
    {
      if (eastl::find(input.begin(), input.end(), id) == input.end())
        sndsys::unload_sample_data(id);
    }

    loaded.clear();
    for (const sndsys::FMODGUID &id : input)
      loaded.push_back(id);
  }

private:
  static constexpr size_t inplace_count = 8;
  dag::RelocatableFixedVector<sndsys::FMODGUID, inplace_count> loaded;
};

using SoundEvent = SoundComponent<sndsys::EventHandle>;
using SoundStream = SoundComponent<sndsys::StreamHandle>;
using SoundVarId = sndsys::VarId;

ECS_DECLARE_RELOCATABLE_TYPE(SoundEvent);
ECS_DECLARE_RELOCATABLE_TYPE(SoundStream);
ECS_DECLARE_RELOCATABLE_TYPE(SoundVarId);
ECS_DECLARE_RELOCATABLE_TYPE(SoundEventsPreload);
