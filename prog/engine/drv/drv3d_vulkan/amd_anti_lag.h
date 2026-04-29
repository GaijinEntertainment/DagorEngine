// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// gpu latency logic implementation via vulkan AMD_anti_lag ext
#include <3d/gpuLatency.h>

namespace drv3d_vulkan
{

GpuLatency *create_gpu_latency_amd_anti_lag();

} // namespace drv3d_vulkan
