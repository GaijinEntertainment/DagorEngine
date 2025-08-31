// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineClassMgr.h"
#include <math/dag_mesh.h>
#include <math/dag_bezierPrec.h>
#include <math/dag_Matrix3.h>
#include <debug/dag_debug.h>
#include <gameMath/objgenPrng.h>

#include <de3_splineClassData.h>
#include <EditorCore/ec_interface.h>
#include <libTools/staticGeomUi/nodeFlags.h>
#include <generic/dag_tab.h>

using namespace objgenerator; // prng
using splineclass::Attr;
using splineclass::SegData;

static inline bool nearlyEquals(const Point3 &p1, const Point3 &p2, float eps)
{
  return (fabs(p1.x - p2.x) < eps) && (fabs(p1.y - p2.y) < eps) && (fabs(p1.z - p2.z) < eps);
}

static inline float get_ht(const Point3 &p, float above_ht, bool place_on_collision, bool follow_hills, bool follow_hollows)
{
  if (place_on_collision | follow_hills | follow_hollows)
  {
    real dist = EDITORCORE->getMaxTraceDistance(), y = p.y + above_ht;
    if (EDITORCORE->traceRay(Point3::xVz(p, y), Point3(0, -1, 0), dist, NULL, false))
    {
      float h = y - dist;
      if (place_on_collision || (follow_hills && h > p.y) || (follow_hollows && h < p.y))
        return h;
    }
  }
  return p.y;
}

