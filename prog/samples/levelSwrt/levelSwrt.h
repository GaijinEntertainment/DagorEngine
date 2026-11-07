// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>
#include <dag/dag_vector.h>
#include <generic/dag_functionRef.h>

class LevelCollision;
struct RenderSWRT;

// the default min_radius: a resource with every instance under it gets no BLAS, and an instance under it no TLAS entry
inline constexpr float LEVEL_SWRT_MIN_RADIUS = 1.f;

// the leaf user value of a face (CollisionGeometryFeeder::PhysMatLeafUser), by resource and PhysMat id
using LevelSwrtLeafUser = dag::FunctionRef<uint8_t(uint32_t resource, int16_t phys_mat_id)>;

// One BLAS per resource with an instance of min_radius or more, built from its chunks on the thread pool
// and given to swrt (Full: fp16 verts only). model_of gets every resource: its model id (a tree or a box), or -1 without one.
// face_filter drops the faces of the PhysMat ids it refuses (unbound keeps all); with leaf_user unbound, a leaf keeps
// its node's palette index. Both run on workers. Returns the count of models with a tree.
// The caller adds only instances of min_radius or more to the TLAS.
int build_level_swrt_models(RenderSWRT &swrt, const LevelCollision &level, dag::Vector<int> &model_of,
  const CollisionGeometryFeeder::PhysMatFilter &face_filter = {}, LevelSwrtLeafUser leaf_user = {},
  float min_radius = LEVEL_SWRT_MIN_RADIUS);
