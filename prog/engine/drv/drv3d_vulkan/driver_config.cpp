// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "driver_config.h"
#include <ioSys/dag_dataBlock.h>
#include "globals.h"
#include "physical_device_set.h"
#include "device_queue.h"
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include "timeline_latency.h"
#include <osApiWrappers/dag_direct.h>

using namespace drv3d_vulkan;

void DriverConfig::fillConfigBits(const DataBlock *cfg)
{
  texUploadLimit = cfg->getInt("texUploadLimitMb", 128) * 1024 * 1024 / (TimelineLatency::replayToGPUCompletionRingBufferSize);
  framememBlockSize = cfg->getInt("framememBlockSizeKb", 16) * 1024;
  // don't use it for now, its about 10% slower than with heaps...
  bits.useDedicatedMemForImages = cfg->getBool("allowDedicatedImageMemory", false);
  bits.useDedicatedMemForRenderTargets = cfg->getBool("alwaysUseDedicatedMemoryForRenderTargets", true);
  dedicatedMemoryForImageTexelCountThreshold = cfg->getInt("dedicatedImageMemoryTexelCountThreshold", 256 * 256);
  memoryStatisticsPeriodUs = cfg->getInt("memoryStatisticsPeriodSeconds", MEMORY_STATISTICS_PERIOD) * 1000 * 1000;
  if (memoryStatisticsPeriodUs)
    debug("vulkan: logging memory usage every %lu seconds", memoryStatisticsPeriodUs / (1000 * 1000));

  bits.resetCommandPools = cfg->getBool("resetCommandPools", true);
  // default is false, true has the potential to kill performance on some devices (intel)
  bits.resetCommandsReleaseToSystem = cfg->getBool("resetCommandsReleasesToSystem", false);
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  bits.flushAfterEachDrawAndDispatch = cfg->getBool("flushAndWaitAfterEachDrawAndDispatch", false);
#endif
  bits.optimizeBufferUploads = cfg->getBool("optimizeBufferUploads", true);
  // mostly for debugging, as the debug layer complains about wrong image states if slices of 3d images are used as 2d or array image
  // this can be removed if the debug layers supports vk_khr_maintenance1 properly
  bits.disableRenderTo3DImage = cfg->getBool("disableRenderTo3DImage", false);

  bits.commandMarkers = cfg->getBool("commandMarkers", false);

  // on some platforms (ex. Android) vulkan drivers has issue with corrupted coherent memory state
  bits.useCoherentMemory = cfg->getBool("allowCoherentMemory", true);

  bits.headless = cfg->getBool("headless", false);
  debug("vulkan: running in headless mode is %s", bits.headless ? "yes" : "no");

  bits.preRotation = cfg->getBool("preRotation", false);
  debug("vulkan: pre-rotation in swapchain: %s", bits.preRotation ? "yes" : "no");

  bits.keepLastRenderedImage = cfg->getBool("keepLastRenderedImage", false);
  debug("vulkan: keep last rendered image in swapchain: %s", bits.keepLastRenderedImage ? "yes" : "no");

  static const char deferred_exection_name[] = "deferred";
  static const char threaded_exection_name[] = "threaded";
  const char *config_exection = cfg->getStr("executionMode", threaded_exection_name);
  if (0 == strcmp(threaded_exection_name, config_exection))
    bits.useThreadedExecution = 1;
  else if (0 == strcmp(deferred_exection_name, config_exection))
    bits.useThreadedExecution = 0;
  else
  {
    String execModeList = String(0, "%s, %s", threaded_exection_name, deferred_exection_name);
    DAG_FATAL("vulkan: invalid execution mode %s, use one of [%s]", config_exection, execModeList);
  }

  if (bits.useThreadedExecution)
    debug("vulkan: threaded execution mode selected");
  else
    debug("vulkan: deferred execution mode selected");

  debugLevel = cfg->getInt("debugLevel", 0);
  debug("vulkan: debug level is %u", debugLevel);

  tempBufferOverAllocWaitMs = cfg->getInt("tempBufferOverAllocWaitMs", 0);
  debug("vulkan: temp buffer over alloc wait: %u ms", tempBufferOverAllocWaitMs);

  bits.allowDebugMarkers = cfg->getBool("allowDebugMarkers", debugLevel > 0 ? true : false);

  bits.recordCommandCaller = cfg->getBool("recordCommandCaller", false);

  bits.allowDMAlockPath = cfg->getBool("allowBufferDMALockPath", true);
  debug("vulkan: %s DMA lock path", bits.allowDMAlockPath ? "allow" : "disallow");

#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_ANDROID || _TARGET_C3)
  bits.validateRenderPassSplits = cfg->getBool("validateRenderPassSplits", false);
  bits.fatalOnNRPSplit = true;
#else
  bits.fatalOnNRPSplit = cfg->getBool("fatalOnNRPSplit", false);
#endif
  bits.debugImageGarbadgeReads = cfg->getBool("debugImageGarbadgeReads", false);
  bits.allowAssertOnValidationFail = cfg->getBool("allowAssertOnValidationFail", false);
  bits.enableRenderDocLayer = cfg->getBool("enableRenderDocLayer", false);
  bits.robustBufferAccess = cfg->getBool("robustBufferAccess", false);
  bits.highPriorityQueues = cfg->getBool("highPriorityQueues", false);
}

