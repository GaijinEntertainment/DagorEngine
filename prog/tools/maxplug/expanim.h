// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class ExpTMAnimCB
{
public:
  virtual ~ExpTMAnimCB() = default;
  virtual void interp_tm(TimeValue, Matrix3 &) = 0;
  virtual void non_orthog_tm(TimeValue) = 0;
  virtual const TCHAR *get_name() = 0;
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