static void buildSegment(const BezierSplinePrecInt3d &bezie_point, float min_step, float max_step, float curvature_strength,
  float max_h_err, float max_hill_h_err, Tab<SegData> &seg_data, int seg_n, float seg_len, bool is_end_segment,
  bool place_on_collision, bool gnd_based_shape, float above_ht, bool follow_hills, bool follow_hollows, float ht_test_step,
  Attr start_scale, Attr end_scale, const Point2 &opacity_mul, //
  float zero_opac_fore_end, float zero_opac_back_end, float start_margin, float end_margin)
{
  union
  {
    int opacity_seed;
    float rnd_val;
  };
  rnd_val = bezie_point.point(0).x + bezie_point.point(0).z;

  if (min_step < 0.001f)
    min_step = 0.001f;

  if (start_scale.roadBhvOverride == 0)
    end_scale.roadBhvOverride = 0;
  else if (end_scale.roadBhvOverride == 0)
    start_scale.roadBhvOverride = 0;

  if (start_scale.followOverride != -1)
  {
    follow_hills = (start_scale.followOverride & 1) ? true : false;
    follow_hollows = (start_scale.followOverride & 2) ? true : false;
  }

  if (seg_len < min_step)
  {
    if (zero_opac_fore_end > 0 && zero_opac_fore_end < min_step)
      min_step = zero_opac_fore_end;
    if (zero_opac_back_end > 0 && zero_opac_back_end < min_step)
      min_step = zero_opac_back_end;
  }

  if (seg_len < min_step)
  {
    append_items(seg_data, 1);
    seg_data.back().segN = seg_n;
    seg_data.back().offset = start_margin > 0 ? bezie_point.getTFromS(start_margin) : 0.f;
    seg_data.back().y = get_ht(bezie_point.point(0), above_ht, place_on_collision, follow_hills, follow_hollows);
    seg_data.back().attr = start_scale;

    if (!is_end_segment)
      return;

    if (end_scale.followOverride != -1)
    {
      if (!(end_scale.followOverride & 1))
        follow_hills = false;
      if (!(end_scale.followOverride & 2))
        follow_hollows = false;
    }

    append_items(seg_data, 1);
    seg_data.back().segN = seg_n;
    seg_data.back().offset = end_margin > 0 ? bezie_point.getTFromS(seg_len - end_margin) : 1.f;
    seg_data.back().y = get_ht(bezie_point.point(1), above_ht, place_on_collision, follow_hills, follow_hollows);
    seg_data.back().attr = end_scale;
    return;
  }
  //  if (follow_hills && gnd_based_shape)
  //    place_on_collision = true;

  // calculate spline points
  const Point3 endPoint = bezie_point.point(1.f);

  const float step = min(ht_test_step, min_step);


  bool curvatureStop = false;
  float min_cosn = 1.f - (0.01f / curvature_strength);

  append_items(seg_data, 1);
  seg_data.back().segN = seg_n;
  seg_data.back().offset = start_margin > 0 ? bezie_point.getTFromS(start_margin) : 0.f;
  seg_data.back().attr = start_scale;
  seg_data.back().attr.opacity =
    clamp(seg_data.back().attr.opacity * (opacity_mul[0] + opacity_mul[1] * srnd(opacity_seed)), 0.f, 1.f);

  Point3 lastPt = bezie_point.point(0.f);
  float lastS = 0, lastTgA = MAX_REAL;

  lastPt.y = get_ht(lastPt, above_ht, place_on_collision, follow_hills, follow_hollows);
  seg_data.back().y = lastPt.y;

  if (end_scale.followOverride != -1)
  {
    if (!(end_scale.followOverride & 1))
      follow_hills = false;
    if (!(end_scale.followOverride & 2))
      follow_hollows = false;
  }

#define SET_INTERP_ATTR(X) seg_data.back().attr.X = start_scale.X + s * (end_scale.X - start_scale.X) / seg_len
  float prev_s = 0;
  if (place_on_collision | follow_hills | follow_hollows)
    for (float s = start_margin > 0 ? start_margin : step; s < (end_margin > 0 ? seg_len - end_margin : seg_len); s += step) //-V1034
    {
      bool force_using_pt = false;
      if (zero_opac_fore_end > 0 && s >= zero_opac_fore_end && prev_s < zero_opac_fore_end)
        s = zero_opac_fore_end, force_using_pt = true;
      if (zero_opac_back_end > 0 && s >= seg_len - zero_opac_back_end && prev_s < seg_len - zero_opac_back_end)
        s = seg_len - zero_opac_back_end, force_using_pt = true;
      const float curT = bezie_point.getTFromS(s);
      Point3 current = bezie_point.point(curT);
      Point3 tang = bezie_point.tang(curT);
      tang.y = 0;
      float max_he = max_h_err;

      float targetLength = s - lastS;

      float h = get_ht(current, above_ht, place_on_collision, follow_hills, follow_hollows);
      if (follow_hills && h > current.y)
        max_he = max_hill_h_err;
      current.y = h;

      Point3 t2(current.x - lastPt.x, 0, current.z - lastPt.z);

      float cosn = normalize(tang) * normalize(t2);

      if (lastTgA >= MAX_REAL && targetLength > 1.0f)
        lastTgA = (current.y - lastPt.y) / targetLength;

      if (cosn < min_cosn)
        curvatureStop = 1;
      else if (lastTgA < MAX_REAL && fabs(current.y - (lastPt.y + lastTgA * targetLength)) > max_he)
        curvatureStop = 1;

      if (targetLength >= max_step || (curvatureStop && targetLength >= min_step) || force_using_pt)
      {
        append_items(seg_data, 1);
        seg_data.back().segN = seg_n;
        seg_data.back().offset = curT;
        seg_data.back().y = current.y;
        seg_data.back().attr = start_scale;
        SET_INTERP_ATTR(scale_h);
        SET_INTERP_ATTR(scale_w);
        SET_INTERP_ATTR(opacity);
        SET_INTERP_ATTR(tc3u);
        SET_INTERP_ATTR(tc3v);
        seg_data.back().attr.opacity =
          clamp(seg_data.back().attr.opacity * (opacity_mul[0] + opacity_mul[1] * srnd(opacity_seed)), 0.f, 1.f);
        lastTgA = (current.y - lastPt.y) / targetLength;
        lastPt = current;
        lastS = s;

        curvatureStop = false;
      }
      prev_s = s;
    }
  else
    for (float s = start_margin > 0 ? start_margin : step; s < (end_margin > 0 ? seg_len - end_margin : seg_len); s += step) //-V1034
    {
      bool force_using_pt = false;
      if (zero_opac_fore_end > 0 && s >= zero_opac_fore_end && prev_s < zero_opac_fore_end)
        s = zero_opac_fore_end, force_using_pt = true;
      if (zero_opac_back_end > 0 && s >= seg_len - zero_opac_back_end && prev_s < seg_len - zero_opac_back_end)
        s = seg_len - zero_opac_back_end, force_using_pt = true;
      const float curT = bezie_point.getTFromS(s);
      Point3 current = bezie_point.point(curT);
      const Point3 tang = normalize(bezie_point.tang(curT));

      float targetLength = s - lastS;

      Point3 t2 = normalize(current - lastPt);

      float cosn = tang * t2;
      if (cosn < min_cosn)
        curvatureStop = 1;

      if (targetLength >= max_step || (curvatureStop && targetLength >= min_step) || force_using_pt)
      {
        append_items(seg_data, 1);
        seg_data.back().segN = seg_n;
        seg_data.back().offset = curT;
        seg_data.back().y = current.y;
        seg_data.back().attr = start_scale;
        SET_INTERP_ATTR(scale_h);
        SET_INTERP_ATTR(scale_w);
        SET_INTERP_ATTR(opacity);
        SET_INTERP_ATTR(tc3u);
        SET_INTERP_ATTR(tc3v);
        seg_data.back().attr.opacity =
          clamp(seg_data.back().attr.opacity * (opacity_mul[0] + opacity_mul[1] * srnd(opacity_seed)), 0.f, 1.f);
        lastPt = current;
        lastS = s;

        curvatureStop = false;
      }
      prev_s = s;
    }
#undef SET_INTERP_ATTR

  if (is_end_segment || end_margin)
  {
    append_items(seg_data, 1);
    seg_data.back().segN = seg_n;
    seg_data.back().offset = end_margin > 0 ? bezie_point.getTFromS(seg_len - end_margin) : 1.f;
    seg_data.back().y = get_ht(bezie_point.point(1), above_ht, place_on_collision, follow_hills, follow_hollows);
    seg_data.back().attr = end_scale;
    seg_data.back().attr.opacity =
      clamp(seg_data.back().attr.opacity * (opacity_mul[0] + opacity_mul[1] * srnd(opacity_seed)), 0.f, 1.f);
  }
}

