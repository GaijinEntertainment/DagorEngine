// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include <de3_rasterizePoly.h>
#include <de3_splineClassData.h>
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
// #include <sceneRay/dag_sceneRay.h>
#include <math/dag_bezierPrec.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include "editorLandRayTracer.h"

static const float BITMASK_SCALE = 0.25;

typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> HugeBitmask;
static HugeBitmask *bmLmeshHtConstr = NULL;

static bool replaceBitmask(HugeBitmask *&dest, HugeBitmask *bm, IBBox2 &out_bb)
{
  if (!bm && !dest)
    return false;

  int w = bm ? bm->getW() : 0, h = bm ? bm->getH() : 0;
  if (dest && dest->getW() == w && dest->getH() == h)
  {
    IBBox2 bb;
    if (!bm->compare(*dest, 0, 0, bb[0].x, bb[0].y, bb[1].x, bb[1].y))
      return false;

    out_bb = bb;
  }
  else if (!dest && bm)
  {
    IBBox2 bb;
    bm->addSetBitsMinMax(0, 0, bb[0].x, bb[0].y, bb[1].x, bb[1].y);
    out_bb = bb;
  }
  else
  {
    out_bb[0].x = 0;
    out_bb[0].y = 0;
    out_bb[1].x = w - 1;
    out_bb[1].y = h - 1;
  }

  del_it(dest);
  dest = bm;
  return true;
}

bool HmapLandPlugin::rebuildHtConstraintBitmask()
{
  if (exportType != EXPORT_LANDMESH)
  {
    del_it(bmLmeshHtConstr);
    return false;
  }

  int t0 = get_time_msec();
  BezierSpline2d s2d_src;
  float sx = BITMASK_SCALE, sy = BITMASK_SCALE;
  int w = sx * gridCellSize * heightMap.getMapSizeX();
  int h = sy * gridCellSize * heightMap.getMapSizeY();
  HugeBitmask *bm = new HugeBitmask(w, h);
  bool any_spline = false;
  int spl_count = 0;

  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (sobj->isPoly())
      continue;

    sobj->getSplineXZ(s2d_src);
    BezierSplinePrec2d &s2d = toPrecSpline(s2d_src);

    const splineclass::AssetData *a = sobj->points[0]->getSplineClass();
    int seg = 0;
    int hw = (a && a->lmeshHtConstrSweepWidth > 0) ? int(a->lmeshHtConstrSweepWidth / 2 / sx) : -1;

    float splineLen = s2d.getLength(), st;
    bool has_width = false;

    for (float s = 0.0; s < splineLen; s += 0.5 / sx)
    {
      float st;
      int n_seg = s2d.findSegment(s, st);
      if (seg != n_seg)
      {
        a = sobj->points[n_seg]->getSplineClass();
        hw = (a && a->lmeshHtConstrSweepWidth > 0) ? int(a->lmeshHtConstrSweepWidth / 2 / sx) : -1;
        seg = n_seg;
        if (hw >= 0)
          has_width = true;
      }
      if (hw < 0)
        continue;

      Point2 p = s2d.segs[n_seg].point(st);
      Point2 t = normalize(s2d.segs[n_seg].tang(st));
      Point2 n(t.y * sx * 0.5, -t.x * sy * 0.5);
      for (int i = -hw; i <= hw; i++)
      {
        int px = (p.x - heightMapOffset.x) * sx + n.x * i;
        int py = (p.y - heightMapOffset.y) * sy + n.y * i;
        if (px >= 0 && py >= 0 && px < w && py < h)
          bm->set(px, py);
      }
    }
    if (has_width)
    {
      spl_count++;
      any_spline = true;
    }
  }
  t0 = get_time_msec() - t0;
  DAEDITOR3.conRemark("rebuilt map %dx%d of %d/%d splines for %dms", bm->getW(), bm->getH(), spl_count, objEd.splinesCount(), t0);

  if (!any_spline)
    del_it(bm);

  t0 = get_time_msec();
  IBBox2 b;
  if (replaceBitmask(bmLmeshHtConstr, bm, b))
  {
    DAEDITOR3.conRemark("new lmesh ht constr map differs (%d,%d)-(%d,%d), compared for %dms", b[0].x, b[0].y, b[1].x, b[1].y,
      get_time_msec() - t0);

    if (bm)
    {
      b[0].x = b[0].x * landClsMap.getMapSizeX() / (sx * gridCellSize * heightMap.getMapSizeX());
      b[0].y = b[0].y * landClsMap.getMapSizeY() / (sy * gridCellSize * heightMap.getMapSizeY());
      b[1].x = b[1].x * landClsMap.getMapSizeX() / (sx * gridCellSize * heightMap.getMapSizeX());
      b[1].y = b[1].y * landClsMap.getMapSizeY() / (sy * gridCellSize * heightMap.getMapSizeY());
    }
    else
    {
      b[0].x = 0;
      b[0].y = 0;
      b[1].x = landClsMap.getMapSizeX();
      b[1].y = landClsMap.getMapSizeY();
    }
    return true;
  }

  DAEDITOR3.conRemark("new lmesh ht constr map is the same, compared for %dms", get_time_msec() - t0);
  return false;
}

