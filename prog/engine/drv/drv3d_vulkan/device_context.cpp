// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_sleepPrecise.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <memory/dag_framemem.h>
#include <3d/gpuLatency.h>
#include "device_context.h"
#include "swapchain.h"
#include "buffer.h"
#include "texture.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "buffer_props.h"
#include "device_queue.h"
#include "backend.h"
#include "global_lock.h"
#include "frontend.h"
#include "global_const_buffer.h"
#include "queries.h"
#include "resource_upload_limit.h"
#include "execution_timings.h"
#include "frontend_pod_state.h"
#include "backend/cmd/renderpass.h"
#include "backend/cmd/sync.h"
#include "backend/cmd/resources.h"
#include "backend/cmd/misc.h"
#include "backend/cmd/screen.h"

using namespace drv3d_vulkan;

#define VULKAN_LOCK_FRONT() OSSpinlockScopedLock lockedDevice(mutex)

void DeviceContext::generateFaultReportAtFrameEnd()
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->generateFaultReport = true;
}

void DeviceContext::waitForItemPushSpace()
{
  DA_PROFILE_WAIT(DA_PROFILE_FUNC);
  while (!Globals::timelines.get<TimelineManager::CpuReplay>().waitAdvance(MAX_PENDING_REPLAY_ITEMS, MAX_REPLAY_WAIT_TRIES))
    D3D_ERROR("vulkan: replay takes too long. Compiling pipeline: %s",
      Backend::interop.blockingPipelineCompilation.load() ? "yes" : "no");
}

