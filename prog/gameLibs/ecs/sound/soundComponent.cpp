// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentType.h>
#include <ecs/core/entityManager.h>
#include <ecs/sound/soundComponent.h>

ECS_REGISTER_RELOCATABLE_TYPE(SoundEvent, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(SoundStream, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(SoundVarId, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(SoundEventsPreload, nullptr);

ECS_DEF_PULL_VAR(soundComponent);
