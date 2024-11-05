// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include "staticShadowsViewMatrices.h"

static TMatrix get_region_proj_tm(const TMatrix &wtm, const Point2 &lt, const Point2 &wd)
{
  TMatrix st = TMatrix::IDENT;
  st[0][0] = 1. / wd.x;
  st[1][1] = -1. / wd.y;
  st[3][0] = -2 * lt.x / wd.x - 1;
  st[3][1] = -2 * lt.y / wd.y - 1;
  return st * wtm;
}

static void calculate_ortho_znzfar(const Point3 &pos, const Point3 &sun_dir, float dist, double &zn, double &zf)
{
  zn = (-light_dir_ortho_tm(sun_dir) * pos).z - dist * 2 * sqrt(2);
  zf = zn + dist * 2 * sqrt(2) * 2;
}

static void calculate_skewed_znzfar(double min_ht, double max_ht, const Point3 &sun_dir, double &zn, double &zf)
{
  double z_range = (max_ht - min_ht) / sun_dir.y;
  zf = max_ht / sun_dir.y;
  zn = zf - z_range;
}