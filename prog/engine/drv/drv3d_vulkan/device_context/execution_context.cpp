#include "device.h"
#include <validation.h>
#if _TARGET_ANDROID
#include <unistd.h>
#endif

using namespace drv3d_vulkan;

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
thread_local ExecutionContext *ExecutionContext::tlsDbgActiveInstance = nullptr;
#endif

void ExecutionContext::writeExectionChekpoint(VulkanCommandBufferHandle cb, VkPipelineStageFlagBits stage)
{
  if (!device.getFeatures().test(DeviceFeaturesConfig::COMMAND_MARKER))
    return;

  device.execMarkers.write(cb, stage, data.id, cmd);
}

void ExecutionContext::imageBarrier(Image *, BarrierImageType, ResourceBarrier, uint32_t, uint32_t) {}

void ExecutionContext::switchFrameCore()
{
  back.contextState.cmdListsToSubmit.push_back(frameCore);
  allocFrameCore();
}

void ExecutionContext::allocFrameCore()
{
  frameCore = back.contextState.frame->allocateCommandBuffer(vkDev);
  VkCommandBufferBeginInfo cbbi;
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.pNext = nullptr;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cbbi.pInheritanceInfo = nullptr;
  VULKAN_EXIT_ON_FAIL(vkDev.vkBeginCommandBuffer(frameCore, &cbbi));
}

void ExecutionContext::flushUnorderedImageColorClears()
{
  TIME_PROFILE(vulkan_flush_unordered_rt_clears);
  for (const CmdClearColorTexture &t : data.unorderedImageColorClears)
    clearColorImage(t.image, t.area, t.value);
}

void ExecutionContext::flushUnorderedImageDepthStencilClears()
{
  TIME_PROFILE(vulkan_flush_unordered_ds_clears);
  for (const CmdClearDepthStencilTexture &t : data.unorderedImageDepthStencilClears)
    clearDepthStencilImage(t.image, t.area, t.value);
}

void ExecutionContext::flushUnorderedImageCopies()
{
  TIME_PROFILE(vulkan_flush_unordered_image_copies);

  for (CmdCopyImage &i : data.unorderedImageCopies)
    copyImage(i.src, i.dst, i.srcMip, i.dstMip, i.mipCount, i.regionCount, i.regionIndex);
}

void ExecutionContext::prepareFrameCore()
{
  G_ASSERTF(is_null(frameCore), "vulkan: execution context already prepared for command execution");
  allocFrameCore();
  TIME_PROFILE(vulkan_pre_frame_core);
  // for optional preFrame
  back.contextState.cmdListsToSubmit.push_back(VulkanCommandBufferHandle{});
  back.contextState.bindlessManagerBackend.advance();
  flushUnorderedImageColorClears();
  flushUnorderedImageDepthStencilClears();
  flushImageUploads();
  flushUnorderedImageCopies();
  // should be after upload, to not overwrite data
  processFillEmptySubresources();
  flushBufferUploads(frameCore);
  flushOrderedBufferUploads(frameCore);
}

void ExecutionContext::processFillEmptySubresources()
{
  if (data.imagesToFillEmptySubresources.empty())
    return;

  TIME_PROFILE(vulkan_fill_empty_images);

  Buffer *zeroBuf = nullptr;
  Tab<VkBufferImageCopy> copies;

  for (Image *i : data.imagesToFillEmptySubresources)
  {
    if (!i->layout.anySubresInState(VK_IMAGE_LAYOUT_UNDEFINED))
      continue;

    // greedy allocation to avoid reallocations and memory wasting
    VkDeviceSize subresSz = i->getSubresourceMaxSize();
    if (!zeroBuf || (zeroBuf->dataSize() < subresSz))
    {
      if (zeroBuf)
        back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*zeroBuf);
      zeroBuf =
        get_device().createBuffer(subresSz, DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER, 1, BufferMemoryFlags::TEMP);
      zeroBuf->setDebugName("fill empty subres zero buf");
      memset(zeroBuf->dataPointer(0), 0, zeroBuf->dataSize());
      zeroBuf->markNonCoherentRange(0, zeroBuf->dataSize(), true);
    }

    ValueRange<uint8_t> mipRange = i->getMipLevelRange();
    ValueRange<uint16_t> arrRange = i->getArrayLayerRange();

    for (uint16_t arr : arrRange)
      for (uint8_t mip : mipRange)
        if (i->layout.get(mip, arr) == VK_IMAGE_LAYOUT_UNDEFINED)
        {
          // better to ensure that image are properly filled with data
          // but even better is to avoid crashing due to wrong barriers caused by such textures
          logwarn("vulkan: image %p:<%s> uninitialized subres %u:%u filled with zero contents", i, i->getDebugName(), arr, mip);

          back.syncTrack.addImageAccess(
            // present barrier is very special, as vkQueuePresent does all visibility ops
            // ref: vkQueuePresentKHR spec notes
            {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, i, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {mip, 1, arr, 1});
          copies.push_back(i->makeBufferCopyInfo(mip, arr, zeroBuf->dataOffset(0)));
        }

    back.syncTrack.completeNeeded(frameCore, vkDev);

    VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, zeroBuf->getHandle(), i->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      copies.size(), copies.data()));

    copies.clear();
  }

  if (zeroBuf)
    back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*zeroBuf);
}

VulkanCommandBufferHandle ExecutionContext::allocAndBeginCommandBuffer()
{
  FrameInfo &frame = back.contextState.frame.get();
  VulkanCommandBufferHandle ncmd = frame.allocateCommandBuffer(vkDev);
  VkCommandBufferBeginInfo cbbi;
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.pNext = nullptr;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cbbi.pInheritanceInfo = nullptr;
  VULKAN_EXIT_ON_FAIL(vkDev.vkBeginCommandBuffer(ncmd, &cbbi));
  return ncmd;
}

VulkanCommandBufferHandle ExecutionContext::flushImageDownloads(VulkanCommandBufferHandle cmd_b)
{
  if (data.imageDownloads.empty())
    return cmd_b;

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  for (auto &&download : data.imageDownloads)
  {
    if (!download.image)
      download.image = getSwapchainColorImage();
    else if (!download.image->isResident())
      continue;

    for (auto iter = begin(data.imageDownloadCopies) + download.copyIndex,
              ed = begin(data.imageDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      uint32_t sz =
        download.image->getFormat().calculateImageSize(iter->imageExtent.width, iter->imageExtent.height, iter->imageExtent.depth, 1) *
        iter->imageSubresource.layerCount;
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, download.buffer,
        {iter->bufferOffset, sz});
      back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, download.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        {iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount});
    }
  }

  back.syncTrack.completeNeeded(cmd_b, vkDev);

  for (auto &&download : data.imageDownloads)
  {
    if (!download.image->isResident())
      continue;

    // do the copy
    VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(cmd_b, download.image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      download.buffer->getHandle(), download.copyCount, data.imageDownloadCopies.data() + download.copyIndex));
  }

  return cmd_b;
}

