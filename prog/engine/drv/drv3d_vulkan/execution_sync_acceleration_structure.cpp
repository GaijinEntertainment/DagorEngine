// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"

#if VULKAN_HAS_RAYTRACING
#include "pipeline_barrier.h"
#include "raytrace_as_resource.h"
#include "vk_to_string.h"
#include "backend_interop.h"
#include "backend.h"

using namespace drv3d_vulkan;

template <>
String AccelerationStructureSyncOp::format() const
{
  return String(128, "syncASOp %p:\n%p-%s\n%s\n%s\n internal caller: %s\n external caller: %s", this, obj, obj->getDebugName(),
    formatPipelineStageFlags(laddr.stage), formatMemoryAccessFlags(laddr.access), caller.getInternal(), caller.getExternal());
}

template <>
bool AccelerationStructureSyncOp::conflicts(const AccelerationStructureSyncOp &cmp) const
{
  return laddr.conflicting(cmp.laddr) && !completed && !cmp.completed;
}

template <>
bool AccelerationStructureSyncOp::verifySelfConflict(const AccelerationStructureSyncOp &cmp) const
{
  // use stageless conflict check, there is no way to self conflict in single action step, based on stages
  return laddr.conflictingStageless(cmp.laddr) && !completed && !cmp.completed;
}

template <>
void AccelerationStructureSyncOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier)
{
  barrier.addMemory({laddr.stage, conflictingLAddr.stage}, {laddr.access, conflictingLAddr.access});
}

template <>
void AccelerationStructureSyncOp::addToBarrierByTemplateDst(PipelineBarrier &)
{}

template <>
void AccelerationStructureSyncOp::onConflictWithDst(AccelerationStructureSyncOp &dst)
{
  conflictingLAddr.merge(dst.laddr);
}

template <>
bool AccelerationStructureSyncOp::allowsConflictFromObject()
{
  return false;
}

template <>
bool AccelerationStructureSyncOp::hasObjConflict()
{
  return false;
}

template <>
bool AccelerationStructureSyncOp::mergeCheck(AccelerationStructureArea, uint32_t)
{
  return true;
}

template <>
bool AccelerationStructureSyncOp::isAreaPartiallyCoveredBy(const AccelerationStructureSyncOp &)
{
  return false;
}

template <>
bool AccelerationStructureSyncOp::processPartials()
{
  return false;
}

template <>
void AccelerationStructureSyncOp::onPartialSplit()
{}

template <>
void ExecutionSyncTracker::AccelerationStructureOpsArray::removeRoSeal(RaytraceAccelerationStructure *obj)
{
  // we don't know actual caller here
  arr[lastProcessed] = AccelerationStructureSyncOp(SyncOpUid::next(), {}, obj->getRoSealReads(), obj, {});
}

template <>
bool ExecutionSyncTracker::AccelerationStructureOpsArray::isRoSealValidForOperation(RaytraceAccelerationStructure *, uint32_t)
{
  return true;
}


#endif // VULKAN_HAS_RAYTRACING