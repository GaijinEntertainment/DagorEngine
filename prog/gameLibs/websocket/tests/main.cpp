// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_cpuJobs.h>

// the websocket threads resolve their worker affinity from the core count on start
#define CUSTOM_UNITTEST_CODE \
  os_sockets_init();         \
  cpujobs::init();
#define CUSTOM_UNITTEST_SHUTDOWN_CODE cpujobs::term(false);

#include <unittest/mainCatch2.inc.cpp>
#include <unittest/waitUserOnProcessExit.inc.cpp>
