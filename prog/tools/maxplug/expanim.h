// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class ExpTMAnimCB
{
public:
  virtual ~ExpTMAnimCB() = default;
  virtual void interp_tm(TimeValue, Matrix3 &) = 0;
};

struct PosKey
{
  TimeValue t;
  Point3 p, i, o;
  int f;
};

struct RotKey
{
  TimeValue t;
  Quat p, i, o;
  int f;
};
