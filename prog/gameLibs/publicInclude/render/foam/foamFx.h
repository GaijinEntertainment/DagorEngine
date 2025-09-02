//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <resourcePool/resourcePool.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_overrideStates.h>

struct FoamFxParams
{
  Point3 scale = Point3(0.05f, 0.75f, 0.1f);
  Point3 gamma = Point3(2.2f, 2.2f, 2.2f);
  Point2 threshold = Point2(0.25f, 0.1f);
  Point2 weight = Point2(0.2f, 1.0f);
  Point3 underfoamColor = Point3(1.0f, 1.0f, 1.0f);
  Point3 overfoamColor = Point3(1.0f, 1.0f, 1.0f);
  float reflectivity = 0.1f;
  String tileTex;
  String gradientTex;
};

class FoamFx
{
public:
  FoamFx(int _width, int _height);
  ~FoamFx();

  static FoamFxParams prepareParams(float tile_uv_scale, float distortion_scale, float normal_scale, float pattern_gamma,
    float mask_gamma, float gradient_gamma, float underfoam_threshold, float overfoam_threshold, float underfoam_weight,
    float overfoam_weight, const Point3 &underfoam_color, const Point3 &overfoam_color, float reflectivity, const String &tile_tex,
    const String &gradient_tex);
  void setParams(const FoamFxParams &params);

  void prepare(const TMatrix4 &view_tm, const TMatrix4 &proj_tm);
  void renderHeight();
  void renderFoam();

  void beginMaskRender();

private:
  int width, height;

  SharedTexHolder foamGeneratorTileTex;
  SharedTexHolder foamGeneratorGradientTex;

  UniqueTex maskTarget;
  UniqueTex maskDepth;

  RTargetPool::Ptr fullTargetPool;
  RTargetPool::Ptr downTargetPool;
  RTargetPool::Ptr tempTargetPool;

  PostFxRenderer foamPrepare;
  PostFxRenderer foamCombine;
  PostFxRenderer foamHeight;
  PostFxRenderer foamBlurX, foamBlurY;

  RTarget::Ptr overfoam;
  RTarget::Ptr underfoam_downsampled;
};
