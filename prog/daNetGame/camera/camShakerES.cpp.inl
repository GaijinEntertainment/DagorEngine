// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <camera/cameraShaker.h>
#include <camera/camShakerEvents.h>
#include <camera/camShaker.h>


ECS_REGISTER_RELOCATABLE_TYPE(CameraShaker, nullptr);
ECS_AUTO_REGISTER_COMPONENT(CameraShaker, "camera_shaker", nullptr, 0);