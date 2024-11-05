// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <generic/dag_relocatableFixedVector.h>
#include "visibility/cullingMath.h"

namespace rendinst
{

struct Cell
{
  float distance = 0.f;
  uint16_t x = 0, z = 0;
  uint16_t rangesStart = 0;
  uint16_t rangesCount = 0; // it is actually only 5 bit, but for the sake of alignment, keep it 16bit
};

struct SubCellRange
{
  uint8_t start = 0;
  uint8_t end = 0;
};

template <int max_visible_cells, int subcell_div>
struct VisibleCells
{
  G_STATIC_ASSERT(subcell_div *subcell_div < eastl::numeric_limits<uint8_t>::max());

  static constexpr int CELL_BBOX_OFFSET = 0;
  static constexpr int SUBCELL_BBOX_OFFSET = 1;
  static constexpr int MAX_VISIBLE_CELLS = max_visible_cells;
  static constexpr int MAX_VISIBLE_SUBCELLS = MAX_VISIBLE_CELLS * 2 * (subcell_div * subcell_div / 2 + 1);

  dag::RelocatableFixedVector<Cell, MAX_VISIBLE_CELLS, false> cells = {};
  dag::RelocatableFixedVector<SubCellRange, MAX_VISIBLE_SUBCELLS, false> subCellRanges = {};
  VisibleCells() {}

  inline void calcInsideVisibility(Cell &cell, const bbox3f *__restrict bbox, Occlusion *occlusion)
  {
    uint8_t startRange = 0;
    uint8_t endRange = subcell_div * subcell_div - 1;
    if (occlusion)
    {
      for (; startRange < subcell_div * subcell_div; ++startRange)
        if (!occlusion->isOccludedBox(bbox[startRange + SUBCELL_BBOX_OFFSET].bmin, bbox[startRange + SUBCELL_BBOX_OFFSET].bmax))
          break;

      for (; endRange > startRange; --endRange)
        if (!occlusion->isOccludedBox(bbox[endRange + SUBCELL_BBOX_OFFSET].bmin, bbox[endRange + SUBCELL_BBOX_OFFSET].bmax))
          break;

      if (endRange < startRange)
        return;
    }

    subCellRanges.emplace_back(SubCellRange{startRange, endRange});
    cell.rangesCount += 1;
  }

  inline void calcIntersectVisibility(Cell &cell, const bbox3f *__restrict bbox, Occlusion *occlusion, const vec4f *__restrict planes,
    int n_planes)
  {
    for (uint8_t range = 0; range < subcell_div * subcell_div; ++range)
    {
      const auto bboxIdx = range + SUBCELL_BBOX_OFFSET;
      if (test_box_planesb(bbox[bboxIdx].bmin, bbox[bboxIdx].bmax, planes, n_planes))
      {
        if (occlusion && occlusion->isOccludedBox(bbox[bboxIdx].bmin, bbox[bboxIdx].bmax))
          continue;
        if (!cell.rangesCount || subCellRanges.back().end != range - 1)
        {
          subCellRanges.emplace_back(SubCellRange{range, range});
          cell.rangesCount++;
        }
        else
          subCellRanges.back().end = range;
      }
    }
  }

  inline void calcCellVisibility(int cell_x, int cell_z, vec3f view_pos, const Frustum &frustum, const bbox3f *__restrict bbox,
    Occlusion *occlusion)
  {
    if (cells.size() >= max_visible_cells)
      return;

    Cell cell{};
    int nPlanes;
    vec4f planes[6];

    int cellIntersection = test_box_planes(bbox[0].bmin, bbox[0].bmax, frustum, planes, nPlanes);

    if (cellIntersection == Frustum::OUTSIDE)
      return;

    if (occlusion && occlusion->isOccludedBox(bbox[CELL_BBOX_OFFSET].bmin, bbox[CELL_BBOX_OFFSET].bmax))
      return;

    cell.distance = v_extract_x(v_sqrt_x(v_distance_sq_to_bbox_x(bbox[0].bmin, bbox[0].bmax, view_pos)));
    cell.rangesStart = subCellRanges.size();

    if (cellIntersection == Frustum::INSIDE) //-V1051
      calcInsideVisibility(cell, bbox, occlusion);
    else
      calcIntersectVisibility(cell, bbox, occlusion, planes, nPlanes);

    if (cell.rangesCount != 0)
    {
      cell.x = cell_x;
      cell.z = cell_z;
      cells.emplace_back(eastl::move(cell));
    }
  }
};

} // namespace rendinst
