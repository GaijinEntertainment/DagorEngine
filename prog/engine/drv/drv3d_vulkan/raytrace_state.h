// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// defines full RT ready state that can be applied to execution state

#if D3D_HAS_RAY_TRACING

#include "util/tracked_state.h"
#include "state_field_raytrace.h"
#include "state_field_resource_binds.h"
namespace drv3d_vulkan
{

struct FrontRaytraceStateStorage
{
  StateFieldRaytraceProgram prog;
  StateFieldStageResourceBinds<STAGE_RAYTRACE> binds;

  void reset() {}
  void dumpLog() const { debug("FrontRaytraceStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

struct BackRaytraceStateStorage
{
  StateFieldRaytraceLayout layout;
  StateFieldRaytracePipeline pipeline;
  StateFieldRaytraceFlush flush;

  void reset() {}
  void dumpLog() const { debug("BackRaytraceStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class FrontRaytraceState
  : public TrackedState<FrontRaytraceStateStorage, StateFieldRaytraceProgram, StateFieldStageResourceBinds<STAGE_RAYTRACE>>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

class BackRaytraceState
  : public TrackedState<BackRaytraceStateStorage, StateFieldRaytraceLayout, StateFieldRaytracePipeline, StateFieldRaytraceFlush>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

} // namespace drv3d_vulkan

#endif // D3D_HAS_RAY_TRACING