VulkanCommandBufferHandle ExecutionContext::flushBufferDownloads(VulkanCommandBufferHandle cmd_b)
{
  if (data.bufferDownloads.empty())
    return cmd_b;

  if (device.getFeatures().test(DeviceFeaturesConfig::OPTIMIZE_BUFFER_UPLOADS))
    BufferCopyInfo::optimizeBufferCopies(data.bufferDownloads, data.bufferDownloadCopies);

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  for (auto &&download : data.bufferDownloads)
  {
    for (auto iter = begin(data.bufferDownloadCopies) + download.copyIndex,
              ed = begin(data.bufferDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, download.src,
        {iter->srcOffset, iter->size});
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, download.dst,
        {iter->dstOffset, iter->size});
    }
  }

  back.syncTrack.completeNeeded(cmd_b, vkDev);

  for (auto &&download : data.bufferDownloads)
  {
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(cmd_b, download.src->getHandle(), download.dst->getHandle(), download.copyCount,
      data.bufferDownloadCopies.data() + download.copyIndex));
  }

  return cmd_b;
}

void ExecutionContext::flushImageUploadsIter(uint32_t start, uint32_t end)
{
  back.syncTrack.completeNeeded(frameCore, vkDev);
  for (uint32_t i = start; i < end; ++i)
  {
    ImageCopyInfo &upload = data.imageUploads[i];
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, upload.buffer->getHandle(), upload.image->getHandle(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, upload.copyCount, data.imageUploadCopies.data() + upload.copyIndex));
  }
}

void ExecutionContext::flushImageUploads()
{
  if (data.imageUploads.empty())
    return;

  TIME_PROFILE(vulkan_flush_image_uploads);
  // access tracking
  for (auto &&upload : data.imageUploads)
  {
    upload.buffer->optionallyActivateRoSeal(data.id);

    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      uint32_t sz =
        upload.image->getFormat().calculateImageSize(iter->imageExtent.width, iter->imageExtent.height, iter->imageExtent.depth, 1) *
        iter->imageSubresource.layerCount;
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.buffer,
        {iter->bufferOffset, sz});
    }
  }

  back.syncTrack.completeNeeded(frameCore, vkDev);

  uint32_t mergedRangeStart = 0;
  for (uint32_t i = 0; i < data.imageUploads.size(); ++i)
  {
    ImageCopyInfo &upload = data.imageUploads[i];

    if (upload.image->hasLastSyncOpIndex())
    {
      flushImageUploadsIter(mergedRangeStart, i);
      mergedRangeStart = i;
    }

    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        {iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount});
    }
  }
  flushImageUploadsIter(mergedRangeStart, data.imageUploads.size());

  bool anyBindless = false;
  for (auto &&upload : data.imageUploads)
  {
    if (!upload.image->isSampledSRV())
    {
      upload.image->layout.roSealTargetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      upload.image->requestRoSeal(data.id);
    }
    // we can't know is it readed or not, so assume readed
    if (upload.image->isUsedInBindless())
    {
      trackBindlessRead(upload.image);
      anyBindless = true;
    }
  }
  if (anyBindless)
    back.syncTrack.completeNeeded(frameCore, vkDev);
}

void ExecutionContext::flushOrderedBufferUploads(VulkanCommandBufferHandle cmd_b)
{
  if (data.orderedBufferUploads.empty())
    return;

  TIME_PROFILE(vulkan_flush_ordered_buffer_uploads);

  for (auto &&upload : data.orderedBufferUploads)
  {
    upload.src->optionallyActivateRoSeal(data.id);

    for (auto iter = begin(data.orderedBufferUploadCopies) + upload.copyIndex,
              ed = begin(data.orderedBufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.src,
        {iter->srcOffset, iter->size});
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.dst,
        {iter->dstOffset, iter->size});
    }

    back.syncTrack.completeNeeded(cmd_b, vkDev);

    VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(cmd_b, upload.src->getHandle(), upload.dst->getHandle(), upload.copyCount,
      data.orderedBufferUploadCopies.data() + upload.copyIndex));
    // to seal it here, we need some trusty conditions
    // upload.dst->requestRoSeal(data.id);
  }
}
void ExecutionContext::flushBufferUploads(VulkanCommandBufferHandle cmd_b)
{
  if (data.bufferUploads.empty())
    return;

  TIME_PROFILE(vulkan_flush_buffer_uploads);

  if (device.getFeatures().test(DeviceFeaturesConfig::OPTIMIZE_BUFFER_UPLOADS))
    BufferCopyInfo::optimizeBufferCopies(data.bufferUploads, data.bufferUploadCopies);

  for (auto &&upload : data.bufferUploads)
  {
    upload.src->optionallyActivateRoSeal(data.id);

    for (auto iter = begin(data.bufferUploadCopies) + upload.copyIndex,
              ed = begin(data.bufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.src,
        {iter->srcOffset, iter->size});
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.dst,
        {iter->dstOffset, iter->size});
    }
  }

  back.syncTrack.completeNeeded(cmd_b, vkDev);

  for (auto &&upload : data.bufferUploads)
  {
    // sadly no way to batch those together...
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(cmd_b, upload.src->getHandle(), upload.dst->getHandle(), upload.copyCount,
      data.bufferUploadCopies.data() + upload.copyIndex));
    // to seal it here, we need some trusty conditions
    // upload.dst->requestRoSeal(data.id);
  }
}

VulkanCommandBufferHandle ExecutionContext::flushBufferToHostFlushes(VulkanCommandBufferHandle cmd_b)
{
  if (data.bufferToHostFlushes.empty())
    return cmd_b;

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  for (auto &&flush : data.bufferToHostFlushes)
    back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT}, flush.buffer, {flush.offset, flush.range});
  back.syncTrack.completeNeeded(cmd_b, vkDev);

  return cmd_b;
}

void ExecutionContext::writeToDebug(StringIndexRef index)
{
#if VK_EXT_debug_utils
  if (vkDev.getInstance().hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsLabelEXT dul = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    dul.pLabelName = data.charStore.data() + index.get();
    dul.color[0] = 1.f;
    dul.color[1] = 1.f;
    dul.color[2] = 1.f;
    dul.color[3] = 1.f;
    VULKAN_LOG_CALL(vkDev.getInstance().vkCmdInsertDebugUtilsLabelEXT(frameCore, &dul));
  }
#else
  G_UNUSED(index);
#endif
}

void ExecutionContext::writeDebugMessage(StringIndexRef message_index, int severity)
{
  if (severity < 1)
    debug("%s", data.charStore.data() + message_index.get());
  else if (severity < 2)
    logwarn("%s", data.charStore.data() + message_index.get());
  else
    logerr("%s", data.charStore.data() + message_index.get());
}

void ExecutionContext::flushDraws()
{
  FrameInfo &frame = back.contextState.frame.get();
  flush(frame.frameDone);
  device.timelineMan.get<TimelineManager::GpuExecute>().advance();
  back.contextState.frame.restart();
}

