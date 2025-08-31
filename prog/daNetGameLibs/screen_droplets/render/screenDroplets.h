// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>

ECS_BROADCAST_EVENT_TYPE(ScreenDropletsReset)

namespace dafg
{
class NodeHandle;
}

dafg::NodeHandle makeWaterDropletsNode();
