#include "device.h"
#include <validation.h>
#if _TARGET_ANDROID
#include <unistd.h>
#endif

using namespace drv3d_vulkan;

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
thread_local ExecutionContext *ExecutionContext::tlsDbgActiveInstance = nullptr;
#endif

static uint32_t same_state_range(Image *img, uint32_t layer, ValueRange<uint32_t> mip_range)
{
  const auto base = img->getState(mip_range[0], layer);
  for (uint32_t i = 1; i < mip_range.length(); ++i)
    if (img->getState(mip_range[i], layer) != base)
      return i;
  return mip_range.length();
}

void ExecutionContext::writeExectionChekpoint(VulkanCommandBufferHandle cb, VkPipelineStageFlagBits stage)
{
  if (!device.getFeatures().test(DeviceFeaturesConfig::COMMAND_MARKER))
    return;

  device.execMarkers.write(cb, stage, data.id, cmd);
}

void ExecutionContext::fixFrameImageState(Image *img, ImageState new_state, ValueRange<uint8_t> mip_range,
  ValueRange<uint16_t> array_range)
{
  if (img->isSealed())
  {
    if (SN_SAMPLED_FROM == new_state)
      return;
    img->unseal("updateImageState", new_state);
  }

  // 3d image has no array range, but image view uses it to select the slices, so reset array range.
  if (VK_IMAGE_TYPE_3D == img->getType())
    array_range.reset(0, 1);

  for (auto layerIndex : array_range)
  {
    auto workingRange = mip_range;
    while (workingRange.length() > 0)
    {
      const auto span = same_state_range(img, layerIndex, workingRange);
      const Image::State old_state = static_cast<Image::State>(img->getState(workingRange[0], layerIndex));
      if (old_state != new_state)
      {
        for (uint32_t i = 0; i < span; ++i)
          img->setState(workingRange[i], layerIndex, new_state);
      }

      workingRange = workingRange.front(span);
    }
  }
}

bool ExecutionContext::updateImageState(Image *img, ImageState new_state, ValueRange<uint8_t> mip_range,
  ValueRange<uint16_t> array_range)
{
  if (img->isSealed())
  {
    if (SN_SAMPLED_FROM == new_state)
      return false;
    img->unseal("updateImageState", new_state);
  }

  PrimaryPipelineBarrier barrier(vkDev);
  barrier.modifyImageTemplate(img);

  // 3d image has no array range, but image view uses it to select the slices, so reset array range.
  if (VK_IMAGE_TYPE_3D == img->getType())
    array_range.reset(0, 1);

  bool subresWasUninitialized = false;

  for (auto layerIndex : array_range)
  {
    auto workingRange = mip_range;
    while (workingRange.length() > 0)
    {
      const auto span = same_state_range(img, layerIndex, workingRange);
      const Image::State old_state = static_cast<Image::State>(img->getState(workingRange[0], layerIndex));

      subresWasUninitialized |= old_state == SN_INITIAL;
      if (old_state != new_state)
      {
        barrier.modifyImageTemplate(workingRange[0], span, layerIndex, 1);
        barrier.addImageByTemplate(old_state, new_state);

        for (uint32_t i = 0; i < span; ++i)
          img->setState(workingRange[i], layerIndex, new_state);
      }

      workingRange = workingRange.front(span);
    }
  }

  if (barrier.empty())
    return false;

  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();
  if (inPassState != InPassStateFieldType::NONE)
  {
    // will be handled at ensureBoundImageStates with stub replacement
    if (subresWasUninitialized)
      return true;
      // not usefull on rel builds for our case
#if DAGOR_DBGLEVEL > 0
    generateFaultReport();
#endif
    debug("vulkan: offending barrier dump");
    barrier.dbgPrint();
    fatal("vulkan: image barrier <%s>->%u must be outside of renderpass, caller %s", img->getDebugName(), new_state,
      getCurrentCmdCaller());
  }
  barrier.submit(frameCore);
  return true;
}

bool ExecutionContext::setFrameImageState(Image *img, ImageState new_state, ValueRange<uint8_t> mip_range,
  ValueRange<uint16_t> array_range)
{
  // maybe some tracking will be added here, so left this method as is
  return updateImageState(img, new_state, mip_range, array_range);
}

