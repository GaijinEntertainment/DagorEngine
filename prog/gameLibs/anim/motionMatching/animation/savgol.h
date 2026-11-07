// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

// Savitzky–Golay filter
// https://en.wikipedia.org/wiki/Savitzky%E2%80%93Golay_filter

// calculate weights
// m - window, n - polyorder, (m*2+1) is window size, needs 0 <= m, 0 <= n <= m*2
dag::Vector<vec4f> savgol_weigths(const int m, const int n);

// apply savgol filter
// w - weigths[m*2+1], y - in features, Y - out features, m - window, n - features count
void savgol_filter(const vec4f *__restrict w, const vec3f *__restrict y, vec3f *__restrict Y, int m, int n);