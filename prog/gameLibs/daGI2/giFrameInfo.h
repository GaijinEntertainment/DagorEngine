// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

struct DaGIFrameInfo
{
  Point4 viewVecLT = {0, 0, 0, 0}, viewVecRT = {0, 0, 0, 0}, viewVecLB = {0, 0, 0, 0}, viewVecRB = {0, 0, 0, 0};
  DPoint3 world_view_pos = {0, 0, 0};
  Point3 viewZ = {0, 0, 0};
  TMatrix viewTm = TMatrix::IDENT, viewItm = TMatrix::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT, globTm = TMatrix4::IDENT, globTmNoOfs = TMatrix4::IDENT;
  TMatrix4 projTmUnjittered = TMatrix4::IDENT, globTmUnjittered = TMatrix4::IDENT, globTmNoOfsUnjittered = TMatrix4::IDENT;
  float znear = 1, zfar = 10;
};

DaGIFrameInfo get_frame_info(const DPoint3 &world_pos, const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf);

inline DaGIFrameInfo get_frame_info(const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf)
{
  // inaccurate version
  return get_frame_info(dpoint3(viewItm.getcol(3)), viewItm, proj, zn, zf);
}
TMatrix4 get_reprojection_campos_to_unjittered_history(const DaGIFrameInfo &current, const DaGIFrameInfo &prev);