void ExecutionContext::imageBarrier(Image *img, BarrierImageType image_type, ResourceBarrier state, uint32_t res_index,
  uint32_t res_range)
{
  ImageState dst_state = SN_INITIAL;

  // ImageStates are mutually exclusive and can't express all the combinations could come from ResourceBarrier
  // (e.g. combination of RB_RO_* usages). For now the most common usecases are prioritized during the mapping
  // to ImageState and the others are silently ignored. (e.g. RB_RO_SRB -> SN_SAMPLED_FROM)

  if (state & RB_RO_SRV && !(state & RB_RW_DEPTH_STENCIL_TARGET))
  {
    dst_state = ImageState::SN_SAMPLED_FROM;
  }
  else if (state & RB_RW_UAV)
  {
    dst_state = ImageState::SN_SHADER_WRITE;
  }
  else if (state & (RB_RW_COPY_DEST | RB_RW_BLIT_DEST))
  {
    dst_state = ImageState::SN_COPY_TO;
  }
  else if (state & (RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE))
  {
    dst_state = ImageState::SN_COPY_FROM;
  }
  else
  {
    return;
  }

  beginCustomStage("ImageBarrier");

  if (image_type == BarrierImageType::SwapchainColor)
    img = getSwapchainColorImage();
  else if (image_type == BarrierImageType::SwapchainDepth)
    img = getSwapchainPrimaryDepthStencilImage();

  uint32_t stop_index = (res_range == 0) ? img->getMipLevels() * img->getArrayLayers() : res_range + res_index;

  G_ASSERTF((img->getMipLevels() * img->getArrayLayers()) >= stop_index,
    "Out of bound subresource index range (%d, %d+%d) in resource barrier for image %s (0, %d)", res_index, res_index, res_range,
    img->getDebugName(), img->getMipLevels() * img->getArrayLayers());

  // TODO:
  // can be optimized by calculating 3 multiple subresource range
  for (uint32_t i = res_index; i < stop_index; i++)
  {
    uint32_t mip_level = i % img->getMipLevels();
    uint32_t layer = i / img->getMipLevels();

    setFrameImageState(img, dst_state, make_value_range(mip_level, 1), make_value_range(layer, 1));
  }
}

void ExecutionContext::prepare()
{
  if (is_null(frameCore))
  {
    frameCore = back.contextState.frame->allocateCommandBuffer(vkDev);
    VkCommandBufferBeginInfo cbbi;
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext = nullptr;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cbbi.pInheritanceInfo = nullptr;
    VULKAN_EXIT_ON_FAIL(vkDev.vkBeginCommandBuffer(frameCore, &cbbi));
  }
}

void ExecutionContext::flushUnorderedImageColorClears()
{
  for (const CmdClearColorTexture &t : data.unorderedImageColorClears)
    clearColorImage(t.image, t.area, t.value, t.followupUsage);
}

void ExecutionContext::flushUnorderedImageDepthStencilClears()
{
  for (const CmdClearDepthStencilTexture &t : data.unorderedImageDepthStencilClears)
    clearDepthStencilImage(t.image, t.area, t.value);
}

void ExecutionContext::flushUnorderedImageCopies()
{
  for (CmdCopyImage &i : data.unorderedImageCopies)
    copyImage(i.src, i.dst, i.srcMip, i.dstMip, i.mipCount, i.regionCount, i.regionIndex);
}

void ExecutionContext::prepareFrameCore()
{
  prepare();
  back.contextState.bindlessManagerBackend.advance();
  flushUnorderedImageColorClears();
  flushUnorderedImageDepthStencilClears();
  flushImageUploads();
  flushUnorderedImageCopies();
  // should be after upload, to not overwrite data
  processFillEmptySubresources();
}

void ExecutionContext::processFillEmptySubresources()
{
  if (data.imagesToFillEmptySubresources.empty())
    return;

  Buffer *zeroBuf = nullptr;
  Tab<VkBufferImageCopy> copies;

  for (Image *i : data.imagesToFillEmptySubresources)
  {
    if (!i->anySubresInState(SN_INITIAL))
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
        if (i->getState(mip, arr) == SN_INITIAL)
        {
          // better to ensure that image are properly filled with data
          // but even better is to avoid crashing due to wrong barriers caused by such textures
          logwarn("vulkan: image %p:<%s> uninitialized subres %u:%u filled with zero contents", i, i->getDebugName(), arr, mip);
          setFrameImageState(i, SN_COPY_TO, make_value_range(mip, 1), make_value_range(arr, 1));
          copies.push_back(i->makeBufferCopyInfo(mip, arr, zeroBuf->dataOffset(0)));
        }

    VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, zeroBuf->getHandle(), i->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      copies.size(), copies.data()));

    // transit to sealed sampled from for speed
    setFrameImageState(i, SN_SAMPLED_FROM, mipRange, arrRange);
    i->seal();
    copies.clear();
  }

  if (zeroBuf)
    back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*zeroBuf);
}

