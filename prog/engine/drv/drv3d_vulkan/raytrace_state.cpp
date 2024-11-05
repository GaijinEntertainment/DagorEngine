// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"

#if D3D_HAS_RAY_TRACING
#include "raytrace_state.h"
#include "execution_state.h"
#include "pipeline_state.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(FrontRaytraceState, raytrace, PipelineStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(BackRaytraceState, raytrace, ExecutionStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void BackRaytraceStateStorage::makeDirty() {}

void FrontRaytraceStateStorage::makeDirty() { binds.makeDirty(); }

void BackRaytraceStateStorage::clearDirty() {}

void FrontRaytraceStateStorage::clearDirty() { binds.clearDirty(); }

#endif // D3D_HAS_RAY_TRACING