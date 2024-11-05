// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_amdFsr.h>

namespace drv3d_vulkan
{
class Image;
}

using FSRUpscalingArgs = amd::FSR::UpscalingArgsBase<drv3d_vulkan::Image>;
