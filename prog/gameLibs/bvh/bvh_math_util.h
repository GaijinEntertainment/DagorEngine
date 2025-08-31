// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath_common.h>

// Takes in worldTM, returns 3x3 matrix inverted, and position in last row
inline TMatrix4 make_packed_precise_itm(mat44f_cref tm)
{
  TMatrix4 inv_world_tm = TMatrix4::IDENT;

  vec4f translation = tm.col3;
  mat44f tmNoTranslation = tm;
  tmNoTranslation.col3 = V_C_UNIT_0001;

  mat44f inv44;
  v_mat44_inverse(inv44, tmNoTranslation);
  inv44.col3 = translation;
  v_mat44_transpose(inv44, inv44);
  v_stu(&inv_world_tm.row[0], inv44.col0);
  v_stu(&inv_world_tm.row[1], inv44.col1);
  v_stu(&inv_world_tm.row[2], inv44.col2);
  v_stu(&inv_world_tm.row[3], inv44.col3);

  return inv_world_tm;
}