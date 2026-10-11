//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds2.h>
#include <3d/dag_resPtr.h>
#include <EASTL/unique_ptr.h>

class ComputeShaderElement;

// toroidal grid of per cell max heights around the viewer over tex_hmap_low;
// heightmap_max_grid.dshl marches it to complete a trace with the terrain.
// Content is position derived only: any window jump refills, no history.
// The owner calls close() after a device reset and after an in place
// heightfield change, like the heightmap handler's own recovery, or
// invalidateBox() when it knows the box of the change
class HeightmapMaxGrid
{
public:
  ~HeightmapMaxGrid();
  // cells must be a power of two; the window spans cells*cell_size meters.
  // Re-init with the same arguments is free
  bool init(uint32_t cells, float cell_size);
  void close();
  // toroidal update around the position, cheap when nothing scrolled
  void update(const Point3 &view_pos);
  // the heightfield changed in place under the world xz box: the next update() refills its cells, not the window;
  // on a mirrored map a change near its edge also changes cells past the edge, which only close() refills
  void invalidateBox(const BBox2 &box) { pending += box; }

private:
  void dispatchRect(const IPoint2 &rect_lt, const IPoint2 &rect_size);
  static constexpr int INVALID_COORD = -0x40000000;
  BBox2 pending;
  UniqueBufWithShaderVar buf;
  eastl::unique_ptr<ComputeShaderElement> updateCs;
  IPoint2 lt{INVALID_COORD, INVALID_COORD};
  uint32_t refreshRow = 0;
  uint32_t cells = 0;
  float cellSize = 8.f;
};
