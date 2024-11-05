// Copyright (C) Gaijin Games KFT.  All rights reserved.

namespace rendinst::gen
{
extern bool custom_get_height(Point3 &pos, Point3 *out_norm = NULL);
}

static Point3 getPtEffRelBezierIn(dag::ConstSpan<ISplineGenObj::SplinePt> pts, int arrId, int t)
{
  const ISplineGenObj::SplinePt &pt = pts[arrId];
  if (pt.cornerType != -2)
    t = pt.cornerType;
  if (t >= 0)
    return t == 0 ? pt.relIn : (pt.relIn - pt.relOut) * 0.5f;

  int pn = pts.size();
  if (arrId > 0)
    return normalize(pts[arrId - 1].pt - pt.pt) * 0.05;
  return normalize(pt.pt - pts[arrId + 1].pt) * 0.05;
}
static Point3 getPtEffRelBezierOut(dag::ConstSpan<ISplineGenObj::SplinePt> pts, int arrId, int t)
{
  const ISplineGenObj::SplinePt &pt = pts[arrId];
  if (pt.cornerType != -2)
    t = pt.cornerType;
  if (t >= 0)
    return t == 0 ? pt.relOut : (pt.relOut - pt.relIn) * 0.5f;

  int pn = pts.size();
  if (arrId + 1 < pn)
    return normalize(pts[arrId + 1].pt - pt.pt) * 0.05;
  return normalize(pt.pt - pts[arrId - 1].pt) * 0.05;
}

void SplineGenEntity::build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl,
  dag::ConstSpan<splineclass::SegData> seg, int ss, int es)
{
  clear_and_shrink(loftSpl.segs);
  if (!seg.size())
  {
    loftSpl.calculateLength();
    return;
  }

  Tab<Point3> pts(tmpmem);
  pts.reserve(seg.size() * 3);

  // debug("loftSegs=%d (%d..%d)", seg.size(), ss, es);
  for (int i = 0; i < seg.size(); ++i)
  {
    if (seg[i].segN + ss < ss)
      continue;
    else if (seg[i].segN + ss > es)
      break;
    int base = append_items(pts, 3);
    pts[base + 0] = pts[base + 1] = pts[base + 2] = Point3::xVz(baseSpl.segs[seg[i].segN + ss].point(seg[i].offset), seg[i].y);
    // debug(" %d: seg=%d ofs=%.4f y=%.4f  %~p3", i, seg[i].segN+ss, seg[i].offset, seg[i].y, pts[base+0]);
  }
  loftSpl.calculate(pts.data(), pts.size(), false);
}
void SplineGenEntity::build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm,
  int corner_type, bool poly, bool poly_smooth, bool is_closed)
{
  Tab<Point3> pt_gnd;
  pt_gnd.resize(points.size() * 3);

  // drop points to collision and recompute Y for helpers using CatmullRom
  {
    for (int pi = 0; pi < points.size(); pi++)
    {
      pt_gnd[pi * 3 + 0] = tm * points[pi].pt;
      rendinst::gen::custom_get_height(pt_gnd[pi * 3 + 0], NULL);

      pt_gnd[pi * 3 + 1] = tm % getPtEffRelBezierIn(points, pi, corner_type);
      pt_gnd[pi * 3 + 2] = tm % getPtEffRelBezierOut(points, pi, corner_type);
    }

    Point3 *catmul[4];
    BezierSplineInt<Point3> sp;
    Point3 v[4];
    int pn = points.size();

    for (int i = -1; i < pn - 2; i++)
    {
      for (int j = 0; j < 4; j++)
        catmul[j] = pt_gnd.data() + ((poly || is_closed) ? (i + j + pn) % pn : clamp(i + j, 0, pn - 1)) * 3;

      for (int j = 0; j < 4; j++)
        v[j] = *catmul[j];
      sp.calculateCatmullRom(v);
      sp.calculateBack(v);

      float pi0_y = v[0].y - v[1].y, pi1_y = v[2].y - v[3].y;
      catmul[1][1].y = pi0_y;
      catmul[1][2].y = -pi0_y;
      catmul[2][1].y = pi1_y;
      catmul[2][2].y = -pi1_y;
    }
  }

  // build spline from ground points
  SmallTab<Point3, TmpmemAlloc> pts;
  int pts_num = points.size() + (poly ? 1 : 0);
  clear_and_resize(pts, pts_num * 3);

  for (int pi = 0; pi < pts_num; ++pi)
  {
    int wpi = pi % points.size();
    pts[pi * 3 + 1] = pt_gnd[wpi * 3 + 0];
    if (poly && !poly_smooth)
      pts[pi * 3 + 0] = pts[pi * 3 + 2] = pts[pi * 3 + 1];
    else
    {
      pts[pi * 3 + 0] = pt_gnd[wpi * 3 + 0] + pt_gnd[wpi * 3 + 1];
      pts[pi * 3 + 2] = pt_gnd[wpi * 3 + 0] + pt_gnd[wpi * 3 + 2];
    }
  }

  gndSpl.calculate(pts.data(), pts.size(), false);
}

void SplineGenEntity::generateObjects(BezierSpline3d &effSpline, int start_seg, int end_seg, int splineSubTypeId, int editLayerIdx,
  int rndSeed, int perInstSeed, Tab<cable_handle_t> *cablesPool)
{
  // debug("%p.gen(%d, %d) cls=%s", this, start_seg, end_seg,
  //   EDITORCORE->queryEditorInterface<IAssetService>()->getSplineClassDataName(splineClass));
  BezierSplinePrec3d loftSpl;
  build_corner_spline(loftSpl, ::toPrecSpline(effSpline), loftSeg, start_seg, end_seg - 1);
  resetUsedPoolsEntities();
  objgenerator::generateBySpline(::toPrecSpline(effSpline), loftSpl.segs.size() ? &loftSpl : NULL, start_seg, end_seg, splineClass,
    entPools, cablesPool, false, splineSubTypeId, editLayerIdx, rndSeed, perInstSeed);
  deleteUnusedPoolsEntities();
}
