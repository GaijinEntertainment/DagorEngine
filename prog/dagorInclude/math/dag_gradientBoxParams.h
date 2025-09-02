//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_color.h>
#include <EASTL/vector.h>

struct GradientBoxSampler
{
  static constexpr int MAX_POINTS = 64;

  struct GradientKey
  {
    float pos;
    E3DCOLOR color;
  };

  eastl::vector<GradientKey> values;

  GradientBoxSampler() {}
  GradientBoxSampler(const GradientBoxSampler &r) : values(r.values) {}

  GradientBoxSampler &operator=(const GradientBoxSampler &r)
  {
    values = r.values;
    return *this;
  }

  E3DCOLOR sample(float v) const
  {
    G_FAST_ASSERT(values.size() > 0);
    G_FAST_ASSERT(v >= 0 && v <= 1.f);

    if (values.size() == 1)
      return values[0].color;

    for (int i = 1; i < values.size(); ++i)
    {
      if (values[i].pos >= v)
      {
        float k0 = values[i - 1].pos;
        float k1 = values[i].pos;

        float k = (v - k0) * safeinv(k1 - k0);
        return e3dcolor_lerp(values[i - 1].color, values[i].color, k);
      }
    }

    return values.back().color;
  }

  bool load(const char *&ptr, int &len)
  {
    int count = *(const int *)ptr;
    G_FAST_ASSERT(count <= MAX_POINTS);

    ptr += sizeof(int);
    len -= sizeof(int);

    for (int i = 0; i < count; ++i)
    {
      float p = *(const float *)ptr;
      ptr += sizeof(int);

      E3DCOLOR c = *(const E3DCOLOR *)ptr;
      ptr += sizeof(E3DCOLOR);

      values.push_back({p, c});
    }

    len -= (sizeof(int) + sizeof(float)) * count;
    return true;
  }
};
