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

namespace dagi25d
{
enum class IrradianceDebugType
{
  INTERSECTED,
  INTERSECTION,
  AMBIENT,
};

class Irradiance
{
  DynamicShaderHelper debug_rasterize_probes;
  UniqueTexHolder volmap;
  // TextureIDHolder volmap_floor;// may be needed, to render it, as resolution is very small, and can include more than heightmap
  eastl::unique_ptr<ComputeShaderElement> // gi_compute_light_25d_cs,
    gi_compute_light_25d_cs_4_4, gi_compute_light_25d_cs_4_8, gi_compute_light_25d_cs_8_4, gi_mark_intersected_25d_cs;
  UniqueBufHolder intersection;
  IPoint2 toroidalOrigin = {-1000000, 100000};
  float voxelSizeXZ = 1.75f, voxelSizeY = 2.5f; // should be at least somehow less than Scene. I.e. we need at least 32 meters around
                                                // to trace
  uint32_t warpSize = 32;

public:
  const ManagedBuf &getIntersection() const { return intersection; };
  int getXZResolution() const;
  int getYResolution() const;
  int getSceneCoordTraceDist() const;
  float getXZWidth() const;
  float getMaxDist() const;
  void invalidate();
  Point2 getWidth() const;
  void init(bool scalar_ao, float xz_size = 1.75, float y_size = 2.5); // size*RESOLUTION should be ~64 meters smaller than
                                                                       // xz_size*SCENE_RESOLUTION!
  void close();
  void drawDebug(IrradianceDebugType type);
  enum UpdateResult
  {
    NO_CHANGE,
    UPDATE_MOVED,
    UPDATE_BIG,
    UPDATE_TELEPORTED
  };
  UpdateResult updateOrigin(const Point3 &baseOrigin);
  void setVars();
};
} // namespace dagi25d
