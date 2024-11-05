//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EditorCore/ec_interface.h>
#include <util/dag_hierBitMap2d.h>
#include <math/random/dag_random.h>
#include <math/dag_e3dColor.h>
#include <de3_multiPointData.h>
#include <de3_genObjHierMask.h>

#include <debug/dag_debug.h>
namespace objgenerator
{
struct WorldHugeBitmask
{
  HugeBitmask *bm;
  float ox, oz, scale;
  int w, h; // in fact, just duplicates dimensions stored in bm

  WorldHugeBitmask() : bm(NULL), ox(0), oz(0), scale(1), w(0), h(0) {}
  ~WorldHugeBitmask() { clear(); }

  void init(float min_x, float min_z, float max_x, float max_z, int target_sz, float min_cell_sz)
  {
    clear();
    if (min_x >= max_x || min_z >= max_z)
      return;

    ox = min_x;
    oz = min_z;
    scale = max(max_x - min_x, max_z - min_z) / target_sz;
    inplace_max(scale, min_cell_sz);
    scale = 1.0f / scale;
    w = (max_x - min_x) * scale;
    h = (max_z - min_z) * scale;
    bm = new HugeBitmask(w, h);
  }

  void initEven(float min_x, float min_z, float max_x, float max_z, int tgt_sz, float min_cellsz)
  {
    init(floorf(min_x), floorf(min_z), ceilf(max_x), ceilf(max_z), tgt_sz, min_cellsz);
  }

  void initExplicit(float min_x, float min_z, float s, int wd, int ht)
  {
    clear();
    ox = min_x;
    oz = min_z;
    scale = s;
    w = wd;
    h = ht;
    bm = new HugeBitmask(w, h);
  }

  bool updateFrom(WorldHugeBitmask &m, IBBox2 &out_bb)
  {
    if (!m.bm && !bm)
      return false;

    if (bm && m.bm && w == m.w && h == m.h && fabs(ox - m.ox) < 1e-3 && fabs(oz - m.oz) < 1e-3 && fabs(scale - m.scale) < 1e-5)
    {
      IBBox2 bb;
      if (!m.bm->compare(*bm, 0, 0, bb[0].x, bb[0].y, bb[1].x, bb[1].y))
        return false;

      out_bb = bb;
    }
    else if (!bm && m.bm)
    {
      IBBox2 bb;
      m.bm->addSetBitsMinMax(0, 0, bb[0].x, bb[0].y, bb[1].x, bb[1].y);
      out_bb = bb;
    }
    else
    {
      out_bb[0].x = 0;
      out_bb[0].y = 0;
      out_bb[1].x = w - 1;
      out_bb[1].y = h - 1;
    }

    clear();
    *this = m;
    m.bm = NULL;
    m.clear();
    return true;
  }

  void clear()
  {
    del_it(bm);
    ox = oz = 0;
    scale = 1;
    w = h = 0;
  }

  inline void shade(int y, int x, int w)
  {
    for (; w > 0; x++, w--)
      bm->set(x, y);
  }

  inline void mark(float px, float pz)
  {
    int ix = (px - ox) * scale;
    int iy = (pz - oz) * scale;
    if (ix >= 0 && iy >= 0 && ix < w && iy < h)
      bm->set(ix, iy);
  }

  inline void mark3x3(float px, float pz)
  {
    int ix = int((px - ox) * scale) - 1, ixe = ix + 3 <= w ? ix + 3 : w;
    int iy = int((pz - oz) * scale) - 1, iye = iy + 3 <= h ? iy + 3 : h;
    for (iy = iy < 0 ? 0 : iy, ix = ix < 0 ? 0 : ix; iy < iye; iy++)
      for (int x = ix; x < ixe; x++)
        bm->set(x, iy);
  }

