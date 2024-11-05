// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>

#define UI_ECS_EVENTS UI_ECS_EVENT(EventUiRegisterRendObjs)

#define UI_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
UI_ECS_EVENTS
#undef UI_ECS_EVENT
