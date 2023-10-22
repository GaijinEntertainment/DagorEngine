#include "buffer_resource.h"
#include "cleanup_queue.h"
#include "image_resource.h"
#include "driver.h"
#if D3D_HAS_RAY_TRACING
#include "raytrace_as_resource.h"
#endif
#include "device.h"
#include <EASTL/algorithm.h>


namespace drv3d_vulkan
{

template <>
Tab<CleanupQueue::DelayedCleanup<Buffer, Buffer::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return bufferDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<Image, Image::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return imageDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<Image, Image::CLEANUP_EVICT>> &CleanupQueue::getQueue()
{
  return imageEvictions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<Image, Image::CLEANUP_DELAYED_DESTROY>> &CleanupQueue::getQueue()
{
  return imageDelayedDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<AsyncCompletionState, AsyncCompletionState::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return asyncCompletionStateDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<VariatedGraphicsPipeline, VariatedGraphicsPipeline::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return variatedGrPipelineDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<ComputePipeline, ComputePipeline::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return computePipelineDestructions;
}

#if D3D_HAS_RAY_TRACING
template <>
Tab<CleanupQueue::DelayedCleanup<RaytraceAccelerationStructure, RaytraceAccelerationStructure::CLEANUP_DESTROY_TOP>>
  &CleanupQueue::getQueue()
{
  return rtASTopDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<RaytraceAccelerationStructure, RaytraceAccelerationStructure::CLEANUP_DESTROY_BOTTOM>>
  &CleanupQueue::getQueue()
{
  return rtASBottomDestructions;
}

template <>
Tab<CleanupQueue::DelayedCleanup<RaytracePipeline, RaytracePipeline::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return raytracePipelineDestructions;
}
#endif

template <>
Tab<CleanupQueue::DelayedCleanup<RenderPassResource, RenderPassResource::CLEANUP_DESTROY>> &CleanupQueue::getQueue()
{
  return renderPassDestructions;
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void CleanupQueue::checkValid()
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(!referencedByGpuWork, "vulkan: use external storage for cleanups or reduce pending CPU replays");
#endif
}

void CleanupQueue::backendAfterFrameSubmitCleanup(ContextBackend &back)
{
#if DAGOR_DBGLEVEL > 0
  referencedByGpuWork = true;
#endif

  visitAll([&back](auto &data) {
    for (auto &i : data)
      i.backendCleanup(back);
  });
}

void CleanupQueue::backendAfterReplayCleanup(ContextBackend &back)
{
#if DAGOR_DBGLEVEL > 0
  referencedByGpuWork = true;
#endif

  visitAll([&back](auto &data) {
    for (auto &i : data)
      i.backendCleanup(back);
  });
  back.contextState.frame->cleanupsRefs.push_back(this);
}

void CleanupQueue::backendAfterGPUCleanup(ContextBackend &)
{
  SharedGuardedObjectAutoLock lock(get_device().resources);
  visitAll([](auto &data) {
    for (auto &i : data)
      i.backendFinish();
    data.clear();
  });

#if DAGOR_DBGLEVEL > 0
  referencedByGpuWork = false;
#endif
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
  return ret * sizeof(DelayedCleanup<uint64_t, 0>);
}
