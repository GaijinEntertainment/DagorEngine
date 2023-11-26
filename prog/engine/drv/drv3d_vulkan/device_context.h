#pragma once

#include <atomic>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_miscApi.h>
#if D3D_HAS_RAY_TRACING
#include "raytrace_state.h"
#endif
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

namespace drv3d_vulkan
{
struct TimestampQuery;
class Device;
class Buffer;
class DeviceContext;
struct SwapchainMode;
#if D3D_HAS_RAY_TRACING
class RaytraceAccelerationStructure;
struct DeviceRaytraceData;
#endif

struct DeviceFeaturesConfig
{
  enum Bits
  {
    USE_DEDICATED_MEMORY_FOR_IMAGES,
    USE_DEDICATED_MEMORY_FOR_RENDER_TARGETS,
    RESET_DESCRIPTOR_SETS_ON_FRAME_START,
    RESUSE_DESCRIPTOR_SETS,
    RESET_COMMAND_POOLS,
    RESET_COMMANDS_RELEASE_TO_SYSTEM,
    USE_ASYNC_BUFFER_COPY,
    USE_ASNYC_IMAGE_COPY,
    DISABLE_RENDER_TO_3D_IMAGE,
    OPTIMIZE_BUFFER_UPLOADS,
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
    FLUSH_AFTER_EACH_DRAW_AND_DISPATCH,
#endif
    USE_DEFERRED_COMMAND_EXECUTION,
    USE_THREADED_COMMAND_EXECUTION,
    COMMAND_MARKER,
    HAS_COHERENT_MEMORY,
    FORCE_GPU_HIGH_POWER_STATE_ON_WAIT,
    UPDATE_DESCRIPTOR_SETS,
    ALLOW_DEBUG_MARKERS,
    HEADLESS,
    PRE_ROTATION,
    RECORD_COMMAND_CALLER,
    KEEP_LAST_RENDERED_IMAGE,
    ALLOW_BUFFER_DMA_LOCK_PATH,
    COUNT,
    INVLID = COUNT
  };
  typedef eastl::bitset<COUNT> Type;
};

#if VULKAN_RECORD_TIMING_DATA
typedef ScopedTimerTicks FrameTimingWatch;
#else
class FrameTimingWatch
{
public:
  FrameTimingWatch(int64_t &) {}
};
#endif

struct ContextState
{
  PipelineStageStateBase stageState[STAGE_MAX_EXT];

  eastl::vector<VulkanCommandBufferHandle> cmdListsToSubmit;
  TimelineSpan<TimelineManager::GpuExecute> frame;
  eastl::vector<eastl::unique_ptr<ShaderModule>> shaderModules;
  RenderStateSystem::Backend renderStateSystem;
  BindlessManagerBackend bindlessManagerBackend{};

  ContextState(TimelineManager &tl_man) : frame(tl_man) {}

  void onFrameStateInvalidate()
  {
    for (auto &&stage : stageState)
      stage.invalidateState();
  }
};

struct ContextFrontend
{
  TempBufferManager tempBufferManagers[TempBufferManager::TYPE_COUNT];
  WinCritSec tempBufferHoldersPoolGuard;
  ObjectPool<TempBufferHolder> tempBufferHoldersPool;

  eastl::vector<AsyncCompletionState *> completionStateRefs;
  // FIXME: it is not tied and not checked with gpu exec timeline
  uint32_t frameIndex = 0;
  TimelineSpan<TimelineManager::CpuReplay> replayRecord;
  uint32_t lastVisibleAllocations = 0;

  eastl::vector<FrameEvents *> frameEventCallbacks;

#if VULKAN_RECORD_TIMING_DATA
  Drv3dTimings timingHistory[FRAME_TIMING_HISTORY_LENGTH]{};
#endif
  uint32_t completedFrameIndex = 0;
  int64_t lastPresentTimeStamp = 0;
  int64_t frontendBackendWaitDuration = 0;

  PipelineState pipelineState;

  ContextFrontend(TimelineManager &tl_man) : replayRecord(tl_man) { pipelineState.reset(); }
};

struct ContextBackend
{
  // state used by a execution context
  ContextState contextState;
  ExecutionState executionState;
  PipelineState pipelineState;
  PipelineStatePendingReferenceList pipelineStatePendingCleanups;
  ImmediateConstBuffer immediateConstBuffers[STAGE_MAX_EXT];
  ExecutionSyncTracker syncTrack;

