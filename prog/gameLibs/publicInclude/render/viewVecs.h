//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point4.h>

class TMatrix4;
class TMatrix;

struct ViewVecs
{
  Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
};

ViewVecs calc_view_vecs(const TMatrix &view_tm, const TMatrix4 &proj_tm);

void set_viewvecs_to_shader(Point4 &view_vec_lt, Point4 &view_vec_rt, Point4 &view_vec_lb, Point4 &view_vec_rb, const TMatrix &view_tm,
  const TMatrix4 &proj_tm);
void set_viewvecs_to_shader(const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb,
  const Point4 &view_vec_rb);
void set_viewvecs_to_shader(const TMatrix &view_tm, const TMatrix4 &proj_tm);
// TODO remove this, uses gettm
void set_viewvecs_to_shader();

// if optional, will not set non-existent variables
void set_inv_globtm_to_shader(const TMatrix4 &view_tm, const TMatrix4 &proj_tm, bool optional);
