//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class Point4;
class TMatrix4;
class TMatrix;

extern void set_viewvecs_to_shader(Point4 &viewVecLT, Point4 &viewVecRT, Point4 &viewVecLB, Point4 &viewVecRB, const TMatrix &viewTm,
  const TMatrix4 &projTm);
extern void set_viewvecs_to_shader(const TMatrix &viewTm, const TMatrix4 &projTm);
// TODO remove this, uses gettm
extern void set_viewvecs_to_shader();

// if optional, will not set non-existent variables
extern void set_inv_globtm_to_shader(const TMatrix4 &viewTm, const TMatrix4 &projTm, bool optional);
