// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <math/dag_lsbVisitor.h>
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "sampler_cache.h"
#include "execution_markers.h"
#include "device_queue.h"
#include "device_memory_report.h"
#include "backend.h"
#include "execution_scratch.h"
#include "execution_timings.h"
#include "execution_context.h"
#include "bindless.h"
#include "execution_sync.h"
#include "stacked_profile_events.h"
#include "predicted_latency_waiter.h"
#include "texture.h"
#include <EASTL/sort.h>
#include "wrapped_command_buffer.h"
#include "execution_sync_capture.h"

using namespace drv3d_vulkan;

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
thread_local ExecutionContext *ExecutionContext::tlsDbgActiveInstance = nullptr;
#endif

ExecutionScratch ExecutionContext::scratch;

ExecutionContext::ExecutionContext(RenderWork &work_item) : data(work_item), swapchain(Globals::swapchain), vkDev(Globals::VK::dev)
{
  Backend::State::exec.setExecutionContext(this);
#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  tlsDbgActiveInstance = this;
#endif
}

ExecutionContext::~ExecutionContext()
{
  G_ASSERT(flushProcessed);
  Backend::State::exec.setExecutionContext(nullptr);
#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  tlsDbgActiveInstance = nullptr;
#endif
}

void ExecutionContext::beginCmd(const void *ptr) { cmd = ptr; }

void ExecutionContext::endCmd() { ++cmdIndex; }

FramebufferState &ExecutionContext::getFramebufferState()
{
  return Backend::State::exec.get<BackGraphicsState, BackGraphicsState>().framebufferState;
}

uint32_t ExecutionContext::beginVertexUserData(uint32_t stride)
{
  using Bind = StateFieldGraphicsVertexBufferStride;
  uint32_t oldStride = Backend::State::pipe.get<StateFieldGraphicsVertexBufferStrides, Bind, FrontGraphicsState>().data;
  if (Backend::State::pipe.set<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({0, stride}))
    invalidateActiveGraphicsPipeline();

  return oldStride;
}

void ExecutionContext::endVertexUserData(uint32_t stride)
{
  using Bind = StateFieldGraphicsVertexBufferStride;
  if (Backend::State::pipe.set<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({0, stride}))
    invalidateActiveGraphicsPipeline();
}

void ExecutionContext::invalidateActiveGraphicsPipeline()
{
  Backend::State::exec.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

void ExecutionContext::writeExectionChekpointNonCommandStream(VkPipelineStageFlagBits stage, uint32_t key)
{
  Backend::gpuJob.get().execTracker.addMarker(&key, sizeof(key));

  if (!Globals::cfg.bits.commandMarkers)
    return;

  uintptr_t ptrKey = key;
  Globals::gpuExecMarkers.write(stage, data.id, (void *)ptrKey);
}

void ExecutionContext::writeExectionChekpoint(VkPipelineStageFlagBits stage)
{
  if (!Globals::cfg.bits.commandMarkers)
    return;

  Globals::gpuExecMarkers.write(stage, data.id, cmd);
}

// barrier is not always describing DST OP
// while we can convert it only to DST OP for following sync step
// and DST OP is not always well convertible by pure bit-scan like logic
// and in general user slection of this RB bits are quite error prone (at least from vulkan POV)
// so do precise barriers handling for now

bool d3d_resource_barrier_to_layout(VkImageLayout &layout, ResourceBarrier barrier)
{
  switch ((int)barrier)
  {
    case RB_STAGE_ALL_SHADERS | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_PIXEL | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_VERTEX | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_COMPUTE | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    default: return false;
  }
  return true;
}

bool d3d_resource_barrier_to_laddr(LogicAddress &laddr, ResourceBarrier barrier)
{
  switch ((int)barrier)
  {
    case RB_STAGE_ALL_SHADERS | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
      break;
    case RB_STAGE_PIXEL | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_STAGE_VERTEX | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      break;
    case RB_STAGE_COMPUTE | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      break;
    case RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_RW_UAV | RB_STAGE_ALL_SHADERS:
      laddr.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
      break;
    default: return false;
  }
  return true;
}

void ExecutionContext::bufferBarrier(const BufferRef &b_ref, ResourceBarrier d3d_barrier)
{
  // tracking on non bindless resources is precise enough, handle only bindless case
  if (!b_ref.buffer->isUsedInBindless())
    return;
  beginCustomStage("bufferBarrier");

  LogicAddress laddr;
  if (!d3d_resource_barrier_to_laddr(laddr, d3d_barrier))
  {
    D3D_ERROR("vulkan: unhandled barrier %u for buf %p:%s caller %s", d3d_barrier, b_ref.buffer, b_ref.buffer->getDebugName(),
      getCurrentCmdCaller());
    return;
  }

  Backend::sync.addBufferAccess(laddr, b_ref.buffer, {b_ref.bufOffset(0), b_ref.visibleDataSize});
}

void ExecutionContext::imageBarrier(Image *img, ResourceBarrier d3d_barrier, uint32_t res_index, uint32_t res_range)
{
  // tracking on non bindless resources is precise enough, handle only bindless case
  if (!img->isUsedInBindless())
    return;
  beginCustomStage("imageBarrier");

  LogicAddress laddr;
  VkImageLayout layout;

  if (!(d3d_resource_barrier_to_layout(layout, d3d_barrier) && d3d_resource_barrier_to_laddr(laddr, d3d_barrier)))
  {
    D3D_ERROR("vulkan: unhandled barrier %u for img %p:%s, range %u-%u caller %s", d3d_barrier, img, img->getDebugName(), res_index,
      res_range, getCurrentCmdCaller());
    return;
  }

  if (res_range == 0)
    Backend::sync.addImageAccess(laddr, img, layout, {0, img->getMipLevels(), 0, img->getArrayLayers()});
  else if (img->getArrayLayers() == 1)
    Backend::sync.addImageAccess(laddr, img, layout, {res_index, res_range, 0, 1});
  else if (img->getMipLevels() == 1)
    Backend::sync.addImageAccess(laddr, img, layout, {0, 1, res_index, res_range});
  else
  {
    for (uint32_t i = res_index; i < res_range + res_index; i++)
    {
      uint32_t mip_level = i % img->getMipLevels();
      uint32_t layer = i / img->getMipLevels();

      Backend::sync.addImageAccess(laddr, img, layout, {mip_level, 1, layer, 1});
    }
  }
}

void ExecutionContext::waitForUploadOnCurrentBuffer()
{
  size_t queueBit = 1 << (uint32_t)frameCoreQueue;
  if (queueBit & uploadQueueWaitMask)
    return;

  if (transferUploadBuffer)
    addQueueDep(transferUploadBuffer, scratch.cmdListsToSubmit.size());
  if (graphicsUploadBuffer)
    addQueueDep(graphicsUploadBuffer, scratch.cmdListsToSubmit.size());

  uploadQueueWaitMask |= queueBit;
}

void ExecutionContext::waitUserQueueSignal(int idx, DeviceQueueType target_queue)
{
  ExecutionScratch::UserQueueSignal &usignal = scratch.userQueueSignals[idx];
  // already waited for this signal on target queue
  if (usignal.waitedOnQueuesMask & (1 << (uint32_t)target_queue))
    return;
  addQueueDep(scratch.userQueueSignals[idx].bufferIdx, lastBufferIdxOnQueue[(uint32_t)target_queue]);
  // mark waited
  usignal.waitedOnQueuesMask |= (1 << (uint32_t)target_queue);
}

void ExecutionContext::recordUserQueueSignal(int idx, DeviceQueueType target_queue)
{
  G_UNUSED(idx);
  G_ASSERTF(scratch.userQueueSignals.size() == idx, "vulkan: back to front queue signal sequencing broken expected %u, got %u",
    scratch.userQueueSignals.size(), idx);
  scratch.userQueueSignals.push_back({lastBufferIdxOnQueue[(uint32_t)target_queue], 0});
}

void ExecutionContext::switchFrameCoreForQueueChange(DeviceQueueType target_queue)
{
  // override any queue to graphics if multi queue is not enabled
  if (!Globals::cfg.bits.allowMultiQueue)
    target_queue = DeviceQueueType::GRAPHICS;
  // switch buffer only if queue changes
  if (frameCoreQueue == target_queue)
    return;

  // reorder should not cross queue boundaries, should be guarantied via buffer interruption, but that's weak
  // so ensure everything is ok via assert
  Backend::cb.verifyReorderEmpty();

  completeSyncForQueueChange();
  switchFrameCore(target_queue);
  waitForUploadOnCurrentBuffer();
}

void ExecutionContext::switchFrameCore(DeviceQueueType target_queue)
{
  // avoid unclosed ranges to not break markers spreading cross list/queue/submit
  // finish them and start anew after allocating new command buffer
  finishDebugEventRanges();

  // note: first switch will push empty buffer, that is used to replace later on for preFrame
  scratch.cmdListsToSubmit.push_back({frameCoreQueue, frameCore});
  frameCoreQueue = target_queue;
  allocFrameCore();
  lastBufferIdxOnQueue[(uint32_t)target_queue] = scratch.cmdListsToSubmit.size();

  restoreDebugEventRanges();
}

void ExecutionContext::allocFrameCore()
{
  frameCore = Backend::gpuJob->allocateCommandBuffer(frameCoreQueue);
  VkCommandBufferBeginInfo cbbi;
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.pNext = nullptr;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cbbi.pInheritanceInfo = nullptr;
  VULKAN_EXIT_ON_FAIL(vkDev.vkBeginCommandBuffer(frameCore, &cbbi));
  Backend::cb.set(frameCore);
}

void ExecutionContext::flushUnorderedImageColorClears()
{
  TIME_PROFILE(vulkan_flush_unordered_rt_clears);
  for (const CmdClearColorTexture &t : data.unorderedImageColorClears)
  {
    clearColorImage(t.image, t.area, t.value);
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_UNORDERED_COLOR_CLEAR);
  }
}

