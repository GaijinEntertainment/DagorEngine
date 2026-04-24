// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <generic/dag_relocatableFixedVector.h>

class DynamicRenderableSceneInstance;

struct AimRenderingData
{
  bool farDofEnabled = false;
  bool lensRenderEnabled = false;
  bool isAiming = false;
  ecs::EntityId entityWithScopeLensEid;
  int lensNodeId = -1;
  int lensCrosshairNodeId = -1;
  int lensCollisionNodeId = -1;
  float lensBoundingSphereRadius = 1.0f;
};

AimRenderingData get_aim_rendering_data();
bool lens_renderer_enabled_globally();
const DynamicRenderableSceneInstance *get_scope_lens(const AimRenderingData &aimData);
