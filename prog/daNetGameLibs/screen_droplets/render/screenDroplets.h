// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>

ECS_BROADCAST_EVENT_TYPE(ScreenDropletsReset)

resource_slot::NodeHandleWithSlotsAccess makeWaterDropletsNode();
