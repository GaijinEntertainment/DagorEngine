//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace statstd
{
// confidence interval (in %)
enum Confidence
{
  C90,
  C95,
  C99,
  C99p9,
  ConfidenceMax
};

double calc_good_average(const float *values, int count, Confidence conf, double *stdV, double *minV, double *maxV, double *spikeV,
  float *tempArr, bool limit_to_actual_vals); // tempArray should be of the size not less than count
// if limit_to_actual_vals, than minV(maxV), will be no less(more) than real values in array

}; // namespace statstd