void DriverConfig::fillDeviceBits()
{
  has.gpuTimestamps = !(Globals::VK::phy.properties.limits.timestampComputeAndGraphics == VK_FALSE ||
                        Globals::VK::queue[DeviceQueueType::GRAPHICS].getTimestampBits() < 1);
  has.fragmentShaderUAV = VK_FALSE != Globals::VK::phy.features.fragmentStoresAndAtomics;
  has.depthBoundsTest = VK_FALSE != Globals::VK::phy.features.depthBounds;
  has.anisotropicSampling = VK_FALSE != Globals::VK::phy.features.samplerAnisotropy;
  has.multiDrawIndirect = VK_FALSE != Globals::VK::phy.features.multiDrawIndirect;
  has.drawIndirectFirstInstance = VK_FALSE != Globals::VK::phy.features.drawIndirectFirstInstance;
  has.depthStencilResolve = Globals::VK::phy.hasDepthStencilResolve;
  has.adrenoViewportConflictWithCS = Globals::VK::phy.vendorId == D3D_VENDOR_QUALCOMM;

  const bool hasEnoughDescriptorSetsForTessShader =
    Globals::VK::phy.properties.limits.maxBoundDescriptorSets > spirv::graphics::evaluation::REGISTERS_SET_INDEX;
  has.tesselationShader = Globals::VK::phy.features.tessellationShader && hasEnoughDescriptorSetsForTessShader;

  has.conditionalRender = 0;
#if VK_EXT_conditional_rendering
  has.conditionalRender |= Globals::VK::phy.hasConditionalRender && Globals::VK::dev.hasExtension<ConditionalRenderingEXT>();
#endif

  // d3d:: interface does not query what samples are supported (precisely)
  // treat this feature is available if at least all variants up to 8 samples are supported
  constexpr VkSampleCountFlags minimalMask =
    VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT;
  has.UAVOnlyForcedSampleCount =
    (Globals::VK::phy.properties.limits.framebufferNoAttachmentsSampleCounts & minimalMask) == minimalMask;

  has.imagelessFramebuffer = 0;
#if VK_KHR_imageless_framebuffer && VK_KHR_maintenance2
  has.imagelessFramebuffer |= Globals::VK::phy.hasImagelessFramebuffer && Globals::VK::dev.hasExtension<ImagelessFramebufferKHR>() &&
                              Globals::VK::dev.hasExtension<Maintenance2KHR>();
#endif

#if VK_KHR_maintenance2
  has.separateImageViewUsage = Globals::VK::dev.hasExtension<Maintenance2KHR>();
#endif

  has.attachmentNoStoreOp = 0;
#if VK_EXT_load_store_op_none
  has.attachmentNoStoreOp |= Globals::VK::dev.hasExtension<LoadStoreOpNoneEXT>();
#endif
#if VK_QCOM_render_pass_store_ops
  has.attachmentNoStoreOp |= Globals::VK::dev.hasExtension<RenderPassStoreOpsQCOM>();
#endif

  has.createRenderPass2 = 0;
#if VK_KHR_create_renderpass2
  has.createRenderPass2 |= Globals::VK::dev.hasExtension<CreateRenderPass2KHR>();
#endif

  has.renderTo3D = 0;
#if VK_KHR_maintenance1
  has.renderTo3D |= !bits.disableRenderTo3DImage && Globals::VK::dev.hasExtension<Maintenance1KHR>();
#endif

  // chosen to match SM6.0 requirements
  has.waveOperationsStageMask = 0;
  static const VkSubgroupFeatureFlags neededSubgroupFeatures = VK_SUBGROUP_FEATURE_BASIC_BIT | // Wave Query
                                                               VK_SUBGROUP_FEATURE_VOTE_BIT | // Wave Vote + (partially) Wave Reduction
                                                               VK_SUBGROUP_FEATURE_BALLOT_BIT |     // Wave Broadcast
                                                               VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | // Wave Scan and Prefix +
                                                                                                    // (partially) Wave Reduction
                                                               VK_SUBGROUP_FEATURE_QUAD_BIT         // Quad-wide Shuffle operations
    ;
  const bool supportsNeededSubgroupFeatures =
    (Globals::VK::phy.subgroupProperties.supportedOperations & neededSubgroupFeatures) == neededSubgroupFeatures;
  if (supportsNeededSubgroupFeatures)
    has.waveOperationsStageMask |= Globals::VK::phy.subgroupProperties.supportedStages;

  has.sixteenBitStorage = Globals::VK::phy.hasSixteenBitStorage;
}

