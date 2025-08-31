// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>

#include "util/scoped_timer.h"
#include "frame_info.h"
#include "globals.h"
#include "device_queue.h"
#include "driver_config.h"
#include "backend.h"
#include "execution_timings.h"
#include "backend_interop.h"
#include "timelines.h"
#include "wrapped_command_buffer.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

void FrameInfo::QueueCommandBuffers::init(DeviceQueueType target_queue)
{
  VulkanDevice &vkDev = Globals::VK::dev;
  queue = target_queue;

  uint32_t cmdQFamily = Globals::VK::queue[queue].getFamily();
  bool resetCmdPool = Globals::cfg.bits.resetCommandPools;

  VkCommandPoolCreateInfo cpci;
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.pNext = NULL;
  cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  if (!resetCmdPool)
    cpci.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = cmdQFamily;
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateCommandPool(vkDev.get(), &cpci, VKALLOC(command_pool), ptr(commandPool)));

  pendingCommandBuffers.reserve(COMMAND_BUFFER_ALLOC_BLOCK_SIZE);
}

VulkanCommandBufferHandle FrameInfo::QueueCommandBuffers::allocateCommandBuffer()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  if (freeCommandBuffers.empty())
  {
    freeCommandBuffers.resize(COMMAND_BUFFER_ALLOC_BLOCK_SIZE);
    VkCommandBufferAllocateInfo cbai;
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext = NULL;
    cbai.commandPool = commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = freeCommandBuffers.size();
    VULKAN_EXIT_ON_FAIL(vkDev.vkAllocateCommandBuffers(vkDev.get(), &cbai, ary(freeCommandBuffers.data())));
  }
  G_ASSERT(!freeCommandBuffers.empty());
  VulkanCommandBufferHandle cmd = freeCommandBuffers.back();
  freeCommandBuffers.pop_back();
  pendingCommandBuffers.push_back(cmd);

  if (Globals::cfg.debugLevel > 0)
    Globals::Dbg::naming.setCommandBufferName(cmd,
      String(64, "Frame %u Pending order %u Queue %u", Backend::gpuJob->index, pendingCommandBuffers.size(), (uint32_t)queue));

  return cmd;
}

void FrameInfo::QueueCommandBuffers::shutdown()
{
  if (is_null(commandPool))
    return;

  VulkanDevice &vkDev = Globals::VK::dev;

  if (!freeCommandBuffers.empty())
  {
    VULKAN_LOG_CALL(vkDev.vkFreeCommandBuffers(vkDev.get(), commandPool, freeCommandBuffers.size(), ary(freeCommandBuffers.data())));
  }
  clear_and_shrink(freeCommandBuffers);

  if (!pendingCommandBuffers.empty())
  {
    VULKAN_LOG_CALL(
      vkDev.vkFreeCommandBuffers(vkDev.get(), commandPool, pendingCommandBuffers.size(), ary(pendingCommandBuffers.data())));
  }
  clear_and_shrink(pendingCommandBuffers);

  VULKAN_LOG_CALL(vkDev.vkDestroyCommandPool(vkDev.get(), commandPool, VKALLOC(command_pool)));
  commandPool = VulkanNullHandle();
}

void FrameInfo::QueueCommandBuffers::finishCmdBuffers()
{
  VulkanDevice &vkDev = Globals::VK::dev;

  freeCommandBuffers.reserve(freeCommandBuffers.size() + pendingCommandBuffers.size());
  if (Globals::cfg.bits.resetCommandPools)
  {
    VkCommandPoolResetFlags poolResetFlags =
      Globals::cfg.bits.resetCommandsReleaseToSystem ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0;
    VULKAN_LOG_CALL(vkDev.vkResetCommandPool(vkDev.get(), commandPool, poolResetFlags));
  }
  else
  {
    VkCommandBufferResetFlags bufferResetFlags =
      Globals::cfg.bits.resetCommandsReleaseToSystem ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
    for (VulkanCommandBufferHandle cmd : pendingCommandBuffers)
      VULKAN_LOG_CALL(vkDev.vkResetCommandBuffer(cmd, bufferResetFlags));
  }
  append_items(freeCommandBuffers, pendingCommandBuffers.size(), pendingCommandBuffers.data());
  pendingCommandBuffers.clear();
}

