//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// this can be make to be universal planar shadows (not only rendinst), but it requires implementing callbacks and other refactor
class BaseTexture;
typedef BaseTexture Texture;
class BBox2;

#include <math/dag_Point2.h>
#include <3d/dag_resPtr.h>
#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>


class ClipmapShadow
{
protected:
  int clipmapShadowSize;
  UniqueTex clipmapShadowTex;

  static constexpr int NUM_CLIPMAP_SHADOW_CASCADES = 2;

  int worldToClipmapShadowVarId[NUM_CLIPMAP_SHADOW_CASCADES];
  float clipmapShadowWorldSize[NUM_CLIPMAP_SHADOW_CASCADES];

  ToroidalHelper torHelpers[NUM_CLIPMAP_SHADOW_CASCADES];

  Color4 worldToToroidal[NUM_CLIPMAP_SHADOW_CASCADES];
  Point2 uvOffset[NUM_CLIPMAP_SHADOW_CASCADES];

  ToroidalGatherCallback::RegionTab regions[NUM_CLIPMAP_SHADOW_CASCADES];
  Tab<ToroidalQuadRegion> quadRegions[NUM_CLIPMAP_SHADOW_CASCADES];
  void setUpSampler() const;

public:
  ClipmapShadow() {}
  ~ClipmapShadow() { close(); }
  void init(int textureSize);
  void setDistance(float distance, float delta, float near_cascade_scale);
  void close();
  bool update(float min_height, float max_height, const Point3 &view_pos);
  void switchOff();
  void reset();
  void invalidate();
  bool getBBox(BBox2 &box) const;
};
