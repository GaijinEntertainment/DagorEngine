// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "allocStep.h"

#if _TARGET_64BIT && !_TARGET_STATIC_LIB
size_t dagormem_first_pool_sz = 16ull << 30;
size_t dagormem_next_pool_sz = 1024 << 20;
#elif _TARGET_64BIT
size_t dagormem_first_pool_sz = 4ull << 30;
size_t dagormem_next_pool_sz = 512 << 20;
#else
size_t dagormem_first_pool_sz = 256 << 20;
size_t dagormem_next_pool_sz = 128 << 20;
#endif
