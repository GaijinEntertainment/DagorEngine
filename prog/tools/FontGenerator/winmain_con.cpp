// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#define __DEBUG_FILEPATH          (::dgs_get_argv("dbgfile") ? ::dgs_get_argv("dbgfile") : "debug")
#include <startup/dag_mainCon.inc.cpp>

#include "mutexAA.h"
Tab<void *> MutexAutoAcquire::g_mutexes(inimem);
Tab<SimpleString> MutexAutoAcquire::g_mutex_names(inimem);
