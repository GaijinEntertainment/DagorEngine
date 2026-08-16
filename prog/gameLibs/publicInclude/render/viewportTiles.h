//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_viewScissor.h>
#include <EASTL/functional.h>
#include <debug/dag_assert.h>

using ViewportTileCallback = eastl::function<void(int tile_x, int tile_y, int tile_w, int tile_h, int index)>;

inline void for_each_viewport_tile(int grid_cols, int grid_rows, bool border_only, const ViewportTileCallback &callback)
{
  G_ASSERT(grid_cols > 0 && grid_rows > 0);
  int vx, vy, vw, vh;
  float zn, zf;
  d3d::getview(vx, vy, vw, vh, zn, zf);

  const int tile_w = vw / grid_cols;
  const int tile_h = vh / grid_rows;

  int displayed_index = 0;
  for (int row = 0; row < grid_rows; ++row)
  {
    for (int col = 0; col < grid_cols; ++col)
    {
      if (border_only && col > 0 && col < grid_cols - 1 && row > 0 && row < grid_rows - 1)
        continue;

      const int tile_x = vx + col * tile_w;
      const int tile_y = vy + row * tile_h;

      d3d::setview(tile_x, tile_y, tile_w, tile_h, zn, zf);
      callback(tile_x, tile_y, tile_w, tile_h, displayed_index);
      ++displayed_index;
    }
  }

  d3d::setview(vx, vy, vw, vh, zn, zf);
}
