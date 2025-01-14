// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_commands.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING
#include <osApiWrappers/dag_spinlock.h>
#include <3d/dag_amdFsr.h>

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
#include "stacked_profile_events.h"
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
    WorkerThread(uint64_t affinity)
      // Not less than 4mb required for molten vk
      :
      DaThread("Vulkan Worker", 4096 << 10, 0, affinity)
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
      // main at core2 with HT core3, core0 and core1 are HT + "system used"
      // use core4 if possible, otherwise core0/core1
      if (coreCount <= 4)
        return 0x2; // core1
      return 0x10;  // core4
    }
#endif
    return ~0ull; // any core safety fallback
  }

  // internal state
  VulkanDevice &vkDev;
  eastl::vector<FrameEvents *> frameEventCallbacks;

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
  void executeDebugFlush(const char *caller);

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
  __forceinline void dispatchCommand(const CmdType &cmd)
  {
    OSSpinlockScopedLock lockedDevice(mutex);
    dispatchCommandNoLock(cmd);
  }

  template <typename CmdType>
  __forceinline void dispatchCommandWithStateSet(const CmdType &cmd)
  {
    OSSpinlockScopedLock lockedDevice(mutex);
    setPipelineState();
    dispatchCommandNoLock(cmd);
  }

  template <typename CmdType>
  __forceinline void dispatchPipeline(const CmdType &cmd, const char *exec_type)
  {
    {
      OSSpinlockScopedLock lockedDevice(mutex);
      setPipelineState();
      dispatchCommandNoLock(cmd);
    }
    executeDebugFlush(exec_type);
  }

  template <typename CmdType>
  __forceinline void dispatchCommandNoLock(const CmdType &cmd)
  {
    Frontend::replay->pushCommand(cmd);
    verifyExecutionMode();
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
  void startPreRotate(uint32_t binding_slot);
  void holdPreRotateStateForOneFrame();

  void updateDebugUIPipelinesData();
  void setPipelineUsability(ProgramID program, bool value);
  void captureRenderPasses(bool capture_call_stack);
  void setConstRegisterBuffer(const BufferRef &ref, ShaderStage stage);

  void bindVertexBuffer(uint32_t stream, Buffer *buffer, uint32_t offset);
  void bindVertexStride(uint32_t stream, uint32_t stride);

  void compileGraphicsPipeline(const VkPrimitiveTopology top);
  void compileComputePipeline();
  uint64_t getTimestampResult(TimestampQueryId query_id);
  TimestampQueryId insertTimestamp();

  void clearView(int clear_flags);
  void allowOpLoad();
  void nativeRenderPassChanged();
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
  void pushEvent(const char *name);
  void clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value);
  void clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value, bool unordered = false);
  void copyImage(Image *src, Image *dst, const VkImageCopy *regions, uint32_t region_count, uint32_t src_mip, uint32_t dst_mip,
    uint32_t mip_count, bool unordered = false);
  void resolveMultiSampleImage(Image *src, Image *dst);
  void flushDraws();
  void copyImageToBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions, AsyncCompletionState *sync);
  void copyBufferToImage(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions, bool seal);
  void copyBufferToImageOrdered(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions);
  void blitImage(Image *src, Image *dst, const VkImageBlit &region, bool whole_subres);
  void processAllPendingWork();
  void wait();
  void beginSurvey(uint32_t name);
  void endSurvey(uint32_t name);
  // allows upload region overlap, keeps uploads ordered between each other
  void uploadBufferOrdered(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void uploadBuffer(const BufferRef &src, const BufferRef &dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void downloadBuffer(const BufferRef &src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void destroyBuffer(Buffer *buffer);
  BufferRef discardBuffer(const BufferRef &srcRef, DeviceMemoryClass memory_class, FormatStore view_format, uint32_t bufFlags,
    uint32_t dynamic_size);
  BufferRef uploadToFrameMem(DeviceMemoryClass memory_class, uint32_t size, const void *src);
  // upload to framemem with memory class that is optimal to be used for device reads
  BufferRef uploadToDeviceFrameMem(uint32_t size, const void *src);
  void destroyRenderPassResource(RenderPassResource *rp);
  void destroyImageDelayed(Image *img);
  void destroyImage(Image *img);
  void present();
  void changeSwapchainMode(const SwapchainMode &new_mode);
  void shutdownSwapchain();
  void shutdownImmediateConstBuffers();
  void addRenderState(shaders::DriverRenderStateId id, const shaders::RenderState &render_state_data);
  void addPipelineCache(VulkanPipelineCacheHandle cache);
  void generateMipmaps(Image *img);
  void flushBufferToHost(const BufferRef &buffer, ValueRange<uint32_t> range);
  void waitForIfPending(AsyncCompletionState &sync);
  void resourceBarrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline);

  void updateBindlessResource(uint32_t index, D3dResource *res);
  void updateBindlessSampler(uint32_t index, SamplerState samplerInfo);
  void copyBindlessDescriptors(uint32_t resource_type, uint32_t src, uint32_t dst, uint32_t count);
  void updateBindlessResourcesToNull(uint32_t resource_type, uint32_t index, uint32_t count);

  void deleteAsyncCompletionStateOnFinish(AsyncCompletionState &sync);
  void generateFaultReport();
  void generateFaultReportAtFrameEnd();

  void writeDebugMessage(const char *msg, intptr_t msg_length, intptr_t severity);

  void executeFSR(amd::FSR *fsr, const amd::FSR::UpscalingArgs &params);

#if D3D_HAS_RAY_TRACING
  void deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc);
  void deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc);
  void traceRays(Buffer *ray_gen_table, uint32_t ray_gen_offset, Buffer *miss_table, uint32_t miss_offset, uint32_t miss_stride,
    Buffer *hit_table, uint32_t hit_offset, uint32_t hit_stride, Buffer *callable_table, uint32_t callable_offset,
    uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth);
#endif

  void addShaderModule(uint32_t id, eastl::unique_ptr<ShaderModuleBlob> shdr_module);
  void removeShaderModule(uint32_t blob_id);

  void addGraphicsProgram(ProgramID program, const ShaderModuleUse &vs_blob, const ShaderModuleUse &fs_blob,
    const ShaderModuleUse *gs_blob, const ShaderModuleUse *tc_blob, const ShaderModuleUse *te_blob);
  void addComputeProgram(ProgramID program, eastl::unique_ptr<ShaderModuleBlob> shdr_module, const ShaderModuleHeader &header);
  void removeProgram(ProgramID program);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void attachComputeProgramDebugInfo(ProgramID program, eastl::unique_ptr<ShaderDebugInfo> dbg);
#endif

  void placeAftermathMarker(const char *name);

  int getFramerateLimitingFactor();

  void advanceAndCheckTimingRecord();

  void registerFrameEventsCallback(FrameEvents *callback, bool useFront);
  void callFrameEndCallbacks();

  void getWorkerCpuCore(int *core, int *thread_id);
};

} // namespace drv3d_vulkan
