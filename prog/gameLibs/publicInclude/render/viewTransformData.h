//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
