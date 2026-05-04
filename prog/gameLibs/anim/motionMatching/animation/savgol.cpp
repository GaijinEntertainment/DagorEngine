// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug.h>
#include "savgol.h"

static double gram_poly(const int i, const int m, const int k)
{
  if (k > 0)
  {
    return (4. * k - 2.) / (k * (2. * m - k + 1.)) * (i * gram_poly(i, m, k - 1)) -
           ((k - 1.) * (2. * m + k)) / (k * (2. * m - k + 1.)) * gram_poly(i, m, k - 2);
  }
  else
  {
    return 1.;
  }
}

static double sub_fact(const int a, const int b)
{
  double gf = 1.;

  for (int j = (a - b) + 1; j <= a; j++)
  {
    gf *= j;
  }
  return gf;
}

static double savgol_weight(const int i, const int m, const int n)
{
  double w = 0.;
  for (int k = 0; k <= n; ++k)
  {
    w += (2 * k + 1) * (sub_fact(2 * m, k) / sub_fact(2 * m + k + 1, k + 1)) * gram_poly(i, m, k) * gram_poly(0, m, k);
  }
  return w;
}

dag::Vector<vec4f> savgol_weigths(const int m, const int n)
{
  dag::Vector<vec4f> weights(2 * m + 1);
  for (int i = 0; i < 2 * m + 1; ++i)
  {
    float w = savgol_weight(i - m, m, n);
    weights[i] = v_splats(w);
  }
  return weights;
}

void savgol_filter(const vec4f *__restrict w, const vec3f *__restrict y, vec3f *__restrict Y, int m, int n)
{
  // NOTE: Y[0:m] and Y[n-m:n] are left untouched. We need some good extrapolation for acceptable
  // result, or use something like scipy.signal.savgol_filter() with mode='interp'
  for (int i = 0; i < n; ++i)
  {
    // caclulate Y[i]
    if (min(i, n - 1 - i) < m)
    {
      Y[i] = y[i];
      continue;
    }
    int j0 = i - m;
    int j1 = i + m;
    vec3f sum = v_zero();
    for (int j = j0, k = 0; j <= j1; ++j, ++k)
      sum = v_add(sum, v_mul(y[j], w[k]));
    Y[i] = sum;
  }
}