void ExecutionContext::flushUnorderedImageDepthStencilClears()
{
  TIME_PROFILE(vulkan_flush_unordered_ds_clears);
  for (const CmdClearDepthStencilTexture &t : data.unorderedImageDepthStencilClears)
  {
    clearDepthStencilImage(t.image, t.area, t.value);
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_UNORDERED_DEPTH_CLEAR);
  }
}

void ExecutionContext::flushUnorderedImageCopies()
{
  TIME_PROFILE(vulkan_flush_unordered_image_copies);

  for (CmdCopyImage &i : data.unorderedImageCopies)
  {
    copyImage(i.src, i.dst, i.srcMip, i.dstMip, i.mipCount, i.regionCount, i.regionIndex);
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_UNORDERED_IMAGE_COPY);
  }
}

void ExecutionContext::cleanupMemory()
{

  bool needMemoryCleanup = false;
  bool cleanupAllPossibleMemory = false;
  needMemoryCleanup |= RenderWork::cleanUpMemoryEveryWorkItem;
  if (Globals::Mem::res.isOutOfMemorySignalReceived())
  {
    needMemoryCleanup |= true;
    cleanupAllPossibleMemory |= true;
    debug("vulkan: processing OOM signal");
  }

  if (!needMemoryCleanup)
    return;

  const VkDeviceSize evictBlockSize = 10 << 20; // 10Mb

  TIME_PROFILE(vulkan_cleanup_memory);

  // wait for all outstanding accesses to finish
  finishAllGPUWorkItems();

  // prepare syscopy for resources that can be evicted
  {
    TIME_PROFILE(vulkan_make_syscopy);

    switchFrameCore(DeviceQueueType::GRAPHICS);
    {
      WinAutoLock lk(Globals::Mem::mutex);
      do
      {
        TIME_PROFILE(vulkan_make_syscopy_iter);
        cleanupAllPossibleMemory &=
          Globals::Mem::res.evictResourcesFor(*this, evictBlockSize, cleanupAllPossibleMemory /*evict_used*/);
      } while (cleanupAllPossibleMemory);
    }

    {
      TIME_PROFILE(vulkan_wait_syscopy);
      // complete syscopy filling
      flush(Backend::gpuJob.get().frameDone);
      Backend::gpuJob.end();
      finishAllGPUWorkItems();
      Backend::gpuJob.start();
    }
  }

  // remove vulkan objects of evicted resources and free up memory
  {
    TIME_PROFILE(vulkan_free_released_memory);
    WinAutoLock lk(Globals::Mem::mutex);
    Globals::Mem::res.processPendingEvictions();
    Globals::Mem::res.consumeOutOfMemorySignal();
  }

  // proceed to normal workload execution
  flushProcessed = false;
  frameCore = VulkanNullHandle();
}

void ExecutionContext::completeSyncForQueueChange()
{
  if (!Globals::cfg.bits.allowMultiQueue)
    return;

  Backend::sync.completeOnQueue(data.id);
}

void ExecutionContext::prepareFrameCore()
{
  if (size_t multiQueueOverride = Backend::interop.toggleMultiQueueSubmit.load(std::memory_order_relaxed))
    Globals::cfg.bits.allowMultiQueue = (multiQueueOverride % 2) == 0;

  G_ASSERTF(is_null(frameCore), "vulkan: execution context already prepared for command execution");
  TIME_PROFILE(vulkan_pre_frame_core);
  // not tied to command buffer
  Backend::bindless.advance();

  bool needGraphicsUpload = data.unorderedImageColorClears.size() || data.unorderedImageDepthStencilClears.size() ||
                            data.imageUploads.size() || data.unorderedImageCopies.size();
  bool needTransferUpload = data.bufferUploads.size() || data.orderedBufferUploads.size();

  if (needGraphicsUpload)
  {
    // can't live on transfer queue (or can't always live) and followup queue should be graphics after prepare
    switchFrameCoreForQueueChange(DeviceQueueType::GRAPHICS);
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_START);
    flushUnorderedImageColorClears();
    flushUnorderedImageDepthStencilClears();
    flushImageUploads();
    flushUnorderedImageCopies();

    graphicsUploadBuffer = scratch.cmdListsToSubmit.size();
  }

  if (needTransferUpload)
  {
    switchFrameCoreForQueueChange(DeviceQueueType::TRANSFER);
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_START_TRANSFER_UPLOAD);
    flushBufferUploads();
    flushOrderedBufferUploads();

    transferUploadBuffer = scratch.cmdListsToSubmit.size();
  }

  uploadQueueWaitMask = 0;
  switchFrameCoreForQueueChange(DeviceQueueType::GRAPHICS);

  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_TIMESTAMPS_RESET_QUEUE_READBACKS);
  data.timestampQueryBlock->ensureSizesAndResetStatus();

  // pre frame completion marker
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_END);

  // not tied to command buffer, but tied to layout changes order on CPU-worker timeline so do after all other uploads
  processBindlessUpdates();
}

void ExecutionContext::processBindlessUpdates()
{
  if (data.bindlessTexUpdates.size())
  {
    TIME_PROFILE(vulkan_bindless_tex_update);
    for (const BindlessTexUpdateInfo &i : data.bindlessTexUpdates)
    {
      if (i.img)
        trackBindlessRead(i.img);
      for (int j = 0; j < i.count; ++j)
        Backend::bindless.updateBindlessTexture(i.index + j, i.img, i.viewState);
    }
  }

  if (data.bindlessBufUpdates.size())
  {
    TIME_PROFILE(vulkan_bindless_buf_update);
    for (const BindlessBufUpdateInfo &i : data.bindlessBufUpdates)
    {
      for (int j = 0; j < i.count; ++j)
        Backend::bindless.updateBindlessBuffer(i.index + j, i.bref);
    }
  }

  if (data.bindlessSamplerUpdates.size())
  {
    TIME_PROFILE(vulkan_bindless_smp_update);
    for (const BindlessSamplerUpdateInfo &i : data.bindlessSamplerUpdates)
      Backend::bindless.updateBindlessSampler(i.index, Globals::samplers.get(i.sampler));
  }
}

void ExecutionContext::restoreImageResidencies()
{
  for (Image *img : scratch.imageResidenceRestores)
    img->delayedRestoreFromSysCopy();
  scratch.imageResidenceRestores.clear();
}

VulkanCommandBufferHandle ExecutionContext::allocAndBeginCommandBuffer(DeviceQueueType queue)
{
  FrameInfo &frame = Backend::gpuJob.get();
  VulkanCommandBufferHandle ncmd = frame.allocateCommandBuffer(queue);
  VkCommandBufferBeginInfo cbbi;
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.pNext = nullptr;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cbbi.pInheritanceInfo = nullptr;
  VULKAN_EXIT_ON_FAIL(vkDev.vkBeginCommandBuffer(ncmd, &cbbi));
  return ncmd;
}

