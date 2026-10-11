// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sockets.h>
#include <perfMon/dag_cpuFreq.h>

#define CUSTOM_UNITTEST_CODE \
  measure_cpu_freq();        \
  os_sockets_init();

#include <unittest/mainCatch2.inc.cpp>
#include <unittest/waitUserOnProcessExit.inc.cpp>