void ExecutionContext::flush(ThreadedFence *fence)
{
  G_ASSERT(!flushProcessed);

  beginCustomStage("flush");

  for (ImmediateConstBuffer &icb : back.immediateConstBuffers)
    icb.onFlush();

  auto preFrame = VulkanCommandBufferHandle{};
  if (device.timestamps.writtenThisFrame())
  {
    preFrame = allocAndBeginCommandBuffer();
    device.timestamps.writeResetsAndQueueReadbacks(preFrame);
  }
  if (!is_null(preFrame))
    back.contextState.cmdListsToSubmit[0] = preFrame;

  auto postFrame = flushBufferDownloads(frameCore);
  postFrame = flushImageDownloads(postFrame);
  postFrame = flushBufferToHostFlushes(postFrame);

  back.syncTrack.completeAll(postFrame, vkDev, data.id);

  back.contextState.cmdListsToSubmit.push_back(postFrame);
  for (VulkanCommandBufferHandle i : back.contextState.cmdListsToSubmit)
  {
    if (!is_null(i))
      VULKAN_EXIT_ON_FAIL(vkDev.vkEndCommandBuffer(i));
  }

  DeviceQueue::TrimmedSubmitInfo si = {};
  si.pCommandBuffers = ary(back.contextState.cmdListsToSubmit.data());
  si.commandBufferCount = back.contextState.cmdListsToSubmit.size();

  // skip 0 element when preFrame is empty
  if (is_null(preFrame))
  {
    ++si.pCommandBuffers;
    --si.commandBufferCount;
  }

  VulkanSemaphoreHandle syncSemaphore;
  syncSemaphore = back.contextState.frame->allocSemaphore(vkDev, true);
  si.pSignalSemaphores = ary(&syncSemaphore);
  si.signalSemaphoreCount = 1;

  if (fence)
  {
    device.getQueue(DeviceQueueType::GRAPHICS).submit(vkDev, si, fence->get());
    fence->setAsSubmited();
  }
  else
  {
    device.getQueue(DeviceQueueType::GRAPHICS).submit(vkDev, si);
  }
  device.getQueue(DeviceQueueType::GRAPHICS).addSubmitSemaphore(syncSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);


  back.contextState.cmdListsToSubmit.clear();
  onFrameCoreReset();

  back.pipelineCompiler.processQueued();
  flushProcessed = true;
}

bool ExecutionContext::isDebugEventsAllowed() { return device.getFeatures().test(DeviceFeaturesConfig::ALLOW_DEBUG_MARKERS); }

void ExecutionContext::insertEvent(const char *marker, uint32_t color /*=0xFFFFFFFF*/)
{
  if (!device.getFeatures().test(DeviceFeaturesConfig::ALLOW_DEBUG_MARKERS))
    return;

#if VK_EXT_debug_utils
  if (vkDev.getInstance().hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = marker;
    info.color[0] = (0xFF & (color >> 0)) / 255.0f;
    info.color[1] = (0xFF & (color >> 8)) / 255.0f;
    ;
    info.color[2] = (0xFF & (color >> 16)) / 255.0f;
    info.color[3] = (0xFF & (color >> 24)) / 255.0f;
    VULKAN_LOG_CALL(vkDev.getInstance().vkCmdInsertDebugUtilsLabelEXT(frameCore, &info));
    return;
  }
#endif

#if VK_EXT_debug_marker
  if (vkDev.hasExtension<DebugMarkerEXT>())
  {
    VkDebugMarkerMarkerInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    info.pNext = nullptr;
    info.pMarkerName = marker;
    info.color[0] = (0xFF & (color >> 0)) / 255.0f;
    info.color[1] = (0xFF & (color >> 8)) / 255.0f;
    ;
    info.color[2] = (0xFF & (color >> 16)) / 255.0f;
    info.color[3] = (0xFF & (color >> 24)) / 255.0f;
    VULKAN_LOG_CALL(vkDev.vkCmdDebugMarkerInsertEXT(frameCore, &info));
  }
#endif
}

void ExecutionContext::pushEventRaw(const char *marker, uint32_t color /*=0xFFFFFFFF*/)
{
  if (!device.getFeatures().test(DeviceFeaturesConfig::ALLOW_DEBUG_MARKERS))
    return;

#if VK_EXT_debug_marker
  if (vkDev.hasExtension<DebugMarkerEXT>())
  {
    VkDebugMarkerMarkerInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    info.pNext = nullptr;
    info.pMarkerName = marker;
    info.color[0] = (0xFF & (color >> 0)) / 255.0f;
    info.color[1] = (0xFF & (color >> 8)) / 255.0f;
    ;
    info.color[2] = (0xFF & (color >> 16)) / 255.0f;
    info.color[3] = (0xFF & (color >> 24)) / 255.0f;
    VULKAN_LOG_CALL(vkDev.vkCmdDebugMarkerBeginEXT(frameCore, &info));
  }
#endif
#if VK_EXT_debug_utils
  auto &instance = vkDev.getInstance();
  if (instance.hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsLabelEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pNext = nullptr;
    info.pLabelName = marker;
    info.color[0] = (0xFF & (color >> 0)) / 255.0f;
    info.color[1] = (0xFF & (color >> 8)) / 255.0f;
    ;
    info.color[2] = (0xFF & (color >> 16)) / 255.0f;
    info.color[3] = (0xFF & (color >> 24)) / 255.0f;
    VULKAN_LOG_CALL(instance.vkCmdBeginDebugUtilsLabelEXT(frameCore, &info));
  }
#endif
}

void ExecutionContext::popEventRaw()
{
  if (!device.getFeatures().test(DeviceFeaturesConfig::ALLOW_DEBUG_MARKERS))
    return;

#if VK_EXT_debug_marker
  if (vkDev.hasExtension<DebugMarkerEXT>())
  {
    VULKAN_LOG_CALL(vkDev.vkCmdDebugMarkerEndEXT(frameCore));
  }
#endif
#if VK_EXT_debug_utils
  auto &instance = vkDev.getInstance();
  if (instance.hasExtension<DebugUtilsEXT>())
  {
    VULKAN_LOG_CALL(instance.vkCmdEndDebugUtilsLabelEXT(frameCore));
  }
#endif
}

void ExecutionContext::pushEvent(StringIndexRef name) { pushEventRaw(data.charStore.data() + name.get()); }

void ExecutionContext::popEvent() { popEventRaw(); }

VulkanImageViewHandle ExecutionContext::getImageView(Image *img, ImageViewState state) { return device.getImageView(img, state); }

void ExecutionContext::beginQuery(VulkanQueryPoolHandle pool, uint32_t index, VkQueryControlFlags flags)
{
  VULKAN_LOG_CALL(vkDev.vkCmdBeginQuery(frameCore, pool, index, flags));
}

void ExecutionContext::endQuery(VulkanQueryPoolHandle pool, uint32_t index)
{
  VULKAN_LOG_CALL(vkDev.vkCmdEndQuery(frameCore, pool, index));
}

void ExecutionContext::writeTimestamp(const TimestampQueryRef &query) { device.timestamps.write(query, frameCore); }

void ExecutionContext::wait(ThreadedFence *fence) { fence->wait(vkDev); }

