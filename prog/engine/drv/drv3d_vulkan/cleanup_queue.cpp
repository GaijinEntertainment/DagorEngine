// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cleanup_queue.h"

#include "buffer_resource.h"
#include "image_resource.h"
#include "driver.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include <EASTL/algorithm.h>
#include "globals.h"
#include "backend.h"
#include "timelines.h"

namespace drv3d_vulkan
{

template <>
Tab<CleanupQueue::DelayedCleanup<Buffer, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return bufferDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<Image, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return imageDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<VariatedGraphicsPipeline, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return variatedGrPipelineDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<ComputePipeline, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return computePipelineDestructions;
}

#if VULKAN_HAS_RAYTRACING
template <>
Tab<CleanupQueue::DelayedCleanup<RaytraceAccelerationStructure, CleanupTag::DESTROY_TOP>> &CleanupQueue::getQueue()
{
  return rtASTopDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<RaytraceAccelerationStructure, CleanupTag::DESTROY_BOTTOM>> &CleanupQueue::getQueue()
{
  return rtASBottomDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<RaytracePipeline, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return raytracePipelineDestructions;
}
#endif

template <>
Tab<CleanupQueue::DelayedCleanup<RenderPassResource, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return renderPassDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<MemoryHeapResource, CleanupTag::DESTROY>> &CleanupQueue::getQueue()
{
  return heapDestructions;
}


} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void CleanupQueue::backendAfterFrameSubmitCleanup()
{
  visitAll([](auto &data) {
    for (auto &i : data)
      i.backendCleanup();
  });
}

void CleanupQueue::backendAfterGPUCleanup()
{
  WinAutoLock lk(Globals::Mem::mutex);
  visitAll([](auto &data) {
    for (auto &i : data)
      i.backendFinish();
    data.clear();
  });
}

void CleanupQueue::dumpData(FaultReportDump &dump) const
{
  enumerateAll([&](const auto &data) {
    for (auto &i : data)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&i,
        String(64, "cleanup %s 0x%p tag %u", i.ptr->resTypeString(), i.ptr, i.getTag()));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)(i.ptr));
    }
  });
}

size_t CleanupQueue::capacity() const
{
  size_t ret = 0;
  enumerateAll([&ret](const auto &data) { ret += data.capacity(); });

  // DelayedCleanup size does not depend on T
  // can multiply once
  return ret * sizeof(DelayedCleanup<uint64_t, CleanupTag::DESTROY>);
}