  eastl::vector<FrameEvents *> frameEventCallbacks;

  struct DiscardNotify
  {
    Buffer *oldBuf;
    BufferRef newBuf;
    uint32_t flags;
  };
  eastl::vector<DiscardNotify> delayedDiscards;

  PipelineCompiler pipelineCompiler;

  int64_t gpuWaitDuration = 0;
  int64_t acquireBackBufferDuration = 0;
  int64_t workWaitDuration = 0;
  int64_t presentWaitDuration = 0;

  int64_t lastMemoryStatTime = 0;
  int64_t memoryStatisticsPeriod = 0;

  ContextBackend(TimelineManager &tl_man) : contextState(tl_man), pipelineCompiler(tl_man)
  {
    executionState.reset();
    pipelineState.reset();
  }
};

enum class ExecutionMode
{
  INVALID,
  DEFERRED,
  THREADED,
};

enum class ExecutionNode
{
  PS,
  CS,
  RP,
  CUSTOM
};

class ExecutionContext
{

public:
  Device &device;
  ContextBackend &back;
  Swapchain &swapchain;
  VulkanDevice &vkDev;

  // work set that is worked on
  RenderWork &data;

  // pointer and index to currently executed command
  const void *cmd = nullptr;
  uint32_t cmdIndex = 0;
  bool flushProcessed = false;

  bool renderAllowed = false;
  // counts amount of non-indirected drawcalls
  uint32_t directDrawCount = 0;
  // store amount of non-indirected drawcalls at start of survey
  // to avoid waiting empty surveys as they can hang on some GPUs
  uint32_t directDrawCountInSurvey = 0;
  VulkanCommandBufferHandle frameCore = VulkanHandle();

  typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::EXECUTION_PRIMARY> PrimaryPipelineBarrier;
  typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::EXECUTION_SECONDARY> SecondaryPipelineBarrier;

  void beginPassInternal(RenderPassClass *pass_class, VulkanFramebufferHandle fb_handle, VkRect2D area);
  void allocFrameCore();
  String getCurrentCmdCaller();
  void reportMissingPipelineComponent(const char *component);

private:
  void writeExectionChekpoint(VulkanCommandBufferHandle cb, VkPipelineStageFlagBits stage);

  VulkanCommandBufferHandle allocAndBeginCommandBuffer();
  VulkanCommandBufferHandle flushTimestampQueryResets(VulkanCommandBufferHandle cmd);
  VulkanCommandBufferHandle flushImageDownloads(VulkanCommandBufferHandle cmd);
  VulkanCommandBufferHandle flushBufferDownloads(VulkanCommandBufferHandle cmd);
  void flushBufferUploads(VulkanCommandBufferHandle cmd);
  void flushOrderedBufferUploads(VulkanCommandBufferHandle cmd);
  VulkanCommandBufferHandle flushBufferToHostFlushes(VulkanCommandBufferHandle cmd);
  void flushImageUploads();
  void flushImageUploadsIter(uint32_t start, uint32_t end);
  void flushUnorderedImageColorClears();
  void flushUnorderedImageDepthStencilClears();
  void flushUnorderedImageCopies();

  void printMemoryStatistics();
  void flush(ThreadedFence *fence);

  void applyStateChanges();
  void applyQueuedDiscards();

  void switchFrameCore();
  void onFrameCoreReset();

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  static thread_local ExecutionContext *tlsDbgActiveInstance;
#endif

public:
  ExecutionContext() = delete;
  ~ExecutionContext()
  {
    G_ASSERT(flushProcessed);
    back.executionState.setExecutionContext(nullptr);
#if VULKAN_VALIDATION_COLLECT_CALLER > 0
    tlsDbgActiveInstance = nullptr;
#endif
  }
  ExecutionContext(const ExecutionContext &) = delete;
  ExecutionContext &operator=(const ExecutionContext &) = delete;
  ExecutionContext(ExecutionContext &&) = delete;
  ExecutionContext &operator=(ExecutionContext &&) = delete;

  ExecutionContext(RenderWork &work_item);

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  static ExecutionContext *dbg_get_active_instance() { return tlsDbgActiveInstance; }
#else
  static ExecutionContext *dbg_get_active_instance() { return nullptr; }
#endif

