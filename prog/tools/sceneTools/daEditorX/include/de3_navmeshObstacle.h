//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>

struct NavMeshObstacle
{
  NavMeshObstacle() = default;
  NavMeshObstacle(uint32_t res_name_hash, const BBox3 &box, float y_angle, const BBox3 &aabb, uint32_t vtx_start, uint32_t vtx_count) :
    resNameHash(res_name_hash), box(box), yAngle(y_angle), aabb(aabb), vtxStart(vtx_start), vtxCount(vtx_count)
  {}

  uint32_t resNameHash;
  BBox3 box;
  float yAngle;
  BBox3 aabb;
  uint32_t vtxStart;
  uint32_t vtxCount;
};
