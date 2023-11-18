#include "device.h"
#include "swapchain.h"
#include <osApiWrappers/dag_files.h>
#include <EASTL/sort.h>
#include <EASTL/functional.h>
#include <perfMon/dag_statDrv.h>
#include "buffer.h"
#include "texture.h"

using namespace drv3d_vulkan;

void DeviceContext::waitForItemPushSpace()
{
  DA_PROFILE_WAIT(DA_PROFILE_FUNC);
  while (!device.timelineMan.get<TimelineManager::CpuReplay>().waitAdvance(MAX_PENDING_WORK_ITEMS, MAX_REPLAY_WAIT_CYCLES))
    logerr("vulkan: replay takes too long");
}

int DeviceContext::getFramerateLimitingFactor()
{
#if !VULKAN_RECORD_TIMING_DATA
  return DRV3D_FRAMERATE_LIMITED_BY_NOTHING;
#else
  if (executionMode != ExecutionMode::THREADED)
    return DRV3D_FRAMERATE_LIMITED_BY_NOTHING;

  auto &completedTimings = front.timingHistory[front.completedFrameIndex % FRAME_TIMING_HISTORY_LENGTH];
  bool replayThreadWait = completedTimings.frontendBackendWaitDuration > 0;
  bool replayUnderfeed = completedTimings.backendFrontendWaitDuration > 0;
  bool gpuOverutilized = (completedTimings.gpuWaitDuration > 0) ||
                         (profile_usec_from_ticks_delta(completedTimings.presentDuration) > LONG_PRESENT_DURATION_THRESHOLD_US);

  return (replayThreadWait ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_WAIT : 0) |
         (replayUnderfeed ? DRV3D_FRAMERATE_LIMITED_BY_REPLAY_UNDERFEED : 0) |
         (gpuOverutilized ? DRV3D_FRAMERATE_LIMITED_BY_GPU_UTILIZATION : 0);
#endif
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
  if (device.getFeatures().test(DeviceFeaturesConfig::FLUSH_AFTER_EACH_DRAW_AND_DISPATCH) && !device.surveys.anyScopesActive() &&
      !front.pipelineState.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>())
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
      front.pipelineState.dumpLog();
    }
  }
#endif
}

void DeviceContext::afterBackendFlush() { on_front_flush(); }

void DeviceContext::submitReplay()
{
  device.allocated_upload_buffer_size = 0;
  cleanupFrontendReplayResources();

  // Any form of command buffer flushing is not supported while mid occlusion query
  // If it does happen, it probably is an application logic error
  // (e.g. someone is trying to wait for a fence mid survey)
  G_ASSERTF(!device.surveys.anyScopesActive(), "vulkan: trying to flush mid survey");
  auto &replay = device.timelineMan.get<TimelineManager::CpuReplay>();

  {
    TIME_PROFILE(vulkan_submit_acquire_wait);
    while (!replay.waitAcquireSpace(MAX_REPLAY_WAIT_CYCLES))
      logerr("vulkan: replay takes too long in full blocking conditions");
  }

  front.replayRecord.restart();
  if (ExecutionMode::THREADED != executionMode)
    replay.advance();
}

void DeviceContext::reportAlliveObjects(FaultReportDump &dump)
{
  uint32_t cnt = 0;

  auto iterCb = [&cnt, &dump](const Resource *i) {
    FaultReportDump::RefId rid =
      dump.addTagged(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i, String(64, "%s %s", i->getDebugName(), i->resTypeString()));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_VK_HANDLE, i->getBaseHandle());

    rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_VK_HANDLE, i->getBaseHandle(), String(64, "handle of %p", i));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i);
    ++cnt;
  };

  SharedGuardedObjectAutoLock resLock(device.resources);
  device.resources.iterateAllocated<Buffer>(iterCb);
  device.resources.iterateAllocated<Image>(iterCb);
  device.resources.iterateAllocated<RenderPassResource>(iterCb);
#if D3D_HAS_RAY_TRACING
  device.resources.iterateAllocated<RaytraceAccelerationStructure>(iterCb);
#endif
  dump.add(String(32, "%u objects are alive", cnt));
}

void DeviceContext::cleanupFrontendReplayResources()
{
  for (auto &i : front.tempBufferManagers)
    i.onFrameEnd([=](auto buffer) { front.replayRecord->cleanups.enqueue(*buffer); });
  front.completionStateRefs.clear();
}

OSSpinlock &DeviceContext::getFrontGuard() { return mutex; }

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

void DeviceContext::setConstRegisterBuffer(VulkanBufferHandle buffer, uint32_t offset, uint32_t size, ShaderStage stage)
{
  auto &resBinds = front.pipelineState.getStageResourceBinds(stage);
  if (resBinds.set<StateFieldGlobalConstBuffer, StateFieldGlobalConstBuffer::BufferRangedRef>({buffer, offset, size}))
    front.pipelineState.markResourceBindDirty(stage);
}

