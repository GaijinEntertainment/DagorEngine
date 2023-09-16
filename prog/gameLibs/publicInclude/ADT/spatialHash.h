//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/array.h>
#include <debug/dag_assert.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_adjpow2.h>
#include <math/integer/dag_IBBox2.h>

/*
 This is Abstact Data Type (ADT) known as Spatial Hash (or Spatial Grid), which basically spatial index
 (which is used for spatial queries)

 This implementation is designed with following restrictions/design decisions at hand:

 * (This is arguably most debatable and need to be futher investigaed). Material points only.
   I.e. object sizes are not considered.
   Pros: point (i.e. object) might be only in one cell at once (no need to filter out dups when quering objects).
   It's also simpler (less code, which might be better from i-cache point of view)
   Cons: you either miss objects near cell edges or have to add nearest cells to queries as well (which hurts perfomance)
 * No locks of any kind. This is intentional. Attempt to use this as shared resource among several threads
   kills your perfomance (even if it lockless)
 * This hash is better suited for sparsely distributed (on level) objects.
 * 2D grid. It's just pure practicality. This is what is used & required right now.
 * Works with specific type (Point3). This is also for practicality (to avoid need to convert it manually Point2 on each operation)
*/

const int SPATIAL_HASH_DEFAULT_CELL_SIZE = 32;

template <typename CellType, unsigned gridSize, bool rayCheck>
class SpatialHash2DBoxRayIterator
{
public:
  typedef eastl::array<CellType, gridSize * gridSize> CellsArray;
  typedef typename CellType::const_iterator ObjectIterator;
  typedef typename CellType::value_type ObjectType;

  template <typename ObjectsIterator>
  __forceinline bool checkObjectBounding(const ObjectsIterator &objects_iterator, const ObjectType &object) const
  {
    if constexpr (rayCheck)
    {
      vec4f pos2d = v_perm_xzxz(object.getWBSph());
      int mask = v_signmask(v_cmp_lt(pos2d, v_perm_xzac(extendedBox.bmin, extendedBox.bmax)));
      if (mask != ((1 << 2) | (1 << 3)))
        return false;
      return objects_iterator.checkObjectBounding(object, from, dir, len, radius);
    }
    else
      return objects_iterator.checkObjectBounding(object, queryBox);
  }

  template <typename ObjectsIterator>
  __forceinline const ObjectType *foreach(const ObjectsIterator &objects_iterator)
  {
    do
    {
      const CellType &cv = cellsData[z * gridSize + x];
      for (ObjectIterator it = cv.begin(), end = cv.end(); EASTL_LIKELY(it != end); it++)
      {
        v_prefetch(&*eastl::next(it));
        const ObjectType &object = *it;
        if (checkObjectBounding(objects_iterator, object) && EASTL_UNLIKELY(objects_iterator.predFunc(&object)))
          return &object;
      }
    } while (advance());
    return nullptr;
  }

  unsigned size() const
  {
    int xsize = maxX + 1 - x;
    int zsize = maxZ + 1 - z;
    if (xsize < 0)
      xsize += gridSize;
    if (zsize < 0)
      zsize += gridSize;
    return xsize * zsize;
  }

private:
  template <typename, unsigned>
  friend class SpatialHash2D;

  __forceinline SpatialHash2DBoxRayIterator(const CellType *grid_cells, IBBox2 limits, bbox3f query_box, bbox3f ext_box) :
    cellsData(grid_cells),
    x(limits.lim[0].x),
    z(limits.lim[0].y),
    maxX(limits.lim[1].x),
    maxZ(limits.lim[1].y),
    minX(limits.lim[0].x),
    queryBox(query_box),
    extendedBox(ext_box)
  {}

  __forceinline SpatialHash2DBoxRayIterator(const CellType *grid_cells, IBBox2 limits, bbox3f query_box, bbox3f ext_box,
    vec3f ray_from, vec3f ray_dir, vec4f ray_len, const vec4f &ray_radius) :
    SpatialHash2DBoxRayIterator(grid_cells, limits, query_box, ext_box)
  {
    from = ray_from;
    dir = ray_dir;
    len = ray_len;
    radius = ray_radius;
  }

