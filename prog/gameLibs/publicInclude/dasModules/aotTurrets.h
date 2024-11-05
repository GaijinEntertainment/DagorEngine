//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <ecs/phys/turretControl.h>

MAKE_TYPE_FACTORY(TurretAimDrivesMult, TurretAimDrivesMult);
MAKE_TYPE_FACTORY(TurretState, TurretState);
MAKE_TYPE_FACTORY(ShootState, TurretState::ShootState);
MAKE_TYPE_FACTORY(RemoteState, TurretState::RemoteState);

namespace bind_dascript
{
inline void set_aim_mult_yaw(TurretAimDrivesMult &state, float mult) { state.setAimMultYaw(mult); }
inline void set_aim_mult_pitch(TurretAimDrivesMult &state, float mult) { state.setAimMultPitch(mult); }
} // namespace bind_dascript
