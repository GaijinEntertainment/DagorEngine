// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/time.h>
#include <math.h>

class AccumTime final : public ITimeManager
{
  double advance(float dt, float &) override
  {
    accum += dt;
    return accum;
  }
  double getSeconds() const override { return accum; }
  int getMillis() const override { return (int)(accum * 1000.0 + 0.5); }
  int getAsyncMillis() const override { return getMillis(); }
  int getType4CC() const override { return _MAKE4C('ACMT'); }
  double accum = 0;
};

ITimeManager *create_accum_time() { return new AccumTime(); }
