// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// fields that related to raytrace part of pipeline

#include "util/tracked_state.h"
#include "driver.h"
#include "raytrace/pipeline.h"

namespace drv3d_vulkan
{

class RaytracePipeline;
struct BackRaytraceStateStorage;
struct FrontRaytraceStateStorage;

struct StateFieldRaytraceProgram : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericTaggedHandle<ProgramID>
{
  ProgramID &getValue() { return handle; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldRaytracePipeline :
  // prefiltered by raytrace prog state
  TrackedStateFieldBase<false, false>,
  TrackedStateFieldGenericPtr<RaytracePipeline>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldRaytraceLayout : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<RaytracePipelineLayout>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

// TODO: remove this when all data is properly tracked
struct StateFieldRaytraceFlush : TrackedStateFieldBase<false, false>
{
  template <typename StorageType>
  void reset(StorageType &)
  {}
  void set(uint32_t) {}
  bool diff(uint32_t) { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

} // namespace drv3d_vulkan
