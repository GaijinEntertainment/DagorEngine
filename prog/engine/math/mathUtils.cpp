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

  resultTm.setcol(0, Point3(xaxis.x, yaxis.x, zaxis.x));
  resultTm.setcol(1, Point3(xaxis.y, yaxis.y, zaxis.y));
  resultTm.setcol(2, Point3(xaxis.z, yaxis.z, zaxis.z));
  resultTm.setcol(3, Point3(-(xaxis * eye), -(yaxis * eye), -(zaxis * eye)));
  resultTm = inverse(resultTm);
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
  return v_segment_box_intersection_side(v_ldu(&line_start.x), v_ldu(&line_end.x), v_ldu_bbox3(box), at);
}
