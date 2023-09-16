//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <soundSystem/events.h>
#include <soundSystem/streams.h>
#include <soundSystem/varId.h>
#include <daECS/core/componentType.h>

template <typename Handle>
struct SoundComponent
{
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
  // game code specific, useful to store event state
  bool enabled = false;
};

using SoundEvent = SoundComponent<sndsys::EventHandle>;
using SoundStream = SoundComponent<sndsys::StreamHandle>;
using SoundVarId = sndsys::VarId;

ECS_DECLARE_RELOCATABLE_TYPE(SoundEvent);
ECS_DECLARE_RELOCATABLE_TYPE(SoundStream);
ECS_DECLARE_RELOCATABLE_TYPE(SoundVarId);
