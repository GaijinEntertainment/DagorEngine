//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resourcePool.h>
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

  void setParams(const FoamFxParams &params);

  void prepare(const TMatrix4 &view_tm, const TMatrix4 &proj_tm);
  void renderHeight();
  void renderFoam();

  void beginMaskRender();
  void endMaskRender();

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

  shaders::OverrideStateId zFuncLeqStateId;
};
