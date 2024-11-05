// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

ECS_BROADCAST_EVENT_TYPE(VromfsUpdaterEarlyInitEvent)
ECS_BROADCAST_EVENT_TYPE(VromfsUpdaterShutdownEvent)
ECS_BROADCAST_EVENT_TYPE(CheckVromfsUpdaterRunningEvent, bool * /*dest_is_running*/)
ECS_BROADCAST_EVENT_TYPE(VromfsUpdateOnStartEvent)

ECS_BROADCAST_EVENT_TYPE(MobileSelfUpdateOnStartEvent, DataBlock * /*preload_settings*/)