void DriverConfig::configurePerDeviceDriverFeatures()
{
  {
    const DataBlock *pipeCompilerProp = getPerDriverPropertyBlock("pipelineCompiler");
    bits.usePipelineCacheMutex = pipeCompilerProp->getBool("usePipelineCacheMutex", false);
    if (bits.usePipelineCacheMutex)
      debug("vulkan: using external sync for pipeline cache mutex");

    bits.compileCachedGrPipelinesUsingSyncCompile =
      pipeCompilerProp->getBool("useSyncCompileForCachedGrPipelines", false) && Globals::VK::phy.hasPipelineCreationCacheControl;
    if (bits.compileCachedGrPipelinesUsingSyncCompile)
      debug("vulkan: using sync compile for cached graphics pipelines");

    bits.compileSeenGrPipelinesUsingSyncCompile = pipeCompilerProp->getBool("useSyncCompileForSeenGrPipelines", false);
    if (bits.compileSeenGrPipelinesUsingSyncCompile)
      debug("vulkan: using sync compile for seen graphics pipelines");
  }

  {
    const DataBlock *frameMemProp = getPerDriverPropertyBlock("frameMem");

    bits.allowFrameMem = frameMemProp->getBool("allow", true);
    if (bits.allowFrameMem)
      debug("vulkan: frame mem buffers enabled");

    bits.debugFrameMemUsage = frameMemProp->getBool("debugUsage", false);
    if (bits.debugFrameMemUsage)
      debug("vulkan: frame mem usage debug enabled");
  }

  {
    const DataBlock *latWaitProp = Globals::cfg.getPerDriverPropertyBlock("latencyWait");
    latencyWaitThresholdUs = latWaitProp->getInt64("thresholdUs", 800);
    latencyWaitDeltaUs = latWaitProp->getInt64("deltaUs", 100);
    // max wait is 33ms by default, for lower frame rate using latency compensation may trigger too much waits on frame rate changes
    latencyWaitMaxUs = latWaitProp->getInt64("maxWaitUs", 33 * 1000);

    bits.allowPredictedLatencyWaitApp = latWaitProp->getBool("waitApp", true);
    bits.autoPredictedLatencyWaitApp = latWaitProp->getBool("waitAppAuto", true);
    bits.allowPredictedLatencyWaitWorker = latWaitProp->getBool("waitWorker", true);
    debug("vulkan: allowPredictedLatencyWaitApp=%d, autoPredictedLatencyWaitApp=%d, allowPredictedLatencyWaitWorker=%d",
      bits.allowPredictedLatencyWaitApp, bits.autoPredictedLatencyWaitApp, bits.allowPredictedLatencyWaitWorker);
  }

  {
    const DataBlock *execTrackerProp = Globals::cfg.getPerDriverPropertyBlock("executionTracker");
    bits.enableDeviceExecutionTracker = execTrackerProp->getBool("enabled", false);
    // check only once, to enable tracker for all driver reinits in single app session
    static bool enableTrackerDueToLastCrash = dd_file_exists(getLastRunWasGPUFaultMarkerFile());
    if (enableTrackerDueToLastCrash && execTrackerProp->getBool("allowAutoEnableOnCrash", true))
    {
      dd_erase(getLastRunWasGPUFaultMarkerFile());
      bits.enableDeviceExecutionTracker = true;
      logwarn("vulkan: enabled device execution tracker due to GPU fault on last run");
    }
    executionTrackerBreakpoint = ((uint64_t)execTrackerProp->getInt("breakpointHI", 0) << 32ULL) |
                                 ((uint64_t)execTrackerProp->getInt("breakpointLO", 0) & 0xFFFFFFFF);
    if (executionTrackerBreakpoint)
      debug("vulkan: will break on execution hash %016llX", executionTrackerBreakpoint);
  }

  {
    const DataBlock *multiQueueProp = Globals::cfg.getPerDriverPropertyBlock("multiQueue");
    bits.allowMultiQueue = multiQueueProp->getBool("allow", false);
    if (bits.allowMultiQueue)
      debug("vulkan: using multi queue submit scheme");
  }
}

