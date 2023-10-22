#include "frame_info.h"
#include "device.h"
#include <perfMon/dag_statDrv.h>

using namespace drv3d_vulkan;

void FrameInfo::sendSignal()
{
  for (auto &&signal : signalRecivers)
    signal->completed();
  signalRecivers.clear();
}

void FrameInfo::addSignalReciver(AsyncCompletionState &reciver) { signalRecivers.push_back(&reciver); }

void FrameInfo::init()
{
  Device &device = get_device();
  VulkanDevice &vkDev = device.getVkDevice();

  uint32_t cmdQFamily = device.getQueue(DeviceQueueType::GRAPHICS).getFamily();
  bool resetCmdPool = device.getFeatures().test(DeviceFeaturesConfig::RESET_COMMAND_POOLS);

  VkCommandPoolCreateInfo cpci;
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.pNext = NULL;
  cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  if (!resetCmdPool)
    cpci.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = cmdQFamily;
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateCommandPool(vkDev.get(), &cpci, NULL, ptr(commandPool)));

  pendingCommandBuffers.reserve(COMMAND_BUFFER_ALLOC_BLOCK_SIZE);

  // alloc one block and return the allocated buffer to this block
  freeCommandBuffers.push_back(allocateCommandBuffer(vkDev));
  // remove block from pending
  pendingCommandBuffers.clear();

  frameDone = new ThreadedFence(vkDev, ThreadedFence::State::SIGNALED);

  initialized = true;
}

void FrameInfo::shutdown()
{
  G_ASSERT(initialized);
  initialized = false;

  VulkanDevice &vkDev = get_device().getVkDevice();

  frameDone->shutdown(vkDev);
  delete frameDone;
  frameDone = NULL;

  sendSignal();

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

  VULKAN_LOG_CALL(vkDev.vkDestroyCommandPool(vkDev.get(), commandPool, NULL));
  commandPool = VulkanNullHandle();

  finishCleanups();
  finishSemaphores();
  for (int i = 0; i < readySemaphores.size(); ++i)
    VULKAN_LOG_CALL(vkDev.vkDestroySemaphore(vkDev.get(), readySemaphores[i], NULL));
  clear_and_shrink(readySemaphores);

  pendingTimestamps.clear();
  finishShaderModules();
}

VulkanCommandBufferHandle FrameInfo::allocateCommandBuffer(VulkanDevice &device)
{
  if (freeCommandBuffers.empty())
  {
    freeCommandBuffers.resize(COMMAND_BUFFER_ALLOC_BLOCK_SIZE);
    VkCommandBufferAllocateInfo cbai;
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext = NULL;
    cbai.commandPool = commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = freeCommandBuffers.size();
    VULKAN_EXIT_ON_FAIL(device.vkAllocateCommandBuffers(device.get(), &cbai, ary(freeCommandBuffers.data())));
  }
  G_ASSERT(!freeCommandBuffers.empty());
  VulkanCommandBufferHandle cmd = freeCommandBuffers.back();
  freeCommandBuffers.pop_back();
  pendingCommandBuffers.push_back(cmd);
  return cmd;
}

VulkanSemaphoreHandle FrameInfo::allocSemaphore(VulkanDevice &device, bool auto_track)
{
  VulkanSemaphoreHandle sem;
  if (readySemaphores.empty())
  {
    VkSemaphoreCreateInfo sci;
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = NULL;
    sci.flags = 0;
    VULKAN_EXIT_ON_FAIL(device.vkCreateSemaphore(device.get(), &sci, NULL, ptr(sem)));
  }
  else
  {
    sem = readySemaphores.back();
    readySemaphores.pop_back();
  }
  if (auto_track)
    pendingSemaphores.push_back(sem);
  return sem;
}

void FrameInfo::submit()
{
  // we finish current frame after chunk of replay work
  // but after replay work is done frame is changed, so we must handle
  // internal backend queued objects after replay event here
  cleanups.backendAfterFrameSubmitCleanup(get_device().getContext().getBackend());
}

