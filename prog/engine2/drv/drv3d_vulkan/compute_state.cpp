#include "pipeline.h"
#include "device.h"
#include "compute_state.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(FrontComputeState, compute, PipelineStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(BackComputeState, compute, ExecutionStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void BackComputeStateStorage::makeDirty() {}

void FrontComputeStateStorage::makeDirty() { binds.makeDirty(); }

void BackComputeStateStorage::clearDirty() {}

void FrontComputeStateStorage::clearDirty() { binds.clearDirty(); }
