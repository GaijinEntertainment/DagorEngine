#include "riGen/riGrid.h"

#include <math/dag_bounds3.h>
#include <math/dag_adjpow2.h>
#include <math/dag_math2d.h>
#include <vecmath/dag_vecMath_common.h>
#include <math/dag_vecMathCompatibility.h>
#include <startup/dag_globalSettings.h>


#define LOCK_WR(NM) /*if (cc) cc->lockWrite(NM)*/
#define UNLOCK_WR() /*if (cc) cc->unlockWrite()*/
#define LOCK_RD(NM) /*if (cc) cc->lockRead(NM) */
#define UNLOCK_RD() /*if (cc) cc->unlockRead() */

#define LOCK_CELLS_WR(NM) /*if (ccCells) ccCells->lockWrite(NM)*/
#define UNLOCK_CELLS_WR() /*if (ccCells) ccCells->unlockWrite()*/
#define LOCK_CELLS_RD(NM) /*if (ccCells) ccCells->lockRead(NM) */
#define UNLOCK_CELLS_RD() /*if (ccCells) ccCells->unlockRead() */


static inline bool is_box_on_edge(bbox3f_cref full_bb, bbox3f_cref bb)
{
  vec4i v_eq = v_cast_vec4i(v_or(v_cmp_eq(full_bb.bmin, bb.bmin), v_cmp_eq(full_bb.bmax, bb.bmax)));
  return v_extract_xi(v_ori(v_ori(v_eq, v_srli(v_eq, 8)), v_ori(v_srli(v_eq, 16), v_srli(v_eq, 24)))) != 0;
}

void RiGrid::posToCell(const Point3 &pos, int &out_cx, int &out_cz)
{
  if (check_nan(pos.x) || check_nan(pos.z))
  {
    out_cx = out_cz = -(int)0x20000000;
    G_ASSERTF(!check_nan(pos.x) || !check_nan(pos.z), "pos=%@", pos);
    return;
  }
  vec4i cxzi = v_cvt_floori(v_add(v_div(v_perm_xzxz(v_ldu(&pos.x)), v_splats(cellSize)), v_splats((CELLS_PER_TILE / 2))));
  out_cx = v_extract_xi(cxzi);
  out_cz = v_extract_yi(cxzi);
}
void RiGrid::cellToTile(int &inout_cx, int &inout_cz, int &out_tx, int &out_tz)
{
  out_tx = (inout_cx - (inout_cx >= 0 ? 0 : CELLS_PER_TILE - 1)) / CELLS_PER_TILE;
  out_tz = (inout_cz - (inout_cz >= 0 ? 0 : CELLS_PER_TILE - 1)) / CELLS_PER_TILE;
  inout_cx -= out_tx * CELLS_PER_TILE;
  inout_cz -= out_tz * CELLS_PER_TILE;
}

