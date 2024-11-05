// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>

#define DEF_INPUT_EVENTS DEF_INPUT_EVENT(UpdateStageUpdateInput, float /* cur_time */, float /* dt */)

#define DEF_INPUT_EVENT ECS_BROADCAST_PROFILE_EVENT_TYPE
DEF_INPUT_EVENTS
#undef DEF_INPUT_EVENT