void DeviceContext::setPipelineState()
{
  // no need to check dirty as we anyway check it inside transit
  // no need to clear dirty as we do it internaly (aka consume)
  front.pipelineState.transit(*this);
}

void DeviceContext::compileGraphicsPipeline(const VkPrimitiveTopology top)
{
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
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdCompileComputePipeline cmd{};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("compileComputePipeline");
}

void DeviceContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDispatch cmd{x, y, z};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("dispatch");
}

void DeviceContext::dispatchIndirect(Buffer *buffer, uint32_t offset)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDispatchIndirect cmd{BufferRef{buffer}, offset};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("dispatchIndirect");
}

void DeviceContext::drawIndirect(VkPrimitiveTopology top, uint32_t count, Buffer *buffer, uint32_t offset, uint32_t stride)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDrawIndirect cmd{top, count, BufferRef{buffer}, offset, stride};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("drawIndirect");
}

void DeviceContext::drawIndexedIndirect(VkPrimitiveTopology top, uint32_t count, Buffer *buffer, uint32_t offset, uint32_t stride)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDrawIndexedIndirect cmd{top, count, BufferRef{buffer}, offset, stride};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("drawIndexedIndirect");
}

void DeviceContext::draw(VkPrimitiveTopology top, uint32_t start, uint32_t count, uint32_t first_instance, uint32_t instance_count)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDraw cmd{top, start, count, first_instance, instance_count};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("draw");
}

void DeviceContext::drawUserData(VkPrimitiveTopology top, uint32_t count, uint32_t stride, BufferSubAllocation user_data)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDrawUserData cmd{top, count, stride, user_data};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("drawUserData");
}

void DeviceContext::drawIndexed(VkPrimitiveTopology top, uint32_t index_start, uint32_t count, int32_t vertex_base,
  uint32_t first_instance, uint32_t instance_count)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDrawIndexed cmd{top, index_start, count, vertex_base, first_instance, instance_count};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("draw");
}

