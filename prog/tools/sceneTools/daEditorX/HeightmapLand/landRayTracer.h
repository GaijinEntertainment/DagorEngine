// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdlib.h>
#include <util/dag_globDef.h>
#include <math/dag_math3d.h>
#include <math/dag_wooray2d.h>
#include <math/dag_mathUtils.h>
#include <math/dag_traceRayTriangle.h>
#include <generic/dag_tab.h>

class Mesh;

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
  __forceinline int getCellFaceIndexCount(int ci) const // face indices count, i.e. faces*3
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
  Point3 offset;
  Point2 offset2;

  IMemAlloc *mem;
  BuildStorage *buildStor;

  void setBBox(const BBox3 &b, const Point3 &ofs, float cellSz)
  {
    bbox = b;
    offset = ofs;
    offset2 = Point2::xz(offset);
    cellSize = cellSz;
    invCellSize = 1.0f / cellSize;
  }

public:
  enum
  {
    MIN_GRID_SIZE = 8,
    MAX_GRID_SIZE = GRIDSIZE_MASK
  };

  //-V:BaseLandRayTracer:730 grid mirrors and spans are set by build before any query
  BaseLandRayTracer(IMemAlloc *allocator = midmem) : mem(allocator), buildStor(NULL), cells(allocator) {}
  ~BaseLandRayTracer();
  const BBox3 &getBBox() const { return bbox; }
  uint32_t getNumCellsX() const { return numCellsX; }
  uint32_t getNumCellsY() const { return numCellsY; }
  float getCellSize() const { return cellSize; }
  const Point3 &getOffset() const { return offset; }
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
  // Cell geometry enumeration: world-space vertices in a stable order plus triangle index
  // triples into that order. Mesh consumers (phys cooking, collision previews) use these
  // instead of the raw packed spans, so the tracer's storage stays an implementation detail.
  int getCellVertCount(int idx) const { return (int)getCellVerts(idx).size(); }
  int getCellTriCount(int idx) const { return getCellFaceIndexCount(idx) / 3; }
  template <class CB>
  void iterateCellVertices(int idx, CB cb) const // cb(const Point3 &world_pos)
  {
    const vec4f s = cells[idx].scale, o = cells[idx].ofs;
    for (const Vertex &vert : getCellVerts(idx))
    {
      Point3 p;
      v_stu_p3(&p.x, unpack_vert(vert, s, o));
      cb(p);
    }
  }
  template <class CB>
  void iterateCellFaces(int idx, CB cb) const // cb(int v0, int v1, int v2), indices into iterateCellVertices' order
  {
    dag::ConstSpan<VertIndex> f = getCellFaces(idx);
    for (int i = 0; i + 2 < (int)f.size(); i += 3)
      cb((int)f[i], (int)f[i + 1], (int)f[i + 2]);
  }

  __forceinline bool getHeight(const Point2 &pos1, float &ht, Point3 *normal)
  {
    if (normal)
      return traceDownFaceVec<true, true>(Point3(pos1.x, 0, pos1.y), ht, normal);
    return traceDownFaceVec<true, false>(Point3(pos1.x, 0, pos1.y), ht, NULL);
  }
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
    if (!v_check_xz_all_true(inside))
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
      v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
      v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
      v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);

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
  // return false, if there is no surface within [ht, pos3.y]
  // otherwise result height is in ht
  __forceinline bool getHeightBelow(const Point3 &pos3, float &ht, Point3 *normal)
  {
    if (normal)
      return traceDownFaceVec<false, true>(pos3, ht, normal);
    return traceDownFaceVec<false, false>(pos3, ht, NULL);
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
#if _TARGET_PC
  bool build(uint32_t cellsX, uint32_t cellsY, float cellSz, const Point3 &ofs, const BBox3 &box, dag::ConstSpan<Mesh *> meshes,
    dag::ConstSpan<Mesh *> combined_meshes, uint32_t min_grid_index, uint32_t max_grid_index);
#endif

protected:
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
      v_mat44_transpose_to_mat43(VERT(0, 0), VERT(1, 0), VERT(2, 0), VERT(3, 0), p0.row0, p0.row1, p0.row2);
      v_mat44_transpose_to_mat43(VERT(0, 1), VERT(1, 1), VERT(2, 1), VERT(3, 1), p1.row0, p1.row1, p1.row2);
      v_mat44_transpose_to_mat43(VERT(0, 2), VERT(1, 2), VERT(2, 2), VERT(3, 2), p2.row0, p2.row1, p2.row2);

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
};