RiGrid::CellsTile::~CellsTile()
{
  for (int i = 0; i < cellTabPtr.size(); i++)
    if (cellTabPtr[i])
      midmem->free(cellTabPtr[i]);
  mem_set_0(cellTabPtr);
  mem_set_0(cellTabCount);
  mem_set_0(cellBb);
  tileBox = cellBb[0];
}
void RiGrid::CellsTile::add(int cx, int cz, rendinst::riex_handle_t h, bbox3f_cref wbb)
{
  int idx = getIdx(cx, cz);
  G_ASSERTF_RETURN(idx >= 0 && idx < cellTabCount.size(), , "cx=%d cz=%d idx=%d", cx, cz, idx);
  int totalCnt = cellTabTotalCount(idx);
  if (cellTabCount[idx] + 1 > totalCnt)
  {
    G_ASSERTF_RETURN(totalCnt <= 0xFF00, , "cellTabTotalCount(%d)=%d", idx, totalCnt);
    unsigned nn = (cellTabCount[idx] + 8) & ~0x7;
    size_t s = 0;
    cellTabPtr[idx] = (rendinst::riex_handle_t *)midmem->realloc(cellTabPtr[idx], nn * sizeof(rendinst::riex_handle_t), &s);
    totalCnt = int16_t(s / sizeof(rendinst::riex_handle_t));
  }

  G_ASSERTF_RETURN(cellTabPtr[idx], , "Cannot find a place at (%d, %d) cell for new handle", cx, cz);

  unsigned nn = cellTabCount[idx];
  unsigned objCnt = objCount();
  float extW = maxExtWidth();

  cellTabPtr[idx][nn] = h;
  cellTabCount[idx]++;
  if (nn)
    v_bbox3_add_box(cellBb[idx], wbb);
  else
    cellBb[idx] = wbb;
  if (objCnt)
    v_bbox3_add_box(tileBox, wbb);
  else
    tileBox = wbb, extW = 0;

  objCount() = objCnt + 1;
  maxExtWidth() = extW;
  cellTabTotalCount(idx) = totalCnt;
}
bool RiGrid::CellsTile::del(int cx, int cz, rendinst::riex_handle_t h, bbox3f_cref wbb)
{
  int idx = getIdx(cx, cz);
  G_ASSERTF_RETURN(idx >= 0 && idx < cellTabCount.size(), false, "cx=%d cz=%d idx=%d", cx, cz, idx);
  rendinst::riex_handle_t *ptr = cellTabPtr[idx];
  for (int i = 0, cnt = cellTabCount[idx]; i < cnt; i++)
    if (ptr[i] == h)
    {
      if (cnt - i - 1 > 0)
        memmove(ptr + i, ptr + i + 1, (cnt - i - 1) * sizeof(rendinst::riex_handle_t));
      cellTabCount[idx]--;
      unsigned objCnt = objCount();
      int totalCnt = cellTabTotalCount(idx);
      float extW = maxExtWidth();
      if (is_box_on_edge(cellBb[idx], wbb))
      {
        //== egde box, should recompute
      }
      objCount() = objCnt - 1;
      maxExtWidth() = extW;
      cellTabTotalCount(idx) = totalCnt;
      return true;
    }
  return false;
}
void RiGrid::CellsTile::upd(int cx, int cz, bbox3f_cref old_wbb, bbox3f_cref new_wbb)
{
  int idx = getIdx(cx, cz);
  if (!cellTabCount[idx])
    return;

  // save values stored in tileBox
  int objCountSave = objCount();
  int maxExtWidthSave = maxExtWidth();
  v_bbox3_add_box(cellBb[idx], new_wbb);
  v_bbox3_add_box(tileBox, new_wbb);
  objCount() = objCountSave;
  maxExtWidth() = maxExtWidthSave;
  if (is_box_on_edge(cellBb[idx], old_wbb))
  {
    //== egde box, should recompute
  }
}
void RiGrid::CellsTile::updMaxExt(int cx, int cz, float x0, float z0, float csize)
{
  int idx = getIdx(cx, cz);
  const bbox3f &bb = cellBb[idx];
  x0 += cx * csize;
  z0 += cz * csize;
  inplace_max(maxExtWidth(), x0 - as_point3(&bb.bmin).x);
  inplace_max(maxExtWidth(), z0 - as_point3(&bb.bmin).z);
  inplace_max(maxExtWidth(), as_point3(&bb.bmax).x - x0 - csize);
  inplace_max(maxExtWidth(), as_point3(&bb.bmax).z - z0 - csize);
  G_ASSERTF(maxExtWidth() < csize, "maxExtWidth=%.1f cellSize=%.0f bb=%@-%@ c0=%.1f,%.1f objCount=%d %p(%d)", maxExtWidth(), csize,
    as_point3(&bb.bmin), as_point3(&bb.bmax), x0, z0, objCount(), this, idx);
}

void RiGrid::init(float cell_size, bool need_cc)
{
  term();

  objCount = 0;
  maxExtWidth = 0;
  cellSize = cell_size;
  if (cell_size >= 128)
    tileX0 = 0, tileZ0 = 0, tileX1 = 1, tileZ1 = 1;
  else
    tileX0 = -1, tileZ0 = -1, tileX1 = 2, tileZ1 = 2;
  tiles.clear();
  tiles.reserve(32);
  tiles.resize((tileX1 - tileX0) * (tileZ1 - tileZ0));
  mem_set_0(tiles);
  if (need_cc)
  {
    // cc = new SmartReadWriteFifoLock;
    // ccCells = new SmartReadWriteLock;
  }
}
void RiGrid::term()
{
  if (cc)
  {
    LOCK_WR("term");
  }
  clear_all_ptr_items(tiles);
  cellSize = 1 << 20;
  tileX0 = 0, tileZ0 = 0;
  tileX1 = 0, tileZ1 = 0;
  objCount = 0;
  if (cc)
  {
    UNLOCK_WR();
  }
  del_it(cc);
  del_it(ccCells);
}

