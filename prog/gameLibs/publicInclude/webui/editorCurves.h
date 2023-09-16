//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

class EditorCurve
{
  int type;

  float steps(float x, const Tab<float> &a) const
  {
    int count = a.size();
    if (count < 2)
      return 0.f;

    int idx = 0;
    for (int i = 0; i < count; i += 2)
      if (a[i] < x)
        idx = i;

    return a[idx + 1];
  }

  float poly(float x, const Tab<float> &a) const
  {
    int count = a.size();
    if (count == 0)
      return 0.f;

    float xp = 1.0f;
    float sum = 0.0f;
    for (int i = 0; i < count; i++)
    {
      sum += xp * a[i];
      xp *= x;
    }

    return sum;
  }

  float piecewiseLinear(float x, const Tab<float> &a) const
  {
    int count = a.size();
    if (count < 2)
      return 0.f;
    if (count < 3)
      return a[1];

    int idx = 0;
    for (int i = 0; i < count; i += 3)
      if (a[i] < x)
        idx = i;

    return a[idx + 1] + a[idx + 2] * (x - a[idx]);
  }

  float piecewiseMonotonic(float x, const Tab<float> &a) const // a = [ xs, ys, c1s, c2s, c3s, ... ]
  {
    int count = a.size();
    if (count < 2)
      return 0.f;
    if (count < 5)
      return a[1];

    int low = 0;
    int mid = 0;
    int high = (count / 5) - 1;
    while (low <= high)
    {
      mid = (low + high) / 2;
      float xHere = a[mid * 5];
      if (xHere < x)
        low = mid + 1;
      else if (xHere > x)
        high = mid - 1;
      else
        return a[mid * 5 + 1];
    }
    int i = max(0, high) * 5;

    float diff = x - a[i];
    float diffSq = diff * diff;
    return a[1 + i] + a[2 + i] * diff + a[3 + i] * diffSq + a[4 + i] * diff * diffSq;
  }


public:
  enum
  {
    STEPS,
    POLYNOM,
    PIECEWISE_LINEAR,
    PIECEWISE_MONOTONIC
  };

  Tab<float> coeffs;
  SimpleString asText;

  int getType() const { return type; }

  bool parse(const char *str, int type_ = -1) // str can be generated for example by curveEditor.js
  {
    G_ASSERT(str);
    if (!str)
      return false;

    asText = str;

    if (type_ >= 0)
      type = type_;
    else
    {
      if (strchr(str, 'S'))
        type = STEPS;
      else if (strchr(str, 'L'))
        type = PIECEWISE_LINEAR;
      else if (strchr(str, 'M'))
        type = PIECEWISE_MONOTONIC;
      else if (strchr(str, 'P'))
        type = POLYNOM;
      else
      {
        type = -1;
        return false;
      }
    }


    const char *next = str;
    clear_and_shrink(coeffs);

    for (;;)
    {
      while (*next == ' ' || *next == ',')
        next++;

      str = next;
      float v = float(strtod(str, (char **)&next));
      if (next == str)
        break;
      else
        coeffs.push_back(v);
    }

    if (type == STEPS)
      return coeffs.size() > 0 && coeffs.size() % 2 == 0;
    if (type == POLYNOM)
      return coeffs.size() > 0;
    else if (type == PIECEWISE_LINEAR)
      return coeffs.size() > 0 && coeffs.size() % 3 == 0;
    else if (type == PIECEWISE_MONOTONIC)
      return coeffs.size() > 0 && coeffs.size() % 5 == 0;
    else
      return false;
  }

  float getValue(float t) const
  {
    switch (type)
    {
      case STEPS: return steps(t, coeffs);

      case POLYNOM: return poly(t, coeffs);

      case PIECEWISE_LINEAR: return piecewiseLinear(t, coeffs);

      case PIECEWISE_MONOTONIC: return piecewiseMonotonic(t, coeffs);

      default: G_ASSERT(0); return 0.f;
    }
  }
};
