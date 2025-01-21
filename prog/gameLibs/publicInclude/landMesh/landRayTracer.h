//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdlib.h>
#include <util/dag_globDef.h>
#include <math/dag_math3d.h>
#include <math/dag_wooray2d.h>
#include <math/dag_mathUtils.h>
#include <math/dag_traceRayTriangle.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_sharedMem.h>

class IGenLoad;
class IGenSave;
class Mesh;
class LandRaySaver;
class GlobalSharedMemStorage;
struct TraceMeshFaces;

template <class VertIndex, class FaceIndex>
class BaseLandRayTracer
{
public:
  struct Vertex
  {
    uint16_t v[4];
    Vertex() = default;
    Vertex(uint16_t x, uint16_t y, uint16_t z, uint16_t w)
    {
      v[0] = x;
      v[1] = y;
      v[2] = z;
      v[3] = w;
    }
  };

private:
  enum
  {
    GRIDSIZE_BITS = 10,
    GRIDSIZE_MASK = (1 << GRIDSIZE_BITS) - 1
  };
  struct LandRayCellD
  {
    vec4f ofs, scale; // vertex unpacking
    float maxHt;
    uint32_t gridHtStart;
    uint32_t gridStart;
    uint32_t fistart_gridsize;
    uint32_t facesStart;
    uint32_t vertsStart;
    uint32_t _resv[2];
  };
  struct LandRayCell
  {
    vec4f ofs, scale; // vertex unpacking
    float maxHt;
    uint32_t fistart_gridsize;
    float *gridHt;
    uint32_t *grid;
    VertIndex *faces;
    Vertex *verts;
  };
  struct MemDump
  {
    int numCellsX;
    int numCellsY;
    float cellSize;
    Point3 ofs;
    BBox3 box;

    dag::Span<LandRayCellD> cellsD;
    dag::Span<VertIndex> allFaces;
    dag::Span<Vertex> allVerts;
    dag::Span<uint32_t> grid;
    dag::Span<float> gridHt;
    dag::Span<FaceIndex> faceIndices;
  };

  Tab<LandRayCell> cells;
  dag::Span<VertIndex> allFaces;
  dag::Span<Vertex> allVerts;
  // dag::Span<Point3> allVerts;
  dag::Span<uint32_t> grid;
  dag::Span<float> gridHt;
  dag::Span<FaceIndex> faceIndices;

  struct BuildStorage
  {
    Tab<VertIndex> allFaces;
    Tab<Vertex> allVerts;
    Tab<uint32_t> grid;
    Tab<float> gridHt;
    Tab<FaceIndex> faceIndices;
    BuildStorage() : allFaces(tmpmem), allVerts(tmpmem), grid(tmpmem), gridHt(tmpmem), faceIndices(tmpmem) {}
  };
  vec4f offsetV, numCellsV, invCellSizeV;
  vec4f offsetVXZ, numCellsVXZ;
  bbox3f boxV;
#if _TARGET_PC
  static void packVerts(Vertex *packed, const Point3 *verts, int vertCount, vec4f &scale, vec4f &ofs)
  {
    bbox3f box;
    v_bbox3_init_empty(box);
    for (int i = 0; i < vertCount; ++i)
      v_bbox3_add_pt(box, v_ldu(&verts[i].x));
    vec4f size = v_bbox3_size(box);
    vec4f cmpofs = box.bmin;

    vec4f shortscale = v_make_vec4f(65535, 65535, 65535, 0);
    // vec4f shortofs = v_make_vec4f(-32768,-32768,-32768,0);
    vec4f cmpscale = v_div(shortscale, size);
    // ensure that 'almost flat' surface is flat
    cmpscale = v_perm_xbzw(cmpscale, v_and(cmpscale, v_cmp_gt(size, V_C_EPS_VAL)));

    scale = v_div(size, shortscale);
    // scale = v_perm_xbzw(scale, v_sel(V_C_ONE, scale, v_cmp_gt(cmpscale, v_zero())));
    // if size.y is 0, we will use 1 scale, as all verts will have 0.
    // however, we can scale it to one even after loading, and this is important information (that cell is flat)
    //  so we just ensure that vertices are zeroed
    scale = v_and(scale, v_cast_vec4f(V_CI_MASK1110));
    ofs = v_and(box.bmin, v_cast_vec4f(V_CI_MASK1110));
    // ofs = v_bbox3_center(box);
    // ofs = v_add(v_bbox3_center(box), v_mul(size, v_splats(0.5/65535.)));
    for (int i = 0; i < vertCount; ++i)
    {
      vec4f vert = v_ldu(&verts[i].x);
      vert = v_mul(v_sub(vert, cmpofs), cmpscale);
      vert = v_max(vert, v_zero());

      DECL_ALIGN16(int, verti[4]);
      v_sti(verti, v_cvt_roundi(vert));
      packed[i].v[0] = clamp(verti[0], 0, 65535);
      packed[i].v[1] = clamp(verti[1], 0, 65535);
      packed[i].v[2] = clamp(verti[2], 0, 65535);
      packed[i].v[3] = clamp(verti[3], 0, 65535);
      //__m128i verti16 = _mm_packus_epi32(verti, verti);
      //_mm_storel_epi64((__m128i*)(packed+i), verti16);
    }
  }
#endif
  __forceinline int getCellFaceCount(int ci) const // returns number of face indices count, i.e. faces*3!
  {
    return (((ci < cells.size() - 1) ? cells[ci + 1].faces : allFaces.data() + allFaces.size()) - cells[ci].faces);
  }
  static __forceinline vec4f unpack_vert(const Vertex &vertex, vec4f scale, vec4f ofs)
  {
    vec4i verti = v_lduush(vertex.v);
    return v_madd(v_cvt_vec4f(verti), scale, ofs);
  }
  static Point3 unpack_cell_vert(LandRayCell &cell, const Vertex &vert)
  {
    vec4f v = unpack_vert(vert, cell.scale, cell.ofs);
    return Point3(v_extract_x(v), v_extract_y(v), v_extract_z(v));
  }
  void initInternal()
  {
    offsetV = v_ldu(&offset.x);
    numCellsV = v_make_vec4f(numCellsX, 0.f, numCellsY, 0.f);
    invCellSizeV = v_splats(invCellSize);
    numCellsVXZ = v_make_vec4f(numCellsX - 0.0001, numCellsY - 0.0001, numCellsX - 0.0001, numCellsY - 0.0001);
    offsetVXZ = v_make_vec4f(offset.x, offset.z, offset.x, offset.z);
    boxV = v_ldu_bbox3(bbox);
    /*packedVerts.resize(allVerts.size()+1);//just add 8 bytes for safety
    packedCells.resize(cells.size());
    packedCellsBounds.resize(cells.size());
    memcpy(packedCells.data(), cells.data(), data_size(cells));
    for (int i = 0; i < packedCells.size(); ++i)
    {
      packedCells[i].verts = packedVerts.data() + (cells[i].verts-allVerts.data());
      int vertCount = (i==packedCells.size()-1 ? allVerts.data()+allVerts.size() : cells[i+1].verts) - cells[i].verts;
      packVerts(packedCells[i].verts, cells[i].verts, vertCount, packedCellsBounds[i].bmin, packedCellsBounds[i].bmax);
    }*/
  } // init vector variables

