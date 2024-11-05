// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "buffer_resource.h"
#include "vk_to_string.h"

using namespace drv3d_vulkan;

String ExecutionSyncTracker::BufferOp::format() const
{
  return String(128, "syncBufOp %p:\n%p-%s [%u-%u]\n%s\n%s\n internal caller: %s\n external caller: %s", this, obj,
    obj->getDebugName(), area.offset, area.offset + area.size, formatPipelineStageFlags(laddr.stage),
    formatMemoryAccessFlags(laddr.access), caller.getInternal(), caller.getExternal());
}

bool ExecutionSyncTracker::BufferOp::conflicts(const BufferOp &cmp) const
{
  return area.intersects(cmp.area) && laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

bool ExecutionSyncTracker::BufferOp::verifySelfConflict(const BufferOp &cmp) const
{
  // use stageless conflict check, there is no way to self conflict in single action step, based on stages
  bool stagelessConflict = area.intersects(cmp.area) && laddr.conflictingStageless(cmp.laddr) && !completed && !cmp.completed;
  return !stagelessConflict || laddr.isNonConflictingUAVAccess(cmp.laddr);
}

void ExecutionSyncTracker::BufferOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier) const { barrier.addStagesSrc(laddr.stage); }

void ExecutionSyncTracker::BufferOp::addToBarrierByTemplateDst(PipelineBarrier &barrier)
{
  barrier.addStagesDst(laddr.stage);
  barrier.modifyBufferTemplate(obj);
  VkAccessFlags dstAccess = laddr.access;
  // validator is not happy about ending access to aliased memory with none access barrier
  // it is not quite clear in spec either (hw must flush data from caches and none barrier does not do that?)
  // use "safe" version for now, i.e. complete all accesses properly instead of discarding/ignoring them
  if (dstAccess == VK_ACCESS_NONE)
    dstAccess = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  barrier.modifyBufferTemplate({conflictingAccessFlags, dstAccess});
  // src ops are object sorted, so merged addition on last element is possible
  barrier.addBufferByTemplateMerged(area.offset, area.size);
  conflictingAccessFlags = VK_ACCESS_NONE;
}

void ExecutionSyncTracker::BufferOp::onConflictWithDst(BufferOp &dst)
{
  dst.conflictingAccessFlags |= laddr.access;
  // src must be fully included in dst range when we generate barriers by dst
  dst.area.merge(area);
}

void ExecutionSyncTracker::BufferOp::aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker)
{
  tracker.addBufferAccess({stage, VK_ACCESS_NONE}, obj, {0, obj->getBlockSize()});
}

void ExecutionSyncTracker::BufferOpsArray::removeRoSeal(Buffer *obj)
{
  arr[lastProcessed] = {obj->getRoSealReads(), obj, {0, obj->getBlockSize()}, {}, // we don't know actual caller
    VK_ACCESS_NONE,
    /*completed*/ false, /*dstConflict*/ false};
}