  void endImmediateExecution() { flushProcessed = true; }
  void beginCmd(const void *ptr);
  void endCmd();
  GraphicsPipelineStateDescription &getGraphicsPipelineState();
  FramebufferState &getFramebufferState();

  // processes out-of-order parts of frame that can affect orderd frame
  void prepareFrameCore();
  void queueBufferDiscard(Buffer *old_buf, const BufferRef &new_buf, uint32_t flags);
  uint32_t beginVertexUserData(uint32_t stride);
  void endVertexUserData(uint32_t stride);
  void invalidateActiveGraphicsPipeline();
  void startPreRotate(uint32_t binding_slot);
  void holdPreRotateStateForOneFrame();
  Image *getSwapchainColorImage();
  Image *getSwapchainPrimaryDepthStencilImage();
  Image *getSwapchainDepthStencilImage(VkExtent2D extent);

  void writeToDebug(StringIndexRef index);
  void writeDebugMessage(StringIndexRef message_index, int severity);
  void flushDraws();
  bool isDebugEventsAllowed();
  void insertEvent(const char *marker, uint32_t color = 0xFFFFFFFF);
  void pushEventRaw(const char *marker, uint32_t color = 0xFFFFFFFF);
  void popEventRaw();
  void pushEvent(StringIndexRef name);
  void popEvent();
  VulkanImageViewHandle getImageView(Image *img, ImageViewState state);
  void beginQuery(VulkanQueryPoolHandle pool, uint32_t index, VkQueryControlFlags flags);
  void endQuery(VulkanQueryPoolHandle pool, uint32_t index);
  void writeTimestamp(const TimestampQueryRef &query);
  void wait(ThreadedFence *fence);
  void beginConditionalRender(const BufferRef &buffer, VkDeviceSize offset);
  void endConditionalRender();
  void addShaderModule(const ShaderModuleBlob *sci, uint32_t id);
  void removeShaderModule(uint32_t id);
  void addComputePipeline(ProgramID program, const ShaderModuleBlob *sci, const ShaderModuleHeader &header);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void attachComputePipelineDebugInfo(ProgramID program, const ShaderDebugInfo *info);
#endif
  void addGraphicsPipeline(ProgramID program, uint32_t vs, uint32_t fs, uint32_t gs, uint32_t tc, uint32_t te);
  void removeProgram(ProgramID prog);
  void addRenderState(shaders::DriverRenderStateId id, const shaders::RenderState &render_state_data);
  void addPipelineCache(VulkanPipelineCacheHandle cache);
  void checkAndSetAsyncCompletionState(AsyncCompletionState *sync);
  void addSyncEvent(AsyncCompletionState *sync);
#if D3D_HAS_RAY_TRACING
  void addRaytracePipeline(ProgramID program, uint32_t max_recursion, uint32_t shader_count, const ShaderModuleUse *shaders,
    uint32_t group_count, const RaytraceShaderGroup *groups);
  void copyRaytraceShaderGroupHandlesToMemory(ProgramID program, uint32_t first_group, uint32_t group_count, uint32_t size, void *ptr);
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  void buildAccelerationStructure(VkAccelerationStructureTypeKHR type, uint32_t instance_count, Buffer *instance_buffer,
    uint32_t instance_offset, uint32_t geometry_count, uint32_t first_geometry, VkBuildAccelerationStructureFlagsKHR flags,
    VkBool32 update, RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, Buffer *scratch);
#endif
  void traceRays(BufferRef ray_gen_table, BufferRef miss_table, BufferRef hit_table, BufferRef callable_table, uint32_t ray_gen_offset,
    uint32_t miss_offset, uint32_t miss_stride, uint32_t hit_offset, uint32_t hit_stride, uint32_t callable_offset,
    uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth);
#endif
  void resolveFrambufferImage(uint32_t color_index, Image *dst);
  void resolveMultiSampleImage(Image *src, Image *dst);
  void recordFrameTimings(Drv3dTimings *timing_data, uint64_t kickoff_stamp);
  void present();
  void doFrameEndCallbacks();
  void flushAndWait(ThreadedFence *user_fence);
  void captureScreen(Buffer *buffer);
  void dispatch(uint32_t x, uint32_t y, uint32_t z);
  void dispatchIndirect(BufferRef buffer, uint32_t offset);
  void copyBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void copyBufferToImageOrdered(Image *dst, Buffer *src, const VkBufferImageCopy &region);
  void fillBuffer(Buffer *buffer, uint32_t offset, uint32_t size, uint32_t value);
  void clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value);
  void clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value);
  void processFillEmptySubresources();
  void copyImage(Image *src, Image *dst, uint32_t src_mip, uint32_t dst_mip, uint32_t mip_count, uint32_t region_count,
    uint32_t first_region);
  void blitImage(Image *src, Image *dst, const VkImageBlit &region);
  void resetQuery(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count);
  void copyQueryResult(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count, Buffer *dst);
  void generateMipmaps(Image *img);
  void setGraphicsPipeline(ProgramID program);
  bool isInMultisampledFBPass();
  void syncConstDepthReadWithInternalStore();
  void ensureStateForDepthAttachment();
  void ensureStateForColorAttachments();
  void endPass(const char *why);
  void endNativePass();
  void beginNativePass();
  void nextNativeSubpass();
  void beginCustomStage(const char *why);
  void bindVertexUserData(BufferSubAllocation bsa);
  void drawIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void drawIndexedIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance);
  void drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance);
  void bindIndexUser(BufferSubAllocation bsa);
  void flushGrahpicsState(VkPrimitiveTopology top);
  void flushComputeState();
  // handled by flush, could be moved into this though
  void ensureActivePass();
  void changeSwapchainMode(const SwapchainMode &new_mode, ThreadedFence *fence);
  void shutdownSwapchain();
  enum BarrierImageType
  {
    Regular = 0,
    SwapchainColor,
    SwapchainDepth
  };
  void imageBarrier(Image *img, BarrierImageType image_type, ResourceBarrier state, uint32_t res_index, uint32_t res_range);
  void clearView(int what);
  void nativeRenderPassChanged();
  void performSyncAtRenderPassEnd();

  void registerFrameEventsCallback(FrameEvents *callback);
  void getWorkerCpuCore(int *core, int *thread_id);

  void setSwappyTargetRate(int rate);
  void getSwappyStatus(int *status);

  // stops write to current frameCore command buffer and starts new one, safely
  // false returned when interrupt is not possible
  bool interruptFrameCore();
  // pass start happens inside state apply, so interrupt must be treated differently
  void interruptFrameCoreForRenderPassStart();
  // tracks current execution "node" which is sequence of state set and sequental action command
  // no stop node for now, as it is not yet needed (redo with RAII if needed)
  void startNode(ExecutionNode cp);

  void trackStageResAccesses(const spirv::ShaderHeader &header, ShaderStage stageIndex);
  void trackIndirectArgAccesses(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void trackBindlessRead(Image *img);
};

