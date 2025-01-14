//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lruCollision.h>

struct LRUCollisionVoxelization
{
  eastl::unique_ptr<ShaderMaterial> renderCollisionMat;
  ShaderElement *renderCollisionElem = 0;
  eastl::unique_ptr<ShaderMaterial> voxelizeCollisionMat;
  ShaderElement *voxelizeCollisionElem = 0;
  eastl::unique_ptr<ComputeShaderElement> voxelize_collision_cs;

  ~LRUCollisionVoxelization();

  void init();
  void rasterize(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles, VolTexture *world_sdf, VolTexture *world_sdf_alpha,
    ShaderElement *e, int instMul, bool prim);
  void rasterizeSDF(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles, bool prim);
  void voxelizeSDFCompute(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles);
};