  uint32_t numCellsX, numCellsY;
  float cellSize, invCellSize;
  BBox3 bbox;
  BBox2 bbox2;
  Point3 offset;
  Point2 offset2;

  MemDump *bindump;
  int bindumpSz;
  IMemAlloc *mem;
  BuildStorage *buildStor;

  void setBBox(const BBox3 &b, const Point3 &ofs, float cellSz)
  {
    bbox = b;
    bbox2[0] = Point2::xz(bbox[0]);
    bbox2[1] = Point2::xz(bbox[1]);
    offset = ofs;
    offset2 = Point2::xz(offset);
    cellSize = cellSz;
    invCellSize = 1.0f / cellSize;
  }
  void convertCellsToOut(Tab<LandRayCellD> &cellsD)
  {
    cellsD.resize(cells.size());
    mem_set_0(cellsD);
    for (uint32_t i = 0; i < cells.size(); ++i)
    {
      cellsD[i].maxHt = cells[i].maxHt;
      cellsD[i].ofs = cells[i].ofs;
      cellsD[i].scale = cells[i].scale;
      cellsD[i].fistart_gridsize = cells[i].fistart_gridsize;
      cellsD[i].gridHtStart = cells[i].gridHt - gridHt.data();
      cellsD[i].gridStart = cells[i].grid - grid.data();
      cellsD[i].facesStart = cells[i].faces - allFaces.data();
      cellsD[i].vertsStart = cells[i].verts - allVerts.data();
    }
  }
  void convertCellsFromOut(dag::ConstSpan<LandRayCellD> cellsD)
  {
    cells.resize(cellsD.size());
    mem_set_0(cells);
    for (uint32_t i = 0; i < cells.size(); ++i)
    {
      cells[i].ofs = cellsD[i].ofs;
      cells[i].scale = v_and(cellsD[i].scale, (vec4f)V_CI_MASK1110);
      cells[i].maxHt = cellsD[i].maxHt;
      cells[i].fistart_gridsize = cellsD[i].fistart_gridsize;
      cells[i].gridHt = gridHt.data() + cellsD[i].gridHtStart;
      cells[i].grid = grid.data() + cellsD[i].gridStart;
      cells[i].faces = allFaces.data() + cellsD[i].facesStart;
      cells[i].verts = allVerts.data() + cellsD[i].vertsStart;
      // replace scale.y=0 in flat cells with scale.y=1 to avoid bugs in computations later
      if (v_test_vec_x_eqi_0(v_splat_y(cells[i].scale)))
        cells[i].scale = v_add(cells[i].scale, V_C_UNIT_0100);
    }
  }

  struct GridParam
  {
    Point2 offset;
    float cellSize, invCellSize;
    uint32_t numCellsX, numCellsY;
    const LandRayCell *cell;
  };

  template <class Shape, class Visitor>
  bool traverseProc(const GridParam &grid_param, const Shape &shape, Visitor &visitor)
  {
    bool result = false;
    float yMin, yMax;
    shape.getBoundsY(yMin, yMax);
    int cellYmin = (int)floorf((yMin - grid_param.offset.y) * grid_param.invCellSize);
    if (cellYmin >= (int)grid_param.numCellsY)
      return false;
    int cellYmax = (uint32_t)floorf((yMax - grid_param.offset.y) * grid_param.invCellSize);
    if (cellYmax < 0)
      return false;
    cellYmin = max(0, cellYmin);
    cellYmax = min(cellYmax, (int)grid_param.numCellsY - 1);
    Point2 cellOffset2;
    cellOffset2.y = grid_param.offset.y + cellYmin * grid_param.cellSize - grid_param.cellSize;
    for (int cellY = cellYmin; cellY <= cellYmax; ++cellY)
    {
      cellOffset2.y += grid_param.cellSize;
      float yMin = grid_param.offset.y + cellY * grid_param.cellSize;
      float yMax = yMin + grid_param.cellSize;
      float xMin, xMax;
      if (!shape.getBoundsX(yMin, yMax, xMin, xMax))
        continue;
      int cellXmin = floorf((xMin - grid_param.offset.x) * grid_param.invCellSize);
      if (cellXmin >= (int)grid_param.numCellsX)
        continue;
      int cellXmax = floorf((xMax - grid_param.offset.x) * grid_param.invCellSize);
      if (cellXmax < 0)
        continue;
      cellXmin = max(0, cellXmin);
      cellXmax = min(cellXmax, (int)grid_param.numCellsX - 1);
      cellOffset2.x = grid_param.offset.x + cellXmin * grid_param.cellSize - grid_param.cellSize;
      for (int cellX = cellXmin; cellX <= cellXmax; ++cellX)
      {
        cellOffset2.x += grid_param.cellSize;
        const bool finalGrid = grid_param.cell != NULL;
        int index = cellX + cellY * (int)grid_param.numCellsX;
        float height = !finalGrid ? cells[index].maxHt : grid_param.cell->gridHt[index];
        const TraverseResult traverseResult =
          visitor.visit(cellOffset2, Point2(grid_param.cellSize, grid_param.cellSize), height, finalGrid);
        if (traverseResult == TRAVERSE_RESULT_SKIP)
          continue;
        else if (traverseResult == TRAVERSE_RESULT_FINISH)
          return true;
        else if (finalGrid)
          result = true;
        else
        {
          const LandRayCell &cell = cells[index];
          const uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
          const GridParam subGridParam = {cellOffset2, cellSize / gridSize, gridSize / cellSize, gridSize, gridSize, &cell};
          if (traverseProc<Shape>(subGridParam, shape, visitor))
            result = true;
        }
      }
    }
    return result;
  }

public:
  enum
  {
    MIN_GRID_SIZE = 8,
    MAX_GRID_SIZE = GRIDSIZE_MASK
  };

  //! setup-once shared memory storage to be used to allocate dumps; when allocated in shared memory, dump is never released
  static GlobalSharedMemStorage *sharedMem;

  // BaseLandRayTracer(IMemAlloc* allocator = midmem);
  BaseLandRayTracer(IMemAlloc *allocator = midmem) : mem(allocator), bindump(NULL), bindumpSz(0), buildStor(NULL), cells(allocator) {}
  ~BaseLandRayTracer()
  {
    if (!sharedMem || !sharedMem->doesPtrBelong(bindump))
      mem->free(bindump);
    bindump = NULL;
    bindumpSz = 0;
    del_it(buildStor);
  }
  const BBox3 &getBBox() const { return bbox; }
  uint32_t dataSize()
  {
    return data_size(faceIndices) + data_size(grid) + data_size(gridHt) + data_size(cells) + data_size(allVerts) + data_size(allFaces);
  }
  int getCellCount() const { return cells.size(); }
  dag::ConstSpan<Vertex> getCellVerts(int idx) const
  {
    return dag::ConstSpan<Vertex>(cells[idx].verts,
      (idx + 1 < cells.size() ? cells[idx + 1].verts : allVerts.cend()) - cells[idx].verts);
  }
  dag::ConstSpan<VertIndex> getCellFaces(int idx) const
  {
    return dag::ConstSpan<VertIndex>(cells[idx].faces, (idx + 1 < cells.size() ? cells[idx + 1].faces
                                                         : allFaces.empty()    ? cells[idx].faces
                                                                               : allFaces.cend()) -
                                                         cells[idx].faces);
  }
  const float *getCellPackScaleF(int idx) const { return (float *)&cells[idx].scale; }
  const float *getCellPackOffsetF(int idx) const { return (float *)&cells[idx].ofs; }
  vec4f getCellPackScale(int idx) const { return cells[idx].scale; }
  vec4f getCellPackOffset(int idx) const { return cells[idx].ofs; }