const DataBlock *DriverConfig::getPerDriverPropertyBlock(const char *prop_name)
{
  const DataBlock *propsBlk = ::dgs_get_settings()
                                ->getBlockByNameEx("vulkan")
                                ->getBlockByNameEx("vendor")
                                ->getBlockByNameEx(Globals::VK::phy.vendorName)
                                ->getBlockByNameEx("driverProps")
                                ->getBlockByNameEx(prop_name);

  uint32_t drvMajor = Globals::VK::phy.driverVersionDecoded[0];
  uint32_t drvMinor = Globals::VK::phy.driverVersionDecoded[1];
  uint32_t drvVerMax = 0x7FFFFFFF;

  const DataBlock *ret = &DataBlock::emptyBlock;
  dblk::iterate_child_blocks_by_name(*propsBlk, "entry", [&](const DataBlock &entry) {
    bool fitsMin = drvMajor >= entry.getInt("driverMinVer0", 0) && drvMinor >= entry.getInt("driverMinVer1", 0);
    bool fitsMax = drvMajor <= entry.getInt("driverMaxVer0", drvVerMax) && drvMinor <= entry.getInt("driverMaxVer1", drvVerMax);

    bool fitsType = true;
    bool gpuMatch = true;
    bool driverInfoMatch = true;
    if (entry.paramExists("hwType"))
      fitsType = Globals::VK::phy.properties.deviceType == entry.getInt("hwType", 0);

    if (entry.paramExists("gpu"))
    {
      gpuMatch = false;
      dblk::iterate_params_by_name(entry, "gpu",
        [&](int idx, int, int) { gpuMatch |= strstr(Globals::VK::phy.properties.deviceName, entry.getStr(idx)) != NULL; });
    }

    if (entry.paramExists("driverInfo"))
    {
      driverInfoMatch = false;
#if VK_KHR_driver_properties
      if (Globals::VK::phy.hasExtension<DriverPropertiesKHR>())
        dblk::iterate_params_by_name(entry, "driverInfo",
          [&](int idx, int, int) { driverInfoMatch |= strstr(Globals::VK::phy.driverProps.driverInfo, entry.getStr(idx)) != NULL; });
#endif
    }

    if (fitsMin && fitsMax && gpuMatch && driverInfoMatch && fitsType)
      ret = entry.getBlockByNameEx("data");
  });

  if (ret == &DataBlock::emptyBlock)
    ret = ::dgs_get_settings()
            ->getBlockByNameEx("vulkan")
            ->getBlockByNameEx("vendor")
            ->getBlockByNameEx("default")
            ->getBlockByNameEx("driverProps")
            ->getBlockByNameEx(prop_name);

  return ret;
}

void DriverConfig::extCapsFillConst(Driver3dDesc &caps)
{
  caps.zcmpfunc = 0;
  caps.acmpfunc = 0;
  caps.sblend = 0;
  caps.dblend = 0;
  caps.mintexw = 1;
  caps.mintexh = 1;
  caps.maxtexw = 0x7FFFFFFF;
  caps.maxtexh = 0x7FFFFFFF;
  caps.mincubesize = 1;
  caps.maxcubesize = 0x7FFFFFFF;
  caps.minvolsize = 1;
  caps.maxvolsize = 0x7FFFFFFF;
  caps.maxtexaspect = 0;
  caps.maxtexcoord = 0x7FFFFFFF;
  caps.maxsimtex = spirv::T_REGISTER_INDEX_MAX;
  caps.maxvertexsamplers = spirv::T_REGISTER_INDEX_MAX;
  caps.maxclipplanes = 0x7FFFFFFF;
  caps.maxstreams = 0x7FFFFFFF;
  caps.maxstreamstr = 0x7FFFFFFF;
  caps.maxvpconsts = 0x7FFFFFFF;
  caps.maxprims = 0x7FFFFFFF;
  caps.maxvertind = 0x7FFFFFFF;
  caps.upixofs = 0.f;
  caps.vpixofs = 0.f;
  caps.shaderModel = 6.0_sm;
  caps.maxSimRT = 0x7FFFFFFF;
  caps.is20ArbitrarySwizzleAvailable = true;
}

