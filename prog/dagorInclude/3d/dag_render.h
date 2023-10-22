//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

#include <supp/dag_define_COREIMP.h>

struct DagorCurView
{
  TMatrix tm, itm;
  Point3 pos; // current scene view position
};

// global render states
extern bool grs_draw_wire;

extern DagorCurView grs_cur_view;


// compute and set gamma correction
void set_gamma(real p);
void set_gamma_shadervar(real p);

// returns current gamma
real get_current_gamma();

#include <supp/dag_undef_COREIMP.h>
