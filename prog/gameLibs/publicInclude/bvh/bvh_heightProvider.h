//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>

namespace bvh
{

struct HeightProvider
{
  virtual bool embedNormals() const = 0;
  virtual void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count) const = 0;
};

} // namespace bvh