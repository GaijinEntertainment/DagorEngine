// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class ExpTMAnimCB
{
public:
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

bool get_tm_anim(Tab<PosKey> &pos, Tab<RotKey> &rot, Tab<PosKey> &scl, Tab<TimeValue> &gkeys, Interval limit, Animatable *ctrl,
  ExpTMAnimCB &cb, TimeValue mindt, float pos_thr, float rot_thr, float scl_thr, float ort_thr, char usekeys, char usegkeys,
  char dontchkkeys);