#define VULKAN_LOCK_FRONT() OSSpinlockScopedLock lockedDevice(getFrontGuard())

// clang-format off

#define VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd)          \
    front.replayRecord->pushCommand(cmd);             \
    verifyExecutionMode();

#define VULKAN_DISPATCH_COMMAND(cmd)     \
  {                                      \
    VULKAN_LOCK_FRONT();                 \
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd) \
  }
// clang-format on

class DeviceContext //-V553
{
  // execution mode
  struct WorkerThread : public DaThread
  {
    WorkerThread(DeviceContext &c)
      // Not less than 4mb required for molten vk
      :
      DaThread("Vulkan Worker", 4096 << 10), ctx(c)
    {}
    // calls device.processCommandPipe() until termination is requested
    void execute() override;
    DeviceContext &ctx;
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

  static uint32_t calculateWorkerAffinityMask()
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
    if (coreCount > 2)
      return 2;
    // no extra care if coreCount is less than 2
    // invalid values have no effect and on one core
    // a mask makes no sense anyway
    return 1;
  }

  // internal state
  Device &device;
  VulkanDevice &vkDev;
  ContextFrontend front;
  ContextBackend back;

  OSSpinlock mutex;
  OSSpinlock &getFrontGuard();

  void verifyExecutionMode() { G_ASSERT(executionMode != ExecutionMode::INVALID); };
  void submitReplay();
  // we should do some cleanups/restarts when backend flush command are executed
  void afterBackendFlush();

  // temp buffers
  void cleanupFrontendReplayResources();

  // misc
  void reportAlliveObjects(FaultReportDump &dump);
  WinCritSec stubSwapGuard;
  void executeDebugFlush(const char *caller);

