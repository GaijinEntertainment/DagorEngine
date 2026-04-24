// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/gpuLatency.h>
#include <generic/dag_enumerate.h>
#include "screen.h"
#include "backend/context.h"
#include "execution_sync.h"
#include "stacked_profile_events.h"
#include "execution_timings.h"
#include "predicted_latency_waiter.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdSwapchainImageAcquire &cmd)
{
  beginCustomStage("SwapchainImageAcquire");
  // discard contents of image
  cmd.img->layout.resetTo(VK_IMAGE_LAYOUT_UNDEFINED);
  // as noted in spec, we must sync acquire to any stages we use image later on
  // treat this as fake read on all stages
  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT}, cmd.img, VK_IMAGE_LAYOUT_UNDEFINED,
    {0, 1, 0, 1});
  Backend::sync.completeNeeded();
  if (!is_null(cmd.acquireSem))
    // non checked assume that swapchain image will be first used in graphics queue, beware
    Globals::VK::queue[DeviceQueueType::GRAPHICS].waitExternSemaphore(cmd.acquireSem, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

TSPEC void BEContext::execCmd(const CmdPresent &params)
{
  Backend::profilerStack.finish(); // ends frame core
  TIME_PROFILE(vulkan_CmdPresent)
  FrameInfo &frame = Backend::gpuJob.get();

  beginCustomStage("present");

  G_ASSERT(frameReadySemaphoresForPresent.empty());

  eastl::fixed_vector<uint32_t, MAX_SECONDARY_SWAPCHAIN_COUNT + 1, false> presentImageIndices;
  eastl::fixed_vector<VkSwapchainKHR, MAX_SECONDARY_SWAPCHAIN_COUNT + 1, false> presentSwapchains;

  if (params.isFullBackendAcquire())
  {
    VulkanSemaphoreHandle acquireSem;
    uint32_t acquiredImgIndex{};
    if (acquireSwapchainImage(params, acquiredImgIndex, acquireSem))
    {
      Image *targetSwapchainImage = params.images[acquiredImgIndex].img;
      Globals::VK::queue[DeviceQueueType::GRAPHICS].waitSemaphore(acquireSem, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
      baseMipBlit(params.img, targetSwapchainImage);
      frameReadySemaphoresForPresent.push_back(params.images[acquiredImgIndex].frame);
      makeImageReadyForPresent(targetSwapchainImage);
      presentImageIndices.push_back(acquiredImgIndex);
      presentSwapchains.push_back(params.swapchain);
    }
  }
  else if (params.img)
  {
    frameReadySemaphoresForPresent.push_back(params.frameSem);
    makeImageReadyForPresent(params.img);
    presentImageIndices.push_back(params.imageIndex);
    presentSwapchains.push_back(params.swapchain);
  }
  bool primaryPresent = !presentSwapchains.empty();

  for (const auto &secondaryPresent : data->secondaryPresents)
  {
    if (!secondaryPresent.isFullBackendAcquire())
      continue; // secondary presents always use backend acquire
    VulkanSemaphoreHandle acquireSem;
    uint32_t acquiredImgIndex{};
    if (acquireSwapchainImage(secondaryPresent, acquiredImgIndex, acquireSem))
    {
      Image *targetSwapchainImage = secondaryPresent.images[acquiredImgIndex].img;
      Globals::VK::queue[DeviceQueueType::GRAPHICS].waitSemaphore(acquireSem, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
      baseMipBlit(secondaryPresent.img, targetSwapchainImage);
      frameReadySemaphoresForPresent.push_back(secondaryPresent.images[acquiredImgIndex].frame);
      makeImageReadyForPresent(targetSwapchainImage);
      presentImageIndices.push_back(acquiredImgIndex);
      presentSwapchains.push_back(secondaryPresent.swapchain);
    }
  }

  writeExceptionChekpointNonCommandStream(MARKER_NCMD_PRE_PRESENT);
  flush(frame.frameDone);

  auto *lowLatencyModule = primaryPresent ? GpuLatency::getInstance() : nullptr;
  if (needRenderSubmitLatencyMarkerBeforePresent && lowLatencyModule)
  {
    needRenderSubmitLatencyMarkerBeforePresent = false;
    lowLatencyModule->setMarker(params.enginePresentFrameId, lowlatency::LatencyMarkerType::RENDERSUBMIT_END);
  }

  int64_t frameCallbacksDurationTicks = 0;
  {
    ScopedTimerTicks watch(frameCallbacksDurationTicks);
    doFrameEndCallbacks();
  }

  VulkanSemaphoreHandle backendAcquireSemaphore{};
  uint32_t imgAcquireIndex = 0;
  if (!presentSwapchains.empty())
  {
    TIME_PROFILE(vulkan_swapchain_present);
    ScopedTimerTicks watch(Backend::timings.presentWaitDuration);
    {
      DeviceQueue::TrimmedPresentInfo pi;
      pi.swapchainCount = presentSwapchains.size();
      pi.pSwapchains = presentSwapchains.data();
      pi.frameReadySemaphores = ary(frameReadySemaphoresForPresent.data());
      pi.frameReadySemaphoreCount = frameReadySemaphoresForPresent.size();
      pi.pImageIndices = presentImageIndices.data();
      G_ASSERT(presentImageIndices.size() == presentSwapchains.size());

      // we ignore result of present operation here
      // - device lost will be handled at submit,
      //   and in some cases it may give device lost when it is not pure device lost (fail wihout guarantie on sync)
      // - suboptimals/outdated/etc will be handled at acquire

      if (lowLatencyModule)
        lowLatencyModule->setMarker(params.enginePresentFrameId, lowlatency::LatencyMarkerType::PRESENT_START);

      // avoid presenting as we may block/crash inside present if device is already lost
      if (!Backend::interop.deviceLost.load())
      {
        if (params.isFullBackendAcquire() || !primaryPresent)
        {
          Globals::VK::queue[DeviceQueueType::GRAPHICS].present(pi);
        }
        else if (params.img)
        {
          WinAutoLock lock(*params.mutex);
          // next frame acquire not happened before we took lock for present
          // do acquire here to avoid waiting for variable timed present call
          if (params.currentAcquireId && params.submitAcquireId == *params.currentAcquireId)
            acquireSwapchainImage(params, imgAcquireIndex, backendAcquireSemaphore);
          Globals::VK::queue[DeviceQueueType::GRAPHICS].present(pi);
        }
        else
          G_ASSERTF(0, "vulkan: %u present was ready to queue but not processed", presentSwapchains.size());
      }
    }

    if (lowLatencyModule)
      lowLatencyModule->setMarker(params.enginePresentFrameId, lowlatency::LatencyMarkerType::PRESENT_END);
  }
  frameReadySemaphoresForPresent.clear();

  Globals::timelines.get<TimelineManager::GpuExecute>().advance();
  Backend::gpuJob.restart();

  // if we acquired for frontend instead of moving semaphore to frontend, wait it right away
  // queue selection is a bit opportunistic here, beware!
  if (!is_null(backendAcquireSemaphore))
    Globals::VK::queue[DeviceQueueType::GRAPHICS].waitAcquireSemaphore(backendAcquireSemaphore, imgAcquireIndex,
      Backend::gpuJob.get());

  if (data->generateFaultReport)
    generateFaultReport();

  if (Globals::cfg.bits.allowPredictedLatencyWaitWorker)
  {
    Backend::latencyWaiter.update(
      Backend::timings.gpuWaitDuration + Backend::timings.presentWaitDuration + frameCallbacksDurationTicks);
    Backend::latencyWaiter.wait();
  }
}
