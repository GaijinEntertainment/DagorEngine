// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "allocStep.h"

#if !_TARGET_STATIC_LIB
size_t dagormem_max_crt_pool_sz = ~0u;
#else
size_t dagormem_max_crt_pool_sz = 256 << 10;
#endif
