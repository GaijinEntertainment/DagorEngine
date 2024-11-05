// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// defines full dispatch ready state that can be applied to execution state

#include "util/tracked_state.h"
#include "state_field_compute.h"
#include "state_field_resource_binds.h"

namespace drv3d_vulkan
{

struct FrontComputeStateStorage
{
  StateFieldComputeProgram prog;
  StateFieldStageResourceBinds<STAGE_CS> binds;

  void reset() {}
  void dumpLog() const { debug("FrontComputeStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

struct BackComputeStateStorage
{
  StateFieldComputeLayout layout;
  StateFieldComputePipeline pipeline;
  StateFieldComputeFlush flush;

  void reset() {}
  void dumpLog() const { debug("BackComputeStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class FrontComputeState
  : public TrackedState<FrontComputeStateStorage, StateFieldComputeProgram, StateFieldStageResourceBinds<STAGE_CS>>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

class BackComputeState
  : public TrackedState<BackComputeStateStorage, StateFieldComputeLayout, StateFieldComputePipeline, StateFieldComputeFlush>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

} // namespace drv3d_vulkan