void RiGrid::insert(rendinst::riex_handle_t h, const Point3 &pos, bbox3f_cref wbb)
{
  LOCK_RD("insert");
  int cx, cz, tx, tz;
  posToCell(pos, cx, cz);
  cellToTile(cx, cz, tx, tz);
  // debug("%p.insert(%08X, %@) -> t=%d,%d c=%d,%d", this, h, pos, tx, tz, cx, cz);
  if (tx < tileX0 || tz < tileZ0 || tx >= tileX1 || tz >= tileZ1)
    rearrange(tx, tz);
  G_ASSERTF_RETURN(tx >= tileX0 && tz >= tileZ0 && tx < tileX1 && tz < tileZ1, , "c=%d,%d t=%d,%d tiles=(%d,%d)-(%d,%d)", cx, cz, tx,
    tz, tileX0, tileZ0, tileX1, tileZ1);
  G_ASSERTF_RETURN(cx >= 0 && cx < CELLS_PER_TILE && cz >= 0 && cz < CELLS_PER_TILE, , "cx=%d cz=%d, tx=%d tz=%d", cx, cz, tx, tz);
  int tidx = (tz - tileZ0) * (tileX1 - tileX0) + (tx - tileX0);
  if (!tiles[tidx])
    tiles[tidx] = new CellsTile;
  LOCK_CELLS_WR("add");
  tiles[tidx]->add(cx, cz, h, wbb);
  tiles[tidx]->updMaxExt(cx, cz, getX0() + (tx - tileX0) * getTileSize(), getZ0() + (tz - tileZ0) * getTileSize(), cellSize);
  inplace_max(maxExtWidth, tiles[tidx]->maxExtWidth());
  UNLOCK_CELLS_WR();
  objCount++;
  UNLOCK_RD();
}
bool RiGrid::remove(rendinst::riex_handle_t h, const Point3 &pos, bbox3f_cref wbb)
{
  LOCK_RD("remove");
  int cx, cz, tx, tz;
  posToCell(pos, cx, cz);
  cellToTile(cx, cz, tx, tz);
  // debug("%p.remove(%08X, %@) -> t=%d,%d c=%d,%d", this, h, pos, tx, tz, cx, cz);
  if (tx < tileX0 || tz < tileZ0 || tx >= tileX1 || tz >= tileZ1)
  {
    UNLOCK_RD();
    return false;
  }
  int tidx = (tz - tileZ0) * (tileX1 - tileX0) + (tx - tileX0);
  G_ASSERT(tiles[tidx]);
  if (!tiles[tidx])
  {
    UNLOCK_RD();
    return false;
  }
  LOCK_CELLS_WR("del");
  if (!tiles[tidx]->del(cx, cz, h, wbb))
  {
    UNLOCK_CELLS_WR();
    UNLOCK_RD();
    return false;
  }
  UNLOCK_CELLS_WR();
  objCount--;
  UNLOCK_RD();
  return true;
}
void RiGrid::update(rendinst::riex_handle_t h, const Point3 &old_pos, const Point3 &new_pos, bbox3f_cref old_wbb, bbox3f_cref new_wbb)
{
  LOCK_RD("update");
  int cx0, cz0, cx1, cz1;
  posToCell(old_pos, cx0, cz0);
  posToCell(new_pos, cx1, cz1);
  if (cx0 == cx1 && cz0 == cz1)
  {
    int tx, tz;
    cellToTile(cx0, cz0, tx, tz);
    int tidx = (tz - tileZ0) * (tileX1 - tileX0) + (tx - tileX0);
    G_ASSERT(tiles[tidx]);
    LOCK_CELLS_WR("upd");
    // debug("%p.update(%08X, %@, %@) -> t=%d,%d c=%d,%d  objCount=%d", this, h, old_pos, new_pos, tx, tz, cx0, cz0,
    // tiles[tidx]->objCount);
    int cidx = CellsTile::getIdx(cx0, cz0);
    if (tiles[tidx] && tiles[tidx]->objCount() && tiles[tidx]->cellTabCount[cidx])
    {
      tiles[tidx]->upd(cx0, cz0, old_wbb, new_wbb);
      tiles[tidx]->updMaxExt(cx0, cz0, getX0() + (tx - tileX0) * getTileSize(), getZ0() + (tz - tileZ0) * getTileSize(), cellSize);
      inplace_max(maxExtWidth, tiles[tidx]->maxExtWidth());
    }
    UNLOCK_CELLS_WR();
    UNLOCK_RD();
    return;
  }
  UNLOCK_RD();
  if (remove(h, old_pos, old_wbb))
    insert(h, new_pos, new_wbb);
}