void FrameInfo::acquire(size_t timeline_abs_idx)
{
  if (!initialized)
    init();

  index = timeline_abs_idx;
  frameDone->reset(get_device().getVkDevice());
}

void FrameInfo::wait()
{
  TIME_PROFILE(vulkan_frame_info_wait);
  finishGpuWork();
  finishCmdBuffers();
  finishCleanups();
  finishSemaphores();

  finishTimeStamps();
  finishShaderModules();
}

void FrameInfo::cleanup() {}

void FrameInfo::process() {}

void FrameInfo::finishCleanups()
{
  for (CleanupQueue *i : cleanupsRefs)
    i->backendAfterGPUCleanup(get_device().getContext().getBackend());
  clear_and_shrink(cleanupsRefs);

  cleanups.backendAfterGPUCleanup(get_device().getContext().getBackend());
}

void FrameInfo::finishCmdBuffers()
{
  Device &device = get_device();
  VulkanDevice &vkDev = device.getVkDevice();

  freeCommandBuffers.reserve(freeCommandBuffers.size() + pendingCommandBuffers.size());
  if (device.getFeatures().test(DeviceFeaturesConfig::RESET_COMMAND_POOLS))
  {
    VkCommandPoolResetFlags poolResetFlags = device.getFeatures().test(DeviceFeaturesConfig::RESET_COMMANDS_RELEASE_TO_SYSTEM)
                                               ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
                                               : 0;
    VULKAN_LOG_CALL(vkDev.vkResetCommandPool(vkDev.get(), commandPool, poolResetFlags));
  }
  else
  {
    VkCommandBufferResetFlags bufferResetFlags = device.getFeatures().test(DeviceFeaturesConfig::RESET_COMMANDS_RELEASE_TO_SYSTEM)
                                                   ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT
                                                   : 0;
    for (VulkanCommandBufferHandle cmd : pendingCommandBuffers)
      VULKAN_LOG_CALL(vkDev.vkResetCommandBuffer(cmd, bufferResetFlags));
  }
  append_items(freeCommandBuffers, pendingCommandBuffers.size(), pendingCommandBuffers.data());
  pendingCommandBuffers.clear();
}

void FrameInfo::finishGpuWork()
{
  Device &device = get_device();
  VulkanDevice &vkDev = device.getVkDevice();

  int64_t &gpuWaitDuration = device.getContext().getBackend().gpuWaitDuration;
  if (!frameDone->isReady(vkDev))
  {
    gpuWaitDuration = 1;
    FrameTimingWatch watch(gpuWaitDuration);
    frameDone->wait(vkDev);
  }
  else
    gpuWaitDuration = 0;

  sendSignal();
}

void FrameInfo::finishSemaphores()
{
  append_items(readySemaphores, pendingSemaphores.size(), pendingSemaphores.data());
  pendingSemaphores.clear();
}

void FrameInfo::finishTimeStamps()
{
  VulkanDevice &vkDev = get_device().getVkDevice();

  for (auto &&ts : pendingTimestamps)
  {
    uint64_t temp;
    VULKAN_EXIT_ON_FAIL(vkDev.vkGetQueryPoolResults(vkDev.get(), ts.pool->pool, ts.index, 1, sizeof(uint64_t), &temp, sizeof(uint64_t),
      VK_QUERY_RESULT_64_BIT));
    ts.pool->values[ts.index].store(temp);
  }
  pendingTimestamps.clear();
}

void FrameInfo::finishShaderModules()
{
  VulkanDevice &vkDev = get_device().getVkDevice();

  for (auto &&module : deletedShaderModules)
  {
    VULKAN_LOG_CALL(vkDev.vkDestroyShaderModule(vkDev.get(), module->module, nullptr));
  }
  deletedShaderModules.clear();
}