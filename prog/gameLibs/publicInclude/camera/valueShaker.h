//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_math3d.h>

class ValueShaker
{
private:
  Point3 value;
  Point3 vel;
  float fadeKoeff;
  float amp;

public:
  ValueShaker();
  ~ValueShaker() {}

  void reset();
  void setup(float fadeKoeff, float amp);
  Point3 getNextValue(float dt);
};
