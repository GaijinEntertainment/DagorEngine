#pragma once
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
inline TMatrix light_dir_ortho_tm(const Point3 &dir) { return tmatrix(matrix_look_at_lh(dir, Point3(0, 0, 0), Point3(0, 1, 0))); }


inline TMatrix calc_skewed_light_tm(const Point3 &sun_dir, bool align)
{
  TMatrix light_itm;
  light_itm.setcol(0, Point3(1, 0, 0));
  light_itm.setcol(1, Point3(0, 0, 1));
  light_itm.setcol(2, -sun_dir);
  light_itm.setcol(3, 0, 0, 0);

  if (align && (abs(sun_dir.z) > 0.00001 || abs(sun_dir.x) > 0.00001))
  {
    float angle = atan2(sun_dir.z, sun_dir.x);
    return inverse(light_itm * rotzTM(-angle));
  }

  return inverse(light_itm);
}

inline TMatrix scale_z_dist(const TMatrix &view_proj, float xy_scale, float inv_zrange, float z_ofs)
{
  TMatrix scale = TMatrix::IDENT;
  scale[0][0] = xy_scale;
  scale[1][1] = -xy_scale;
  scale[2][2] = inv_zrange;
  scale[3][2] = z_ofs;
  return scale * view_proj;
}

inline TMatrix under_horizon_tm()
{
  // Dummy matrix, that transforms everything to (0,0,2) which is guaranteed to be in shadow
  TMatrix result = TMatrix::ZERO;
  result.setcol(3, Point3(0, 0, 2));
  return result;
}