static void refineRoadSegments(dag::Span<SegData> seg_data, int st_idx, const BezierSplinePrec3d &path, float above_ht, bool road_bhv,
  float max_up_ang, float road_test_wd)
{
  if (seg_data.size() < 2)
    return;

  bool any_road_pt = false;
  if (road_bhv)
  {
    for (int i = 0; i < seg_data.size(); i++)
      if (seg_data[i].attr.roadBhvOverride != 0)
      {
        any_road_pt = true;
        break;
      }
  }
  else
  {
    for (int i = 0; i < seg_data.size(); i++)
      if (seg_data[i].attr.roadBhvOverride == 1)
      {
        any_road_pt = true;
        break;
      }
  }
  if (!any_road_pt)
    return;

  bool use_side_dir = road_test_wd > 0;

  Tab<Point2> pts(tmpmem);
  Tab<Point2> ptsSide(tmpmem);
  Tab<float> seg_len(tmpmem);
  seg_len.resize(seg_data.size() - 1);
  pts.resize(seg_data.size());

  pts[0].set_xz(path.segs[seg_data[0].segN + st_idx].point(seg_data[0].offset));
  for (int i = 1; i < seg_data.size(); i++)
  {
    pts[i].set_xz(path.segs[seg_data[i].segN + st_idx].point(seg_data[i].offset));
    seg_len[i - 1] = length(pts[i] - pts[i - 1]);
  }
  if (use_side_dir)
  {
    ptsSide.resize(seg_data.size());
    for (int i = 0; i < seg_data.size(); i++)
    {
      ptsSide[i].set_xz(path.segs[seg_data[i].segN + st_idx].tang(seg_data[i].offset));
      float l = ptsSide[i].length();
      if (l > 1e-6)
        ptsSide[i].set(-ptsSide[i].y * road_test_wd * 0.5f / l, ptsSide[i].x * road_test_wd * 0.5f / l);
      else
        ptsSide[i].zero();
    }
  }

  float tg_a = tan(max_up_ang);
  Point3 sideDir0(0, 0, 0), sideDir1(0, 0, 0);
  Point3 p0_l(0, 0, 0), p0_r(0, 0, 0), dir_l(0, 0, 0), dir_r(0, 0, 0);
  float dist_l = 1, dist_r = 1;

  for (int attempts = 0; attempts < 128; attempts++)
  {
    int changes = 0;
    for (int i = 1; i < seg_data.size(); i++)
    {
      if (seg_data[i - 1].attr.roadBhvOverride == 0)
        continue;
      if (!road_bhv && seg_data[i - 1].attr.roadBhvOverride != 1)
        continue;

      float xz_len = seg_len[i - 1];
      if (seg_data[i].y - seg_data[i - 1].y >= xz_len * tg_a + 0.01)
      {
        // debug("%d: dy=%.3f xz_len=%.3f", i, seg_data[i].y-seg_data[i-1].y, xz_len);
        seg_data[i - 1].y = seg_data[i].y - xz_len * tg_a * 0.995f;
        changes++;
      }
      else if (seg_data[i].y - seg_data[i - 1].y <= -xz_len * tg_a - 0.01)
      {
        // debug("%d: dy=%.3f xz_len=%.3f", i, seg_data[i].y-seg_data[i-1].y, xz_len);
        seg_data[i].y = seg_data[i - 1].y - xz_len * tg_a * 0.995f;
        changes++;
      }
    }

    for (int i = 1; i < seg_data.size(); i++)
    {
      if (seg_data[i - 1].attr.roadBhvOverride == 0)
        continue;
      if (!road_bhv && seg_data[i - 1].attr.roadBhvOverride != 1)
        continue;

      Point3 p0, dir;
      p0.set_xVy(pts[i - 1], seg_data[i - 1].y + 0.01);
      dir = Point3::xVy(pts[i], seg_data[i].y + 0.01) - p0;

      float dist = dir.length();
      // debug("%d: p0=" FMT_P3 " dir=" FMT_P3 " dist=%.3f", i-1, P3D(p0), P3D(dir), dist);
      if (dist < 1e-3)
        continue;
      if (use_side_dir)
      {
        sideDir0.set_x0y(ptsSide[i - 1] * seg_data[i - 1].attr.scale_w);
        sideDir1.set_x0y(ptsSide[i] * seg_data[i].attr.scale_w);
        p0_l = p0 - sideDir0;
        p0_r = p0 + sideDir0;
        dir_l = p0 + dir - sideDir1 - p0_l;
        dir_r = p0 + dir + sideDir1 - p0_r;
        dist_l = dir_l.length();
        dist_r = dir_r.length();
      }

      if (dist < 1e-6 || !EDITORCORE->traceRay(p0, dir / dist, dist, NULL, false))
      {
        if (!use_side_dir)
          continue;
        else if ((dist_l < 1e-6 || !EDITORCORE->traceRay(p0_l, dir_l / dist_l, dist, NULL, false)) &&
                 (dist_r < 1e-6 || !EDITORCORE->traceRay(p0_r, dir_r / dist_r, dist, NULL, false)))
          continue;
      }

      float f = 0, w = 0, max_dh = 0;
      for (int j = 1; j < 8; j++)
      {
        Point3 p = p0 + dir * (0.125f * j);
        float h = get_ht(p, above_ht, true, false, false);
        if (h > p.y)
        {
          w += h - p.y;
          f += (h - p.y) * (0.125f * j);
          inplace_max(max_dh, h - p.y);
        }
      }

      if (use_side_dir)
        for (int j = 1; j < 8; j++)
        {
          Point3 p = p0_l + dir_l * (0.125f * j);
          float h = get_ht(p, above_ht, true, false, false);
          if (h > p.y)
          {
            w += h - p.y;
            f += (h - p.y) * (0.125f * j);
            inplace_max(max_dh, h - p.y);
          }

          p = p0_r + dir_r * (0.125f * j);
          h = get_ht(p, above_ht, true, false, false);
          if (h > p.y)
          {
            w += h - p.y;
            f += (h - p.y) * (0.125f * j);
            inplace_max(max_dh, h - p.y);
          }
        }

      // debug("seg_data.%p.%d: dist=%.3f/%.3f w=%.3f f=%.3f max_dh=%.3f",
      //   seg_data.data(), i, dist, dir.length(), w, f, max_dh);
      if (w > 0)
      {
        max_dh *= 1.2;
        seg_data[i - 1].y += max_dh * (1.0 - f / w) + 0.1;
        seg_data[i].y += max_dh * f / w + 0.1;
      }
      else
      {
        seg_data[i - 1].y += 0.5;
        seg_data[i].y += 0.5;
      }
      changes++;
    }

    if (!changes)
    {
      // debug("done for %d iter", attempts);
      break;
    }
    // debug("iter %d: %d changes", attempts, changes);
  }
  // debug_flush(false);
}