int DeviceContext::getFramerateLimitingFactor()
{
  if (executionMode != ExecutionMode::THREADED)
    return DRV3D_FRAMERATE_LIMITED_BY_NOTHING;

  auto &completedTimings = Frontend::timings.get(0);
  bool replayThreadWait = completedTimings.frontendBackendWaitDuration > 0;
  bool replayUnderfeed = completedTimings.backendFrontendWaitDuration > 0;
  bool gpuOverutilized = (completedTimings.gpuWaitDuration > FRAME_GPU_BOUND_THRESHOLD_US) ||
                         (profile_usec_from_ticks_delta(completedTimings.presentDuration) > LONG_PRESENT_DURATION_THRESHOLD_US);

  return (replayThreadWait ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_WAIT : 0) |
         (replayUnderfeed ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_UNDERFEED : 0) |
         (gpuOverutilized ? DRV3D_FRAMERATE_LIMITED_BY_GPU_UTILIZATION : 0);
}

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
void DeviceContext::executeDebugFlushImpl(const char *caller)
{
  // Debug flushes are attempted after every draw/dispatch call,
  // but when mid-query we cannot actually flush, as that would break
  // the occlusion query logic horribly.
  // If we tried to support this properly, we would have to do a separate
  // query per draw call and then somehow merge their results. This
  // merging must happen GPU-side and would require either additional
  // compute dispatches, or additional draw calls.
  // As occlusion queries are barely used in game code and debug flush mode
  // is not used all that often as well, implementing a complicated
  // merging scheme is not worth it.
  if (!Globals::surveys.anyScopesActive() && !Globals::occlusionQueries.activeScope() &&
      !Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
  {
    uint64_t flushIdx = ++Frontend::State::pod.debugFlushCount;

    if (flushIdx == Globals::cfg.flushAfterEachDrawAndDispatchRange.start ||
        flushIdx == Globals::cfg.flushAfterEachDrawAndDispatchRange.end ||
        (flushIdx % Globals::cfg.flushAfterEachDrawAndDispatchRange.dumpPeriod == 0))
      debug("vulkan: debug flush hit idx %016llX", flushIdx);

    if (flushIdx < Globals::cfg.flushAfterEachDrawAndDispatchRange.start ||
        flushIdx > Globals::cfg.flushAfterEachDrawAndDispatchRange.end)
      return;

    int64_t flushTime = 0;
    {
      ScopedTimer timer(flushTime);
      wait();
    }

    if (flushTime > LONG_FRAME_THRESHOLD_US)
    {
      D3D_ERROR("vulkan: debug flush for %s is too long. taken %u us (threshold %u us)", caller, flushTime, LONG_FRAME_THRESHOLD_US);
      debug("vulkan: front state dump for draw/dispatch ===");
      Frontend::State::pipe.dumpLog();
    }
  }
}
#endif

void DeviceContext::afterBackendFlush()
{
  Globals::bindless.afterBackendFlush();
  Frontend::GCB.onFrontFlush();
  Frontend::replay->frontFrameIndex = Frontend::State::pod.frameIndex;
  Frontend::replay->prevReadbacks = Frontend::readbacks.getForProcess();
  Frontend::readbacks.advance();
}

void DeviceContext::beforeSubmitReplay()
{
  setPipelineState();
  dispatchCmdNoLock<CmdCleanupPendingReferences>({Frontend::State::pipe});
  Frontend::replay->timestampQueryBlock = Globals::timestamps.onFrontFlush();
  Frontend::replay->occlusionQueryBlock = Globals::occlusionQueries.onFrontFlush();
  Frontend::replay->readbacks = Frontend::readbacks.getForProcess();
  Frontend::replay->nextReadbacks = &Frontend::readbacks.getForWrite();
}

void DeviceContext::submitReplay() DAG_TS_REQUIRES(mutex)
{
  Frontend::resUploadLimit.reset();
  cleanupFrontendReplayResources();

  // Any form of command buffer flushing is not supported while mid occlusion query
  // If it does happen, it probably is an application logic error
  // (e.g. someone is trying to wait for a fence mid survey)
  G_ASSERTF(!Globals::surveys.anyScopesActive(), "vulkan: trying to flush mid survey");
  auto &replay = Globals::timelines.get<TimelineManager::CpuReplay>();

  {
    TIME_PROFILE(vulkan_submit_acquire_wait);
    while (!replay.waitAcquireSpace(MAX_REPLAY_WAIT_TRIES))
      D3D_ERROR("vulkan: replay takes too long in full blocking conditions. Compiling pipeline: %s",
        Backend::interop.blockingPipelineCompilation.load() ? "yes" : "no");
  }

  Frontend::replay.restart();
  if (ExecutionMode::TRANSIENT == executionMode)
  {
    if (worker)
    {
      worker->terminate(true);
      worker.reset();
      initMode(ExecutionMode::DEFERRED);
    }
    else
      initMode(ExecutionMode::THREADED);
  }

  if (ExecutionMode::DEFERRED == executionMode)
  {
    mutex.unlock();
    replay.advance();
    mutex.lock();
  }
  else if (ExecutionMode::THREADED != executionMode)
    G_ASSERTF(0, "vulkan: trying to submit replay with bad execution mode %u", (int)executionMode);
}

void DeviceContext::reportAlliveObjects(FaultReportDump &dump)
{
  uint32_t cnt = 0;

  auto iterCb = [&cnt, &dump](const Resource *i) {
    FaultReportDump::RefId rid =
      dump.addTagged(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i, String(64, "%s %s", i->getDebugName(), i->resTypeString()));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_VK_HANDLE, i->getBaseHandle());

    if (i->getMemoryId() != -1)
    {
      const ResourceMemory &rmem = i->getMemory();
      rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_OBJ_MEM_SIZE, (uint64_t)rmem.size,
        String(64, "mem %u size, original %u", rmem.index, rmem.originalSize));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i);
    }

    rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_VK_HANDLE, i->getBaseHandle(), String(64, "handle of %p", i));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i);

    ++cnt;
  };

  WinAutoLock lk(Globals::Mem::mutex);
  Globals::Mem::res.iterateAllocated<Buffer>(iterCb);
#if VK_KHR_buffer_device_address
  auto iterCbDeviceAddr = [&dump](const Buffer *i) {
    // framemem buffers may reference outdated resource, so we can't get GPU addr for them (check handle!)
    if (!i->isFrameMem() && (i->getDescription().memoryClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER))
    {
      FaultReportDump::RefId rid =
        dump.addTagged(FaultReportDump::GlobalTag::TAG_GPU_ADDR, i->devOffsetLoc(0), String(64, "GPU address of %p", i));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i);
    }
  };
  if (Globals::VK::phy.hasDeviceBufferDeviceAddress)
    Globals::Mem::res.iterateAllocated<Buffer>(iterCbDeviceAddr);
