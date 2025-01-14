// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_context.h"
#include "swapchain.h"
#include <perfMon/dag_statDrv.h>
#include "buffer.h"
#include "texture.h"
#include <perfMon/dag_sleepPrecise.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include "globals.h"
#include "raytrace_scratch_buffer.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "buffer_alignment.h"
#include "device_queue.h"
#include "backend.h"
#include "global_lock.h"
#include "frontend.h"
#include "global_const_buffer.h"
#include "timestamp_queries.h"
#include "resource_upload_limit.h"
#include "execution_timings.h"
#include "frontend_pod_state.h"

using namespace drv3d_vulkan;

#define VULKAN_LOCK_FRONT() OSSpinlockScopedLock lockedDevice(mutex)

// clang-format off

#define VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd)          \
    Frontend::replay->pushCommand(cmd);             \
    verifyExecutionMode();

#define VULKAN_DISPATCH_COMMAND(cmd)     \
  {                                      \
    VULKAN_LOCK_FRONT();                 \
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd) \
  }

// clang-format on

void DeviceContext::generateFaultReportAtFrameEnd()
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->generateFaultReport = true;
}

void DeviceContext::waitForItemPushSpace()
{
  DA_PROFILE_WAIT(DA_PROFILE_FUNC);
  while (!Globals::timelines.get<TimelineManager::CpuReplay>().waitAdvance(MAX_PENDING_REPLAY_ITEMS, MAX_REPLAY_WAIT_CYCLES))
    D3D_ERROR("vulkan: replay takes too long");
}

int DeviceContext::getFramerateLimitingFactor()
{
  if (executionMode != ExecutionMode::THREADED)
    return DRV3D_FRAMERATE_LIMITED_BY_NOTHING;

  auto &completedTimings = Frontend::timings.get(0);
  bool replayThreadWait = completedTimings.frontendBackendWaitDuration > 0;
  bool replayUnderfeed = completedTimings.backendFrontendWaitDuration > 0;
  bool gpuOverutilized = (completedTimings.gpuWaitDuration > 0) ||
                         (profile_usec_from_ticks_delta(completedTimings.presentDuration) > LONG_PRESENT_DURATION_THRESHOLD_US);

  return (replayThreadWait ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_WAIT : 0) |
         (replayUnderfeed ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_UNDERFEED : 0) |
         (gpuOverutilized ? DRV3D_FRAMERATE_LIMITED_BY_GPU_UTILIZATION : 0);
}

void DeviceContext::executeDebugFlush(const char *caller)
{
  G_UNUSED(caller);
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
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
  if (Globals::cfg.bits.flushAfterEachDrawAndDispatch && !Globals::surveys.anyScopesActive() &&
      !Frontend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
  {
    int64_t flushTime = 0;
    {
      ScopedTimer timer(flushTime);
      wait();
    }

    if (flushTime > LONG_FRAME_THRESHOLD_US)
    {
      debug("vulkan: debug flush for %s is too long. taken %u us (threshold %u us)", caller, flushTime, LONG_FRAME_THRESHOLD_US);
      debug("vulkan: front state dump for draw/dispatch ===");
      Frontend::State::pipe.dumpLog();
    }
  }
#endif
}

void DeviceContext::afterBackendFlush()
{
  Frontend::GCB.onFrontFlush();
  Frontend::replay->frontFrameIndex = Frontend::State::pod.frameIndex;
}

void DeviceContext::beforeSubmitReplay()
{
  setPipelineState();
  CmdCleanupPendingReferences cleanupCmd{Frontend::State::pipe};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cleanupCmd);
  Globals::timestamps.onFrontFlush();
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
    while (!replay.waitAcquireSpace(MAX_REPLAY_WAIT_CYCLES))
      D3D_ERROR("vulkan: replay takes too long in full blocking conditions");
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
#if D3D_HAS_RAY_TRACING
  Globals::Mem::res.iterateAllocated<RaytraceAccelerationStructure>(iterCb);
#endif
  dump.add(String(32, "%u objects are alive", cnt));
}

void DeviceContext::cleanupFrontendReplayResources()
{
  Frontend::tempBuffers.onFrameEnd([&](auto buffer) {
    CmdDestroyBuffer cmd{buffer};
    Frontend::replay->pushCommand(cmd);
  });
  Frontend::frameMemBuffers.onFrameEnd();
}

