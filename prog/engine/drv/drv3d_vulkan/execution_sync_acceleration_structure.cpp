#include "execution_sync.h"

#if D3D_HAS_RAY_TRACING
#include "pipeline_barrier.h"
#include "raytrace_as_resource.h"

using namespace drv3d_vulkan;

String ExecutionSyncTracker::AccelerationStructureOp::format() const
{
  return String(128, "syncASOp: %p %s %s %s \n internal caller: %s", obj, formatPipelineStageFlags(laddr.stage),
    formatMemoryAccessFlags(laddr.access), obj->getDebugName(), caller.getInternal());
}

bool ExecutionSyncTracker::AccelerationStructureOp::conflicts(const AccelerationStructureOp &cmp) const
{
  return laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

void ExecutionSyncTracker::AccelerationStructureOp::modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src,
  LogicAddress &dst)
{
  // laddr is already merged so we can do single memory barrier per whole object "type" sync
  barrier.addMemory({src.access, dst.access});
}

void ExecutionSyncTracker::AccelerationStructureOp::onConflictWithDst(const ExecutionSyncTracker::AccelerationStructureOp &dst,
  size_t gpu_work_id)
{
  if (!dst.laddr.isWrite() && obj->requestedRoSeal(gpu_work_id))
    obj->activateRoSeal();
}

#endif // D3D_HAS_RAY_TRACING