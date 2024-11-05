//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_color.h>

#define SET_REPROJECTION_VARS_LIST \
  VAR(prev_globtm_no_ofs_psf_0)    \
  VAR(prev_globtm_no_ofs_psf_1)    \
  VAR(prev_globtm_no_ofs_psf_2)    \
  VAR(prev_globtm_no_ofs_psf_3)    \
  VAR(prev_zn_zfar)                \
  VAR(globtm_no_ofs_psf_0)         \
  VAR(globtm_no_ofs_psf_1)         \
  VAR(globtm_no_ofs_psf_2)         \
  VAR(globtm_no_ofs_psf_3)         \
  VAR(projtm_psf_0)                \
  VAR(projtm_psf_1)                \
  VAR(projtm_psf_2)                \
  VAR(projtm_psf_3)                \
  VAR(view_vecLT)                  \
  VAR(view_vecRT)                  \
  VAR(view_vecLB)                  \
  VAR(view_vecRB)                  \
  VAR(prev_view_vecLT)             \
  VAR(prev_view_vecRT)             \
  VAR(prev_view_vecLB)             \
  VAR(prev_view_vecRB)             \
  VAR(prev_world_view_pos)         \
  VAR(move_world_view_pos)         \
  VAR(reprojected_world_view_pos)

class DPoint3;
class Point4;
class TMatrix4;
class TMatrix;

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const TMatrix4 &prev_glob_tm, const Point4 &view_vec_lt,
  const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb, const Point4 &prev_view_vec_lt,
  const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb, const DPoint3 &world_pos,
  const DPoint3 &prev_world_pos);

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const TMatrix4 &prev_proj_tm, const TMatrix4 &prev_glob_tm,
  const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb,
  const Point4 &prev_view_vec_lt, const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb,
  const DPoint3 &world_pos, const DPoint3 &prev_world_pos);

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const Point2 &prev_zn_zfar, const TMatrix4 &prev_glob_tm,
  const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb,
  const Point4 &prev_view_vec_lt, const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb,
  const DPoint3 &world_pos, const DPoint3 &prev_world_pos);

void set_reprojection(const TMatrix &view_tm, const TMatrix4 &proj_tm, DPoint3 &prev_world_pos, TMatrix4 &prev_glob_tm,
  Point4 &prev_view_vec_lt, Point4 &prev_view_vec_rt, Point4 &prev_view_vec_lb, Point4 &prev_view_vec_rb, const DPoint3 *world_pos);

void set_reprojection(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &prev_proj_tm, DPoint3 &prev_world_pos,
  TMatrix4 &prev_glob_tm, Point4 &prev_view_vec_lt, Point4 &prev_view_vec_rt, Point4 &prev_view_vec_lb, Point4 &prev_view_vec_rb,
  const DPoint3 *world_pos);

struct ScopeReprojection
{
#define VAR(a) Color4 a;
  SET_REPROJECTION_VARS_LIST
#undef VAR
  ScopeReprojection();
  ~ScopeReprojection();
};
