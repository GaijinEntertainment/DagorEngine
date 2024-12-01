// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMathDecl.h>

struct HeroWtmAndBox
{
  mutable mat44f resWtm = {};
  mutable vec3f resWofs = {};
  mutable bbox3f resLbox = {};
  mutable bool resReady = false;
  enum
  {
    WEAPON = 1,
    VEHICLE = 2
  };
  mutable uint8_t resFlags = 0; // WEAPON|VEHICLE
};