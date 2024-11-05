// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/strHash.h>
#include <daECS/core/event.h>

ECS_BROADCAST_EVENT_TYPE(EventOnSoundPresetLoaded, sndsys::str_hash_t /*preset_name_hash*/, bool /*is_loaded*/)
ECS_BROADCAST_EVENT_TYPE(EventSoundDrawDebug)
