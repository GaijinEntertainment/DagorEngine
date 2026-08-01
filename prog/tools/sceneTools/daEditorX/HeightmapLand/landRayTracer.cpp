// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landRayTracer.h"
#include <math/dag_rayIntersectBox.h>
#include <math/dag_mesh.h>
#include <math/dag_math2d.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_tab.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <generic/dag_smallTab.h>
#include <memory/dag_framemem.h>

template <class VertIndex, class FaceIndex>
BaseLandRayTracer<VertIndex, FaceIndex>::~BaseLandRayTracer()
{
  del_it(buildStor);
}

static inline bool line_intersect_box(const BBox3 &box, const Point3 &p0, const Point3 &p1, float &maxHt)
{
  float mint = 0.f, maxt = 0.f;
  if (!isect_line_box(Point2::xz(p0), Point2::xz(p1 - p0), BBox2(Point2::xz(box[0]), Point2::xz(box[1])), mint, maxt))
    return false;
  bool ret = false;
  if (mint >= 0 && mint <= 1.0f)
  {
    maxHt = max(maxHt, (p1.y - p0.y) * mint + p0.y);
    ret = true;
  }

  if (maxt >= 0 && maxt <= 1.0f)
  {
    maxHt = max(maxHt, (p1.y - p0.y) * maxt + p0.y);
    ret = true;
  }
  return ret;
}

static inline bool getTriangleHt(const Point3 &xz, real &maxht, const Point3 &vert0, const Point3 &edge1, const Point3 &edge2)
{
  float ht = 0.f;
  if (::getTriangleHt(xz.x, xz.z, ht, vert0, edge1, edge2))
  {
    maxht = max(ht, maxht);
    return true;
  }
  return false;
}

static inline bool is_triangle_in_box2d(const Point3 &p0, const Point3 &p1, const Point3 &p2, const BBox3 &box, float &maxHt)
{
  maxHt = MIN_REAL;
  int corners = 0;
  if (box & p0)
  {
    maxHt = max(maxHt, p0.y);
    corners++;
  }
  if (box & p1)
  {
    maxHt = max(maxHt, p1.y);
    corners++;
  }
  if (box & p2)
  {
    maxHt = max(maxHt, p2.y);
    corners++;
  }
  float maxy = max(max(p0.y, p1.y), p2.y);
  if (corners)
  {
    // either all three corners inside box, or highest corner is inside box
    if (corners == 3 || fabsf(maxy - maxHt) < 0.00005)
      return true;
  }
  float miny = min(min(p0.y, p1.y), p2.y);
  // check if box lines intersects triangle
  Point3 edge1 = p1 - p0;
  Point3 edge2 = p2 - p0;
  // z = lim[0] quad
  int boxcorners = 0;
  if (getTriangleHt(box.point(0), maxHt, p0, edge1, edge2))
    boxcorners++;
  if (getTriangleHt(box.point(1), maxHt, p0, edge1, edge2))
    boxcorners++;
  if (getTriangleHt(box.point(4), maxHt, p0, edge1, edge2))
    boxcorners++;
  if (getTriangleHt(box.point(5), maxHt, p0, edge1, edge2))
    boxcorners++;
  maxHt = max(min(maxy, maxHt), miny);
  if (boxcorners == 4)
    return true;
  bool ret = false;
  if (line_intersect_box(box, p0, p1, maxHt))
    ret = true;
  if (line_intersect_box(box, p0, p2, maxHt))
    ret = true;
  if (line_intersect_box(box, p1, p2, maxHt))
    ret = true;
  maxHt = max(min(maxy, maxHt), miny);
  //  G_ASSERT(maxHt<=maxy+0.00001f); assert not allow to export levels (bulge for example). Commented by Kirill with permission from
  //  Anton

  return ret | (boxcorners > 0) | (corners > 0);
}