void ExecutionContext::flushImageDownloads()
{
  if (data.imageDownloads.empty())
    return;

  for (auto &&download : data.imageDownloads)
  {
    verifyResident(download.image);
    verifyResident(download.buffer);

    for (auto iter = begin(data.imageDownloadCopies) + download.copyIndex,
              ed = begin(data.imageDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      uint32_t sz =
        download.image->getFormat().calculateImageSize(iter->imageExtent.width, iter->imageExtent.height, iter->imageExtent.depth, 1) *
        iter->imageSubresource.layerCount;
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, download.buffer,
        {iter->bufferOffset, sz});
      Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, download.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        {iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount});
    }
  }

  Backend::sync.completeNeeded();

  for (auto &&download : data.imageDownloads)
  {
    // do the copy
    VULKAN_LOG_CALL(Backend::cb.wCmdCopyImageToBuffer(download.image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      download.buffer->getHandle(), download.copyCount, data.imageDownloadCopies.data() + download.copyIndex));
  }
}

void ExecutionContext::flushBufferDownloads()
{
  if (data.bufferDownloads.empty())
    return;

  if (Globals::cfg.bits.optimizeBufferUploads)
    BufferCopyInfo::optimizeBufferCopies(data.bufferDownloads, data.bufferDownloadCopies);

  for (auto &&download : data.bufferDownloads)
  {
    verifyResident(download.src);
    verifyResident(download.dst);

    for (auto iter = begin(data.bufferDownloadCopies) + download.copyIndex,
              ed = begin(data.bufferDownloadCopies) + download.copyIndex + download.copyCount;
         iter != ed; ++iter)
    {
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, download.src,
        {iter->srcOffset, iter->size});
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, download.dst,
        {iter->dstOffset, iter->size});
    }
  }

  Backend::sync.completeNeeded();

  for (auto &&download : data.bufferDownloads)
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(download.src->getHandle(), download.dst->getHandle(), download.copyCount,
      data.bufferDownloadCopies.data() + download.copyIndex));
  }
}

void ExecutionContext::flushImageUploadsIter(uint32_t start, uint32_t end)
{
  Backend::sync.completeNeeded();
  for (uint32_t i = start; i < end; ++i)
  {
    ImageCopyInfo &upload = data.imageUploads[i];
    VULKAN_LOG_CALL(Backend::cb.wCmdCopyBufferToImage(upload.buffer->getHandle(), upload.image->getHandle(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, upload.copyCount, data.imageUploadCopies.data() + upload.copyIndex));
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_IMAGE_UPLOAD);
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
    verifyResident(upload.buffer);

    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      uint32_t sz =
        upload.image->getFormat().calculateImageSize(iter->imageExtent.width, iter->imageExtent.height, iter->imageExtent.depth, 1) *
        iter->imageSubresource.layerCount;
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.buffer,
        {iter->bufferOffset, sz});
    }
  }

  Backend::sync.completeNeeded();

  uint32_t mergedRangeStart = 0;
  for (uint32_t i = 0; i < data.imageUploads.size(); ++i)
  {
    ImageCopyInfo &upload = data.imageUploads[i];

    if (upload.image->hasLastSyncOpIndex())
    {
      flushImageUploadsIter(mergedRangeStart, i);
      mergedRangeStart = i;
    }

    verifyResident(upload.image);
    for (auto iter = begin(data.imageUploadCopies) + upload.copyIndex,
              ed = begin(data.imageUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        {iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer, iter->imageSubresource.layerCount});
    }
  }
  flushImageUploadsIter(mergedRangeStart, data.imageUploads.size());

  bool anyBindless = false;
  for (auto &&upload : data.imageUploads)
  {
    if (upload.image->isSampledSRV())
      upload.image->layout.roSealTargetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // we can't know is it readed or not, so assume readed
    if (upload.image->isUsedInBindless())
    {
      trackBindlessRead(upload.image);
      anyBindless = true;
    }
  }
  if (anyBindless)
    Backend::sync.completeNeeded();
}

void ExecutionContext::flushOrderedBufferUploads()
{
  if (data.orderedBufferUploads.empty())
    return;

  TIME_PROFILE(vulkan_flush_ordered_buffer_uploads);

  for (auto &&upload : data.orderedBufferUploads)
  {
    verifyResident(upload.src);
    verifyResident(upload.dst);

    // we can't ro seal SRC here as it may be used as write before, causing conflict
    for (auto iter = begin(data.orderedBufferUploadCopies) + upload.copyIndex,
              ed = begin(data.orderedBufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.src,
        {iter->srcOffset, iter->size});
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.dst,
        {iter->dstOffset, iter->size});
    }

    Backend::sync.completeNeeded();

    VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(upload.src->getHandle(), upload.dst->getHandle(), upload.copyCount,
      data.orderedBufferUploadCopies.data() + upload.copyIndex));
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_BUFFER_UPLOAD_ORDERED);
  }
}
void ExecutionContext::flushBufferUploads()
{
  if (data.bufferUploads.empty())
    return;

  TIME_PROFILE(vulkan_flush_buffer_uploads);

  if (Globals::cfg.bits.optimizeBufferUploads)
    BufferCopyInfo::optimizeBufferCopies(data.bufferUploads, data.bufferUploadCopies);

  for (auto &&upload : data.bufferUploads)
  {
    upload.src->optionallyActivateRoSeal(data.id);
    verifyResident(upload.src);
    verifyResident(upload.dst);

    for (auto iter = begin(data.bufferUploadCopies) + upload.copyIndex,
              ed = begin(data.bufferUploadCopies) + upload.copyIndex + upload.copyCount;
         iter != ed; ++iter)
    {
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, upload.src,
        {iter->srcOffset, iter->size});
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, upload.dst,
        {iter->dstOffset, iter->size});
    }
  }

  Backend::sync.completeNeeded();

  for (auto &&upload : data.bufferUploads)
  {
    // sadly no way to batch those together...
    VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(upload.src->getHandle(), upload.dst->getHandle(), upload.copyCount,
      data.bufferUploadCopies.data() + upload.copyIndex));
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_BUFFER_UPLOAD);
  }
}

void ExecutionContext::flushBufferToHostFlushes()
{
  if (data.bufferToHostFlushes.empty())
    return;

  for (auto &&flush : data.bufferToHostFlushes)
  {
    verifyResident(flush.buffer);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT}, flush.buffer, {flush.offset, flush.range});
  }
  Backend::sync.completeNeeded();
}

void ExecutionContext::flushPostFrameCommands()
{
  flushBufferDownloads();
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_DOWNLOAD_BUFFERS);
  flushImageDownloads();
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_DOWNLOAD_IMAGES);
  flushBufferToHostFlushes();
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_BUFFER_HOST_FLUSHES);

  Backend::sync.completeAll(data.id);
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_FRAME_END_SYNC);
}

