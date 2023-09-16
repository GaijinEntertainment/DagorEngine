#pragma once

#include <rendInst/rendInstGen.h>

#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <math/dag_wooray2d.h>
#include <osApiWrappers/dag_rwLock.h>


class alignas(16) RiGrid
{
public:
  static constexpr int CELLS_PER_TILE = 16;
  struct CellsTile
  {
    bbox3f tileBox;
    carray<uint16_t, CELLS_PER_TILE * CELLS_PER_TILE> cellTabCount;
    carray<rendinst::riex_handle_t *, CELLS_PER_TILE * CELLS_PER_TILE> cellTabPtr;
    carray<bbox3f, CELLS_PER_TILE * CELLS_PER_TILE> cellBb;

    CellsTile()
    {
      mem_set_0(cellTabCount);
      mem_set_0(cellTabPtr);
      mem_set_0(cellBb);
      tileBox = cellBb[0];
    }
    ~CellsTile();
    void add(int cx, int cz, rendinst::riex_handle_t h, bbox3f_cref wbb);
    bool del(int cx, int cz, rendinst::riex_handle_t h, bbox3f_cref wbb);
    void upd(int cx, int cz, bbox3f_cref old_wbb, bbox3f_cref new_wbb);
    void updMaxExt(int cx, int cz, float tx0, float tz0, float tsize);

    static inline int getIdx(int cx, int cz) { return cz * CELLS_PER_TILE + cx; }

    // cellTabTotalCount[] stored in cellBb[].bmin.w
    int &cellTabTotalCount(int idx) { return ((int *)&cellBb[idx])[3]; }
    int cellTabTotalCount(int idx) const { return ((int *)&cellBb[idx])[3]; }
    // objCount stored in tileBox.bmin.w
    unsigned &objCount() { return ((unsigned *)&tileBox)[3]; }
    unsigned objCount() const { return ((unsigned *)&tileBox)[3]; }
    // maxExtWidth stored in tileBox.bmax.w
    float &maxExtWidth() { return ((float *)&tileBox)[7]; }
    float maxExtWidth() const { return ((float *)&tileBox)[7]; }
  };

  RiGrid()
  {
    objCount = 0;
    maxExtWidth = 0;
    cellSize = 1 << 20;
    tileX0 = 0, tileZ0 = 0;
    tileX1 = 0, tileZ1 = 0;
    cc = NULL;
    ccCells = NULL;
  }
  ~RiGrid() { term(); }

  void init(float cell_size, bool need_cc);
  void term();

  void insert(rendinst::riex_handle_t h, const Point3 &pos, bbox3f_cref wbb);
  bool remove(rendinst::riex_handle_t h, const Point3 &pos, bbox3f_cref wbb);
  void update(rendinst::riex_handle_t h, const Point3 &old_pos, const Point3 &new_pos, bbox3f_cref old_wbb, bbox3f_cref new_wbb);

  float getX0() const { return (tileX0 * CELLS_PER_TILE - CELLS_PER_TILE / 2) * cellSize; }
  float getZ0() const { return (tileZ0 * CELLS_PER_TILE - CELLS_PER_TILE / 2) * cellSize; }
  float getCellSize() const { return cellSize; }
  float getTileSize() const { return CELLS_PER_TILE * cellSize; }
  bool hasLocks() const { return cc != NULL; }
  bool hasObjects() const { return objCount > 0; }

  struct alignas(16) BoxLimits
  {
    int tx0, tz0, tx1, tz1;
    int cx0, cz0, cx1, cz1;
    int tix0, tiz0, tix1, tiz1;

    void getCellsLimits(int tx, int tz, int &out_cx0, int &out_cz0, int &out_cx1, int &out_cz1)
    {
      out_cx0 = max(tx * CELLS_PER_TILE, cx0) - tx * CELLS_PER_TILE;
      out_cz0 = max(tz * CELLS_PER_TILE, cz0) - tz * CELLS_PER_TILE;
      out_cx1 = min((tx + 1) * CELLS_PER_TILE, cx1) - tx * CELLS_PER_TILE;
      out_cz1 = min((tz + 1) * CELLS_PER_TILE, cz1) - tz * CELLS_PER_TILE;
    }
  };
  bool startBoxIterator(bbox3f_cref box, BoxLimits &out_box) const;
  void startRayIterator(WooRay2d &inout_ray, const float *p0_len, const float *norm_dir, float len) const;
  void finishIterator() const;

