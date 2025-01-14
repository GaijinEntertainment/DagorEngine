// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>

struct HeroWtmAndBox
{
  TMatrix resWtm = TMatrix::IDENT;
  Point3 resWofs = {};
  BBox3 resLbox;
  bool resReady = false;
  enum
  {
    WEAPON = 1,
    VEHICLE = 2
  };
  uint8_t resFlags = 0; // WEAPON|VEHICLE
  bool onlyWeapons = false;
};