void ExecutionContext::stackUpCommandBuffers()
{
  FrameInfo &frame = Backend::gpuJob.get();
  DeviceQueueType activeQueue = DeviceQueueType::INVALID;
  int32_t lastSubmitsOnQueues[(uint32_t)DeviceQueueType::COUNT] = {};
  for (int32_t &i : lastSubmitsOnQueues)
    i = -1;
  int32_t submitIdx = 0;
  for (ExecutionScratch::CommandBufferSubmit &i : scratch.cmdListsToSubmit)
  {
    // ignore empty buffers, that faciliate optional reordered commands
    if (is_null(i.handle))
      continue;
    // any queue change or signal/wait is candidate for separate queue submit
    // in some cases we can continue "old" submit to previously seen queue, to avoid useless waits
    if ((activeQueue != i.queue) || i.signals || i.waits || submitIdx < 0)
    {
      // if we don't wait on this buffer, we can append to last submit after switching queues
      if (i.waits == 0 && lastSubmitsOnQueues[(uint32_t)i.queue] >= 0)
      {
        submitIdx = lastSubmitsOnQueues[(uint32_t)i.queue];
        // patch data for dep logic
        scratch.submitGraph[submitIdx].originalSignalId = (uint32_t)(&i - scratch.cmdListsToSubmit.begin());
      }
      else
      {
        submitIdx = scratch.submitGraph.size();
        lastSubmitsOnQueues[(uint32_t)i.queue] = submitIdx;
        scratch.submitGraph.push_back({{}, {}, {}, 0, i.queue, (uint32_t)(&i - scratch.cmdListsToSubmit.begin()),
          (uint32_t)(&i - scratch.cmdListsToSubmit.begin()), false});
      }
    }
    activeQueue = i.queue;
    scratch.submitGraph[submitIdx].cbs.push_back(i.handle);
    // if queue submit signals something, we can't add more buffers after that
    // because it will break following waits on other queues (will happen too late)
    G_ASSERTF(scratch.submitGraph[submitIdx].signalsCount == 0,
      "vulkan: queue submit item should contain zero signals on append to it!");
    scratch.submitGraph[submitIdx].signalsCount += i.signals;
    // if we signal, we can't append followup buffers
    if (i.signals)
    {
      lastSubmitsOnQueues[(uint32_t)i.queue] = -1;
      submitIdx = -1;
    }
  }

  // last submit should happen with fence, due to overall restrictions of driver
  // completed fence = completed anything for user perspective
  scratch.submitGraph.back().fenceWait = true;

  // fill signals buffer ahead of time
  for (ExecutionScratch::QueueSubmitItem &i : scratch.submitGraph)
    for (int j = 0; j < i.signalsCount; ++j)
      i.signals.push_back(frame.allocSemaphore(Globals::VK::dev));
}

void ExecutionContext::sortAndCountDependencies()
{
  eastl::sort(scratch.cmdListsSubmitDeps.begin(), scratch.cmdListsSubmitDeps.end(),
    [](const ExecutionScratch::CommandBufferSubmitDeps &l, const ExecutionScratch::CommandBufferSubmitDeps &r) //
    { return l.from < r.from; });

  for (ExecutionScratch::CommandBufferSubmitDeps &i : scratch.cmdListsSubmitDeps)
  {
    ++scratch.cmdListsToSubmit[i.from].signals;
    ++scratch.cmdListsToSubmit[i.to].waits;
  }
}

// next gpu job should start after prev gpu job last work item
// track and put semaphores for all active queues on this frame to guarantie that
// otherwise gpu jobs can overlap
struct FrameEndQueueJoin
{
  eastl::bitset<(uint32_t)DeviceQueueType::COUNT> activeQueueList;
  uint32_t frameJoinPoints = 0;
  DeviceQueueType fenceWaitQueue = DeviceQueueType::INVALID;

  void markActiveQueue(DeviceQueueType queue) { activeQueueList.set(static_cast<uint32_t>(queue)); }

  void addSignals(ExecutionScratch::QueueSubmitItem &target)
  {
    G_ASSERTF(frameJoinPoints == 0, "vulkan: frame joint points already populated, should not happen twice");
    FrameInfo &frame = Backend::gpuJob.get();
    frameJoinPoints = activeQueueList.count();
    for (uint32_t i = 0; i < frameJoinPoints; ++i)
      target.signals.push_back(frame.allocSemaphore(Globals::VK::dev));
    fenceWaitQueue = target.queue;
  }

  void addWaits(ExecutionScratch::QueueSubmitItem &target)
  {
    if (frameJoinPoints)
    {
      // check for double wait or mismatch somewhere
      G_ASSERTF(frameJoinPoints == target.signals.size(),
        "vulkan: mismatched signals size %u vs %u, should be equal to added one in add signals", frameJoinPoints,
        target.signals.size());
      for (uint32_t i : LsbVisitor{activeQueueList.to_uint32()})
      {
        G_ASSERTF(!target.signals.empty(), "vulkan: frame queue join signaled more than it supposed to");
        Globals::VK::queue[static_cast<DeviceQueueType>(i)].addSubmitSemaphore(target.signals.back(),
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        target.signals.pop_back();
      }
    }
  }
};

void ExecutionContext::enqueueCommandListsToMultipleQueues(ThreadedFence *fence)
{
  TIME_PROFILE(vulkan_queue_submits);
  FrameInfo &frame = Backend::gpuJob.get();

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
  for (ExecutionScratch::CommandBufferSubmitDeps &i : scratch.cmdListsSubmitDeps)
    Backend::syncCapture.bufferLinks.push_back(
      {(uint32_t)scratch.cmdListsToSubmit[i.from].queue, i.from, (uint32_t)scratch.cmdListsToSubmit[i.to].queue, i.to});
#endif

  // sum up waits and signals for each buffer submit and sort dependencies to reduce complexity of submit logic
  sortAndCountDependencies();
  // generate queue submits, consisting of continous block of command lists
  // while following dependencies
  stackUpCommandBuffers();

  FrameEndQueueJoin queueJoin;
  ExecutionScratch::CommandBufferSubmitDeps *depPtr = scratch.cmdListsSubmitDeps.begin();

  // NOTE: this logic will fail if earlier submit wants sync to later submit, hopefully triggering some of assert checks
  for (ExecutionScratch::QueueSubmitItem &i : scratch.submitGraph)
  {
    queueJoin.markActiveQueue(i.queue);
    if (fence && i.fenceWait)
    {
      queueJoin.addSignals(i);
      if (!is_null(presentSignal))
        i.signals.push_back(presentSignal);
    }

    DeviceQueue::TrimmedSubmitInfo si = {};
    si.pCommandBuffers = ary(i.cbs.data());
    si.commandBufferCount = i.cbs.size();
    si.pSignalSemaphores = ary(i.signals.data());
    si.signalSemaphoreCount = i.signals.size();

    // add submit semaphores from buffer because wait can happen after other submits on same queue
    for (VulkanSemaphoreHandle j : i.submitSemaphores)
      Globals::VK::queue[i.queue].addSubmitSemaphore(j, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    if (fence && i.fenceWait)
    {
      Globals::VK::queue[i.queue].submit(vkDev, frame, si, fence->get());
      if (!is_null(presentSignal))
        i.signals.pop_back();
      fence->setAsSubmited();
    }
    else
      Globals::VK::queue[i.queue].submit(vkDev, frame, si);

    ExecutionScratch::CommandBufferSubmitDeps *depRangeRestart = nullptr;
    // add submit semaphores for related dst deps, using dynamic loop ranges
    while (depPtr < scratch.cmdListsSubmitDeps.end())
    {
      if (depPtr->from != i.originalSignalId)
      {
        if (!depRangeRestart)
          depRangeRestart = depPtr;
        ++depPtr;
        continue;
      }
      ExecutionScratch::QueueSubmitItem *k = &i;
      ++k;
      while (k < scratch.submitGraph.end())
      {
        if (k->originalWaitId != depPtr->to)
        {
          ++k;
          continue;
        }
        k->submitSemaphores.push_back(i.signals.back());
        i.signals.pop_back();
        if (i.signals.empty())
          break;
        ++k;
      }
      ++depPtr;
    }
    // submits merged over queue switch, restart search range as it can "skip" some deps
    if (depRangeRestart)
      depPtr = depRangeRestart;
    // join queues on frame end
    // NOTE: specially inside loop, to avoid leaving some work "floating" somewhere after fence
    queueJoin.addWaits(i);
    // all signals on this submit must be used for related waits, otherwise something is broken

    G_ASSERTF(i.signals.empty(), "vulkan: some signals are not consumed!");
  }
  scratch.submitGraph.clear();
  scratch.cmdListsSubmitDeps.clear();
  scratch.cmdListsToSubmit.clear();
  scratch.userQueueSignals.clear();
}

void ExecutionContext::addQueueDep(uint32_t src_submit, uint32_t dst_submit)
{
  // nothing to wait in this case
  if (src_submit == 0 || dst_submit == 0)
    return;
  G_ASSERTF(src_submit != dst_submit, "vulkan: wrong dependency between queues detected src equal dst %u == %u", src_submit,
    dst_submit);
  scratch.cmdListsSubmitDeps.push_back({src_submit, dst_submit});
}

void ExecutionContext::joinQueuesToSubmitFence()
{
  // all buffers that we used on gpu job, must be completed after fence wait
  // to make sure of this
  // add dependency for each final command buffer on every unique queue where we never "joined" workloads
  for (uint32_t i : lastBufferIdxOnQueue)
  {
    // queue was fully inactive
    if (!i)
      continue;
    // last queues buffer is one we will wait with fence
    if (i == (scratch.cmdListsToSubmit.size() - 1))
      continue;
    // last buffer signals semaphore that is waited for someone
    bool signaling = false;
    for (ExecutionScratch::CommandBufferSubmitDeps &j : scratch.cmdListsSubmitDeps)
      signaling |= j.from == i;
    if (signaling)
      continue;
    addQueueDep(i, scratch.cmdListsToSubmit.size() - 1);
  }
}

void ExecutionContext::flush(ThreadedFence *fence)
{
  G_ASSERT(!flushProcessed);

  beginCustomStage("flush");

  Backend::immediateConstBuffers.flush();

  if (scratch.imageResidenceRestores.size())
  {
    scratch.cmdListsToSubmit[0].queue = DeviceQueueType::GRAPHICS;
    scratch.cmdListsToSubmit[0].handle = allocAndBeginCommandBuffer(scratch.cmdListsToSubmit[0].queue);
    Backend::cb.set(scratch.cmdListsToSubmit[0].handle);

    restoreImageResidencies();
    writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_IMAGE_RESIDENCY_RESTORE);
    Backend::cb.set(frameCore);
  }

  // transfer queue is very limited, not fitting for all post frame commands
  // splitting it into different submits is also non effective
  // so live on graphics, but with "split" for possible end frame overlapping
  switchFrameCore(DeviceQueueType::GRAPHICS);
  flushPostFrameCommands();
  // no more command buffers after this point! so push last active one in list to submit
  scratch.cmdListsToSubmit.push_back({frameCoreQueue, frameCore});

  // if work diverged around frame end, must sync it to ensure buffers are completed before fence wait
  joinQueuesToSubmitFence();

  // finalize command buffers
  for (ExecutionScratch::CommandBufferSubmit &i : scratch.cmdListsToSubmit)
  {
    if (!is_null(i.handle))
      VULKAN_EXIT_ON_FAIL(vkDev.vkEndCommandBuffer(i.handle));
  }
  enqueueCommandListsToMultipleQueues(fence);

  onFrameCoreReset();
  Backend::gpuJob.get().pendingTimestamps = data.timestampQueryBlock;
  Backend::gpuJob.get().replayId = data.id;

  Backend::pipelineCompiler.processQueued();
  flushProcessed = true;
  Backend::cb.verifyReorderEmpty();
  scratch.debugEventStack.clear();
}

void ExecutionContext::insertEvent(const char *marker, uint32_t color /*=0xFFFFFFFF*/)
{
  if (!Globals::cfg.bits.allowDebugMarkers)
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
    VULKAN_LOG_CALL(Backend::cb.wCmdDebugMarkerInsertEXT(&info));
    return;
  }
#endif

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
    VULKAN_LOG_CALL(Backend::cb.wCmdInsertDebugUtilsLabelEXT(&info));
  }
