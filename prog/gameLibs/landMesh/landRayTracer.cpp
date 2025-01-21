// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/landRayTracer.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_mesh.h>
#include <math/dag_math2d.h>
#include <vecmath/dag_vecMath.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <util/dag_bitArray.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <generic/dag_smallTab.h>
#include <memory/dag_framemem.h>
#include <gameMath/traceUtils.h> //requried only for TraceMeshFaces

template <class VertIndex, class FaceIndex>
void BaseLandRayTracer<VertIndex, FaceIndex>::loadStreamToDump(void *dump, IGenLoad &cb, int sz)
{
  if (!sharedMem || !sharedMem->doesPtrBelong(bindump))
    mem->free(bindump);
  bindump = (MemDump *)dump;
  bindumpSz = sz;
  memset(bindump, 0, sizeof(MemDump));

  char sign[6];
  cb.read(sign, 6);
  if (memcmp(sign, "LTdump", 6) != 0)
  {
    G_ASSERT(0 && "bad LTdump label");
    return;
  }

  bindump->numCellsX = cb.readInt();
  bindump->numCellsY = cb.readInt();
  bindump->cellSize = cb.readReal();
  cb.read(&bindump->ofs, sizeof(bindump->ofs));
  cb.read(&bindump->box, sizeof(bindump->box));

  char *p = (char *)((intptr_t(bindump + 1) + 15) & ~15);

#define SETUP_SLICE(S, TYPE)                                               \
  bindump->S.set((TYPE *)(intptr_t(p) - intptr_t(bindump)), cb.readInt()); \
  cb.read(p, data_size(bindump->S));                                       \
  p += (data_size(bindump->S) + 15) & ~15;

  SETUP_SLICE(cellsD, LandRayCellD);
  SETUP_SLICE(grid, uint32_t);
  SETUP_SLICE(gridHt, float);
  SETUP_SLICE(allFaces, VertIndex);
  SETUP_SLICE(allVerts, Vertex);
  SETUP_SLICE(faceIndices, FaceIndex);

#undef SETUP_SLICE

  // compute proper end-of-data for later checks
  p -= (data_size(bindump->faceIndices) + 15) & ~15;
  p += data_size(bindump->faceIndices);

  // G_ASSERTF(p-(char*)bindump <= sz, "%d > sz=%d", p-(char*)bindump, sz);

  initFromDump();
}

template <class VertIndex, class FaceIndex>
void BaseLandRayTracer<VertIndex, FaceIndex>::load(void *dump, int sz)
{
  if (!sharedMem || !sharedMem->doesPtrBelong(bindump))
    mem->free(bindump);
  bindump = (MemDump *)dump;
  bindumpSz = sz;

  initFromDump();
}

template <class VertIndex, class FaceIndex>
void BaseLandRayTracer<VertIndex, FaceIndex>::initFromDump()
{
  numCellsX = bindump->numCellsX;
  numCellsY = bindump->numCellsY;
  cellSize = bindump->cellSize;
  setBBox(bindump->box, bindump->ofs, cellSize);

#define SET_FROM_DUMP(cont, typ) cont.set((typ *)((char *)bindump + (intptr_t)bindump->cont.data()), bindump->cont.size())

  dag::Span<LandRayCellD> cellsD;
  SET_FROM_DUMP(cellsD, LandRayCellD);

  SET_FROM_DUMP(allFaces, VertIndex);
  SET_FROM_DUMP(allVerts, Vertex);
  SET_FROM_DUMP(grid, uint32_t);
  SET_FROM_DUMP(gridHt, float);
  SET_FROM_DUMP(faceIndices, FaceIndex);

  convertCellsFromOut(cellsD);
#undef SET_FROM_DUMP

  G_ASSERT(cells.size() == numCellsX * numCellsY);

#define AR(a) a.size(), unsigned(data_size(a) >> 10), ((float)data_size(a) / total) * 100.f
  float total = (float)(data_size(cells) + data_size(grid) + data_size(gridHt) + data_size(allFaces) + data_size(allVerts) +
                        data_size(faceIndices));
  debug("LandRayTracer data size = %dK\n"
        "\tcells count = %d, %uK, %1.f%%\n"
        "\tgrid count = %d, %uK, %1.f%%\n"
        "\tgridHt count = %d, %uK, %1.f%%\n"
        "\tallFaces count = %d, %uK, %1.f%%\n"
        "\tallVerts count = %d, %uK, %1.f%%\n"
        "\tfaceIndices count = %d, %uK, %1.f%%\n",
    ((int)total) >> 10, AR(cells), AR(grid), AR(gridHt), AR(allFaces), AR(allVerts), AR(faceIndices));
#undef AR

  if (sharedMem && sharedMem->doesPtrBelong(bindump))
  {
    mark_global_shared_mem_readonly(bindump, bindumpSz, true);
    sharedMem->markPtrDataReady(bindump);
  }
  initInternal();
}