void DriverConfig::extCapsFillPCWinOnly(Driver3dDesc &caps)
{
  G_UNUSED(caps);
#if _TARGET_PC_WIN
  caps.caps.hasDepthReadOnly = true;
  caps.caps.hasStructuredBuffers = true;
  caps.caps.hasNoOverwriteOnShaderResourceBuffers = true;
  // Until implemented, for non pc this is constant false
  // DX11/DX12 approach with single sample RT attachment is not available on vulkan
  // only with combination of multisample DS and VK_AMD_mixed_attachment_samples/VK_NV_framebuffer_mixed_samples exts
  // VUID-VkGraphicsPipelineCreateInfo-subpass-00757
  // VUID-VkGraphicsPipelineCreateInfo-subpass-01411
  // VUID-VkGraphicsPipelineCreateInfo-subpass-01505
  caps.caps.hasForcedSamplerCount = false;
  caps.caps.hasVolMipMap = true;
  caps.caps.hasAsyncCompute = false;
  caps.caps.hasOcclusionQuery = false;
  caps.caps.hasConstBufferOffset = false;
  caps.caps.hasResourceCopyConversion = true;
  caps.caps.hasReadMultisampledDepth = true;
  caps.caps.hasGather4 = true;
  caps.caps.hasNVApi = false;
  caps.caps.hasATIApi = false;
  // needs VK_NV_shading_rate_image
  caps.caps.hasVariableRateShading = false;
  caps.caps.hasVariableRateShadingTexture = false;
  // no extension supports these yet
  caps.caps.hasVariableRateShadingShaderOutput = false;
  caps.caps.hasVariableRateShadingCombiners = false;
  caps.caps.hasVariableRateShadingBy4 = false;
  caps.caps.hasAliasedTextures = false;
  caps.caps.hasBufferOverlapCopy = false;
  caps.caps.hasBufferOverlapRegionsCopy = false;
  caps.caps.hasShader64BitIntegerResources = false;
  caps.caps.hasTiled2DResources = false;
  caps.caps.hasTiled3DResources = false;
  caps.caps.hasTiledSafeResourcesAccess = false;
  caps.caps.hasTiledMemoryAliasing = false;
  caps.caps.hasDLSS = false;
  caps.caps.hasXESS = false;
  caps.caps.hasMeshShader = false;
  caps.caps.hasBasicViewInstancing = false;
  caps.caps.hasOptimizedViewInstancing = false;
  caps.caps.hasAcceleratedViewInstancing = false;
  caps.caps.hasUAVOnEveryStage = true;
  caps.caps.hasDrawID = true;
  caps.caps.hasNativeRenderPassSubPasses = true;
  caps.caps.hasRayAccelerationStructure = false;
  caps.caps.hasRayQuery = false;
  caps.caps.hasRayDispatch = false;
  caps.caps.hasIndirectRayDispatch = false;
  caps.caps.hasGeometryIndexInRayAccelerationStructure = false;
  caps.caps.hasSkipPrimitiveTypeInRayTracingShaders = false;
  caps.caps.hasNativeRayTracePipelineExpansion = false;
  if (getPerDriverPropertyBlock("clearColorBug")->getBool("affected", false))
  {
    caps.issues.hasClearColorBug = true;
    debug("vulkan: DeviceDriverCapabilities::hasClearColorBug - GPU hangs on clearview of some targets with some colors");
  }
#endif
}

void DriverConfig::extCapsFillMultiplatform(Driver3dDesc &caps)
{
  G_UNUSED(caps);
#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID
  caps.caps.hasQuadTessellation = has.tesselationShader;
  caps.caps.hasResourceHeaps = getPerDriverPropertyBlock("resourceHeaps")->getBool("allowed", true);
  if (!caps.caps.hasResourceHeaps)
    debug("vulkan: resource heaps disabled by device-driver config");
  caps.caps.hasDepthBoundsTest = has.depthBoundsTest;
  // only partially instance id support, no idea how to expos this right.
  caps.caps.hasInstanceID = has.drawIndirectFirstInstance;
  caps.caps.hasConservativeRassterization = Globals::VK::dev.hasExtension<ConservativeRasterizationEXT>();

  caps.caps.hasWellSupportedIndirect = has.multiDrawIndirect;
  // Some GPUs will drop to immediate mode when multi draw indirect is present
  // this is performance-wise-bad, so allow to explicitly disable it via config
  if (getPerDriverPropertyBlock("disableMultiDrawIndirect")->getBool("affected", false))
    caps.caps.hasWellSupportedIndirect = false;
  debug("vulkan: hasWellSupportedIndirect=%d", (int)caps.caps.hasWellSupportedIndirect);

  caps.caps.hasUAVOnlyForcedSampleCount = has.UAVOnlyForcedSampleCount;
#if D3D_HAS_RAY_TRACING
  caps.caps.hasRayAccelerationStructure = Globals::VK::phy.hasAccelerationStructure;
  caps.caps.hasRayQuery = Globals::VK::phy.hasRayQuery;
  caps.caps.hasRayDispatch = Globals::VK::phy.hasRayTracingPipeline;
  caps.caps.hasIndirectRayDispatch = Globals::VK::phy.hasIndirectRayTrace;
  // TODO: Need to lookup if there is something like this
  caps.caps.hasGeometryIndexInRayAccelerationStructure = false;
  caps.caps.hasSkipPrimitiveTypeInRayTracingShaders = Globals::VK::phy.hasRayTracePrimitiveCulling;

  caps.raytrace.topAccelerationStructureInstanceElementSize = Globals::VK::phy.raytraceTopAccelerationInstanceElementSize;
  caps.raytrace.accelerationStructureBuildScratchBufferOffsetAlignment = Globals::VK::phy.raytraceScratchBufferAlignment;
  caps.raytrace.maxRecursionDepth = Globals::VK::phy.raytraceMaxRecursionDepth;

  // Explicitly disable pipeline raytracing until it's ready just like DX12 driver does
  caps.caps.hasRayDispatch = false;
  caps.caps.hasIndirectRayDispatch = false;
  // Only offer AS support when we can actually use it with anything
  // -V:caps.caps.hasRayAccelerationStructure:1048 not true
  caps.caps.hasRayAccelerationStructure = caps.caps.hasRayAccelerationStructure && caps.caps.hasRayQuery;
#endif
#endif
#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_C3 || _TARGET_ANDROID
  caps.caps.hasConditionalRender = has.conditionalRender;
#endif
#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID
  caps.caps.hasTileBasedArchitecture = Globals::cfg.getPerDriverPropertyBlock("hasTileBasedArchitecture")->getBool("affected", false);
#endif
}