#endif
  Globals::Mem::res.iterateAllocated<Image>(iterCb);
  Globals::Mem::res.iterateAllocated<RenderPassResource>(iterCb);
#if VULKAN_HAS_RAYTRACING
  Globals::Mem::res.iterateAllocated<RaytraceAccelerationStructure>(iterCb);
#endif
  dump.add(String(32, "%u objects are alive", cnt));
}

void DeviceContext::cleanupFrontendReplayResources()
{
  Frontend::tempBuffers.onFrameEnd([&](auto buffer) { dispatchCmdNoLock<CmdDestroyBuffer>({buffer}); });
  Frontend::frameMemBuffers.onFrameEnd();
}

void DeviceContext::setPipelineState()
{
  // no need to check dirty as we anyway check it inside transit
  // no need to clear dirty as we do it internaly (aka consume)
  Frontend::State::pipe.transit(*this);
}

void DeviceContext::copyBufferDiscardReorderable(const BufferRef &source, BufferRef &dest, uint32_t src_offset, uint32_t dst_offset,
  uint32_t data_size)
{
  RenderPassResource *rp =
    Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>();
  if (!rp)
  {
    copyBuffer(source, dest, src_offset, dst_offset, data_size);
    return;
  }
  D3D_ERROR("vulkan: reordering buffer copy to %p:%s out of nrp %p:%s", dest.buffer, dest.buffer->getDebugName(), rp,
    rp->getDebugName());
  // discarding buffer with followup restore of its contents, viable logic, but consumes too much memory
  // BufferRef oldDest = dest;
  // dest = dest.discard(memory_class, uav_format, buf_flags, 0);
  // Frontend::replay->reorderedBufferCopies.push_back({oldDest, dest, 0, 0, dest.visibleDataSize,
  // Frontend::State::pod.nativeRenderPassesCount});
  Frontend::replay->reorderedBufferCopies.push_back(
    {source, dest, src_offset, dst_offset, data_size, Frontend::State::pod.nativeRenderPassesCount});
}

