//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>


struct HeightmapSmoothCell
{
  Point2 nXz[5];
  Point3 mid;
  real h[4]; // 0, y, xy, x;
};


// Get cells from heightmap with normals at each point
//
// Class HM must implement following methods:
// real getHeightmapCellSize();
// bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid);
template <class HM>
void get_smooth_faces_from_midpoint_heightmap(HM &heightmap, Tab<HeightmapSmoothCell> &cells, real &half_cell_sz, real &norm_y,
  const BBox3 &box)
{
  if (box.isempty())
    return;

  const Point3 ofs = heightmap.getHeightmapOffset();
  real cellSize = heightmap.getHeightmapCellSize();
  real halfCellSize = cellSize * 0.5f;

  int x0 = int(floorf((box[0].x - ofs.x) / cellSize));
  int x1 = int(floorf((box[1].x - ofs.x) / cellSize));
  int y0 = int(floorf((box[0].z - ofs.z) / cellSize));
  int y1 = int(floorf((box[1].z - ofs.z) / cellSize));

  HeightmapSmoothCell cell;
  half_cell_sz = halfCellSize;
  norm_y = 2 * cellSize;

  for (int my = y0; my <= y1; ++my)
  {
    for (int mx = x0; mx <= x1; ++mx)
    {
      real hmid;

      if (!heightmap.getHeightmapCell5Pt(IPoint2(mx, my), cell.h[0], cell.h[3], cell.h[1], cell.h[2], hmid))
        continue;

      // calc cell max Y
      real maxY = cell.h[0];
      if (cell.h[3] > maxY)
        maxY = cell.h[3];
      if (cell.h[1] > maxY)
        maxY = cell.h[1];
      if (cell.h[2] > maxY)
        maxY = cell.h[2];
      if (hmid > maxY)
        maxY = hmid;

      if (box[0].y - ofs.y > maxY)
        continue;

      // calc cell min Y
      real minY = cell.h[0];
      if (cell.h[3] < minY)
        minY = cell.h[3];
      if (cell.h[1] < minY)
        minY = cell.h[1];
      if (cell.h[2] < minY)
        minY = cell.h[2];
      if (hmid < minY)
        minY = hmid;

      if (box[1].y - ofs.y < minY)
        continue;

      heightmap.getHeightmapCell4Norm(IPoint2(mx, my), cell.h, cell.nXz);

      // add triangles
      cell.mid = Point3((mx + 0.5f) * cellSize + ofs.x, hmid + ofs.y, (my + 0.5f) * cellSize + ofs.z);
      cell.h[0] += ofs.y;
      cell.h[1] += ofs.y;
      cell.h[2] += ofs.y;
      cell.h[3] += ofs.y;
      cell.nXz[4] = Point2(cell.h[0] + cell.h[1] - cell.h[2] - cell.h[3], cell.h[0] + cell.h[3] - cell.h[2] - cell.h[1]);
      cells.push_back(cell);
    }
  }
}
