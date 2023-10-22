#include <math/dag_wooray2d.h>
#include <vecmath/dag_vecMath.h>

WooRay2d::WooRay2d(const Point2 &from, const Point2 &dir, float t, const Point2 &leaf_size, const IBBox2 &leaf_limits)
{
  vec4f end = v_madd(v_ldu_half(&dir.x), v_splats(t), v_ldu_half(&from.x));
  vec4f startEnd = v_perm_xyab(v_ldu_half(&from.x), end);
  startEnd = v_div(startEnd, v_perm_xyxy(v_ldu_half(&leaf_size.x)));
  vec4i startEndCells = v_cvt_floori(startEnd);
  v_stui(&pt.x, startEndCells);
  G_STATIC_ASSERT(offsetof(WooRay2d, endCell) == offsetof(WooRay2d, pt) + 8);

  if (v_extract_xi64(startEndCells) == v_extract_yi64(startEndCells))
  {
    step.x = step.y = 0;
    out = getEndCell();
    tMax.x = tMax.y = VERY_BIG_NUMBER;
    tDelta.x = tDelta.y = 0;
    return;
  }

  Point2 start;
  v_stu_half(&start.x, startEnd);
  double cb;

  for (int coord = 0; coord < 2; coord++)
  {
    if (dir[coord] >= 0.0f)
    {
      step[coord] = 1;
      double csp = ceil(start[coord]);
      cb = leaf_size[coord] * (csp + fsel(start[coord] - csp, 1, 0));
    }
    else
    {
      step[coord] = -1;
      cb = leaf_size[coord] * floor(start[coord]);
    }
    double wdircm = fabs(dir[coord]) - 1e-9;
    double rzr = fsel(wdircm, safeinv(double(dir[coord])), 0.0);
    tMax[coord] = fsel(wdircm, (cb - from[coord]) * rzr, (double)VERY_BIG_NUMBER);
    tDelta[coord] = leaf_size[coord] * fabs(rzr);
  }

  vec4i vStep = v_ldui_half(&step.x);
  vec4i vDir = v_cmp_lti(vStep, v_zeroi());
  vec4i vEnd = v_ldui_half(&endCell.x);
  vec4i llMin = v_ldui(&leaf_limits[0].x);
  vec4i llMax = v_roti_2(llMin);
  vec4i vOut = v_addi(v_seli(llMax, llMin, vDir), vStep);
  v_stui_half(&out.x, vOut);

  vec4i vCurrent = v_ldui_half(&pt.x);
  vec4i vClampedDir = v_cmp_gti(vCurrent, vOut);
  vec4f vSignDiff = v_cast_vec4f(v_xori(vClampedDir, vDir));
  vec4i vForceEnd = v_cmp_lti(v_cast_vec4i(v_or(v_splat_x(vSignDiff), v_splat_y(vSignDiff))), v_zeroi());
  vCurrent = v_seli(vCurrent, vEnd, vForceEnd);
  v_stui_half(&pt.x, vCurrent);
}
