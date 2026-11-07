// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "kernelExact/exactPoint.h"
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/string.h>

namespace dacsg::exact
{
// grid = round((world - center) * scale); scale is a power of two, center a multiple of 1 / scale
struct GridFrame
{
  DPoint3 center = DPoint3(0, 0, 0);
  double scale = 1;
};

DPoint3 grid_to_world(const GridFrame &f, const DPoint3 &g);
// out[leaf][vertex]
bool snap_leaves(dag::ConstSpan<Solid> leaves, int grid_bits, GridFrame &f, dag::Vector<dag::Vector<GridPoint>> &out,
  eastl::string &err);
} // namespace dacsg::exact
