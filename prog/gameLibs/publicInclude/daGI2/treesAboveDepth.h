//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IBBox2.h>
#include <vecmath/dag_vecMath.h>
#include <math/integer/dag_IPoint3.h>
#include <render/toroidalHelper.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/fixed_function.h>
#include <generic/dag_tab.h>

class TreesAboveDepth
{
public:
  typedef eastl::fixed_function<64, void(const BBox3 &box, bool depth)> render_cb;
  void prepare(const Point3 &origin, float minZ, float maxZ, const render_cb &rcb);
  void init(float half_dist, float texel_size);
  void invalidate();
  void invalidateTrees2d(const BBox3 &box, const TMatrix &tm);

  // ToroidalGatherCallback
  void start(const IPoint2 &) {}
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom);
  void end() {}

protected:
  void renderRegionAlpha(IBBox2 &reg);
  void renderRegion(IBBox2 &reg, float texelSize, float minZ, float maxZ, const render_cb &cb, bool depth_min);
  ToroidalHelper trees2dHelper;
  TextureIDHolderWithVar trees2d;
  TextureIDHolderWithVar trees2dDepth, trees2dDepthMin;
  float trees2dDist = 384;
  PostFxRenderer writeDepthToAlpha, clearRegions;
  Tab<IBBox2> regionsToClear, regionsToUpdate;
};