void FrameInfo::CommandBuffersGroup::init()
{
  if (Globals::cfg.bits.allowMultiQueue)
  {
    for (uint32_t i = 0; i < (uint32_t)DeviceQueueType::COUNT; ++i)
      group[i].init(static_cast<DeviceQueueType>(i));
  }
  else
    group[0].init(DeviceQueueType::GRAPHICS);
}

void FrameInfo::CommandBuffersGroup::shutdown()
{
  for (QueueCommandBuffers &i : group)
    i.shutdown();
}

void FrameInfo::CommandBuffersGroup::finishCmdBuffers()
{
  if (Globals::cfg.bits.allowMultiQueue)
  {
    for (QueueCommandBuffers &i : group)
      i.finishCmdBuffers();
  }
  else
    group[0].finishCmdBuffers();
}


void FrameInfo::init()
{
  commandBuffers.init();
  frameDone = new ThreadedFence(ThreadedFence::State::SIGNALED);
  if (Globals::cfg.bits.allowAsyncReadback)
    readbackDone = new ThreadedFence(ThreadedFence::State::SIGNALED);
  execTracker.init();

  initialized = true;
}

void FrameInfo::shutdown()
{
  G_ASSERT(initialized);
  initialized = false;

  VulkanDevice &vkDev = Globals::VK::dev;

  frameDone->shutdown();
  delete frameDone;
  frameDone = nullptr;
  if (readbackDone)
  {
    readbackDone->shutdown();
    delete readbackDone;
    readbackDone = nullptr;
  }

  commandBuffers.shutdown();

  finishCleanups();
  finishSemaphores();
  for (Tab<VulkanSemaphoreHandle> &i : pendingSemaphores)
  {
    for (VulkanSemaphoreHandle j : i)
      VULKAN_LOG_CALL(vkDev.vkDestroySemaphore(vkDev.get(), j, VKALLOC(semaphore)));
    i.clear();
  }

  finishGpuEvents();
  clear_and_shrink(gpuEvents);

  pendingSemaphoresRingIdx = 0;
  for (int i = 0; i < readySemaphores.size(); ++i)
    VULKAN_LOG_CALL(vkDev.vkDestroySemaphore(vkDev.get(), readySemaphores[i], VKALLOC(semaphore)));
  clear_and_shrink(readySemaphores);

  pendingTimestamps = nullptr;
  pendingOcclusionQueries = nullptr;
  finishShaderModules();
  execTracker.shutdown();
}

VulkanCommandBufferHandle FrameInfo::allocateCommandBuffer(DeviceQueueType queue)
{
  return commandBuffers[queue].allocateCommandBuffer();
}

void FrameInfo::recycleSemaphore(VulkanSemaphoreHandle sem) { readySemaphores.push_back(sem); }

VulkanSemaphoreHandle FrameInfo::allocSemaphore()
{
  VulkanSemaphoreHandle sem;
  if (readySemaphores.empty())
  {
    VkSemaphoreCreateInfo sci;
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = NULL;
    sci.flags = 0;
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSemaphore(Globals::VK::dev.get(), &sci, VKALLOC(semaphore), ptr(sem)));
  }
  else
  {
    sem = readySemaphores.back();
    readySemaphores.pop_back();
  }
  return sem;
}

void FrameInfo::addPendingSemaphore(VulkanSemaphoreHandle sem)
{
  for (VulkanSemaphoreHandle i : pendingSemaphores[pendingSemaphoresRingIdx])
  {
    if (i == sem)
      return;
  }
  pendingSemaphores[pendingSemaphoresRingIdx].push_back(sem);
}

