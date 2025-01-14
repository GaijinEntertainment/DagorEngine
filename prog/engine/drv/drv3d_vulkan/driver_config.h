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

  struct ConfigBits
  {
    // forces dedicated memory allocation for images, aside of driver reported requirements
    uint64_t useDedicatedMemForImages : 1;
    // allows useDedicatedMemForImages logic for render targets too
    uint64_t useDedicatedMemForRenderTargets : 1;
    // reset whole command pools instead of resetting each command buffer
    uint64_t resetCommandPools : 1;
    // release resources of command pools/buffers on reset
    uint64_t resetCommandsReleaseToSystem : 1;
    // disable render to 3d images
    uint64_t disableRenderTo3DImage : 1;
    // optimize buffer upload by range-merges
    uint64_t optimizeBufferUploads : 1;
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
    // flush after every "action" to catch state issues or buggy shaders
    uint64_t flushAfterEachDrawAndDispatch : 1;
#endif
    // use worker thread, otherwise execute commands on flush/update screen caller thread
    uint64_t useThreadedExecution : 1;
    // use GPU execution markers for device lost debugging
    uint64_t commandMarkers : 1;
    // allow coherent memory usage, treat coherent memory as non coherent otherwise
    uint64_t useCoherentMemory : 1;
    // enables debug markers in command buffers
    uint64_t allowDebugMarkers : 1;
    // disables swapchain, running application headlessly
    uint64_t headless : 1;
    // enable pre rotation in swapchain on android
    uint64_t preRotation : 1;
    // record command caller stack for each replay command
    uint64_t recordCommandCaller : 1;
    // keep last rendered image presented when user renders nothing but presents
    uint64_t keepLastRenderedImage : 1;
    // allow DMA buffer lock path (i.e. mapped memory non-staging writes)
    uint64_t allowDMAlockPath : 1;
    // validate that render passes do not use OP_LOAD in not-allowed conditions
    uint64_t validateRenderPassSplits : 1;
    // use pipeline cache mutex for MT pipeline compilation, needed for some drivers
    uint64_t usePipelineCacheMutex : 1;
    // allows framemem buffers to use internal memory ring, instead of resizeable discard
    uint64_t allowFrameMem : 1;
    // keeps namings of original buffer for framemem buffer usage error reporting
    uint64_t debugFrameMemUsage : 1;
    // check that we are not making underfined->read layout transitions on images (i.e. reading garbage)
    uint64_t debugImageGarbadgeReads : 1;
    // compile cached graphics pipelines using sync compile regardless of async allowance
    uint64_t compileCachedGrPipelinesUsingSyncCompile : 1;
    // compile seen graphics pipelines using sync compile regardless of async allowance
    uint64_t compileSeenGrPipelinesUsingSyncCompile : 1;
    // allow predicted latency wait in app code, compensating for worker-app latency
    uint64_t allowPredictedLatencyWaitApp : 1;
    // enables auto mode for latency wait on app side, when disabled app must trigger this wait
    uint64_t autoPredictedLatencyWaitApp : 1;
    // allow predicted latency wait in worker code, compensating for worker-GPU latency
    uint64_t allowPredictedLatencyWaitWorker : 1;
    // issue fatal when NRP split is detected
    uint64_t fatalOnNRPSplit : 1;
    // trigger assertion when validation fails, otherwise trigger logerr
    uint64_t allowAssertOnValidationFail : 1;
    // enables RenderDoc layer if system provide it
    uint64_t enableRenderDocLayer : 1;
    // enables robust buffer access feature on device when exists
    uint64_t robustBufferAccess : 1;
    // enables device execution tracking to hunt down device lost sources without exts/tooling
    uint64_t enableDeviceExecutionTracker : 1;
    // enables multi queue submit scheme
    uint64_t allowMultiQueue : 1;
    // sets queue priorities to high if possible
    uint64_t highPriorityQueues : 1;
  };

  struct DeviceBits
  {
    uint64_t gpuTimestamps : 1;
    uint64_t geometryShader : 1;
    uint64_t tesselationShader : 1;
    uint64_t fragmentShaderUAV : 1;
    uint64_t depthBoundsTest : 1;
    uint64_t anisotropicSampling : 1;
    uint64_t multiDrawIndirect : 1;
    uint64_t drawIndirectFirstInstance : 1;
    uint64_t conditionalRender : 1;
    uint64_t UAVOnlyForcedSampleCount : 1;
    uint64_t imagelessFramebuffer : 1;
    uint64_t attachmentNoStoreOp : 1;
    uint64_t depthStencilResolve : 1;
    uint64_t createRenderPass2 : 1;
    uint64_t renderTo3D : 1;
    uint64_t adrenoViewportConflictWithCS : 1;
    uint64_t separateImageViewUsage : 1;
    uint64_t sixteenBitStorage : 1;
    VkShaderStageFlags waveOperationsStageMask;
  };

  ConfigBits bits;
  DeviceBits has;
  // FIXME: move it bindless header to avoid including much stuff here
  BindlessSetLimits bindlessSetLimits;

  void fillConfigBits(const DataBlock *cfg);
  void fillDeviceBits();
  void fillExternalCaps(Driver3dDesc &caps);
  void configurePerDeviceDriverFeatures();

  const DataBlock *getPerDriverPropertyBlock(const char *prop_name);

  static const char *getFaultVendorDumpFile();
  static const char *getLastRunWasGPUFaultMarkerFile();

private:
  void extCapsFillConst(Driver3dDesc &caps);
  void extCapsFillUniversal(Driver3dDesc &caps);
  void extCapsFillPCWinOnly(Driver3dDesc &caps);
  void extCapsFillMultiplatform(Driver3dDesc &caps);
  void extCapsFillAndroidOnly(Driver3dDesc &caps);
  void setBindlessConfig();
};

} // namespace drv3d_vulkan
