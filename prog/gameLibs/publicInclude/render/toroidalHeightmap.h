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
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>

class Point2;
class Point3;
struct Color4;

class ToroidalHeightmapRenderer
{
public:
  virtual void startRenderTiles(const Point2 &center) = 0;
  virtual void renderTile(const BBox2 &region) = 0;
  virtual void endRenderTiles() = 0;
};

class ToroidalHeightmap
{
public:
  ~ToroidalHeightmap() { close(); }
  void close();
  void init(int heightmap_size = 2048, float near_lod_size = 32.0f, float far_lod_size = 96.0f, uint32_t format = TEXFMT_L8,
    int treshold = 128, E3DCOLOR clear_value = 0);

  void setHeightmapTex();
  void setBlackTex(TEXTUREID black_tex_array);
  void setTexFilter(d3d::FilterMode filter);
  Point2 getWorldSize();
  int getBufferSize();

  ToroidalHeightmap() :
    heightmapCacheSize(1024),
    heightmapSizeLod0(32.0f),
    heightmapSizeLod1(96.0f),
    pixelTreshold(128),
    toroidalClipmap_world2uv_1VarId(-1),
    toroidalClipmap_world2uv_2VarId(-1),
    toroidalClipmap_world_offsetsVarId(-1),
    toroidalHeightmapSamplerVarId(-1),
    invalidated(false),
    flushRegions(false),
    clearValue(0)
  {}

  void updateHeightmap(ToroidalHeightmapRenderer &renderer, const Point3 &camera_position, float min_texel_size = 0.0f,
    float max_move = 0.5f);
  void invalidateBox(const BBox2 box, bool flush_regions = false);
  void invalidate();

protected:
  void updateTile(ToroidalHeightmapRenderer &renderer, const ToroidalQuadRegion &reg, float texel_size);

  void addRegionToRender(const ToroidalQuadRegion &reg, int cascade);

  int heightmapCacheSize;
  int pixelTreshold;
  float heightmapSizeLod0, heightmapSizeLod1;
  bool invalidated;
  bool flushRegions;
  int toroidalClipmap_world2uv_1VarId, toroidalClipmap_world2uv_2VarId, toroidalClipmap_world_offsetsVarId;
  int toroidalHeightmapSamplerVarId;
  static constexpr int LOD_COUNT = 2;
  E3DCOLOR clearValue;

  ToroidalHelper torHelpers[LOD_COUNT];

  Color4 worldToToroidal[LOD_COUNT];
  Point2 uvOffset[LOD_COUNT];

  ToroidalGatherCallback::RegionTab regions[LOD_COUNT];
  Tab<ToroidalQuadRegion> quadRegions[LOD_COUNT];
  Tab<IBBox2> invalidRegions[LOD_COUNT];

  UniqueTexHolder toroidalHeightmap;
  d3d::SamplerInfo heightmapSampler = {};
};
