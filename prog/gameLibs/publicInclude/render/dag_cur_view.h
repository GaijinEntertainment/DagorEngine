//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>

#include <supp/dag_define_COREIMP.h>

struct DagorCurView
{
  TMatrix tm, itm;
  Point3 pos; // current scene view position
};

extern DagorCurView grs_cur_view;

#include <supp/dag_undef_COREIMP.h>
