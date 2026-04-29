// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_spinlock.h>
#include <3d/dag_amdFsr.h>
#include <3d/dag_nvFeatures.h>

#include "os.h"
#include "render_state_system.h"
#include "util/scoped_timer.h"
#include "pipeline_barrier.h"
#include "async_completion_state.h"
#include "render_work.h"
#include "temp_buffers.h"
#include "frame_info.h"
#include "timelines.h"
#include "image_resource.h"
#include <osApiWrappers/dag_threads.h>
#include "execution_state.h"
#include "pipeline_state.h"
#include "pipeline_state_pending_references.h"
#include "util/fault_report.h"
#include "bindless.h"
#include "execution_sync.h"
#include "pipeline/compiler.h"
#include "sampler_resource.h"
#include "memory_heap_resource.h"
#include "predicted_latency_waiter.h"
#include "execution_scratch.h"
#include "frontend.h"

namespace drv3d_vulkan
{
struct TimestampQuery;
struct SwapchainMode;

enum class ExecutionMode
{
  INVALID,
  DEFERRED,
  THREADED,
  TRANSIENT
};

class DeviceContext //-V553
{
  struct WorkerThread : public DaThread
  {
    WorkerThread(uint64_t affinity) :
      // use higher priority insead of dedicated affinity, as we don't have system to reserve core properly
      DaThread("Vulkan Worker", 4096 << 10, cpujobs::DEFAULT_THREAD_PRIORITY + 1, affinity)
    {}
    // calls device.processCommandPipe() until termination is requested
    void execute() override;
  };

  eastl::unique_ptr<WorkerThread> worker;
  ExecutionMode executionMode = ExecutionMode::INVALID;

  void shutdownWorkerThread()
  {
    if (ExecutionMode::THREADED == executionMode)
    {
      worker->terminate(true);
      worker.reset();
    }
  }

  static uint64_t calculateWorkerAffinityMask()
  {
    const DataBlock *vulkan_block = ::dgs_get_settings()->getBlockByName("vulkan");
    if (vulkan_block)
    {
      const int workerAffinityOverride = vulkan_block->getInt("workerAffinity", -1);
      if (workerAffinityOverride >= 0)
        return workerAffinityOverride;
    }

    auto coreCount = cpujobs::get_core_count();
    // should make sure that we do not share with the main
    // thread in any way (via hypethreading and so on).
#if _TARGET_ANDROID
    if (coreCount > 2)
      // Fast cores have higher numbers, last one is main thread, previous is ours
      return 1ull << (cpujobs::get_core_count() - 2);
#elif _TARGET_C3

#else
    if (coreCount < 3)
      return coreCount < 2 ? 0x1 : 0x2; // main is always core0, so use whatver non core0 possible
    else
    {
      // use worker threads mask, leaving it for system to decide how to balance it
      // because using fixed one tends to get in conflict with other workload
      return WORKER_THREADS_AFFINITY_USE;
    }
#endif
    return ~0ull; // any core safety fallback
  }

  // internal state
  dag::Vector<FrameEvents *> frameEventCallbacks;
  eastl::vector<DeviceResetEventHandler *> deviceResetEventHandlers;

  OSSpinlock mutex;

  void verifyExecutionMode() { G_ASSERT(executionMode != ExecutionMode::INVALID); };
  void beforeSubmitReplay();
  void submitReplay();
  // we should do some cleanups/restarts when backend flush command are executed
  void afterBackendFlush();

  // temp buffers
  void cleanupFrontendReplayResources();

  // misc
  void reportAlliveObjects(FaultReportDump &dump);

  void setPipelineState();

public:
  ~DeviceContext() = default;

  DeviceContext(const DeviceContext &) = delete;
  DeviceContext &operator=(const DeviceContext &) = delete;

  DeviceContext(DeviceContext &&) = delete;
  DeviceContext &operator=(DeviceContext &&) = delete;
  DeviceContext();

  OSSpinlock &getFrontLock() { return mutex; }

  template <typename CmdType>
  __forceinline void dispatchCmd(const CmdType &cmd)
  {
    OSSpinlockScopedLock lockedDevice(mutex);
    dispatchCmdNoLock(cmd);
  }

  template <typename CmdType>
  __forceinline void dispatchCmdNoLock(const CmdType &cmd)
  {
    Frontend::replay->pushCmd(cmd);
    verifyExecutionMode();
  }