  int getCellIdxNear(int *out_idx, int idx_count, float px, float pz, float rad) const
  {
    vec4i xzi = v_cvt_floori(v_div(v_sub(v_make_vec4f(px, pz, px, pz), v_make_vec4f(bindump->ofs.x + rad, bindump->ofs.z + rad,
                                                                         bindump->ofs.x - rad, bindump->ofs.z - rad)),
      v_splats(bindump->cellSize)));
    alignas(16) int xz01[4];
    v_sti(xz01, xzi);
    int x0 = xz01[0], z = xz01[1], x1 = xz01[2], z1 = xz01[3];
    if (x0 < 0)
      x0 = 0;
    if (x1 >= bindump->numCellsX)
      x1 = bindump->numCellsX - 1;
    if (z < 0)
      z = 0;
    if (z1 >= bindump->numCellsY)
      z1 = bindump->numCellsY - 1;

    int cnt = 0;
#if DAGOR_DBGLEVEL > 0
    int cellCount = (z1 - z + 1) * (x1 - x0 + 1);
#endif
    for (; z <= z1; z++)
      for (int x = x0; x <= x1 && cnt < idx_count; x++, cnt++)
        out_idx[cnt] = z * bindump->numCellsX + x;
    G_ASSERTF(cnt == 0 || cellCount <= idx_count, "Output array is small, idx_count=%d, needed_size=%d", idx_count, cellCount);
    return cnt;
  }

  __forceinline bool getHeight(const Point2 &pos1, float &ht, Point3 *normal)
  {
    if (normal)
      return traceDownFaceVec<true, true>(Point3(pos1.x, 0, pos1.y), ht, normal);
    return traceDownFaceVec<true, false>(Point3(pos1.x, 0, pos1.y), ht, NULL);
  }
  /*template <bool get_max_height>
  bool traceDownFaceScalar(const Point3 &pos3, float &ht_ret, Point3 *normal)
  {
    Point2 pos = Point2::xz(pos3) - offset2;
    if (pos.x < 0 || pos.y < 0 || (!get_max_height && ht_ret > bbox[1].y))
      return false;
    Point2 adjpos = pos*invCellSize;
    Point2 cellf, gridPos;
    gridPos.x = modff(adjpos.x, &cellf.x);
    gridPos.y = modff(adjpos.y, &cellf.y);
    int cellX = (int)cellf.x, cellY = (int)cellf.y;
    if (cellX>=numCellsX || cellY >= numCellsY)
      return false;
    uint32_t ci = cellX + cellY*numCellsX;
    const LandRayCell &cell = cells[ci];
    uint32_t gridSize = cell.fistart_gridsize&GRIDSIZE_MASK;
    if (!gridSize)
      return false;
    uint32_t gridX = (uint32_t)(floorf(gridPos.x*gridSize)),
             gridY = (uint32_t)(floorf(gridPos.y*gridSize));
    uint32_t gi = gridX + gridY*gridSize;
    float ht;
    if (get_max_height)
      ht = MIN_REAL;
    else
    {
      ht = max(bbox[0].y, ht_ret);
      if (cell.gridHt[gi] < ht)
        return false;
    }
    uint32_t fistart = cell.fistart_gridsize>>GRIDSIZE_BITS;
    uint32_t sfi = fistart+cell.grid[gi];
    uint32_t efi = fistart+cell.grid[gi+1];
    bool ret = false;
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(((uint32_t)(efi-sfi)) < (1 << 20), "%s infinite loop", __FUNCTION__);
#endif
    for (uint32_t i = sfi; i < efi; ++i)
    {
      uint32_t fi = faceIndices[i]*3;
      const Point3 &v0 = cell.verts[cell.faces[fi]];
      const Point3 &v1 = cell.verts[cell.faces[fi+1]];
      const Point3 &v2 = cell.verts[cell.faces[fi+2]];
      Point3 edge1 = v1-v0, edge2 = v2-v0;
      float triht;
      if (::getTriangleHtCull<get_max_height>(pos3.x, pos3.z,
        triht,
        v0,
        v1-v0,
        v2-v0))
      {
        if (triht>=ht && (get_max_height || triht<=pos3.y))
        {
          if (normal)
            *normal = edge1%edge2;
          ht = triht;
          ret = true;
        }
      }
    }
    if (ret)
    {
      ht_ret = ht;
      if (normal)
        normal->normalize();
      return true;
    }
    return false;
  }*/

#if _TARGET_ANDROID
#define VERT(I, J) v_cvt_vec4f(v_lduush(verts[faces[fi[I] + (J)]].v))
  static inline void load_4_tri(mat43f &p0, mat43f &p1, mat43f &p2, const VertIndex *faces, const Vertex *verts, int fi[4])
  {
    v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
    v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
    v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);
  }
#undef VERT
#define VERT(I, J) v_mul(v_cvt_vec4f(v_lduush(verts[faces[fi[I] + (J)]].v)), pkScale)
  static inline void load_4_tri(mat43f &p0, mat43f &p1, mat43f &p2, const VertIndex *faces, const Vertex *verts, uint32_t fi[4],
    vec4f pkScale)
  {
    v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
    v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
    v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);
  }
