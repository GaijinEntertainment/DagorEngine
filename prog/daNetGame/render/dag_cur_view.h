// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>

struct DagorCurView
{
  TMatrix tm, itm;
  Point3 pos; // current scene view position
};

extern DagorCurView grs_cur_view;