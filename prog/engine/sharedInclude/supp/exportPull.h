// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if !_TARGET_STATIC_LIB && defined(__B_KERNEL_LIB) && defined(EXPORT_PULL)
int EXPORT_PULL = 0;
#endif
