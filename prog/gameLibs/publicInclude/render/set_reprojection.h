//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_color.h>

#define SET_REPROJECTION_VARS_LIST \
  VAR(prev_globtm_no_ofs_psf_0)    \
  VAR(prev_globtm_no_ofs_psf_1)    \
  VAR(prev_globtm_no_ofs_psf_2)    \
  VAR(prev_globtm_no_ofs_psf_3)    \
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
extern void set_reprojection(DPoint3 &prevWorldPos, TMatrix4 &prevGlobTm, Point4 &prevViewVecLT, Point4 &prevViewVecRT,
  Point4 &prevViewVecLB, Point4 &prevViewVecRB, const DPoint3 *world_pos);
extern void set_reprojection(const TMatrix &viewTm, const TMatrix4 &projTm, DPoint3 &prevWorldPos, TMatrix4 &prevGlobTm,
  Point4 &prevViewVecLT, Point4 &prevViewVecRT, Point4 &prevViewVecLB, Point4 &prevViewVecRB, const DPoint3 *world_pos);

struct ScopeReprojection
{
#define VAR(a) Color4 a;
  SET_REPROJECTION_VARS_LIST
#undef VAR
  ScopeReprojection();
  ~ScopeReprojection();
};
