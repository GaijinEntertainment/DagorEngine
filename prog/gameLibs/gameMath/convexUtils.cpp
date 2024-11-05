// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/convexUtils.h>
#include <vecmath/dag_vecMath.h>

void gamemath::calc_convex_segments(dag::ConstSpan<plane3f> convex, Tab<Point3> &segments)
{
  for (int i = 0; i < convex.size(); ++i)
  {
    for (int j = i + 1; j < convex.size(); ++j)
    {
      // line formula for pair of planes
      vec3f line = v_cross3(convex[i], convex[j]);
      vec3f len = v_length3(line);
      if (v_test_vec_x_le(len, V_C_EPS_VAL))
        continue;

      // point on that line
      vec3f point =
        v_add(v_mul(v_cross3(line, convex[j]), v_splat_w(convex[i])), v_mul(v_cross3(convex[i], line), v_splat_w(convex[j])));
      point = v_div(point, v_dot3(line, line));

      vec3f minT = v_splats(-FLT_MAX);
      vec3f maxT = v_splats(+FLT_MAX);
      for (int k = 0; k < convex.size(); ++k)
      {
        if (k == i || k == j)
          continue;
        vec3f det = v_dot3(line, convex[k]);
        if (v_test_vec_x_le(v_abs(det), V_C_EPS_VAL))
          continue;
        // calc limits of the line
        vec3f t = v_div(v_sub(v_neg(v_splat_w(convex[k])), v_dot3(point, convex[k])), det);
        if (v_test_vec_x_le_0(det)) // TODO: vectorize minmax a bit better
          minT = v_max(t, minT);
        else
          maxT = v_min(t, maxT);
      }
      if (v_test_vec_x_le(maxT, minT))
        continue;
      // output segment positions
      Point3_vec4 p0, p1;
      v_st(&p0.x, v_add(v_mul(line, v_splat_x(minT)), point));
      v_st(&p1.x, v_add(v_mul(line, v_splat_x(maxT)), point));
      segments.push_back(Point3::xyz(p0));
      segments.push_back(Point3::xyz(p1));
    }
  }
}
