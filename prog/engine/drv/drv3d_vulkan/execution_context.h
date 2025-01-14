// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_commands.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "render_state_system.h"
#include "util/scoped_timer.h"
#include "pipeline_barrier.h"
#include "async_completion_state.h"
#include "render_work.h"
#include "frame_info.h"
#include "timelines.h"
#include "image_resource.h"
#include <osApiWrappers/dag_threads.h>
#include "execution_state.h"
#include "pipeline_state.h"
#include "pipeline_state_pending_references.h"
#include "util/fault_report.h"
#include "execution_scratch.h"

namespace drv3d_vulkan
{
struct TimestampQuery;
struct SwapchainMode;
#if D3D_HAS_RAY_TRACING
class RaytraceAccelerationStructure;
#endif

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

  // increments on any "action"-like (draw,dispatch,custom op) command
  uint32_t actionIdx = 0;
  // tracks last timestamp action index, to skip empty timestamp requests (they are slowing GPU down)
  uint32_t lastTimestampActionIdx = -1;
  // consume offset for NRP reordered buffer copies
  uint32_t reorderedBufferCopyOffset = 0;

  VulkanCommandBufferHandle frameCore = VulkanHandle();
  DeviceQueueType frameCoreQueue = DeviceQueueType::INVALID;
  size_t lastBufferIdxOnQueue[(uint32_t)DeviceQueueType::COUNT] = {};

  typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::EXECUTION_PRIMARY> PrimaryPipelineBarrier;
  typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::EXECUTION_SECONDARY> SecondaryPipelineBarrier;

  void beginPassInternal(RenderPassClass *pass_class, VulkanFramebufferHandle fb_handle, VkRect2D area);
  void allocFrameCore();
  String getCurrentCmdCaller();
  uint64_t getCurrentCmdCallerHash();
  void reportMissingPipelineComponent(const char *component);
  void queueImageResidencyRestore(Image *img);
  void writeExectionChekpoint(VkPipelineStageFlagBits stage);
  void recordUserQueueSignal(int idx, DeviceQueueType target_queue);
  void waitUserQueueSignal(int idx, DeviceQueueType target_queue);

private:
  // temporal "pooled" contents
  static ExecutionScratch scratch;

  // upload mutliqueue deps
  size_t transferUploadBuffer = 0;
  size_t graphicsUploadBuffer = 0;
  size_t uploadQueueWaitMask = ~0;
  void waitForUploadOnCurrentBuffer();
  VulkanSemaphoreHandle presentSignal = VulkanHandle();

  void finishAllGPUWorkItems();
  enum
  {
    // 0x4E434D = NCM
    MARKER_NCMD_START = 0x4E434D00,
    MARKER_NCMD_START_TRANSFER_UPLOAD,
    MARKER_NCMD_UNORDERED_COLOR_CLEAR,
    MARKER_NCMD_UNORDERED_DEPTH_CLEAR,
    MARKER_NCMD_UNORDERED_IMAGE_COPY,
    MARKER_NCMD_FILL_EMPTY_SUBRES,
    MARKER_NCMD_IMAGE_UPLOAD,
    MARKER_NCMD_BUFFER_UPLOAD_ORDERED,
    MARKER_NCMD_BUFFER_UPLOAD,
    MARKER_NCMD_PRE_PRESENT,
    MARKER_NCMD_TIMESTAMPS_RESET_QUEUE_READBACKS,
    MARKER_NCMD_IMAGE_RESIDENCY_RESTORE,
    MARKER_NCMD_DOWNLOAD_BUFFERS,
    MARKER_NCMD_DOWNLOAD_IMAGES,
    MARKER_NCMD_BUFFER_HOST_FLUSHES,
    MARKER_NCMD_FRAME_END_SYNC,
    MARKER_NCMD_END
  };
  void writeExectionChekpointNonCommandStream(VkPipelineStageFlagBits stage, uint32_t key);

  void restoreImageResidencies();
  VulkanCommandBufferHandle allocAndBeginCommandBuffer(DeviceQueueType queue);
  void flushImageDownloads();
  void flushBufferDownloads();
  void flushBufferUploads();
  void flushOrderedBufferUploads();
  void flushBufferToHostFlushes();
  void flushImageUploads();
  void flushImageUploadsIter(uint32_t start, uint32_t end);
  void flushUnorderedImageColorClears();
  void flushUnorderedImageDepthStencilClears();
  void flushUnorderedImageCopies();
  void flushUploads();
  void flushPostFrameCommands();

  void stackUpCommandBuffers();
  void sortAndCountDependencies();
  void joinQueuesToSubmitFence();
  void enqueueCommandListsToMultipleQueues(ThreadedFence *fence);

  void printMemoryStatistics();

  void applyStateChanges();

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  static thread_local ExecutionContext *tlsDbgActiveInstance;
#endif

public:
  void addQueueDep(uint32_t src_submit, uint32_t dst_submit);
  void applyQueuedDiscards();

  ExecutionContext() = delete;
  ~ExecutionContext();
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

  void beginCmd(const void *ptr);
  void endCmd();
  FramebufferState &getFramebufferState();

