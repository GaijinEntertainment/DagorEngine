// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath_common.h>

inline TMatrix4 make_full_itm(mat44f_cref tm)
{
  mat44f inv44;
  v_mat44_inverse(inv44, tm);
  v_mat44_transpose(inv44, inv44);
  TMatrix4 itm;
  v_mat_44cu_from_mat44(&itm._11, inv44);
  return itm;
}

// Takes in worldTM, returns 3x3 matrix inverted, and position in last row
inline TMatrix4 make_packed_precise_itm(mat44f_cref tm)
{
  vec4f translation = tm.col3;
  mat44f tmNoTranslation = tm;
  tmNoTranslation.col3 = V_C_UNIT_0001;

  mat44f inv44;
  v_mat44_inverse(inv44, tmNoTranslation);
  inv44.col3 = translation;
  v_mat44_transpose(inv44, inv44);
  TMatrix4 inv_world_tm;
  v_mat_44cu_from_mat44(&inv_world_tm._11, inv44);

  return inv_world_tm;
}