template <class VertIndex, class FaceIndex>
void BaseLandRayTracer<VertIndex, FaceIndex>::save(IGenSave &cb)
{
  cb.write((void *)"LTdump", 6);
  cb.writeInt(numCellsX);
  cb.writeInt(numCellsY);
  cb.writeReal(cellSize);
  cb.writeReal(offset.x);
  cb.writeReal(offset.y);
  cb.writeReal(offset.z);
  cb.writeReal(bbox[0].x);
  cb.writeReal(bbox[0].y);
  cb.writeReal(bbox[0].z);
  cb.writeReal(bbox[1].x);
  cb.writeReal(bbox[1].y);
  cb.writeReal(bbox[1].z);

  Tab<LandRayCellD> cellsD(tmpmem);
  convertCellsToOut(cellsD);

  cb.writeTab(cellsD);
  cb.writeTab(grid);
  cb.writeTab(gridHt);
  cb.writeTab(allFaces);
  cb.writeTab(allVerts);
  cb.writeTab(faceIndices);
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

template <class Indices>
static inline void remap_indices(const Indices *indices, const int numIndices, Indices *out_indices, const unsigned int numVerts,
  int *const vertind)
{
  // caches oldIndex --> newIndex conversion
  int *const indexCache = vertind;
  memset(indexCache, -1, sizeof(int) * numVerts);

  // loop over primitive groups
  unsigned int indexCtr = 0;

  for (int j = 0; j < numIndices; j++)
  {
    int in_index = indices[j];
    int cachedIndex = indexCache[in_index];
    if (cachedIndex == -1) // we haven't seen this index before
    {
      // point to "last" vertex in VB
      out_indices[j] = indexCtr;

      // add to index cache, increment
      indexCache[in_index] = indexCtr++;
    }
    else
    {
      // we've seen this index before
      out_indices[j] = cachedIndex;
    }
  }
}

template <class Indices>
static inline bool optimize_4cache(unsigned char *verts, int vertSize, int numv, Indices *inds, int numIndices)
{
  return true; //== not working?
  if (!numv)
    return true;
  for (int i = 0; i < numIndices; ++i)
    G_ASSERT(inds[i] < numv);

  Tab<unsigned char> vertices(tmpmem);
  Tab<int> vertind(tmpmem);
  vertind.resize(numv);
  vertices.resize(numv * vertSize);

  SmallTab<Indices, TmpmemAlloc> newInds;
  clear_and_resize(newInds, numIndices);
  memcpy(newInds.data(), inds, data_size(newInds));

  remap_indices(inds, numIndices, newInds.data(), numv, &vertind[0]);
  memcpy(&vertices[0], verts, data_size(vertices));
  int *in = &vertind[0];
  unsigned char *vert = &vertices[0];
  debug("vertSize=%d numv=%d faces=%d", vertSize, numv, numIndices / 3);
  for (int i = 0; i < numv; ++i, ++in, vert += vertSize)
  {
    if (*in == -1)
    {
      continue;
    }

    memcpy(&verts[*in * vertSize], vert, vertSize);
  }
  return true;
}

#if _TARGET_PC
template <class VertIndex, class FaceIndex>
bool BaseLandRayTracer<VertIndex, FaceIndex>::build(uint32_t cellsX, uint32_t cellsY, float cellSz, const Point3 &ofs,
  const BBox3 &box, dag::ConstSpan<Mesh *> meshes, dag::ConstSpan<Mesh *> combined_meshes, uint32_t min_grid, uint32_t max_grid,
  bool cache_optimize)
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
    if (startVerts - cellStartVerts)
    {
      packVerts(&allVerts[cellStartVerts], &sourceVerts[0], startVerts - cellStartVerts, cell.scale, cell.ofs);
      if (cache_optimize)
        optimize_4cache((unsigned char *)cell.verts, elem_size(allVerts), startVerts - cellStartVerts, cell.faces,
          allFaces.data() + startFaces - cell.faces);
    }
  }
  RESIZE_STOR(grid, currentGridSize);
  RESIZE_STOR(gridHt, currentGridSize - cells.size());
  for (int i = 0; i < gridHt.size(); ++i)
    gridHt[i] = MIN_REAL;
  currentGridSize = 0;
  Tab<FaceIndex> faceInds(tmpmem);
  for (int lt = 0, y = 0; y < numCellsY; ++y)
  {
    for (int x = 0; x < numCellsY; ++x, ++lt)
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

template class BaseLandRayTracer<uint16_t, uint16_t>;
template class BaseLandRayTracer<uint32_t, uint32_t>;
