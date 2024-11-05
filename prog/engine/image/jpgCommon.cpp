// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jpgCommon.h"

extern "C" void *_jpeg_alloc(unsigned sz) { return memalloc(sz, tmpmem); }

extern "C" void _jpeg_free(void *p) { memfree(p, tmpmem); }