void RiGrid::rearrange(int tx0, int tz0, int tx1, int tz1)
{
  G_ASSERTF_RETURN(tx1 > tx0 && tz1 > tz0, , "(%d,%d)-(%d,%d)", tx0, tz0, tx1, tz1);
  G_ASSERTF_RETURN((tx1 - tx0) * CELLS_PER_TILE < 0xFFFF && (tz1 - tz0) * CELLS_PER_TILE < 0xFFFF, , "(%d,%d)-(%d,%d) dim=%dx%d cells",
    tx0, tz0, tx1, tz1, (tx1 - tx0) * CELLS_PER_TILE, (tz1 - tz0) * CELLS_PER_TILE);

  LOCK_WR("rearrange");
  Tab<CellsTile *> new_tiles;
  new_tiles.resize((tx1 - tx0) * (tz1 - tz0));
  mem_set_0(new_tiles);
  for (int z = tileZ0; z < tileZ1; z++)
    for (int x = tileX0; x < tileX1; x++)
      if (x >= tx0 && x < tx1 && z >= tz0 && z < tz1)
      {
        new_tiles[(z - tz0) * (tx1 - tx0) + (x - tx0)] = tiles[(z - tileZ0) * (tileX1 - tileX0) + (x - tileX0)];
        tiles[(z - tileZ0) * (tileX1 - tileX0) + (x - tileX0)] = NULL;
      }

  clear_all_ptr_items(tiles);
  tiles = eastl::move(new_tiles);
  tileX0 = tx0, tileZ0 = tz0;
  tileX1 = tx1, tileZ1 = tz1;
  UNLOCK_WR();
}
void RiGrid::rearrange(int include_tx, int include_tz)
{
  int tx0 = tileX0, tz0 = tileZ0, tx1 = tileX1, tz1 = tileZ1;
  if (tx0 > include_tx)
    tx0 = include_tx;
  else if (tx1 <= include_tx)
    tx1 = include_tx + 1;
  if (tz0 > include_tz)
    tz0 = include_tz;
  else if (tz1 <= include_tz)
    tz1 = include_tz + 1;
  rearrange(tx0, tz0, tx1, tz1);
}

bool RiGrid::startBoxIterator(bbox3f_cref box, RiGrid::BoxLimits &out_box) const
{
  LOCK_RD("boxIter");
  vec4f box2d = v_perm_xzac(box.bmin, box.bmax);
  /* Unvectorized:
  out_box.cx0 = (int)floorf((box2d[0][0]-maxExtWidth+(CELLS_PER_TILE/2)*cellSize)/cellSize);
  out_box.cz0 = (int)floorf((box2d[0][1]-maxExtWidth+(CELLS_PER_TILE/2)*cellSize)/cellSize);
  out_box.cx1 = (int) ceilf((box2d[1][0]+maxExtWidth+(CELLS_PER_TILE/2)*cellSize)/cellSize);
  out_box.cz1 = (int) ceilf((box2d[1][1]+maxExtWidth+(CELLS_PER_TILE/2)*cellSize)/cellSize);

  out_box.tx0 = clamp((int)floorf(out_box.cx0/(float)CELLS_PER_TILE), tileX0, tileX1);
  out_box.tz0 = clamp((int)floorf(out_box.cz0/(float)CELLS_PER_TILE), tileZ0, tileZ1);
  out_box.tx1 = clamp((int)ceilf(out_box.cx1/(float)CELLS_PER_TILE), tileX0, tileX1);
  out_box.tz1 = clamp((int)ceilf(out_box.cz1/(float)CELLS_PER_TILE), tileZ0, tileZ1);*/

  vec4f cxz = v_add(v_div(v_add(box2d, v_make_vec4f(-maxExtWidth, -maxExtWidth, maxExtWidth, maxExtWidth)), v_splats(cellSize)),
    v_splats(CELLS_PER_TILE / 2));
  cxz = v_perm_xycd(v_floor(cxz), v_ceil(cxz));
  vec4i cxzi = v_cvt_vec4i(cxz);
  // check v_cvt_vec4i result for indefinite integer value (0x80000000) that mean signed integer overflow
  if (EASTL_UNLIKELY(v_signmask(v_cast_vec4f(v_cmp_eqi(cxzi, V_CI_SIGN_MASK)))))
  {
    Point4 b2d;
    v_stu(&b2d.x, box2d);
    logerr("RiGrid::startBoxIterator int overflow in cx-cz, box2d=(%f %f, %f %f)", P4D(b2d));
    UNLOCK_RD();
    return false;
  }

  v_sti(&out_box.cx0, cxzi);
  vec4f clampTo = v_cast_vec4f(v_ldi(&tileX0));
  vec4f txz = v_div(cxz, v_splats((float)CELLS_PER_TILE));
  v_sti(&out_box.tx0, v_maxi(v_mini(v_cvt_vec4i(v_perm_xycd(v_floor(txz), v_ceil(txz))), v_cast_vec4i(v_perm_zwzw(clampTo))),
                        v_cast_vec4i(v_perm_xyxy(clampTo))));
  if (EASTL_UNLIKELY(out_box.tx0 >= out_box.tx1 || out_box.tz0 >= out_box.tz1))
  {
    UNLOCK_RD();
    return false;
  }

  out_box.tix0 = (out_box.tx0 * CELLS_PER_TILE >= out_box.cx0) ? out_box.tx0 : out_box.tx0 + 1;
  out_box.tiz0 = (out_box.tz0 * CELLS_PER_TILE >= out_box.cz0) ? out_box.tz0 : out_box.tz0 + 1;
  out_box.tix1 = (out_box.tx1 * CELLS_PER_TILE <= out_box.cx1) ? out_box.tx1 : out_box.tx1 - 1;
  out_box.tiz1 = (out_box.tz1 * CELLS_PER_TILE <= out_box.cz1) ? out_box.tz1 : out_box.tz1 - 1;
  LOCK_CELLS_RD("iterBox");
  return true;
}
void RiGrid::startRayIterator(WooRay2d &, const float *, const float *, float) const
{
  LOCK_RD("rayIter");
  LOCK_CELLS_RD("iterRay");
}
void RiGrid::finishIterator() const
{
  UNLOCK_CELLS_RD();
  UNLOCK_RD();
}

