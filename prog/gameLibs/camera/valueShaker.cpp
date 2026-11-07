// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <camera/valueShaker.h>
#include <math/dag_mathUtils.h>
#include <math/random/dag_random.h>

ValueShaker::ValueShaker() : value(ZERO<Point3>()), vel(ZERO<Point3>()), fadeKoeff(0.0f), amp(0.0f) {}

void ValueShaker::reset()
{
  value = Point3(0, 0, 0);
  vel = Point3(0, 0, 0);
}

void ValueShaker::setup(float fadeKoeff_, float amp_)
{
  fadeKoeff = fadeKoeff_;
  amp = amp_;
}

Point3 ValueShaker::getNextValue(float dt)
{
  for (; dt > 0; dt -= 0.02f)
  {
    float t = min(dt, 0.02f), decay = approach(1.f, 0.f, t, fadeKoeff);
    vel = vel * decay + Point3(gsrnd(), gsrnd(), gsrnd()) * t * amp;
    value = value * decay + vel * amp;
  }
  return value;
}