#undef VERT
#endif

  template <bool get_max_height, bool use_normal>
  bool traceDownFaceVec(const Point3 &pos3, float &ht_ret, Point3 *normal)
  {
    if (!get_max_height && ht_ret > bbox[1].y)
      return false;

    vec4f offsetedPos = v_sub(v_ldu(&pos3.x), offsetV);
    vec4f cellCoord = v_mul(offsetedPos, invCellSizeV);
    vec4f cellCoordFloored = v_floor(cellCoord);
    vec4f inside = v_and(v_cmp_ge(offsetedPos, v_zero()), v_cmp_lt(cellCoordFloored, numCellsV));
    inside = v_and(inside, v_cmp_gt(boxV.bmax, cellCoord));
    if ((v_signmask(inside) & 0b101) != 0b101)
      return false;

    vec4i cellIndices = v_cvt_vec4i(cellCoordFloored);
    int ci = v_extract_zi(cellIndices) * numCellsX + v_extract_xi(cellIndices);
    const LandRayCell &cell = cells[ci];
    uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
    if (!gridSize)
      return false;

    vec4f cellPart = v_sub(cellCoord, cellCoordFloored);
    vec4i gridCoord = v_cvt_floori(v_mul(cellPart, v_splats(gridSize)));
    uint32_t gridX = v_extract_xi(gridCoord);
    uint32_t gridY = v_extract_zi(gridCoord);
    uint32_t gi = gridX + gridY * gridSize;
    vec4f pkScale = cell.scale, pkOfs = cell.ofs;
    vec4f maxht;
    if (get_max_height)
      maxht = v_splats(MIN_REAL);
    else
    {
      float ht = max(bbox[0].y - 1.f, ht_ret);
      if (cell.gridHt[gi] < ht)
        return false;
      maxht = v_div(v_sub(v_splats(ht), v_splat_y(pkOfs)), v_splat_y(pkScale));
    }
    uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;
    uint32_t sfi = fistart + cell.grid[gi];
    uint32_t efi = fistart + cell.grid[gi + 1];
    int resulti = use_normal ? -1 : 0;
    DECL_ALIGN16(int, fi[4]);
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(((uint32_t)(efi - sfi)) < (1 << 20), "%s infinite loop", __FUNCTION__);
#endif
    vec4f gridPosV = v_div(v_sub(v_ldu(&pos3.x), pkOfs), pkScale);
#define VERT(I, J) v_cvt_vec4f(v_lduush(cell.verts[cell.faces[fi[I] + (J)]].v))

    for (uint32_t i = sfi; i < efi; i += 4)
    {
      fi[0] = faceIndices[i + 0] * 3;
      fi[1] = faceIndices[i + 1] * 3;
      fi[2] = faceIndices[i + 2] * 3;
      fi[3] = faceIndices[i + 3] * 3;

      mat43f p0, p1, p2;
#if _TARGET_ANDROID
      load_4_tri(p0, p1, p2, cell.faces, cell.verts, fi);
#else
      v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
      v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
      v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);
#endif

      if (use_normal)
      {
        int ret = get4TrianglesMaxHtId<get_max_height, !get_max_height>(gridPosV, maxht, p0, p1, p2);
        resulti = ret >= 0 ? fi[ret] : resulti;
      }
      else
        resulti |= get4TrianglesMaxHt<get_max_height, !get_max_height>(gridPosV, maxht, p0, p1, p2);
    }