  void setPipelineState();

public:
  DeviceContext() = delete;
  ~DeviceContext() = default;

  DeviceContext(const DeviceContext &) = delete;
  DeviceContext &operator=(const DeviceContext &) = delete;

  DeviceContext(DeviceContext &&) = delete;
  DeviceContext &operator=(DeviceContext &&) = delete;
  DeviceContext(Device &dvc);

  template <typename CmdType>
  __forceinline void dispatchCommand(const CmdType &cmd)
  {
    VULKAN_LOCK_FRONT();
    dispatchCommandNoLock(cmd);
  }

  template <typename CmdType>
  __forceinline void dispatchCommandNoLock(const CmdType &cmd)
  {
    VULKAN_DISPATCH_COMMAND_NO_LOCK(cmd);
  }

  void shutdown();
  void initTempBuffersConfiguration();

  void initMode(ExecutionMode mode)
  {
    executionMode = mode;
    if (ExecutionMode::THREADED == executionMode)
    {
      worker.reset(new WorkerThread{*this});
      worker->start();
      // pin thread to a dedicated core to avoid performance penalties due to thread migration
      worker->setAffinity(calculateWorkerAffinityMask());
    }
  }

  void setPipelineCompilationTimeBudget(unsigned usecs);
  uint32_t getPiplineCompilationQueueLength();

  size_t getCurrentWorkItemId();

  void waitForItemPushSpace();
  void startPreRotate(uint32_t binding_slot);
  void holdPreRotateStateForOneFrame();

  void updateDebugUIPipelinesData();
  void setPipelineUsability(ProgramID program, bool value);
  void captureRenderPasses(bool capture_call_stack);
  void setConstRegisterBuffer(VulkanBufferHandle buffer, uint32_t offset, uint32_t size, ShaderStage stage);

  void bindVertexBuffer(uint32_t stream, Buffer *buffer, uint32_t offset);
  void bindVertexStride(uint32_t stream, uint32_t stride);

  void compileGraphicsPipeline(const VkPrimitiveTopology top);
  void compileComputePipeline();

