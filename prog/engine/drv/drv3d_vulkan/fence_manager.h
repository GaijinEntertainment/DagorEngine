#pragma once

#include <generic/dag_objectPool.h>
#include "vulkan_device.h"
#include <generic/dag_tab.h>

namespace drv3d_vulkan
{
class ThreadedFence
{
public:
  // time out if we wait >20seconds for fence to be submitted
  // and if we wait GPU for >2 seconds time out too
  static constexpr int SUBMISSION_TIMEOUT = 20000000;

  enum class State
  {
    NOT_SIGNALED,
    SUBMITTED,
    SIGNALED,
  };

  ThreadedFence(VulkanDevice &device, State s) : state(s)
  {
    G_ASSERT(s != State::SUBMITTED);
    VkFenceCreateInfo fci;
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = NULL;
    fci.flags = s == State::SIGNALED ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VULKAN_EXIT_ON_FAIL(device.vkCreateFence(device.get(), &fci, NULL, ptr(fence)));
  }

  inline void setAsSubmited() { state = State::SUBMITTED; }

  inline bool isSignaled() { return (State::SIGNALED == state); }

  inline bool isReady(VulkanDevice &device)
  {
    if (State::NOT_SIGNALED == state)
      return false;
    if (State::SUBMITTED == state)
      if (VULKAN_LOG_CALL_R(device.vkGetFenceStatus(device.get(), fence)) == VK_SUCCESS)
      {
        state = State::SIGNALED;
        return true;
      }
    return isSignaled();
  }

  inline void wait(VulkanDevice &device)
  {
    uint64_t waitStart = ref_time_ticks();
    uint64_t tryNumber = 1;
    while (State::NOT_SIGNALED == state)
    {
      if (ref_time_delta_to_usec(ref_time_ticks() - waitStart) >= SUBMISSION_TIMEOUT)
      {
        logerr("vulkan: submission of fence takes too long (above %u seconds) try %u", SUBMISSION_TIMEOUT / 1000000, tryNumber);
        waitStart = ref_time_ticks();
        ++tryNumber;
      }

      sleep_usec(0); // sleep_usec does not invoke ScopeLockProfiler. Total wait time is profiled externally with vulkan_sync_wait.
    }

    if (gpuFenceWait(device) != State::SIGNALED)
      DAG_FATAL("vulkan: waited fence is unreachable");
  }

  inline void shutdown(VulkanDevice &device)
  {
    gpuFenceWait(device);
    VULKAN_LOG_CALL(device.vkDestroyFence(device.get(), fence, NULL));
    state = State::NOT_SIGNALED;
    fence = VulkanNullHandle();
  }

  inline VulkanFenceHandle get() const { return fence; }

  inline void reset(VulkanDevice &device)
  {
    if (gpuFenceWait(device) != State::SIGNALED)
      return;
    VULKAN_EXIT_ON_FAIL(device.vkResetFences(device.get(), 1, ptr(fence)));
    state = State::NOT_SIGNALED;
  }

  void setSignaledExternally() { state = State::SIGNALED; }

private:
  State gpuFenceWait(VulkanDevice &device)
  {
    State stCopy = state;
    if (State::SUBMITTED != stCopy)
      return stCopy;

    VkResult waitRet;
    if (d3d::get_driver_desc().issues.hasPollDeviceFences)
    {
      for (;;)
      {
        waitRet = device.vkGetFenceStatus(device.get(), fence);
        if (waitRet == VK_SUCCESS || VULKAN_FAIL(waitRet))
          break;
        sleep_usec(0);
      }
    }
    else
    {
      // wait with timeout to be verbose on error result
      // and avoid possible (buggy) endless wait on systems that supports it
      // if timeout reached: check work load & integrity
      //  -reduce complexity/add flushes if work load is too long
      //  -fix workload/shaders integrity at caller site (for(;;) in shaders, -1 in dispatch groups, etc.)
      waitRet = device.vkWaitForFences(device.get(), 1, ptr(fence), VK_TRUE, GENERAL_GPU_TIMEOUT_NS);
      while (waitRet == VK_TIMEOUT)
      {
        // be verbose only in dev builds, as some systems can drop into sleep mode right inside vkWaitForFences
        // causing this method to fail and crash the game
        // still, use non infinite timeout, as this can be unsafe
        //(blocking core from being used by other threads, sometimes)
#if DAGOR_DBGLEVEL > 0 && !_TARGET_ANDROID
        logerr("vulkan: timeout waiting for GPU fence, retrying wait");
#endif
        waitRet = device.vkWaitForFences(device.get(), 1, ptr(fence), VK_TRUE, GENERAL_GPU_TIMEOUT_NS);
      }
    }

    if (VULKAN_FAIL(waitRet))
    {
      const bool ignoreDeviceLost = d3d::get_driver_desc().issues.hasIgnoreDeviceLost;
      static int ignoredDeviceLostCount = 0;

      // VK_ERROR_INITIALIZATION_FAILED is some form of DEVICE_LOST on Mali.
      if (ignoreDeviceLost && (waitRet == VK_ERROR_DEVICE_LOST || waitRet == VK_ERROR_INITIALIZATION_FAILED))
      {
        if (ignoredDeviceLostCount < 3)
          logerr("vulkan: GPU device lost detected and ignored (wait with timeout, code: %08lX)", waitRet);
        ignoredDeviceLostCount++;
      }
      else
      {
        generateFaultReport();
        DAG_FATAL("vulkan: GPU crash detected (code: %08lX)", waitRet);
      }

      // if fatal is skipped, just wait without timeout
      waitRet = device.vkWaitForFences(device.get(), 1, ptr(fence), VK_TRUE, UINT64_MAX);
      if (ignoreDeviceLost && (waitRet == VK_ERROR_DEVICE_LOST || waitRet == VK_ERROR_INITIALIZATION_FAILED))
      {
        if (ignoredDeviceLostCount < 3)
          logerr("vulkan: GPU device lost detected and ignored (wait without timeout, code: %08lX)", waitRet);
        ignoredDeviceLostCount++;
      }
      else
        VULKAN_EXIT_ON_FAIL(waitRet);
    }
    state = State::SIGNALED;

    return State::SIGNALED;
  }

  VulkanFenceHandle fence;
  std::atomic<State> state;
};

class FenceManager
{
  ObjectPool<ThreadedFence> fencePool;
  eastl::vector<ThreadedFence *> pool;

public:
  ThreadedFence *alloc(VulkanDevice &device, bool ready)
  {
    ThreadedFence *fence = nullptr;
    if (!pool.empty())
    {
      fence = pool.back();
      pool.pop_back();
      if (!ready)
        fence->reset(device);
    }
    else
      fence = fencePool.allocate(device, ready ? ThreadedFence::State::SIGNALED : ThreadedFence::State::NOT_SIGNALED);

    return fence;
  }
  void free(ThreadedFence *fence)
  {
    G_ASSERTF(fence->isSignaled(), "vulkan: fence should be signaled when pushed back into pool!");
    pool.push_back(fence);
  }
  void resetAll(VulkanDevice &device)
  {
    for (auto &&fence : pool)
    {
      fence->shutdown(device);
      fencePool.free(fence);
    }
    pool.clear();
  }
};
} // namespace drv3d_vulkan