#endif
}

void ExecutionContext::pushEventRaw(const char *marker, uint32_t color)
{
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
    VULKAN_LOG_CALL(Backend::cb.wCmdDebugMarkerBeginEXT(&info));
    return;
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
    VULKAN_LOG_CALL(Backend::cb.wCmdBeginDebugUtilsLabelEXT(&info));
  }
#endif
}

void ExecutionContext::popEventRaw()
{
#if VK_EXT_debug_marker
  if (vkDev.hasExtension<DebugMarkerEXT>())
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdDebugMarkerEndEXT());
    return;
  }
#endif
#if VK_EXT_debug_utils
  auto &instance = vkDev.getInstance();
  if (instance.hasExtension<DebugUtilsEXT>())
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdEndDebugUtilsLabelEXT());
  }
#endif
}

void ExecutionContext::finishDebugEventRanges()
{
  for (size_t i = 0; i < scratch.debugEventStack.size(); ++i)
    popEventRaw();
}

void ExecutionContext::restoreDebugEventRanges()
{
  for (const ExecutionScratch::DebugEvent &i : scratch.debugEventStack)
    pushEventRaw(i.name, i.color);
}

void ExecutionContext::pushEventTracked(const char *marker, uint32_t color /* = 0xFFFFFFFF*/)
{
  if (!Globals::cfg.bits.allowDebugMarkers)
    return;

  scratch.debugEventStack.push_back({color, marker});
  pushEventRaw(marker, color);
}

void ExecutionContext::popEventTracked()
{
  if (!Globals::cfg.bits.allowDebugMarkers)
    return;

  // avoid issues on markers that live over frame
  if (scratch.debugEventStack.size())
    scratch.debugEventStack.pop_back();
  popEventRaw();
}

void ExecutionContext::pushEvent(StringIndexRef name)
{
  Backend::profilerStack.pushInterruptChain(data.charStore.data() + name.get());
  pushEventTracked(data.charStore.data() + name.get());
}

void ExecutionContext::popEvent()
{
  Backend::profilerStack.popInterruptChain();
  popEventTracked();
}

void ExecutionContext::beginQuery(VulkanQueryPoolHandle pool, uint32_t index, VkQueryControlFlags flags)
{
  VULKAN_LOG_CALL(Backend::cb.wCmdBeginQuery(pool, index, flags));
  directDrawCountInSurvey = directDrawCount;
}

void ExecutionContext::endQuery(VulkanQueryPoolHandle pool, uint32_t index)
{
  directDrawCountInSurvey = directDrawCount - directDrawCountInSurvey;
  VULKAN_LOG_CALL(Backend::cb.wCmdEndQuery(pool, index));
}

void ExecutionContext::wait(ThreadedFence *fence) { fence->wait(vkDev); }

#if D3D_HAS_RAY_TRACING

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query

void ExecutionContext::accumulateRaytraceBuildAccesses(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    auto &tlbd = build_data.tlas;
    verifyResident(tlbd.instanceBuffer.buffer);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
      tlbd.instanceBuffer.buffer, {tlbd.instanceBuffer.bufOffset(0), sizeof(VkAccelerationStructureInstanceKHR) * tlbd.instanceCount});
  }
  else
  {
    auto &blbd = build_data.blas;
    for (const RaytraceBLASBufferRefs &bufferRefs :
      make_span(data.raytraceBLASBufferRefsStore.data() + blbd.firstGeometry, blbd.geometryCount))
    {
      verifyResident(bufferRefs.geometry);
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
        bufferRefs.geometry, {bufferRefs.geometryOffset, bufferRefs.geometrySize});

      if (bufferRefs.index)
      {
        verifyResident(bufferRefs.index);
        Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
          bufferRefs.index, {bufferRefs.indexOffset, bufferRefs.indexSize});
      }

      if (bufferRefs.transform)
      {
        verifyResident(bufferRefs.transform);
        Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
          bufferRefs.transform, {bufferRefs.transformOffset, RT_TRANSFORM_SIZE});
      }
    }
    if (blbd.compactionSizeBuffer)
      verifyResident(blbd.compactionSizeBuffer.buffer);
  }

  verifyResident(build_data.scratchBuf.buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                  VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
    build_data.scratchBuf.buffer, {build_data.scratchBuf.bufOffset(0), build_data.scratchBuf.visibleDataSize});

  if (build_data.update)
    Backend::sync.addAccelerationStructureAccess(
      {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
      build_data.dst);
  else
    Backend::sync.addAccelerationStructureAccess(
      {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR}, build_data.dst);
}

void ExecutionContext::accumulateAssumedRaytraceStructureReads(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
  {
    // there is no processed references to BLAS from TLAS in build data/resource, so for speed
    // do assumed read on CS/RT stage, if we do no read really - that barrier will fail!
    // set seal for safety here, to not conflict with followup writes without reads
    Backend::sync.addAccelerationStructureAccess(LogicAddress::forBLASIndirectReads(), build_data.dst);
  }
}

void initBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR &dst, const RaytraceStructureBuildData &build_data)
{
  dst = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};

  dst.mode = build_data.update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  dst.flags = build_data.flags;
  dst.type = build_data.type;
  dst.dstAccelerationStructure = build_data.dst->getHandle();
  if (build_data.update)
    dst.srcAccelerationStructure = build_data.dst->getHandle();
  dst.scratchData.deviceAddress = build_data.scratchBuf.devOffset(0);
}