void SplineClassAssetMgr::createLoftMesh(Mesh &mesh, const splineclass::LoftGeomGenData::Loft &loft, const BezierSplinePrec3d &path,
  int path_subdiv_count_, const BezierSplinePrec2d &shape, dag::ConstSpan<Point4> spts, int shape_subdiv_count, bool extrude,
  bool place_on_collision, float above_ht, dag::ConstSpan<splineclass::LoftGeomGenData::Loft::PtAttr> pt_attr, int start_idx,
  int end_idx, float min_step, float max_step, float curvature, float max_h_err, float max_hill_h_err, bool a_follow_hills,
  bool a_follow_hollows, float ht_test_step, bool road_bhv, float road_max_abs_ang, float road_max_inter_ang, float road_test_wd,
  float scale_tc_along, int select_mat, dag::ConstSpan<Attr> splineScales, float zero_opac_fore_end, float zero_opac_back_end,
  float path_margin_s, float path_margin_e, Tab<SegData> *out_loftSeg)
{
  // precompute TC scales
  float newuSize = loft.uSize * scale_tc_along;
  if (loft.integralMappingLength)
  {
    float spl_len = path.getLength();
    float div = spl_len / newuSize;
    int tilesCount = (div > 0.5) ? (int)(div + 0.5) : 1;
    newuSize = spl_len / tilesCount;
    if (loft.maxMappingDistortionThreshold > 0)
    {
      float threshold = loft.maxMappingDistortionThreshold;
      if (div > 0.5f && fabsf(1.0f - div / floorf(div + 0.5f)) < threshold)
        newuSize = spl_len / tilesCount;
      else
        newuSize = loft.uSize * scale_tc_along;
    }
  }


  SmallTab<char, TmpmemAlloc> shapeSmgr;
  SmallTab<float, TmpmemAlloc> shapeYofs;
  if (shape_subdiv_count < 1)
    shape_subdiv_count = 1;

  Tab<SegData> segData(tmpmem);

  if (path_subdiv_count_ < 1)
    path_subdiv_count_ = 1;
  const int segCnt = end_idx - start_idx;

  const int lastSgN = segCnt - 1;

  bool gnd_based_shape = false;
  bool follow_hills = a_follow_hills, follow_hollows = a_follow_hollows;

  for (int i = 0; i < pt_attr.size(); i++)
    switch (pt_attr[i].type)
    {
      case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_REL_TO_GND:
      case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_GROUP_REL_TO_GND:
      case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_MOVE_TO_MIN:
      case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_MOVE_TO_MAX: gnd_based_shape = true;
    }

  union
  {
    int opacity_seed;
    float rnd_val;
  };
  rnd_val = segCnt ? path.segs[start_idx].point(0).x - path.segs[start_idx].point(0).z : 0;

  float seg_s0 = 0, full_len = 0, slen = 0;
  for (int i = 0; i < segCnt; i++)
    full_len += path.segs[i + start_idx].len;
  float zo0_pos = zero_opac_fore_end > 0 ? path_margin_s + zero_opac_fore_end : 0.f;
  float zo1_pos = zero_opac_back_end > 0 ? full_len - path_margin_e - zero_opac_back_end : 0.f;

  float m1_pos = path_margin_e > 0 ? full_len - path_margin_e : 0.f;

  for (int i = 0; i < segCnt; i++, seg_s0 += slen)
  {
    slen = path.segs[i + start_idx].len;
    if (path_margin_s > 0 && seg_s0 + slen < path_margin_s)
      continue;
    if (path_margin_e > 0 && seg_s0 + path_margin_e > full_len)
      break;

    float eff_zo0_end = zo0_pos > 0 && zo0_pos > seg_s0 && zo0_pos < seg_s0 + slen ? zo0_pos - seg_s0 : 0.f;
    float eff_zo1_end = zo1_pos > 0 && zo1_pos > seg_s0 && zo1_pos < seg_s0 + slen ? seg_s0 + slen - zo1_pos : 0.f;
    float eff_m0 = path_margin_s > 0 && path_margin_s > seg_s0 && path_margin_s < seg_s0 + slen ? path_margin_s - seg_s0 : 0.f;
    float eff_m1 = path_margin_e > 0 && m1_pos > seg_s0 && m1_pos < seg_s0 + slen ? seg_s0 + slen - m1_pos : 0.f;

    buildSegment(path.segs[i + start_idx], min_step, max_step, curvature, max_h_err, max_hill_h_err, segData, i, slen, i == lastSgN,
      place_on_collision, gnd_based_shape, above_ht, follow_hills, follow_hollows, ht_test_step, splineScales[i], splineScales[i + 1],
      loft.randomOpacityMulAlong, eff_zo0_end, eff_zo1_end, eff_m0, eff_m1);
  }

  refineRoadSegments(make_span(segData), start_idx, path, above_ht, road_bhv, DEG_TO_RAD * road_max_abs_ang, road_test_wd);

  if (out_loftSeg)
    *out_loftSeg = segData;

  const int totalPathPoints = segData.size();
  if (totalPathPoints < 2)
    return;
  Tab<Face> &face = mesh.face;
  Tab<TFace> &tface = mesh.tface[0];

  int totalShapePoints = shape_subdiv_count * shape.segs.size() + 1;
  int faceCount = (totalShapePoints - 1) * (totalPathPoints - 1) * 2;
  int startFace = append_items(face, faceCount);
  int startTFace = append_items(tface, faceCount);

  if (shape.isClosed())
    totalShapePoints--;
  // DEBUG_CTX("faces: %d path =%d * shape=%d  verts = %d",
  //   faceCount, totalPathPoints, totalShapePoints, totalPathPoints * totalShapePoints);
  int startVert = append_items(mesh.vert, totalPathPoints * totalShapePoints);
  int startTVert = append_items(mesh.tvert[0], totalPathPoints * (totalShapePoints + 1));
  G_VERIFY(startTVert == append_items(mesh.tvert[1], totalPathPoints * (totalShapePoints + 1)));
  G_VERIFY(startTVert == append_items(mesh.tvert[2], totalPathPoints * (totalShapePoints + 1)));
  G_VERIFY(startTVert == append_items(mesh.tvert[3], totalPathPoints * (totalShapePoints + 1)));
  G_VERIFY(startTVert == append_items(mesh.tvert[4], totalPathPoints * (totalShapePoints + 1)));

  clear_and_resize(shapeYofs, pt_attr.size() * shape_subdiv_count);
  clear_and_resize(shapeSmgr, shape.segs.size());
  mem_set_0(shapeSmgr);
  for (int i = 0; i < shape.segs.size(); i++)
  {
    int prev = i - 1;
    if (prev < 0)
    {
      if (!shape.isClosed())
        continue;
      else
        prev = shape.segs.size() - 1;
    }

    if (fabsf(normalize(shape.segs[i].tang(0)) * normalize(shape.segs[prev].tang(1)) - 1.0f) > 0.001)
      shapeSmgr[i] = 1;
  }

  Point3 *verts = &mesh.vert[startVert];
  Point2 *tverts0 = &mesh.tvert[0][startTVert];
  Point2 *tverts1 = &mesh.tvert[1][startTVert];
  Point2 *tverts2 = &mesh.tvert[2][startTVert];
  Point2 *tverts3 = &mesh.tvert[3][startTVert];
  Point2 *tverts4 = &mesh.tvert[4][startTVert];
  int pathSmoothGroup = 1;
  bool firstPathSmoothSeem = false;
  int vi, tvi, pathi;
  int lastPathSegI = 0, pathSubdivI = -1;
  float fullPathLen = path.segs.back().tlen;

  for (vi = 0, tvi = 0, pathi = 0; pathi < totalPathPoints; ++pathi)
  {
    int pathSegI = segData[pathi].segN;
    const int nowSegI = pathSegI + start_idx;
    if ((pathSegI == lastPathSegI) && (pathi != totalPathPoints - 1))
      pathSubdivI++;
    else
      pathSubdivI = 0;

    lastPathSegI = pathSegI;

    float st = segData[pathi].offset;
    float atPath = (nowSegI ? path.segs[nowSegI - 1].tlen : 0) + path.segs[nowSegI].getLength(st);

    if (!pathSubdivI)
    {
      int prevSegI = pathSegI - 1;
      if (prevSegI < 0)
        prevSegI = 0;
      if (fabsf(normalize(path.segs[nowSegI].tang(0)) * normalize(path.segs[prevSegI + start_idx].tang(1)) - 1.0f) > 0.001)
      {
        if (!pathi)
          firstPathSmoothSeem = true;
        else
          pathSmoothGroup = 1 - pathSmoothGroup;
      }
    }
    unsigned pathSmgr = pathSmoothGroup + 1;

    Point3 pos = path.segs[nowSegI].point(st);
    pos.y = segData[pathi].y;

    Matrix3 ctm;
    Point3 tang = normalize(path.segs[nowSegI].tang(st));
    if (tang.lengthSq() < 0.25)
      tang = normalize(path.segs[nowSegI].point(1) - path.segs[nowSegI].point(0));
    Point3 ya(0, 1, 0);
    if (extrude)
    {
      if (fabsf(tang.y) >= .99f)
        ya = Point3(1, 0, 0);
      ctm.setcol(0, normalize(ya % tang));
      ctm.setcol(1, ya);
      ctm.setcol(2, normalize(ctm.getcol(0) % ya));
    }
    else
    {
      if (fabsf(tang.y) >= .99f)
        ya = Point3(1, 0, 0);
      ctm.setcol(0, normalize(ya % tang));
      ctm.setcol(1, normalize(tang % ctm.getcol(0)));
      ctm.setcol(2, tang);
    }
    // Point3 extrudeOfs;
    int shapeSmoothGroup = 1;
    int start_vi = vi;
    const Attr &cur_attr = segData[pathi].attr;
    float opacity_scale = 1.0f;
    if (zero_opac_fore_end > 0 && atPath < path_margin_s + zero_opac_fore_end)
      opacity_scale = (atPath - path_margin_s) / zero_opac_fore_end;
    if (zero_opac_back_end > 0 && atPath > full_len - path_margin_e - zero_opac_back_end)
      opacity_scale *= (full_len - path_margin_e - atPath) / zero_opac_back_end;

    for (int shapei = 0; shapei < totalShapePoints; ++shapei, vi++, tvi++)
    {
      int shapeSegI = shapei / shape_subdiv_count;
      int shapeSubdivI = shapei % shape_subdiv_count;
      float shapeSegLen = shapeSegI ? shape.segs[shapeSegI - 1].tlen : 0;
      float segLen = (shapeSegI < shape.segs.size()) ? shape.segs[shapeSegI].len : 1.0;
      float atSeg = segLen * shapeSubdivI / shape_subdiv_count;
      if (!shapeSubdivI && shapeSmgr[shapeSegI < shape.segs.size() ? shapeSegI : 0])
        shapeSmoothGroup = 1 - shapeSmoothGroup;

      Point2 ofs = shape.get_pt(shapeSegLen + atSeg);
      int nextSegI = (shapeSegI + 1) % spts.size();
      float v = ((shape.isClosed() && nextSegI == 0) ? 1.0 : spts[nextSegI].w) * atSeg / segLen;
      v += spts[shapeSegI].w * (1.0 - atSeg / segLen);

      if (shapei < shapeYofs.size())
        shapeYofs[shapei] = ofs.y * cur_attr.scale_h;

      verts[vi] = pos + ctm * Point3(ofs.x * cur_attr.scale_w, ofs.y * cur_attr.scale_h, 0);
      tverts0[tvi] = Point2(atPath, v);
      tverts1[tvi] = Point2(
        opacity_scale *
          clamp(cur_attr.opacity * (loft.randomOpacityMulAcross[0] + loft.randomOpacityMulAcross[1] * srnd(opacity_seed)), 0.f, 1.f),
        atPath / fullPathLen);
      tverts2[tvi] =
        Point2(shapei == totalShapePoints - 1 ? -1 : shapei, pt_attr.size() ? pt_attr[shapei / shape_subdiv_count].attr : 0);
      tverts3[tvi] = Point2(cur_attr.tc3u, cur_attr.tc3v);
      tverts4[tvi] = Point2(tang.x, tang.z);

      if ((shapei != totalShapePoints - 1 || shape.isClosed()) && (pathi != totalPathPoints - 1))
      {
        int faceI = startFace + ((totalShapePoints - (shape.isClosed() ? 0 : 1)) * pathi + shapei) * 2;
        int tfaceI = faceI - startTFace + startFace;
        int tvertPerPathPt = totalShapePoints + (shape.isClosed() ? 1 : 0), nextTShapeI = (shapei + 1) % tvertPerPathPt;

        int nextShapeI = (shapei == totalShapePoints - 1) ? 0 : shapei + 1, nextPathI = (pathi == totalPathPoints - 1) ? 0 : pathi + 1;
        face[faceI].v[0] = startVert + pathi * totalShapePoints + shapei;
        face[faceI].v[1] = startVert + nextPathI * totalShapePoints + shapei;
        face[faceI].v[2] = startVert + (pathi)*totalShapePoints + nextShapeI;
        face[faceI + 1].v[0] = startVert + (nextPathI)*totalShapePoints + shapei;
        face[faceI + 1].v[1] = startVert + (nextPathI)*totalShapePoints + nextShapeI;
        face[faceI + 1].v[2] = startVert + (pathi)*totalShapePoints + nextShapeI;

        tface[tfaceI].t[0] = startTVert + pathi * tvertPerPathPt + shapei;
        tface[tfaceI].t[1] = startTVert + nextPathI * tvertPerPathPt + shapei;
        tface[tfaceI].t[2] = startTVert + (pathi)*tvertPerPathPt + nextTShapeI;
        tface[tfaceI + 1].t[0] = startTVert + (nextPathI)*tvertPerPathPt + shapei;
        tface[tfaceI + 1].t[1] = startTVert + (nextPathI)*tvertPerPathPt + nextTShapeI;
        tface[tfaceI + 1].t[2] = startTVert + (pathi)*tvertPerPathPt + nextTShapeI;

        unsigned smgr = (pathSmgr) << ((shapeSmoothGroup + 1) * 2);
        face[faceI + 1].smgr = face[faceI].smgr = smgr;
        int mat = 0;
        if (pt_attr.size())
          mat = pt_attr[shapei / shape_subdiv_count].matId;
        face[faceI + 1].mat = face[faceI].mat = mat;
      }
    }


    float grp_rel_ofs = 0;
    if (pt_attr.size())
      for (int shapei = 0; shapei < totalShapePoints; ++shapei)
      {
        int pti = shapei / shape_subdiv_count;
        if (pt_attr[pti].type == splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_NORMAL)
          continue;

        Point3 p = verts[start_vi + shapei];
        real dist = EDITORCORE->getMaxTraceDistance(), y = p.y - shapeYofs[shapei] + above_ht;
        real offset = 0;

        if (pt_attr[pti].type == pt_attr[pti].TYPE_GROUP_REL_TO_GND)
        {
          if (shapei == 0 || pt_attr[pti].grpId != pt_attr[(shapei - 1) / shape_subdiv_count].grpId)
            if (EDITORCORE->traceRay(Point3(p.x, y, p.z), Point3(0, -1, 0), dist, NULL, false))
              grp_rel_ofs = above_ht - dist;
        }
        else if (EDITORCORE->traceRay(Point3(p.x, y, p.z), Point3(0, -1, 0), dist, NULL, false))
          offset = above_ht - dist;

        switch (pt_attr[pti].type)
        {
          case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_REL_TO_GND: verts[start_vi + shapei].y += offset; break;

          case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_GROUP_REL_TO_GND: verts[start_vi + shapei].y += grp_rel_ofs; break;

          case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_MOVE_TO_MIN:
            for (int j = 0; j < totalShapePoints; j++)
              if (shapei != j && pt_attr[pti].grpId == pt_attr[j / shape_subdiv_count].grpId)
              {
                p = verts[start_vi + j];
                dist = EDITORCORE->getMaxTraceDistance();
                y = p.y - shapeYofs[j] + above_ht;
                if (EDITORCORE->traceRay(Point3(p.x, y, p.z), Point3(0, -1, 0), dist, NULL, false))
                  inplace_min(offset, above_ht - dist);
              }
            if (offset > 0)
              for (int j = 0; j < totalShapePoints; j++)
                if (pt_attr[pti].grpId == pt_attr[j / shape_subdiv_count].grpId)
                  verts[start_vi + j].y += offset;
            break;

          case splineclass::LoftGeomGenData::Loft::PtAttr::TYPE_MOVE_TO_MAX:
            for (int j = 0; j < totalShapePoints; j++)
              if (shapei != j && pt_attr[pti].grpId == pt_attr[j / shape_subdiv_count].grpId)
              {
                p = verts[start_vi + j];
                dist = EDITORCORE->getMaxTraceDistance();
                y = p.y - shapeYofs[j] + above_ht;
                if (EDITORCORE->traceRay(Point3(p.x, y, p.z), Point3(0, -1, 0), dist, NULL, false))
                  inplace_max(offset, above_ht - dist);
              }
            if (offset > 0)
              for (int j = 0; j < totalShapePoints; j++)
                if (pt_attr[pti].grpId == pt_attr[j / shape_subdiv_count].grpId)
                  verts[start_vi + j].y += offset;
            break;
        }
      }

    if (shape.isClosed())
    {
      tverts0[tvi] = Point2(atPath, 1.0);
      tverts1[tvi] = Point2(
        opacity_scale *
          clamp(cur_attr.opacity * (loft.randomOpacityMulAcross[0] + loft.randomOpacityMulAcross[1] * srnd(opacity_seed)), 0.f, 1.f),
        atPath / fullPathLen);
      tverts2[tvi] = Point2(0, pt_attr.size() ? pt_attr[0].attr : 0);
      tverts3[tvi] = Point2(cur_attr.tc3u, cur_attr.tc3v);
      tvi++;
    }
  }

  G_ASSERT(vi <= mesh.vert.size() - startVert);
  G_ASSERT(tvi <= mesh.tvert[0].size() - startTVert);
  mesh.vert.resize(vi + startVert);
  mesh.tvert[0].resize(tvi + startTVert);
  mesh.tvert[1].resize(tvi + startTVert);
  mesh.tvert[2].resize(tvi + startTVert);
  mesh.tvert[3].resize(tvi + startTVert);
  mesh.tvert[4].resize(tvi + startTVert);
  mesh.tface[1] = tface;
  mesh.tface[2] = tface;
  mesh.tface[3] = tface;
  mesh.tface[4] = tface;

  // scale TC and post-process mesh
  if (select_mat >= 0)
    for (int faceNo = mesh.face.size() - 1; faceNo >= 0; faceNo--)
    {
      if (mesh.face[faceNo].mat != select_mat)
      {
        erase_items(mesh.face, faceNo, 1);
        erase_items(mesh.tface[0], faceNo, 1);
        erase_items(mesh.tface[1], faceNo, 1);
        erase_items(mesh.tface[2], faceNo, 1);
        erase_items(mesh.tface[3], faceNo, 1);
        erase_items(mesh.tface[4], faceNo, 1);
      }
    }

  for (int i = 0; i < mesh.tvert[0].size(); ++i)
  {
    mesh.tvert[0][i].x /= newuSize;
    mesh.tvert[0][i].y = 1 - mesh.tvert[0][i].y * loft.vTile;
  }

  if (loft.flipUV)
    for (int i = 0; i < mesh.tvert[0].size(); ++i)
    {
      real k = mesh.tvert[0][i].x;
      mesh.tvert[0][i].x = mesh.tvert[0][i].y;
      mesh.tvert[0][i].y = k;
    }

  mesh.calc_ngr();
  mesh.calc_vertnorms();

  if (loft.flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS)
  {
    for (unsigned int faceNormalNo = 0; faceNormalNo < mesh.facenorm.size(); faceNormalNo++)
      mesh.facenorm[faceNormalNo] = loft.normalsDir;

    for (unsigned int vertNormalNo = 0; vertNormalNo < mesh.vertnorm.size(); vertNormalNo++)
      mesh.vertnorm[vertNormalNo] = loft.normalsDir;
  }

  if (loft.cullcw)
    mesh.flip_normals();

  // invert normals to provide proper geometry in shader (fixes long standing bug in generation)
  for (unsigned int faceNormalNo = 0; faceNormalNo < mesh.facenorm.size(); faceNormalNo++)
    mesh.facenorm[faceNormalNo] = -mesh.facenorm[faceNormalNo];
  for (unsigned int vertNormalNo = 0; vertNormalNo < mesh.vertnorm.size(); vertNormalNo++)
    mesh.vertnorm[vertNormalNo] = -mesh.vertnorm[vertNormalNo];
}

