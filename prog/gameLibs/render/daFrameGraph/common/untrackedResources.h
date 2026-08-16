// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_texFlags.h>

#include <render/daFrameGraph/detail/resourceType.h>


namespace dafg
{

inline bool untracked_resources_supported() { return d3d::get_driver_desc().caps.hasEnhancedResourceBarriers; }

inline uint32_t drop_unsupported_no_state_tracking(uint32_t flags, ResourceType res_type)
{
  if (untracked_resources_supported())
    return flags;
  if (res_type == ResourceType::Buffer)
    return flags & ~static_cast<uint32_t>(SBCF_NO_STATE_TRACKING);
  if (res_type == ResourceType::Texture)
    return flags & ~static_cast<uint32_t>(TEXCF_NO_STATE_TRACKING);
  return flags;
}

} // namespace dafg
