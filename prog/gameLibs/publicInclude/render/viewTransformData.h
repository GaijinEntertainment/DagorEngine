//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>

struct ViewTransformData
{
  mat44f globtm;
  int x, y, w, h;
  float minz, maxz;

  union
  {
    int layer; // for testing against texture array
    int cascade;
  };
};
