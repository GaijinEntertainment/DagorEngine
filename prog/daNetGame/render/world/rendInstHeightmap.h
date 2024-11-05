// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_relocatableFixedVector.h>
#include <render/toroidal_update.h>

class BBox3;
class Point3;

class RendInstHeightmap
{
public:
  RendInstHeightmap(int tex_size, float box_size, float land_height_min, float land_height_max);
  ~RendInstHeightmap();

  RendInstHeightmap(const RendInstHeightmap &) = delete;
  RendInstHeightmap(RendInstHeightmap &&other) = delete;
  RendInstHeightmap &operator=(const RendInstHeightmap &) = delete;
  RendInstHeightmap &operator=(RendInstHeightmap &&other) = delete;

  static bool has_heightmap_objects_on_scene();

  void updatePos(const Point3 &cam_pos);
  void prepareRiVisibilityAsync();
  void updateToroidalTextureRegions(int globalFrameBlockId);
  float getMaxHt() const { return maxHt; }
  void driverReset();

private:
  void invalidate();
  void setVar();

  bool enabled;
  int texSize;
  float rectSize;
  float landHeightMin, landHeightMax;

  ToroidalGatherCallback::RegionTab regionsToUpdate;
  static_assert(!ToroidalGatherCallback::RegionTab::canOverflow);
  eastl::array<TMatrix4, ToroidalGatherCallback::RegionTab::static_size> projTMs;

  UniqueTex depthTexture;

  dag::RelocatableFixedVector<struct RiGenVisibility *, ToroidalGatherCallback::RegionTab::static_size> rendinstHeightmapVisibilities;

  ToroidalHelper helper;
  ToroidalHelper nextHelper; // Will be used only when all required lods for RI will be loaded and toroidal texture finally updated
  float texelSize;

  Point3 prevCamPos;

  TMatrix vtm;

  float maxHt = -10000.0f;
};