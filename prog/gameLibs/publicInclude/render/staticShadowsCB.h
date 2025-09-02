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
  virtual void startRenderStaticShadow(const TMatrix &view, const Point2 &zn_zf, float minHt, float maxHt) = 0;
  virtual bool readyToRenderRegionWithFinalQuality(const ViewTransformData & /*curTransform*/, int /*region*/) { return true; }
  // toroidal: culling_view_proj has twice the zfar size, then globtm
  // scrolled: zNear plane may be extended to reduce shadow popping
  virtual void renderStaticShadowDepth(const mat44f &culling_view_proj, const ViewTransformData &curTransform, int region) = 0;
  virtual void endRenderStaticShadow() = 0;
};


// sample
/*class MyStaticShadowClient: public IStaticShadowsCB
{
public:
  void startRenderStaticShadow( const TMatrix & view, const Point2 & znzf, float targetHt, float minHt, float maxHt)
  {

    shaders::set(overrideWithZBiasAndDepthClamp);
    char *one = 0; one++;
    d3d::driver_command( Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, one );    //-V566
  }
  void renderStaticShadowDepth(const TMatrix4 & viewproj)
  {
  //==
  }
  void endRenderStaticShadow()
  {
    shaders::reset_override();
    d3d::driver_command( Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL );
  }
}*/
