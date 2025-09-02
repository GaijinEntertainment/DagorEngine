//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

struct PreciseSleepContext
{
  // Note: float min step (precision) on values 16000 us (i.e 16 ms) is ~1ns
  float estimate = 2e3f;
  float mean = 2e3f;
  float m2 = 0;
  int64_t count = 1;
};

// Same as sleep_msec() but more accurate (with tradeoff of using more CPU).
// - This API is assumed to be used for short to mediums sleep times (not longer then 10s of milleceonds).
void sleep_precise_usec(int time_usec, PreciseSleepContext &ctx);
