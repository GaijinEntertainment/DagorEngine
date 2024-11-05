// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>

ECS_BROADCAST_EVENT_TYPE(CheckConnectivityState, /*ui_state__connectivity*/ int *);

namespace ui
{

enum
{
  CONNECTIVITY_OK,
  CONNECTIVITY_NO_PACKETS,
  CONNECTIVITY_OUTDATED_AAS
};

}
