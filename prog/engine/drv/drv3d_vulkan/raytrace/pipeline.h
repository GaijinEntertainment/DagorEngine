// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if VULKAN_HAS_RAYTRACING && 0 // VK_NV_ray_tracing
#include "nv/pipeline.h"
#else
#include "stub/pipeline.h"
#endif