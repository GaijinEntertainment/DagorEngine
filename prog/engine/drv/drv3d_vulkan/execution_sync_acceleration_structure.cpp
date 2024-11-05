// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"

#if D3D_HAS_RAY_TRACING
#include "pipeline_barrier.h"
#include "raytrace_as_resource.h"
#include "vk_to_string.h"

using namespace drv3d_vulkan;

String ExecutionSyncTracker::AccelerationStructureOp::format() const
{
  return String(128, "syncASOp %p:\n%p-%s\n%s\n%s\n internal caller: %s\n external caller: %s", this, obj, obj->getDebugName(),
    formatPipelineStageFlags(laddr.stage), formatMemoryAccessFlags(laddr.access), caller.getInternal(), caller.getExternal());
}

bool ExecutionSyncTracker::AccelerationStructureOp::conflicts(const AccelerationStructureOp &cmp) const
{
  return laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

void ExecutionSyncTracker::AccelerationStructureOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier) const
{
  barrier.addStagesSrc(laddr.stage);
  barrier.addMemory({laddr.access, conflictingAccessFlags});
}

void ExecutionSyncTracker::AccelerationStructureOp::addToBarrierByTemplateDst(PipelineBarrier &barrier) const
{
  barrier.addStagesDst(laddr.stage);
}

void ExecutionSyncTracker::AccelerationStructureOp::onConflictWithDst(const ExecutionSyncTracker::AccelerationStructureOp &dst)
{
  conflictingAccessFlags |= dst.laddr.access;
}

void ExecutionSyncTracker::AccelerationStructureOpsArray::removeRoSeal(RaytraceAccelerationStructure *obj)
{
  arr[lastProcessed] = {obj->getRoSealReads(), obj, {}, {}, // we don't know actual
                                                            // caller
    VK_ACCESS_NONE,
    /*completed*/ false,
    /*dstConflict*/ false};
}

#endif // D3D_HAS_RAY_TRACING