void DriverConfig::extCapsFillAndroidOnly(Driver3dDesc &caps)
{
  G_UNUSED(caps);
#if _TARGET_ANDROID
  // ~99% of device / drivers support it, the remaining 1% is probably not compatible anyways
  caps.caps.hasAnisotropicFilter = has.anisotropicSampling;

  switch (Globals::VK::phy.vendorId)
  {
    // Qualcomm Adreno GPUs
    case D3D_VENDOR_QUALCOMM:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      // CS writes to 3d texture are not working for some reason
      caps.issues.hasComputeCanNotWrite3DTex = true;
      // driver bug/crash in pipeline construction when attachment ref is VK_ATTACHMENT_UNUSED
      caps.issues.hasStrictRenderPassesOnly = true;
      // have buggy/time limited compute stage
      caps.issues.hasComputeTimeLimited = true;

      // have buggy maxTexelBufferElements defenition, above 65k, but in real is 65k!
      // will device lost on some chips/drivers when sampling above 65k elements
      caps.issues.hasSmallSampledBuffers = true;

      caps.issues.hasRenderPassClearDataRace = getPerDriverPropertyBlock("adrenoClearStoreRace")->getBool("affected", false);

      break;
    // Arm Mali GPUs
    case D3D_VENDOR_ARM:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      // have buggy/time limited compute stage
      caps.issues.hasComputeTimeLimited = true;
      // have buggy maxTexelBufferElements defenition, above 65k, but in real is 65k!
      // will device lost on all chips/drivers when sampling above 65k elements
      caps.issues.hasSmallSampledBuffers = true;
      break;
    // Imagetec PowerVR GPUs and Samsung Exynos
    case D3D_VENDOR_IMGTEC:
    case D3D_VENDOR_SAMSUNG:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      break;
    default:
      // we can run android on non-mobile hardware too
      break;
  }

  if (getPerDriverPropertyBlock("msaaAndInstancingHang")->getBool("affected", false))
  {
    caps.issues.hasMultisampledAndInstancingHang = true;
    debug("vulkan: DeviceDriverCapabilities::bugMultisampledAndInstancingHang - GPU hangs with combination of MSAA, non-RGB8 RT and "
          "instancing in RIEx");
  }

  if (getPerDriverPropertyBlock("ignoreDeviceLost")->getBool("affected", false))
  {
    caps.issues.hasIgnoreDeviceLost = true;
    debug("vulkan: DeviceDriverCapabilities::bugIgnoreDeviceLost - spams device_lost, but continues to work as normal");
  }

  if (getPerDriverPropertyBlock("pollFences")->getBool("affected", false))
  {
    caps.issues.hasPollDeviceFences = true;
    debug("vulkan: DeviceDriverCapabilities::bugPollDeviceFences - vkWaitForFences randomly returns device_lost, use vkGetFenceStatus "
          "instead");
  }

  if (getPerDriverPropertyBlock("brokenShadersAfterAppSwitch")->getBool("affected", false))
  {
    caps.issues.hasBrokenShadersAfterAppSwitch = true;
    debug("vulkan: DeviceDriverCapabilities::hasBrokenShadersAfterAppSwitch - may not render some long shaders after switching from "
          "the application and back");
  }

  if (getPerDriverPropertyBlock("brokenMTRecreateImage")->getBool("affected", false))
  {
    caps.issues.hasBrokenMTRecreateImage = true;
    debug(
      "vulkan: DeviceDriverCapabilities::hasBrokenMTRecreateImage - mt texture recreation (e.g. when changing quality in settings) "
      "might cause a crash");
  }

  if (caps.caps.hasTileBasedArchitecture)
  {
    // only check on TBDRs, all other stuff is considered bugged driver or some software renderers
    // and we don't want to support them at cost of exploding shader variants
    // 0x4000000 is next majorly supported limit from vulkan.gpuinfo.org
    caps.issues.hasSmallSampledBuffers |= Globals::VK::phy.properties.limits.maxTexelBufferElements < 0x4000000;
    // FIXME:
    // issuing draws with
    //   -disabled depth test, enabled  depth bound
    //   -enabled  depth test, disabled depth bound
    // in one render pass corrupt frame on tilers
    // it looks like they can't decide who should write to RT
    // buggin it out by not writing data at all or flickering between said draws
    // we can't make separate passes for this situation right now
    // so disable it for time being
    caps.caps.hasDepthBoundsTest = false;
  }

  if (caps.issues.hasRenderPassClearDataRace)
    debug("vulkan: running on device-driver combo with clear-store race");

  if (getPerDriverPropertyBlock("disableShaderFloat16")->getBool("affected", false))
  {
    caps.caps.hasShaderFloat16Support = false;
    debug("vulkan: running on device-driver with bad shader float16 support, related features are OFF!");
  }

  if (getPerDriverPropertyBlock("brokenSRGBConverionWithMRT")->getBool("affected", false))
  {
    caps.issues.hasBrokenSRGBConverionWithMRT = true;
    debug("vulkan: running on device-driver with hasBrokenSRGBConverionWithMRT");
  }

  if (getPerDriverPropertyBlock("brokenComputeFormattedOutput")->getBool("affected", false))
  {
    caps.issues.hasBrokenComputeFormattedOutput = true;
    debug("vulkan: running on device-driver with hasBrokenComputeFormattedOutput");
  }

  caps.caps.hasLazyMemory = Globals::VK::phy.hasLazyMemory;
  debug("vulkan: hasLazyMemory=%d", (int)caps.caps.hasLazyMemory);

  if (getPerDriverPropertyBlock("brokenSubpasses")->getBool("affected", false))
  {
    caps.issues.hasBrokenSubpasses = true;
    debug("vulkan: running on device-driver with hasBrokenSubpasses");
  }

  if (getPerDriverPropertyBlock("brokenInstanceID")->getBool("affected", false))
  {
    caps.issues.hasBrokenBaseInstanceID = true;
    debug("vulkan: running on device-driver with brokenInstanceID");
  }
#endif
}

