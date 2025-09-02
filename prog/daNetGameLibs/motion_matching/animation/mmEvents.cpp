// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentType.h>
#include <animation/mmEvents.h>

ECS_REGISTER_EVENT(MotionMatchingStartSearchJob);
ECS_REGISTER_EVENT(InvalidateAnimationDataBase);
ECS_REGISTER_EVENT(AnimationDataBaseAssigned);

ECS_DEF_PULL_VAR(mmEvents);