void ExecutionContext::buildAccelerationStructure(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    initBuildInfo(buildInfo, build_data);

    VkAccelerationStructureGeometryInstancesDataKHR instancesData = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instancesData.data.deviceAddress = build_data.tlas.instanceBuffer.devOffset(0);

    VkAccelerationStructureGeometryKHR tlasGeo = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeo.geometry.instances = instancesData;

    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &tlasGeo;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveOffset = 0;
    rangeInfo.primitiveCount = build_data.tlas.instanceCount;

    const VkAccelerationStructureBuildRangeInfoKHR *build_range = &rangeInfo;
    VULKAN_LOG_CALL(Backend::cb.wCmdBuildAccelerationStructuresKHR(1, &buildInfo, &build_range));
  }
  else
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    initBuildInfo(buildInfo, build_data);

    buildInfo.geometryCount = build_data.blas.geometryCount;
    buildInfo.pGeometries = data.raytraceGeometryKHRStore.data() + build_data.blas.firstGeometry;

    const VkAccelerationStructureBuildRangeInfoKHR *build_range = &data.raytraceBuildRangeInfoKHRStore[build_data.blas.firstGeometry];
    VULKAN_LOG_CALL(Backend::cb.wCmdBuildAccelerationStructuresKHR(1, &buildInfo, &build_range));

    if (build_data.blas.compactionSizeBuffer)
    {
      Backend::sync.addAccelerationStructureAccess(
        {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR}, build_data.dst);

      constexpr VkDeviceSize querySize = sizeof(uint64_t);
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT},
        build_data.blas.compactionSizeBuffer.buffer, {build_data.blas.compactionSizeBuffer.bufOffset(0), querySize});
    }
  }
}

void ExecutionContext::queryAccelerationStructureCompationSizes(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type != VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
    return;
  BufferRef compactSizeBuf = build_data.blas.compactionSizeBuffer;
  if (!compactSizeBuf)
    return;

  VkAccelerationStructureKHR dstAc = build_data.dst->getHandle();

  // do blocking-like size query for now, if not efficient - redo with per frame query pool copy
  constexpr VkDeviceSize querySize = sizeof(uint64_t);
  VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(Globals::rtSizeQueryPool.getPool(), 0, 1));
  VULKAN_LOG_CALL(Backend::cb.wCmdWriteAccelerationStructuresPropertiesKHR(1, &dstAc,
    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, Globals::rtSizeQueryPool.getPool(), 0));
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyQueryPoolResults(Globals::rtSizeQueryPool.getPool(), 0, 1, compactSizeBuf.getHandle(),
    compactSizeBuf.bufOffset(0), querySize, VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));
}

void ExecutionContext::buildAccelerationStructures(RaytraceStructureBuildData *build_data, uint32_t count)
{
  auto dataRange = make_span(build_data, count);
  for (RaytraceStructureBuildData &itr : dataRange)
    accumulateRaytraceBuildAccesses(itr);
  Backend::sync.completeNeeded();
  for (RaytraceStructureBuildData &itr : dataRange)
    buildAccelerationStructure(itr);
  Backend::sync.completeNeeded();
  for (RaytraceStructureBuildData &itr : dataRange)
    queryAccelerationStructureCompationSizes(itr);
  for (RaytraceStructureBuildData &itr : dataRange)
    accumulateAssumedRaytraceStructureReads(itr);
}

#endif

#endif

void ExecutionContext::printMemoryStatistics()
{
  WinAutoLock lk(Globals::Mem::mutex);
  Globals::Mem::pool.printStats();
  uint32_t freeDeviceLocalMemoryKb = Globals::Mem::pool.getCurrentAvailableDeviceKb();
  if (freeDeviceLocalMemoryKb)
    debug("System reported %u Mb free GPU memory", freeDeviceLocalMemoryKb >> 10);
  Globals::Mem::report.dumpInfo();
}

String ExecutionContext::getCurrentCmdCaller()
{
  String ret(32, "cmd %p = <unknown caller>", cmd);

#if DAGOR_DBGLEVEL > 0
  if (cmdIndex < data.commandCallers.size())
    ret = String(2048, "cmd %p = %s", cmd, backtrace::get_stack_by_hash(data.commandCallers[cmdIndex]));
#endif

  return ret;
}

uint64_t ExecutionContext::getCurrentCmdCallerHash()
{
#if DAGOR_DBGLEVEL > 0
  if (cmdIndex < data.commandCallers.size())
    return data.commandCallers[cmdIndex];
#endif

  return 0;
}

void ExecutionContext::reportMissingPipelineComponent(const char *component)
{
  D3D_ERROR("vulkan: missing pipeline component %s at state apply, caller %s", component, getCurrentCmdCaller());
  generateFaultReport();
  DAG_FATAL("vulkan: missing pipeline %s (broken state flush)", component);
}

void ExecutionContext::flushAndWait(ThreadedFence *user_fence)
{
  FrameInfo &frame = Backend::gpuJob.get();
  flush(frame.frameDone);
  Backend::gpuJob.end();
  while (Globals::timelines.get<TimelineManager::GpuExecute>().advance())
    ; // VOID
  Backend::gpuJob.start();

  // we can't wait fence from 2 threads, so
  // shortcut to avoid submiting yet another fence
  if (user_fence)
    user_fence->setSignaledExternally();
}

void ExecutionContext::doFrameEndCallbacks()
{
  if (scratch.frameEventCallbacks.size())
  {
    TIME_PROFILE(vulkan_frame_end_user_cb);
    for (FrameEvents *callback : scratch.frameEventCallbacks)
      callback->endFrameEvent();
    scratch.frameEventCallbacks.clear();
  }

  {
    WinAutoLock lk(Globals::Mem::mutex);
    Globals::Mem::res.onBackFrameEnd();
  }

  if (Globals::cfg.memoryStatisticsPeriodUs)
  {
    int64_t currentTimeRef = ref_time_ticks();

    if (ref_time_delta_to_usec(currentTimeRef - Backend::timings.lastMemoryStatTime) > Globals::cfg.memoryStatisticsPeriodUs)
    {
      printMemoryStatistics();
      Backend::timings.lastMemoryStatTime = currentTimeRef;
    }
  }
}

void ExecutionContext::present()
{
  FrameInfo &frame = Backend::gpuJob.get();

  beginCustomStage("present");

  swapchain.prePresent();
  writeExectionChekpointNonCommandStream(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MARKER_NCMD_PRE_PRESENT);
  presentSignal = frame.allocSemaphore(Globals::VK::dev);
  flush(frame.frameDone);

  int64_t frameCallbacksDurationTicks = 0;
  {
    ScopedTimerTicks watch(frameCallbacksDurationTicks);
    doFrameEndCallbacks();
  }

  {
    ScopedTimerTicks watch(Backend::timings.presentWaitDuration);
    swapchain.present(presentSignal);
    presentSignal = VulkanNullHandle();
  }

  Globals::timelines.get<TimelineManager::GpuExecute>().advance();
  Backend::gpuJob.restart();

  {
    ScopedTimerTicks watch(Backend::timings.acquireBackBufferDuration);
    swapchain.onFrameBegin(*this);
  }

  if (data.generateFaultReport)
    generateFaultReport();

  if (Globals::cfg.bits.allowPredictedLatencyWaitWorker)
  {
    Backend::latencyWaiter.update(
      Backend::timings.gpuWaitDuration + Backend::timings.presentWaitDuration + frameCallbacksDurationTicks);
    Backend::latencyWaiter.wait();
  }
}

void ExecutionContext::queueBufferDiscard(BufferRef old_buf, const BufferRef &new_buf, uint32_t flags)
{
  scratch.delayedDiscards.push_back({old_buf, new_buf, flags});
}

void ExecutionContext::applyQueuedDiscards()
{
  for (const ExecutionScratch::DiscardNotify &i : scratch.delayedDiscards)
  {
    bool wasReplaced = Backend::State::pipe.processBufferDiscard(i.oldBuf, i.newBuf, i.flags);
    if ((i.oldBuf.buffer != i.newBuf.buffer) && !(i.flags & SBCF_FRAMEMEM))
    {
      i.oldBuf.buffer->markDead();
      if (wasReplaced)
        Backend::State::pendingCleanups.getArray<Buffer *>().push_back(i.oldBuf.buffer);
      else
        Backend::gpuJob.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*i.oldBuf.buffer);
    }
  }
  scratch.delayedDiscards.clear();
}

