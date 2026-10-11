// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <levelCollision.h>
#include <render/lruCollision.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>

struct LRUCollision
{
  LevelCollision level;
  LRUCollisionVoxelization voxelize;
  eastl::unique_ptr<LRURendinstCollision> lruColl;

  void init()
  {
    lruColl.reset(new LRURendinstCollision);
    voxelize.init();
    set_lru_collision_level(&level);
  }

  void gatherBox(bbox3f_cref box, dag::Vector<uint64_t, framemem_allocator> &handles) const
  {
    level.boxCull(box, [&](uint64_t h, mat44f_cref) { handles.push_back(h); });
    level.sortByPool(dag::Span<uint64_t>(handles.data(), handles.size()));
  }

  // any thread
  template <class Handles>
  void cull(mat44f_cref viewproj, Handles &handles) const
  {
    handles.clear();
    level.frustumCull(viewproj, v_zero(), [&](uint64_t h, mat44f_cref, vec4f) { handles.push_back(h); });
    level.sortByPool(dag::Span<uint64_t>(handles.data(), handles.size()));
  }
  void rasterize(mat44f_cref viewproj)
  {
    DA_PROFILE;
    FRAMEMEM_REGION;
    dag::Vector<uint64_t, framemem_allocator> handles;
    cull(viewproj, handles);
    draw(dag::ConstSpan<uint64_t>(handles.data(), handles.size()));
  }
  void draw(dag::ConstSpan<uint64_t> handles)
  {
    if (!lruColl || handles.empty())
      return;
    const int old = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(ShaderGlobal::getBlockId("global_frame", ShaderGlobal::LAYER_FRAME), ShaderGlobal::LAYER_FRAME);
    voxelize.rasterize(*lruColl, handles, nullptr, nullptr, voxelize.renderCollisionElem, 1, false);
    ShaderGlobal::setBlock(old, ShaderGlobal::LAYER_FRAME);
  }
};
