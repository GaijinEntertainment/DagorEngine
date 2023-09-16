//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
    d3d::driver_command( DRV3D_COMMAND_OVERRIDE_MAX_ANISOTROPY_LEVEL, one, NULL, NULL );    //-V566
  }
  void renderStaticShadowDepth(const TMatrix4 & viewproj)
  {
  //==
  }
  void endRenderStaticShadow()
  {
    shaders::reset_override();
    d3d::driver_command( DRV3D_COMMAND_OVERRIDE_MAX_ANISOTROPY_LEVEL, (void *)0, NULL, NULL );
  }
}*/
