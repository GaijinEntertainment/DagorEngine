// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// global driver configuration accumulated for quick access

#include <util/dag_stdint.h>
#include "vulkan_api.h"
#include "bindless_common.h"

class DataBlock;

namespace drv3d_vulkan
{

struct DriverConfig
{
  // if amount of texels is above this value for texture, we may force dedicated memory allocation for it
  // when related bits are set
  uint32_t dedicatedMemoryForImageTexelCountThreshold;
  // time between memory statistics are dumped to log
  uint32_t memoryStatisticsPeriodUs;
  // amount of per frame tex upload limit (RUB path auto-limit config)
  uint32_t texUploadLimit;
  // size of framemem buffer block (growth step)
  uint32_t framememBlockSize;
  // debug level of driver
  //   0 debug is off
  //   1 enables markers and naming
  //   2 enables validation
  uint32_t debugLevel;
  // time to wait when temp buffers allocated over limit (wrongly configured or used too much)
  uint32_t tempBufferOverAllocWaitMs;

  // minimal time of wait observed to trigger predicted latency wait logic
  int64_t latencyWaitThresholdUs;
  // delta used to adjust wait time in latency wait logic
  int64_t latencyWaitDeltaUs;
  // max wait that can be triggered by latency wait logic
  int64_t latencyWaitMaxUs;
  // executuion tracker breakpoint hash, will stop and dump GPU fault on this hash
  uint64_t executionTrackerBreakpoint;
  // executuion tracker breakpoint for data hash, will stop and dump GPU fault when marker name with this hash is occuried
  uint64_t executionTrackerDataBreakpoint;
  // default barrier merge mode, contols how stages are merged, different GPUs may give different perf results on this
  size_t barrierMergeMode;
  // amount of images swapchain can extra allocate if it blocked at acquire
  uint32_t swapchainMaxExtraImages;
  // time in microseconds that should pass before command buffer/command pool memory will be released
  // to system when memory release option is active
  // allows avoiding memory leaks while keeping reallocation overhead reasonable
  uint64_t resetCommandsReleasePeriodUs;
  // will skip save of pipeline cache on shutdown if size is above this threshold
  // game should use runtime defined points to save pipe cache (like on level load/change)
  uint32_t pipelineCacheBlockingSaveMaxSizeMb;
  // will skip save of pipeline cache entirely if size is above this threshold
  // this means there is too much pipes or we trashed cache by something like caching all variants of various configs
  // or we must allow system to handle bigger file sizes
  uint32_t pipelineCacheMaxSizeMb;

  struct
  {
    uint64_t start;
    uint64_t end;
    uint64_t dumpPeriod;
  } flushAfterEachDrawAndDispatchRange;

