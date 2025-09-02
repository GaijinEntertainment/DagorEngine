// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <math/dag_bounds3.h>

class GpuDeformObjectsManager
{
public:
  void init(uint32_t width, float cell);
  void close();
  void setOrigin(const Point3 &origin);
  void updateDeforms();

  Sbuffer *getIndicesBuf() const { return indices.getBuf(); }
  Sbuffer *getDeformCBuf() const { return deformsCB.getBuf(); }

protected:
  uint32_t width = 0, maxIndices = 0;
  float cellSize = 1;
  UniqueBufHolder indices, deformsCB;
  void setEmpty() { empty = true; }
  bool isEmpty() const { return empty; }
  void fillEmpty();
  void initIndicesBuffer(uint32_t max_indices);

  Point3 origin;
  float maxDistPreCheckSq = 1e9f;
  bool empty = false;
};
