//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_tab.h>


class HeightmapTriangle
{
public:
  Point3 v0;     // vertex
  Point3 e1, e2; // edges to other vertices


  HeightmapTriangle() {}

  HeightmapTriangle(const Point3 &vert, const Point3 &edge1, const Point3 &edge2) : v0(vert), e1(edge1), e2(edge2) {}
};


// Get faces from heightmap with cells represented as 4-triangle 5-point geometry,
// with 4 points at grid vertices and 1 at the cell center.
//
// Class HM must implement following methods:
// real getHeightmapCellSize();
// bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid);
template <class HM>
void get_faces_from_midpoint_heightmap(HM &heightmap, Tab<HeightmapTriangle> &faces, const BBox3 &box)
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

  for (int my = y0; my <= y1; ++my)
  {
    for (int mx = x0; mx <= x1; ++mx)
    {
      real h0, hx, hy, hxy, hmid;

      if (!heightmap.getHeightmapCell5Pt(IPoint2(mx, my), h0, hx, hy, hxy, hmid))
        continue;

      // calc cell max Y
      real maxY = h0;
      if (hx > maxY)
        maxY = hx;
      if (hy > maxY)
        maxY = hy;
      if (hxy > maxY)
        maxY = hxy;
      if (hmid > maxY)
        maxY = hmid;

      if (box[0].y - ofs.y > maxY)
        continue;

      // calc cell min Y
      real minY = h0;
      if (hx < minY)
        minY = hx;
      if (hy < minY)
        minY = hy;
      if (hxy < minY)
        minY = hxy;
      if (hmid < minY)
        minY = hmid;

      if (box[1].y - ofs.y < minY)
        continue;

      // add triangles
      Point3 midPt((mx + 0.5f) * cellSize + ofs.x, hmid + ofs.y, (my + 0.5f) * cellSize + ofs.z);

      Point3 edge0(-halfCellSize, h0 - hmid, -halfCellSize);
      Point3 edge1(-halfCellSize, hy - hmid, +halfCellSize);
      Point3 edge2(+halfCellSize, hxy - hmid, +halfCellSize);
      Point3 edge3(+halfCellSize, hx - hmid, -halfCellSize);

#define ADD(ea, eb) faces.push_back(HeightmapTriangle(midPt, ea, eb))
      ADD(edge0, edge1);
      ADD(edge1, edge2);
      ADD(edge2, edge3);
      ADD(edge3, edge0);
#undef ADD
    }
  }
}


// Get faces from heightmap with cells represented as 4-triangle 5-point geometry,
// with 4 points at grid vertices and 1 at the cell center.
// Cached version: appends
template <class HM, class Tri, class BitChk>
void get_faces_hmap_cached(HM &heightmap, Tab<Tri> &faces, const BBox3 &box, BitChk &tchk)
{

  const Point3 ofs = heightmap.getHeightmapOffset();
  real cellSize = heightmap.getHeightmapCellSize();
  real halfCellSize = cellSize * 0.5f;

  int x0 = int(floorf((box[0].x - ofs.x) / cellSize));
  int x1 = int(floorf((box[1].x - ofs.x) / cellSize));
  int y0 = int(floorf((box[0].z - ofs.z) / cellSize));
  int y1 = int(floorf((box[1].z - ofs.z) / cellSize));

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 > tchk.getW() - 1)
    x1 = tchk.getW() - 1;
  if (y1 > tchk.getH() - 1)
    y1 = tchk.getH() - 1;

  if (x1 < x0 || y1 < y0)
    return;

  for (int my = y0; my <= y1; ++my)
  {
    for (int mx = x0; mx <= x1; ++mx)
    {
      real h0, hx, hy, hxy, hmid;

      if (tchk.get(mx, my))
        continue;

      if (!heightmap.getHeightmapCell5Pt(IPoint2(mx, my), h0, hx, hy, hxy, hmid))
        continue;

      // calc cell max Y
      real maxY = h0;
      if (hx > maxY)
        maxY = hx;
      if (hy > maxY)
        maxY = hy;
      if (hxy > maxY)
        maxY = hxy;
      if (hmid > maxY)
        maxY = hmid;

      if (box[0].y - ofs.y > maxY)
        continue;

      // calc cell min Y
      real minY = h0;
      if (hx < minY)
        minY = hx;
      if (hy < minY)
        minY = hy;
      if (hxy < minY)
        minY = hxy;
      if (hmid < minY)
        minY = hmid;

      if (box[1].y - ofs.y < minY)
        continue;

      // add triangles
      Point3 midPt((mx + 0.5f) * cellSize + ofs.x, hmid + ofs.y, (my + 0.5f) * cellSize + ofs.z);

      Point3 edge0(-halfCellSize, h0 - hmid, -halfCellSize);
      Point3 edge1(-halfCellSize, hy - hmid, +halfCellSize);
      Point3 edge2(+halfCellSize, hxy - hmid, +halfCellSize);
      Point3 edge3(+halfCellSize, hx - hmid, -halfCellSize);

      int l = append_items(faces, 4);
      faces[l].setP0(midPt);
      faces[l].setE1(edge0);
      faces[l].setE2(edge1);

      faces[l + 1].setP0(midPt);
      faces[l + 1].setE1(edge1);
      faces[l + 1].setE2(edge2);

      faces[l + 2].setP0(midPt);
      faces[l + 2].setE1(edge2);
      faces[l + 2].setE2(edge3);

      faces[l + 3].setP0(midPt);
      faces[l + 3].setE1(edge3);
      faces[l + 3].setE2(edge0);

      tchk.set(mx, my);
    }
  }
}
