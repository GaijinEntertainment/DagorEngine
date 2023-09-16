//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <daECS/core/componentType.h>
#include <gamePhys/ballistics/projectileBallistics.h>
#include <gamePhys/ballistics/shellBallistics.h>
#include <ecs/game/weapons/gunES.h>
#include <ecs/gamePhys/ballistics.h>

MAKE_TYPE_FACTORY(ProjectileBallisticsState, ballistics::ProjectileBallistics::State);
MAKE_TYPE_FACTORY(ProjectileBallistics, ballistics::ProjectileBallistics);
MAKE_TYPE_FACTORY(ShellEnv, ballistics::ShellEnv);
MAKE_TYPE_FACTORY(ShellState, ballistics::ShellState);