  __forceinline bool advance()
  {
    if (EASTL_LIKELY(x == maxX))
    {
      if (EASTL_LIKELY(z == maxZ))
        return false;
      z = (z + 1) & (gridSize - 1);
      x = minX - 1;
    }
    x = (x + 1) & (gridSize - 1);
    return true;
  }

  const CellType *__restrict cellsData;
  unsigned x, z;
  unsigned maxX, maxZ;
  unsigned minX;
  bbox3f queryBox;
  bbox3f extendedBox;
  vec3f from, dir;
  vec4f len, radius;
};

template <typename CellType, unsigned gridSize = 32> // Number of cells in one row/column. Power of 2 is required.
class SpatialHash2D
{
public:
  static_assert(is_pow2(gridSize), "gridSize must be power of 2!");

  typedef SpatialHash2DBoxRayIterator<CellType, gridSize, false> BoxIterator;
  typedef SpatialHash2DBoxRayIterator<CellType, gridSize, true> RayIterator;
  typedef eastl::array<CellType, gridSize * gridSize> CellsArray;
  typedef typename CellType::value_type ObjectType;

  SpatialHash2D()
  {
    cellSizeLog2 = get_const_log2(SPATIAL_HASH_DEFAULT_CELL_SIZE);
    gridSizeMinus1 = gridSize - 1;
    boxOverflowWidth = float((gridSize - 1) * getCellSize());
    maxObjBoundingRadius = 0.f;
  }

  ~SpatialHash2D() { clear(); }

  __forceinline BoxIterator getBoxIterator(bbox3f bbox, bool extend_by_max_object_bounding) const
  {
    bbox3f extBBox = bbox;
    if (extend_by_max_object_bounding)
      v_bbox3_extend(extBBox, v_splats(maxObjBoundingRadius));
    return BoxIterator(cells.data(), getBoxLimits(extBBox), bbox, extBBox);
  }

  __forceinline RayIterator getRayIterator(vec3f from, vec3f dir, vec4f len, vec4f radius, bool extend_by_max_object_bounding) const
  {
    bbox3f bbox;
    v_bbox3_init_by_ray(bbox, from, dir, len);
    v_bbox3_extend(bbox, radius);
    bbox3f extBBox = bbox;
    if (extend_by_max_object_bounding)
      v_bbox3_extend(extBBox, v_splats(maxObjBoundingRadius));
    return RayIterator(cells.data(), getBoxLimits(extBBox), bbox, extBBox, from, dir, len, radius);
  }

  void insert(ObjectType &val, vec3f pos, float radius)
  {
    insertAt(val, hashFn(pos));
    maxObjBoundingRadius = max(maxObjBoundingRadius, radius);
  }

  // Remove object by position. Return false if wasn't found.
  void erase(ObjectType &val, vec3f pos) { eraseAt(val, hashFn(pos)); }

  // Update object's position. Object assumed to be present in old position (won't be added if it's not).
  void update(ObjectType &val, vec3f old_pos, vec3f new_pos, float new_radius)
  {
    vec4i vOldCellIds = v_srli_n(v_cvt_floori(old_pos), constants);
    vec4i vNewCellIds = v_srli_n(v_cvt_floori(new_pos), constants);
    vec4f cmp = v_cast_vec4f(v_cmp_eqi(vNewCellIds, vOldCellIds));
    maxObjBoundingRadius = max(maxObjBoundingRadius, new_radius);
    if (EASTL_LIKELY((v_signmask(cmp) & 0b0101) == 0b0101)) // Assume that objects are not that often crosses cells borders (hence
                                                            // likely)
      return;
    unsigned oldIdx = hashFn(old_pos), newIdx = hashFn(new_pos);
    eraseAt(val, oldIdx);
    insertAt(val, newIdx);
  }

  void setCellSizeWithoutObjectsReposition(unsigned cellSize)
  {
    if (is_pow2(cellSize))
    {
      cellSizeLog2 = get_log2i_of_pow2(cellSize);
      boxOverflowWidth = float((gridSize - 1) * getCellSize());
    }
    else
      logerr("SpatialHash2D: %s(%u) error, new value should be power of 2", __FUNCTION__, cellSize);
  }