void SplineClassAssetMgr::build2DSpline(BezierSpline2d &spline, dag::ConstSpan<Point4> spts, bool closed)
{
  SmallTab<Point2, TmpmemAlloc> pts;
  bool smooth = false;
  for (int pi = 0; pi < spts.size(); ++pi)
    if (spts[pi].z)
    {
      smooth = true;
      break;
    }

  if (smooth)
  {
    clear_and_resize(pts, spts.size());
    for (int pi = 0; pi < spts.size(); ++pi)
      pts[pi] = Point2(spts[pi].x, spts[pi].y);
    spline.calculateCatmullRom(pts.data(), pts.size(), closed);

    Point2 v[4];
    for (int pi = 0; pi < spts.size(); ++pi)
      if (!spts[pi].z)
      {
        if (closed)
        {
          spline.segs[pi].calculateBack(v);
          v[1] = v[0];
          spline.segs[pi].calculate(v);
          int prev = (pi + spts.size() - 1) % spts.size();
          spline.segs[prev].calculateBack(v);
          v[2] = v[3];
          spline.segs[prev].calculate(v);
        }
        else
        {
          if (pi + 1 < spts.size())
          {
            spline.segs[pi].calculateBack(v);
            v[1] = v[0];
            spline.segs[pi].calculate(v);
          }
          if (pi > 0)
          {
            spline.segs[pi - 1].calculateBack(v);
            v[2] = v[3];
            spline.segs[pi - 1].calculate(v);
          }
        }
      }
    return;
  }

  clear_and_resize(pts, spts.size() * 3);

  for (int pi = 0; pi < spts.size(); ++pi)
  {
    Point4 p = spts[pi];
    pts[pi * 3] = pts[pi * 3 + 1] = pts[pi * 3 + 2] = Point2(p.x, p.y);
  }

  spline.calculate(pts.data(), pts.size(), closed);
}
