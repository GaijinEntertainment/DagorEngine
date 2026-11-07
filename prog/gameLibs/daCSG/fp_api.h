// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace dacsg
{
// Non-inline on purpose: they prove the lib's FP flags
double two_sum_tail(double a, double b);
double fma_contraction_probe(double a, double b, double c);
bool nan_compare_probe(double a);
} // namespace dacsg