void ExecutionContext::beginConditionalRender(VulkanBufferHandle buffer, VkDeviceSize offset)
{
#if VK_EXT_conditional_rendering
  VkConditionalRenderingBeginInfoEXT crbi = //
    {VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT, nullptr, buffer, offset, 0};
  VULKAN_LOG_CALL(vkDev.vkCmdBeginConditionalRenderingEXT(frameCore, &crbi));
#else
  G_UNUSED(buffer);
  G_UNUSED(offset);
#endif
}

void ExecutionContext::endConditionalRender()
{
#if VK_EXT_conditional_rendering
  VULKAN_LOG_CALL(vkDev.vkCmdEndConditionalRenderingEXT(frameCore));
#endif
}

void ExecutionContext::addShaderModule(const ShaderModuleBlob *sci, uint32_t id)
{
  back.contextState.shaderModules.push_back(
    eastl::make_unique<ShaderModule>(device.makeVkModule(sci), id, sci->hash, sci->getBlobSize()));
  delete sci;
}

void ExecutionContext::removeShaderModule(uint32_t id)
{
  // add-remove order is important, as we use tight packing for module ids
  auto moduleRef = eastl::find_if(begin(back.contextState.shaderModules), end(back.contextState.shaderModules),
    [=](const eastl::unique_ptr<ShaderModule> &sm) { return sm->id == id; });
  back.contextState.frame->deletedShaderModules.push_back(eastl::move(*moduleRef));
  *moduleRef = eastl::move(back.contextState.shaderModules.back());
  back.contextState.shaderModules.pop_back();
}

void ExecutionContext::addComputePipeline(ProgramID program, const ShaderModuleBlob *sci, const ShaderModuleHeader &header)
{
  device.pipeMan.addCompute(vkDev, device.getPipeCache(), program, *sci, header);
  delete sci;
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
void ExecutionContext::attachComputePipelineDebugInfo(ProgramID program, const ShaderDebugInfo *info)
{
  device.pipeMan.get<ComputePipeline>(program).setDebugInfo({{*info}});
  delete info;
}
#endif

void ExecutionContext::addGraphicsPipeline(ProgramID program, uint32_t vs, uint32_t fs, uint32_t gs, uint32_t tc, uint32_t te)
{
  struct ShaderModules
  {
    ShaderModule *module = nullptr;
    ShaderModuleHeader *header = nullptr;
    ShaderModuleUse *use = nullptr;
  };

  auto getShaderModules = [&](uint32_t index) {
    ShaderModules modules;
    if (index < data.shaderModuleUses.size())
    {
      modules.use = &data.shaderModuleUses[index];
      modules.header = &modules.use->header;
      auto ref = eastl::find_if(begin(back.contextState.shaderModules), end(back.contextState.shaderModules),
        [id = modules.use->blobId](const eastl::unique_ptr<ShaderModule> &module) { return id == module->id; });
      modules.module = ref->get();
    }
    return modules;
  };

  ShaderModules vsModules = getShaderModules(vs);
  ShaderModules fsModules = getShaderModules(fs);
  ShaderModules tcModules = getShaderModules(tc);
  ShaderModules teModules = getShaderModules(te);
  ShaderModules gsModules = getShaderModules(gs);

  device.pipeMan.addGraphics(vkDev, program, *vsModules.module, *vsModules.header, *fsModules.module, *fsModules.header,
    gsModules.module, gsModules.header, tcModules.module, tcModules.header, teModules.module, teModules.header);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  device.pipeMan.get<VariatedGraphicsPipeline>(program).setDebugInfo(
    {{vsModules.use->dbg, fsModules.use->dbg, gsModules.use ? gsModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      tcModules.use ? tcModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      teModules.use ? teModules.use->dbg : GraphicsPipeline::emptyDebugInfo}});
#endif
}

void ExecutionContext::removeProgram(ProgramID prog)
{
  if (back.pipelineStatePendingCleanups.removeWithReferenceCheck(prog, back.pipelineState))
  {
    device.pipeMan.prepareRemoval(prog);
    get_shader_program_database().reuseId(prog);
  }
}

void ExecutionContext::addRenderState(shaders::DriverRenderStateId id, const shaders::RenderState &render_state_data)
{
  back.contextState.renderStateSystem.setRenderStateData(id, render_state_data, device);
}

void ExecutionContext::addPipelineCache(VulkanPipelineCacheHandle cache)
{
  if (VULKAN_CHECK_OK(vkDev.vkMergePipelineCaches(vkDev.get(), device.getPipeCache(), 1, ptr(cache))))
    debug("vulkan: additional pipeline cache added");

  VULKAN_LOG_CALL(vkDev.vkDestroyPipelineCache(vkDev.get(), cache, NULL));
}

void ExecutionContext::checkAndSetAsyncCompletionState(AsyncCompletionState *)
{
  // called only for old frames, so we simply must wait for them to finish
  while (device.timelineMan.get<TimelineManager::GpuExecute>().advance())
    ; // VOID
}

void ExecutionContext::addSyncEvent(AsyncCompletionState *sync) { back.contextState.frame->addSignalReciver(*sync); }

#if D3D_HAS_RAY_TRACING

void ExecutionContext::addRaytracePipeline(ProgramID program, uint32_t max_recursion, uint32_t shader_count,
  const ShaderModuleUse *shaders, uint32_t group_count, const RaytraceShaderGroup *groups)
{
  eastl::unique_ptr<const ShaderModuleUse[]> shaderModules(shaders);
  eastl::unique_ptr<const RaytraceShaderGroup[]> shaderGroups(groups);
  device.pipeMan.addRaytrace(vkDev, device.getPipeCache(), program, max_recursion, shader_count, shaderModules.get(), group_count,
    shaderGroups.get(), back.contextState.shaderModules.data());
}

void ExecutionContext::copyRaytraceShaderGroupHandlesToMemory(ProgramID program, uint32_t first_group, uint32_t group_count,
  uint32_t size, void *ptr)
{
  G_UNUSED(program);
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(ptr);
  G_ASSERTF(false, "ExecutionContext::copyRaytraceShaderGroupHandlesToMemory called on API without support");
}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query

void ExecutionContext::buildAccelerationStructure(VkAccelerationStructureTypeKHR type, uint32_t instance_count,
  Buffer *instance_buffer, uint32_t instance_offset, uint32_t geometry_count, uint32_t first_geometry,
  VkBuildAccelerationStructureFlagsKHR flags, VkBool32 update, RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src,
  Buffer *scratch)
{
  if (type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
      instance_buffer, {instance_buffer->dataOffset(instance_offset), sizeof(VkAccelerationStructureInstanceKHR) * instance_count});
  }
  else
  {
    for (uint32_t i = 0; i < geometry_count; ++i)
    {
      const RaytraceBLASBufferRefs &bufferRefs = *(data.raytraceBLASBufferRefsStore.data() + first_geometry + i);
      back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
        bufferRefs.geometry, {bufferRefs.geometryOffset, bufferRefs.geometrySize});

      if (bufferRefs.index)
        back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
          bufferRefs.index, {bufferRefs.indexOffset, bufferRefs.indexSize});
    }
  }

  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                   VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
    scratch, {scratch->dataOffset(0), scratch->dataSize()});

  if (update)
    back.syncTrack.addAccelerationStructureAccess(
      {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR}, src);
  back.syncTrack.addAccelerationStructureAccess(
    {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR}, dst);

  back.syncTrack.completeNeeded(frameCore, vkDev);

  if (type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    VkAccelerationStructureGeometryInstancesDataKHR instancesData = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instancesData.data.deviceAddress = instance_buffer->getDeviceAddress(vkDev) + instance_offset;

    VkAccelerationStructureGeometryKHR tlasGeo = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeo.geometry.instances = instancesData;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.flags = flags;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &tlasGeo;

    buildInfo.dstAccelerationStructure = dst->getHandle();
    buildInfo.scratchData.deviceAddress = scratch->getDeviceAddress(vkDev);

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveOffset = 0;
    rangeInfo.primitiveCount = instance_count;

    const VkAccelerationStructureBuildRangeInfoKHR *build_ranges = &rangeInfo;
    VULKAN_LOG_CALL(vkDev.vkCmdBuildAccelerationStructuresKHR(frameCore, 1, &buildInfo, &build_ranges));
  }
  else
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.flags = flags;
    buildInfo.geometryCount = geometry_count;
    buildInfo.pGeometries = data.raytraceGeometryKHRStore.data() + first_geometry;

    buildInfo.dstAccelerationStructure = dst->getHandle();
    buildInfo.scratchData.deviceAddress = scratch->getDeviceAddress(vkDev);

    const VkAccelerationStructureBuildRangeInfoKHR *build_ranges = &data.raytraceBuildRangeInfoKHRStore[first_geometry];
    VULKAN_LOG_CALL(vkDev.vkCmdBuildAccelerationStructuresKHR(frameCore, 1, &buildInfo, &build_ranges));

    // there is no processed references to BLAS from TLAS in build data/resource, so for speed
    // do assumed read on CS/RT stage, if we do no read really - that barrier will fail!
    // set seal for safety here, to not conflict with followup writes without reads

    dst->requestRoSeal(data.id);
    back.syncTrack.addAccelerationStructureAccess(ExecutionSyncTracker::LogicAddress::forBLASIndirectReads(), dst);
  }
}