  inline bool isMarked(float px, float pz)
  {
    if (!bm)
      return false;
    int ix = (px - ox) * scale;
    int iz = (pz - oz) * scale;

    if (ix < 0 || iz < 0 || ix >= w || iz >= h)
      return false;

    return bm->get(ix, iz);
  }
};

extern WorldHugeBitmask lcmapExcl;
extern WorldHugeBitmask splgenExcl;

static inline bool is_place_allowed(float px, float pz) { return !lcmapExcl.isMarked(px, pz); }
static inline bool is_place_allowed_g(float px, float pz) { return !splgenExcl.isMarked(px, pz); }

static inline float dist_to_ground(const Point3 &p0, float start_above = 0)
{
  real dist = EDITORCORE->getMaxTraceDistance() + start_above;
  Point3 p = p0;
  p.y += start_above;
  if (EDITORCORE->traceRay(p, Point3(0, -1, 0), dist, NULL, false))
    p.y -= dist;
  else
    p.y -= start_above;
  return p0.y - p.y;
}

static inline void place_on_plane(Point3 &p, const Point3 &n, float start_above = 0)
{
  real dist = EDITORCORE->getMaxTraceDistance() + start_above;
  p += start_above * n;
  if (EDITORCORE->traceRay(p, -n, dist, NULL, false))
    p -= dist * n;
  else
    p -= start_above * n;
}

static inline void place_on_ground(Point3 &p, float start_above = 0) { place_on_plane(p, Point3(0, 1, 0), start_above); }

static inline void place_on_ground(Point3 &p, Point3 &out_norm, float start_above = 0)
{
  float dist = EDITORCORE->getMaxTraceDistance() + start_above;
  Point3 orig_norm = out_norm;
  p.y += start_above;
  if (EDITORCORE->traceRay(p, Point3(0, -1, 0), dist, &out_norm, false))
    p.y -= dist;
  else
  {
    p.y -= start_above;
    out_norm = orig_norm;
  }
}

template <class T>
static void calc_matrix_33(T &obj, TMatrix &tm, int &seed, float force_x_scale = 0)
{
  float ax, ay, az;
  _rnd_svec(seed, az, ay, ax);
  float s = obj.scale[0] + _srnd(seed) * obj.scale[1], sz = s;
  if (force_x_scale > 0)
    s = force_x_scale, sz = 1;
  ax = obj.rotX[0] + ax * obj.rotX[1];
  ay = obj.rotY[0] + ay * obj.rotY[1];
  az = obj.rotZ[0] + az * obj.rotZ[1];
  float sy = (obj.yScale[0] + _srnd(seed) * obj.yScale[1]) * (force_x_scale > 0 ? obj.scale[0] / force_x_scale : 1);
  int mask = (float_nonzero(ax) << 0) | (float_nonzero(ay) << 1) | (float_nonzero(az) << 2) | (float_nonzero(s - 1) << 3) |
             (float_nonzero(sy - 1) << 4) | (float_nonzero(sz - 1) << 5);
  if (!mask)
    return;

  Matrix3 m3;
  memcpy(m3.m, tm.m, 9 * sizeof(float));

  if (mask & 4)
    m3 = m3 * rotzM3(az);
  if (mask & 1)
    m3 = m3 * rotxM3(ax);
  if (mask & 2)
    m3 = m3 * rotyM3(ay);

  if (mask & (8 | 16 | 32))
  {
    Matrix3 stm;
    stm.setcol(0, Point3(s, 0, 0));
    stm.setcol(1, Point3(0, s * sy, 0));
    stm.setcol(2, Point3(0, 0, sz));
    m3 = m3 * stm;
  }

  memcpy(tm.m, m3.m, 9 * sizeof(float));
}


static inline void place_multipoint(MpPlacementRec &mppRec, Point3 &worldPos, TMatrix &rot_tm, float start_above = 0)
{
  mppRec.mpPoint1 = rot_tm * mppRec.mpPoint1 + worldPos;
  place_on_ground(mppRec.mpPoint1, start_above);

  mppRec.mpPoint2 = rot_tm * mppRec.mpPoint2 + worldPos;
  place_on_ground(mppRec.mpPoint2, start_above);

  if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_3POINT)
  {
    mppRec.mpPoint3 = rot_tm * mppRec.mpPoint3 + worldPos;
    place_on_ground(mppRec.mpPoint3, start_above);
    worldPos = mppRec.mpPoint1 + (mppRec.mpPoint2 - mppRec.mpPoint1) * mppRec.pivotBc21 +
               (mppRec.mpPoint3 - mppRec.mpPoint1) * mppRec.pivotBc31;
  }
  else
    place_on_ground(worldPos, start_above);
}


static inline void rotate_multipoint(TMatrix &tm, MpPlacementRec &mppRec)
{
  Point3 v_orient = tm.getcol(1);
  TMatrix mpp_tm = TMatrix::IDENT;
  Point3 place_vec = normalize(mppRec.mpPoint1 - mppRec.mpPoint2);

  if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_YZ)
  {
    mpp_tm.setcol(0, normalize(v_orient % place_vec));
    mpp_tm.setcol(1, normalize(place_vec % mpp_tm.getcol(0)));
    mpp_tm.setcol(2, normalize(mpp_tm.getcol(0) % mpp_tm.getcol(1)));
  }
  else if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_XY)
  {
    mpp_tm.setcol(2, normalize(place_vec % v_orient));
    mpp_tm.setcol(1, normalize(mpp_tm.getcol(2) % place_vec));
    mpp_tm.setcol(0, normalize(mpp_tm.getcol(1) % mpp_tm.getcol(2)));
  }
  else if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_3POINT)
  {
    Point3 place_vec2 = normalize(mppRec.mpPoint3 - mppRec.mpPoint2);
    Point3 v_norm = normalize(place_vec2 % place_vec);
    if (lengthSq(v_norm) < VERY_SMALL_NUMBER)
      v_norm = Point3(0, 1, 0);

    if (v_norm.y < 0)
      v_norm *= -1;
    Quat rot_quat = quat_rotation_arc(v_orient, v_norm);
    mpp_tm = makeTM(rot_quat);
    mpp_tm = mpp_tm * tm;
  }

  tm = mpp_tm;
}
} // namespace objgenerator
