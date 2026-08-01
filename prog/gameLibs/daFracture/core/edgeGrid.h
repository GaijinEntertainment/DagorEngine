// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_span.h>


namespace frx
{

// 2D uniform edge (segment) grid: built once, queried many times
struct EdgeGrid
{
  struct Cell
  {
    int offset = 0, cnt = 0;
  };
  dag::Vector<Cell, framemem_allocator> cells;
  dag::Vector<int, framemem_allocator> cellEdges;

  struct CellBBox
  {
    int x0, y0, x1, y1;
    int cellCount() const { return (x1 - x0 + 1) * (y1 - y0 + 1); }
    bool overlaps(const CellBBox &o) const { return !(x1 < o.x0 || o.x1 < x0 || y1 < o.y0 || o.y1 < y0); }
  };
  dag::Vector<CellBBox, framemem_allocator> edgeBBoxes; // per-edge, raw (pre-wrap) cell coords
  dag::Vector<int, framemem_allocator> hugeEdges;       // ids of edges with cellCount > MAX_CELLS_PER_EDGE

  uint32_t sizeLog2 = 0;
  float scaleToGrid = 1.f;

  static constexpr int MAX_CELLS_PER_EDGE = 4;

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

  __forceinline bool isHuge(int edge_id) const { return edgeBBoxes[edge_id].cellCount() > MAX_CELLS_PER_EDGE; }

  // get_bbox: callable (int edge_id) -> BBox2 in world space
  template <typename GetBBoxFn>
  void build(int n_edges, uint32_t size_log_2, float cell_size, GetBBoxFn &&get_bbox)
  {
    G_ASSERT(size_log_2 > 0 && cell_size > 0.f);
    sizeLog2 = size_log_2;
    scaleToGrid = 1.f / cell_size;

    const uint32_t cellCnt = 1u << (sizeLog2 * 2u);
    cells.clear();
    cells.resize(cellCnt);
    edgeBBoxes.resize(n_edges);
    hugeEdges.clear();

    for (int ei = 0; ei < n_edges; ei++)
    {
      const BBox2 bb = get_bbox(ei);
      int x0, y0, x1, y1;
      getCellXY(bb[0], x0, y0);
      getCellXY(bb[1], x1, y1);
      edgeBBoxes[ei] = {x0, y0, x1, y1};

      if (edgeBBoxes[ei].cellCount() > MAX_CELLS_PER_EDGE)
        hugeEdges.push_back(ei);
      else
        for (int y = y0; y <= y1; y++)
          for (int x = x0; x <= x1; x++)
            cells[getCellIdx(x, y)].cnt++;
    }

    int offset = 0;
    for (auto &cell : cells)
    {
      cell.offset = offset;
      offset += cell.cnt;
      cell.cnt = 0;
    }
    cellEdges.resize(offset);

    for (int ei = 0; ei < n_edges; ei++)
    {
      const auto &bb = edgeBBoxes[ei];
      if (bb.cellCount() > MAX_CELLS_PER_EDGE)
        continue;
      for (int y = bb.y0; y <= bb.y1; y++)
        for (int x = bb.x0; x <= bb.x1; x++)
        {
          auto &cell = cells[getCellIdx(x, y)];
          cellEdges[cell.offset + cell.cnt++] = ei;
        }
    }
  }

  template <typename Fn>
  DAGOR_NOINLINE void iteratePotentialPairs(Fn &&fn) const
  {
#if 0
    for (int ei = 0, ne = int(edgeBBoxes.size()); ei < ne; ei++)
      for (int ej = ei + 1; ej < ne; ej++)
        fn(ei, ej);
    return;
#endif

    // Regular x Regular
    for (int cidx = 0, ie = int(cells.size()); cidx < ie; cidx++)
    {
      const auto &cell = cells[cidx];
      if (cell.cnt < 2)
        continue;
      for (int i = 0; i < cell.cnt; i++)
      {
        const int e1 = cellEdges[cell.offset + i];
        const auto &b1 = edgeBBoxes[e1];
        for (int j = i + 1; j < cell.cnt; j++)
        {
          const int e2 = cellEdges[cell.offset + j];
          const auto &b2 = edgeBBoxes[e2];
          const int ix = eastl::max(b1.x0, b2.x0);
          const int iy = eastl::max(b1.y0, b2.y0);
          if (int(getCellIdx(ix, iy)) == cidx) // dedup
            fn(e1, e2);
        }
      }
    }

    // Regular x Huge
    for (int ei = 0, ne = int(edgeBBoxes.size()); ei < ne; ei++)
    {
      if (isHuge(ei))
        continue;
      for (int h : hugeEdges)
        if (edgeBBoxes[ei].overlaps(edgeBBoxes[h]))
          fn(eastl::min(ei, h), eastl::max(ei, h));
    }

    // Huge x Huge
    for (int i = 0, nh = int(hugeEdges.size()); i < nh; i++)
      for (int j = i + 1; j < nh; j++)
      {
        const int h1 = hugeEdges[i];
        const int h2 = hugeEdges[j];
        if (edgeBBoxes[h1].overlaps(edgeBBoxes[h2]))
          fn(h1, h2); // hugeEdges is filled in ascending edge-id order, so h1 < h2
      }
  }
};

} // namespace frx