  void dispatch(uint32_t x, uint32_t y, uint32_t z);
  void dispatchIndirect(Buffer *buffer, uint32_t offset);
  void drawIndirect(VkPrimitiveTopology top, uint32_t count, Buffer *buffer, uint32_t offset, uint32_t stride);
  void drawIndexedIndirect(VkPrimitiveTopology top, uint32_t count, Buffer *buffer, uint32_t offset, uint32_t stride);
  void draw(VkPrimitiveTopology top, uint32_t start, uint32_t count, uint32_t first_instance, uint32_t instance_count);
  void drawUserData(VkPrimitiveTopology top, uint32_t count, uint32_t stride, BufferSubAllocation user_data);
  void drawIndexed(VkPrimitiveTopology top, uint32_t index_start, uint32_t count, int32_t vertex_base, uint32_t first_instance,
    uint32_t instance_count);
  void drawIndexedUserData(VkPrimitiveTopology top, uint32_t count, uint32_t vertex_stride, BufferSubAllocation vertex_data,
    BufferSubAllocation index_data);
  void clearView(int clear_flags);
  void nativeRenderPassChanged();
  void copyBuffer(Buffer *source, Buffer *dest, uint32_t src_offset, uint32_t dst_offset, uint32_t data_size);
  void clearBufferFloat(Buffer *buffer, const float values[4]);
  void clearBufferInt(Buffer *buffer, const unsigned values[4]);
  void pushEvent(const char *name);
  void popEvent();
  void clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value);
  void clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value);
  void copyImage(Image *src, Image *dst, const VkImageCopy *regions, uint32_t region_count, uint32_t src_mip, uint32_t dst_mip,
    uint32_t mip_count);
  void fillEmptySubresources(Image *image);
  void resolveMultiSampleImage(Image *src, Image *dst);
  void flushDraws();
  void copyImageToHostCopyBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions);
  void copyImageToBuffer(Image *image, Buffer *buffer, uint32_t region_count, VkBufferImageCopy *regions, AsyncCompletionState *sync);
  void copyHostCopyToImage(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions);
  void copyBufferToImage(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions, bool seal);
  void copyBufferToImageOrdered(Buffer *src, Image *dst_id, uint32_t region_count, VkBufferImageCopy *regions);
  void blitImage(Image *src, Image *dst, const VkImageBlit &region);
  void processAllPendingWork();
  void wait();
  void beginSurvey(uint32_t name);
  void endSurvey(uint32_t name);
  // allows upload region overlap, keeps uploads ordered between each other
  void uploadBufferOrdered(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void uploadBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void downloadBuffer(Buffer *src, Buffer *dst, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  void destroyBuffer(Buffer *buffer);
  Buffer *discardBuffer(Buffer *to_discared, DeviceMemoryClass memory_class, FormatStore view_format, uint32_t bufFlags);
  void destroyRenderPassResource(RenderPassResource *rp);
  void destroyImageDelayed(Image *img);
  void destroyImage(Image *img);
  void evictImage(Image *img);
  void present();
  void changeSwapchainMode(const SwapchainMode &new_mode);
  void shutdownSwapchain();
  void shutdownImmediateConstBuffers();
  void addRenderState(shaders::DriverRenderStateId id, const shaders::RenderState &render_state_data);
  void addPipelineCache(VulkanPipelineCacheHandle cache);
  void insertTimestampQuery(TimestampQuery *query);
  void generateMipmaps(Image *img);
  void addSyncEvent(AsyncCompletionState &sync);
  void flushBufferToHost(Buffer *buffer, ValueRange<uint32_t> range);
  void waitFor(AsyncCompletionState &sync);
  TempBufferHolder copyToTempBuffer(int type, uint32_t size, const void *src);
  TempBufferHolder *allocTempBuffer(int type, uint32_t size);
  void freeTempBuffer(TempBufferHolder *temp_buff_holder);
  void resourceBarrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline);

  void updateBindlessResource(uint32_t index, D3dResource *res);
  void updateBindlessSampler(uint32_t index, SamplerInfo *samplerInfo);
  void copyBindlessTextureDescriptors(uint32_t src, uint32_t dst, uint32_t count);
  void updateBindlessResourcesToNull(uint32_t resource_type, uint32_t index, uint32_t count);

  // NOTE: this does not wait, just issues the command
  void captureScreen(Buffer *dst);
  void deleteAsyncCompletionStateOnFinish(AsyncCompletionState &sync);
  void generateFaultReport();
  void generateFaultReportAtFrameEnd();

  void writeDebugMessage(const char *msg, intptr_t msg_length, intptr_t severity);

#if D3D_HAS_RAY_TRACING
  void raytraceBuildBottomAccelerationStructure(RaytraceBottomAccelerationStructure *as, RaytraceGeometryDescription *desc,
    uint32_t count, RaytraceBuildFlags flags, bool update, Buffer *scratch_buf);
  void raytraceBuildTopAccelerationStructure(RaytraceTopAccelerationStructure *as, Buffer *instance_buffer, uint32_t instance_count,
    RaytraceBuildFlags flags, bool update, Buffer *scratch_buf);
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

#if D3D_HAS_RAY_TRACING
  void addRaytraceProgram(ProgramID program, uint32_t max_recursion, uint32_t shader_count, const ShaderModuleUse *shaders,
    uint32_t group_count, const RaytraceShaderGroup *groups);
  void copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size,
    Buffer *buffer, uint32_t offset);
#endif

  void setSwappyTargetRate(int rate);
  void getSwappyStatus(int *status);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void attachComputeProgramDebugInfo(ProgramID program, eastl::unique_ptr<ShaderDebugInfo> dbg);
#endif

  void placeAftermathMarker(const char *name);

  WinCritSec &getStubSwapGuard() { return stubSwapGuard; }

  int getFramerateLimitingFactor();

#if VULKAN_RECORD_TIMING_DATA
  const Drv3dTimings &getTiming(uintptr_t offset) const
  {
    return front.timingHistory[(front.completedFrameIndex - offset) % FRAME_TIMING_HISTORY_LENGTH];
  }
#endif
  void advanceAndCheckTimingRecord();
  void initTimingRecord();

  void registerFrameEventsCallback(FrameEvents *callback, bool useFront);
  void callFrameEndCallbacks();

  void getWorkerCpuCore(int *core, int *thread_id);

  ContextFrontend &getFrontend() { return front; }
  ContextBackend &getBackend() { return back; }
};

} // namespace drv3d_vulkan