void ExecutionContext::ensureBoundImageStates(const spirv::ShaderHeader &header, ShaderStage stageIndex)
{
  InPassStateFieldType inPassState = back.executionState.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();
  PipelineStageStateBase &state = back.contextState.stageState[stageIndex];
  uint32_t tRegisterMask = header.tRegisterUseMask;
  for (uint32_t i = 0; tRegisterMask && i < spirv::T_REGISTER_INDEX_MAX; ++i, tRegisterMask >>= 1)
  {
    if ((tRegisterMask & 1) && state.tRegisters[i].type == TRegister::TYPE_IMG)
    {
      ImageState imgState = state.isConstDepthStencil.test(i) ? SN_CONST_RENDER_TARGET : SN_SAMPLED_FROM;
      if (setFrameImageState(state.tRegisters[i].img.ptr, imgState, state.tRegisters[i].img.view.getMipRange(),
            state.tRegisters[i].img.view.getArrayRange()))
      {
        if (inPassState != InPassStateFieldType::NONE)
          state.setTempty(i);
      }
    }
  }

  uint32_t uRegisterMask = header.uRegisterUseMask;
  for (uint32_t i = 0; uRegisterMask && i < spirv::U_REGISTER_INDEX_MAX; ++i, uRegisterMask >>= 1)
  {
    if ((uRegisterMask & 1) && state.uRegisters[i].image)
    {
      if (setFrameImageState(state.uRegisters[i].image, SN_SHADER_WRITE, state.uRegisters[i].imageView.getMipRange(),
            state.uRegisters[i].imageView.getArrayRange()))
      {
        if (inPassState != InPassStateFieldType::NONE)
          state.setUempty(i);
      }
    }
  }
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

  PrimaryPipelineBarrier imageBarrier(vkDev);

  // first flush image writes and move them into copy from state
  for (auto &&download : data.imageDownloads)
  {
    if (!download.image)
    {
      download.image = getSwapchainColorImage();
      download.img = download.image->getHandle();
    }

    if (!download.image->isResident())
      continue;

    imageBarrier.modifyImageTemplate(download.image);

    for (auto iter = begin(data.imageDownloadCopies) + download.copyIndex,
              ed = begin(data.imageDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      // do for each array layer its own barrier, this could be merged, but for now leave it as is
      for (uint32_t i = 0; i < iter->imageSubresource.layerCount; ++i)
      {
        uint32_t layer = iter->imageSubresource.baseArrayLayer + i;
        Image::State oldState = (ImageState)download.image->getState(iter->imageSubresource.mipLevel, layer);

        G_ASSERTF(oldState != ImageState::SN_INITIAL, "vulkan: can't read from uninitialized texture %s",
          download.image->getDebugName());

        imageBarrier.modifyImageTemplate(iter->imageSubresource.mipLevel, 1, layer, 1);
        imageBarrier.addImageByTemplate(oldState, ImageState::SN_COPY_FROM);
      }
    }
  }

  imageBarrier.submit(cmd_b, true);

  SecondaryPipelineBarrier bufferBarrier(vkDev, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
  bufferBarrier.modifyBufferTemplate({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT});

  for (auto &&download : data.imageDownloads)
  {
    if (!download.image->isResident())
      continue;
    // do the copy
    VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(cmd_b, download.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, download.buf,
      download.copyCount, data.imageDownloadCopies.data() + download.copyIndex));

    bufferBarrier.modifyBufferTemplate(download.buf);

    // build up the barriers to flush the writes to the buffer and make them visible to the host
    for (auto iter = begin(data.imageDownloadCopies) + download.copyIndex,
              ed = begin(data.imageDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      bufferBarrier.addBufferByTemplate(iter->bufferOffset,
        download.image->getFormat().calculateImageSize(iter->imageExtent.width, iter->imageExtent.height, iter->imageExtent.depth, 1) *
          iter->imageSubresource.layerCount);
    }
  }

  bufferBarrier.submit(cmd_b);


  // this patches the image barrier definitions to revert the image state back to pre copy and make
  // them ready for use next frame
  imageBarrier.revertImageBarriers({VK_ACCESS_TRANSFER_READ_BIT, 0}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  imageBarrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  imageBarrier.submit(cmd_b);

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

  // flush gpu writes to the src buffers

  PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.modifyBufferTemplate({VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT});

  for (auto &&download : data.bufferDownloads)
  {
    barrier.modifyBufferTemplate(download.src);
    for (auto iter = begin(data.bufferDownloadCopies) + download.copyIndex,
              ed = begin(data.bufferDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
      barrier.addBufferByTemplate(iter->srcOffset, iter->size);
  }

  // wait for all stages until we can copy
  barrier.submit(cmd_b);

  // issue copies
  for (auto &&download : data.bufferDownloads)
  {
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(cmd_b, download.src, download.dst, download.copyCount,
      data.bufferDownloadCopies.data() + download.copyIndex));
  }

  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
  barrier.modifyBufferTemplate({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT});

  // flush copy and make it visible to host
  for (auto &&download : data.bufferDownloads)
  {
    barrier.modifyBufferTemplate(download.dst);
    for (auto iter = begin(data.bufferDownloadCopies) + download.copyIndex,
              ed = begin(data.bufferDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
      barrier.addBufferByTemplate(iter->dstOffset, iter->size);
  }

  // only wait on copy stage
  barrier.submit(cmd_b);
  return cmd_b;
}

void ExecutionContext::flushImageUploads()
{
  if (data.imageUploads.empty())
    return;

  // internal deduplicate logic uses assumes that not checked/honored by callers
  // we can upload multiple times to same subres using imageOffset/imageExtent
  // and we can use different buffers with different offsets too
  // causing very complex search loop that should be later on expanded back to make proper barriers
  // so just don't use it for now and upload one by one
  // ImageCopyInfo::deduplicate(data.imageUploads, data.imageUploadCopies);

  // TODO:
  // 1. sort by image
  // 2. add image-image barrier when copy ops overlap same subresource
  //(it may be ok to have overlap on subresource number but not in subresource, need to test!)
  // 3. tune deduplicate so it does not fail in this conditions
  // 4. make execution this way so it calls vk method only when situation with overlap are hitted
  // or on "micro" stage ends

  // TODO: in light of deduplication stuff, it maybe worth checking that upload element is valid
  // it does not make sense to generate duplicated areas in upload element, so we are somewhat safe
  // to use approach below (with 3 stages)

  // barrier to sync copy to same image inside loop properly (copy 2 copy barrier)
  SecondaryPipelineBarrier c2cBarrier(vkDev);
  c2cBarrier.modifyImageTemplate({VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // perform copy ops with additional sync if needed
  for (auto &&upload : data.imageUploads)
  {
    c2cBarrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      c2cBarrier.modifyImageTemplate(upload.image);
      if (!setFrameImageState(upload.image, SN_COPY_TO, make_value_range(iter->imageSubresource.mipLevel, 1),
            make_value_range(iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount)))
      {
        c2cBarrier.modifyImageTemplate(iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer,
          iter->imageSubresource.layerCount);
        c2cBarrier.addImageByTemplate();
      }
    }

    if (!c2cBarrier.empty())
      c2cBarrier.submit(frameCore);

    // actual copy commands
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, upload.buf, upload.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      upload.copyCount, data.imageUploadCopies.data() + upload.copyIndex));
  }

  // transit to asked target
  for (auto &&upload : data.imageUploads)
  {
    // seal it here, to optimize state tracking
    if (upload.targetState == SN_SAMPLED_FROM)
    {
      setFrameImageState(upload.image, SN_SAMPLED_FROM, upload.image->getMipLevelRange(), upload.image->getArrayLayerRange());
      upload.image->seal();
      continue;
    }

    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      setFrameImageState(upload.image, upload.targetState, make_value_range(iter->imageSubresource.mipLevel, 1),
        make_value_range(iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount));
    }
  }
}

VulkanCommandBufferHandle ExecutionContext::flushOrderedBufferUploads(VulkanCommandBufferHandle cmd_b)
{
  if (data.orderedBufferUploads.empty())
    return cmd_b;

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  PrimaryPipelineBarrier barrier(vkDev);

  // writes from unordered uploads
  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.addMemory({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT});
  barrier.submit(cmd_b);

  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.modifyBufferTemplate({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT});

  // barrier every non unique buffer or every 16 uniques, to not burn CPU
  const int CACHE_SIZE = 16;
  StaticTab<VulkanBufferHandle, CACHE_SIZE> scratchBuffer;

  for (auto &&upload : data.orderedBufferUploads)
  {
    bool unique = true;
    for (VulkanBufferHandle i : scratchBuffer)
    {
      if (i == upload.dst)
      {
        unique = false;
        break;
      }
    }
    if (unique)
    {
      if (scratchBuffer.size() == scratchBuffer.capacity())
      {
        scratchBuffer.clear();
        unique = false;
      }
      scratchBuffer.push_back(upload.dst);
    }

    if (!unique)
    {
      barrier.submit(cmd_b);
      barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      barrier.modifyBufferTemplate({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT});
    }

    // sadly no way to batch those together...
    VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(cmd_b, upload.src, upload.dst, upload.copyCount,
      data.orderedBufferUploadCopies.data() + upload.copyIndex));

    barrier.modifyBufferTemplate(upload.dst);
    // batch all barriers together and submit all with one barrier call
    for (auto iter = begin(data.orderedBufferUploadCopies) + upload.copyIndex,
              ed = begin(data.orderedBufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
      // this barrier makes copy writes to dst visible to any reads
      barrier.addBufferByTemplate(iter->dstOffset, iter->size);
  }

  if (!barrier.empty())
    barrier.submit(cmd_b);

  // followup reads

  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  barrier.addMemory({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT});
  barrier.submit(cmd_b);

  return cmd_b;
}
VulkanCommandBufferHandle ExecutionContext::flushBufferUploads(VulkanCommandBufferHandle cmd_b)
{
  if (data.bufferUploads.empty())
    return cmd_b;

  if (device.getFeatures().test(DeviceFeaturesConfig::OPTIMIZE_BUFFER_UPLOADS))
    BufferCopyInfo::optimizeBufferCopies(data.bufferUploads, data.bufferUploadCopies);

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  // NOTE: no host write flush barriers are needed as submitting command
  // buffers to a queue do a implicit host write flush
  PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  barrier.modifyBufferTemplate({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT});

  for (auto &&upload : data.bufferUploads)
  {
    // sadly no way to batch those together...
    VULKAN_LOG_CALL(
      vkDev.vkCmdCopyBuffer(cmd_b, upload.src, upload.dst, upload.copyCount, data.bufferUploadCopies.data() + upload.copyIndex));

    barrier.modifyBufferTemplate(upload.dst);
    // batch all barriers together and submit all with one barrier call
    for (auto iter = begin(data.bufferUploadCopies) + upload.copyIndex,
              ed = begin(data.bufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
      // this barrier makes copy writes to dst visible to any reads
      barrier.addBufferByTemplate(iter->dstOffset, iter->size);
  }

  barrier.submit(cmd_b);

  return cmd_b;
}

VulkanCommandBufferHandle ExecutionContext::flushBufferToHostFlushes(VulkanCommandBufferHandle cmd_b)
{
  if (data.bufferToHostFlushes.empty())
    return cmd_b;

  if (is_null(cmd_b))
    cmd_b = allocAndBeginCommandBuffer();

  PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT);
  barrier.modifyBufferTemplate({VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_HOST_READ_BIT});

  for (auto &&flush : data.bufferToHostFlushes)
  {
    // which stage did the write was not tracked
    // assume any could be it
    // (could be narrowed down to shader write
    // or copy op)
    barrier.modifyBufferTemplate(flush.buffer);
    barrier.addBufferByTemplate(flush.offset, flush.range);
  }

  barrier.submit(cmd_b);
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
  preFrame = flushBufferUploads(preFrame);
  preFrame = flushOrderedBufferUploads(preFrame);

  // append post frame core into it, no need to chunk this up

  auto postFrame = flushBufferDownloads(frameCore);
  postFrame = flushImageDownloads(postFrame);
  postFrame = flushBufferToHostFlushes(postFrame);

  VkCommandBuffer submits[2];
  uint32_t submitCount = 0;

  if (!is_null(preFrame))
  {
    VULKAN_EXIT_ON_FAIL(vkDev.vkEndCommandBuffer(preFrame));
    submits[submitCount++] = preFrame;
  }
  if (!is_null(postFrame))
  {
    VULKAN_EXIT_ON_FAIL(vkDev.vkEndCommandBuffer(postFrame));
    submits[submitCount++] = postFrame;
  }

  if (fence || submitCount)
  {
    DeviceQueue::TrimmedSubmitInfo si = {};
    si.pCommandBuffers = submits;
    si.commandBufferCount = submitCount;

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
  }

  back.contextState.onFrameStateInvalidate();
  back.pipelineState.makeDirty();
  back.executionState.reset();
  back.executionState.makeDirty();

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

void ExecutionContext::copyImageToBuffer(VulkanImageHandle src, VkImageLayout src_layout, VulkanBufferHandle dst,
  uint32_t region_count, const VkBufferImageCopy *regions)
{
  beginCustomStage("copyImageToBuffer");
  VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(frameCore, src, src_layout, dst, region_count, regions));
}

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
  VkBuildAccelerationStructureFlagsKHR flags, VkBool32 update, VulkanAccelerationStructureKHR dst, VulkanAccelerationStructureKHR src,
  Buffer *scratch)
{
  if (is_null(src) && update)
    src = dst;

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

    buildInfo.dstAccelerationStructure = dst;
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

    buildInfo.dstAccelerationStructure = dst;
    buildInfo.scratchData.deviceAddress = scratch->getDeviceAddress(vkDev);

    const VkAccelerationStructureBuildRangeInfoKHR *build_ranges = &data.raytraceBuildRangeInfoKHRStore[first_geometry];
    VULKAN_LOG_CALL(vkDev.vkCmdBuildAccelerationStructuresKHR(frameCore, 1, &buildInfo, &build_ranges));
  }

  PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
  barrier.addMemory({VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT});
  barrier.submit(frameCore);
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

void ExecutionContext::resolveFrambufferImage(uint32_t color_index, Image *dst)
{
  auto src = getFramebufferState().frontendFrameBufferInfo.colorAttachments[color_index].image;
#if !_TARGET_PC_WIN // for pc-win we just rely on stack unwinding on crash
  if (!src)
  {
    logerr("vulkan.resolveFrambufferImage: color_index=%d src=%p dst=%p (%s)", color_index, src, dst, dst ? dst->getDebugName() : "");
    return;
  }
#endif

  if (dst != src)
  {
    G_ASSERT(src->getType() == VK_IMAGE_TYPE_2D && dst->getType() == VK_IMAGE_TYPE_3D);
    G_ASSERT(src->getMipLevels() == 1);
    // for missing render to voltex emulation
    if (src->getType() == VK_IMAGE_TYPE_2D && dst->getType() == VK_IMAGE_TYPE_3D)
    {
      setFrameImageState(src, SN_COPY_FROM, src->getMipLevelRange(), src->getArrayLayerRange());
      setFrameImageState(dst, SN_COPY_TO, dst->getMipLevelRange(), dst->getArrayLayerRange());

      G_ASSERT(src->getArrayLayers() == dst->getBaseExtent().depth);
      size_t imageDataSize =
        src->getFormat().calculateImageSize(src->getBaseExtent().width, src->getBaseExtent().height, 1, src->getMipLevels());
      imageDataSize *= min<uint32_t>(MAX_MIPMAPS, src->getArrayLayers());
      // FIXME: possible deadlock (!) due to access to context from execution
      Buffer *convBuffer = device.getArrayImageTo3DBuffer(imageDataSize);

      StaticTab<VkBufferImageCopy, MAX_MIPMAPS> bics;
      StaticTab<VkBufferImageCopy, MAX_MIPMAPS> bicd;

      PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      barrier.addBuffer({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT}, convBuffer, convBuffer->offset(0), imageDataSize);

      size_t copyOffset = 0;
      for (auto &&m : src->getMipLevelRange())
      {
        uint32_t layersToCopy = max<uint32_t>(1, src->getArrayLayers() >> m);
        for (uint32_t l = 0; l < layersToCopy; ++l)
        {
          if (bics.size() == bics.capacity())
          {
            VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(frameCore, src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              convBuffer->getHandle(), bics.capacity(), bics.data()));

            barrier.submit(frameCore, true);

            VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, convBuffer->getHandle(), dst->getHandle(),
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bicd.capacity(), bicd.data()));

            bics.clear();
            bicd.clear();
            copyOffset = 0;
          }

          VkBufferImageCopy &copySliceToBuffer = bics.push_back();
          copySliceToBuffer.bufferOffset = copyOffset;
          copySliceToBuffer.bufferRowLength = 0;
          copySliceToBuffer.bufferImageHeight = 0;
          copySliceToBuffer.imageSubresource.aspectMask = src->getFormat().getAspektFlags();
          copySliceToBuffer.imageSubresource.mipLevel = m;
          copySliceToBuffer.imageSubresource.baseArrayLayer = l;
          copySliceToBuffer.imageSubresource.layerCount = 1;
          copySliceToBuffer.imageOffset.x = 0;
          copySliceToBuffer.imageOffset.y = 0;
          copySliceToBuffer.imageOffset.z = 0;
          copySliceToBuffer.imageExtent = src->getMipExtents(m);

          VkBufferImageCopy &copyBufferToSlice = bicd.push_back();
          copyBufferToSlice.bufferOffset = copyOffset;
          copyBufferToSlice.bufferRowLength = 0;
          copyBufferToSlice.bufferImageHeight = 0;
          copyBufferToSlice.imageSubresource.aspectMask = copySliceToBuffer.imageSubresource.aspectMask;
          copyBufferToSlice.imageSubresource.mipLevel = m;
          copyBufferToSlice.imageSubresource.baseArrayLayer = 0;
          copyBufferToSlice.imageSubresource.layerCount = 1;
          copyBufferToSlice.imageOffset.x = 0;
          copyBufferToSlice.imageOffset.y = 0;
          copyBufferToSlice.imageOffset.z = l;
          copyBufferToSlice.imageExtent = copySliceToBuffer.imageExtent;

          copyOffset +=
            src->getFormat().calculateImageSize(copySliceToBuffer.imageExtent.width, copySliceToBuffer.imageExtent.height, 1, 1);
        }
      }

      if (!bics.empty())
      {
        VULKAN_LOG_CALL(vkDev.vkCmdCopyImageToBuffer(frameCore, src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          convBuffer->getHandle(), bics.size(), bics.data()));

        barrier.submit(frameCore);

        VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(frameCore, convBuffer->getHandle(), dst->getHandle(),
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bicd.size(), bicd.data()));
      }
    }
  }

  if (dst->getMipLevels() > 1)
    generateMipmaps(dst);
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

  setFrameImageState(src, SN_COPY_FROM, {0, 1}, {0, 1});
  setFrameImageState(effectiveDst, SN_COPY_TO, {0, 1}, {0, 1});

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

  swapchain.prePresent(frameCore, frame);
  flush(frame.frameDone);
  doFrameEndCallbacks();

  {
    FrameTimingWatch watch(back.presentWaitDuration);
    swapchain.present(frame);
  }

  device.timelineMan.get<TimelineManager::GpuExecute>().advance();
  back.contextState.frame.restart();

  {
    FrameTimingWatch watch(back.acquireBackBufferDuration);
    swapchain.onFrameBegin(back.contextState.frame.get());
  }

  if (data.generateFaultReport)
    generateFaultReport();
}

void ExecutionContext::captureScreen(Buffer *buffer)
{
  auto colorImage = getSwapchainColorImage();

  PrimaryPipelineBarrier barrier(vkDev);
  barrier.modifyImageTemplate(0, 1, 0, 1);

  // first flush image writes and move it into copy from state
  barrier.modifyImageTemplate(colorImage);
  barrier.addImageByTemplate((ImageState)colorImage->getState(0, 0), ImageState::SN_COPY_FROM);
  barrier.submit(frameCore, true);

  // do the copy to buffer
  auto extent = colorImage->getMipExtents2D(0);
  VkBufferImageCopy copy{};
  copy.bufferOffset = buffer->dataOffset(0);
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.layerCount = 1;
  copy.imageExtent.width = extent.width;
  copy.imageExtent.height = extent.height;
  copy.imageExtent.depth = 1;
  copyImageToBuffer(colorImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->getHandle(), 1, &copy);

  const auto copySize = colorImage->getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, 1, 1);

  SecondaryPipelineBarrier bufBarrier(vkDev, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
  bufBarrier.addBuffer({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT}, buffer, copy.bufferOffset, copySize);
  bufBarrier.submit(frameCore);

  // this patches the image barrier definition to revert the image state ctx.back to pre copy
  barrier.revertImageBarriers({VK_ACCESS_TRANSFER_READ_BIT, 0}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  barrier.submit(frameCore);
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
#if DAGOR_DBGLEVEL > 0
  if (device.getFeatures().test(DeviceFeaturesConfig::STOP_PASS_EVERY_DRAW_DISPATCH) && !isInMultisampledFBPass())
    back.executionState.interruptRenderPass("perDrawDispatchPassEnd");
#endif
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
  flushComputeState();

  VULKAN_LOG_CALL(vkDev.vkCmdDispatchIndirect(frameCore, buffer.getHandle(), buffer.dataOffset(offset)));
  writeExectionChekpoint(frameCore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void ExecutionContext::copyBuffer(VulkanBufferHandle src, VulkanBufferHandle dst, uint32_t src_offset, uint32_t dst_offset,
  uint32_t size)
{
  PipelineBarrier::AccessFlags anyRW2TransferRead = {
    VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT};
  PipelineBarrier::AccessFlags anyRW2TransferWrite = {
    VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};

  PrimaryPipelineBarrier barrier(vkDev);
  barrier.addStages(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.addBuffer(anyRW2TransferRead, src, src_offset, size);
  barrier.addBuffer(anyRW2TransferWrite, dst, dst_offset, size);
  barrier.submit(frameCore);

  VkBufferCopy bc;
  bc.srcOffset = src_offset;
  bc.dstOffset = dst_offset;
  bc.size = size;
  VULKAN_LOG_CALL(vkDev.vkCmdCopyBuffer(frameCore, src, dst, 1, &bc));

  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  barrier.addBuffer(anyRW2TransferRead.swap(), src, src_offset, size);
  barrier.addBuffer(anyRW2TransferWrite.swap(), dst, dst_offset, size);
  barrier.submit(frameCore);
}

void ExecutionContext::fillBuffer(VulkanBufferHandle buffer, uint32_t offset, uint32_t size, uint32_t value)
{
  PrimaryPipelineBarrier barrier(vkDev);
  barrier.addStages(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.addBuffer({VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buffer, offset, size);
  barrier.submit(frameCore);

  VULKAN_LOG_CALL(vkDev.vkCmdFillBuffer(frameCore, buffer, offset, size, value));

  barrier.addStages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  barrier.addBuffer({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT}, buffer, offset, size);
  barrier.submit(frameCore);
}

void ExecutionContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  // TODO flush_clear_state is only needed if the image is a cleared attachment
  beginCustomStage("clearDepthStencilImage");
  ValueRange<uint8_t> mipRange(area.baseMipLevel, area.baseMipLevel + area.levelCount);
  ValueRange<uint16_t> arrayRange(area.baseArrayLayer, area.baseArrayLayer + area.layerCount);
  setFrameImageState(image, SN_WRITE_CLEAR, mipRange, arrayRange);
  VULKAN_LOG_CALL(
    vkDev.vkCmdClearDepthStencilImage(frameCore, image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
  if (image->getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
    setFrameImageState(image, SN_SAMPLED_FROM, mipRange, arrayRange);
}

void ExecutionContext::beginCustomStage(const char *why)
{
  bool insertMarker =
    back.executionState.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::CUSTOM;

  back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::CUSTOM);
  applyStateChanges();

  if (insertMarker)
    insertEvent(why, 0xFFFF00FF);
}

void ExecutionContext::clearView(int what)
{
  ensureActivePass();
  getFramebufferState().clearMode = what;
  back.executionState.interruptRenderPass("ClearView");
  back.executionState.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
  applyStateChanges();
  invalidateActiveGraphicsPipeline();
}

void ExecutionContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value,
  ImageState followup_usage /* = SN_SAMPLED_FROM*/)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);
  // avoid skipping sync as same sate == no sync!
  G_ASSERT(followup_usage != SN_WRITE_CLEAR);

  beginCustomStage("clearColorImage");
  ValueRange<uint8_t> mipRange(area.baseMipLevel, area.baseMipLevel + area.levelCount);
  ValueRange<uint16_t> arrayRange(area.baseArrayLayer, area.baseArrayLayer + area.layerCount);
  setFrameImageState(image, SN_WRITE_CLEAR, mipRange, arrayRange);
  VULKAN_LOG_CALL(vkDev.vkCmdClearColorImage(frameCore, image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
  setFrameImageState(image, followup_usage, mipRange, arrayRange);
}

void ExecutionContext::copyBufferToImageOrdered(Image *dst, VulkanBufferHandle src, const VkBufferImageCopy &region)
{
  if (!dst->isResident())
    return;

  VulkanCommandBufferHandle cmd_b = frameCore;

  ValueRange<uint8_t> dstMipRange(region.imageSubresource.mipLevel, region.imageSubresource.mipLevel + 1);
  setFrameImageState(dst, SN_COPY_TO, dstMipRange, dst->getArrayLayerRange());

  SecondaryPipelineBarrier barrier(vkDev);
  barrier.addStages(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  barrier.submit(cmd_b);

  VULKAN_LOG_CALL(vkDev.vkCmdCopyBufferToImage(cmd_b, src, dst->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

  if (dst->getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
    setFrameImageState(dst, SN_SAMPLED_FROM, dstMipRange, dst->getArrayLayerRange());
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
        G_ASSERTF(src->getState(src_mip + k, region.srcSubresource.baseArrayLayer + j) != SN_INITIAL,
          "vulkan: copyImage %s <- %s layer: %u mip: %u, src uninitialized", dst->getDebugName(), src->getDebugName(),
          region.srcSubresource.baseArrayLayer + j, src_mip + k);
  }
#endif

  ValueRange<uint8_t> srcMipRange(src_mip, src_mip + mip_count);
  ValueRange<uint8_t> dstMipRange(dst_mip, dst_mip + mip_count);
  setFrameImageState(src, SN_COPY_FROM, srcMipRange, src->getArrayLayerRange());
  setFrameImageState(dst, SN_COPY_TO, dstMipRange, dst->getArrayLayerRange());

  VULKAN_LOG_CALL(vkDev.vkCmdCopyImage(frameCore, src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count, data.imageCopyInfos.data() + first_region));

  // promote dst to sampled state if possible
  // avoiding extra before-pass barrier & write-write hazard for multiple copy commands
  if (dst->getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
    setFrameImageState(dst, SN_SAMPLED_FROM, dstMipRange, dst->getArrayLayerRange());
  // same for src
  if (src->getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
    setFrameImageState(src, SN_SAMPLED_FROM, srcMipRange, src->getArrayLayerRange());
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

  setFrameImageState(srcI, SN_COPY_FROM, srcMipRange, srcArrayRange);
  setFrameImageState(dstI, SN_COPY_TO, dstMipRange, dstArrayRange);

  VULKAN_LOG_CALL(vkDev.vkCmdBlitImage(frameCore, srcI->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstI->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR));

  // avoid write-write hazards it we blit to same image in sequence
  setFrameImageState(dstI, SN_SAMPLED_FROM, dstMipRange, dstArrayRange);
}

void ExecutionContext::resetQuery(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count)
{
  // TODO: put into a list and do the reset at frame work item start
  VULKAN_LOG_CALL(vkDev.vkCmdResetQueryPool(frameCore, pool, index, count));
}

void ExecutionContext::copyQueryResult(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count, VulkanBufferHandle buffer,
  uint32_t offset, uint32_t stride, VkQueryResultFlags flags)
{
  // TODO: put this at the end of the work item
  VULKAN_LOG_CALL(vkDev.vkCmdCopyQueryPoolResults(frameCore, pool, index, count, buffer, offset, stride, flags));

#if VK_EXT_conditional_rendering
  PrimaryPipelineBarrier barrier(vkDev, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT);
  barrier.addBuffer({VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT}, buffer, offset, count * stride);
  barrier.submit(frameCore);
#endif
}

void ExecutionContext::generateMipmaps(Image *img)
{
  setFrameImageState(img, Image::State::SN_COPY_FROM, {0, 1}, img->getArrayLayerRange());
  setFrameImageState(img, Image::State::SN_COPY_TO, img->getMipLevelRange().front(1), img->getArrayLayerRange());

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
      updateImageState(img, Image::State::SN_COPY_FROM, ValueRange<uint8_t>(m - 1, m), img->getArrayLayerRange());

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

  updateImageState(img, Image::State::SN_SAMPLED_FROM, img->getMipLevelRange(), img->getArrayLayerRange());
}

VulkanCommandBufferHandle ExecutionContext::getFrameCmdBuffer() { return frameCore; }

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
  vkDev.vkCmdBindIndexBuffer(getFrameCmdBuffer(), bsa.buffer->getHandle(), bsa.buffer->offset(bsa.offset), VK_INDEX_TYPE_UINT16);
  // set state dirty to restore after this intervention
  back.executionState.set<StateFieldGraphicsIndexBuffer, uint32_t, BackGraphicsState>(1);
}

void ExecutionContext::flushGrahpicsState(VkPrimitiveTopology top)
{
  if (back.executionState.set<StateFieldGraphicsPrimitiveTopology, VkPrimitiveTopology, BackGraphicsState>(top))
    invalidateActiveGraphicsPipeline();

  // workaround for adreno drivers crash on viewport/scissor setted in command buffer before cs dispatch
  // apply view/scissor again if we back from non draw apply
  if (!back.executionState.getRO<StateFieldGraphicsFlush, bool, BackGraphicsState>())
  {
    back.executionState.set<StateFieldGraphicsViewport, StateFieldGraphicsViewport::MakeDirty, BackGraphicsState>({});
    back.executionState
      .set<StateFieldGraphicsScissor, StateFieldGraphicsScissor::MakeDirty, BackGraphicsState, BackDynamicGraphicsState>({});
  }

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
  swapchain.changeSwapchainMode(back.contextState.frame.get(), new_mode);
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