#endif

void ExecutionContext::traceRays(BufferRef ray_gen_table, BufferRef miss_table, BufferRef hit_table, BufferRef callable_table,
  uint32_t ray_gen_offset, uint32_t miss_offset, uint32_t miss_stride, uint32_t hit_offset, uint32_t hit_stride,
  uint32_t callable_offset, uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth)
{
  G_UNUSED(ray_gen_table);
  G_UNUSED(miss_table);
  G_UNUSED(hit_table);
  G_UNUSED(callable_table);
  G_UNUSED(ray_gen_offset);
  G_UNUSED(miss_offset);
  G_UNUSED(miss_stride);
  G_UNUSED(hit_offset);
  G_UNUSED(hit_stride);
  G_UNUSED(callable_offset);
  G_UNUSED(callable_stride);
  G_UNUSED(width);
  G_UNUSED(height);
  G_UNUSED(depth);
  G_ASSERTF(false, "ExecutionContext::traceRays called on API without support");
}

#endif

void ExecutionContext::resolveFrambufferImage(uint32_t, Image *)
{
  // TODO: remove this completely
  G_ASSERTF(false, "vulkan: removed support for render to 3d emulation");
}

void ExecutionContext::resolveMultiSampleImage(Image *src, Image *dst)
{
  VkImageResolve area = {};
  area.srcSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.srcSubresource.layerCount = 1;
  area.dstSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.dstSubresource.layerCount = 1;
  area.extent = src->getBaseExtent();

  Image *effectiveDst = dst ? dst : swapchain.getColorImage();

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, effectiveDst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1});

  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(vkDev.vkCmdResolveImage(frameCore, src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst ? dst->getHandle() : swapchain.getColorImage()->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &area));
}

void ExecutionContext::printMemoryStatistics()
{
  GuardedObjectAutoLock memoryLock(device.memory);
  device.memory->printStats();
  uint32_t freeDeviceLocalMemoryKb = device.getCurrentAvailableMemoryKb();
  if (freeDeviceLocalMemoryKb)
    debug("System reported %u Mb free GPU memory", freeDeviceLocalMemoryKb >> 10);
  device.getMemoryReport().dumpInfo();
}

String ExecutionContext::getCurrentCmdCaller()
{
  String ret("<unknown caller>");

#if DAGOR_DBGLEVEL > 0
  if (cmdIndex < data.commandCallers.size())
    ret = String(2048, "cmd %p = %s", cmd, backtrace::get_stack_by_hash(data.commandCallers[cmdIndex]));
#endif

  return ret;
}

void ExecutionContext::reportMissingPipelineComponent(const char *component)
{
  logerr("vulkan: missing pipeline component %s at state apply, caller %s", component, getCurrentCmdCaller());
  generateFaultReport();
  fatal("vulkan: missing pipeline %s (broken state flush)", component);
}

void ExecutionContext::recordFrameTimings(Drv3dTimings *timing_data, uint64_t kickoff_stamp)
{
  // profile ref ticks can be inconsistent between threads, skip such data
  uint64_t now = profile_ref_ticks();
  timing_data->frontendToBackendUpdateScreenLatency = (int64_t)(now > kickoff_stamp ? now - kickoff_stamp : 0);

  timing_data->gpuWaitDuration = back.gpuWaitDuration;
  timing_data->backendFrontendWaitDuration = back.workWaitDuration;
  timing_data->backbufferAcquireDuration = back.acquireBackBufferDuration;
  // time blocked in present are counted from last frame
  timing_data->presentDuration = back.presentWaitDuration;
}

void ExecutionContext::flushAndWait(ThreadedFence *user_fence)
{
  FrameInfo &frame = back.contextState.frame.get();
  flush(frame.frameDone);
  back.contextState.frame.end();
  while (device.timelineMan.get<TimelineManager::GpuExecute>().advance())
    ; // VOID
  back.contextState.frame.start();

  // we can't wait fence from 2 threads, so
  // shortcut to avoid submiting yet another fence
  if (user_fence)
    user_fence->setSignaledExternally();
}

void ExecutionContext::doFrameEndCallbacks()
{
  if (back.frameEventCallbacks.size())
  {
    TIME_PROFILE(vulkan_frame_end_user_cb);
    for (FrameEvents *callback : back.frameEventCallbacks)
      callback->endFrameEvent();
    back.frameEventCallbacks.clear();
  }

  {
    SharedGuardedObjectAutoLock lock(device.resources);
    device.resources.onBackFrameEnd();
  }

  if (back.memoryStatisticsPeriod)
  {
    int64_t currentTimeRef = ref_time_ticks();

    if (ref_time_delta_to_usec(currentTimeRef - back.lastMemoryStatTime) > back.memoryStatisticsPeriod)
    {
      printMemoryStatistics();
      back.lastMemoryStatTime = currentTimeRef;
    }
  }
}