bool HmapLandPlugin::applyHtConstraintBitmask(Mesh &mesh)
{
  if (!bmLmeshHtConstr)
    return false;

  int t0 = get_time_msec();

  float sx = BITMASK_SCALE, sy = BITMASK_SCALE;
  int w = sx * gridCellSize * heightMap.getMapSizeX();
  int h = sy * gridCellSize * heightMap.getMapSizeY();
  float aboveHt = meshErrorThreshold * 4;
  Tab<float> vshift(tmpmem);
  vshift.resize(mesh.vert.size());
  mem_set_0(vshift);
  bool changed = false;
  exportType = -1;
  BBox2 landBoundsXZ(Point2::xz(landMeshMap.getEditorLandRayTracer()->getBBox()[0]),
    Point2::xz(landMeshMap.getEditorLandRayTracer()->getBBox()[1]));
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (bmLmeshHtConstr->get(x, y))
      {
        Point3 p(heightMapOffset.x + x / sx, 0, heightMapOffset.y + y / sy);
        float t = aboveHt;
        getHeightmapPointHt(p, NULL);
        p.y += t;

        if (!(landBoundsXZ & Point2::xz(p)))
          continue;

        // int fi = rt.traceray(p, Point3(0,-1,0), t);

        float htOther;
        int fi = landMeshMap.getEditorLandRayTracer()->traceDownFaceVec<true, false>(p, htOther, NULL);

        if (fi < 0)
          continue;
        if (htOther > p.y || htOther < p.y - aboveHt)
          continue;
        fi /= 3;

        // compute barycentric coordinates
        const uint32_t *vi = mesh.face[fi].v;
        Point3 &v1 = mesh.vert[vi[0]], &v2 = mesh.vert[vi[1]], &v3 = mesh.vert[vi[2]];

        float dx = p.x - v3.x, dz = p.z - v3.z;
        float m11 = v1.x - v3.x, m12 = v2.x - v3.x, m21 = v1.z - v3.z, m22 = v2.z - v3.z;
        float invdet = 1.0 / (m11 * m22 - m12 * m21);
        float l1 = invdet * (m22 * dx - m12 * dz);
        float l2 = invdet * (-m21 * dx + m11 * dz);
        float l3 = 1 - l1 - l2;

        float lsq = l1 * l1 + l2 * l2 + l3 * l3;

        float dy1 = (t - aboveHt) * l1 / lsq;
        float dy2 = (t - aboveHt) * l2 / lsq;
        float dy3 = (t - aboveHt) * l3 / lsq;
        if (vshift[vi[0]] > dy1)
          vshift[vi[0]] = dy1;
        if (vshift[vi[1]] > dy2)
          vshift[vi[1]] = dy2;
        if (vshift[vi[2]] > dy3)
          vshift[vi[2]] = dy3;
        changed = true;
        // debug("diff %.2f at (%d,%d), " FMT_P3 " " FMT_P3 "",
        //   t-aboveHt, x, y, l1, l2, l3, vshift[vi[0]], vshift[vi[1]], vshift[vi[2]]);
      }
  exportType = EXPORT_LANDMESH;
  DAEDITOR3.conRemark("height constraints applied to landmesh for %dms", get_time_msec() - t0);
  if (!changed)
    return false;
  for (int i = 0; i < mesh.vert.size(); i++)
    mesh.vert[i].y += vshift[i];

  return true;
}
