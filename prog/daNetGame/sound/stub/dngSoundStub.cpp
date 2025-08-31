// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sound/dngSound.h"
#include "sound/ecsEvents.h"

ECS_REGISTER_EVENT(EventOnSoundPresetLoaded);
ECS_REGISTER_EVENT(EventOnMasterSoundPresetLoaded);
ECS_REGISTER_EVENT(EventSoundDrawDebug);

namespace dngsound
{
void init() {}
void close() {}
void update(float) {}
void apply_config_volumes() {}
void debug_draw() {}
void reset_listener() {}
void flush_commands() {}
} // namespace dngsound
