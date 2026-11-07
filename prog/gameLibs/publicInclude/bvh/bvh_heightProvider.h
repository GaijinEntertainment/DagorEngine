//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <util/dag_stdint.h>

namespace bvh
{

struct HeightProvider
{
  virtual bool embedNormals() const = 0;
  virtual void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count) const = 0;
  // update_terrain rebuilds every lod when this changes
  virtual uint32_t getVersion() const { return 0; }
};

} // namespace bvh