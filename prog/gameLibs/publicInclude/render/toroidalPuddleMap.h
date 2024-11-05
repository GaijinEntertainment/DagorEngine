//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_color.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_consts.h>
#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/array.h>
#include <generic/dag_tab.h>

class Point2;
class Point3;
struct Color4;

class ToroidalPuddlesRenderer
{
public:
  virtual void startRenderTiles(const Point2 &center) = 0;
  virtual void renderTile(const BBox2 &region) = 0;
  virtual void endRenderTiles() = 0;
};

class ToroidalPuddles
{
public:
  ~ToroidalPuddles() { close(); }
  void close();
  void init(int puddles_size = 2048, float near_lod_size = 64.0f, float far_lod_size = 192.0f, int treshold = 128,
    E3DCOLOR clear_value = 0);

  int getBufferSize();

  ToroidalPuddles() : puddlesCacheSize(1024), pixelTreshold(128), invalidated(false), flushRegions(false), clearValue(0) {}

  void updatePuddles(ToroidalPuddlesRenderer &renderer, const Point3 &camera_position, float max_move = 0.5f);
  void invalidateBox(const BBox2 box, bool flush_regions = false);
  void invalidate();

protected:
  static constexpr int LOD_COUNT = 2;
  struct PerLodData
  {
    ToroidalHelper torHelper;
    float puddlesRadius, worldSize, texelSize;
    Color4 worldToToroidal;
    Point2 uvOffset;
    ToroidalGatherCallback::RegionTab regions;
    Tab<ToroidalQuadRegion> quadRegions;
    Tab<IBBox2> invalidRegions;
  };
  eastl::array<PerLodData, LOD_COUNT> lodData;

  void updateTile(ToroidalPuddlesRenderer &renderer, const ToroidalQuadRegion &reg, float texel_size);

  void addRegionToRender(const ToroidalQuadRegion &reg, int cascade);

  int puddlesCacheSize;
  int pixelTreshold;
  bool invalidated;
  bool flushRegions;

  E3DCOLOR clearValue;

  UniqueTexHolder toroidalPuddles;
};