void FrameInfo::submit()
{
  // we finish current frame after chunk of replay work
  // but after replay work is done frame is changed, so we must handle
  // internal backend queued objects after replay event here
  cleanups.backendAfterFrameSubmitCleanup();
  {
    WinAutoLock lock(Backend::interop.pendingGpuWork.cs);
    Backend::interop.pendingGpuWork.gpuFence = frameDone;
    Backend::interop.pendingGpuWork.replayId = replayId;
  }
}

void FrameInfo::acquire(size_t timeline_abs_idx)
{
  if (!initialized)
    init();

  index = timeline_abs_idx;
  frameDone->reset();
  execTracker.restart(index);
}

void FrameInfo::wait()
{
  TIME_PROFILE(vulkan_frame_info_wait);
  finishGpuWork();
  finishGpuEvents();
  commandBuffers.finishCmdBuffers();
  finishCleanups();
  finishSemaphores();

  if (pendingTimestamps)
  {
    pendingTimestamps->fillDataFromPool();
    pendingTimestamps = nullptr;
  }
  if (pendingOcclusionQueries)
  {
    pendingOcclusionQueries->fillDataFromPool();
    pendingOcclusionQueries = nullptr;
  }
  finishShaderModules();
  execTracker.verify();
}

void FrameInfo::cleanup() {}

void FrameInfo::process() {}

void FrameInfo::finishCleanups() { cleanups.backendAfterGPUCleanup(); }

void FrameInfo::finishGpuWork()
{
  int64_t &gpuWaitDuration = Backend::timings.gpuWaitDuration;
  {
    ScopedTimerTicks watch(gpuWaitDuration);
    Backend::interop.pendingGpuWork.cs.lock();
    bool sharedWait = frameDone == Backend::interop.pendingGpuWork.gpuFence;
    if (!sharedWait)
      Backend::interop.pendingGpuWork.cs.unlock();
    if (!frameDone->isReady())
      frameDone->wait();

    if (sharedWait)
    {
      Backend::interop.pendingGpuWork.gpuFence = nullptr;
      Backend::interop.pendingGpuWork.cs.unlock();
    }
  }
  if (readbackDone && !readbackDone->isSignaled())
  {
    // should not take too long, yet better to know if something goes wrong
    TIME_PROFILE(vulkan_readback_gpu_wait);
    if (!readbackDone->isReady())
      readbackDone->wait();
  }
  Backend::interop.lastGPUCompletedReplayWorkId.store(replayId, std::memory_order_release); //-V1020
}

void FrameInfo::finishSemaphores()
{
  pendingSemaphoresRingIdx = (pendingSemaphoresRingIdx + 1) % GPU_TIMELINE_HISTORY_SIZE;
  Tab<VulkanSemaphoreHandle> &finishedSems = pendingSemaphores[pendingSemaphoresRingIdx];
  append_items(readySemaphores, finishedSems.size(), finishedSems.data());
  finishedSems.clear();
}

void FrameInfo::finishShaderModules()
{
  VulkanDevice &vkDev = Globals::VK::dev;

  // we don't count references to modules, yet keep references to them in pipeline compiler
  // for simplicity, process deletions when there is nothing queued to avoid deleting module too early
  if (Backend::pipelineCompiler.getQueueLength())
    return;

  for (auto &&module : deletedShaderModules)
  {
    VULKAN_LOG_CALL(vkDev.vkDestroyShaderModule(vkDev.get(), module->module, VKALLOC(shader_module)));
  }
  deletedShaderModules.clear();
}

void FrameInfo::finishGpuEvents()
{
  // note: this may be inefficient, but resetting on GPU timeline is also inefficient, reiterate on this if needed
  for (VulkanEventHandle i : gpuEvents)
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyEvent(Globals::VK::dev.get(), i, VKALLOC(event)));
  gpuEvents.clear();
}

VulkanEventHandle FrameInfo::allocateGpuEvent()
{
  VulkanEventHandle ret;
  VkEventCreateInfo eci = {VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, nullptr, VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR};
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateEvent(Globals::VK::dev.get(), &eci, VKALLOC(event), ptr(ret)));
  gpuEvents.push_back(ret);
  return ret;
}
