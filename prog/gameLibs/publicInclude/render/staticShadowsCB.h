//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class TMatrix;
class Point2;
struct mat44f;
struct ViewTransformData;

class IStaticShadowsCB
{
public:
  enum class RasterizedResult
  {
    NothingRendered,
    SomethingRendered
  };

  virtual void startRenderStaticShadow(const TMatrix &view, const Point2 &zn_zf, float minHt, float maxHt) = 0;
  virtual bool readyToRenderRegionWithFinalQuality(const ViewTransformData & /*curTransform*/, int /*region*/) { return true; }
  // toroidal: culling_view_proj has twice the zfar size, then globtm
  // scrolled: zNear plane may be extended to reduce shadow popping
  virtual RasterizedResult renderStaticShadowDepth(const mat44f &culling_view_proj, const ViewTransformData &curTransform,
    int region) = 0;
  virtual void endRenderStaticShadow() = 0;
};
