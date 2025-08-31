// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/dag_math.h>
#include <math/dag_mathUtils.h>
#include <vecmath/dag_vecMath.h>

void lookAt(const Point3 &eye, const Point3 &at, const Point3 &up, TMatrix &resultTm)
{
  Point3 zaxis = -normalize(eye - at);
  Point3 xaxis = up % zaxis;
  if (lengthSq(xaxis) < FLT_EPSILON)
  {
    Point3 upReplacement = lengthSq(Point2::xz(up)) < FLT_EPSILON ? Point3(1, 0, 0) : Point3(0, 1, 0);
    xaxis = upReplacement % zaxis;
  }
  xaxis = normalize(xaxis);
  Point3 yaxis = zaxis % xaxis;

  resultTm.setcol(0, xaxis);
  resultTm.setcol(1, yaxis);
  resultTm.setcol(2, zaxis);
  resultTm.setcol(3, eye);
}

#ifdef DAGOR_EXPF_NOINLINE
#undef expf
__declspec(noinline) float expf_noinline(float arg) { return expf(arg); }
#endif

bool test_bbox_bbox_intersection(const BBox3 &box0, const BBox3 &box1, const TMatrix &tm1)
{
  bbox3f vbox0 = v_ldu_bbox3(box0);
  bbox3f vbox1 = v_ldu_bbox3(box1);
  mat44f vtm1;
  v_mat44_make_from_43cu_unsafe(vtm1, tm1.array);
  return v_bbox3_test_trasformed_box_intersect(vbox0, vbox1, vtm1);
}

bool check_bbox_intersection(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1)
{
  bbox3f vbox0 = v_ldu_bbox3(box0);
  bbox3f vbox1 = v_ldu_bbox3(box1);
  mat44f vtm0, vtm1;
  v_mat44_make_from_43cu_unsafe(vtm0, tm0.array);
  v_mat44_make_from_43cu_unsafe(vtm1, tm1.array);
  return v_bbox3_test_trasformed_box_intersect(vbox0, vtm0, vbox1, vtm1);
}

bool check_bbox_intersection(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1, float size_factor)
{
  bbox3f vbox0 = v_ldu_bbox3(box0);
  bbox3f vbox1 = v_ldu_bbox3(box1);
  mat44f vtm0, vtm1;
  v_mat44_make_from_43cu_unsafe(vtm0, tm0.array);
  v_mat44_make_from_43cu_unsafe(vtm1, tm1.array);
  return v_bbox3_test_trasformed_box_intersect(vbox0, vtm0, vbox1, vtm1, v_splats(size_factor));
}

int does_line_intersect_box_side(const BBox3 &box, const Point3 &line_start, const Point3 &line_end, real &at)
{
  float atMax;
  return v_segment_box_intersection_side(v_ldu(&line_start.x), v_ldu(&line_end.x), v_ldu_bbox3(box), at, atMax);
}

bool clip_homogeneous(Point4 &p0, Point4 &p1)
{
  if (p0.w < 0.0f && p1.w < 0.0f)
    return false;

  float t0 = 0.0f;
  float t1 = 1.0f;
  Point4 delta = p1 - p0;

  auto clipTest = [&](float q, float p) -> bool {
    if (std::abs(p) < FLT_EPSILON && q < 0.0f)
      return false;
    float r = q / p;
    if (p < 0.0f)
    {
      if (r > t1)
        return false;
      if (r > t0)
        t0 = r;
    }
    else
    {
      if (r < t0)
        return false;
      if (r < t1)
        t1 = r;
    }
    return true;
  };

  if (!clipTest(p0.w - p0.x, -delta.w + delta.x))
    return false;
  if (!clipTest(p0.w + p0.x, -delta.w - delta.x))
    return false;
  if (!clipTest(p0.w - p0.y, -delta.w + delta.y))
    return false;
  if (!clipTest(p0.w + p0.y, -delta.w - delta.y))
    return false;
  if (!clipTest(p0.w - p0.z, -delta.w + delta.z))
    return false;
  if (!clipTest(p0.w + p0.z, -delta.w - delta.z))
    return false;

  if (t1 < 1.0f)
    p1 = p0 + t1 * delta;
  if (t0 > 0.0f)
    p0 = p0 + t0 * delta;

  return true;
}

bool test_point_convex_intersection(const Point2 &p, const Point2 *pts, int32_t count)
{
  if (count < 3)
    return false;
  bool allNonPositive = true;
  bool allNonNegative = true;
  for (int i = 0; i < count; ++i)
  {
    const Point2 &a = pts[i];
    const Point2 &b = pts[(i + 1) % count];
    float cr = cross(b - a, p - a);
    if (cr > 0)
      allNonPositive = false;
    if (cr < 0)
      allNonNegative = false;
  }
  return allNonPositive || allNonNegative;
}
