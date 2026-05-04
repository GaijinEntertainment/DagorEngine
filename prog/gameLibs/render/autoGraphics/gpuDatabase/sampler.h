// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>

namespace auto_graphics::auto_quality_preset::detail
{
using GpuScore = int;
using VramGb = int;
using GpuInfo = eastl::pair<GpuScore, VramGb>;

eastl::optional<GpuInfo> get_gpu_info();
} // namespace auto_graphics::auto_quality_preset::detail
