// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#define __DEBUG_FILEPATH          "*"
#include <startup/dag_mainCon.inc.cpp>

size_t dagormem_max_crt_pool_sz = 256 << 20;
size_t dagormem_first_pool_sz = 512 << 20;
size_t dagormem_next_pool_sz = 256 << 20;