void DeviceContext::copyBuffer(const BufferRef &source, const BufferRef &dest, uint32_t src_offset, uint32_t dst_offset,
  uint32_t data_size)
{
  G_ASSERTF(Globals::lock.isAcquired(), "vulkan: copy buffer %p:%s -> %p:%s called without GPU acquire!", source.buffer,
    source.buffer->getDebugName(), dest.buffer, dest.buffer->getDebugName());
  if (Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
    DAG_FATAL("vulkan: copy buffer %p:%s -> %p:%s called inside native render pass %p:%s! fix application logic to avoid this!",
      source.buffer, source.buffer->getDebugName(), dest.buffer, dest.buffer->getDebugName(),
      Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(),
      Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>()
        ->getDebugName());

  dispatchCmd<CmdCopyBuffer>({source, dest, src_offset, dst_offset, data_size});
}

void DeviceContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value,
  bool unordered)
{
  CmdClearDepthStencilTexture cmd{image, area, value};

  if (Globals::lock.isAcquired() && !unordered)
  {
    if (Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
      DAG_FATAL("vulkan: clear depth called inside native render pass %p:%s! fix application logic to avoid this!",
        Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(),
        Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>()
          ->getDebugName());
    dispatchCmd(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->unorderedImageDepthStencilClears.push_back(cmd);
  }
}

void DeviceContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value, bool unordered)
{
  G_ASSERTF(image, "vulkan: trying to clear null image");
  CmdClearColorTexture cmd{image, area, value};

  if (Globals::lock.isAcquired() && !unordered)
  {
    if (Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
      DAG_FATAL("vulkan: clear color called inside native render pass %p:%s! fix application logic to avoid this!",
        Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(),
        Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>()
          ->getDebugName());
    dispatchCmd(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->unorderedImageColorClears.push_back(cmd);
  }
}

void DeviceContext::flushDraws()
{
  waitForItemPushSpace();

  VULKAN_LOCK_FRONT();
  beforeSubmitReplay();
  dispatchCmdNoLock<CmdFlushDraws>({});
  submitReplay();
  afterBackendFlush();
}

void DeviceContext::copyImageToBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions,
  AsyncCompletionState *sync)
{
  ImageCopyInfo info;
  info.image = image;
  info.buffer = buffer;
  info.copyCount = region_count;
  VULKAN_LOCK_FRONT();
  auto &rb = Frontend::readbacks.getForWrite().images;
  info.copyIndex = rb.copies.size();
  rb.info.push_back(info);
  rb.copies.insert(end(rb.copies), regions, regions + region_count);

  if (sync)
    sync->request(Frontend::replay->id + Frontend::readbacks.getLatency());
}

void DeviceContext::copyBufferToImage(Buffer *src, Image *dst, uint32_t region_count, VkBufferImageCopy *regions, bool ordered_copy)
{
  if (ordered_copy)
  {
    CmdCopyBufferToImageOrdered cmd{dst, src, regions[0]};
    VULKAN_LOCK_FRONT();
    dispatchCmdNoLock(cmd);

    for (uint32_t i = 1; i < region_count; ++i)
    {
      cmd.region = regions[i];
      dispatchCmdNoLock(cmd);
    }
  }
  else
  {
    ImageCopyInfo info;
    info.image = dst;
    info.buffer = src;
    info.copyCount = region_count;
    VULKAN_LOCK_FRONT();
    info.copyIndex = Frontend::replay->imageUploadCopies.size();
    Frontend::replay->imageUploads.push_back(info);
    Frontend::replay->imageUploadCopies.insert(end(Frontend::replay->imageUploadCopies), regions, regions + region_count);
  }
}

bool DeviceContext::updateBindlessResource(uint32_t index, D3dResource *res)
{
  G_ASSERTF(Globals::desc.caps.hasBindless, "vulkan: updateBindlessResource somehow reached without bindless available");
  auto resType = res->getType();
  if (D3DResourceType::SBUF != resType)
  {
    auto tex = (BaseTex *)res;
    tex = tex->getForBindless();
    const ImageViewState &viewState = tex->getViewInfo();

    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessTexUpdates.push_back(BindlessTexUpdateInfo(index, tex->image, (BaseTex *)res, viewState));
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessBufUpdates.push_back(BindlessBufUpdateInfo(index, ((GenericBufferInterface *)res)->getBufferRef()));
  }

  return true;
}

void DeviceContext::updateBindlessResourceRange(D3DResourceType type, uint32_t index, const dag::ConstSpan<D3dResource *> &resources)
{
  G_ASSERTF(Globals::desc.caps.hasBindless, "vulkan: updateBindlessBufRange somehow reached without bindless available");

  if (type == D3DResourceType::SBUF)
  {
    dag::Vector<BindlessBufUpdateInfo, framemem_allocator> src;
    src.reserve(resources.size());
    for (const auto &resource : resources)
    {
      if (resource)
      {
        auto buf = (GenericBufferInterface *)resource;
        src.emplace_back(index++, buf->getBufferRef());
      }
      else
      {
        src.emplace_back(index++, 1);
      }
    }
    VULKAN_LOCK_FRONT();
    auto &dst = Frontend::replay->bindlessBufUpdates;
    dst.insert(dst.end(), src.begin(), src.end());
  }
  else
  {
    dag::Vector<BindlessTexUpdateInfo, framemem_allocator> src;
    src.reserve(resources.size());
    for (const auto &resource : resources)
    {
      if (resource)
      {
        auto owner = (BaseTex *)resource;
        auto tex = owner->getForBindless();
        src.emplace_back(index++, tex->image, owner, tex->getViewInfo());
      }
      else
      {
        src.emplace_back(index++, 1);
      }
    }
    VULKAN_LOCK_FRONT();
    auto &dst = Frontend::replay->bindlessTexUpdates;
    dst.insert(dst.end(), src.begin(), src.end());
  }
}

void DeviceContext::updateBindlessResourcesToNull(D3DResourceType type, uint32_t index, uint32_t count)
{
  VULKAN_LOCK_FRONT();
  if (D3DResourceType::SBUF != type)
    Frontend::replay->bindlessTexUpdates.push_back(BindlessTexUpdateInfo(index, count));
  else
    Frontend::replay->bindlessBufUpdates.push_back(BindlessBufUpdateInfo(index, count));
}

void DeviceContext::updateBindlessSampler(uint32_t index, const SamplerResource *sampler_res)
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->bindlessSamplerUpdates.push_back({index, sampler_res});
}

void DeviceContext::copyBindlessDescriptors(D3DResourceType type, uint32_t src, uint32_t dst, uint32_t count)
{
  VULKAN_LOCK_FRONT();
  if (D3DResourceType::SBUF != type)
    Frontend::replay->bindlessTexUpdates.push_back(BindlessTexUpdateInfo(src, dst, count));
  else
    Frontend::replay->bindlessBufUpdates.push_back(BindlessBufUpdateInfo(src, dst, count));
}

void DeviceContext::wait()
{
  TIME_PROFILE(vulkan_sync_wait);
  waitForItemPushSpace();

  ThreadedFence *fence;
  {
    // remove this lock when we are no longer depend on state on submits
    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    // avoid slowdown when we can't treat 2 screen updates as proper frame
    Frontend::latencyWaiter.reset();
    VULKAN_LOCK_FRONT();
    beforeSubmitReplay();

    fence = Globals::fences.alloc(false);
    dispatchCmdNoLock<CmdFlushAndWait>({fence});
    submitReplay();
    afterBackendFlush();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
  }
  // CmdFlushAndWait will end frame and trigger needed signals
  // so we are free to wait on GPU fence only
  fence->wait();
  {
    VULKAN_LOCK_FRONT();
    Globals::fences.free(fence);
  }
}

void DeviceContext::processAllPendingWork()
{
  // process pending work and cleanup on hold resources
  // this way replay queue and gpu queue will be fully processed
  for (int i = 0; i < GPU_TIMELINE_HISTORY_SIZE + REPLAY_TIMELINE_HISTORY_SIZE; ++i)
    wait();
}

void DeviceContext::shutdownImmediateConstBuffers()
{
  // disable immediate binding
  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);
  d3d::set_immediate_const(STAGE_CS, nullptr, 0);
}

