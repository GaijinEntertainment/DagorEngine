// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <scene/dag_visibility.h>
#include <scene/dag_occlusion.h>
#include "de3_visibility_finder.h"
#include "dag_cur_view.h"

void update_visibility_finder(VisibilityFinder &vf) // legacy
{
  mat44f viewMatrix4;
  d3d::gettm(TM_VIEW, viewMatrix4);
  mat44f projTm4;
  d3d::gettm(TM_PROJ, projTm4);
  Driver3dPerspective p(1.3f, 2.3f, 1.f, 10000.f, 0.f, 0.f);

  mat44f pv;
  v_mat44_mul(pv, projTm4, viewMatrix4); // Avoid floating point errors from v_mat44_orthonormalize33.
  Frustum f;
  f.construct(pv);
  d3d::getpersp(p);
  vf.set(v_ldu(&::grs_cur_view.pos.x), f, 0, 0, 1, p.hk, current_occlusion);
}
