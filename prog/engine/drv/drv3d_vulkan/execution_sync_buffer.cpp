#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "buffer_resource.h"

using namespace drv3d_vulkan;

String ExecutionSyncTracker::BufferOp::format() const
{
  return String(128, "syncBufOp: %p [%u-%u] %s %s %s \n internal caller: %s", obj, area.offset, area.offset + area.size,
    formatPipelineStageFlags(laddr.stage), formatMemoryAccessFlags(laddr.access), obj->getDebugName(), caller.getInternal());
}

bool ExecutionSyncTracker::BufferOp::conflicts(const BufferOp &cmp) const
{
  return area.intersects(cmp.area) && laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

void ExecutionSyncTracker::BufferOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier) const
{
  barrier.modifyBufferTemplate(obj);
  barrier.addBufferByTemplate(area.offset, area.size, laddr.access);
}

void ExecutionSyncTracker::BufferOp::modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src, LogicAddress &dst)
{
  barrier.modifyBufferTemplate({src.access, dst.access});
}

void ExecutionSyncTracker::BufferOp::onConflictWithDst(const BufferOp &dst, size_t gpu_work_id)
{
  if (!dst.laddr.isWrite() && obj->requestedRoSeal(gpu_work_id))
    obj->activateRoSeal();
}
