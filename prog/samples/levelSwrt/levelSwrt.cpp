// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "levelSwrt.h"
#include <levelCollision.h>
#include <daSWRT/swBVH.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_parallelForInline.h>

static bool has_swrt_instance(const LevelCollision &level, uint32_t r)
{
  for (uint32_t i = level.firstInstance(r), ie = level.firstInstance(r + 1); i < ie; ++i)
    if (v_extract_w(level.sphere(i)) >= LEVEL_SWRT_MIN_RADIUS)
      return true;
  return false;
}

int build_level_swrt_models(RenderSWRT &swrt, const LevelCollision &level, dag::Vector<int> &model_of,
  const CollisionGeometryFeeder::PhysMatFilter &face_filter, LevelSwrtLeafUser leaf_user)
{
  model_of.assign(level.resourceCount(), -1);
  WinCritSec addLock;
  int trees = 0;
  threadpool::parallel_for_inline(
    0, level.resourceCount(), 64,
    [&](uint32_t tbegin, uint32_t tend, uint32_t) {
      for (uint32_t r = tbegin; r < tend; ++r)
      {
        if (!has_swrt_instance(level, r))
          continue;
        CollisionGeometryFeeder::PhysMatLeafUser leaf;
        if (leaf_user)
          leaf = [&leaf_user, r](int16_t phys_mat_id) { return leaf_user(r, phys_mat_id); };
        daSWRT::BuiltBLAS built;
        if (!CollisionGeometryFeeder::buildSwrtBLAS(*level.resource(r), face_filter, 2.f, 16.f, leaf, swrt.tlasBoxUser, built))
          continue;
        const bool tree = !built.isBox();
        WinAutoLock lk(addLock);
        model_of[r] = swrt.addBuiltModel(eastl::move(built));
        trees += tree ? 1 : 0;
      }
    },
    32); // past 32 jobs the one hand-off lock starves the builders
  return trees;
}