  template <typename T>
  inline bool iterateBox(bbox3f_cref box, T process_iterated_handles) const;
  template <typename T>
  inline bool iterateRay(const Point3 &p0, const Point3 &norm_dir, float len, T process_iterated_handles) const;

  const CellsTile *getTile(int tx, int tz) const
  {
    G_ASSERTF(tx >= tileX0 && tx < tileX1 && tz >= tileZ0 && tz < tileZ1, "t=%d,%d  tile=(%d,%d)-(%d,%d)", tx, tz, tileX0, tileZ0,
      tileX1, tileZ1);
    int idx = (tz - tileZ0) * (tileX1 - tileX0) + tx - tileX0;
    G_ASSERTF(idx >= 0 && idx < tiles.size(), "idx=%d tiles=%d", idx, tiles.size());
    return tiles[idx];
  }
  const CellsTile *getTileChecked(int tx, int tz) const
  {
    if (tx < tileX0 || tx >= tileX1 || tz < tileZ0 || tz >= tileZ1)
      return NULL;
    return tiles[(tz - tileZ0) * (tileX1 - tileX0) + tx - tileX0];
  }

  void dumpGridState();

protected:
  int tileX0, tileZ0, tileX1, tileZ1;
  float cellSize;
  unsigned objCount;
  Tab<CellsTile *> tiles;
  SmartReadWriteFifoLock *cc;
  SmartReadWriteLock *ccCells;
  float maxExtWidth;

  void rearrange(int include_tx, int include_tz);
  void rearrange(int tx0, int tz0, int tx1, int tz1);
  inline void posToCell(const Point3 &pos, int &out_cx, int &out_cz);
  inline void cellToTile(int &inout_cx, int &inout_cz, int &out_tx, int &out_tz);
};