#if _TARGET_PC
template <class VertIndex, class FaceIndex>
bool BaseLandRayTracer<VertIndex, FaceIndex>::build(uint32_t cellsX, uint32_t cellsY, float cellSz, const Point3 &ofs,
  const BBox3 &box, dag::ConstSpan<Mesh *> meshes, dag::ConstSpan<Mesh *> combined_meshes, uint32_t min_grid, uint32_t max_grid)
{
#define RESIZE_STOR(NM, COUNT) \
  buildStor->NM.resize(COUNT); \
  NM.set(buildStor->NM.data(), buildStor->NM.size());
  if (!buildStor)
    buildStor = new BuildStorage;
  min_grid = clamp(min_grid, (uint32_t)MIN_GRID_SIZE, (uint32_t)MAX_GRID_SIZE);
  max_grid = clamp(max_grid, (uint32_t)MIN_GRID_SIZE, (uint32_t)MAX_GRID_SIZE);
  numCellsX = cellsX;
  numCellsY = cellsY;
  setBBox(box, ofs, cellSz);
  G_ASSERT(meshes.size() == numCellsX * numCellsY);
  G_ASSERT(combined_meshes.size() == numCellsX * numCellsY || combined_meshes.size() == 0);
  cells.resize(numCellsX * numCellsY);
  int totalFaces = 0, minFaces = 100000000, maxFaces = 0;
  int totalVerts = 0;
  unsigned bits = 8 * sizeof(FaceIndex) - 1;
  const unsigned max_faces = ((unsigned(1 << bits) - 1u) << 1u) | 1u;
  const unsigned max_vertex = ((unsigned(1 << (8 * sizeof(VertIndex) - 1)) - 1u) << 1u) | 1u;


  // SmallTab<int, TmpmemAlloc> faceCount;
  // faceCount.resize(cells.size());
  SmallTab<int, TmpmemAlloc> usedFaceCount;
  clear_and_resize(usedFaceCount, cells.size());
  int maxVerts = 0;

  for (int i = 0; i < meshes.size(); ++i)
  {
    Mesh &mesh = *meshes[i];
    usedFaceCount[i] = meshes[i] ? mesh.face.size() : 0;
    if (combined_meshes.size() && combined_meshes[i])
      usedFaceCount[i] += combined_meshes[i]->face.size();
    if (usedFaceCount[i] > max_faces)
    {
      logerr("too much faces in mesh %d out of %d", usedFaceCount[i], max_faces);
      return false;
    }
    int cellVerts = (combined_meshes.size() && combined_meshes[i]) ? combined_meshes[i]->getVert().size() : 0;
    if (cellVerts > max_vertex)
    {
      logerr("too much verts in mesh %d out of %d", mesh.vert.size(), max_vertex);
      return false;
    }

    // faceCount[lt] = mesh.face.size();
    if (usedFaceCount[i])
    {
      totalFaces += usedFaceCount[i];
      minFaces = min(usedFaceCount[i], minFaces);
      maxFaces = max(usedFaceCount[i], maxFaces);
      if (meshes[i])
        cellVerts += mesh.vert.size();
    }
    totalVerts += cellVerts;
    maxVerts = max(cellVerts, maxVerts);
  }
  debug("landraytracer totalFaces %d", totalFaces);
  // todo: remove unused verts!

  int currentGridSize = 0;

  RESIZE_STOR(allFaces, totalFaces * 3);
  RESIZE_STOR(allVerts, totalVerts + 1);
  SmallTab<Point3, TmpmemAlloc> sourceVerts;
  clear_and_resize(sourceVerts, maxVerts + 1);
  allVerts[totalVerts] = Vertex(0, 0, 0, 0);
  int startFaces = 0, startVerts = 0;
  float denom_faces;
  if (minFaces != maxFaces)
    denom_faces = float(sqrtf(maxFaces) - sqrtf(minFaces));
  else
    denom_faces = 1.f;
  for (int lt = 0; lt < cells.size(); ++lt)
  {
    LandRayCell &cell = cells[lt];
    int cellStartVerts = startVerts;
    Mesh &mesh = *meshes[lt];

    float t = (sqrtf(usedFaceCount[lt]) - sqrtf(minFaces)) / denom_faces;
    uint32_t gridSize = usedFaceCount[lt] ? (uint32_t)lerp((float)min_grid, (float)max_grid, t) : 0;
    cell.fistart_gridsize = ::min(gridSize, (uint32_t)GRIDSIZE_MASK);
    currentGridSize += gridSize * gridSize + 1;
    cell.faces = &allFaces[startFaces];
    if (meshes[lt])
      for (int i = 0; i < mesh.getFace().size(); ++i)
      {
        allFaces[startFaces + 0] = mesh.getFace()[i].v[0];
        allFaces[startFaces + 1] = mesh.getFace()[i].v[1];
        allFaces[startFaces + 2] = mesh.getFace()[i].v[2];
        startFaces += 3;
      }
    cell.verts = &allVerts[startVerts];
    if (meshes[lt] && mesh.vert.size())
    {
      memcpy(&sourceVerts[0], &mesh.getVert()[0], data_size(mesh.getVert()));
      startVerts += mesh.vert.size();
    }
    if (combined_meshes.size() && combined_meshes[lt])
    {
      Mesh &mesh = *combined_meshes[lt];
      int vertOfs = startVerts - cellStartVerts;
      for (int i = 0; i < mesh.getFace().size(); ++i)
      {
        allFaces[startFaces + 0] = mesh.getFace()[i].v[0] + vertOfs;
        allFaces[startFaces + 1] = mesh.getFace()[i].v[1] + vertOfs;
        allFaces[startFaces + 2] = mesh.getFace()[i].v[2] + vertOfs;
        startFaces += 3;
      }
      if (mesh.vert.size())
      {
        memcpy(&sourceVerts[startVerts - cellStartVerts], &mesh.getVert()[0], data_size(mesh.getVert()));
        startVerts += mesh.vert.size();
      }
    }
    if (startVerts - cellStartVerts != 0) //-V793
      packVerts(&allVerts[cellStartVerts], &sourceVerts[0], startVerts - cellStartVerts, cell.scale, cell.ofs);
  }
  RESIZE_STOR(grid, currentGridSize);
  RESIZE_STOR(gridHt, currentGridSize - cells.size());
  for (int i = 0; i < gridHt.size(); ++i)
    gridHt[i] = MIN_REAL;
  currentGridSize = 0;
  Tab<FaceIndex> faceInds(tmpmem);
  for (int lt = 0, y = 0; y < numCellsY; ++y)
  {
    for (int x = 0; x < numCellsX; ++x, ++lt)
    {
      LandRayCell &cell = cells[lt];
      cell.grid = &grid[currentGridSize];
      cell.gridHt = &gridHt[currentGridSize - lt];
      uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
      currentGridSize += gridSize * gridSize + 1;
      Tab<Tabint> gridindices(tmpmem);
      gridindices.resize(gridSize * gridSize);
      float gridwidth = cellSize / gridSize;
      BBox3 cellBox(Point3(x * cellSize + offset.x, bbox[0].y, y * cellSize + offset.z),
        Point3((x + 1) * cellSize + offset.x, bbox[1].y, (y + 1) * cellSize + offset.z));
      float halfGSzSc = gridSize / cellSize;
      float halfGSzOfsX = -halfGSzSc * cellBox[0].x, halfGSzOfsY = -halfGSzSc * cellBox[0].z;
      cell.maxHt = MIN_REAL;
      for (int i = 0; i < usedFaceCount[lt]; ++i)
      {
        G_ASSERT(cell.verts + cell.faces[i * 3 + 0] < allVerts.data() + allVerts.size() &&
                 cell.verts + cell.faces[i * 3 + 1] < allVerts.data() + allVerts.size() &&
                 cell.verts + cell.faces[i * 3 + 2] < allVerts.data() + allVerts.size());
        Point3 v[3] = {unpack_cell_vert(cell, cell.verts[cell.faces[i * 3 + 0]]),
          unpack_cell_vert(cell, cell.verts[cell.faces[i * 3 + 1]]), unpack_cell_vert(cell, cell.verts[cell.faces[i * 3 + 2]])};
        BBox3 fbox;
        fbox += v[0];
        fbox += v[1];
        fbox += v[2];

        int sx = clamp((int)floor(fbox[0].x * halfGSzSc + halfGSzOfsX), 0, (int)gridSize - 1),
            sy = clamp((int)floor(fbox[0].z * halfGSzSc + halfGSzOfsY), 0, (int)gridSize - 1);
        int ex = clamp((int)floor(fbox[1].x * halfGSzSc + halfGSzOfsX), 0, (int)gridSize - 1),
            ey = clamp((int)floor(fbox[1].z * halfGSzSc + halfGSzOfsY), 0, (int)gridSize - 1);
        cell.maxHt = max(cell.maxHt, fbox[1].y);
        for (int gy = sy; gy <= ey; ++gy)
          for (int gx = sx; gx <= ex; ++gx)
          {
            BBox3 gridBox = cellBox;
            gridBox[0] += Point3(gx * gridwidth, 0, gy * gridwidth);
            gridBox[1] = Point3(gridBox[0].x + gridwidth, cellBox[1].y, gridBox[0].z + gridwidth);

            // test if triangle is really in the box
            // make better prediction (actual height in grid cell is smaller, than triangle max)
            float maxHt;
            if (!is_triangle_in_box2d(v[0], v[1], v[2], gridBox, maxHt))
              continue;
            gridindices[gx + gy * gridSize].push_back(i);

            inplace_max(cell.gridHt[gx + gy * gridSize], maxHt);
          }
      }

      int addedFi = 0, addedRoundedFi = 0;
      for (int i = 0; i < gridindices.size(); ++i)
      {
        addedFi += gridindices[i].size();
        addedRoundedFi += (gridindices[i].size() + 3) & (~3);
      }

      uint32_t fistart = faceInds.size();
      uint32_t cellFistart = fistart;
      cell.fistart_gridsize |= fistart << GRIDSIZE_BITS;
      if (fistart >= 1 << (32 - GRIDSIZE_BITS))
      {
        logerr("too much faces for grid %u .. %u", min_grid, max_grid);
        return false;
      }
      append_items(faceInds, addedRoundedFi);
      for (int gi = 0, gridY = 0; gridY < gridSize; ++gridY)
        for (int gridX = 0; gridX < gridSize; ++gridX, ++gi)
        {
          // G_ASSERT(cell.gridHt[gi] > MIN_REAL);
          cell.grid[gi] = fistart - cellFistart;
          G_ASSERT(!(cell.grid[gi] & 3));
          /*if (faceInds.size()-fistart>max_index)
          {
            logerr("too much faces for grid %u .. %u",min_grid, max_grid);
            if (max_grid==min_grid && min_grid == MIN_GRID_SIZE)
              return false;
            if (max_grid==min_grid)
              min_grid -=2;
            gridindices.clear();
            faceInds.clear();
            usedFaceCount.clear();
            min_grid = clamp(min_grid, (uint32_t)MIN_GRID_SIZE, (uint32_t)MAX_GRID_SIZE);
            max_grid = clamp(max_grid, (uint32_t)MIN_GRID_SIZE, (uint32_t)MAX_GRID_SIZE);
            return build(cellsX, cellsY, cellSz, ofs, box, meshes, combined_meshes, min_grid, max_grid-2);
          }*/
          for (int i = 0; i < gridindices[gi].size(); ++i)
          {
            faceInds[fistart] = gridindices[gi][i];
            fistart++;
          }
          for (uint32_t i = gridindices[gi].size(); i < ((gridindices[gi].size() + 3) & (~3)); ++i)
          {
            faceInds[fistart] = gridindices[gi][gridindices[gi].size() - 1];
            fistart++;
          }
        }
      cell.grid[gridSize * gridSize] = addedRoundedFi;
      // G_ASSERT(faceInds.size()-fistart<=max_index);
    }
  }
  buildStor->faceIndices = faceInds;
  faceIndices.set(buildStor->faceIndices.data(), buildStor->faceIndices.size());
  initInternal();
  return true;
#undef RESIZE_STOR
}
#endif

template class BaseLandRayTracer<uint32_t, uint32_t>;
