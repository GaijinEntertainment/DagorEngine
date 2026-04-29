//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_shaders.h>
#include <EASTL/unique_ptr.h>

#include "math/dag_bounds3.h"


class DebugCollisionDensityRenderer
{
  BufPtr debugCollisionSB, debugCollisionCounterSB;
  eastl::unique_ptr<ShaderMaterial> drawFacesCountMat;
  ShaderElement *drawFacesCountElem = 0;
  ShaderVariableInfo commonBufferReg{"debug_buffer_reg_no"};

  int debugVoxelsCount = 0;
  int lastThreshold = -1;
  bool lastRasterCollision = false;
  bool lastRasterPhys = false;
  BBox3 worldBox;

  void fillCollisionDensity(int threshold, const Point3 &cell, bool raster_collision, bool raster_phys);
  void init();
  void close();

public:
  DebugCollisionDensityRenderer();
  ~DebugCollisionDensityRenderer();

  void prepare_render(const BBox3 &world_box, int threshold, bool raster_collision, bool raster_phys);
  void render();
};
