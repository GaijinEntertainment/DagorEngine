// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>
#include <dag/dag_vector.h>
#include <generic/dag_functionRef.h>

class LevelCollision;
struct RenderSWRT;

// a resource with every instance under this radius gets no BLAS, and an instance under it no TLAS entry
inline constexpr float LEVEL_SWRT_MIN_RADIUS = 1.f;

// the leaf user value of a face (CollisionGeometryFeeder::PhysMatLeafUser), by resource and PhysMat id
using LevelSwrtLeafUser = dag::FunctionRef<uint8_t(uint32_t resource, int16_t phys_mat_id)>;

// One BLAS per resource with an instance of LEVEL_SWRT_MIN_RADIUS or more, built from its chunks on the thread pool
// and given to swrt (fp16 verts only). model_of gets every resource: its model id (a tree or a box), or -1 without one.
// face_filter drops the faces of the PhysMat ids it refuses (unbound keeps all); with leaf_user unbound, a leaf keeps
// its node's palette index. Both run on workers. Returns the count of models with a tree; the caller adds the instances.
int build_level_swrt_models(RenderSWRT &swrt, const LevelCollision &level, dag::Vector<int> &model_of,
  const CollisionGeometryFeeder::PhysMatFilter &face_filter = {}, LevelSwrtLeafUser leaf_user = {});