#undef VERT
    if (use_normal)
    {
      if (resulti >= 0)
      {
        ht_ret = v_extract_x(v_madd_x(maxht, v_splat_y(pkScale), v_splat_y(pkOfs)));
#define VERT(v) unpack_vert(v, pkScale, pkOfs)
        vec3f v0v = VERT(cell.verts[cell.faces[resulti]]);
        vec3f edge1 = v_sub(VERT(cell.verts[cell.faces[resulti + 1]]), v0v);
        vec3f edge2 = v_sub(VERT(cell.verts[cell.faces[resulti + 2]]), v0v);
        vec3f normalV = v_cross3(edge1, edge2);
        Point3_vec4 norm;
        v_st(&norm.x, v_norm3(normalV));
        *normal = norm;
#undef VERT
        return true;
      }
      else
        return false;
    }
    if (resulti)
      ht_ret = v_extract_x(v_madd_x(maxht, v_splat_y(pkScale), v_splat_y(pkOfs)));
    return resulti;
  }
  void calcVertHighestHorizon(vec4f &high, int ci, vec3f v_pos)
  {
    const LandRayCell &cell = cells[ci];
    vec4f pkScale = cell.scale;
    vec4f pkOfs = cell.ofs;
    vec3f v_offseted_pos = v_sub(v_pos, pkOfs);
    int vertCnt = ((ci < cells.size() - 1) ? cells[ci + 1].verts : allVerts.data() + allVerts.size()) - cell.verts;
    for (int vi = 0; vi < vertCnt; ++vi)
    {
      vec3f vert = v_mul(pkScale, v_cvt_vec4f(v_lduush(cell.verts[vi].v)));
      vec3f dir = v_norm3(v_sub(vert, v_offseted_pos));
      high = v_max(high, dir);
    }
    high = v_splat_y(high);
  }
  void calcFaceHighestHorizon(vec4f &high, int ci, vec3f v_pos)
  {
    const LandRayCell &cell = cells[ci];
    // int fiend = fistart + cell.grid[gridSize*gridSize];
    vec4f pkScale = cell.scale;
    vec4f pkOfs = cell.ofs;
    vec3f v_offseted_pos = v_sub(v_pos, pkOfs);
    int faceCnt = getCellFaceCount(ci);
    // for (int face = 0; face < faceCnt; face+=3)

    // bruteforce horizon
    for (int face = 0; face < faceCnt; face += 3)
    {
#define VERT(J) v_mul(pkScale, v_cvt_vec4f(v_lduush(cell.verts[cell.faces[face + (J)]].v)))
      vec3f vert0 = VERT(0);
      vec3f vert1 = VERT(1);
      vec3f vert2 = VERT(2);
#undef VERT
      high = v_max(high, v_norm3(v_sub(closest_point_on_segment(v_offseted_pos, vert0, vert1), v_offseted_pos)));
      high = v_max(high, v_norm3(v_sub(closest_point_on_segment(v_offseted_pos, vert1, vert2), v_offseted_pos)));
      high = v_max(high, v_norm3(v_sub(closest_point_on_segment(v_offseted_pos, vert0, vert2), v_offseted_pos)));
    }
    high = v_splat_y(high);
  }
  float calcHighestHorizon(vec3f v_pos)
  {
    vec4f cellCoord = v_mul(v_pos, invCellSizeV);
    vec4f cellCoordFloored = v_floor(cellCoord);
    vec4f cellCoordClamped = v_clamp(cellCoordFloored, v_zero(), v_sub(numCellsV, V_C_ONE));
    vec4i cellIndices = v_cvt_vec4i(cellCoordClamped);
    int startCell = v_extract_zi(cellIndices) * numCellsX + v_extract_xi(cellIndices);

    vec4f high = v_neg(V_C_ONE);

    calcFaceHighestHorizon(high, startCell, v_pos);
    calcVertHighestHorizon(high, startCell, v_pos);
    int startCellX = startCell % numCellsX;
    int startCellY = startCell / numCellsX;

    vec4f shortscale = v_make_vec4f(65535, 65535, 65535, 0);

    for (int cy = 0, ci = 0; cy < numCellsY; ++cy)
      for (int cx = 0; cx < numCellsX; ++cx, ++ci)
      {
        if (ci == startCell)
          continue;
        const LandRayCell &cell = cells[ci];
        vec4f pkScale = cell.scale;
        vec4f pkOfs = cell.ofs;
        bbox3f cellBox;
        cellBox.bmin = pkOfs;
        cellBox.bmax = v_madd(shortscale, pkScale, pkOfs);
        vec3f boxClosestPoint = v_closest_bbox_point(cellBox.bmin, cellBox.bmax, v_pos);
        boxClosestPoint = v_sel(cellBox.bmax, boxClosestPoint, v_cast_vec4f(V_CI_MASK1011));
        vec3f boxDir;
        boxDir = v_norm3(v_sub(boxClosestPoint, v_pos));
        if (v_test_vec_x_lt(v_splat_y(boxDir), high))
          continue;
        calcVertHighestHorizon(high, startCell, v_pos);
        if (abs(startCellY - cy) + abs(startCellX - cx) <= 2) // we calculate face horizon only in nearby cells
        {
          if (v_test_vec_x_lt(v_splat_y(boxDir), high))
            continue;
          calcFaceHighestHorizon(high, ci, v_pos);
        }
      }
    return min(1.0f, v_extract_x(high) * 1.1f); // this is needed only due to inaccuracy of horizon determination
  }

  void debugDumpCells()
  {
    for (int i = 0; i < (int)cells.size() - 1; ++i)
    {
      const LandRayCell &cell = cells[i];
      uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
      uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;

      int vertCnt = ((i < cells.size() - 1) ? cells[i + 1].verts : allVerts.data() + allVerts.size()) - cell.verts;
      int faceCnt = getCellFaceCount(i);

      int maxFaces = 0, minFaces = 100000;
      int totalFaces = (cells[i + 1].fistart_gridsize >> GRIDSIZE_BITS) - fistart;
      for (int gi = 0; gi < gridSize * gridSize; ++gi)
      {
        if (totalFaces > 40000)
          debug("gi=%dx%d faces=%d", gi / gridSize, gi % gridSize, (int)cell.grid[gi + 1] - (int)cell.grid[gi]);
        maxFaces = max(maxFaces, (int)cell.grid[gi + 1] - (int)cell.grid[gi]),
        minFaces = min(minFaces, (int)cell.grid[gi + 1] - (int)cell.grid[gi]);
      }
      debug("%d cell all faces=%d (grid indices %d == %d), verts=%d grid=%d, gridSize=%d, faces: max=%d, min=%d,avg=%d", i, faceCnt,
        totalFaces, cell.grid[gridSize * gridSize], vertCnt, cells[i + 1].grid - cell.grid, gridSize, maxFaces, minFaces,
        gridSize > 0 ? ((cells[i + 1].fistart_gridsize >> GRIDSIZE_BITS) - fistart) / (gridSize * gridSize) : 0);
    }
  }

  typedef uint32_t bitword;
  enum
  {
    BITWORD_BITS = 5,
    BITWORD_MASK = (1 << BITWORD_BITS) - 1
  };

  bool getFaces(const LandRayCell &cell, int faceIndCount, int gridX0, int gridZ0, int gridX1, int gridZ1, bbox3f_cref worldBBox,
    vec4f *__restrict triangles, int &left) // each triangle is vert0, edge0, edge1
  {
    // DECL_ALIGN16(bitword, used_bits[65536/(sizeof(bitword)*8)]);
    // memset(used_bits, 0, sizeof(used_bits));
    DECL_ALIGN16(bitword, used_bits[(1 << (3 * sizeof(FaceIndex) + 10)) / (sizeof(bitword) * 8)]); // 65536/faces bits for short, and
                                                                                                   // 524288 faces for int

    uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
    if (!gridSize)
      return true;
    int faceBytes = ((faceIndCount / 3 + 7) >> 3);
    G_ASSERTF(faceBytes <= sizeof(used_bits), "faceIndCount = %d > %d", faceBytes, sizeof(used_bits));
    G_ASSERT(faceIndCount % 3 == 0);
    // if (faceBytes > sizeof(used_bits))
    //   return false;
    memset(used_bits, 0, faceBytes);
    uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;
    vec4f pkScale = cell.scale;
    vec4f pkOfs = cell.ofs;

    bbox3f offseted_box;
    offseted_box.bmin = v_sub(worldBBox.bmin, pkOfs);
    offseted_box.bmax = v_sub(worldBBox.bmax, pkOfs);

    for (int gridY = gridZ0; gridY <= gridZ1; ++gridY)
      for (int gridX = gridX0; gridX <= gridX1; ++gridX)
      {
        uint32_t gi = gridX + gridY * gridSize;

        uint32_t efi = cell.grid[gi + 1];

        for (uint32_t i = cell.grid[gi], fi = i + fistart; i < efi; i++, fi++)
        {
          int face = faceIndices[fi];
          int bitwordNo = face >> BITWORD_BITS;
          bitword cmask = 1 << (face & BITWORD_MASK);
          if (used_bits[bitwordNo] & cmask)
            continue;
          used_bits[bitwordNo] |= cmask;
          face *= 3;
#define VERT(J) v_mul(pkScale, v_cvt_vec4f(v_lduush(cell.verts[cell.faces[face + (J)]].v)))
          vec4f vert0 = VERT(0);
          vec4f vert1 = VERT(1);
          vec4f vert2 = VERT(2);
#undef VERT
          bbox3f tribox;
          v_bbox3_init(tribox, vert0);
          v_bbox3_add_pt(tribox, vert1);
          v_bbox3_add_pt(tribox, vert2);
          if (!v_bbox3_test_box_intersect(tribox, offseted_box))
            continue;
          triangles[1] = v_sub(vert1, vert0);
          triangles[2] = v_sub(vert2, vert0);
          // triangles[1] = v_add(vert1, pkOfs);
          // triangles[2] = v_add(vert2, pkOfs);
          triangles[0] = v_perm_xyzd(v_add(vert0, pkOfs), v_cast_vec4f(v_splatsi(0x80000000))); // -0.0 in float and magic number we
                                                                                                // can detect
          triangles += 3;
          left--;
          if (!left)
            return false;
        }
      }
    return true;
  }

  bool getFaces(bbox3f_cref worldBBox, vec4f *__restrict triangles, int &left) // each triangle is vert0, edge0, edge1
  {
    if (!v_bbox3_test_box_intersect(worldBBox, boxV))
      return false;

    vec4f worldBboxXZ = v_perm_xzac(worldBBox.bmin, worldBBox.bmax);
    vec4f regionV = v_sub(worldBboxXZ, offsetVXZ);
    regionV = v_mul(regionV, invCellSizeV);
    regionV = v_clamp(regionV, v_zero(), numCellsVXZ);
    vec4f flooredOfsGridPos1 = v_floor(regionV);
    vec4i regionI = v_cvt_vec4i(flooredOfsGridPos1);

    vec4f inside = v_cmp_eq(flooredOfsGridPos1, v_perm_zwxy(flooredOfsGridPos1));

    vec4f vCellOfs = v_sub(regionV, flooredOfsGridPos1);
    int ci = v_extract_yi(regionI) * numCellsX + v_extract_xi(regionI);

    const LandRayCell &cell = cells[ci];
    int gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;

    DECL_ALIGN16(int, gridXZ[4]);
    v_sti(gridXZ, v_cvt_vec4i(v_mul(vCellOfs, v_splats(gridSize))));

#if _TARGET_SIMD_SSE
    if ((_mm_movemask_ps(inside) & 0x3) == 0x3)
#else
    if (!v_test_vec_x_eqi_0(v_and(v_splat_x(inside), v_splat_y(inside))))
#endif
    {
      // we are in one cell! most common case
      return getFaces(cell, getCellFaceCount(ci), gridXZ[0], gridXZ[1], gridXZ[2], gridXZ[3], worldBBox, triangles, left);
    }
    else
    {
      DECL_ALIGN16(int, cellGrid[4]);
      v_sti(cellGrid, regionI);
      if (cellGrid[2] <= cellGrid[0] + 1 && cellGrid[3] <= cellGrid[1] + 1)
      {
        int origleft = left;
        if (!getFaces(cell, getCellFaceCount(ci), gridXZ[0], gridXZ[1], cellGrid[2] == cellGrid[0] ? gridXZ[2] : gridSize - 1,
              cellGrid[3] == cellGrid[1] ? gridXZ[3] : gridSize - 1, worldBBox, triangles, left))
          return false;
        else
          triangles += (origleft - left) * 3;
        if (cellGrid[2] != cellGrid[0])
        {
          const LandRayCell &curCell = cells[ci + 1];
          uint32_t curGridSize = curCell.fistart_gridsize & GRIDSIZE_MASK;
          DECL_ALIGN16(int, curGridXZ[4]);
          v_sti(curGridXZ, v_cvt_vec4i(v_mul(vCellOfs, v_splats(curGridSize))));
          int curOrigleft = left;
          if (!getFaces(curCell, getCellFaceCount(ci + 1), 0, curGridXZ[1], curGridXZ[2],
                cellGrid[3] == cellGrid[1] ? curGridXZ[3] : (cells[ci + 1].fistart_gridsize & GRIDSIZE_MASK) - 1, worldBBox, triangles,
                left))
            return false;
          else
            triangles += (curOrigleft - left) * 3;
        }
        if (cellGrid[3] != cellGrid[1])
        {
          const LandRayCell &curCell = cells[ci + numCellsX];
          uint32_t curGridSize = curCell.fistart_gridsize & GRIDSIZE_MASK;
          DECL_ALIGN16(int, curGridXZ[4]);
          v_sti(curGridXZ, v_cvt_vec4i(v_mul(vCellOfs, v_splats(curGridSize))));
          int curOrigleft = left;
          if (!getFaces(curCell, getCellFaceCount(ci + numCellsX), curGridXZ[0], 0,
                cellGrid[2] == cellGrid[0] ? curGridXZ[2] : (cells[ci + 1].fistart_gridsize & GRIDSIZE_MASK) - 1, curGridXZ[3],
                worldBBox, triangles, left))
            return false;
          else
            triangles += (curOrigleft - left) * 3;
        }
        if (cellGrid[3] != cellGrid[1] && cellGrid[2] != cellGrid[0])
        {
          const LandRayCell &curCell = cells[ci + numCellsX + 1];
          uint32_t curGridSize = curCell.fistart_gridsize & GRIDSIZE_MASK;
          DECL_ALIGN16(int, curGridXZ[4]);
          v_sti(curGridXZ, v_cvt_vec4i(v_mul(vCellOfs, v_splats(curGridSize))));
          int curOrigleft = left;
          if (!getFaces(curCell, getCellFaceCount(ci + numCellsX + 1), 0, 0, curGridXZ[2], curGridXZ[3], worldBBox, triangles, left))
            return false;
          else
            triangles += (curOrigleft - left) * 3;
        }
        return true;
      }
      else
        return false; // do not make other cases, as it is doesn't make much sense
    }
    return false;
  }

  // return false, if there is no surface within [ht, pos3.y]
  // otherwise result height is in ht
  __forceinline bool getHeightBelow(const Point3 &pos3, float &ht, Point3 *normal)
  {
    if (normal)
      return traceDownFaceVec<false, true>(pos3, ht, normal);
    return traceDownFaceVec<false, false>(pos3, ht, NULL);
  }

  bool getHeightBounding(const Point2 &pos1, float &ht)
  {
    Point2 pos = pos1 - offset2;
    if (pos.x < 0 || pos.y < 0)
      return false;
    Point2 adjpos = pos * invCellSize;
    Point2 cellf, gridPos;
    gridPos.x = modff(adjpos.x, &cellf.x);
    gridPos.y = modff(adjpos.y, &cellf.y);
    int cellX = (int)cellf.x, cellY = (int)cellf.y;
    if (cellX >= numCellsX || cellY >= numCellsY)
      return false;
    uint32_t ci = cellX + cellY * numCellsX;
    const LandRayCell &cell = cells[ci];
    int gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
    if (!gridSize)
      return false;
    uint32_t gridX = (uint32_t)(floorf(gridPos.x * gridSize)), gridY = (uint32_t)(floorf(gridPos.y * gridSize));
    uint32_t gi = gridX + gridY * gridSize;
    ht = cell.gridHt[gi];
    return true;
  }

  enum TraverseResult
  {
    TRAVERSE_RESULT_SKIP,
    TRAVERSE_RESULT_CONTINUE,
    TRAVERSE_RESULT_FINISH
  };
  template <class Shape, class Visitor>
  bool traverse(const Shape &shape, Visitor &visitor)
  {
    const GridParam gridParam = {offset2, cellSize, invCellSize, numCellsX, numCellsY, NULL};
    return traverseProc<Shape, Visitor>(gridParam, shape, visitor);
  }

  template <bool use_normal>
  bool traceRayVec(const Point3 &startOfs, const Point3 &ddir, float &mint, vec3f &normalV)
  {
    mint = min(mint, 1073741824.f); // 1<<30. it is 3 light years, anyway!
    float minY = startOfs.y + ddir.y * mint;
    minY = min(minY, startOfs.y);
    if (minY > bbox[1].y)
      return false;

    vec3f ddirV = v_ldu(&ddir.x);
#ifndef OPTIMIZE_SHORT_RAYS
#define OPTIMIZE_SHORT_RAYS 1
#endif
#if OPTIMIZE_SHORT_RAYS
    vec4f gridPosV = v_ldu(&startOfs.x);
    vec4f mintV = v_splats(mint);
    vec3f endPos = v_add(gridPosV, v_mul(mintV, ddirV));
    bbox3f worldBBox;
    worldBBox.bmin = v_min(gridPosV, endPos);
    worldBBox.bmax = v_max(gridPosV, endPos);
    if (!v_bbox3_test_box_intersect(worldBBox, boxV))
      return false;

    vec4f worldBboxXZ = v_perm_xzac(worldBBox.bmin, worldBBox.bmax);
    vec4f regionV = v_sub(worldBboxXZ, offsetVXZ);
    regionV = v_mul(regionV, invCellSizeV);
    regionV = v_clamp(regionV, v_zero(), numCellsVXZ);
    vec4f flooredOfsGridPos1 = v_floor(regionV);
    vec4i regionI = v_cvt_vec4i(flooredOfsGridPos1);

    vec4f inside = v_cmp_eq(flooredOfsGridPos1, v_perm_zwxy(flooredOfsGridPos1));
#if _TARGET_SIMD_SSE
    if ((_mm_movemask_ps(inside) & 0x3) == 0x3)
#else
    if (!v_test_vec_x_eqi_0(v_and(inside, v_splat_y(inside))))
#endif
    {
      ///*
      int ci = v_extract_yi(regionI) * numCellsX + v_extract_xi(regionI);
      const LandRayCell &cell = cells[ci];
      if (minY > cell.maxHt)
        return false;
      Point3_vec4 mint_p3;
      v_st(&mint_p3.x, mintV);
      if (!traceRayGridVec<use_normal>(ddirV, gridPosV, IPoint2(ci % numCellsX, ci / numCellsX), ddir, Point2::xz(ddir), startOfs,
            Point2::xz(startOfs) - Point2::xz(offset), mint_p3, cell, normalV))
        return false;
      mint = mint_p3.x;
      return true;
    }
#endif

    Point3 startUnOfs(startOfs - offset);
    Point2 dir2d = Point2::xz(ddir);
    Point2 start2d = Point2::xz(startUnOfs);
    IBBox2 limits(IPoint2(0, 0), IPoint2(numCellsX - 1, numCellsY - 1));
    WooRay2d woo(start2d, dir2d, mint, Point2(cellSize, cellSize), limits);
    // write_svg(woo.currentCell().x, woo.currentCell().y, startOfs);
    IPoint2 diff = woo.getEndCell() - woo.currentCell();
    uint16_t n = 2 * (abs(diff.x) + abs(diff.y)) + 1; // limit to 64k steps!
    float t = 0;
    bool ret = false;
    double t_ = 0.0;
    for (; n; n--)
    {
      IPoint2 currentCell = woo.currentCell();
      float curt = t;
      bool nextCell = woo.nextCell(t_);
      if (!nextCell)
        t = mint;
      else
        t = float(t_);
      if (limits & currentCell)
      {
        uint32_t ci = currentCell.x + currentCell.y * numCellsX;
        const LandRayCell &cell = cells[ci];
        if (startOfs.y + min(ddir.y * t, ddir.y * curt) <= cell.maxHt)
        {
          Point3_vec4 gridPos = startOfs + ddir * curt;
          Point3_vec4 mint2_p3;
          mint2_p3.x = min(mint, t) - curt;
          if ((ret = traceRayGridVec<use_normal>(ddirV, v_ld(&gridPos.x), currentCell, ddir, dir2d, gridPos, start2d + dir2d * curt,
                 mint2_p3, cell, normalV)))
          {
            mint = mint2_p3.x + curt;
            break;
          }
        }
      }
      if (!nextCell || mint <= curt)
        break;
    }
    return ret;
  }
  bool traceRay(const Point3 &startOfs, const Point3 &ddir, float &mint, Point3 *normal)
  {
    if (check_nan(startOfs.x + startOfs.z + ddir.x + ddir.z + mint))
#if !DAGOR_DBGLEVEL
      return false;
#else
      G_ASSERTF(0, "%g %g %g %g %g", startOfs.x, startOfs.y, ddir.x, ddir.y, mint);
#endif
    vec3f normalV;
    if (normal)
    {
      bool ret = traceRayVec<true>(startOfs, ddir, mint, normalV);
      if (!ret)
        return false;
      v_stu_p3(&normal->x, v_norm3(normalV));
      return true;
    }
    return traceRayVec<false>(startOfs, ddir, mint, normalV);
  }
  bool rayHit(const Point3 &startOfs, const Point3 &ddir, float mint)
  {
    mint = min(mint, 1073741824.f); // 1<<30, it is 3 light years, anyway!
    if (check_nan(startOfs.x + startOfs.z + ddir.x + ddir.z + mint))
#if !DAGOR_DBGLEVEL
      return false;
#else
      G_ASSERTF(0, "%g %g %g %g %g", startOfs.x, startOfs.y, ddir.x, ddir.y, mint);
#endif
    Point3 startUnOfs(startOfs - offset);
    Point2 dir2d = Point2::xz(ddir);
    Point2 start2d = Point2::xz(startUnOfs);
    IBBox2 limits(IPoint2(0, 0), IPoint2(numCellsX - 1, numCellsY - 1));
    WooRay2d woo(start2d, dir2d, mint, Point2(cellSize, cellSize), limits);
    IPoint2 diff = woo.getEndCell() - woo.currentCell();
    uint16_t n = 2 * (abs(diff.x) + abs(diff.y)) + 1; // limit to 64k steps!
    float t = 0;
    double t_ = 0.0;
    for (; n; n--)
    {
      IPoint2 currentCell = woo.currentCell();
      float curt = t;
      bool nextCell = woo.nextCell(t_);
      if (!nextCell)
        t = mint;
      else
        t = float(t_);
      if (limits & currentCell)
      {
        uint32_t ci = currentCell.x + currentCell.y * numCellsX;
        const LandRayCell &cell = cells[ci];
        if (startOfs.y + min(ddir.y * t, ddir.y * curt) <= cell.maxHt)
        {
          float mint2 = min(mint, t) - curt;
          if (rayHitGrid(currentCell, ddir, dir2d, startOfs + ddir * curt, start2d + dir2d * curt, mint2, mint - curt, cell))
            return true;
        }
      }
      if (!nextCell || mint <= curt)
        break;
    }
    return false;
  }
  void loadStreamToDump(void *dump, IGenLoad &cb, int sz);
  void load(void *dump, int sz);
  void initFromDump();
  void save(IGenSave &cb);