void ExecutionContext::present()
{
  FrameInfo &frame = back.contextState.frame.get();

  beginCustomStage("present");

  swapchain.prePresent(*this);
  flush(frame.frameDone);
  doFrameEndCallbacks();

  {
    FrameTimingWatch watch(back.presentWaitDuration);
    swapchain.present(*this);
  }

  device.timelineMan.get<TimelineManager::GpuExecute>().advance();
  back.contextState.frame.restart();

  {
    FrameTimingWatch watch(back.acquireBackBufferDuration);
    swapchain.onFrameBegin(*this);
  }

  if (data.generateFaultReport)
    generateFaultReport();
}

void ExecutionContext::captureScreen(Buffer *buffer)
{
  auto colorImage = getSwapchainColorImage();

  // do the copy to buffer
  auto extent = colorImage->getMipExtents2D(0);
  VkBufferImageCopy copy{};
  copy.bufferOffset = buffer->dataOffset(0);
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.layerCount = 1;
  copy.imageExtent.width = extent.width;
  copy.imageExtent.height = extent.height;
  copy.imageExtent.depth = 1;
  const auto copySize = colorImage->getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, 1, 1);

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, colorImage,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buffer,
    {buffer->dataOffset(0), copySize});

  back.syncTrack.completeNeeded(frameCore, vkDev);
  VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(frameCore, colorImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    buffer->getHandle(), 1, &copy));
}

void ExecutionContext::queueBufferDiscard(Buffer *old_buf, const BufferRef &new_buf, uint32_t flags)
{
  back.delayedDiscards.push_back({old_buf, new_buf, flags});
}

bool ExecutionContext::isInMultisampledFBPass()
{
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();

  RenderPassClass::Identifier passClassIdent = getFramebufferState().renderPassClass;
  return inPassState == InPassStateFieldType::NORMAL_PASS && (passClassIdent.colorTargetMask & 1) &&
         passClassIdent.colorSamples[0] > 1;
}

void ExecutionContext::applyQueuedDiscards()
{
  for (const ContextBackend::DiscardNotify &i : back.delayedDiscards)
  {
    bool wasReplaced = back.pipelineState.processBufferDiscard(i.oldBuf, i.newBuf, i.flags);
    if (i.oldBuf != i.newBuf.buffer)
    {
      i.oldBuf->markDead();
      if (wasReplaced)
        back.pipelineStatePendingCleanups.getArray<Buffer *>().push_back(i.oldBuf);
      else
        back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*i.oldBuf);
    }
  }
  back.delayedDiscards.clear();
}

void ExecutionContext::applyStateChanges()
{
  applyQueuedDiscards();
  // pipeline state are persistent, while execution state resets every work item
  // so they can't be merged in one state right now
  back.pipelineState.apply(back.executionState);
  back.executionState.apply(*this);
}

void ExecutionContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  flushComputeState();

  VULKAN_LOG_CALL(vkDev.vkCmdDispatch(frameCore, x, y, z));
  writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void ExecutionContext::dispatchIndirect(BufferRef buffer, uint32_t offset)
{
  trackIndirectArgAccesses(buffer, offset, 1, sizeof(VkDispatchIndirectCommand));
  flushComputeState();

  VULKAN_LOG_CALL(vkDev.vkCmdDispatchIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset)));
  writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void ExecutionContext::copyBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size)
{
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src, {src_offset, size});
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst, {dst_offset, size});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VkBufferCopy bc;
  bc.srcOffset = src_offset;
  bc.dstOffset = dst_offset;
  bc.size = size;
  VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(frameCore, src->getHandle(), dst->getHandle(), 1, &bc));
}

void ExecutionContext::fillBuffer(Buffer *buffer, uint32_t offset, uint32_t size, uint32_t value)
{
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buffer, {offset, size});
  back.syncTrack.completeNeeded(frameCore, vkDev);
  VULKAN_LOG_CALL(vkDev.vkCmdFillBuffer(frameCore, buffer->getHandle(), offset, size, value));
}

void ExecutionContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  // TODO flush_clear_state is only needed if the image is a cleared attachment
  beginCustomStage("clearDepthStencilImage");

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {area.baseMipLevel, area.levelCount, area.baseArrayLayer, area.layerCount});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(
    vkDev.vkCmdClearDepthStencilImage(frameCore, image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
}

void ExecutionContext::beginCustomStage(const char *why)
{
  startNode(ExecutionNode::CUSTOM);

  bool insertMarker =
    back.executionState.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::CUSTOM;

  back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::CUSTOM);
  applyStateChanges();

  if (insertMarker)
    insertEvent(why, 0xFFFF00FF);
}

void ExecutionContext::clearView(int what)
{
  startNode(ExecutionNode::RP);
  ensureActivePass();
  getFramebufferState().clearMode = what;
  back.executionState.interruptRenderPass("ClearView");
  back.executionState.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
  applyStateChanges();
  invalidateActiveGraphicsPipeline();

  getFramebufferState().clearMode = 0;
}

void ExecutionContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  beginCustomStage("clearColorImage");

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {area.baseMipLevel, area.levelCount, area.baseArrayLayer, area.layerCount});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(vkDev.vkCmdClearColorImage(frameCore, image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
}

void ExecutionContext::copyBufferToImageOrdered(Image *dst, Buffer *src, const VkBufferImageCopy &region)
{
  if (!dst->isResident())
    return;

  const auto sz =
    dst->getFormat().calculateImageSize(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth, 1);
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src, {region.bufferOffset, sz});
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    {region.imageSubresource.mipLevel, 1, region.imageSubresource.baseArrayLayer, region.imageSubresource.layerCount});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(
    vkDev.vkCmdCopyBufferToImage(frameCore, src->getHandle(), dst->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
}

void ExecutionContext::copyImage(Image *src, Image *dst, uint32_t src_mip, uint32_t dst_mip, uint32_t mip_count, uint32_t region_count,
  uint32_t first_region)
{
  if (!src->isResident())
    return;
  if (!dst->isResident())
    return;

#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < region_count; ++i)
  {
    const VkImageCopy &region = *(data.imageCopyInfos.data() + first_region + i);

    // check that aspects are correct
    G_ASSERTF((region.srcSubresource.aspectMask & region.dstSubresource.aspectMask),
      "vulkan: copyImage %s <- %s, aspectMask does not match", dst->getDebugName(), src->getDebugName());

    G_ASSERTF((src->getFormat().getAspektFlags() & region.srcSubresource.aspectMask),
      "vulkan: copyImage %s <- %s, aspectMask does not match src format", dst->getDebugName(), src->getDebugName());

    G_ASSERTF((dst->getFormat().getAspektFlags() & region.dstSubresource.aspectMask),
      "vulkan: copyImage %s <- %s, aspectMask does not match dst format", dst->getDebugName(), src->getDebugName());

    // check that we are not copying from garbadge
    for (int j = 0; j < region.srcSubresource.layerCount; ++j)
      for (int k = 0; k < mip_count; ++k)
        G_ASSERTF(src->layout.get(src_mip + k, region.srcSubresource.baseArrayLayer + j) != VK_IMAGE_LAYOUT_UNDEFINED,
          "vulkan: copyImage %s <- %s layer: %u mip: %u, src uninitialized", dst->getDebugName(), src->getDebugName(),
          region.srcSubresource.baseArrayLayer + j, src_mip + k);
  }
#endif

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {src_mip, mip_count, 0, src->getArrayLayers()});
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {dst_mip, mip_count, 0, dst->getArrayLayers()});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(vkDev.vkCmdCopyImage(frameCore, src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count, data.imageCopyInfos.data() + first_region));

  if (dst->isSampledSRV())
    dst->layout.roSealTargetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // we can't know is it readed or not, so assume readed
  if (dst->isUsedInBindless())
  {
    trackBindlessRead(dst);
    back.syncTrack.completeNeeded(frameCore, vkDev);
  }
}