void DeviceContext::shutdown()
{
  cleanupFrontendReplayResources();
  Frontend::tempBuffers.shutdown();
  Frontend::frameMemBuffers.shutdown();
  shutdownImmediateConstBuffers();

  // stop compiler early, to cleanup pending module deletions properly
  Backend::pipelineCompiler.shutdown();
  // remove any resources that are not fully
  // deleted due to being destroyed in binded state
  dispatchCmd<CmdShutdownPendingReferences>({});

  processAllPendingWork();
  shutdownWorkerThread();
  // no more commands can be executed after this point
  executionMode = ExecutionMode::INVALID;
  // consume semaphores that no one will wait for anymore
  Globals::VK::queue.consumeWaitSemaphores(Backend::gpuJob.get());

  Backend::bindless.shutdown();
  Backend::aliasedMemory.shutdown();
  Backend::gpuJob.end();
  Frontend::replay.end();
  Frontend::readbacks.shutdown();
  deviceResetEventHandlers.clear();
}

void DeviceContext::uploadBuffer(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src.bufOffset(src_offset);
  copy.dstOffset = dst.bufOffset(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src.buffer;
  info.dst = dst.buffer;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  info.copyIndex = Frontend::replay->bufferUploadCopies.size();
  Frontend::replay->bufferUploads.push_back(info);
  Frontend::replay->bufferUploadCopies.push_back(copy);
}

void DeviceContext::uploadBufferOrdered(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset,
  uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src.bufOffset(src_offset);
  copy.dstOffset = dst.bufOffset(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src.buffer;
  info.dst = dst.buffer;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  info.copyIndex = Frontend::replay->orderedBufferUploadCopies.size();
  Frontend::replay->orderedBufferUploads.push_back(info);
  Frontend::replay->orderedBufferUploadCopies.push_back(copy);
}

void DeviceContext::downloadBuffer(const BufferRef &src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src.bufOffset(src_offset);
  copy.dstOffset = dst->bufOffsetLoc(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src.buffer;
  info.dst = dst;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  auto &rb = Frontend::readbacks.getForWrite().buffers;
  info.copyIndex = rb.copies.size();
  rb.info.push_back(info);
  rb.copies.push_back(copy);
}

BufferRef DeviceContext::uploadToDeviceFrameMem(uint32_t size, const void *src)
{
  // use device resident memory in dedicated configuration to avoid minor device slowdown
  // otherwise use shared host/device memory to avoid issues (PowerVR) and unnecesary memory usage and operations
  return uploadToFrameMem(Globals::Mem::pool.hasDedicatedMemory() ? DeviceMemoryClass::DEVICE_RESIDENT_BUFFER
                                                                  : DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER,
    size, src);
}

BufferRef DeviceContext::uploadToFrameMem(DeviceMemoryClass memory_class, uint32_t size, const void *src)
{
  BufferRef ret = Frontend::frameMemBuffers.acquire(size, memory_class);
  if (!ret.buffer->hasMappedMemory())
  {
    BufferRef stage = Frontend::frameMemBuffers.acquire(size, DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER);
    memcpy(stage.ptrOffset(0), src, size);
    stage.markNonCoherentRange(0, size, true);
    uploadBuffer(stage, ret, 0, 0, size);
  }
  else
  {
    memcpy(ret.ptrOffset(0), src, size);
    ret.markNonCoherentRange(0, size, true);
  }
  return ret;
}

uint32_t DeviceContext::getPiplineCompilationQueueLength()
{
  VULKAN_LOCK_FRONT();
  return Backend::pipelineCompiler.getQueueLength();
}

size_t DeviceContext::getCurrentWorkItemId()
{
  VULKAN_LOCK_FRONT(); // we probably should avoid this
  return Frontend::replay->id;
}

void DeviceContext::present(uint32_t engine_present_frame_id)
{
  {
    ScopedTimerTicks watch(Frontend::timings.frontendBackendWaitDuration);
    waitForItemPushSpace();
  }

  uint32_t allocations = 0;
  {
    // if we allocated something on this frame, make a message to texql
    // to properly fit in memory restrictions
    WinAutoLock lk(Globals::Mem::mutex);
    allocations = Globals::Mem::pool.getAllocationCounter();
  }

  // should be called without memory guard locked
  {
    if (allocations > Frontend::State::pod.lastVisibleAllocations)
      Frontend::State::pod.lastVisibleAllocations = allocations;
  }

  callFrameEndCallbacks();

  Frontend::currentSwapchainToPresent->prePresent();

  size_t submittedReplay = 0;
  {
    VULKAN_LOCK_FRONT();
    submittedReplay = Frontend::replay->id;
    beforeSubmitReplay();

    advanceAndCheckTimingRecord();
    CmdPresent cmd = Frontend::currentSwapchainToPresent->present();
    cmd.enginePresentFrameId = engine_present_frame_id;
    dispatchCmdNoLock(cmd);
    submitReplay();
    Frontend::State::pod.frameIndex++;
    Frontend::State::pod.summaryAsyncPipelineCompilationFeedback = 0;
    afterBackendFlush();

    if (Globals::cfg.bits.allowPredictedLatencyWaitApp)
      Frontend::latencyWaiter.update(Frontend::timings.frontendBackendWaitDuration);
  }

  if (Globals::cfg.bits.allowSharedFenceLatencyWait)
  {
    DA_PROFILE_WAIT("vulkan_shared_fence_latency_wait");
    WinAutoLock lock(Backend::interop.pendingGpuWork.cs);
    if (submittedReplay > Backend::interop.pendingGpuWork.replayId && Backend::interop.pendingGpuWork.gpuFence)
      Backend::interop.pendingGpuWork.gpuFence->wait();
  }

  auto lowLatencyModule = GpuLatency::getInstance();
  if (Globals::cfg.bits.allowPredictedLatencyWaitApp && !(lowLatencyModule && lowLatencyModule->isEnabled()))
  {
    if (Globals::cfg.bits.autoPredictedLatencyWaitApp)
      Frontend::latencyWaiter.wait();
    else
      Frontend::latencyWaiter.markAsyncWait();
  }
}

void DeviceContext::flushBufferToHost(const BufferRef &buffer, ValueRange<uint32_t> range)
{
  BufferFlushInfo info;
  info.buffer = buffer.buffer;
  info.offset = buffer.bufOffset(range.front());
  info.range = range.size();
  VULKAN_LOCK_FRONT();
  Frontend::readbacks.getForWrite().bufferFlushes.push_back(info);
}

void DeviceContext::waitForIfPending(AsyncCompletionState &sync)
{
  // wait over history length looks unreasonable, so limit with it
  int waitLimit = REPLAY_TIMELINE_HISTORY_SIZE;
  while (!sync.isCompleted() && waitLimit)
  {
    wait();
    --waitLimit;
  }
  G_ASSERTF(sync.isCompleted(), "vulkan: AsyncCompletionState %p not completed after wait", &sync);
}

void DeviceContext::initTempBuffersConfiguration()
{
  VkDeviceSize optimalTempBuffAllocBlockMb = ::dgs_get_settings()
                                               ->getBlockByNameEx("vulkan")
                                               ->getBlockByNameEx("allocators")
                                               ->getBlockByNameEx("ring")
                                               ->getInt64("pageSizeMb", 8);
  // leave 1Mb unoccupied in ring block, as other logic may use that block too,
  // greatly reducing chance to fit into it
  optimalTempBuffAllocBlockMb = optimalTempBuffAllocBlockMb > 1 ? optimalTempBuffAllocBlockMb - 1 : 1;
  VkDeviceSize tempBufferAligments = Globals::VK::phy.properties.limits.optimalBufferCopyOffsetAlignment;
  VkDeviceSize optimalTempBuffAllocBlock = optimalTempBuffAllocBlockMb << 20;

  Frontend::tempBuffers.setConfig(optimalTempBuffAllocBlock, tempBufferAligments, 0);

  Frontend::frameMemBuffers.init();
}

void DeviceContext::WorkerThread::execute()
{
  TIME_PROFILE_THREAD(getCurrentThreadName());

  auto &replayTimeline = Globals::timelines.get<TimelineManager::CpuReplay>();
  while (!isThreadTerminating())
  {
    // wait for at least one work item to be processed
    if (replayTimeline.waitSubmit(1, 1))
      replayTimeline.advance();
  }
}

void DeviceContext::advanceAndCheckTimingRecord()
{
  // save current frame data
  uint64_t now = profile_ref_ticks();
  auto &frameTiming = Frontend::timings.next(Frontend::State::pod.frameIndex, now);
  frameTiming.frontendWaitForSwapchainDuration = Frontend::latencyWaiter.getLastWaitTimeTicks();
  frameTiming.backbufferAcquireDuration = Frontend::timings.acquireBackBufferDuration;

  dispatchCmdNoLock<CmdRecordFrameTimings>({&frameTiming, now});
}

void DeviceContext::registerFrameEventsCallback(FrameEvents *callback, bool useFront)
{
  VULKAN_LOCK_FRONT();
  if (useFront)
  {
    callback->beginFrameEvent();
    frameEventCallbacks.emplace_back(callback);
  }
  else
    dispatchCmdNoLock<CmdRegisterFrameEventsCallback>({callback});
}

void DeviceContext::callFrameEndCallbacks()
{
  for (FrameEvents *callback : frameEventCallbacks)
    callback->endFrameEvent();
  frameEventCallbacks.clear();
}

DeviceContext::DeviceContext()
{
  Backend::State::exec.reset();
  Backend::State::pipe.reset();
}
