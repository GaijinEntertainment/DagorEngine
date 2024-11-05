// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if D3D_HAS_RAY_TRACING && 0 // VK_NV_ray_tracing
#include "nv/pipeline.h"
#else
#include "stub/pipeline.h"
#endif