void ExecutionContext::blitImage(Image *src, Image *dst, const VkImageBlit &region)
{
  G_ASSERTF(src != dst, "Don't blit image on it self, if you need to build mipmaps, use the "
                        "dedicated functions for it!");
  ValueRange<uint8_t> srcMipRange(region.srcSubresource.mipLevel, region.srcSubresource.mipLevel + 1);
  ValueRange<uint8_t> dstMipRange(region.dstSubresource.mipLevel, region.dstSubresource.mipLevel + 1);
  ValueRange<uint16_t> srcArrayRange(region.srcSubresource.baseArrayLayer,
    region.srcSubresource.baseArrayLayer + region.srcSubresource.layerCount);
  ValueRange<uint16_t> dstArrayRange(region.dstSubresource.baseArrayLayer,
    region.dstSubresource.baseArrayLayer + region.dstSubresource.layerCount);

  auto srcI = src;
  auto dstI = dst;

  if (!srcI)
    srcI = getSwapchainColorImage();
  if (!dstI)
    dstI = getSwapchainColorImage();

  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, srcI,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    {region.srcSubresource.mipLevel, 1, region.srcSubresource.baseArrayLayer, region.srcSubresource.layerCount});
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dstI,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    {region.dstSubresource.mipLevel, 1, region.dstSubresource.baseArrayLayer, region.dstSubresource.layerCount});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  VULKAN_LOG_CALL(vkDev.vkCmdBlitImage(frameCore, srcI->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstI->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR));
}

void ExecutionContext::resetQuery(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count)
{
  // TODO: put into a list and do the reset at frame work item start
  VULKAN_LOG_CALL(vkDev.vkCmdResetQueryPool(frameCore, pool, index, count));
}

void ExecutionContext::copyQueryResult(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count, Buffer *dst,
  VkQueryResultFlags flags)
{
  const uint32_t stride = sizeof(uint32_t);
  uint32_t ofs = dst->dataOffset(index * stride);
  uint32_t sz = stride * count;

  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst, {ofs, sz});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  // TODO: put this at the end of the work item
  VULKAN_LOG_CALL(vkDev.vkCmdCopyQueryPoolResults(frameCore, pool, index, count, dst->getHandle(), ofs, sz, flags));
}

void ExecutionContext::generateMipmaps(Image *img)
{
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, img,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, img->getArrayLayers()});
  back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {1, 1, 0, img->getArrayLayers()});
  back.syncTrack.completeNeeded(frameCore, vkDev);

  // blit mip 0 into mip 1
  VkImageBlit blit;
  blit.srcSubresource.aspectMask = img->getFormat().getAspektFlags();
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = img->getArrayLayers();
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1].x = img->getBaseExtent().width;
  blit.srcOffsets[1].y = img->getBaseExtent().height;
  blit.srcOffsets[1].z = 1;
  blit.dstSubresource.aspectMask = img->getFormat().getAspektFlags();
  blit.dstSubresource.mipLevel = 1;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = img->getArrayLayers();
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  auto mipExtent = img->getMipExtents2D(1);
  blit.dstOffsets[1].x = mipExtent.width;
  blit.dstOffsets[1].y = mipExtent.height;
  blit.dstOffsets[1].z = 1;

  VULKAN_LOG_CALL(vkDev.vkCmdBlitImage(frameCore, img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR));

  if (img->getMipLevels() > 2)
  {
    auto lastMipExtent = mipExtent;
    for (auto &&m : img->getMipLevelRange().front(2))
    {
      back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {(uint32_t)(m - 1), 1, 0, img->getArrayLayers()});
      back.syncTrack.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {(uint32_t)m, 1, 0, img->getArrayLayers()});
      back.syncTrack.completeNeeded(frameCore, vkDev);

      // blit mip M-1 into mip M
      blit.srcSubresource.mipLevel = m - 1;
      blit.srcOffsets[1].x = lastMipExtent.width;
      blit.srcOffsets[1].y = lastMipExtent.height;

      auto mipExtent = img->getMipExtents2D(m);
      blit.dstSubresource.mipLevel = m;
      blit.dstOffsets[1].x = mipExtent.width;
      blit.dstOffsets[1].y = mipExtent.height;
      VULKAN_LOG_CALL(vkDev.vkCmdBlitImage(frameCore, img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->getHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR));

      lastMipExtent = mipExtent;
    }
  }
}

void ExecutionContext::bindVertexUserData(BufferSubAllocation bsa)
{
  back.executionState.set<StateFieldGraphicsVertexBuffersBindArray, BufferSubAllocation, BackGraphicsState>(bsa);
  back.executionState.apply(*this);

  // reset to user-provided bindings on next state apply
  back.pipelineState.set<StateFieldGraphicsVertexBuffers, uint32_t, FrontGraphicsState>(0);
}

void ExecutionContext::drawIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  if (!renderAllowed)
    return;

  trackIndirectArgAccesses(buffer, offset, count, stride);

  if (count > 1 && !device.hasMultiDrawIndirect())
  {
    // emulate multi draw indirect
    for (uint32_t i = 0; i < count; ++i)
    {
      VULKAN_LOG_CALL(vkDev.vkCmdDrawIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset), 1, stride));
      writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
      offset += stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(vkDev.vkCmdDrawIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset), count, stride));
    writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  }
}

void ExecutionContext::drawIndexedIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  if (!renderAllowed)
    return;

  trackIndirectArgAccesses(buffer, offset, count, stride);

  if (count > 1 && !device.hasMultiDrawIndirect())
  {
    // emulate multi draw indirect
    for (uint32_t i = 0; i < count; ++i)
    {
      VULKAN_LOG_CALL(vkDev.vkCmdDrawIndexedIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset), 1, stride));
      writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
      offset += stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(vkDev.vkCmdDrawIndexedIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset), count, stride));
    writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  }
}

