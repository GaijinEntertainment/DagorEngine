// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "buffer_resource.h"
#include "vk_to_string.h"
#include "backend_interop.h"
#include "backend.h"


using namespace drv3d_vulkan;

template <>
String BufferSyncOp::format() const
{
  return String(128, "syncBufOp %p:\n%p-%s [%u-%u]\n%s\n%s\n internal caller: %s\n external caller: %s", this, obj,
    obj->getDebugName(), area.offset, area.offset + area.size, formatPipelineStageFlags(laddr.stage),
    formatMemoryAccessFlags(laddr.access), caller.getInternal(), caller.getExternal());
}

template <>
bool BufferSyncOp::conflicts(const BufferSyncOp &cmp) const
{
  return area.intersects(cmp.area) && laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

template <>
bool BufferSyncOp::verifySelfConflict(const BufferSyncOp &cmp) const
{
  // use stageless conflict check, there is no way to self conflict in single action step, based on stages
  bool stagelessConflict = area.intersects(cmp.area) && laddr.conflictingStageless(cmp.laddr) && !completed && !cmp.completed;
  return !stagelessConflict || laddr.isNonConflictingUAVAccess(cmp.laddr);
}

template <>
void BufferSyncOp::addToBarrierByTemplateSrc(PipelineBarrier &)
{}

template <>
void BufferSyncOp::addToBarrierByTemplateDst(PipelineBarrier &barrier)
{
  barrier.modifyBufferTemplate(obj);
  barrier.modifyBufferTemplateStage({conflictingLAddr.stage, laddr.stage});
  VkAccessFlags dstAccess = laddr.access;
  // validator is not happy about ending access to aliased memory with none access barrier
  // it is not quite clear in spec either (hw must flush data from caches and none barrier does not do that?)
  // use "safe" version for now, i.e. complete all accesses properly instead of discarding/ignoring them
  if (dstAccess == VK_ACCESS_NONE)
    dstAccess = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
  barrier.modifyBufferTemplate({conflictingLAddr.access, dstAccess});
  // src ops are object sorted, so merged addition on last element is possible
  barrier.addBufferByTemplateMerged(area.offset, area.size);

  conflictingLAddr.clear();
}

template <>
void BufferSyncOp::onConflictWithDst(BufferSyncOp &dst)
{
  dst.conflictingLAddr.merge(laddr);
  // src must be fully included in dst range when we generate barriers by dst
  dst.area.merge(area);
}

template <>
void BufferSyncOp::aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker)
{
  tracker.addBufferAccess({stage, VK_ACCESS_NONE}, obj, {0, obj->getBlockSize()});
}

template <>
bool BufferSyncOp::allowsConflictFromObject()
{
  return false;
}
template <>
bool BufferSyncOp::hasObjConflict()
{
  return false;
}
template <>
bool BufferSyncOp::mergeCheck(BufferArea, uint32_t)
{
  return true;
}
template <>
bool BufferSyncOp::isAreaPartiallyCoveredBy(const BufferSyncOp &)
{
  return false;
}
template <>
bool BufferSyncOp::processPartials()
{
  return false;
}

template <>
void BufferSyncOp::onPartialSplit()
{}

template <>
void ExecutionSyncTracker::BufferOpsArray::removeRoSeal(Buffer *obj)
{
  arr[lastProcessed] = BufferSyncOp(SyncOpUid::next(), {}, obj->getRoSealReads(), obj, {0, obj->getBlockSize()});
}

template <>
bool ExecutionSyncTracker::BufferOpsArray::isRoSealValidForOperation(Buffer *, uint32_t)
{
  return true;
}
