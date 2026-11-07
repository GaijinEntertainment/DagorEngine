// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/snap.h"
#include "mesh/solidUtil.h"
#include <math/dag_check_nan.h>

namespace dacsg::exact
{
static bool make_grid_frame(const BBoxD &box, int grid_bits, GridFrame &f, eastl::string &err)
{
  if (grid_bits < 1 || grid_bits > MAX_GRID_BITS)
  {
    err.sprintf("gridBits %d is outside 1..%d", grid_bits, MAX_GRID_BITS);
    return false;
  }
  double extent = 0;
  for (int i = 0; i < 3; ++i)
    extent = box.lim[1][i] - box.lim[0][i] > extent ? box.lim[1][i] - box.lim[0][i] : extent;
  if (!check_finite(extent) || extent <= 0)
  {
    err = "the model is empty or not finite";
    return false;
  }
  int e;
  const double m = frexp(extent, &e);
  f.scale = ldexp(1.0, grid_bits - (m == 0.5 ? e - 1 : e));
  for (int i = 0; i < 3; ++i)
  {
    f.center[i] = round((box.lim[0][i] + box.lim[1][i]) * 0.5 * f.scale) / f.scale;
    if (!check_finite(f.center[i]))
    {
      err = "model too small for the grid: its extent is too small for its distance from the origin";
      return false;
    }
  }
  return true;
}

static GridPoint snap_point(const GridFrame &f, const DPoint3 &p)
{
  GridPoint g;
  for (int i = 0; i < 3; ++i)
    g.c[i] = int64_t(round((p[i] - f.center[i]) * f.scale));
  return g;
}

DPoint3 grid_to_world(const GridFrame &f, const DPoint3 &g) { return g * (1.0 / f.scale) + f.center; }

bool snap_leaves(dag::ConstSpan<Solid> leaves, int grid_bits, GridFrame &f, dag::Vector<dag::Vector<GridPoint>> &out,
  eastl::string &err)
{
  BBoxD box;
  for (const Solid &s : leaves)
    add_bbox(box, points_bbox(s.pos));
  if (!make_grid_frame(box, grid_bits, f, err))
    return false;
  out.resize(leaves.size());
  for (uint32_t l = 0; l < leaves.size(); ++l)
  {
    const Solid &s = leaves[l];
    dag::Vector<GridPoint> &g = out[l];
    g.resize(s.vertCount());
    for (uint32_t v = 0; v < s.vertCount(); ++v)
      g[v] = snap_point(f, s.pos[v]);
  }
  return true;
}
} // namespace dacsg::exact
