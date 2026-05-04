// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_threads.h>
#include "render_state_system.h"
#include "util/scoped_timer.h"
#include "pipeline_barrier.h"
#include "async_completion_state.h"
#include "render_work.h"
#include "frame_info.h"
#include "timelines.h"
#include "image_resource.h"
#include "execution_state.h"
#include "pipeline_state.h"
#include "pipeline_state_pending_references.h"
#include "util/fault_report.h"
#include "execution_scratch.h"

namespace drv3d_vulkan
{
struct TimestampQuery;
struct SwapchainMode;
struct ImageArea;
#if VULKAN_HAS_RAYTRACING
class RaytraceAccelerationStructure;
#endif

enum class ExecutionNode
{
  PS,
  CS,
  RP,
  CUSTOM
};

struct CmdPresent;

class BEContext
{

public:
  // work set that is worked on
  RenderWork *data;

  // pointer and index to currently executed command
  const void *cmdPtr;
  uint32_t cmdIndex;
  bool flushProcessed;

  bool renderAllowed;
  // counts amount of non-indirected drawcalls
  uint32_t directDrawCount;
  // store amount of non-indirected drawcalls at start of survey
  // to avoid waiting empty surveys as they can hang on some GPUs
  uint32_t directDrawCountInSurvey;

  // increments on any "action"-like (draw,dispatch,custom op) command
  uint32_t actionIdx;
  // tracks last timestamp action index, to skip empty timestamp requests (they are slowing GPU down)
  uint32_t lastTimestampActionIdx;
  // consume offset for NRP reordered buffer copies
  uint32_t reorderedBufferCopyOffset;
  // non command stream saved loop key, to trigger device execution tracker once per ncmd loop
  uint32_t ncmdLoopKey;
  // on next present we should place render submit latency marker
  // because it should happen after submit yet before present
  bool needRenderSubmitLatencyMarkerBeforePresent;

  VulkanCommandBufferHandle frameCore;
  DeviceQueueType frameCoreQueue;
  size_t lastBufferIdxOnQueue[(uint32_t)DeviceQueueType::COUNT];

  typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::EXECUTION_PRIMARY> PrimaryPipelineBarrier;

  void beginPassInternal(RenderPassClass *pass_class, VulkanFramebufferHandle fb_handle, VkRect2D area);
  void allocFrameCore();
  String getCurrentCmdCaller();
  uint64_t getCurrentCmdCallerHash();
  void reportMissingPipelineComponent(const char *component);
  void queueImageResidencyRestore(Image *img);
  void queueBufferResidencyRestore(Buffer *buf);

  template <typename T>
  void execCmd(const T &cmd);

  void reset(RenderWork *in_work);

  thread_local static bool active;

  BEContext() = default;
  ~BEContext() = default;

private:
  // temporal "pooled" contents
  ExecutionScratch scratch;

  // upload mutliqueue deps
  size_t transferUploadBuffer;
  size_t graphicsUploadBuffer;
  size_t uploadQueueWaitMask;
  void waitForUploadOnCurrentBuffer();
  eastl::fixed_vector<VulkanSemaphoreHandle, MAX_SECONDARY_SWAPCHAIN_COUNT + 1, false> frameReadySemaphoresForPresent;

  void finishAllGPUWorkItems();
  enum
  {
    // 0x4E434D = NCM
    MARKER_NCMD_START = 0x4E434D00,
    MARKER_NCMD_START_TRANSFER_UPLOAD,
    MARKER_NCMD_UNORDERED_COLOR_CLEAR,
    MARKER_NCMD_UNORDERED_DEPTH_CLEAR,
    MARKER_NCMD_UNORDERED_IMAGE_COPY,
    MARKER_NCMD_UNORDERED_IMAGE_COPY_BARRIER,
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
    MARKER_NCMD_END,
    MARKER_NCMD_ASYNC_READBACK_ACQUIRE,
    MARKER_NCMD_ASYNC_READBACK_RELEASE,
  };
  void writeExceptionChekpointNonCommandStream(uint32_t key);

