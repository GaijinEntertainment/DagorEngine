// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/boundsUtils.h"
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathUtils.h>

BBox3 get_containing_box(const TMatrix &transform)
{
  bbox3f ibb, wbb;
  v_bbox3_init_ident(ibb);
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  v_bbox3_init(wbb, tm, ibb);
  BBox3 aabb;
  v_stu_bbox3(aabb, wbb);
  return aabb;
}