void DeviceContext::updateDebugUIPipelinesData()
{
  CmdUpdateDebugUIPipelinesData cmd{};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::setPipelineUsability(ProgramID program, bool value)
{
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  CmdSetPipelineUsability cmd{program, value};
  VULKAN_DISPATCH_COMMAND(cmd);
#else
  G_UNUSED(program);
  G_UNUSED(value);
#endif
}

void DeviceContext::setConstRegisterBuffer(const BufferRef &ref, ShaderStage stage)
{
  auto &resBinds = Frontend::State::pipe.getStageResourceBinds(stage);
  if (resBinds.set<StateFieldGlobalConstBuffer, BufferRef>(ref))
    Frontend::State::pipe.markResourceBindDirty(stage);
}

void DeviceContext::setPipelineState()
{
  // no need to check dirty as we anyway check it inside transit
  // no need to clear dirty as we do it internaly (aka consume)
  Frontend::State::pipe.transit(*this);
}

void DeviceContext::compileGraphicsPipeline(const VkPrimitiveTopology top)
{
  Frontend::GCB.flushGraphics(*this);
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdCompileGraphicsPipeline cmd{top};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("compileGraphicsPipeline");
}
void DeviceContext::compileComputePipeline()
{
  Frontend::GCB.flushCompute(*this);
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdCompileComputePipeline cmd{};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("compileComputePipeline");
}

uint64_t DeviceContext::getTimestampResult(TimestampQueryId query_id)
{
  VULKAN_LOCK_FRONT();
  return Globals::timestamps.getResult(query_id);
}

TimestampQueryId DeviceContext::insertTimestamp()
{
  VULKAN_LOCK_FRONT();
  TimestampQueryIndex qIdx = Globals::timestamps.allocate();
  CmdInsertTimesampQuery cmd{qIdx};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  return Globals::timestamps.encodeId(qIdx, Frontend::replay->id);
}

void DeviceContext::nativeRenderPassChanged()
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdNativeRenderPassChanged cmd{};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
}

void DeviceContext::clearView(int clear_flags)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdClearView cmd{clear_flags};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("clearView");
}

