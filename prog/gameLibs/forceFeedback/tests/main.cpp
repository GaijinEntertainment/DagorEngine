// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_cpuJobs.h>

// the rumble thread reads the core count from cpujobs
#define CUSTOM_UNITTEST_CODE cpujobs::init();

#include <unittest/mainCatch2.inc.cpp>