#if _TARGET_PC
  bool build(uint32_t cellsX, uint32_t cellsY, float cellSz, const Point3 &ofs, const BBox3 &box, dag::ConstSpan<Mesh *> meshes,
    dag::ConstSpan<Mesh *> combined_meshes, uint32_t min_grid_index, uint32_t max_grid_index, bool optimize_for_cache);
#endif

protected:
  template <bool use_normal>
  bool traceRayGridNodeVec1(const LandRayCell &cell, uint32_t gi, vec3f scaledDirV, vec3f gridPosV, vec3f &mint, vec3f &normal)
  {
    uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;
    uint32_t sfi = fistart + cell.grid[gi];
    uint32_t efi = fistart + cell.grid[gi + 1];
    bool res = false;
    vec4f pkScale = cell.scale, pkOfs = cell.ofs;
    gridPosV = v_sub(gridPosV, pkOfs);
#define VERT(vertex) v_mul(v_cvt_vec4f(v_lduush((vertex).v)), pkScale)
    for (uint32_t i = sfi; i < efi; ++i)
    {
      uint32_t fi = faceIndices[i] * 3;
      // const Point3 &v0 = cell.verts[cell.faces[fi]];
      vec3f v0v = VERT(cell.verts[cell.faces[fi]]);
      // const Point3 &v1 = cell.verts[cell.faces[fi+1]];
      vec3f edge1 = v_sub(VERT(cell.verts[cell.faces[fi + 1]]), v0v);
      // const Point3 &v2 = cell.verts[cell.faces[fi+2]];
      vec3f edge2 = v_sub(VERT(cell.verts[cell.faces[fi + 2]]), v0v);
      float t = v_extract_x(mint);
      bool ret = ::traceRayToTriangleCullCCW(gridPosV, scaledDirV, t, v0v, edge1, edge2);
      mint = v_set_x(t);
      res |= ret;
      if (ret && use_normal)
        normal = v_cross3(edge1, edge2);
#undef VERT
    }
    return res;
  }

  template <bool use_normal>
  bool traceRayGridNodeVec(const LandRayCell &cell, uint32_t gi, vec3f scaledDirV, vec3f gridPosV, vec3f &mint, vec3f &normal)
  {

    uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;
    uint32_t sfi = fistart + cell.grid[gi];
    uint32_t efi = fistart + cell.grid[gi + 1];

    bool result = false;
    DECL_ALIGN16(uint32_t, fi[4]);
    vec4f pkScale = cell.scale, pkOfs = cell.ofs;
    gridPosV = v_sub(gridPosV, pkOfs);
#define VERT(I, J) v_mul(v_cvt_vec4f(v_lduush(cell.verts[cell.faces[fi[I] + (J)]].v)), pkScale)

    for (uint32_t i = sfi; i < efi; i += 4)
    {
      fi[0] = faceIndices[i + 0] * 3;
      fi[1] = faceIndices[i + 1] * 3;
      fi[2] = faceIndices[i + 2] * 3;
      fi[3] = faceIndices[i + 3] * 3;

      mat43f p0, p1, p2;
#if _TARGET_ANDROID
      load_4_tri(p0, p1, p2, cell.faces, cell.verts, fi, pkScale);
#else
      v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
      v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
      v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);
#endif

      float t = v_extract_x(mint);
      int ret = traceray4Triangles(gridPosV, scaledDirV, t, p0, p1, p2, false);
      mint = v_splats(t);
      if (ret >= 0)
      {
        result = true;
        if (use_normal)
        {
          vec3f v0 = VERT(ret, 0);
          normal = v_cross3(v_sub(VERT(ret, 1), v0), v_sub(VERT(ret, 2), v0));
        }
      }
    }
    return result;
  }
  template <bool use_normal>
  bool traceRayGridVec(vec3f scaledDirV, vec3f gridPosV, const IPoint2 &cell_id, const Point3 &scaledDir, const Point2 &scaledDir2d,
    const Point3 &gridPos, const Point2 &gridPos2d, Point3_vec4 &mintp3, const LandRayCell &cell, vec3f &normalV)
  {
    vec3f mintV = v_splat_x(v_ld(&mintp3.x));
    uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
    if (!gridSize)
      return false;
    IPoint2 cellGrid = cell_id * gridSize;
    IBBox2 limits(cellGrid, IPoint2(gridSize - 1, gridSize - 1) + cellGrid);
    float gridCellSize = cellSize / gridSize;
    WooRay2d woo(gridPos2d, scaledDir2d, mintp3.x, Point2(gridCellSize, gridCellSize), limits);

    limits[0] -= cellGrid;
    limits[1] -= cellGrid;
    bool ret = false;

    float t = 0, curt = 0;
    double t_ = 0.0;
    for (;;) // infite that never should happen
    {
      IPoint2 currentCell = woo.currentCell() - cellGrid;
      curt = t;
      bool nextCell = woo.nextCell(t_);
      if (!nextCell)
        t = mintp3.x;
      else
        t = float(t_);
      if (limits & currentCell)
      {
        // trace through grid node
        uint32_t gi = currentCell.x + currentCell.y * gridSize;
        if (gridPos.y + min(scaledDir.y * t, scaledDir.y * curt) <= cell.gridHt[gi])
        {
          if (traceRayGridNodeVec<use_normal>(cell, gi, scaledDirV, gridPosV, mintV, normalV))
          {
            v_st(&mintp3.x, mintV);
            ret = true;
          }
        }
      }
      if (!nextCell || mintp3.x <= curt)
        break;
    }
    return ret;
  }

  __forceinline bool rayHitGridNode(const LandRayCell &cell, uint32_t gi, uint32_t fistart, vec3f scaledDirV, vec3f gridPosV,
    vec4f mint)
  {
    uint32_t sfi = fistart + cell.grid[gi];
    uint32_t efi = fistart + cell.grid[gi + 1];
    DECL_ALIGN16(uint32_t, fi[4]);
    vec4f pkScale = cell.scale, pkOfs = cell.ofs;
    gridPosV = v_sub(gridPosV, pkOfs);
#define VERT(I, J) v_mul(v_cvt_vec4f(v_lduush(cell.verts[cell.faces[fi[I] + (J)]].v)), pkScale)

    for (uint32_t i = sfi; i < efi; i += 4)
    {
      fi[0] = faceIndices[i + 0] * 3;
      fi[1] = faceIndices[i + 1] * 3;
      fi[2] = faceIndices[i + 2] * 3;
      fi[3] = faceIndices[i + 3] * 3;

      mat43f p0, p1, p2;
#if _TARGET_ANDROID
      load_4_tri(p0, p1, p2, cell.faces, cell.verts, fi, pkScale);
#else
      v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
      v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
      v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);
#endif

      if (rayhit4Triangles(gridPosV, scaledDirV, v_extract_x(mint), p0, p1, p2, false))
        return true;
    }
    return false;
  }

  bool rayHitGrid(const IPoint2 &cell_id, const Point3_vec4 &scaledDir, const Point2 &scaledDir2d, const Point3_vec4 &gridPos,
    const Point2 &gridPos2d, float mint, float mint_check, const LandRayCell &cell)
  {
    uint32_t gridSize = cell.fistart_gridsize & GRIDSIZE_MASK;
    if (gridSize == 0)
      return false;
    uint32_t fistart = cell.fistart_gridsize >> GRIDSIZE_BITS;
    IPoint2 cellGrid = cell_id * gridSize;
    IBBox2 limits(cellGrid, IPoint2(gridSize - 1, gridSize - 1) + cellGrid);
    float gridCellSize = cellSize / gridSize;
    WooRay2d woo(gridPos2d, scaledDir2d, mint, Point2(gridCellSize, gridCellSize), limits); // Normalized

    limits[0] -= cellGrid;
    limits[1] -= cellGrid;

    IPoint2 diff = woo.getEndCell() - woo.currentCell();
    uint32_t n = 2 * (abs(diff.x) + abs(diff.y)) + 1;
    float t = 0, start = 0, curt = 0;
    double t_ = 0.0;
    vec3f scaledDirV = v_ld(&scaledDir.x);
    vec3f gridPosV = v_ld(&gridPos.x);
    vec4f mintV = v_splats(mint_check);
    for (; n; n--)
    {
      IPoint2 currentCell = woo.currentCell() - cellGrid;
      // debug("%dx%d : %dx%d", cell_id, currentCell);
      start = curt;
      curt = t;
      bool nextCell = woo.nextCell(t_);
      if (!nextCell)
        t = mint;
      else
        t = float(t_);
      if (limits & currentCell)
      {
        // trace through grid node
        uint32_t gi = currentCell.x + currentCell.y * gridSize;
        if (gridPos.y + min(scaledDir.y * t, scaledDir.y * curt) <= cell.gridHt[gi])
          if (rayHitGridNode(cell, gi, fistart, scaledDirV, gridPosV, mintV))
            return true;
      }
      if (!nextCell || mint <= curt)
        break;
    }
    return false;
  }
  friend class LandRaySaver;
};

class LandRayTracer : public BaseLandRayTracer<uint16_t, uint16_t>
{
public:
  LandRayTracer(IMemAlloc *allocator = midmem) : BaseLandRayTracer<uint16_t, uint16_t>(allocator) {}
};

template <class VertIndex, class FaceIndex>
GlobalSharedMemStorage *BaseLandRayTracer<VertIndex, FaceIndex>::sharedMem = NULL;
