//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityId.h>

class DynamicRenderableSceneInstance;
class CollisionResource;

namespace dafg
{
class NodeHandle;
}

struct AimRenderingData
{
  bool farDofEnabled = false;
  bool lensRenderEnabled = false;
  bool isAiming = false;
  ecs::EntityId entityWithScopeLensEid;
  int lensNodeId = -1;
  int lensCollisionNodeId = -1;
  float lensBoundingSphereRadius = 1.0f;
  float scopeLength = 0.0f;
  float aimingTime = 0.0f;
};

AimRenderingData get_aim_rendering_data();
const DynamicRenderableSceneInstance *get_scope_lens(const AimRenderingData &aim_data);
float get_scope_lens_bs_radius(const CollisionResource *collres, const int node_id);
float get_scope_length(const CollisionResource *collres);
dafg::NodeHandle makeGenAimRenderingDataNode();

// ViewportEllipse blob with history, from the lens node wtm and current_cockpit_camera; no lens = no pixel inside
inline constexpr const char *SCOPE_LENS_ELLIPSE_BLOB = "scope_lens_ellipse";
dafg::NodeHandle makeScopeLensEllipseNode();