  struct ConfigBits
  {
    // forces dedicated memory allocation for images, aside of driver reported requirements
    bool useDedicatedMemForImages : 1;
    // allows useDedicatedMemForImages logic for render targets too
    bool useDedicatedMemForRenderTargets : 1;
    // reset whole command pools instead of resetting each command buffer
    bool resetCommandPools : 1;
    // release resources of command pools/buffers on reset
    bool resetCommandsReleaseToSystem : 1;
    // disable render to 3d images
    bool disableRenderTo3DImage : 1;
    // optimize buffer upload by range-merges
    bool optimizeBufferUploads : 1;
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
    // flush after every "action" to catch state issues or buggy shaders
    bool flushAfterEachDrawAndDispatch : 1;
#endif
    // use worker thread, otherwise execute commands on flush/update screen caller thread
    bool useThreadedExecution : 1;
    // allow coherent memory usage, treat coherent memory as non coherent otherwise
    bool useCoherentMemory : 1;
    // enables debug markers in command buffers
    bool allowDebugMarkers : 1;
    // disables swapchain, running application headlessly
    bool headless : 1;
    // enable pre rotation in swapchain on android
    bool preRotation : 1;
    // record command caller stack for each replay command
    bool recordCommandCaller : 1;
    // allow DMA buffer lock path (i.e. mapped memory non-staging writes)
    bool allowDMAlockPath : 1;
    // validate that render passes do not use OP_LOAD in not-allowed conditions
    bool validateRenderPassSplits : 1;
    // use pipeline cache mutex for MT pipeline compilation, needed for some drivers
    bool usePipelineCacheMutex : 1;
    // allows framemem buffers to use internal memory ring, instead of resizeable discard
    bool allowFrameMem : 1;
    // keeps namings of original buffer for framemem buffer usage error reporting
    bool debugFrameMemUsage : 1;
    // check that we are not making undefined->read layout transitions on images
    // or reading resources after device reset before contents are restored
    // (i.e. reading garbage)
    bool debugGarbadgeReads : 1;
    // compile cached graphics pipelines using sync compile regardless of async allowance
    bool compileCachedGrPipelinesUsingSyncCompile : 1;
    // compile seen graphics pipelines using sync compile regardless of async allowance
    bool compileSeenGrPipelinesUsingSyncCompile : 1;
    // allow latency wait using shared wait on fence
    bool allowSharedFenceLatencyWait : 1;
    // allow predicted latency wait in app code, compensating for worker-app latency
    bool allowPredictedLatencyWaitApp : 1;
    // enables auto mode for latency wait on app side, when disabled app must trigger this wait
    bool autoPredictedLatencyWaitApp : 1;
    // allow predicted latency wait in worker code, compensating for worker-GPU latency
    bool allowPredictedLatencyWaitWorker : 1;
    // issue fatal when NRP split is detected
    bool fatalOnNRPSplit : 1;
    // trigger assertion when validation fails, otherwise trigger D3D_ERROR
    bool allowAssertOnValidationFail : 1;
    // enables RenderDoc layer if system provide it
    bool enableRenderDocLayer : 1;
    // enables robust buffer access feature on device when exists
    bool robustBufferAccess : 1;
    // enables device execution tracking to hunt down device lost sources without exts/tooling
    bool enableDeviceExecutionTracker : 1;
    // enables multi queue submit scheme
    bool allowMultiQueue : 1;
    // enables usage of separate transfer queue for async readback operations
    bool allowAsyncReadback : 1;
    // enables user provided multi queue logic (SwitchQueue/d3d fences)
    bool allowUserProvidedMultiQueue : 1;
    // sets queue priorities to high if possible
    bool highPriorityQueues : 1;
    // every pipeline compiled will dump its executable statistics if possible
    bool dumpPipelineExecutableStatistics : 1;
    // on some Android devices, for unclear reasons, memset is required for successful fread to the RUB memory
    bool memsetOnRub : 1;
    // use vulkan memory allocation callbacks to track memory consumption
    bool useCustomAllocationCallbacks : 1;
    // do not use QFOT barriers, possible with maintenance 9 or with NV because,
    // but better to keep such logic data driven
    bool ignoreQueueFamilyOwnershipTransferBarriers : 1;
    // force swapchain to do acquire and present only at backend thread
    bool forceSwapchainOnlyBackendAcquire : 1;
    // use acquire exclusive swapchain test, it is optional because it needed on limited amout of targets
    // and test itself is a bit "spec-questionable"
    bool useSwapchainAcquireExclusiveTest : 1;
    // recreate swapchain if image was acquired from it, but not presented
    // to avoid issues on systems that treat image still acquired after update of swapchain
    bool recreateSwapchainWhenImageAcquired : 1;
    // some of androids have issue with depth clamp, but that's not clearly recorded which ones
    // allow to hot patch if they are found
    bool disableDepthClamp : 1;
    // log any pipeline compilations if enabled and VULKAN_LOG_PIPELINE_ACTIVITY set
    bool logPipelineCompile : 1;
    // log any pipeline bindings if enabled and VULKAN_LOG_PIPELINE_ACTIVITY set
    bool logPipelineBinds : 1;
    // allows doing full device reset on device lost to restore application without crashing
    bool resetOnDeviceLost : 1;
    // when enabled will add resource allocations to memory profiler
    bool profileResourceMemUsage : 1;
  };

  struct DeviceBits
  {
    bool gpuTimestamps : 1;
    bool geometryShader : 1;
    bool tesselationShader : 1;
    bool fragmentShaderUAV : 1;
    bool depthBoundsTest : 1;
    bool anisotropicSampling : 1;
    bool multiDrawIndirect : 1;
    bool drawIndirectFirstInstance : 1;
    bool conditionalRender : 1;
    bool UAVOnlyForcedSampleCount : 1;
    bool imagelessFramebuffer : 1;
    bool attachmentNoStoreOp : 1;
    bool depthStencilResolve : 1;
    bool createRenderPass2 : 1;
    bool renderTo3D : 1;
    bool adrenoViewportConflictWithCS : 1;
    bool separateImageViewUsage : 1;
    bool sixteenBitStorage : 1;
    VkShaderStageFlags waveOperationsStageMask;
  };

  ConfigBits bits;
  DeviceBits has;
  // FIXME: move it bindless header to avoid including much stuff here
  BindlessSetLimits bindlessSetLimits;
  dag::Vector<bool> formatSupportMask;

  void fillConfigBits(const DataBlock *cfg);
  void fillDeviceBits();
  void fillExternalCaps(DriverDesc &caps);
  void configurePerDeviceDriverFeatures();

  const DataBlock *getPerDriverPropertyBlock(const char *prop_name);

  static const char *getFaultVendorDumpFile();
  static const char *getLastRunWasGPUFaultMarkerFile();
  static const char *getSupportQueryFailedMarkerFile();

private:
  void extCapsFillConst(DriverDesc &caps);
  void extCapsFillUniversal(DriverDesc &caps);
  void extCapsFillPCWinOnly(DriverDesc &caps);
  void extCapsFillMultiplatform(DriverDesc &caps);
  void extCapsFillAndroidOnly(DriverDesc &caps);
  void setBindlessConfig();
};

} // namespace drv3d_vulkan
