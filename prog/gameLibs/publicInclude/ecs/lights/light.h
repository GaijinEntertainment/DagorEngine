//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/componentType.h>
#include "lightHandle.h"


struct OmniLightEntity
{
  LightHandle lightId;
  bool ensureLightIdCreated(bool should_create);
};
ECS_DECLARE_RELOCATABLE_TYPE(OmniLightEntity);

struct SpotLightEntity
{
  LightHandle lightId;
  bool ensureLightIdCreated(bool should_create);
};
ECS_DECLARE_RELOCATABLE_TYPE(SpotLightEntity);

ECS_BROADCAST_EVENT_TYPE(CmdUpdateHighPriorityLights);
ECS_BROADCAST_EVENT_TYPE(CmdRecreateSpotLights);
ECS_BROADCAST_EVENT_TYPE(CmdRecreateOmniLights);
ECS_BROADCAST_EVENT_TYPE(CmdRecreateAllLights);
