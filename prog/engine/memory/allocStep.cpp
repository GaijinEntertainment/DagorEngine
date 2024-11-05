// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "allocStep.h"

#if _TARGET_64BIT && !_TARGET_STATIC_LIB
size_t dagormem_first_pool_sz = size_t(8192) << 20;
size_t dagormem_next_pool_sz = 512 << 20;
#elif _TARGET_64BIT
size_t dagormem_first_pool_sz = size_t(2048) << 20;
size_t dagormem_next_pool_sz = 256 << 20;
#else
size_t dagormem_first_pool_sz = 256 << 20;
size_t dagormem_next_pool_sz = 128 << 20;
#endif
