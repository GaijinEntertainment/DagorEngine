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

// dynamic states that exist only when VK_EXT_extended_dynamic_state is available; kept separate from
// BackDynamicGraphicsState so the whole group is trivially skipped (never dirtied, never applied) on
// devices without the extension
struct BackExtDynamicGraphicsStateStorage
{
  StateFieldGraphicsExtCullMode cullMode;
  StateFieldGraphicsExtDepthTest depthTest;
  StateFieldGraphicsExtDepthBoundsTestEnable depthBoundsTestEnable;
  StateFieldGraphicsExtStencilTest stencilTest;

  void reset() {}
  void dumpLog() const { debug("BackExtDynamicGraphicsStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class BackExtDynamicGraphicsState
  : public TrackedState<BackExtDynamicGraphicsStateStorage, StateFieldGraphicsExtCullMode, StateFieldGraphicsExtDepthTest,
      StateFieldGraphicsExtDepthBoundsTestEnable, StateFieldGraphicsExtStencilTest>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

} // namespace drv3d_vulkan