void DeviceContext::allowOpLoad()
{
  CmdAllowOpLoad cmd{};
  VULKAN_DISPATCH_COMMAND(cmd);
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
  logerr("vulkan: reordering buffer copy to %p:%s out of nrp %p:%s", dest.buffer, dest.buffer->getDebugName(), rp, rp->getDebugName());
  // discarding buffer with followup restore of its contents, viable logic, but consumes too much memory
  // BufferRef oldDest = dest;
  // dest = discardBuffer(dest, memory_class, uav_format, buf_flags, 0);
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

  CmdCopyBuffer cmd{source, dest, src_offset, dst_offset, data_size};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::pushEvent(const char *name)
{
  CmdPushEvent cmd;
  VULKAN_LOCK_FRONT();
  cmd.index = StringIndexRef{Frontend::replay->charStore.size()};
  Frontend::replay->charStore.insert(Frontend::replay->charStore.end(), name, name + strlen(name) + 1);
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value)
{
  CmdClearDepthStencilTexture cmd{image, area, value};

  if (Globals::lock.isAcquired())
  {
    VULKAN_DISPATCH_COMMAND(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->unorderedImageDepthStencilClears.push_back(cmd);
  }
}

void DeviceContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value, bool unordered)
{
  CmdClearColorTexture cmd{image, area, value};

  if (Globals::lock.isAcquired() && !unordered)
  {
    VULKAN_DISPATCH_COMMAND(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->unorderedImageColorClears.push_back(cmd);
  }
}

void DeviceContext::copyImage(Image *src, Image *dst, const VkImageCopy *regions, uint32_t region_count, uint32_t src_mip,
  uint32_t dst_mip, uint32_t mip_count, bool unordered)
{
  {
    CmdCopyImage cmd;
    cmd.src = src;
    cmd.dst = dst;
    cmd.regionCount = region_count;
    cmd.srcMip = src_mip;
    cmd.dstMip = dst_mip;
    cmd.mipCount = mip_count;
    VULKAN_LOCK_FRONT();
    cmd.regionIndex = Frontend::replay->imageCopyInfos.size();
    Frontend::replay->imageCopyInfos.insert(Frontend::replay->imageCopyInfos.end(), regions, regions + region_count);

    // allow image copy reorder if we do them outside of render thread or with unordered flag
    if (Globals::lock.isAcquired() && !unordered)
    {
      VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    }
    else
      Frontend::replay->unorderedImageCopies.push_back(cmd);
  }
}

void DeviceContext::resolveMultiSampleImage(Image *src, Image *dst)
{
  CmdResolveMultiSampleImage cmd{src, dst};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::flushDraws()
{
  waitForItemPushSpace();

  CmdFlushDraws cmd{};
  VULKAN_LOCK_FRONT();
  beforeSubmitReplay();
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
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
  info.copyIndex = Frontend::replay->imageDownloadCopies.size();
  Frontend::replay->imageDownloads.push_back(info);
  Frontend::replay->imageDownloadCopies.insert(end(Frontend::replay->imageDownloadCopies), regions, regions + region_count);

  // no layout transform here, as we need to know which layout is the last one used for the flush
  if (sync)
    sync->request(Frontend::replay->id);
}

void DeviceContext::copyBufferToImage(Buffer *src, Image *dst, uint32_t region_count, VkBufferImageCopy *regions, bool)
{
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

void DeviceContext::copyBufferToImageOrdered(Buffer *src, Image *dst, uint32_t region_count, VkBufferImageCopy *regions)
{
  CmdCopyBufferToImageOrdered cmd{dst, src, regions[0]};
  VULKAN_LOCK_FRONT();
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);

  for (uint32_t i = 1; i < region_count; ++i)
  {
    cmd.region = regions[i];
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
}

void DeviceContext::blitImage(Image *src, Image *dst, const VkImageBlit &region, bool whole_subres)
{
  CmdBlitImage cmd{src, dst, region, whole_subres};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::updateBindlessResource(uint32_t index, D3dResource *res)
{
  auto resType = res->restype();
  if (RES3D_SBUF != resType)
  {
    auto tex = (BaseTex *)res;
    if (tex->isStub())
      tex = tex->getStubTex();
    Image *image = tex->getDeviceImage();
    const ImageViewState &viewState = tex->getViewInfo();

    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessTexUpdates.push_back({index, 1, image, viewState});
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessBufUpdates.push_back({index, 1, ((GenericBufferInterface *)res)->getBufferRef()});
  }
}

void DeviceContext::updateBindlessResourcesToNull(uint32_t resourceType, uint32_t index, uint32_t count)
{
  if (RES3D_SBUF != resourceType)
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessTexUpdates.push_back({index, count, nullptr, {}});
  }
  else
  {
    VULKAN_LOCK_FRONT();
    Frontend::replay->bindlessBufUpdates.push_back({index, 1, BufferRef{nullptr}});
  }
}

void DeviceContext::updateBindlessSampler(uint32_t index, SamplerState sampler)
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->bindlessSamplerUpdates.push_back({index, sampler});
}

void DeviceContext::copyBindlessDescriptors(uint32_t resource_type, uint32_t src, uint32_t dst, uint32_t count)
{
  VULKAN_LOCK_FRONT();
  CmdCopyBindlessDescriptors cmd{resource_type, src, dst, count};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::wait()
{
  TIME_PROFILE(vulkan_sync_wait);
  waitForItemPushSpace();

  ThreadedFence *fence;
  {
    // remove this lock when we are no longer depend on state on submits
    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    VULKAN_LOCK_FRONT();
    beforeSubmitReplay();

    fence = Globals::fences.alloc(vkDev, false);
    CmdFlushAndWait cmd{fence};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    submitReplay();
    afterBackendFlush();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
  }
  // CmdFlushAndWait will end frame and trigger needed signals
  // so we are free to wait on GPU fence only
  fence->wait(vkDev);
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

  // shutdown backend buffers
  CmdShutdownImmediateConstBuffers cmd;
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::shutdown()
{
  cleanupFrontendReplayResources();
  Frontend::tempBuffers.shutdown();
  Frontend::frameMemBuffers.shutdown();
  shutdownSwapchain();
  shutdownImmediateConstBuffers();

  // remove any resources that are not fully
  // deleted due to being destroyed in binded state
  CmdShutdownPendingReferences cmd{Frontend::State::pipe};
  VULKAN_DISPATCH_COMMAND(cmd);

  processAllPendingWork();
  shutdownWorkerThread();
  // no more commands can be executed after this point
  executionMode = ExecutionMode::INVALID;
  // consume semaphores that noone will wait for anymore
  Globals::VK::queue.consumeWaitSemaphores(Backend::gpuJob.get());

  Backend::bindless.shutdown(vkDev);
  Backend::pipelineCompiler.shutdown();
  Backend::aliasedMemory.shutdown();
  Backend::gpuJob.end();
  Frontend::replay.end();
}

void DeviceContext::beginSurvey(uint32_t name)
{
  VULKAN_LOCK_FRONT();
  CmdBeginSurvey cmd = Globals::surveys.start(name);
  Frontend::State::pipe.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({cmd.pool, cmd.index});
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::endSurvey(uint32_t name)
{
  VULKAN_LOCK_FRONT();
  Frontend::State::pipe.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({});
  VULKAN_DISPATCH_COMMAND_NO_LOCK(Globals::surveys.end(name));
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
  info.copyIndex = Frontend::replay->bufferDownloadCopies.size();
  Frontend::replay->bufferDownloads.push_back(info);
  Frontend::replay->bufferDownloadCopies.push_back(copy);
}

void DeviceContext::destroyBuffer(Buffer *buffer)
{
  G_ASSERT(buffer != nullptr);
  CmdDestroyBuffer cmd{buffer};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::destroyRenderPassResource(RenderPassResource *rp)
{
  G_ASSERT(rp != nullptr);
  CmdDestroyRenderPassResource cmd{rp};
  VULKAN_DISPATCH_COMMAND(cmd);
}

BufferRef DeviceContext::uploadToDeviceFrameMem(uint32_t size, const void *src)
{
  // use device resident memory in dedicated configuration to avoid minor device slowdown
  // otherwise use shared host/device memory to avoid issues (PowerVR) and unnecesary memory usage and operations
  return uploadToFrameMem(Globals::Mem::pool.getMemoryConfiguration() == DeviceMemoryConfiguration::DEDICATED_DEVICE_MEMORY
                            ? DeviceMemoryClass::DEVICE_RESIDENT_BUFFER
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

BufferRef DeviceContext::discardBuffer(const BufferRef &src_ref, DeviceMemoryClass memory_class, FormatStore view_format,
  uint32_t bufFlags, uint32_t dynamic_size)
{
  BufferRef ret;

  if (bufFlags & SBCF_FRAMEMEM)
  {
    G_ASSERTF(dynamic_size, "vulkan: framemem discard size must be > 0");
    ret = Frontend::frameMemBuffers.acquire(dynamic_size, bufFlags);
    if (ret.buffer != src_ref.buffer)
      Frontend::frameMemBuffers.swapRefCounters(src_ref.buffer, ret.buffer);
    if (src_ref)
    {
      CmdNotifyBufferDiscard cmd{src_ref, ret, bufFlags};
      VULKAN_DISPATCH_COMMAND(cmd);
    }
  }
  else
  {
    Buffer *buf = src_ref.buffer;
    uint32_t reallocated_size = buf->getBlockSize();
    G_ASSERTF(dynamic_size <= reallocated_size, "vulkan: discard size (%u) more than max buffer size (%u) requested for buffer %p:%s",
      dynamic_size, reallocated_size, buf, buf->getDebugName());

    if (dynamic_size > 0)
    {
      dynamic_size = Globals::VK::bufAlign.alignSize(dynamic_size);
      G_ASSERTF(!buf->hasView() || (dynamic_size == reallocated_size),
        "vulkan: variable sized discard for sampled buffers is not allowed asked %u bytes while buffer %p:%s size is %u", dynamic_size,
        buf, buf->getDebugName(), reallocated_size);
      reallocated_size = dynamic_size;
    }

    // lock ahead of time to avoid errors on frontFrameIndex increments from other thread
    // this is valid pattern for background thread data uploads,
    // but we must not corrupt memory if same happens for other reasons
    VULKAN_LOCK_FRONT();
    uint32_t frontFrameIndex = Frontend::State::pod.frameIndex;
    if (!buf->onDiscard(frontFrameIndex, reallocated_size))
    {
      uint32_t discardCount = buf->getDiscardBlocks();
      if (discardCount > 1)
        discardCount += Buffer::pending_discard_frames;
      else
        discardCount = Buffer::pending_discard_frames;

      buf = Buffer::create(src_ref.buffer->getBlockSize(), memory_class, discardCount, src_ref.buffer->getDescription().memFlags);
      // must mark range used as we will give this range to user without calling onDiscard
      buf->markDiscardUsageRange(frontFrameIndex, reallocated_size);
      if (src_ref.buffer->hasView())
        buf->addBufferView(view_format);

      Globals::Dbg::naming.setBufName(buf, src_ref.buffer->getDebugName());
    }
    ret = BufferRef{buf};
    if (src_ref)
    {
      CmdNotifyBufferDiscard cmd{src_ref, ret, bufFlags};
      VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    }
  }

  G_ASSERTF(ret.buffer, "vulkan: discard failed for %p:%s", src_ref.buffer, src_ref.buffer->getDebugName());
  return ret;
}

void DeviceContext::destroyImageDelayed(Image *img)
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->cleanups.enqueue<Image::CLEANUP_DELAYED_DESTROY>(*img);
}

void DeviceContext::destroyImage(Image *img)
{
  CmdDestroyImage cmd{img};
  VULKAN_DISPATCH_COMMAND(cmd);
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

void DeviceContext::present()
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

  {
    VULKAN_LOCK_FRONT();
    beforeSubmitReplay();

    advanceAndCheckTimingRecord();
    CmdPresent cmd{};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    submitReplay();
    Frontend::State::pod.frameIndex++;
    Frontend::State::pod.summaryAsyncPipelineCompilationFeedback = 0;
    afterBackendFlush();

    if (Globals::cfg.bits.allowPredictedLatencyWaitApp)
      Frontend::latencyWaiter.update(Frontend::timings.frontendBackendWaitDuration);
  }

  if (Globals::cfg.bits.allowPredictedLatencyWaitApp)
  {
    if (Globals::cfg.bits.autoPredictedLatencyWaitApp)
      Frontend::latencyWaiter.wait();
    else
      Frontend::latencyWaiter.markAsyncWait();
  }
}

void DeviceContext::changeSwapchainMode(const SwapchainMode &new_mode)
{
  waitForItemPushSpace();

  // special case wait,
  // as we should do some operations after gpu wait and wait for that operations too
  ThreadedFence *fence;
  {
    VULKAN_LOCK_FRONT();
    beforeSubmitReplay();

    fence = Globals::fences.alloc(vkDev, false);
    CmdChangeSwapchainMode cmd{new_mode, fence};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    submitReplay();
    afterBackendFlush();
  }

  fence->wait(vkDev);

  {
    VULKAN_LOCK_FRONT();
    Globals::fences.free(fence);
  }
}

void DeviceContext::shutdownSwapchain()
{
  CmdShutdownSwapchain cmd{};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::writeDebugMessage(const char *msg, intptr_t msg_length, intptr_t severity)
{
  if (msg_length <= 0)
    msg_length = strlen(msg);

  CmdWriteDebugMessage cmd;
  cmd.message_length = msg_length;
  cmd.severity = severity;
  VULKAN_LOCK_FRONT();
  cmd.message_index = StringIndexRef{Frontend::replay->charStore.size()};
  Frontend::replay->charStore.insert(Frontend::replay->charStore.end(), msg, msg + msg_length);
  Frontend::replay->charStore.push_back(0);
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::executeFSR(amd::FSR *fsr, const amd::FSR::UpscalingArgs &params)
{
  auto cast_to_image = [](BaseTexture *src) {
    if (src)
      return cast_to_texture_base(src)->getDeviceImage();
    return (Image *)nullptr;
  };

  CmdExecuteFSR cmd{fsr, params};
  cmd.fsr = fsr;
  cmd.params.colorTexture = cast_to_image(params.colorTexture);
  cmd.params.depthTexture = cast_to_image(params.depthTexture);
  cmd.params.motionVectors = cast_to_image(params.motionVectors);
  cmd.params.exposureTexture = cast_to_image(params.exposureTexture);
  cmd.params.outputTexture = cast_to_image(params.outputTexture);
  cmd.params.reactiveTexture = cast_to_image(params.reactiveTexture);
  cmd.params.transparencyAndCompositionTexture = cast_to_image(params.transparencyAndCompositionTexture);

  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::addPipelineCache(VulkanPipelineCacheHandle cache)
{
  CmdAddPipelineCache cmd{cache};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::addRenderState(shaders::DriverRenderStateId id, const shaders::RenderState &render_state_data)
{
  CmdAddRenderState cmd{id, render_state_data};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::generateMipmaps(Image *img)
{
  CmdGenerateMipmaps cmd{img};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::startPreRotate(uint32_t binding_slot)
{
  CmdStartPreRotate cmd{binding_slot};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::holdPreRotateStateForOneFrame()
{
  CmdHoldPreRotate cmd;
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::flushBufferToHost(const BufferRef &buffer, ValueRange<uint32_t> range)
{
  BufferFlushInfo info;
  info.buffer = buffer.buffer;
  info.offset = buffer.bufOffset(range.front());
  info.range = range.size();
  VULKAN_LOCK_FRONT();
  Frontend::replay->bufferToHostFlushes.push_back(info);
}

void DeviceContext::waitForIfPending(AsyncCompletionState &sync)
{
  bool needWait = !sync.isCompleted();
  if (needWait)
  {
    wait();
    G_ASSERTF(sync.isCompleted(), "vulkan: AsyncCompletionState %p not completed after wait");
  }
}

void drv3d_vulkan::DeviceContext::resourceBarrier(ResourceBarrierDesc desc, GpuPipeline /* gpu_pipeline*/)
{
  VULKAN_LOCK_FRONT();

  desc.enumerateTextureBarriers([this](BaseTexture *tex, ResourceBarrier state, unsigned res_index, unsigned res_range) {
    G_ASSERT(tex);

    Image *image = cast_to_texture_base(tex)->tex.image;
    uint32_t stop_index = (res_range == 0) ? image->getMipLevels() * image->getArrayLayers() : res_range + res_index;
    G_UNUSED(stop_index);
    G_ASSERTF((image->getMipLevels() * image->getArrayLayers()) >= stop_index,
      "Out of bound subresource index range (%d, %d+%d) in resource barrier for image %s (0, %d)", res_index, res_index, res_range,
      image->getDebugName(), image->getMipLevels() * image->getArrayLayers());

    CmdImageBarrier cmd{image, state, res_index, res_range};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  });

  desc.enumerateBufferBarriers([this](Sbuffer *buf, ResourceBarrier state) {
    // ignore global UAV write flushes
    // they will not work properly in current sync logic and should be already tracked
    // only REAL acceses can be processed properly/without risk, not dummy/assumed global ones
    // reasons:
    //  1. drivers are buggy on global memory barriers (even validator can simply ignore them)
    //  2. execution on GPU can be async task based, not linear FIFO, giving different results on "wide" barriers
    if (!buf && (state & RB_FLUSH_UAV))
      return;
    // RB_NONE is "hack" to skip next sync
    // but it is non efficient as we must track src op for proper sync on vulkan
    // and also sync step must be reordered/delayed, as other operations must be RB_NONE-d too
    // yet if it is done, no RB_NONE is NOT needed at all
    // because if operations can be batch completed, reordered/delayed sync step will verify it and
    // make proper batch-fashion barriers
    if (state == RB_NONE)
      return;
    G_ASSERT(buf);
    auto gbuf = (GenericBufferInterface *)buf;
    CmdBufferBarrier cmd{gbuf->getBufferRef(), state}; //-V522
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  });
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

#if D3D_HAS_RAY_TRACING

void DeviceContext::deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc)
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->cleanups.enqueue<RaytraceAccelerationStructure::CLEANUP_DESTROY_BOTTOM>(*((RaytraceAccelerationStructure *)desc));
}
void DeviceContext::deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc)
{
  VULKAN_LOCK_FRONT();
  Frontend::replay->cleanups.enqueue<RaytraceAccelerationStructure::CLEANUP_DESTROY_TOP>(*((RaytraceAccelerationStructure *)desc));
}
void DeviceContext::traceRays(Buffer *ray_gen_table, uint32_t ray_gen_offset, Buffer *miss_table, uint32_t miss_offset,
  uint32_t miss_stride, Buffer *hit_table, uint32_t hit_offset, uint32_t hit_stride, Buffer *callable_table, uint32_t callable_offset,
  uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth)
{
  CmdTraceRays cmd;
  cmd.rayGenTable = BufferRef{ray_gen_table};
  cmd.missTable = BufferRef{miss_table};
  cmd.hitTable = BufferRef{hit_table};
  cmd.callableTable = BufferRef{callable_table};
  cmd.rayGenOffset = ray_gen_offset;
  cmd.missOffset = miss_offset;
  cmd.missStride = miss_stride;
  cmd.hitOffset = hit_offset;
  cmd.hitStride = hit_stride;
  cmd.callableOffset = callable_offset;
  cmd.callableStride = callable_stride;
  cmd.width = width;
  cmd.height = height;
  cmd.depth = depth;
  VULKAN_DISPATCH_COMMAND(cmd);
  executeDebugFlush("traceRays");
}

#endif

void DeviceContext::addShaderModule(uint32_t id, eastl::unique_ptr<ShaderModuleBlob> shdr_module)
{
  CmdAddShaderModule cmd{shdr_module.release(), id};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::removeShaderModule(uint32_t blob_id)
{
  CmdRemoveShaderModule cmd{blob_id};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::addGraphicsProgram(ProgramID program, const ShaderModuleUse &vs_blob, const ShaderModuleUse &fs_blob,
  const ShaderModuleUse *gs_blob, const ShaderModuleUse *tc_blob, const ShaderModuleUse *te_blob)
{
  CmdAddGraphicsProgram cmd{program};
  VULKAN_LOCK_FRONT();
  cmd.vs = Frontend::replay->shaderModuleUses.size();
  Frontend::replay->shaderModuleUses.push_back(vs_blob);

  cmd.fs = Frontend::replay->shaderModuleUses.size();
  Frontend::replay->shaderModuleUses.push_back(fs_blob);

  if (gs_blob)
  {
    cmd.gs = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*gs_blob);
  }
  else
    cmd.gs = ~uint32_t(0);

  if (tc_blob)
  {
    cmd.tc = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*tc_blob);
  }
  else
    cmd.tc = ~uint32_t(0);

  if (te_blob)
  {
    cmd.te = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*te_blob);
  }
  else
    cmd.te = ~uint32_t(0);

  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::addComputeProgram(ProgramID program, eastl::unique_ptr<ShaderModuleBlob> shdr_module,
  const ShaderModuleHeader &header)
{
  CmdAddComputeProgram cmd{program, shdr_module.release(), header};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::removeProgram(ProgramID program)
{
  CmdRemoveProgram cmd{program};
  VULKAN_DISPATCH_COMMAND(cmd);
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
void DeviceContext::attachComputeProgramDebugInfo(ProgramID program, eastl::unique_ptr<ShaderDebugInfo> dbg)
{
  CmdAttachComputeProgramDebugInfo cmd{program, dbg.release()};
  VULKAN_DISPATCH_COMMAND(cmd);
}
#endif

void DeviceContext::placeAftermathMarker(const char *name)
{
  if (!Globals::cfg.debugLevel)
    return;

  CmdPlaceAftermathMarker cmd;
  cmd.stringLength = strlen(name) + 1;
  VULKAN_LOCK_FRONT();
  cmd.stringIndex = StringIndexRef{Frontend::replay->charStore.size()};
  Frontend::replay->charStore.insert(Frontend::replay->charStore.end(), name, name + cmd.stringLength);
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::getWorkerCpuCore(int *core, int *thread_id)
{
  CmdGetWorkerCpuCore cmd = {core, thread_id};
  VULKAN_DISPATCH_COMMAND(cmd);
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
  frameTiming.frontendWaitForSwapchainDuration = Frontend::latencyWaiter.getLastWaitTimeUs();

  CmdRecordFrameTimings cmd{&frameTiming, now};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
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
  {
    CmdRegisterFrameEventsCallback cmd{callback};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
}

void DeviceContext::callFrameEndCallbacks()
{
  for (FrameEvents *callback : frameEventCallbacks)
    callback->endFrameEvent();
  frameEventCallbacks.clear();
}

DeviceContext::DeviceContext() : vkDev(Globals::VK::dev)
{
  Backend::State::exec.reset();
  Backend::State::pipe.reset();
}
