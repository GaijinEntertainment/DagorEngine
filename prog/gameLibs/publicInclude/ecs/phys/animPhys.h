//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityComponent.h>
#include <daECS/core/event.h>
#include <gamePhys/phys/animatedPhys.h>

ECS_DECLARE_RELOCATABLE_TYPE(AnimatedPhys);
ECS_UNICAST_EVENT_TYPE(InvalidateAnimatedPhys);
