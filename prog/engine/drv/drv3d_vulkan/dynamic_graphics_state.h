// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// dynamic states that are applied post pipe bind

#include "util/tracked_state.h"
#include "state_field_graphics.h"

namespace drv3d_vulkan
{
struct BackDynamicGraphicsStateStorage
{
  StateFieldGraphicsScissor scissor;
  StateFieldGraphicsDepthBias depthBias;
  StateFieldGraphicsStencilMask stencilMask;
  StateFieldGraphicsStencilRef stencilRef;
  StateFieldGraphicsStencilRefOverride stencilRefOverride;

  void reset() {}
  void dumpLog() const { debug("BackDynamicGraphicsStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class BackDynamicGraphicsState
  : public TrackedState<BackDynamicGraphicsStateStorage, StateFieldGraphicsScissor, StateFieldGraphicsDepthBias,
      StateFieldGraphicsStencilRef, StateFieldGraphicsStencilRefOverride, StateFieldGraphicsStencilMask>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

} // namespace drv3d_vulkan
