// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_texFlags.h>

// DCC for the render targets that gain from it. Off on Xbox Series until it is measured there.
#if _TARGET_C2

#else
static constexpr uint32_t COMPRESSED_RT_OVERRIDE_FLAG = 0;
#endif
