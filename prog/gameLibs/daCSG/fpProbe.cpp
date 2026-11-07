// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameLibs/daCSG/fp_api.h>

namespace dacsg
{
double two_sum_tail(double a, double b)
{
  double s = a + b;
  double bv = s - a;
  double av = s - bv;
  return (a - av) + (b - bv);
}

double fma_contraction_probe(double a, double b, double c) { return a * b + c; }

bool nan_compare_probe(double a) { return a != a; }
} // namespace dacsg
