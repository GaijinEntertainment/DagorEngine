// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "math/dag_color.h"
#include <3d/dag_resPtr.h>
#include <math/dag_bounds3.h>
#include <EASTL/array.h>

class GpuDeformObjectsManager
{
public:
  void init(uint32_t width, float cell);
  void close();
  void setOrigin(const Point3 &origin);
  void updateDeforms();

  Sbuffer *getIndicesBuf() const { return indices[activeDeformsCBIndex].getBuf(); }
  Sbuffer *getDeformCBuf() const { return deformsCB[activeDeformsCBIndex].getBuf(); }

protected:
  constexpr static int OBSTACLE_BUFFER_COUNT = 2;

  uint32_t width = 0, maxIndices = 0;
  float cellSize = 1;
  uint32_t activeDeformsCBIndex = 0;
  eastl::array<UniqueBuf, OBSTACLE_BUFFER_COUNT> indices, deformsCB;
  void setEmpty() { empty = true; }
  bool isEmpty() const { return empty; }
  void fillEmpty();
  void initIndicesBuffer(uint32_t max_indices);

  Point3 origin;
  float maxDistPreCheckSq = 1e9f;
  bool empty = false;
};
