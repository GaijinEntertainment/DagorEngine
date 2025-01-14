// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if HAS_AMD_GPU_SERVICES
#include <driver.h>

#include <amd_ags.h>

namespace drv3d_dx12::debug::ags
{

AGSContext *get_context();

bool create_device_with_user_markers(AGSContext *ags_context, DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level,
  void **ptr);

} // namespace drv3d_dx12::debug::ags
#endif