void ExecutionContext::draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance)
{
  if (!renderAllowed)
    return;

  VULKAN_LOG_CALL(vkDev.vkCmdDraw(frameCore, count, instance_count, start, first_instance));
  writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void ExecutionContext::drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base,
  uint32_t first_instance)
{
  if (!renderAllowed)
    return;

  VULKAN_LOG_CALL(vkDev.vkCmdDrawIndexed(frameCore, count, instance_count, index_start, vertex_base, first_instance));
  writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void ExecutionContext::bindIndexUser(BufferSubAllocation bsa)
{
  vkDev.vkCmdBindIndexBuffer(frameCore, bsa.buffer->getHandle(), bsa.buffer->offset(bsa.offset), VK_INDEX_TYPE_UINT16);
  // set state dirty to restore after this intervention
  back.executionState.set<StateFieldGraphicsIndexBuffer, uint32_t, BackGraphicsState>(1);
}

void ExecutionContext::flushGrahpicsState(VkPrimitiveTopology top)
{
  if (back.executionState.set<StateFieldGraphicsPrimitiveTopology, VkPrimitiveTopology, BackGraphicsState>(top))
    invalidateActiveGraphicsPipeline();

  back.executionState.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(1);
  applyStateChanges();
}

void ExecutionContext::flushComputeState()
{
  back.executionState.set<StateFieldComputeFlush, uint32_t, BackComputeState>(0);
  applyStateChanges();
}

void ExecutionContext::ensureActivePass() { back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::GRAPHICS); }

void ExecutionContext::changeSwapchainMode(const SwapchainMode &new_mode, ThreadedFence *fence)
{
  flushAndWait(nullptr);
  swapchain.changeSwapchainMode(*this, new_mode);
  fence->setSignaledExternally();
}

void ExecutionContext::shutdownSwapchain() { swapchain.shutdown(back.contextState.frame.get()); }

void ExecutionContext::registerFrameEventsCallback(FrameEvents *callback)
{
  callback->beginFrameEvent();
  back.frameEventCallbacks.push_back(callback);
}

void ExecutionContext::getWorkerCpuCore(int *core, int *thread_id)
{
#if _TARGET_ANDROID
  *core = sched_getcpu();
  *thread_id = gettid();
#endif
  G_UNUSED(core);
  G_UNUSED(thread_id);
}

void ExecutionContext::setSwappyTargetRate(int rate) { swapchain.setSwappyTargetFrameRate(rate); }

void ExecutionContext::getSwappyStatus(int *status) { swapchain.getSwappyStatus(status); }

bool ExecutionContext::interruptFrameCore()
{
  // can't interrupt native pass
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();
  if (inPassState == InPassStateFieldType::NATIVE_PASS)
    return false;

  // apply any pending changes & finish all scopes
  back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::CUSTOM);
  applyStateChanges();

  switchFrameCore();
  onFrameCoreReset();

  return true;
}

void ExecutionContext::onFrameCoreReset()
{
  back.contextState.onFrameStateInvalidate();
  back.pipelineState.makeDirty();
  back.executionState.reset();
  back.executionState.makeDirty();
}

void ExecutionContext::startNode(ExecutionNode node)
{
  // workaround for adreno drivers crash on viewport/scissor setted in command buffer before cs dispatch
  if ((node == ExecutionNode::CS) &&
      (back.executionState.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::COMPUTE) &&
      device.adrenoGraphicsViewportConflictWithCS())
  {
    // start new command buffer on graphics->cs stage change
    // otherwise driver will crash if viewport/scissor was set in command buffer before
    // and there was no draw or no render pass and no global memory barrier
    interruptFrameCore();
  }
}

void ExecutionContext::trackStageResAccesses(const spirv::ShaderHeader &header, ShaderStage stageIndex)
{
  PipelineStageStateBase &state = back.contextState.stageState[stageIndex];
  if (uint32_t applyBits = ~state.tRegisterValidMask & header.tRegisterUseMask)
    for (uint8_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
    {
      if ((applyBits & (1 << i)) == 0)
        continue;

      const TRegister &reg = state.tRegisters[i];
      switch (reg.type)
      {
        case TRegister::TYPE_IMG:
        {
          bool roDepth = state.isConstDepthStencil.test(i);
          VkImageLayout srvLayout =
            roDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          back.syncTrack.addImageAccess(ExecutionSyncTracker::LogicAddress::forImageOnExecStage(stageIndex, RegisterType::T),
            reg.img.ptr, srvLayout,
            {reg.img.view.getMipBase(), reg.img.view.getMipCount(), reg.img.view.getArrayBase(), reg.img.view.getArrayCount()});
        }
        break;
        case TRegister::TYPE_BUF:
          back.syncTrack.addBufferAccess(ExecutionSyncTracker::LogicAddress::forBufferOnExecStage(stageIndex, RegisterType::T),
            reg.buf.buffer, {reg.buf.offset(0), reg.buf.dataSize()});
          break;
#if D3D_HAS_RAY_TRACING
        case TRegister::TYPE_AS:
          back.syncTrack.addAccelerationStructureAccess(
            ExecutionSyncTracker::LogicAddress::forAccelerationStructureOnExecStage(stageIndex, RegisterType::T), reg.rtas);
          break;
#endif
        default: break;
      }
    }

  if (uint32_t applyBits = ~state.uRegisterValidMask & header.uRegisterUseMask)
    for (uint8_t i = 0; i < spirv::U_REGISTER_INDEX_MAX; ++i)
    {
      if ((applyBits & (1 << i)) == 0)
        continue;
      const URegister &reg = state.uRegisters[i];
      if (reg.image)
      {
        back.syncTrack.addImageAccess(ExecutionSyncTracker::LogicAddress::forImageOnExecStage(stageIndex, RegisterType::U), reg.image,
          VK_IMAGE_LAYOUT_GENERAL,
          {reg.imageView.getMipBase(), reg.imageView.getMipCount(), reg.imageView.getArrayBase(), reg.imageView.getArrayCount()});
      }
      else if (reg.buffer.buffer)
      {
        back.syncTrack.addBufferAccess(ExecutionSyncTracker::LogicAddress::forBufferOnExecStage(stageIndex, RegisterType::U),
          reg.buffer.buffer, {reg.buffer.offset(0), reg.buffer.dataSize()});
      }
    }

  if (uint32_t applyBits = ~state.bRegisterValidMask & header.bRegisterUseMask)
    for (uint8_t i = 0; i < spirv::B_REGISTER_INDEX_MAX; ++i)
    {
      if ((applyBits & (1 << i)) == 0)
        continue;

      const BufferRef &reg = state.bRegisters[i];
      if (reg.buffer)
        back.syncTrack.addBufferAccess(ExecutionSyncTracker::LogicAddress::forBufferOnExecStage(stageIndex, RegisterType::B),
          reg.buffer, {reg.offset(0), reg.dataSize()});
    }
}

void ExecutionContext::trackIndirectArgAccesses(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  back.syncTrack.addBufferAccess({VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}, buffer.buffer,
    {buffer.offset(offset), stride * count});
}

void ExecutionContext::trackBindlessRead(Image *img)
{
  VkImageLayout srvLayout = img->getUsage() & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                              ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                              : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  back.syncTrack.addImageAccess(ExecutionSyncTracker::LogicAddress::forImageBindlessRead(), img, srvLayout,
    {0, img->getMipLevels(), 0, img->getArrayLayers()});
}
