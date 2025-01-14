// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// defines a set of states that control scope like elements of command buffer
// that should be handled at start of state apply

#include "util/tracked_state.h"
#include "compute_state.h"
#include "graphics_state2.h"
#include "state_field_execution_context.h"

namespace drv3d_vulkan
{

struct BackScopeStateStorage
{
  StateFieldGraphicsConditionalRenderingScopeCloser conditionalRenderCloser;
  StateFieldGraphicsRenderPassScopeCloser renderPassCloser;
  StateFieldGraphicsNativeRenderPassScopeCloser nativeRenderPassCloser;
  StateFieldExecutionStageCloser stageCloser;
  StateFieldGraphicsQueryScopeCloser graphicsQueryCloser;

  void reset() {}
  void dumpLog() const { debug("BackScopeStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class BackScopeState : public TrackedState<BackScopeStateStorage,
                         // order important operation finishes
                         StateFieldGraphicsConditionalRenderingScopeCloser, StateFieldGraphicsNativeRenderPassScopeCloser,
                         StateFieldGraphicsRenderPassScopeCloser, StateFieldGraphicsQueryScopeCloser, StateFieldExecutionStageCloser>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

} // namespace drv3d_vulkan