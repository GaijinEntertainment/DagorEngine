// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

inline Point3 _rnd_fvec(int &seed)
{
  Point3 vec;
  _rnd_fvec(seed, vec.x, vec.y, vec.z);
  return vec;
}

inline Point3 _rnd_svec(int &seed)
{
  Point3 vec;
  _rnd_svec(seed, vec.x, vec.y, vec.z);
  return vec;
}

inline Point3 _rnd_vec2f_1s(int &seed)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  return Point3(ix / 32768.0f, iy / 32768.0f, (iz * 2 - 32768.0) / 32768.0f);
}

inline Point3 _rnd_vec2s_1f(int &seed)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  return Point3((ix * 2 - 32768.0) / 32768.0f, (iy * 2 - 32768.0) / 32768.0f, iz / 32768.0f);
}
