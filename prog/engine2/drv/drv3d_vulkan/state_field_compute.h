// fields that related to compute part of pipeline
#pragma once
#include "util/tracked_state.h"
#include "driver.h"
#include "pipeline/main_pipelines.h"

namespace drv3d_vulkan
{

class ComputePipeline;
struct BackComputeStateStorage;
struct FrontComputeStateStorage;

struct StateFieldComputeProgram : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericTaggedHandle<ProgramID>
{
  ProgramID &getValue() { return handle; }
  const ProgramID &getValueRO() const { return handle; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldComputePipeline :
  // prefiltered by compute prog state
  TrackedStateFieldBase<false, false>,
  TrackedStateFieldGenericPtr<ComputePipeline>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldComputeLayout : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<ComputePipelineLayout>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

// TODO: remove this when all data is properly tracked
struct StateFieldComputeFlush : TrackedStateFieldBase<false, false>
{
  template <typename StorageType>
  void reset(StorageType &)
  {}
  void set(uint32_t) {}
  bool diff(uint32_t) { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

} // namespace drv3d_vulkan
