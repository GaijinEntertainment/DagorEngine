//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <EASTL/vector.h>
#include <memort/dag_framemem.h>

// warning: compute is almost always pereferrable and faster, although allocates gpu and requires assumpion on count
//  see fill_windows_grid_cs in enlisted as example.

// nodes are in inverse matices
template <class Grid, bool test_precise = false>
static void fill_clustered_grid(const bbox3f *__restrict boxes, const mat44f *__restrict nodes_itm, int nodes_cnt, Grid &grid,
  const IPoint3 &grid_res, uint32_t offset_bits, uint32_t max_cluster_count, const vec4f lt_cellSize)
{
  TIME_PROFILE(fill_clustered_grid);
  G_ASSERT(max_cluster_count < (1 << (32 - offset_bits)));
  G_UNUSED(nodes);
  const vec4f lt = lt_cellSize;
  const vec4f cellSizeV = v_splat_w(lt_cellSize);
  const vec4f gridMin1 = v_make_vec4f(grid_res.x - 1, grid_res.y - 1, grid_res.z - 1, 0);

  auto voxelizeNodes = [&](auto Call) {
    bbox3f unitaryBox;
    unitaryBox.bmax = V_C_HALF;
    unitaryBox.bmin = v_neg(unitaryBox.bmax);
    for (int i = 0, ei = nodes_cnt; i < ei; ++i)
    {
      vec3f pMin = v_max(v_floor(v_div(v_sub(boxes[i].bmin, lt), cellSizeV)), v_zero());
      vec3f pMax = v_min(v_floor(v_div(v_sub(boxes[i].bmax, lt), cellSizeV)), gridMin1);
      if (v_signmask(v_cmp_gt(pMin, pMax)) & 7)
        continue;
      alignas(16) int imin[4], imax[4];
      v_sti(imin, v_cvt_vec4i(pMin));
      v_sti(imax, v_cvt_vec4i(pMax));
      vec4f ltNode;
      mat44f m44;
      if (test_precise)
        m44 = nodes_itm[i];
      for (int z = imin[2]; z <= imax[2]; ++z)
        for (int y = imin[1]; y <= imax[1]; ++y)
          for (int x = imin[0]; x <= imax[0]; ++x)
          {
            if (test_precise)
            {
              bbox3f cellBox;
              cellBox.bmin = v_madd(v_make_vec4f(x, y, z, 0), cellSizeV, lt);
              cellBox.bmax = v_add(cellBox.bmin, cellSizeV);
              v_bbox3_init(cellBox, m44, cellBox);
              if (!v_bbox3_test_box_intersect(cellBox, unitaryBox))
                continue;
            }
            Call(x, y, z, i);
          }
    }
  };

  const int gridListOffset = grid_res.x * grid_res.y * grid_res.z;
  const int gridXYSlice = grid_res.x * grid_res.y;
  eastl::vector<uint32_t, framemem_allocator> gridOffset(gridListOffset, 0);
  voxelizeNodes([&](int x, int y, int z, int) { gridOffset[z * gridXYSlice + y * grid_res.x + x]++; });
  uint32_t totalCount = 0, maxCount = 0;
  for (int i = 0, ei = gridOffset.size(); i < ei; ++i)
  {
    uint32_t cnt = min(gridOffset[i], max_cluster_count);
    maxCount = max(maxCount, cnt);
    gridOffset[i] = totalCount;
    totalCount += cnt;
  }
  grid.resize(0);
  grid.assign(gridOffset.size() + totalCount, 0);

  voxelizeNodes([&](int x, int y, int z, int ni) {
    uint32_t &gridCurCount = grid[z * gridXYSlice + y * grid_res.x + x];
    if (gridCurCount >= max_cluster_count)
      return;
    grid[gridListOffset + gridCurCount + gridOffset[z * gridXYSlice + y * grid_res.x + x]] = ni;
    gridCurCount++;
  });

  for (int i = 0, ei = gridOffset.size(); i < ei; ++i)
  {
    G_ASSERT(grid[i] <= max_cluster_count);
    G_ASSERT(gridOffset[i] < (1 << (offset_bits)));
    if (grid[i])
      grid[i] = gridOffset[i] | (grid[i] << offset_bits);
  }
}
