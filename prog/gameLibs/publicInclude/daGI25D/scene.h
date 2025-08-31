//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds3.h>

namespace dagi25d
{
enum class SceneDebugType
{
  VIS_RAYCAST,
  EXACT_VOXELS,
  LIT_VOXELS
};

enum class UpdateResult
{
  NO_CHANGE,
  MOVED,
  BIG,
  TELEPORTED
};

using voxelize_scene_fun_cb = eastl::function<void(const BBox3 &box, const Point3 &voxel_size)>;

// 2d scene
class Scene
{
  DynamicShaderHelper debug_rasterize_voxels;
  UniqueBufHolder sceneAlpha;
  eastl::unique_ptr<ComputeShaderElement> ssgi_clear_scene_25d_cs, ssgi_clear_scene_25d_full_cs;
  IPoint2 toroidalOrigin = {-1000000, 100000};
  float voxelSizeXZ = 2.35f, voxelSizeY = 2.5f;

public:
  const ManagedBuf &getSceneVoxels() const { return sceneAlpha; };
  float getMaxDist() const;
  float getVoxelSizeXZ() const { return voxelSizeXZ; }
  int moveDistThreshold() const;
  int getXZResolution() const;
  int getYResolution() const;
  float getXZWidth() const { return getVoxelSizeXZ() * getXZResolution(); }
  void invalidate() { toroidalOrigin += IPoint2{10000, -10000}; }
  void init(bool scalar_ao, float xz_size = 2.0, float y_size = 2.0);
  void close();
  void debugRayCast(SceneDebugType debug_scene);
  UpdateResult updateOrigin(const Point3 &baseOrigin, const voxelize_scene_fun_cb &voxelize_cb, bool &update_pending);
  void setVars();
};
} // namespace dagi25d