void DriverConfig::extCapsFillUniversal(Driver3dDesc &caps)
{
  VkPhysicalDeviceProperties &properties = Globals::VK::phy.properties;
  caps.caps.hasBindless = false;
  caps.caps.hasRenderPassDepthResolve = Globals::VK::phy.hasDepthStencilResolve;

#define GJN_DEPTH_RESOLVE_MODES          \
  GJN_DEPTH_RESOLVE_MODE_FN(SAMPLE_ZERO) \
  GJN_DEPTH_RESOLVE_MODE_FN(AVERAGE)     \
  GJN_DEPTH_RESOLVE_MODE_FN(MIN)         \
  GJN_DEPTH_RESOLVE_MODE_FN(MAX)

#define GJN_DEPTH_RESOLVE_MODE_VK(baseName)  VK_RESOLVE_MODE_##baseName##_BIT
#define GJN_DEPTH_RESOLVE_MODE_D3D(baseName) DepthResolveMode::DEPTH_RESOLVE_MODE_##baseName

  VkResolveModeFlags depthResolveModes = Globals::VK::phy.depthStencilResolveProps.supportedDepthResolveModes;
  if (Globals::VK::phy.depthStencilResolveProps.independentResolveNone != VK_TRUE)
  {
    depthResolveModes &= Globals::VK::phy.depthStencilResolveProps.supportedStencilResolveModes;
  }
  {
#define GJN_DEPTH_RESOLVE_MODE_FN(modeName)                         \
  if (depthResolveModes & GJN_DEPTH_RESOLVE_MODE_VK(modeName))      \
  {                                                                 \
    caps.depthResolveModes |= GJN_DEPTH_RESOLVE_MODE_D3D(modeName); \
    debug("vulkan: depth resolve mode %s supported", #modeName);    \
  }
    GJN_DEPTH_RESOLVE_MODES
#undef GJN_DEPTH_RESOLVE_MODE_FN
  }
#undef GJN_DEPTH_RESOLVE_MODE_D3D
#undef GJN_DEPTH_RESOLVE_MODE_VK
#undef GJN_DEPTH_RESOLVE_MODES

  caps.shaderModel = has.sixteenBitStorage ? 6.2_sm : 6.0_sm;
  VkShaderStageFlags requiredWaveStages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
  if ((has.waveOperationsStageMask & requiredWaveStages) != requiredWaveStages)
  {
    caps.shaderModel = 5.0_sm;
    caps.caps.hasWaveOps = false;
  }
  else
    caps.caps.hasWaveOps = true;

  if (!has.fragmentShaderUAV)
  {
    // not entirely correct
    caps.shaderModel = 4.1_sm;
  }

  caps.caps.hasShaderFloat16Support = Globals::VK::phy.hasShaderFloat16;
  caps.maxtexw = min(caps.maxtexw, (int)properties.limits.maxImageDimension2D);
  caps.maxtexh = min(caps.maxtexh, (int)properties.limits.maxImageDimension2D);
  caps.maxcubesize = min(caps.maxcubesize, (int)properties.limits.maxImageDimensionCube);
  caps.maxvolsize = min(caps.maxvolsize, (int)properties.limits.maxImageDimension3D);
  caps.maxtexaspect = max(caps.maxtexaspect, 0);
  caps.maxtexcoord = min(caps.maxtexcoord, (int)properties.limits.maxVertexInputAttributes);
  caps.maxsimtex = min(caps.maxsimtex, (int)properties.limits.maxDescriptorSetSampledImages);
  caps.maxvertexsamplers = min(caps.maxvertexsamplers, (int)properties.limits.maxPerStageDescriptorSampledImages);
  caps.maxclipplanes = min(caps.maxclipplanes, (int)properties.limits.maxClipDistances);
  caps.maxstreams = min(caps.maxstreams, (int)properties.limits.maxVertexInputBindings);
  caps.maxstreamstr = min(caps.maxstreamstr, (int)properties.limits.maxVertexInputBindingStride);
  caps.maxvpconsts = min(caps.maxvpconsts, (int)(properties.limits.maxUniformBufferRange / (sizeof(float) * 4)));
  caps.maxprims = min(caps.maxprims, (int)properties.limits.maxDrawIndexedIndexValue);
  caps.maxvertind = min(caps.maxvertind, (int)properties.limits.maxDrawIndexedIndexValue);
  caps.maxSimRT = min(caps.maxSimRT, (int)properties.limits.maxColorAttachments);
  caps.minWarpSize = Globals::VK::phy.warpSize;
  caps.maxWarpSize = Globals::VK::phy.warpSize;
}

void DriverConfig::setBindlessConfig()
{
  if (!Globals::VK::phy.hasBindless)
    return;

  const DataBlock *propsBlk = ::dgs_get_settings()->getBlockByNameEx("vulkan");

  // disable bindless by default, enable bindless by per-project configs
  const bool enableBindless = propsBlk->getBool("enableBindless", false);
  // some drivers have buggy bindless support, allow to disable it there if needed
  const bool driverDisabledBindless = Globals::cfg.getPerDriverPropertyBlock("disableBindless")->getBool("affected", !enableBindless);
  // we use static sets layout, so we must support MAX possible combo instead of max used
  const bool hasEnoughDescriptorSets =
    Globals::VK::phy.properties.limits.maxBoundDescriptorSets >= spirv::graphics::MAX_SETS + spirv::bindless::MAX_SETS;

  bindlessSetLimits[spirv::bindless::TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX] = {
    256 * 1024, 0, 0, Globals::VK::phy.maxBindlessTextures, "bindlessTextureCountMin", "bindlessTextureCountMax", "textures", false};
  bindlessSetLimits[spirv::bindless::SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX] = {
    2048, 0, 0, Globals::VK::phy.maxBindlessSamplers, "bindlessSamplerCountMin", "bindlessSamplerCountMax", "samplers", false};
  bindlessSetLimits[spirv::bindless::BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX] = {
    256 * 1024, 0, 0, Globals::VK::phy.maxBindlessBuffers, "bindlessBufferCountMin", "bindlessBufferCountMax", "buffers", false};

  bool allLimitsFit = true;
  for (BindlessSetLimit &i : bindlessSetLimits)
  {
    i.maxReq = propsBlk->getInt(i.configPathMax, i.maxReq);
    i.minReq = propsBlk->getInt(i.configPathMin, i.maxReq);
    i.fits = i.max >= i.minReq;
    i.slots = min(i.max, i.maxReq);
    allLimitsFit &= i.fits;
  }

  Globals::VK::phy.hasBindless = hasEnoughDescriptorSets && allLimitsFit && !driverDisabledBindless;

  if (Globals::VK::phy.hasBindless)
  {
    debug("vulkan: bindless enabled");
    for (BindlessSetLimit &i : bindlessSetLimits)
      debug("vulkan: bindless %s limit: %u (inside %u-%u range) slots, %u max", i.limitName, i.slots, i.minReq, i.maxReq, i.max);
  }
  else
  {
    debug("vulkan: bindless disabled [%s %s %s %s]", enableBindless ? "-" : "global", driverDisabledBindless ? "hw_driver" : "-",
      hasEnoughDescriptorSets ? "-" : "descriptor_sets", allLimitsFit ? "-" : "limits");
    if (!allLimitsFit)
      for (BindlessSetLimit &i : bindlessSetLimits)
        if (!i.fits)
          debug("vulkan: bindless limit %s not fitting to req (%u-%u) (max %u)", i.limitName, i.minReq, i.maxReq, i.max);
  }
}

void DriverConfig::fillExternalCaps(Driver3dDesc &caps)
{
  extCapsFillConst(caps);
  extCapsFillUniversal(caps);
  extCapsFillPCWinOnly(caps);
  extCapsFillMultiplatform(caps);
  extCapsFillAndroidOnly(caps);

  // bindless can fail and depends on mobile caps
  // do it right here to properly reflect dependencies
  setBindlessConfig();
  caps.caps.hasBindless = Globals::VK::phy.hasBindless;
}

const char *DriverConfig::getFaultVendorDumpFile() { return "vk_fault_vendor_dump.bin"; }

const char *DriverConfig::getLastRunWasGPUFaultMarkerFile() { return "vk_last_run_gpu_fault.bin"; }
