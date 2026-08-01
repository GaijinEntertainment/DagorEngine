// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <generic/dag_span.h>


namespace frx
{

// 2D uniform vertex grid: built once, queried many times
struct VertexGrid
{
  struct Cell
  {
    int offset = 0, cnt = 0;
  };
  dag::Vector<Cell, framemem_allocator> cells;
  dag::Vector<int, framemem_allocator> verts;

  uint32_t sizeLog2 = 0;
  float scaleToGrid = 1.f;

  __forceinline uint32_t getCellIdx(int x, int y) const
  {
    const uint32_t mask = (1u << sizeLog2) - 1u;
    return ((uint32_t(y) & mask) << sizeLog2) | (uint32_t(x) & mask);
  }

  __forceinline void getCellXY(Point2 p, int &x, int &y) const
  {
    x = int(floorf(p.x * scaleToGrid));
    y = int(floorf(p.y * scaleToGrid));
  }

  void build(dag::ConstSpan<Point2> world_verts, uint32_t size_log_2, float cell_size)
  {
    G_ASSERT(size_log_2 > 0 && cell_size > 0.f);
    sizeLog2 = size_log_2;
    scaleToGrid = 1.f / cell_size;

    const uint32_t cellCnt = 1u << (sizeLog2 * 2u);
    cells.clear();
    cells.resize(cellCnt);
    verts.resize(world_verts.size());

    for (const auto &v : world_verts)
    {
      int x, y;
      getCellXY(v, x, y);
      cells[getCellIdx(x, y)].cnt++;
    }

    int offset = 0;
    for (auto &cell : cells)
    {
      cell.offset = offset;
      offset += cell.cnt;
      cell.cnt = 0;
    }

    for (int i = 0, ie = int(world_verts.size()); i < ie; i++)
    {
      int x, y;
      getCellXY(world_verts[i], x, y);
      auto &cell = cells[getCellIdx(x, y)];
      verts[cell.offset + cell.cnt++] = i;
    }
  }

  template <typename Fn>
  __forceinline void queryBBox(const BBox2 &bb, Fn &&fn) const
  {
    int x0, y0, x1, y1;
    getCellXY(bb[0], x0, y0);
    getCellXY(bb[1], x1, y1);
    if (DAGOR_LIKELY(x0 == x1 && y0 == y1))
    {
      const auto &cell = cells[getCellIdx(x0, y0)];
      for (int i = 0; i < cell.cnt; i++)
        fn(verts[cell.offset + i]);
      return;
    }
    for (int y = y0; y <= y1; y++)
      for (int x = x0; x <= x1; x++)
      {
        const auto &cell = cells[getCellIdx(x, y)];
        for (int i = 0; i < cell.cnt; i++)
          fn(verts[cell.offset + i]);
      }
  }
};

} // namespace frx