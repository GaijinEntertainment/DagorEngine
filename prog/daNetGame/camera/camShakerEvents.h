// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <math/dag_Point3.h>
#define CAM_SHAKER_EVENTS                                                          \
  CAM_SHAKER_EVENT_TYPE(ProjectileHitShake, Point3 /*position*/, float /*damage*/) \
  CAM_SHAKER_EVENT_TYPE(EventVehicleEarthTremor, Point3 /*position*/, float /*mass*/, float /*speed*/)

#define CAM_SHAKER_EVENT_TYPE ECS_BROADCAST_EVENT_TYPE
CAM_SHAKER_EVENTS
#undef CAM_SHAKER_EVENT_TYPE
ECS_UNICAST_EVENT_TYPE(EventShellExplosionShockWave, Point3 /*position*/, float /*dmgRadius*/, float /*dmgHp*/)