template <typename T>
inline bool RiGrid::iterateBox(bbox3f_cref box, T process_iterated_handles) const
{
  BoxLimits blim;
  if (!startBoxIterator(box, blim))
    return false;

  for (int tz = blim.tz0; tz < blim.tz1; tz++)
    for (int tx = blim.tx0; tx < blim.tx1; tx++)
    {
      const RiGrid::CellsTile *t = getTile(tx, tz);
      if (!t || !t->objCount() || !v_bbox3_test_box_intersect(box, t->tileBox))
        continue;

      if (tx >= blim.tix0 && tx < blim.tix1 && tz >= blim.tiz0 && tz < blim.tiz1)
      {
        // tile fully inside
        for (int ci = 0; ci < t->cellTabCount.size(); ci++)
        {
          int cnt = t->cellTabCount[ci];
          if (cnt && v_bbox3_test_box_intersect(box, t->cellBb[ci]))
            if (process_iterated_handles(t->cellTabPtr[ci], cnt))
            {
              finishIterator();
              return true;
            }
        }
      }
      else
      {
        // tile partially under box
        int cx0, cz0, cx1, cz1;
        blim.getCellsLimits(tx, tz, cx0, cz0, cx1, cz1);
        for (int cz = cz0; cz < cz1; cz++)
          for (int cx = cx0; cx < cx1; cx++)
          {
            int ci = t->getIdx(cx, cz);
            int cnt = t->cellTabCount[ci];
            if (cnt && v_bbox3_test_box_intersect(box, t->cellBb[ci]))
              if (process_iterated_handles(t->cellTabPtr[ci], cnt))
              {
                finishIterator();
                return true;
              }
          }
      }
    }
  finishIterator();
  return false;
}
template <typename T>
inline bool RiGrid::iterateRay(const Point3 &p0, const Point3 &dir, float len, T process_iterated_handles) const
{
  IBBox2 limits(IPoint2(0, 0), IPoint2((tileX1 - tileX0) * CELLS_PER_TILE - 1, (tileZ1 - tileZ0) * CELLS_PER_TILE - 1));
  WooRay2d ray(Point2(p0[0] - getX0(), p0[2] - getZ0()), Point2(dir[0], dir[2]), len, Point2(cellSize, cellSize), limits);
  StaticTab<unsigned, 16> visited_cells;
  const CellsTile *t = NULL;
  int tx = INT_MIN, tz = INT_MIN;
  int max_ocx = CELLS_PER_TILE * (tileX1 - tileX0) - 1;
  int max_ocz = CELLS_PER_TILE * (tileZ1 - tileZ0) - 1;

  startRayIterator(ray, &p0.x, &dir.x, len);

  Point3_vec4 seg_p0 = p0;
  vec3f v_p0 = v_ld(&p0.x);
  vec3f v_p1 = v_madd(v_ld(&dir.x), v_splats(len), v_p0);
  const vec4i tileOfs = v_make_vec4i(CELLS_PER_TILE / 2 - tileX0 * CELLS_PER_TILE, CELLS_PER_TILE / 2 - tileZ0 * CELLS_PER_TILE,
    CELLS_PER_TILE / 2 - tileX0 * CELLS_PER_TILE, CELLS_PER_TILE / 2 - tileZ0 * CELLS_PER_TILE);
  const float extInCell = maxExtWidth / cellSize;
  const vec4f cellExt = v_make_vec4f(-extInCell, -extInCell, extInCell, extInCell);
  const vec4f invCellSize = v_splats(1. / cellSize);
  for (int iter_state = 0; iter_state < 2;)
  {
    vec4f segP0Cell = v_madd(v_perm_xzxz(v_ld(&seg_p0.x)), invCellSize, cellExt);
    vec4i segP0CellFloored = v_cvt_vec4i(v_perm_xycd(v_floor(segP0Cell), v_ceil(segP0Cell)));
    vec4i ocxz =
      v_mini(v_maxi(v_addi(segP0CellFloored, tileOfs), v_cast_vec4i(v_zero())), v_make_vec4i(max_ocx, max_ocz, max_ocx, max_ocz));
    alignas(16) int oc[4];
    v_sti(oc, ocxz);
    const int ocx0 = oc[0], ocz0 = oc[1], ocx1 = oc[2], ocz1 = oc[3];

    if (iter_state == 0)
    {
      double curT = 0;
      if (ray.nextCell(curT) && curT < len)
        seg_p0 = p0 + dir * float(curT);
      else
        v_st(&seg_p0.x, v_p1), iter_state = 1;
    }
    else
      iter_state++;

    for (int cz = ocz0; cz < ocz1; cz++)
      for (int cx = ocx0; cx < ocx1; cx++)
      {
        unsigned c_idx = (cz << 16) + cx;
        int ntx = tileX0 + cx / CELLS_PER_TILE;
        int ntz = tileZ0 + cz / CELLS_PER_TILE;
        if (ntx != tx || ntz != tz)
        {
          tx = ntx;
          tz = ntz;
          t = getTileChecked(tx, tz);
          if (t && !t->objCount())
            t = NULL;
        }
        if (!t || find_value_idx(visited_cells, c_idx) >= 0)
          continue;

        int idx = t->getIdx(cx % CELLS_PER_TILE, cz % CELLS_PER_TILE);
        if (int cnt = t->cellTabCount[idx])
        {
          if (!v_test_segment_box_intersection(v_p0, v_p1, t->cellBb[idx]))
            continue;

          if (process_iterated_handles(t->cellTabPtr[idx], cnt))
          {
            finishIterator();
            return true;
          }
        }

        if (visited_cells.size() > 4)
          for (int i = 0, idx_x = ray.currentCell().x << 16, idx_y = ray.currentCell().y << 16; i < visited_cells.size(); i++)
            if (abs(idx_x - int(visited_cells[i] << 16)) > 0x10000 || abs(idx_y - int(visited_cells[i] & 0xFFFF0000)) > 0x10000)
              erase_items(visited_cells, i, 1);
        visited_cells.push_back(c_idx);
      }
  }
  finishIterator();
  return false;
}
