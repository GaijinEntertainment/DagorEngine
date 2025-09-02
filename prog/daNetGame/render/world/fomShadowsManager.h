// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <math/dag_math3d.h>


// Fourier opacity mapping shadows
class FomShadowsManager
{
public:
  FomShadowsManager(int size, float z_distance, float xy_distance, float height);
  ~FomShadowsManager();

  void prepareFOMShadows(const TMatrix &itm, float worldWaterLevel, float worldMinimumHeight, Point3 dirToSun);

private:
  const int shadowMapSize;
  const float zDistance;
  const float xyViewBoxSize;
  const float zViewBoxSize;

  TMatrix4 fomProj;
  TMatrix fomView;
  Point3 fomViewPos;

  dafg::NodeHandle fomRenderingNode;
};