  void verifyResourcesResidencyRestoration();
  void restoreResourcesResidency();
  VulkanCommandBufferHandle allocAndBeginCommandBuffer(DeviceQueueType queue);
  void flushImageDownloads();
  void flushBufferDownloads();
  void flushBufferUploads(bool overlapped);
  void flushOrderedBufferUploads(bool overlapped);
  void transferOwnershipForOverlappedBufferUploads(DeviceQueueType src, DeviceQueueType dst);
  void transferOwnershipForOverlappedBufferUploads(DeviceQueueType src, DeviceQueueType dst,
    const dag::Vector<BufferCopyInfo> &uploads, const dag::Vector<VkBufferCopy> &copies);
  void flushBufferToHostFlushes();
  void flushImageUploads();
  void flushImageUploadsIter(uint32_t start, uint32_t end);
  void flushUnorderedImageColorClears();
  void flushUnorderedImageDepthStencilClears();
  void flushUnorderedImageCopies();
  void flushUploads();
  void flushPostFrameCommands();

  void processReadbacks();

  void stackUpCommandBuffers();
  void sortAndCountDependencies();
  void joinQueuesToSubmitFence();
  void enqueueCommandListsToMultipleQueues(ThreadedFence *fence);

  void printMemoryStatistics();

  void applyStateChanges();

public:
  void addQueueDep(uint32_t src_submit, uint32_t dst_submit);
  void applyQueuedDiscards();
  bool referencedInQueuedDiscards(Buffer *obj);

  BEContext(const BEContext &) = delete;
  BEContext &operator=(const BEContext &) = delete;
  BEContext(BEContext &&) = delete;
  BEContext &operator=(BEContext &&) = delete;

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
  void invalidateActiveGraphicsPipeline();

  void finishDebugEventRanges();
  void restoreDebugEventRanges();
  void insertEvent(const char *marker, uint32_t color = 0xFFFFFFFF);
  void pushEventRaw(const char *marker, uint32_t color);
  void popEventRaw();
  void pushEventTracked(const char *marker, uint32_t color = 0xFFFFFFFF);
  void popEventTracked();
  void beginQuery(VulkanQueryPoolHandle pool, uint32_t index, VkQueryControlFlags flags);
  void endQuery(VulkanQueryPoolHandle pool, uint32_t index);
  void wait(ThreadedFence *fence);
#if VULKAN_HAS_RAYTRACING
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  void accumulateRaytraceBuildAccesses(const RaytraceStructureBuildData &build_data);
  void accumulateAssumedRaytraceStructureReads(const RaytraceStructureBuildData &build_data);
  void buildAccelerationStructure(const RaytraceStructureBuildData &build_data);
  void queryAccelerationStructureCompationSizes(const RaytraceStructureBuildData &build_data);
#endif
#endif
  bool copyImageCmdTailBarrier(const CmdCopyImage &cmd);
  void copyImageCmdMainAction(const CmdCopyImage &cmd);
  void processMipGenBatch();
  void baseMipBlit(Image *from, Image *to);
  void makeImageReadyForPresent(Image *img);
  bool acquireSwapchainImage(const CmdPresent &params, uint32_t &out_index, VulkanSemaphoreHandle &out_sem);
  void doFrameEndCallbacks();
  void copyImageToBufferOrdered(Buffer *dst, Image *src, const VkBufferImageCopy *regions, int count);
  void fillBuffer(Buffer *buffer, uint32_t offset, uint32_t size, uint32_t value);
  void processBindlessUpdates();
  void syncConstDepthReadWithInternalStore();
  void ensureStateForDepthAttachment(VkRect2D area);
  void ensureStateForNativeRPDepthAttachment();
  void ensureStateForColorAttachments(VkRect2D area);
  void ensureStateForShadingRateAttachment();
  void endPass(const char *why);
  void endNativePass();
  void beginNativePass();
  void nextNativeSubpass();
  void beginCustomStage(const char *why);
  void draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance);
  void drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance);
  void flushGrahpicsState(VkPrimitiveTopology top);
  void flushComputeState();
  // handled by flush, could be moved into this though
  void ensureActivePass();
  void processBufferCopyReorders(uint32_t pass_index);
  void finishReorderAndPerformSync();
  void interruptFrameCoreForRenderPassStart();

  // stops write to current frameCore command buffer and starts new one, safely
  // false returned when interrupt is not possible
  bool interruptFrameCore();
  // tracks current execution "node" which is sequence of state set and sequental action command
  // no stop node for now, as it is not yet needed (redo with RAII if needed)
  void startNode(ExecutionNode cp);

  void trackIndirectArgAccesses(BufferRef buffer, uint32_t offset, uint32_t count, uint32_t stride);
  void trackBindlessRead(Image *img, ImageArea area);

  template <typename ResType>
  void verifyResident(ResType *obj);
};

} // namespace drv3d_vulkan
