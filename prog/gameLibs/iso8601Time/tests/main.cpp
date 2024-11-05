// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <UnitTest++/UnitTestPP.h>
#include <iso8601Time/iso8601Time.h>
#include <math/random/dag_random.h>
#include <perfMon/dag_cpuFreq.h>

#define TEST_CASE(x)  TEST(x)
#define TEST_CHECK(x) CHECK(x)

int generate_rand_int(int maxValue)
{
  static bool initialized = false;
  if (!initialized)
  {
    measure_cpu_freq();
    int seed = ref_time_ticks();
    printf("Seed: %d\n", seed);
    dagor_random::set_rnd_seed(seed);
  }
  // cannot reliably use rnd_int for numbers bigger than 65536
  int result = dagor_random::rnd_float(0, maxValue);
  printf("Random [0, %d]: %d\n", maxValue, result);
  return result;
}

#include "testsImpl.inc.cpp"

#include <unittest/main.inc.cpp>