void RiGrid::dumpGridState()
{
  LOCK_RD("dump");
  int tiles_used = 0;
  for (int ti = 0; ti < tiles.size(); ti++)
    if (tiles[ti])
      tiles_used++;
  debug("grid(%p) cellSize=%.0f tile=(%d,%d)-(%d,%d) tiles=%d (%d used) cc=%p objs=%d maxExt=%.1f", this, cellSize, tileX0, tileZ0,
    tileX1, tileZ1, tiles.size(), tiles_used, cc, objCount, maxExtWidth);
  for (int tz = tileZ0; tz < tileZ1; tz++)
    for (int tx = tileX0; tx < tileX1; tx++)
    {
      const CellsTile *t = getTile(tx, tz);
      if (!t)
      {
        debug("  tile(%+d,%+d)=empty", tx, tz, t);
        continue;
      }
      LOCK_CELLS_RD("dump");
      int cells_used = 0;
      for (int j = 0; j < t->cellTabCount.size(); j++)
        if (t->cellTabCount[j])
          cells_used++;
      debug("  tile(%+d,%+d)=%p objs=%d (%d cells used) maxExt=%.1f tileBox=%@-%@ loc=%@", tx, tz, t, t->objCount(), cells_used,
        t->maxExtWidth(), as_point3(&t->tileBox.bmin), as_point3(&t->tileBox.bmax),
        BBox2(Point2(getX0() + (tx - tileX0) * CELLS_PER_TILE * cellSize, getZ0() + (tz - tileZ0) * CELLS_PER_TILE * cellSize),
          Point2(getX0() + (tx + 1 - tileX0) * CELLS_PER_TILE * cellSize, getZ0() + (tz + 1 - tileZ0) * CELLS_PER_TILE * cellSize)));
      for (int j = 0; j < t->cellTabCount.size(); j++)
      {
        if (!t->cellTabCount[j])
          continue;
        debug("    cell(%2d,%2d) count=%d (total=%d) bbox=%@-%@", j % CELLS_PER_TILE, j / CELLS_PER_TILE, t->cellTabCount[j],
          t->cellTabTotalCount(j), as_point3(&t->cellBb[j].bmin), as_point3(&t->cellBb[j].bmax));
        for (int k = 0; k < t->cellTabCount[j]; k++)
          debug("      %08X", t->cellTabPtr[j][k]);
      }
      UNLOCK_CELLS_RD();
    }
  UNLOCK_RD();
}