  template <typename CmdType>
  __forceinline void dispatchCmdWithStateSet(const CmdType &cmd)
  {
    OSSpinlockScopedLock lockedDevice(mutex);
    setPipelineState();
    dispatchCmdNoLock(cmd);
  }

  template <typename CmdType>
  __forceinline void dispatchPipeline(const CmdType &cmd, const char *exec_type)
  {
    {
      OSSpinlockScopedLock lockedDevice(mutex);
      setPipelineState();
      dispatchCmdNoLock(cmd);
    }
    executeDebugFlush(exec_type);
  }

  void shutdown();
  void initTempBuffersConfiguration();

  bool isWorkerRunning() { return executionMode == ExecutionMode::THREADED; }
  void toggleWorkerThread() { executionMode = ExecutionMode::TRANSIENT; }
  void initMode(ExecutionMode mode)
  {
    executionMode = mode;
    if (ExecutionMode::THREADED == executionMode)
    {
      worker.reset(new WorkerThread(calculateWorkerAffinityMask()));
      worker->start();
      // pin thread to a dedicated core to avoid performance penalties due to thread migration
    }
  }

  uint32_t getPiplineCompilationQueueLength();

  size_t getCurrentWorkItemId();

  void waitForItemPushSpace();

  void captureRenderPasses(bool capture_call_stack);
  void bindVertexBuffer(uint32_t stream, Buffer *buffer, uint32_t offset);
  void bindVertexStride(uint32_t stream, uint32_t stride);

  // workaround for pass splits
  //   will auto reorder copy before native render pass if copy requested inside native render pass
  //   this must be avoided, but higher level code can't provide proper usage due to old api / legacy reasons
  //   right now only in rare situations we have such issues, so every reorder is logged
  //   this reorder is batched, so if copy ranges overlap, error is generated
  //   yet if range is used twice before and after copy command inside render pass - rendering will be broken
  //   using more safe solution with discard is too greedy on memory
  void copyBufferDiscardReorderable(const BufferRef &source, BufferRef &dest, uint32_t src_offset, uint32_t dst_offset,
    uint32_t data_size);
  void copyBuffer(const BufferRef &source, const BufferRef &dest, uint32_t src_offset, uint32_t dst_offset, uint32_t data_size);
  void clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value,
    bool unordered = false);
  void clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value, bool unordered = false);
  void flushDraws();

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  void executeDebugFlush(const char *caller)
  {
    if (Globals::cfg.bits.flushAfterEachDrawAndDispatch)
      executeDebugFlushImpl(caller);
  }
  void executeDebugFlushImpl(const char *caller);
#else
  void executeDebugFlush(const char *) {};
#endif

  void copyImageToBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions, AsyncCompletionState *sync);
  void copyBufferToImage(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions, bool ordered_copy = false);
  void processAllPendingWork();
  void wait();
  // allows upload region overlap, keeps uploads ordered between each other
  void uploadBufferOrdered(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void uploadBuffer(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void downloadBuffer(const BufferRef &src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  BufferRef uploadToFrameMem(DeviceMemoryClass memory_class, uint32_t size, const void *src);
  // upload to framemem with memory class that is optimal to be used for device reads
  BufferRef uploadToDeviceFrameMem(uint32_t size, const void *src);
  void present(uint32_t engine_present_frame_id);
  void shutdownImmediateConstBuffers();
  void flushBufferToHost(const BufferRef &buffer, ValueRange<uint32_t> range);
  void waitForIfPending(AsyncCompletionState &sync);

  bool updateBindlessResource(uint32_t index, D3dResource *res);
  void updateBindlessResourceRange(D3DResourceType type, uint32_t index, const dag::ConstSpan<D3dResource *> &resources);
  void updateBindlessSampler(uint32_t index, const SamplerResource *sampler_res);
  void copyBindlessDescriptors(D3DResourceType type, uint32_t src, uint32_t dst, uint32_t count);
  void updateBindlessResourcesToNull(D3DResourceType type, uint32_t index, uint32_t count);

  void deleteAsyncCompletionStateOnFinish(AsyncCompletionState &sync);
  void generateFaultReport();
  void generateFaultReportAtFrameEnd();

  int getFramerateLimitingFactor();
  void advanceAndCheckTimingRecord();

  void registerFrameEventsCallback(FrameEvents *callback, bool useFront);
  void callFrameEndCallbacks();
  eastl::vector<DeviceResetEventHandler *> &getDeviceResetEventHandlers() { return deviceResetEventHandlers; }
};

} // namespace drv3d_vulkan