void ExecutionContext::applyStateChanges()
{
  // custom stages are not pushing front state from caller thread
  // so discard may by "rolled back" by front state if we process discards at custom stage
  if (Backend::State::exec.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::CUSTOM)
    applyQueuedDiscards();
  // pipeline state are persistent, while execution state resets every work item
  // so they can't be merged in one state right now
  Backend::State::pipe.apply(Backend::State::exec);
  Backend::State::exec.apply(*this);
}

void ExecutionContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  flushComputeState();

  VULKAN_LOG_CALL(Backend::cb.wCmdDispatch(x, y, z));
  writeExectionChekpoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void ExecutionContext::dispatchIndirect(BufferRef buffer, uint32_t offset)
{
  trackIndirectArgAccesses(buffer, offset, 1, sizeof(VkDispatchIndirectCommand));
  flushComputeState();

  VULKAN_LOG_CALL(Backend::cb.wCmdDispatchIndirect(buffer.getHandle(), buffer.bufOffset(offset)));
  writeExectionChekpoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void ExecutionContext::fillBuffer(Buffer *buffer, uint32_t offset, uint32_t size, uint32_t value)
{
  verifyResident(buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buffer, {offset, size});
  Backend::sync.completeNeeded();
  VULKAN_LOG_CALL(Backend::cb.wCmdFillBuffer(buffer->getHandle(), offset, size, value));
}

void ExecutionContext::clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  // TODO flush_clear_state is only needed if the image is a cleared attachment
  beginCustomStage("clearDepthStencilImage");

  verifyResident(image);

  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {area.baseMipLevel, area.levelCount, area.baseArrayLayer, area.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdClearDepthStencilImage(image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
}

void ExecutionContext::beginCustomStage(const char *why)
{
  startNode(ExecutionNode::CUSTOM);

  bool enteringCustomStage =
    Backend::State::exec.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::CUSTOM;

  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::CUSTOM);
  applyStateChanges();

  if (enteringCustomStage)
  {
    if (Globals::cfg.bits.enableDeviceExecutionTracker)
      Backend::gpuJob.get().execTracker.addMarker(why, strlen(why));
    insertEvent(why, 0xFFFF00FF);
  }
}

void ExecutionContext::clearView(int what)
{
  G_ASSERTF((what & FramebufferState::CLEAR_LOAD) == 0,
    "vulkan: used special clear bit outside of driver internal code, need to adjust internal special clear bit");

  startNode(ExecutionNode::RP);
  ensureActivePass();

  // when async pipelines enabled and we trying to discard target, use clear instead
  // this avoids leaking some garbadge when pipelines are not ready
  if (Globals::pipelines.asyncCompileEnabledGR() && (what & CLEAR_DISCARD) != 0)
  {
    if (what & CLEAR_DISCARD_TARGET)
      what |= CLEAR_TARGET;
    if (what & CLEAR_DISCARD_ZBUFFER)
      what |= CLEAR_ZBUFFER;
    if (what & CLEAR_DISCARD_STENCIL)
      what |= CLEAR_STENCIL;
    what &= ~CLEAR_DISCARD;
  }

  getFramebufferState().clearMode |= what;
  Backend::State::exec.interruptRenderPass("ClearView");
  Backend::State::exec.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
  applyStateChanges();
  invalidateActiveGraphicsPipeline();

  writeExectionChekpoint(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

  getFramebufferState().clearMode = 0;
}

void ExecutionContext::clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  beginCustomStage("clearColorImage");

  verifyResident(image);

  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {area.baseMipLevel, area.levelCount, area.baseArrayLayer, area.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdClearColorImage(image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &area));
}

void ExecutionContext::copyImageToBufferOrdered(Buffer *dst, Image *src, const VkBufferImageCopy *regions, int count)
{
  verifyResident(src);
  verifyResident(dst);
  for (int i = 0; i < count; ++i)
  {
    const VkBufferImageCopy &region = regions[i];
    const auto sz =
      src->getFormat().calculateImageSize(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth, 1);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst, {region.bufferOffset, sz});
    Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      {region.imageSubresource.mipLevel, 1, region.imageSubresource.baseArrayLayer, region.imageSubresource.layerCount});
  }
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(
    Backend::cb.wCmdCopyImageToBuffer(src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->getHandle(), count, regions));
}

void ExecutionContext::copyImage(Image *src, Image *dst, uint32_t src_mip, uint32_t dst_mip, uint32_t mip_count, uint32_t region_count,
  uint32_t first_region)
{
  verifyResident(src);
  verifyResident(dst);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {src_mip, mip_count, 0, src->getArrayLayers()});
  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {dst_mip, mip_count, 0, dst->getArrayLayers()});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdCopyImage(src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count, data.imageCopyInfos.data() + first_region));

  if (dst->isSampledSRV())
    dst->layout.roSealTargetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // we can't know is it readed or not, so assume readed
  if (dst->isUsedInBindless())
  {
    trackBindlessRead(dst);
    Backend::sync.completeNeeded();
  }
}

void ExecutionContext::copyQueryResult(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count, Buffer *dst)
{
  const uint32_t stride = sizeof(uint32_t);
  uint32_t ofs = dst->bufOffsetLoc(index * stride);
  uint32_t sz = stride * count;

  // do not copy if there was no writes to query, as copy will crash on some GPUs in this situation
  if (directDrawCountInSurvey == 0)
  {
    // fill zero instead
    fillBuffer(dst, ofs, sz, 1);
    return;
  }

  verifyResident(dst);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst, {ofs, sz});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdCopyQueryPoolResults(pool, index, count, dst->getHandle(), ofs, sz, VK_QUERY_RESULT_WAIT_BIT));
}

void ExecutionContext::bindVertexUserData(const BufferRef &ref)
{
  Backend::State::exec.set<StateFieldGraphicsVertexBuffersBindArray, BufferRef, BackGraphicsState>(ref);
  Backend::State::exec.apply(*this);

  // reset to user-provided bindings on next state apply
  Backend::State::pipe.set<StateFieldGraphicsVertexBuffers, uint32_t, FrontGraphicsState>(0);
}

void ExecutionContext::drawIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  if (!renderAllowed)
  {
    insertEvent("drawIndirect: async compile", 0xFF0000FF);
    return;
  }

  trackIndirectArgAccesses(buffer, offset, count, stride);

  if (count > 1 && !Globals::cfg.has.multiDrawIndirect)
  {
    // emulate multi draw indirect
    for (uint32_t i = 0; i < count; ++i)
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndirect(buffer.getHandle(), buffer.bufOffset(offset), 1, stride));
      writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
      offset += stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndirect(buffer.getHandle(), buffer.bufOffset(offset), count, stride));
    writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  }
}

void ExecutionContext::drawIndexedIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  if (!renderAllowed)
  {
    insertEvent("drawIndexedIndirect: async compile", 0xFF0000FF);
    return;
  }

  trackIndirectArgAccesses(buffer, offset, count, stride);

  if (count > 1 && !Globals::cfg.has.multiDrawIndirect)
  {
    // emulate multi draw indirect
    for (uint32_t i = 0; i < count; ++i)
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndexedIndirect(buffer.getHandle(), buffer.bufOffset(offset), 1, stride));
      writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
      offset += stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndexedIndirect(buffer.getHandle(), buffer.bufOffset(offset), count, stride));
    writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  }
}

void ExecutionContext::draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance)
{
  if (!renderAllowed)
  {
    insertEvent("draw: async compile", 0xFF0000FF);
    return;
  }

  ++directDrawCount;
  VULKAN_LOG_CALL(Backend::cb.wCmdDraw(count, instance_count, start, first_instance));
  writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void ExecutionContext::drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base,
  uint32_t first_instance)
{
  if (!renderAllowed)
  {
    insertEvent("drawIndexed: async compile", 0xFF0000FF);
    return;
  }

  ++directDrawCount;
  VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndexed(count, instance_count, index_start, vertex_base, first_instance));
  writeExectionChekpoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void ExecutionContext::flushGrahpicsState(VkPrimitiveTopology top)
{
  if (Backend::State::exec.set<StateFieldGraphicsPrimitiveTopology, VkPrimitiveTopology, BackGraphicsState>(top))
    invalidateActiveGraphicsPipeline();

  Backend::State::exec.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(1);
  applyStateChanges();
}