void DeviceContext::drawIndexedUserData(VkPrimitiveTopology top, uint32_t count, uint32_t vertex_stride,
  BufferSubAllocation vertex_data, BufferSubAllocation index_data)
{
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();
    CmdDrawIndexedUserData cmd{top, count, vertex_stride, vertex_data, index_data};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  executeDebugFlush("drawIndexedUserData");
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

void DeviceContext::copyBuffer(Buffer *source, Buffer *dest, uint32_t src_offset, uint32_t dst_offset, uint32_t data_size)
{
  CmdCopyBuffer cmd{BufferRef{source}, BufferRef{dest}, src_offset, dst_offset, data_size};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::clearBufferFloat(Buffer *buffer, const float values[4])
{
  CmdClearBufferFloat cmd{BufferRef{buffer}};
  memcpy(cmd.values, values, sizeof(cmd.values));
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::clearBufferInt(Buffer *buffer, const unsigned values[4])
{
  CmdClearBufferInt cmd{BufferRef{buffer}};
  memcpy(cmd.values, values, sizeof(cmd.values));
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::pushEvent(const char *name)
{
  CmdPushEvent cmd;
  VULKAN_LOCK_FRONT();
  cmd.index = StringIndexRef{front.replayRecord->charStore.size()};
  front.replayRecord->charStore.insert(front.replayRecord->charStore.end(), name, name + strlen(name) + 1);
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::popEvent()
{
  CmdPopEvent cmd;
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value)
{
  CmdClearDepthStencilTexture cmd{image, area, value};

  if (is_global_lock_acquired())
  {
    VULKAN_DISPATCH_COMMAND(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    front.replayRecord->unorderedImageDepthStencilClears.push_back(cmd);
  }
}

void DeviceContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value)
{
  CmdClearColorTexture cmd{image, area, value};

  if (is_global_lock_acquired())
  {
    VULKAN_DISPATCH_COMMAND(cmd);
  }
  else
  {
    VULKAN_LOCK_FRONT();
    front.replayRecord->unorderedImageColorClears.push_back(cmd);
  }
}

void DeviceContext::fillEmptySubresources(Image *image)
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->imagesToFillEmptySubresources.push_back(image);
}

void DeviceContext::copyImage(Image *src, Image *dst, const VkImageCopy *regions, uint32_t region_count, uint32_t src_mip,
  uint32_t dst_mip, uint32_t mip_count)
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
    cmd.regionIndex = front.replayRecord->imageCopyInfos.size();
    front.replayRecord->imageCopyInfos.insert(front.replayRecord->imageCopyInfos.end(), regions, regions + region_count);

    // allow image copy reorder if we do them outside of render thread
    if (is_global_lock_acquired())
    {
      VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    }
    else
      front.replayRecord->unorderedImageCopies.push_back(cmd);
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
  setPipelineState();
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  submitReplay();
  afterBackendFlush();
}

// special versions of copyBufferToImage/copyImageToBuffer that work without lock
void DeviceContext::copyImageToHostCopyBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions)
{
  ImageCopyInfo info;
  info.image = image;
  info.buffer = buffer;
  info.copyCount = region_count;
  info.copyIndex = front.replayRecord->imageDownloadCopies.size();
  front.replayRecord->imageDownloads.push_back(info);
  front.replayRecord->imageDownloadCopies.insert(end(front.replayRecord->imageDownloadCopies), regions, regions + region_count);
}

void DeviceContext::copyHostCopyToImage(Buffer *src, Image *dst, uint32_t region_count, VkBufferImageCopy *regions)
{
  ImageCopyInfo info;
  info.image = dst;
  info.buffer = src;
  info.copyCount = region_count;
  info.copyIndex = front.replayRecord->imageUploadCopies.size();
  front.replayRecord->imageUploads.push_back(info);
  front.replayRecord->imageUploadCopies.insert(end(front.replayRecord->imageUploadCopies), regions, regions + region_count);
  front.replayRecord->cleanups.enqueue(*src);
}

void DeviceContext::copyImageToBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions,
  AsyncCompletionState *sync)
{
  ImageCopyInfo info;
  info.image = image;
  info.buffer = buffer;
  info.copyCount = region_count;
  VULKAN_LOCK_FRONT();
  info.copyIndex = front.replayRecord->imageDownloadCopies.size();
  front.replayRecord->imageDownloads.push_back(info);
  front.replayRecord->imageDownloadCopies.insert(end(front.replayRecord->imageDownloadCopies), regions, regions + region_count);

  // no layout transform here, as we need to know which layout is the last one used for the flush
  if (sync)
  {
    sync->pending();
    front.completionStateRefs.push_back(sync);
    CmdAddSyncEvent cmd{sync};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
}

void DeviceContext::copyBufferToImage(Buffer *src, Image *dst, uint32_t region_count, VkBufferImageCopy *regions, bool)
{
  {
    ImageCopyInfo info;
    info.image = dst;
    info.buffer = src;
    info.copyCount = region_count;
    VULKAN_LOCK_FRONT();
    info.copyIndex = front.replayRecord->imageUploadCopies.size();
    front.replayRecord->imageUploads.push_back(info);
    front.replayRecord->imageUploadCopies.insert(end(front.replayRecord->imageUploadCopies), regions, regions + region_count);
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

void DeviceContext::blitImage(Image *src, Image *dst, const VkImageBlit &region)
{
  CmdBlitImage cmd{src, dst, region};
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
    tex->setUsed();
    Image *image = tex->getDeviceImage(true);
    const ImageViewState &viewState = tex->getViewInfo();

    VULKAN_LOCK_FRONT();
    CmdUpdateBindlessTexture cmd{index, image, viewState};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
  else
  {
    G_ASSERTF(false, "vulkan: trying to update resource %s while RES3D_SBUF resources are not supported", res->getResName());
  }
}

void DeviceContext::updateBindlessResourcesToNull(uint32_t resourceType, uint32_t index, uint32_t count)
{
  if (RES3D_SBUF != resourceType)
  {
    VULKAN_LOCK_FRONT();

    for (uint32_t i = index; i < index + count; ++i)
    {
      CmdUpdateBindlessTexture cmd{i, nullptr, {}};
      VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    }
  }
  else
  {
    G_ASSERTF(false, "vulkan: trying to update buffer resources to NULL while RES3D_SBUF resources are not supported");
  }
}

void DeviceContext::updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo)
{
  VULKAN_LOCK_FRONT();
  CmdUpdateBindlessSampler cmd{index, samplerInfo};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::copyBindlessTextureDescriptors(uint32_t src, uint32_t dst, uint32_t count)
{
  VULKAN_LOCK_FRONT();
  CmdCopyBindlessTextureDescriptors cmd{src, dst, count};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::wait()
{
  TIME_PROFILE(vulkan_sync_wait);

  ScopedGPUPowerState gpuPowerState(device.getFeatures().test(DeviceFeaturesConfig::FORCE_GPU_HIGH_POWER_STATE_ON_WAIT));

  waitForItemPushSpace();

  ThreadedFence *fence;
  {
    // remove this lock when we are no longer depend on state on submits
    d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, nullptr, nullptr, nullptr);
    VULKAN_LOCK_FRONT();
    setPipelineState();

    fence = device.fenceManager.alloc(vkDev, false);
    CmdFlushAndWait cmd{fence};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    submitReplay();
    afterBackendFlush();
    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, nullptr, nullptr, nullptr);
  }
  // CmdFlushAndWait will end frame and trigger needed signals
  // so we are free to wait on GPU fence only
  fence->wait(vkDev);
  {
    VULKAN_LOCK_FRONT();
    device.fenceManager.free(fence);
  }
}

void DeviceContext::processAllPendingWork()
{
  // process pending work and cleanup on hold resources
  // this way replay queue and gpu queue will be fully processed
  for (int i = 0; i < FRAME_FRAME_BACKLOG_LENGTH + MAX_RETIREMENT_QUEUE_ITEMS; ++i)
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
  // leaked temp buffers will be already reported per frame basis
  // no need to verify that pool is empty
  front.tempBufferHoldersPool.freeAll();
  shutdownSwapchain();
  shutdownImmediateConstBuffers();

  // remove any resources that are not fully
  // deleted due to being destroyed in binded state
  CmdShutdownPendingReferences cmd{front.pipelineState};
  VULKAN_DISPATCH_COMMAND(cmd);

  processAllPendingWork();
  shutdownWorkerThread();
  // no more commands can be executed after this point
  executionMode = ExecutionMode::INVALID;

  back.contextState.bindlessManagerBackend.shutdown(vkDev);
  back.pipelineCompiler.shutdown();
  back.contextState.frame.end();
  front.replayRecord.end();
}

void DeviceContext::beginSurvey(uint32_t name)
{
  VULKAN_LOCK_FRONT();
  CmdBeginSurvey cmd = device.surveys.start(name);
  front.pipelineState.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({cmd.pool, cmd.index});
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::endSurvey(uint32_t name)
{
  VULKAN_LOCK_FRONT();
  front.pipelineState.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({});
  VULKAN_DISPATCH_COMMAND_NO_LOCK(device.surveys.end(name));
}

void DeviceContext::uploadBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src->dataOffset(src_offset);
  copy.dstOffset = dst->dataOffset(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src;
  info.dst = dst;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  info.copyIndex = front.replayRecord->bufferUploadCopies.size();
  front.replayRecord->bufferUploads.push_back(info);
  front.replayRecord->bufferUploadCopies.push_back(copy);
}

void DeviceContext::uploadBufferOrdered(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src->dataOffset(src_offset);
  copy.dstOffset = dst->dataOffset(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src;
  info.dst = dst;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  info.copyIndex = front.replayRecord->orderedBufferUploadCopies.size();
  front.replayRecord->orderedBufferUploads.push_back(info);
  front.replayRecord->orderedBufferUploadCopies.push_back(copy);
}

void DeviceContext::downloadBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  VkBufferCopy copy;
  copy.srcOffset = src->dataOffset(src_offset);
  copy.dstOffset = dst->dataOffset(dst_offset);
  copy.size = size;
  BufferCopyInfo info;
  info.src = src;
  info.dst = dst;
  info.copyCount = 1;
  VULKAN_LOCK_FRONT();
  info.copyIndex = front.replayRecord->bufferDownloadCopies.size();
  front.replayRecord->bufferDownloads.push_back(info);
  front.replayRecord->bufferDownloadCopies.push_back(copy);
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

Buffer *DeviceContext::discardBuffer(Buffer *to_discared, DeviceMemoryClass memory_class, FormatStore view_format, uint32_t bufFlags)
{
  Buffer *ret = to_discared;
  if (!to_discared->onDiscard(front.frameIndex))
  {
    uint32_t discardCount = to_discared->getMaxDiscardLimit() + FRAME_FRAME_BACKLOG_LENGTH + MAX_PENDING_WORK_ITEMS;
    ret = device.createBuffer(to_discared->dataSize(), memory_class, discardCount, BufferMemoryFlags::NONE);

    if (to_discared->hasView())
      device.addBufferView(ret, view_format);

    device.setBufName(ret, to_discared->getDebugName());
  }

  // FIXME: ensure discard is not called for active buffer from another thread
  CmdNotifyBufferDiscard cmd{to_discared, BufferRef(ret), bufFlags};
  VULKAN_DISPATCH_COMMAND(cmd);

  return ret;
}

void DeviceContext::destroyImageDelayed(Image *img)
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->cleanups.enqueue<Image::CLEANUP_DELAYED_DESTROY>(*img);
}

void DeviceContext::destroyImage(Image *img)
{
  CmdDestroyImage cmd{img};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::evictImage(Image *img) { front.replayRecord->cleanups.enqueue<Image::CLEANUP_EVICT>(*img); }

void DeviceContext::setPipelineCompilationTimeBudget(unsigned usecs)
{
  CmdPipelineCompilationTimeBudget cmd{usecs};
  VULKAN_DISPATCH_COMMAND(cmd);
}

uint32_t DeviceContext::getPiplineCompilationQueueLength()
{
  VULKAN_LOCK_FRONT();
  return back.pipelineCompiler.getQueueLength();
}

size_t DeviceContext::getCurrentWorkItemId()
{
  VULKAN_LOCK_FRONT(); // we probably should avoid this
  return front.replayRecord->id;
}

void DeviceContext::present()
{
  {
    FrameTimingWatch watch(front.frontendBackendWaitDuration);
    waitForItemPushSpace();
  }

  uint32_t allocations = 0;
  {
    // if we allocated something on this frame, make a message to texql
    // to properly fit in memory restrictions
    GuardedObjectAutoLock lock(device.memory);
    allocations = device.memory->getAllocationCounter();
  }

  // should be called without memory guard locked
  {
    if (allocations > front.lastVisibleAllocations)
    {
      front.lastVisibleAllocations = allocations;
      TEXQL_PRE_CLEAN(4);
    }
  }

  callFrameEndCallbacks();

  VULKAN_LOCK_FRONT();
  setPipelineState();

  advanceAndCheckTimingRecord();
  CmdCleanupPendingReferences cleanupCmd{front.pipelineState};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cleanupCmd);
  CmdPresent cmd{};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  submitReplay();
  afterBackendFlush();
  front.frameIndex++;
}

void DeviceContext::changeSwapchainMode(const SwapchainMode &new_mode)
{
  waitForItemPushSpace();

  // special case wait,
  // as we should do some operations after gpu wait and wait for that operations too
  ThreadedFence *fence;
  {
    VULKAN_LOCK_FRONT();
    setPipelineState();

    fence = device.fenceManager.alloc(vkDev, false);
    CmdChangeSwapchainMode cmd{new_mode, fence};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
    submitReplay();
    afterBackendFlush();
  }

  fence->wait(vkDev);

  {
    VULKAN_LOCK_FRONT();
    device.fenceManager.free(fence);
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
  cmd.message_index = StringIndexRef{front.replayRecord->charStore.size()};
  front.replayRecord->charStore.insert(front.replayRecord->charStore.end(), msg, msg + msg_length);
  front.replayRecord->charStore.push_back(0);
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
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

void DeviceContext::insertTimestampQuery(TimestampQuery *query)
{
  CmdInsertTimesampQuery cmd = device.timestamps.issue(query);
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

void DeviceContext::addSyncEvent(AsyncCompletionState &sync)
{
  VULKAN_LOCK_FRONT();

  // we are not guarded from this and it can break ordering
  // overall idea to add same sync event from 2 threads is looking bogus
  // so just log call stack here and fix at caller site
  if (sync.isPending())
    logerr("vulkan: race on addSyncEvent");

  sync.pending();
  front.completionStateRefs.push_back(&sync);
  CmdAddSyncEvent cmd{&sync};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::flushBufferToHost(Buffer *buffer, ValueRange<uint32_t> range)
{
  BufferFlushInfo info;
  info.buffer = buffer;
  info.offset = buffer->dataOffset(range.front());
  info.range = range.size();
  VULKAN_LOCK_FRONT();
  front.replayRecord->bufferToHostFlushes.push_back(info);
}

void DeviceContext::waitFor(AsyncCompletionState &sync)
{
  bool hasToWait;
  {
    VULKAN_LOCK_FRONT();
    // check if this is a sync event of this frame or not
    hasToWait = end(front.completionStateRefs) != eastl::find(begin(front.completionStateRefs), end(front.completionStateRefs), &sync);
  }

  if (hasToWait)
  {
    // this frame, needs extra fence and wait for it
    wait();
  }
  else
  {
    // event was submitted on earlier frames, need to tell backend
    // to check past frames now and update state.
    CmdCheckAndSetAsyncCompletionState cmd{&sync};
    VULKAN_DISPATCH_COMMAND(cmd);
    flushDraws();
    int64_t timeoutRef = rel_ref_time_ticks(ref_time_ticks(), GENERAL_GPU_TIMEOUT_US);
    while (!sync.isCompleted())
    {
      sleep_msec(0);
      if (timeoutRef < ref_time_ticks())
      {
        logerr("vulkan: too long AsyncCompletionState partial wait, trying full wait");
        wait();
        break; // if it still not completed, drop to final exit check
      }
    }
  }

  // object must be completed after waits, otherwise we have a bug somewhere
  if (!sync.isCompleted())
  {
    // we can't really continue, caller hardly depend on this sync
    fatal("vulkan: AsyncCompletionState wait failed");
  }
}

TempBufferHolder drv3d_vulkan::DeviceContext::copyToTempBuffer(int type, uint32_t size, const void *src)
{
  G_ASSERT(type < TempBufferManager::TYPE_COUNT);

  return TempBufferHolder(device, front.tempBufferManagers[type], size, src);
}

TempBufferHolder *drv3d_vulkan::DeviceContext::allocTempBuffer(int type, uint32_t size)
{
  G_ASSERT(type < TempBufferManager::TYPE_COUNT);

  WinAutoLock poolLock(front.tempBufferHoldersPoolGuard);
  return front.tempBufferHoldersPool.allocate(device, front.tempBufferManagers[type], size);
}

void drv3d_vulkan::DeviceContext::freeTempBuffer(TempBufferHolder *temp_buff_holder)
{
  WinAutoLock poolLock(front.tempBufferHoldersPoolGuard);
  front.tempBufferHoldersPool.free(temp_buff_holder);
}

void drv3d_vulkan::DeviceContext::resourceBarrier(ResourceBarrierDesc desc, GpuPipeline /* gpu_pipeline*/)
{
  VULKAN_LOCK_FRONT();

  desc.enumerateTextureBarriers([this](BaseTexture *tex, ResourceBarrier state, unsigned res_index, unsigned res_range) {
    G_ASSERT(tex);

    Image *image = nullptr;
    auto image_type = ExecutionContext::BarrierImageType::Regular;

    if (tex == d3d::get_backbuffer_tex())
      image_type = ExecutionContext::BarrierImageType::SwapchainColor;
    else if (tex == d3d::get_backbuffer_tex_depth())
      image_type = ExecutionContext::BarrierImageType::SwapchainDepth;
    else
    {
      image = cast_to_texture_base(tex)->tex.image;

      uint32_t stop_index = (res_range == 0) ? image->getMipLevels() * image->getArrayLayers() : res_range + res_index;

      G_UNUSED(stop_index);
      G_ASSERTF((image->getMipLevels() * image->getArrayLayers()) >= stop_index,
        "Out of bound subresource index range (%d, %d+%d) in resource barrier for image %s (0, %d)", res_index, res_index, res_range,
        image->getDebugName(), image->getMipLevels() * image->getArrayLayers());
    }

    CmdImageBarrier cmd{image, uint32_t(image_type), state, res_index, res_range};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  });
}

void DeviceContext::initTempBuffersConfiguration()
{
  VkDeviceSize maxTempBufferAlloc[TempBufferManager::TYPE_COUNT] = {
    UNIFORM_BUFFER_BLOCK_SIZE, USER_POINTER_VERTEX_BLOCK_SIZE, USER_POINTER_INDEX_BLOCK_SIZE, INITIAL_UPDATE_BUFFER_BLOCK_SIZE};

  VkDeviceSize tempBufferAligments[TempBufferManager::TYPE_COUNT] = {
    device.getDeviceProperties().properties.limits.minUniformBufferOffsetAlignment, device.getMinimalBufferAlignment(),
    device.getMinimalBufferAlignment(), 1};

  for (int i = 0; i != TempBufferManager::TYPE_COUNT; ++i)
    front.tempBufferManagers[i].setConfig(maxTempBufferAlloc[i], tempBufferAligments[i], i);
}

void DeviceContext::captureScreen(Buffer *dst)
{
  CmdCaptureScreen cmd{dst};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::deleteAsyncCompletionStateOnFinish(AsyncCompletionState &sync)
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->cleanups.enqueue(sync);
}

#if D3D_HAS_RAY_TRACING

RaytraceBLASBufferRefs getRaytraceGeometryRefs(const RaytraceGeometryDescription &desc)
{
  switch (desc.type)
  {
    case RaytraceGeometryDescription::Type::TRIANGLES:
    {
      auto devVbuf = ((GenericBufferInterface *)desc.data.triangles.vertexBuffer)->getDeviceBuffer();
      uint32_t vofs = desc.data.triangles.vertexOffset * desc.data.triangles.vertexStride;
      if (desc.data.triangles.indexBuffer)
      {
        auto devIbuf = ((GenericBufferInterface *)desc.data.triangles.indexBuffer)->getDeviceBuffer();
        uint32_t indexSize =
          ((GenericBufferInterface *)desc.data.triangles.indexBuffer)->getIndexType() == VkIndexType::VK_INDEX_TYPE_UINT32 ? 4 : 2;
        return {devVbuf, (uint32_t)devVbuf->dataOffset(vofs), (uint32_t)devVbuf->dataSize() - vofs, devIbuf,
          (uint32_t)devIbuf->dataOffset(desc.data.triangles.indexOffset * indexSize), desc.data.triangles.indexCount * indexSize};
      }
      else
        return {devVbuf, (uint32_t)devVbuf->dataOffset(vofs), (uint32_t)devVbuf->dataSize() - vofs, nullptr, 0, 0};
    }
    case RaytraceGeometryDescription::Type::AABBS:
    {
      auto devBuf = ((GenericBufferInterface *)desc.data.aabbs.buffer)->getDeviceBuffer();
      return {
        devBuf, (uint32_t)devBuf->dataOffset(desc.data.aabbs.offset), desc.data.aabbs.stride * desc.data.aabbs.count, nullptr, 0, 0};
    }
    default: G_ASSERTF(0, "vulkan: unknown geometry type %u in RaytraceGeometryDescription", (uint32_t)desc.type);
  }
  return {nullptr, 0, 0, nullptr, 0, 0};
}

void DeviceContext::raytraceBuildBottomAccelerationStructure(RaytraceBottomAccelerationStructure *as,
  RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags flags, bool update, Buffer *scratch_buf)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  CmdRaytraceBuildBottomAccelerationStructure cmd;
  cmd.scratchBuffer = scratch_buf;
  cmd.as = (RaytraceAccelerationStructure *)as;
  cmd.descCount = count;
  cmd.flags = flags;
  cmd.update = update;
  VULKAN_LOCK_FRONT();
  cmd.descIndex = uint32_t(front.replayRecord->raytraceGeometryKHRStore.size());
  G_ASSERT(front.replayRecord->raytraceGeometryKHRStore.size() == front.replayRecord->raytraceBuildRangeInfoKHRStore.size());
  for (uint32_t i = 0; i < count; ++i)
  {
    uint32_t primitiveOffset = 0;
    const auto *ibuf = (const GenericBufferInterface *)desc[i].data.triangles.indexBuffer;
    if (ibuf)
    {
      const VkIndexType indexType = ibuf->getIndexType();
      if (indexType == VkIndexType::VK_INDEX_TYPE_UINT32)
      {
        primitiveOffset = desc[i].data.triangles.indexOffset * 4;
      }
      else
      {
        G_ASSERT(indexType == VkIndexType::VK_INDEX_TYPE_UINT16);
        primitiveOffset = desc[i].data.triangles.indexOffset * 2;
      }
    }
    front.replayRecord->raytraceBuildRangeInfoKHRStore.push_back({
      desc[i].data.triangles.indexCount / 3, // primitiveCount
      primitiveOffset,
      0, // firstVertex
      0  // transformOffset
    });
    front.replayRecord->raytraceGeometryKHRStore.push_back(
      RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(dev, desc[i]));
    front.replayRecord->raytraceBLASBufferRefsStore.push_back(getRaytraceGeometryRefs(desc[i]));
  }
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
#else
  G_UNUSED(as);
  G_UNUSED(desc);
  G_UNUSED(count);
  G_UNUSED(flags);
  G_UNUSED(update);
  G_UNUSED(scratch_buf);
#endif
}

void DeviceContext::raytraceBuildTopAccelerationStructure(RaytraceTopAccelerationStructure *as, Buffer *instance_buffer,
  uint32_t instance_count, RaytraceBuildFlags flags, bool update, Buffer *scratch_buf)
{
  CmdRaytraceBuildTopAccelerationStructure cmd;
  cmd.scratchBuffer = scratch_buf;
  cmd.as = (RaytraceAccelerationStructure *)as;
  cmd.instanceBuffer = BufferRef{instance_buffer};
  cmd.instanceCount = instance_count;
  cmd.flags = flags;
  cmd.update = update;
  VULKAN_DISPATCH_COMMAND(cmd);
}
void DeviceContext::deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc)
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->cleanups.enqueue<RaytraceAccelerationStructure::CLEANUP_DESTROY_BOTTOM>(
    *((RaytraceAccelerationStructure *)desc));
}
void DeviceContext::deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc)
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->cleanups.enqueue<RaytraceAccelerationStructure::CLEANUP_DESTROY_TOP>(*((RaytraceAccelerationStructure *)desc));
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
  cmd.vs = front.replayRecord->shaderModuleUses.size();
  front.replayRecord->shaderModuleUses.push_back(vs_blob);

  cmd.fs = front.replayRecord->shaderModuleUses.size();
  front.replayRecord->shaderModuleUses.push_back(fs_blob);

  if (gs_blob)
  {
    cmd.gs = front.replayRecord->shaderModuleUses.size();
    front.replayRecord->shaderModuleUses.push_back(*gs_blob);
  }
  else
    cmd.gs = ~uint32_t(0);

  if (tc_blob)
  {
    cmd.tc = front.replayRecord->shaderModuleUses.size();
    front.replayRecord->shaderModuleUses.push_back(*tc_blob);
  }
  else
    cmd.tc = ~uint32_t(0);

  if (te_blob)
  {
    cmd.te = front.replayRecord->shaderModuleUses.size();
    front.replayRecord->shaderModuleUses.push_back(*te_blob);
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

void DeviceContext::setSwappyTargetRate(int rate)
{
  CmdSetSwappyTargetRate cmd{rate};
  VULKAN_DISPATCH_COMMAND(cmd);
}

void DeviceContext::getSwappyStatus(int *status)
{
  CmdGetSwappyStatus cmd{status};
  VULKAN_DISPATCH_COMMAND(cmd);
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::addRaytraceProgram(ProgramID program, uint32_t max_recursion, uint32_t shader_count,
  const ShaderModuleUse *shaders, uint32_t group_count, const RaytraceShaderGroup *groups)
{
  CmdAddRaytraceProgram cmd{program};
  VULKAN_LOCK_FRONT();

  cmd.shaders = shaders;
  cmd.shaderGroups = groups;
  cmd.shaderCount = shader_count;
  cmd.groupCount = group_count;
  cmd.maxRecursion = max_recursion;

  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
}

void DeviceContext::copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size,
  Buffer *buffer, uint32_t offset)
{
  CmdCopyRaytraceShaderGroupHandlesToMemory cmd{prog, first_group, group_count, size, nullptr};

  if (buffer->hasMappedMemory())
  {
    cmd.ptr = buffer->dataPointer(offset);
    VULKAN_DISPATCH_COMMAND(cmd);
  }
  else
  {
    auto tempBuffer = TempBufferHolder(device, front.tempBufferManagers[TempBufferManager::TYPE_UPDATE], size);
    cmd.ptr = tempBuffer.getPtr();

    // if memory is not host visible then use update buffer temp memory for it
    VULKAN_LOCK_FRONT();
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);

    copyBuffer(tempBuffer.get().buffer, buffer, tempBuffer.get().offset, offset, size);
  }
}
#endif

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
void DeviceContext::attachComputeProgramDebugInfo(ProgramID program, eastl::unique_ptr<ShaderDebugInfo> dbg)
{
  CmdAttachComputeProgramDebugInfo cmd{program, dbg.release()};
  VULKAN_DISPATCH_COMMAND(cmd);
}
#endif

void DeviceContext::placeAftermathMarker(const char *name)
{
  if (!device.getDebugLevel())
    return;

  CmdPlaceAftermathMarker cmd;
  cmd.stringLength = strlen(name) + 1;
  VULKAN_LOCK_FRONT();
  cmd.stringIndex = StringIndexRef{front.replayRecord->charStore.size()};
  front.replayRecord->charStore.insert(front.replayRecord->charStore.end(), name, name + cmd.stringLength);
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

  auto &replayTimeline = ctx.device.timelineMan.get<TimelineManager::CpuReplay>();
  while (!interlocked_acquire_load(terminating))
  {
    // wait for at least one work item to be processed
    if (replayTimeline.waitSubmit(1, 1))
      replayTimeline.advance();
  }
}

void DeviceContext::initTimingRecord()
{
#if VULKAN_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
  front.lastPresentTimeStamp = now;
#endif
}

void DeviceContext::advanceAndCheckTimingRecord()
{
#if VULKAN_RECORD_TIMING_DATA
  // save current frame data
  auto &frameTiming = front.timingHistory[front.frameIndex % FRAME_TIMING_HISTORY_LENGTH];
  frameTiming.frontendBackendWaitDuration = front.frontendBackendWaitDuration;
  uint64_t now = profile_ref_ticks();
  frameTiming.frontendUpdateScreenInterval = now - front.lastPresentTimeStamp;
  front.lastPresentTimeStamp = now;

  CmdRecordFrameTimings cmd{&frameTiming, now};
  VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);

  // advance to surely finished frame index
  // A queued, B in thread, C on GPU, D is done
  front.completedFrameIndex = max<uint32_t>(front.frameIndex - MAX_PENDING_WORK_ITEMS - FRAME_FRAME_BACKLOG_LENGTH, 0);
#endif
}

void DeviceContext::registerFrameEventsCallback(FrameEvents *callback, bool useFront)
{
  VULKAN_LOCK_FRONT();
  if (useFront)
  {
    callback->beginFrameEvent();
    front.frameEventCallbacks.emplace_back(callback);
  }
  else
  {
    CmdRegisterFrameEventsCallback cmd{callback};
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }
}

void DeviceContext::callFrameEndCallbacks()
{
  for (FrameEvents *callback : front.frameEventCallbacks)
    callback->endFrameEvent();
  front.frameEventCallbacks.clear();
}

DeviceContext::DeviceContext(Device &dvc) : device(dvc), front(dvc.timelineMan), back(dvc.timelineMan), vkDev(dvc.getVkDevice()) {}