  void setCellSizeSlow(unsigned cellSize)
  {
    if (!is_pow2(cellSize))
    {
      logerr("SpatialHash2D: %s(%u) error, new value should be power of 2", __FUNCTION__, cellSize);
      return;
    }
    cellSizeLog2 = get_log2i_of_pow2(cellSize);
    boxOverflowWidth = float((gridSize - 1) * getCellSize());
    CellsArray oldCells;
    eastl::swap(cells, oldCells);
    for (CellType &oldCell : oldCells)
    {
      while (!oldCell.empty())
      {
        ObjectType &obj = oldCell.front();
        oldCell.remove(obj);
        insertAt(obj, hashFn(obj.wbsph));
      }
    }
  }

  float getMaxExtension() const { return maxObjBoundingRadius; }

  float getMaxObjectBoundingRadius() const { return maxObjBoundingRadius; }

  unsigned getCellSize() const { return 1 << uint8_t(cellSizeLog2); }

  unsigned getGridWidth() const { return gridSize; }

  unsigned getGridHeight() const { return gridSize; }

  const CellType &getCellByIndex(unsigned x, unsigned z) const { return cells[z * gridSize + x]; };

  void clear()
  {
    for (auto &cv : cells)
      cv.clear();
    maxObjBoundingRadius = 0.f;
  }

  struct GridCellsStats
  {
    int totalCells;
    int notEmptyCells;
    int totalObjects;
    int maxObjectsInCell;
  };

  void getCellsStats(GridCellsStats &stats) const
  {
    memset(&stats, 0, sizeof(GridCellsStats));
    stats.totalCells = getGridWidth() * getGridHeight();
    for (const CellType &c : cells)
    {
      if (!c.empty())
      {
        int objectCount = c.size();
        stats.notEmptyCells++;
        stats.totalObjects += objectCount;
        stats.maxObjectsInCell = max(stats.maxObjectsInCell, int(objectCount));
      }
    }
  }

private:
  __forceinline IBBox2 getBoxLimits(bbox3f bbox) const
  {
    IBBox2 limits;
    vec4f box2d = v_perm_xzac(bbox.bmin, bbox.bmax);
    vec4i vGridSizeMinus1 = v_splat_zi(constants);
    vec4f vBoxWidth = v_sub(v_perm_zwzw(box2d), v_perm_xyxy(box2d));
    vec4f vBoxOverflow = v_cmp_ge(vBoxWidth, v_splat_w(v_cast_vec4f(constants)));
    vec4i vCellIds = v_srli_n(v_cvt_floori(box2d), constants);
    vec4i vMaxBoxWidth = v_permi_xycd(v_zeroi(), vGridSizeMinus1);
    vCellIds = v_seli(vCellIds, vMaxBoxWidth, v_cast_vec4i(vBoxOverflow)); // if iterator box side >= gridSize, replace it by [0;
                                                                           // gridSize)
    vCellIds = v_andi(vCellIds, vGridSizeMinus1);
    v_stui(&limits, vCellIds);
    return limits;
  }
  void insertAt(ObjectType &val, unsigned cell_id)
  {
    CellType &cell = cells.data()[cell_id];
    for (ObjectType &obj : cell)
    {
      if (ptrdiff_t(&val) < ptrdiff_t(&obj))
      {
        cell.insert(typename CellType::iterator(&obj), val);
        return;
      }
    }
    cell.push_back(val);
  }
  void eraseAt(ObjectType &val, unsigned cell_id) { cells.data()[cell_id].remove(val); }
  unsigned hashFn(vec3f pos) const
  {
    vec4i vCellIds = v_srli_n(v_cvt_floori(pos), constants);
    vec4i vGridSizeMinus1 = v_splat_zi(constants);
    vCellIds = v_andi(vCellIds, vGridSizeMinus1);
    unsigned x = v_extract_xi(vCellIds);
    unsigned z = v_extract_zi(vCellIds);
    return z * gridSize + x;
  }

  union
  {
    vec4i constants;
    struct
    {
      uint64_t cellSizeLog2;
      int gridSizeMinus1;
      float boxOverflowWidth;
    };
  };
  float maxObjBoundingRadius;
  CellsArray cells;
};