void ExecutionContext::flushComputeState()
{
  Backend::State::exec.set<StateFieldComputeFlush, uint32_t, BackComputeState>(0);
  applyStateChanges();
}

void ExecutionContext::ensureActivePass() { Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::GRAPHICS); }

void ExecutionContext::registerFrameEventsCallback(FrameEvents *callback)
{
  callback->beginFrameEvent();
  scratch.frameEventCallbacks.push_back(callback);
}

bool ExecutionContext::interruptFrameCore()
{
  // can't interrupt native pass
  InPassStateFieldType inPassState = Backend::State::exec.get<StateFieldGraphicsInPass, InPassStateFieldType, BackGraphicsState>();
  if (inPassState == InPassStateFieldType::NATIVE_PASS)
    return false;

  // apply any pending changes & finish all scopes
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::CUSTOM);
  applyStateChanges();

  switchFrameCore(frameCoreQueue);
  onFrameCoreReset();

  return true;
}

void ExecutionContext::onFrameCoreReset()
{
  Backend::State::pipe.makeDirty();
  Backend::State::exec.reset();
  Backend::State::exec.makeDirty();
}

void ExecutionContext::startNode(ExecutionNode node)
{
  // workaround for adreno drivers crash on viewport/scissor setted in command buffer before cs dispatch
  if ((node == ExecutionNode::CS) &&
      (Backend::State::exec.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::COMPUTE) &&
      Globals::cfg.has.adrenoViewportConflictWithCS)
  {
    // start new command buffer on graphics->cs stage change
    // otherwise driver will crash if viewport/scissor was set in command buffer before
    // and there was no draw or no render pass and no global memory barrier
    interruptFrameCore();
  }
  ++actionIdx;
}

void ExecutionContext::trackTResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage)
{
  const TRegister &reg = state.tBinds[slot];
  switch (reg.type)
  {
    case TRegister::TYPE_IMG:
    {
      bool roDepth = state.isConstDepthStencil.test(slot);
      VkImageLayout srvLayout = roDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::T), reg.img.ptr, srvLayout,
        {reg.img.view.getMipBase(), reg.img.view.getMipCount(), reg.img.view.getArrayBase(), reg.img.view.getArrayCount()});
    }
    break;
    case TRegister::TYPE_BUF:
      Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::T), reg.buf.buffer,
        {reg.buf.bufOffset(0), reg.buf.visibleDataSize});
      break;
#if D3D_HAS_RAY_TRACING
    case TRegister::TYPE_AS:
      Backend::sync.addAccelerationStructureAccess(LogicAddress::forAccelerationStructureOnExecStage(stage, RegisterType::T),
        reg.rtas);
      break;
#endif
    default: break;
  }
}

void ExecutionContext::trackUResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage)
{
  const URegister &reg = state.uBinds[slot];
  if (reg.image)
  {
    Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(stage, RegisterType::U), reg.image, VK_IMAGE_LAYOUT_GENERAL,
      {reg.imageView.getMipBase(), reg.imageView.getMipCount(), reg.imageView.getArrayBase(), reg.imageView.getArrayCount()});
  }
  else
  {
    Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::U), reg.buffer.buffer,
      {reg.buffer.bufOffset(0), reg.buffer.visibleDataSize});
  }
}

void ExecutionContext::trackBResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage)
{
  const BufferRef &reg = state.bBinds[slot];
  if (slot == state.GCBSlot && state.bGCBActive)
    return;
  Backend::sync.addBufferAccess(LogicAddress::forBufferOnExecStage(stage, RegisterType::B), reg.buffer,
    {reg.bufOffset(0), reg.visibleDataSize});
}

void ExecutionContext::trackStageResAccessesNonParallel(const spirv::ShaderHeader &header, ExtendedShaderStage stage)
{
  PipelineStageStateBase &state = Backend::State::exec.getResBinds(stage);
  for (uint32_t i : LsbVisitor{state.tBinds.validMask() & header.tRegisterUseMask})
    trackTResAccesses(i, state, stage);

  for (uint32_t i : LsbVisitor{state.uBinds.validMask() & header.uRegisterUseMask})
    trackUResAccesses(i, state, stage);

  for (uint32_t i : LsbVisitor{state.bBinds.validMask() & header.bRegisterUseMask})
    trackBResAccesses(i, state, stage);
}

void ExecutionContext::trackStageResAccesses(const spirv::ShaderHeader &header, ExtendedShaderStage stage)
{
  PipelineStageStateBase &state = Backend::State::exec.getResBinds(stage);
  for (uint32_t i : LsbVisitor{state.tBinds.validDirtyMask() & header.tRegisterUseMask})
    trackTResAccesses(i, state, stage);

  for (uint32_t i : LsbVisitor{state.uBinds.validDirtyMask() & header.uRegisterUseMask})
    trackUResAccesses(i, state, stage);

  for (uint32_t i : LsbVisitor{(state.bBinds.validDirtyMask() | state.bOffsetDirtyMask) & header.bRegisterUseMask})
    trackBResAccesses(i, state, stage);
}

void ExecutionContext::trackIndirectArgAccesses(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride)
{
  verifyResident(buffer.buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}, buffer.buffer,
    {buffer.bufOffset(offset), stride * count});
}

void ExecutionContext::trackBindlessRead(Image *img)
{
  VkImageLayout srvLayout = img->getUsage() & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                              ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                              : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  verifyResident(img);
  Backend::sync.addImageAccess(LogicAddress::forImageBindlessRead(), img, srvLayout,
    {0, img->getMipLevels(), 0, img->getArrayLayers()});
}

void ExecutionContext::executeFSR(amd::FSR *fsr, const FSRUpscalingArgs &params)
{
  auto prepareImage = [&](Image *src) {
    if (src)
    {
      verifyResident(src);
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(ExtendedShaderStage::CS, RegisterType::T), src,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
    }
  };

  auto toPair = [&](Image *src) -> eastl::pair<VkImage, VkImageCreateInfo> {
    if (src)
      return eastl::make_pair(src->getHandle(), src->getDescription().ici.toVk());
    return eastl::make_pair(VkImage(0), VkImageCreateInfo{});
  };

  prepareImage(params.colorTexture);
  prepareImage(params.depthTexture);
  prepareImage(params.motionVectors);
  prepareImage(params.exposureTexture);
  prepareImage(params.reactiveTexture);
  prepareImage(params.transparencyAndCompositionTexture);
  prepareImage(params.outputTexture);

  auto colorTexture = toPair(params.colorTexture);
  auto depthTexture = toPair(params.depthTexture);
  auto motionTexture = toPair(params.motionVectors);
  auto exposureTexture = toPair(params.exposureTexture);
  auto reactiveTexture = toPair(params.reactiveTexture);
  auto transparencyAndCompositionTexture = toPair(params.transparencyAndCompositionTexture);
  auto outputTexture = toPair(params.outputTexture);

  amd::FSR::UpscalingPlatformArgs args = params;
  args.colorTexture = &colorTexture;
  args.depthTexture = &depthTexture;
  args.motionVectors = &motionTexture;
  args.exposureTexture = &exposureTexture;
  args.outputTexture = &outputTexture;
  args.reactiveTexture = &reactiveTexture;
  args.transparencyAndCompositionTexture = &transparencyAndCompositionTexture;

  Backend::sync.completeNeeded();

  fsr->doApplyUpscaling(args, frameCore);
}

template <typename ResType>
void ExecutionContext::verifyResident(ResType *obj)
{
  obj->setUsedInRendering();
  if (!obj->isResident())
  {
    TIME_PROFILE(vulkan_restore_residency);
    WinAutoLock lk(Globals::Mem::mutex);
    ResourceAlgorithm<ResType> alg(*obj);
    G_ASSERTF(obj->getBaseHandle(), "vulkan: trying to make resident already destroyed or invalid resource %p at %s", obj,
      getCurrentCmdCaller());
    if (!alg.makeResident(*this))
      obj->reportOutOfMemory();
  }
}

template void ExecutionContext::verifyResident<Image>(Image *);

void ExecutionContext::finishAllGPUWorkItems()
{
  while (Globals::timelines.get<TimelineManager::GpuExecute>().advance())
    ; // VOID
}

void ExecutionContext::queueImageResidencyRestore(Image *img) { scratch.imageResidenceRestores.push_back(img); }
