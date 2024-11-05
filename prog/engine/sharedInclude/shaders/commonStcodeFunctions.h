// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <EASTL/array.h>

inline Color4 srgb_read(Color4 v) { return Color4(pow(v.r, 2.2f), pow(v.g, 2.2f), pow(v.b, 2.2f), v.a); }

inline Color4 anim_frame(float arg0, float arg1, float arg2, float arg3)
{
  // arg0 - 0..1 - frame index
  int x = (int)arg1;     // 1 - frames by x
  int y = (int)arg2;     // 2 - frames by y
  int total = (int)arg3; // 3 - total frame count
  if (!x || !y || !total)
  {
    DAG_FATAL("invalid arguments in shader function 'anim_frame(%.4f, %d, %d, %d)'", arg0, x, y, total);
  }
  int picture = (int)(arg0 * (total - 1));

  // return frame texture coords
  return Color4(float(picture % x) / x, float(int(picture / y)) / y, 0.f, 0.f);
}

inline Color4 wind_coeff(float t)
{
  // wind variables - can be refactored in future (when times & power will totally corrected)
  constexpr size_t TIMES_NUM = 11;
  constexpr eastl::array<float, TIMES_NUM> TIMES = {1.0f, 0.66f, 0.8f, 1.0f, 0.66f, 0.8f, 1.0f, 0.66f, 0.8f, 1.0f, 1.0f};
  constexpr eastl::array<float, TIMES_NUM> MAX_POWERS = {1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.0f};
  constexpr eastl::array<float, TIMES_NUM> MIN_POWERS = {1.0f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f};
  eastl::array<float, TIMES_NUM> _times;

  float totF = 0.0f;
  for (int i = 0; i < TIMES_NUM; ++i)
    totF += TIMES[i];

  float f = 0.0f;
  for (int i = 0; i < TIMES_NUM; ++i)
  {
    f += TIMES[i] / totF;
    _times[i] = f;
  }

  int ind = 0;
  for (int i = 0; i < TIMES_NUM; ++i)
  {
    if (t <= _times[i])
    {
      ind = i;
      break;
    }
  }

  if (ind <= 0)
    t = t / _times[0];
  else
    t = (t - _times[ind - 1]) / (_times[ind] - _times[ind - 1]);

  return Color4(t, MIN_POWERS[ind] * (1.0f - t) + MAX_POWERS[ind] * t, 0.f, 0.f);
}

inline float fade_val(float t, float arg2)
{
  if (t > 0.5f)
  {
    t = 1.0f - (t - 0.5f) * 2.0f;
    t = 1.0f - t * t;
  }
  else
  {
    t *= 2.0f;
    t = 1.0f - t * t;
  }
  t = arg2 + t * (1.0f - arg2);

  return t;
}
