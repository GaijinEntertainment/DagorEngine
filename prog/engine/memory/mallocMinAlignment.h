// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if USE_DLMALLOC | USE_MIMALLOC | USE_IMPORTED_ALLOC
#define MALLOC_MIN_ALIGNMENT 16
#elif _TARGET_PC_WIN && !_TARGET_64BIT
#define MALLOC_MIN_ALIGNMENT 8
#elif _TARGET_C1 | _TARGET_C2

#else
#define MALLOC_MIN_ALIGNMENT 16
#endif