  void completeSyncForQueueChange();
  void switchFrameCoreForQueueChange(DeviceQueueType target_queue);
  void switchFrameCore(DeviceQueueType target_queue);
  void onFrameCoreReset();
  // processes out-of-order parts of frame that can affect orderd frame
  void prepareFrameCore();
  void flush(ThreadedFence *fence);
  void cleanupMemory();
  void queueBufferDiscard(BufferRef old_buf, const BufferRef &new_buf, uint32_t flags);
  uint32_t beginVertexUserData(uint32_t stride);
  void endVertexUserData(uint32_t stride);
  void invalidateActiveGraphicsPipeline();

  void finishDebugEventRanges();
  void restoreDebugEventRanges();
  void insertEvent(const char *marker, uint32_t color = 0xFFFFFFFF);
  void pushEventRaw(const char *marker, uint32_t color);
  void popEventRaw();
  void pushEventTracked(const char *marker, uint32_t color = 0xFFFFFFFF);
  void popEventTracked();
  void pushEvent(StringIndexRef name);
  void popEvent();
  void beginQuery(VulkanQueryPoolHandle pool, uint32_t index, VkQueryControlFlags flags);
  void endQuery(VulkanQueryPoolHandle pool, uint32_t index);
  void wait(ThreadedFence *fence);
#if D3D_HAS_RAY_TRACING
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  void accumulateRaytraceBuildAccesses(const RaytraceStructureBuildData &build_data);
  void accumulateAssumedRaytraceStructureReads(const RaytraceStructureBuildData &build_data);
  void buildAccelerationStructure(const RaytraceStructureBuildData &build_data);
  void queryAccelerationStructureCompationSizes(const RaytraceStructureBuildData &build_data);
  void buildAccelerationStructures(RaytraceStructureBuildData *build_data, uint32_t count);
#endif
#endif
  void present();
  void doFrameEndCallbacks();
  void flushAndWait(ThreadedFence *user_fence);
  void dispatch(uint32_t x, uint32_t y, uint32_t z);
  void dispatchIndirect(BufferRef buffer, uint32_t offset);
  void copyImageToBufferOrdered(Buffer *dst, Image *src, const VkBufferImageCopy *regions, int count);
  void fillBuffer(Buffer *buffer, uint32_t offset, uint32_t size, uint32_t value);
  void clearDepthStencilImage(Image *image, const VkImageSubresourceRange &area, const VkClearDepthStencilValue &value);
  void clearColorImage(Image *image, const VkImageSubresourceRange &area, const VkClearColorValue &value);
  void processBindlessUpdates();
  void copyImage(Image *src, Image *dst, uint32_t src_mip, uint32_t dst_mip, uint32_t mip_count, uint32_t region_count,
    uint32_t first_region);
  void copyQueryResult(VulkanQueryPoolHandle pool, uint32_t index, uint32_t count, Buffer *dst);
  void syncConstDepthReadWithInternalStore();
  void ensureStateForDepthAttachment(VkRect2D area);
  void ensureStateForNativeRPDepthAttachment();
  void ensureStateForColorAttachments(VkRect2D area);
  void endPass(const char *why);
  void endNativePass();
  void beginNativePass();
  void nextNativeSubpass();
  void beginCustomStage(const char *why);
  void bindVertexUserData(const BufferRef &bsa);
  void drawIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void drawIndexedIndirect(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance);
  void drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance);
  void flushGrahpicsState(VkPrimitiveTopology top);
  void flushComputeState();
  // handled by flush, could be moved into this though
  void ensureActivePass();
  void imageBarrier(Image *img, ResourceBarrier d3d_barrier, uint32_t res_index, uint32_t res_range);
  void bufferBarrier(const BufferRef &b_ref, ResourceBarrier d3d_barrier);
  void clearView(int what);
  void processBufferCopyReorders(uint32_t pass_index);
  void nativeRenderPassChanged();
  void finishReorderAndPerformSync();
  void interruptFrameCoreForRenderPassStart();

  void registerFrameEventsCallback(FrameEvents *callback);

  // stops write to current frameCore command buffer and starts new one, safely
  // false returned when interrupt is not possible
  bool interruptFrameCore();
  // tracks current execution "node" which is sequence of state set and sequental action command
  // no stop node for now, as it is not yet needed (redo with RAII if needed)
  void startNode(ExecutionNode cp);

  void trackTResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage);
  void trackUResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage);
  void trackBResAccesses(uint32_t slot, PipelineStageStateBase &state, ExtendedShaderStage stage);
  void trackStageResAccesses(const spirv::ShaderHeader &header, ExtendedShaderStage stage);
  // ignores dirty mask to track executions, that are not parallel treatable (like a dispatch-dispatch pairs)
  void trackStageResAccessesNonParallel(const spirv::ShaderHeader &header, ExtendedShaderStage stage);

  void trackIndirectArgAccesses(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void trackBindlessRead(Image *img);

  template <typename ResType>
  void verifyResident(ResType *obj);

  void executeFSR(amd::FSR *fsr, const FSRUpscalingArgs &params);
};

} // namespace drv3d_vulkan
