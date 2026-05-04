// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef _DAG_DRIVERDESC_INL_
#define _DAG_DRIVERDESC_INL_

#include "dag_consts.h"
#include "dag_shaderModelVersion.h"

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_versionQuery.h>

#include <EASTL/fixed_string.h>
#include <EASTL/string_view.h>


/**
 * \brief Contains non-volatile info about the currently used GPU.
 */
struct DeviceAttributesBase
{
  static constexpr uint32_t UNKNOWN = 0;

  // Source amd_ags.h
  enum AmdFamily : uint32_t
  {
    PRE_GCN,
    GCN1,
    GCN2,
    GCN3,
    GCN4,
    Vega,
    RDNA,
    RDNA2,
    RDNA3,
    RDNA4,
  };

  // Source https://github.com/intel/intel-graphics-compiler/blob/master/inc/common/igfxfmid.h
  enum IntelFamily : uint32_t
  {
    SANDYBRIDGE = 1,
    IVYBRIDGE,
    HASWELL,
    VALLEYVIEW,
    BROADWELL,
    CHERRYVIEW,
    SKYLAKE,
    KABYLAKE,
    COFFEELAKE,
    WILLOWVIEW,
    BROXTON,
    GEMINILAKE,
    WHISKEYLAKE,
    CANNONLAKE,
    COMETLAKE,
    ICELAKE,
    ICELAKE_LP,
    LAKEFIELD,
    ELKHARTLAKE,
    JASPERLAKE,
    TIGERLAKE_LP,
    ROCKETLAKE,
    ALDERLAKE_S,
    ALDERLAKE_P,
    ALDERLAKE_N,
    TWINLAKE,
    RAPTORLAKE,
    RAPTORLAKE_S,
    RAPTORLAKE_P,
    DG1,
    XE_HP_SDV,
    ALCHEMIST,
    PONTEVECCHIO,
    METEORLAKE,
    ARROWLAKE_H,
    ARROWLAKE_S,
    BATTLEMAGE,
    LUNARLAKE,
    PANTHERLAKE,
    WILDCATLAKE,
  };

  // Source nvapi.h
  enum NvidiaFamily : uint32_t
  {
    T2X = 0xE0000020,
    T3X = 0xE0000030,
    T4X = 0xE0000040,
    T12X = 0xE0000040,
    NV40 = 0x00000040,
    NV50 = 0x00000050,
    G78 = 0x00000060,
    G80 = 0x00000080,
    G90 = 0x00000090,
    GT200 = 0x000000A0,
    GF100 = 0x000000C0,
    GF110 = 0x000000D0,
    GK100 = 0x000000E0,
    GK110 = 0x000000F0,
    GK200 = 0x00000100,
    GM000 = 0x00000110,
    GM200 = 0x00000120,
    GP100 = 0x00000130,
    GV100 = 0x00000140,
    GV110 = 0x00000150,
    TU100 = 0x00000160,
    GA100 = 0x00000170,
    AD100 = 0x00000190,
    GB200 = 0x000001B0,
  };

  /**
   * \brief Dagor specific value about the vendor of the currently use GPU.
   */
  GpuVendor vendor = GpuVendor::UNKNOWN;
  /**
   * \brief Unified Memory Architecture, true if the GPU is integrated.
   */
  bool isUMA = false;
  /**
   * \brief ID of the vendor.
   */
  uint32_t vendorId = UNKNOWN;
  /**
   * \brief ID of the device.
   */
  uint32_t deviceId = UNKNOWN;
  /**
   * \brief Internal representation of the currently used GPU's microarchitecture, aka family.
   */
  uint32_t family = UNKNOWN;
  /**
   * \brief The currently used GPU's driver version.
   */
  DriverVersion driverVersion = {};
  /**
   * \brief The release date of the GPU driver.
   */
  DagorDateTime driverDate = {};
};

/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \xbone.
 */
struct DeviceAttributesXboxOne : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::AMD;
  static constexpr bool isUMA = true;
  static constexpr uint32_t vendorId = UNKNOWN;
  static constexpr uint32_t deviceId = UNKNOWN;
  static constexpr DriverVersion driverVersion = {};
  static constexpr DagorDateTime driverDate = {};
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for
 * \scarlett.
 */
struct DeviceAttributesScarlett : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::AMD;
  static constexpr bool isUMA = true;
  static constexpr uint32_t vendorId = UNKNOWN;
  static constexpr uint32_t deviceId = UNKNOWN;
  static constexpr uint32_t family = RDNA2;
  static constexpr DriverVersion driverVersion = {};
  static constexpr DagorDateTime driverDate = {};
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \ps4.
 */
struct DeviceAttributesPS4 : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::AMD;
  static constexpr bool isUMA = true;
  static constexpr uint32_t vendorId = UNKNOWN;
  static constexpr uint32_t deviceId = UNKNOWN;
  static constexpr DriverVersion driverVersion = {};
  static constexpr DagorDateTime driverDate = {};
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \ps5.
 */
struct DeviceAttributesPS5 : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::AMD;
  static constexpr bool isUMA = true;
  static constexpr uint32_t vendorId = UNKNOWN;
  static constexpr uint32_t deviceId = UNKNOWN;
  static constexpr DriverVersion driverVersion = {};
  static constexpr DagorDateTime driverDate = {};
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \ios.
 */
struct DeviceAttributesIOS : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::APPLE;
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \tvos.
 */
struct DeviceAttributesTVOS : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::APPLE;
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for
 * \nswitch.
 */
struct DeviceAttributesNintendoSwitch : DeviceAttributesBase
{
  static constexpr GpuVendor vendor = GpuVendor::NVIDIA;
  static constexpr bool isUMA = true;
  static constexpr DagorDateTime driverDate = {};
};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for
 * \android.
 */
struct DeviceAttributesAndroid : DeviceAttributesBase
{};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \mac.
 */
struct DeviceAttributesMacOSX : DeviceAttributesBase
{};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for \linux.
 */
struct DeviceAttributesLinux : DeviceAttributesBase
{};
/**
 * \brief Optimized device attribute structure, hiding bitfield entries with static const values of known platform features for
 * \windows.
 */
struct DeviceAttributesWindows : DeviceAttributesBase
{};

#if _TARGET_XBOXONE
using DeviceAttributes = DeviceAttributesXboxOne;
#elif _TARGET_SCARLETT
using DeviceAttributes = DeviceAttributesScarlett;
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS
using DeviceAttributes = DeviceAttributesIOS;
#elif _TARGET_TVOS
using DeviceAttributes = DeviceAttributesTVOS;
#elif _TARGET_C3

#elif _TARGET_ANDROID
using DeviceAttributes = DeviceAttributesAndroid;
#elif _TARGET_PC_MACOSX
using DeviceAttributes = DeviceAttributesMacOSX;
#elif _TARGET_PC_LINUX
using DeviceAttributes = DeviceAttributesLinux;
#elif _TARGET_PC_WIN
using DeviceAttributes = DeviceAttributesWindows;
#else
using DeviceAttributes = DeviceAttributesBase;
#endif

/**
 * A boolean bitfield that describes which optional features that are available with the used device / driver combination.
 * \note
 * See derived types for platform specific constant overrides.
 * \remark To add new capability indicators, you need to follow those steps:
 * -# Add the new cap indicator boolean bit to the end of this structure.
 * -# For platforms where this cap is always be available or not available, add a static constexpr boolean with the same name with the
 * platform specific derived structures. This will hide the boolean bitfield for the target platform and the constant will be used
 * instead.
 * -# Add the documentation to all the platform specific constant overrides. Use existing documentation as a basis. Here are the
 * basics:
 *  * Use \\briefconstcap{"const value", DeviceDriverCapabilitiesBase::"cap name"} to generate the basic description for the overriding
 * value.
 *    + "const value" should be replaced with the constant value of this override.
 *    + "cap name" should be the name of the cap.
 *  * Use \\NYI to indicate that the driver could support this feature, but do not implemented it yet.
 * -# Add documentation to the cap indicator boolean bit of this struct.
 *  * Use \\capbrief "brief cap description" to generate the brief description of this cap. This alias will start the brief with the
 * standard introducing phrase "Indicates that the device driver".
 *  * Use \\platformtable{"cap name","xbone","scarlett","ps4","ps5","ios","tvos","nswitch","android","mac","linux","win32"} to generate
 * the platform value table.
 *    + "cap name" should be the name of the cap.
 *    + "xbone", "scarlett", "ps4", "ps5", "ios", "tvos", "nswitch", "android", "mac", "linux", "win32" should be one of the following
 * values:
 *      - c for constant, the actual value is pulled from the description generated by \\briefconstcap for the cosponsoring platform.
 *      - a for alias, this works only for "scarlett" and "ps5", for others this is the same as c. It uses the value of "xbone" and
 * "ps4", for "scarlett" and "ps5" respectively. This is needed when a const value is specified for "xbone" or "ps4" platform and
 * inherited by the "scarlett" or "ps5" platform, to correctly display the inherited value.
 *      - r for runtime determined. This is for platforms where there is no constant override and it is determined by the active driver
 * of the platform.
 *    + Ensure that there are no spaces around the platform specific values (c, a or r), otherwise the alias expansion will result in
 * broken statements and documentation.
 *  * Use \\someNYI to indicate that there are drivers that could support this feature, but do not implement it yet.
 */
struct DeviceDriverCapabilitiesBase
{
  /**
   * \capbrief supports anisotropic filtering of textures.
   * \details Devices without support will silently treat any value for anisotropic filtering as 1.0.
   * \platformtable{hasAnisotropicFilter,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasAnisotropicFilter : 1;
  /**
   * \capbrief supports constant depth stencil targets that allow simultaneous sampling as a shader resource.
   * \platformtable{hasDepthReadOnly,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasDepthReadOnly : 1;
  /**
   * \capbrief supports structured buffer types.
   * \platformtable{hasStructuredBuffers,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasStructuredBuffers : 1;
  /**
   * \capbrief supports locking of structured buffers with the no-overwrite method.
   * \platformtable{hasNoOverwriteOnShaderResourceBuffers,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasNoOverwriteOnShaderResourceBuffers : 1;
  /**
   * \capbrief supports forced multi-sample count during raster phase.
   * \platformtable{hasForcedSamplerCount,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasForcedSamplerCount : 1;
  /**
   * \capbrief supports mipmaps for vol (eg 3D) textures.
   * \platformtable{hasVolMipMap,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasVolMipMap : 1;
  /**
   * \capbrief supports \ref GpuPipeline::ASYNC_COMPUTE as selected pipeline when applicable.
   * \someNYI
   * \platformtable{hasAsyncCompute,c,a,r,r,c,c,c,c,c,c,r}
   */
  bool hasAsyncCompute : 1;
  /**
   * \capbrief supports occlusion queries.
   * \platformtable{hasOcclusionQuery,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasOcclusionQuery : 1;
  /**
   * \capbrief supports values other than 0 for \p consts_offset of \ref d3d::set_const_buffer
   * \platformtable{hasConstBufferOffset,c,a,c,a,c,c,c,c,c,c,r}
   * \someNYI
   */
  bool hasConstBufferOffset : 1;
  /**
   * \capbrief supports depth bounds testing. See @ref d3d::set_depth_bounds for details of depth bounds testing.
   * \platformtable{hasDepthBoundsTest,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasDepthBoundsTest : 1;
  /**
   * \capbrief supports conditional rendering.
   * \platformtable{hasConditionalRender,c,a,c,a,c,c,r,r,c,r,r}
   */
  bool hasConditionalRender : 1;
  /**
   * \capbrief supports copy of textures of different but compatible formats.
   * \platformtable{hasResourceCopyConversion,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasResourceCopyConversion : 1;
  /**
   * \capbrief supports asynchronously issued copy operations.
   * \platformtable{hasAsyncCopy,c,a,c,a,c,c,c,c,c,c,c}
   */
  bool hasAsyncCopy : 1;
  /**
   * \capbrief supports reading from multi-sampled depth stencil targets.
   * \platformtable{hasReadMultisampledDepth,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasReadMultisampledDepth : 1;
  /**
   * \capbrief supports instance id system variable in shaders.
   * \platformtable{hasInstanceID,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasInstanceID : 1;
  /**
   * \capbrief supports conservative rastering.
   * \someNYI
   * \platformtable{hasConservativeRassterization,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasConservativeRassterization : 1;
  /**
   * \capbrief supports quad tessellation.
   * \someNYI
   * \platformtable{hasQuadTessellation,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasQuadTessellation : 1;
  /**
   * \capbrief supports gather intrinsic in shaders.
   * \platformtable{hasGather4,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasGather4 : 1;
  /**
   * \capbrief supports the alpha to coverage raster feature.
   * \todo Candidate for removal if situation for PS4/5 is clarified, there it seems to be not implemented.
   * \someNYI
   * \platformtable{hasAlphaCoverage,c,a,c,a,c,c,c,c,c,c,c}
   */
  bool hasAlphaCoverage : 1;
  /**
   * \capbrief supports indirect drawing.
   * \platformtable{hasWellSupportedIndirect,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasWellSupportedIndirect : 1;
  /**
   * \capbrief supports the bindless API and resource access in shaders.
   * \todo May need to split into multiple caps as some drivers do not support buffers.
   * \bug \nswitch is experimental support and so is not a constant cap yet.
   * \platformtable{hasBindless,c,a,c,a,c,c,r,r,r,r,r}
   */
  bool hasBindless : 1;
  /**
   * \capbrief has detected that the Nvidia driver API is available for the render device.
   * \platformtable{hasNVApi,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasNVApi : 1;
  /**
   * \capbrief has detected that the \ati driver API is available for the render device.
   * \platformtable{hasATIApi,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasATIApi : 1;
  /**
   * \capbrief supports basic per draw shading rates.
   * \someNYI
   * \platformtable{hasVariableRateShading,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasVariableRateShading : 1;
  /**
   * \capbrief supports shading rate textures as a source of shading rate information.
   * \someNYI
   * \platformtable{hasVariableRateShadingTexture,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasVariableRateShadingTexture : 1;
  /**
   * \capbrief supports shader generated shading rates.
   * \someNYI
   * \platformtable{hasVariableRateShadingShaderOutput,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasVariableRateShadingShaderOutput : 1;
  /**
   * \capbrief supports combiners for variable rate shading to select the final shading rate.
   * \someNYI
   * \platformtable{hasVariableRateShadingCombiners,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasVariableRateShadingCombiners : 1;
  /**
   * \capbrief supports variable rate shading blocks with sizes of 4 in X and Y direction.
   * \someNYI
   * \platformtable{hasVariableRateShadingBy4,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasVariableRateShadingBy4 : 1;
  /**
   * \capbrief supports stream output.
   * \someNYI
   * \platformtable{
   * hasStreamOutput, c, a, c, a, c, c, c, c, c, c, r}
   */
  bool hasStreamOutput : 1;
  /**
   * \capbrief supports creation of aliased textures.
   * \someNYI
   * \platformtable{hasAliasedTextures,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasAliasedTextures : 1;
  /**
   * \capbrief supports the resource heap API.
   * \someNYI
   * \platformtable{hasResourceHeaps,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasResourceHeaps : 1;
  /**
   * \capbrief supports overlapping buffer copies.
   * \platformtable{hasBufferOverlapCopy,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasBufferOverlapCopy : 1;
  /**
   * \capbrief uses region based copies when overlapping copies are executed.
   * \platformtable{hasBufferOverlapRegionsCopy,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasBufferOverlapRegionsCopy : 1;
  /**
   * \capbrief supports forced multisample count without render targets and only outputting to UAVs.
   * \someNYI
   * \platformtable{hasUAVOnlyForcedSampleCount,c,a,c,a,c,c,c,r,r,r,r}
   * \bug There is no way of querying the number of samples that are supported. So the Vulkan driver, for example, assumes 1, 2, 4 and
   * 8 samples to be required.
   */
  bool hasUAVOnlyForcedSampleCount : 1;
  /**
   * \capbrief supports 64 bit integers shader resource types.
   * \platformtable{hasShader64BitIntegerResources,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasShader64BitIntegerResources : 1;
  /**
   * \capbrief is not emulating render sup-passes.
   * \platformtable{hasNativeRenderPassSubPasses,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasNativeRenderPassSubPasses : 1;
  /**
   * \capbrief supports tiled 2D textures.
   * \platformtable{hasTiled2DResources,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasTiled2DResources : 1;
  /**
   * \capbrief supports tiled 3D textures;
   * \platformtable{hasTiled3DResources,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasTiled3DResources : 1;
  /**
   * \capbrief supports safe read and write access for not mapped tiles of tiled resources. Such reads return 0
   * and writes are ignored.
   * \platformtable{hasTiledSafeResourcesAccess,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasTiledSafeResourcesAccess : 1;
  /**
   * \capbrief supports memory aliasing of multiple tiles.
   * \platformtable{hasTiledMemoryAliasing,c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasTiledMemoryAliasing : 1;
  /**
   * \capbrief supports Nvidia DLSS.
   * DLSS stand for Deep Learning Super Sampling.
   * \platformtable{hasDLSS,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasDLSS : 1;
  /**
   * \capbrief supports Intel XESS.
   * XESS stands for Xe Super Sampling.
   * \platformtable{hasXESS,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasXESS : 1;
  /**
   * \capbrief supports draw id in shaders.
   * \platformtable{hasDrawID,c,a,c,a,c,c,c,c,r,c,r}
   */
  bool hasDrawID : 1;
  /**
   * \capbrief supports the mesh shader pipeline.
   * \details The mesh shader pipeline is a one or two shader stage that replaces the vertex shader based pre raster portion of the
   * graphics pipeline. Mesh shaders are compute shaders that generate vertices and indices which form primitives like triangles or
   * lines. \platformtable{hasMeshShader,c,a,c,a,r,c,c,c,r,c,r}
   */
  bool hasMeshShader : 1;
  /**
   * \capbrief supports the mesh shader indirect count function.
   * \details if true the dispatch_mesh_indirect_count is supported
   * \platformtable{hasMeshShaderIndirectCount,c,a,c,a,r,c,c,c,r,c,r}
   */
  bool hasMeshShaderIndirectCount : 1;
  /**
   * \capbrief supports basic view instancing.
   * \details View instancing may be implemented with replicating render commands for each view.
   * \platformtable{hasBasicViewInstancing,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasBasicViewInstancing : 1;
  /**
   * \capbrief has some optimizations for view instanced rendering.
   * \platformtable{hasOptimizedViewInstancing,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasOptimizedViewInstancing : 1;
  /**
   * \capbrief has hardware acceleration to natively support view instanced rendering.
   * \platformtable{hasAcceleratedViewInstancing,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasAcceleratedViewInstancing : 1;
  /**
   * \capbrief can resolve multi-sampled depth stencil render targets in a render pass.
   * \platformtable{hasRenderPassDepthResolve,c,a,c,a,r,r,r,r,r,r,r}
   */
  bool hasRenderPassDepthResolve : 1;
  /**
   * \capbrief supports stereo render expansion.
   * \platformtable{hasStereoExpansion,c,a,c,c,c,c,c,c,c,c,c}
   */
  bool hasStereoExpansion : 1;
  /**
   * \capbrief architecture is known to be a tile renderer.
   * \details Tile renderers bin raster work into tiles and execute rastering and pixel shaders tile by tile to reduce bandwidth to off
   * chip memory (RAM). Certain optimizations favor regular rasterers over tiled rasterers and vice versa.
   * \platformtable{hasTileBasedArchitecture,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasTileBasedArchitecture : 1;
  /**
   * \capbrief supports lazily allocated memory.
   * \details Supported on most TBDR GPUs, allocates memory by on-chip tile/framebuffer memory for transient render targets.
   * \platformtable{hasLazyMemory,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasLazyMemory : 1;
  /**
   * \capbrief set when hw doesn't support indirect calls
   * \details This is at least true on some of the older iOS hardware (A8) which can't do either drawIndirect nor dispatchIndirect
   * \platformtable{hasIndirectSupport,c,c,c,c,r,r,c,c,c,c,c}
   */
  bool hasIndirectSupport : 1;
  /**
   * \capbrief set when hw doesn't support comparison samplers
   * \details This is at least true on some of the older iOS hardware (A8) which can only do constexp inline samplers
   * and we don't wanna open this can of worm
   * \platformtable{hasCompareSampler,c,c,c,c,r,r,c,c,c,c,c}
   */
  bool hasCompareSampler : 1;
  /**
   * \capbrief supports 16-bit floating-point types in shaders for arithmetic operations.
   * \someNYI
   * \platformtable{"hasShaderFloat16Support",c,a,c,a,c,c,r,r,c,r,r}
   */
  bool hasShaderFloat16Support : 1;
  /**
   * \capbrief supports UAV access in every shader stage, without this cap UAV is only available in pixel and compute shaders.
   * \someNYI
   * \platformtable{"hasUAVOnEveryStage",c,a,c,a,c,c,c,r,c,r,r}
   */
  bool hasUAVOnEveryStage : 1;
  /**
   * \capbrief supports acceleration structures for ray tracing / ray queries, will be true if hasRayAccelerationStructure,
   * hasRayQuery and / or hasRayDispatch is supported. The vertex format of VSDT_FLOAT3 is guaranteed, for additional formats
   * d3d::raytrace::check_vertex_format_support_for_acceleration_structure_build needs to be used to check for support.
   * \someNYI
   * \platformtable{hasRayAccelerationStructure,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasRayAccelerationStructure : 1;
  /**
   * \capbrief supports ray queries ("inline ray tracing") in any shader stage.
   * \someNYI
   * \platformtable{hasRayQuery,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasRayQuery : 1;
  /**
   * \capbrief supports ray dispatch with its own ray tracing shader stage.
   * \someNYI
   * \platformtable{hasRayDispatch,c,c,c,a,c,c,c,r,c,r,r}
   */
  bool hasRayDispatch : 1;
  /**
   * \capbrief supports indirect dispatch for dispatch rays.
   * \someNYI
   * \platformtable{hasIndirectRayDispatch,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasIndirectRayDispatch : 1;
  /**
   * \capbrief uses emulation withing gapi for ray tracing features.
   * \someNYI
   * \platformtable{hasEmulatedRaytracing,c,c,c,c,r,c,c,c,r,c,c}
   */
  bool hasEmulatedRaytracing : 1;
  /**
   * \capbrief supports geometry index information in acceleration structures in ray tracing / ray query shaders.
   * \someNYI
   * \platformtable{hasGeometryIndexInRayAccelerationStructure,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasGeometryIndexInRayAccelerationStructure : 1;
  /**
   * \capbrief supports masking flags in shaders to ignore triangle or procedural geometry during traversal.
   * \someNYI
   * \platformtable{hasSkipPrimitiveTypeInRayTracingShaders,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasSkipPrimitiveTypeInRayTracingShaders : 1;
  /**
   * \capbrief is false when HW does not support draw cals with non-zero baseVertex
   * \details This feature is not supported by older iOS hardware (such as A8 chips)
   * \platformtable{hasBaseVertexSupport,c,c,c,c,r,r,c,c,r,c,c}
   */
  bool hasBaseVertexSupport : 1;
  /**
   * \capbrief castingFullyTypedFormatsSupported
   supports casting (aliasing) fully typed formats between each other (same as op3.CastingFullyTypedFormatSupported in DX12)
   * \someNYI
   * \platformtable{castingFullyTypedFormatsSupported,c,c,c,c,r,r,c,r,r,r,r}
   */
  bool castingFullyTypedFormatsSupported : 1;
  /*
   * \capbrief supports expansion of ray trace pipelines without extra effort by our driver.
   * \someNYI
   * \platformtable{hasNativeRayTracePipelineExpansion,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasNativeRayTracePipelineExpansion : 1;

  /**
   * \capbrief supports wave ops
   *    + "xbone", "scarlett", "ps4", "ps5", "ios", "tvos", "nswitch", "android", "mac", "linux", "win32" should be one of the
   *following \platformtable{hasWaveOps,c,a,c,a,c,c,r,r,c,r,r} \brief supports waveOps
   **/
  bool hasWaveOps : 1;
  /**
   * \capbrief hasPersistentShaderHandles indicates that shader handles of pipeline objects, that are used for shader binding tables,
   * are persistent and are the same for derived pipelines and pipelines that use the same shader from the same shader library.
   * \platformtable{hasPersistentShaderHandles,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasPersistentShaderHandles : 1;
  /**
   * \capbrief hasDualSourceBlending indicates that dual source blending is available (only for one render target).
   * \someNYI
   * \platformtable{hasDualSourceBlending,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasDualSourceBlending : 1;
  /**
   * \capbrief Supports the Opacity Micro Maps (OMM) acceleration structure type. This type is a extension to the basic ray tracing
   * acceleration structure concept to speed up intersection tests against (partially) transparent geometry (like tree leafs). \someNYI
   * \platformtable{hasRayTraceOpacityMicroMapTriangleArrays,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasRayTraceOpacityMicroMapTriangleArrays : 1;
  /**
   * \capbrief hasRayTraceOpacityMicroMapTriangleArrays provided by \nvidia extension.
   * \note This is a mutual exclusive feature cap with hasRayTraceOpacityMicroMapTriangleArrays. Where
   * hasRayTraceOpacityMicroMapTriangleArrays indicates support all OMM features by the core API / multi vendor extensions.
   * hasNvidiaRayTraceOpacityMicroMapTriangleArrays indicates implementation of OMM by the \nvidia driver with driver extensions.
   *
   * \someNYI
   * \platformtable{hasNvidiaRayTraceOpacityMicroMapTriangleArrays,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays : 1;
  /**
   * \capbrief hasAtomicInt64OnGroupShared Indicates if the device can use 64 bit integer atomic ops on group shared memory.
   * \platformtable{hasAtomicInt64OnGroupShared,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasAtomicInt64OnGroupShared : 1;
  /**
   * \capbrief supports Unordered Access View (UAV).
   * \note Some GPU drivers have broken UAV support.
   * \someNYI
   * \platformtable{hasProperUAVSupport, c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasProperUAVSupport : 1;
  /**
   * \capbrief Supports shader execution reorder (SER) shader commands.
   *
   * \someNYI
   * \platformtable{hasRayTraceShaderExecutionReorder,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasRayTraceShaderExecutionReorder : 1;
  /**
   * \capbrief hasRayTraceShaderExecutionReorder provided by \nvidia extension.
   *
   * \someNYI
   * \platformtable{hasNvidiaRayTraceShaderExecutionReorder,c,a,c,a,c,c,c,r,c,c,c}
   */
  bool hasNvidiaRayTraceShaderExecutionReorder : 1;
  /**
   * \capbrief Supports RB_NONE resource barrier
   *
   * \someNYI
   * \platformtable{hasBarrierNone,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasBarrierNone : 1;
  /**
   * \capbrief Supports query about graphics-pipeline activity
   *
   * \someNYI
   * \platformtable{hasPipelineStatisticsQuery,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasPipelineStatisticsQuery : 1;
  /* !!!!! TO ADD NEW VALUES, FOLOW THE STEPS DESCRIBED AT THE REMARK SECTION, KEEP THIS AT THE END OF THIS STRUCT !!!!! */
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for \xbone.
 */
struct DeviceDriverCapabilitiesXboxOne : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  //! \NYI
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  static constexpr bool hasConstBufferOffset = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasConditionalRender}
  static constexpr bool hasConditionalRender = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  static constexpr bool hasQuadTessellation = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBindless}
  static constexpr bool hasBindless = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  static constexpr bool hasAliasedTextures = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceHeaps}
  static constexpr bool hasResourceHeaps = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  static constexpr bool hasUAVOnlyForcedSampleCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRenderPassDepthResolve}
  static constexpr bool hasRenderPassDepthResolve = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShaderFloat16Support}
  static constexpr bool hasShaderFloat16Support = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayQuery}
  static constexpr bool hasRayQuery = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasGeometryIndexInRayAccelerationStructure}
  static constexpr bool hasGeometryIndexInRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasSkipPrimitiveTypeInRayTracingShaders}
  static constexpr bool hasSkipPrimitiveTypeInRayTracingShaders = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRayTracePipelineExpansion}
  static constexpr bool hasNativeRayTracePipelineExpansion = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = true;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for \scarlett
 * \details Uses \xbone optimized values and overrides only differences.
 * \see DeviceDriverCapabilitiesXboxOne
 */
struct DeviceDriverCapabilitiesScarlett : DeviceDriverCapabilitiesXboxOne
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  //! \warning Documentation states that HLSL 6.6 64 bit atomics are not supported directly, but HLSL xbox extensions that offer 64 bit
  //! atomics are. So they may need different instructions to support the same operations.
  static constexpr bool hasShader64BitIntegerResources = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasMeshShader}
  //! \warning Documentation is contradicting it self about proper support of indirect dispatch with pipelines that use amplification
  //! shaders.
  static constexpr bool hasMeshShader = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayQuery}
  //! \warning This is not directly supported, there is a software implementation available, see GDK documentation for details.
  static constexpr bool hasRayQuery = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGeometryIndexInRayAccelerationStructure}
  static constexpr bool hasGeometryIndexInRayAccelerationStructure = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasSkipPrimitiveTypeInRayTracingShaders}
  static constexpr bool hasSkipPrimitiveTypeInRayTracingShaders = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNativeRayTracePipelineExpansion}
  static constexpr bool hasNativeRayTracePipelineExpansion = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = true;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \ps4.
 */
struct DeviceDriverCapabilitiesPS4 : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasConditionalRender}
  static constexpr bool hasConditionalRender = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  static constexpr bool hasQuadTessellation = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasGather4}
  //! \todo Clarify why no support is advertised.
  static constexpr bool hasGather4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  //! \NYI
  static constexpr bool hasAlphaCoverage = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBindless}
  static constexpr bool hasBindless = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  static constexpr bool hasAliasedTextures = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  static constexpr bool hasUAVOnlyForcedSampleCount = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRenderPassDepthResolve}
  static constexpr bool hasRenderPassDepthResolve = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShaderFloat16Support}
  static constexpr bool hasShaderFloat16Support = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayQuery}
  static constexpr bool hasRayQuery = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasGeometryIndexInRayAccelerationStructure}
  static constexpr bool hasGeometryIndexInRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasSkipPrimitiveTypeInRayTracingShaders}
  static constexpr bool hasSkipPrimitiveTypeInRayTracingShaders = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRayTracePipelineExpansion}
  static constexpr bool hasNativeRayTracePipelineExpansion = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \ps5.
 * \details Uses \ps4 optimized values and overrides only differences.
 * \see DeviceDriverCapabilitiesPS4
 */
struct DeviceDriverCapabilitiesPS5 : DeviceDriverCapabilitiesPS4
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayQuery}
  static constexpr bool hasRayQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasGeometryIndexInRayAccelerationStructure}
  static constexpr bool hasGeometryIndexInRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasSkipPrimitiveTypeInRayTracingShaders}
  static constexpr bool hasSkipPrimitiveTypeInRayTracingShaders = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \ios.
 */
struct DeviceDriverCapabilitiesIOS : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConditionalRender}
  static constexpr bool hasConditionalRender = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  static constexpr bool hasQuadTessellation = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBindless}
  static constexpr bool hasBindless = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  //! \NYI
  static constexpr bool hasUAVOnlyForcedSampleCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasShaderFloat16Support}
  static constexpr bool hasShaderFloat16Support = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \tvos.
 */
struct DeviceDriverCapabilitiesTVOS : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConditionalRender}
  static constexpr bool hasConditionalRender = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  static constexpr bool hasQuadTessellation = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBindless}
  static constexpr bool hasBindless = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasResourceHeaps}
  //! \NYI
  static constexpr bool hasResourceHeaps = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  static constexpr bool hasUAVOnlyForcedSampleCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasShaderFloat16Support}
  static constexpr bool hasShaderFloat16Support = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \nswitch.
 */
struct DeviceDriverCapabilitiesNintendoSwitch : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  //! \NYI
  static constexpr bool hasQuadTessellation = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasResourceHeaps}
  //! \NYI
  static constexpr bool hasResourceHeaps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  static constexpr bool hasUAVOnlyForcedSampleCount = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayQuery}
  static constexpr bool hasRayQuery = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasEmulatedRaytracing}
  static constexpr bool hasEmulatedRaytracing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasGeometryIndexInRayAccelerationStructure}
  static constexpr bool hasGeometryIndexInRayAccelerationStructure = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasSkipPrimitiveTypeInRayTracingShaders}
  static constexpr bool hasSkipPrimitiveTypeInRayTracingShaders = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRayTracePipelineExpansion}
  static constexpr bool hasNativeRayTracePipelineExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \android.
 */
struct DeviceDriverCapabilitiesAndroid : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \mac.
 */
struct DeviceDriverCapabilitiesMacOSX : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDepthBoundsTest}
  static constexpr bool hasDepthBoundsTest = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasInstanceID}
  static constexpr bool hasInstanceID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConservativeRassterization}
  static constexpr bool hasConservativeRassterization = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasQuadTessellation}
  static constexpr bool hasQuadTessellation = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasWellSupportedIndirect}
  static constexpr bool hasWellSupportedIndirect = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  static constexpr bool hasVariableRateShadingBy4 = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConditionalRender}
  static constexpr bool hasConditionalRender = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled2DResources}
  static constexpr bool hasTiled2DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiled3DResources}
  static constexpr bool hasTiled3DResources = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledSafeResourcesAccess}
  static constexpr bool hasTiledSafeResourcesAccess = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTiledMemoryAliasing}
  static constexpr bool hasTiledMemoryAliasing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasTileBasedArchitecture}
  static constexpr bool hasTileBasedArchitecture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShaderFloat16Support}
  //! \NYI
  static constexpr bool hasShaderFloat16Support = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceOpacityMicroMapTriangleArrays }
  static constexpr bool hasRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceOpacityMicroMapTriangleArrays}
  static constexpr bool hasNvidiaRayTraceOpacityMicroMapTriangleArrays = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAtomicInt64OnGroupShared}
  static constexpr bool hasAtomicInt64OnGroupShared = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasRayTraceShaderExecutionReorder }
  static constexpr bool hasRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNvidiaRayTraceShaderExecutionReorder }
  static constexpr bool hasNvidiaRayTraceShaderExecutionReorder = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \linux.
 */
struct DeviceDriverCapabilitiesLinux : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDepthReadOnly}
  static constexpr bool hasDepthReadOnly = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasStructuredBuffers}
  static constexpr bool hasStructuredBuffers = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNoOverwriteOnShaderResourceBuffers}
  static constexpr bool hasNoOverwriteOnShaderResourceBuffers = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasVolMipMap}
  static constexpr bool hasVolMipMap = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasReadMultisampledDepth}
  static constexpr bool hasReadMultisampledDepth = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasGather4}
  static constexpr bool hasGather4 = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasShader64BitIntegerResources}
  static constexpr bool hasShader64BitIntegerResources = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasNativeRenderPassSubPasses}
  static constexpr bool hasNativeRenderPassSubPasses = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDLSS}
  static constexpr bool hasDLSS = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasXESS}
  static constexpr bool hasXESS = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasDrawID}
  static constexpr bool hasDrawID = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShaderIndirectCount}
  static constexpr bool hasMeshShaderIndirectCount = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStreamOutput}
  static constexpr bool hasStreamOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  //! \NYI
  static constexpr bool hasAliasedTextures = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasConstBufferOffset}
  //! \NYI
  static constexpr bool hasConstBufferOffset = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceCopyConversion}
  static constexpr bool hasResourceCopyConversion = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasProperUAVSupport}
  static constexpr bool hasProperUAVSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBarrierNone }
  static constexpr bool hasBarrierNone = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPipelineStatisticsQuery }
  static constexpr bool hasPipelineStatisticsQuery = false;
};
/**
 * \brief Optimized capabilities structure, hiding bitfield entries with static const values of known platform features for
 * \win32.
 */
struct DeviceDriverCapabilitiesWindows : DeviceDriverCapabilitiesBase
{
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAnisotropicFilter}
  static constexpr bool hasAnisotropicFilter = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAlphaCoverage}
  static constexpr bool hasAlphaCoverage = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAsyncCopy}
  static constexpr bool hasAsyncCopy = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasStereoExpansion}
  static constexpr bool hasStereoExpansion = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasLazyMemory}
  static constexpr bool hasLazyMemory = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectSupport}
  static constexpr bool hasIndirectSupport = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasCompareSampler}
  static constexpr bool hasCompareSampler = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasDualSourceBlending}
  static constexpr bool hasDualSourceBlending = true;
};
#if _TARGET_XBOXONE
using DeviceDriverCapabilities = DeviceDriverCapabilitiesXboxOne;
#elif _TARGET_SCARLETT
using DeviceDriverCapabilities = DeviceDriverCapabilitiesScarlett;
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS
using DeviceDriverCapabilities = DeviceDriverCapabilitiesIOS;
#elif _TARGET_TVOS
using DeviceDriverCapabilities = DeviceDriverCapabilitiesTVOS;
#elif _TARGET_C3

#elif _TARGET_ANDROID
using DeviceDriverCapabilities = DeviceDriverCapabilitiesAndroid;
#elif _TARGET_PC_MACOSX
using DeviceDriverCapabilities = DeviceDriverCapabilitiesMacOSX;
#elif _TARGET_PC_LINUX
using DeviceDriverCapabilities = DeviceDriverCapabilitiesLinux;
#elif _TARGET_PC_WIN
using DeviceDriverCapabilities = DeviceDriverCapabilitiesWindows;
#else
using DeviceDriverCapabilities = DeviceDriverCapabilitiesBase;
#endif

/**
 * A boolean bitfield that describes which known issues the used device / driver combination has.
 */
struct DeviceDriverIssuesBase
{
  /**
   * Some devices have issues with long running compute stages and may cause a TDR (Timeout Detection and Recovery) to reset the
   * device.
   * \note
   * Known on the following devices:
   * - \adreno
   * - \mali
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasComputeTimeLimited}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasComputeTimeLimited : 1;
  /**
   * Some device have issues with writing to 3D textures in the compute stage.
   * \note
   * Known on the following devices:
   * - \adreno
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasComputeCanNotWrite3DTex}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasComputeCanNotWrite3DTex : 1;
  /**
   * Some device drivers have issues with unused render pass attachments and my crash during pipeline creation.
   * \note
   * Known on the following devices:
   * - \adreno
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasStrictRenderPassesOnly}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasStrictRenderPassesOnly : 1;
  /**
   * Some device driver have a very limited size for sampled buffers. This indicates that device driver only works reliably with
   * buffers not exceeding the 64KiByte size limit.
   * \note
   * Some mobile device are actually limited and also fall in this category, even when this is expected behavior.
   * Known to be a bug on the following devices, as they report larger supported sizes, but fail to work with those limits:
   * - \adreno
   * - \mali
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasSmallSampledBuffers}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasSmallSampledBuffers : 1;
  /**
   * Some devices have issues in render passes properly scheduling clears before render, which may result in random flickering.
   * \note
   * Known on the following devices:
   * - \adreno with certain driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasRenderPassClearDataRace}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasRenderPassClearDataRace : 1;
  /**
   * Some device drivers have broken base instance ids in shaders.
   * \note
   * Known on the following devices:
   * - \mali with certain driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenBaseInstanceID}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenBaseInstanceID : 1;
  /**
   * Some device drivers may hang during instanced rendering with multisampled rastering.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasMultisampledAndInstancingHang}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasMultisampledAndInstancingHang : 1;
  /**
   * Some device drivers have device lost when trying to bind multisampled input attachment
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenMultisampledInputAttachment}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenMultisampledInputAttachment : 1;
  /**
   * Some device drivers spam device lost error codes while the device still remains functional normally.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasIgnoreDeviceLost}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasIgnoreDeviceLost : 1;
  /**
   * Some device drivers may randomly fail with a device reset on fence waits, the driver falls back to polling the state of fences
   * instead.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasPollDeviceFences}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasPollDeviceFences : 1;
  /**
   * Some device drivers may not render some long shaders after switching from the application and back.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenShadersAfterAppSwitch}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenShadersAfterAppSwitch : 1;
  /**
   * Some device drivers have issues with sRGB conversion for render targets in the MRT setup.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenSRGBConverionWithMRT}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenSRGBConverionWithMRT : 1;
  /**
   * Some device drivers have issues with format conversion while outputting to image with format other than fp32.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenComputeFormattedOutput}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenComputeFormattedOutput : 1;
  /**
   * AMD Radeon hangs with DEVICE_LOST on clearview of R8G8 target with a color other than the basic B/W combinations
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasClearColorBug}
   * - \runtimeissue{DeviceDriverIssuesWindows, \win32}
   */
  bool hasClearColorBug : 1;
  /**
   * Some device drivers have some issues with subpasses.
   * \note
   * Known on the following devices:
   * - \adreno with certain driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenSubpasses}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenSubpasses : 1;
  /**
   * 551.xx nvidia driver versions have an issue with multpiple CopySubresourceRegion to one target.
   * \note
   * Known on the following devices:
   * - \nvidia with 551.23 and newer driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasMultipleCopySubresourceBug}
   * - \runtimeissue{DeviceDriverIssuesWindows, \win32}
   */
  bool hasMultipleCopySubresourceBug : 1;
  /**
   * 572.xx nvidia driver versions have an issue with querying NvApi_GetSleepStatus after device creation.
   * \note
   * Known on the following devices:
   * - \nvidia with 572.xx driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenNvApiGetSleepStatus}
   * - \runtimeissue{DeviceDriverIssuesWindows, \win32}
   */
  bool hasBrokenNvApiGetSleepStatus : 1;
  /**
   * In some sequences of clear, RT switches, and rendering the texture may end up filled with NaNs
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasFloatClearBug}
   * - \runtimeissue{DeviceDriverIssuesWindows, \win32}
   */
  bool hasFloatClearBug : 1;
  /**
   * On some intel GPUs there is a problems with z testing if the texture with a depth format was written manually or updated from some
   * other texture.
   * \note
   * Known on the following devices:
   * - \intel with certain driver versions
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasDepthCopyResourceBug}
   * - \runtimeissue{DeviceDriverIssuesWindows, \win32}
   */
  bool hasDepthCopyResourceBug : 1;
  /**
   * Some device drivers cant process render passes with no targets and uav only write.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenUAVOnlyPasses}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenUAVOnlyPasses : 1;
};

/**
 * \brief Issues structure specific for \android.
 */
struct DeviceDriverIssuesAndroid : DeviceDriverIssuesBase
{
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasClearColorBug}
   **/
  static constexpr bool hasClearColorBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasMultipleCopySubresourceBug}
   **/
  static constexpr bool hasMultipleCopySubresourceBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasBrokenNvApiGetSleepStatus}
   **/
  static constexpr bool hasBrokenNvApiGetSleepStatus = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasDepthCopyResourceBug}
   **/
  static constexpr bool hasDepthCopyResourceBug = false;
};

/**
 * \brief Issues structure specific for \win32.
 */
struct DeviceDriverIssuesWindows : DeviceDriverIssuesBase
{
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasComputeTimeLimited}
   **/
  static constexpr bool hasComputeTimeLimited = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasComputeCanNotWrite3DTex}
   **/
  static constexpr bool hasComputeCanNotWrite3DTex = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasStrictRenderPassesOnly}
   **/
  static constexpr bool hasStrictRenderPassesOnly = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasSmallSampledBuffers}
   **/
  static constexpr bool hasSmallSampledBuffers = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasRenderPassClearDataRace}
   **/
  static constexpr bool hasRenderPassClearDataRace = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenBaseInstanceID}
   **/
  static constexpr bool hasBrokenBaseInstanceID = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasMultisampledAndInstancingHang}
   **/
  static constexpr bool hasMultisampledAndInstancingHang = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenMultisampledInputAttachment}
   **/
  static constexpr bool hasBrokenMultisampledInputAttachment = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasIgnoreDeviceLost}
   **/
  static constexpr bool hasIgnoreDeviceLost = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasPollDeviceFences}
   **/
  static constexpr bool hasPollDeviceFences = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenShadersAfterAppSwitch}
   **/
  static constexpr bool hasBrokenShadersAfterAppSwitch = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenSRGBConverionWithMRT}
   **/
  static constexpr bool hasBrokenSRGBConverionWithMRT = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenComputeFormattedOutput}
   **/
  static constexpr bool hasBrokenComputeFormattedOutput = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenSubpasses}
   **/
  static constexpr bool hasBrokenSubpasses = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasFloatClearBug}
   **/
  static constexpr bool hasFloatClearBug = false;
};

/**
 * \brief Issues structure specific for \ios.
 */
struct DeviceDriverIssuesIOS : DeviceDriverIssuesBase
{
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasComputeTimeLimited}
   **/
  static constexpr bool hasComputeTimeLimited = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasComputeCanNotWrite3DTex}
   **/
  static constexpr bool hasComputeCanNotWrite3DTex = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasStrictRenderPassesOnly}
   **/
  static constexpr bool hasStrictRenderPassesOnly = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasSmallSampledBuffers}
   **/
  static constexpr bool hasSmallSampledBuffers = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasRenderPassClearDataRace}
   **/
  static constexpr bool hasRenderPassClearDataRace = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenBaseInstanceID}
   **/
  static constexpr bool hasBrokenBaseInstanceID = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasMultisampledAndInstancingHang}
   **/
  static constexpr bool hasMultisampledAndInstancingHang = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenMultisampledInputAttachment}
   **/
  static constexpr bool hasBrokenMultisampledInputAttachment = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasIgnoreDeviceLost}
   **/
  static constexpr bool hasIgnoreDeviceLost = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasPollDeviceFences}
   **/
  static constexpr bool hasPollDeviceFences = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenShadersAfterAppSwitch}
   **/
  static constexpr bool hasBrokenShadersAfterAppSwitch = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenSRGBConverionWithMRT}
   **/
  static constexpr bool hasBrokenSRGBConverionWithMRT = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenComputeFormattedOutput}
   **/
  static constexpr bool hasBrokenComputeFormattedOutput = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenSubpasses}
   **/
  static constexpr bool hasBrokenSubpasses = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasFloatClearBug}
   **/
  static constexpr bool hasFloatClearBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasClearColorBug}
   **/
  static constexpr bool hasClearColorBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasMultipleCopySubresourceBug}
   **/
  static constexpr bool hasMultipleCopySubresourceBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasBrokenNvApiGetSleepStatus}
   **/
  static constexpr bool hasBrokenNvApiGetSleepStatus = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasDepthCopyResourceBug}
   **/
  static constexpr bool hasDepthCopyResourceBug = false;
};

/**
 * \brief Optimized issues structure for all platforms that are known to suffer from none of those.
 */
struct DeviceDriverIssuesNoIssues : DeviceDriverIssuesWindows
{
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasClearColorBug}
   **/
  static constexpr bool hasClearColorBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasMultipleCopySubresourceBug}
   **/
  static constexpr bool hasMultipleCopySubresourceBug = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasBrokenNvApiGetSleepStatus}
   **/
  static constexpr bool hasBrokenNvApiGetSleepStatus = false;
  /**
   * \brief Is constant false on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \android
   * \baseissue{DeviceDriverIssuesBase::hasDepthCopyResourceBug}
   **/
  static constexpr bool hasDepthCopyResourceBug = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenUAVOnlyPasses}
   **/
  static constexpr bool hasBrokenUAVOnlyPasses = false;
};

#if _TARGET_XBOXONE
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_SCARLETT
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS
using DeviceDriverIssues = DeviceDriverIssuesIOS;
#elif _TARGET_TVOS
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_C3

#elif _TARGET_ANDROID
using DeviceDriverIssues = DeviceDriverIssuesAndroid;
#elif _TARGET_PC_MACOSX
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_PC_LINUX
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_PC_WIN
using DeviceDriverIssues = DeviceDriverIssuesWindows;
#else
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#endif

#if _TARGET_XBOXONE
using DeviceDriverShaderModelVersion = decltype(d3d::sm60);
#elif _TARGET_SCARLETT
using DeviceDriverShaderModelVersion = decltype(d3d::sm66);
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS
using DeviceDriverShaderModelVersion = decltype(d3d::sm50);
#elif _TARGET_TVOS
using DeviceDriverShaderModelVersion = decltype(d3d::sm50);
#elif _TARGET_C3

#elif _TARGET_ANDROID
using DeviceDriverShaderModelVersion = d3d::shadermodel::Version;
#elif _TARGET_PC_MACOSX
using DeviceDriverShaderModelVersion = d3d::shadermodel::Version;
#elif _TARGET_PC_LINUX
using DeviceDriverShaderModelVersion = d3d::shadermodel::Version;
#elif _TARGET_PC_WIN
using DeviceDriverShaderModelVersion = d3d::shadermodel::Version;
#else
using DeviceDriverShaderModelVersion = d3d::shadermodel::Version;
#endif

/// Holds the device and driver specific ray tracing related properties.
/// NOTE some platforms may override these with fixed values.
struct DeviceDriverRaytraceProperties
{
  // required for calculating the size of index buffers for top level acceleration structures
  // only valid when caps.hasRaytracing is set
  unsigned topAccelerationStructureInstanceElementSize = 0;
  /// Offsets for scratch buffers for acceleration structures builds have to be aligned to this value.
  unsigned accelerationStructureBuildScratchBufferOffsetAlignment = 0;
  /// Describes how many "recursions" ray dispatching supports.
  /// This describes how many rays can be cast without returning.
  /// A value of 1 would mean that only the ray gen shader can shoot the initial ray and no other
  /// shader invoked by the rays miss or hit shader. A value of 2 means that the ray gen
  /// shader shoots the first ray and the invoked hit or miss shader may also shoot one ray.
  unsigned maxRecursionDepth = 0;
  /// Sizes of acceleration structure pools have to be aligned to this value or the create call will fail.
  /// May be 0 when pools are not supported.
  unsigned accelerationStructurePoolSizeAlignment = 0;
  /// Offsets of acceleration structures in pools have to be aligned to this value or create call will fail.
  unsigned accelerationStructurePoolOffsetAlignment = 0;
  /// May vary from 128 to 256, depending on implementation
  unsigned opacityMicroMapInputBufferAlignment = 0;
};

//--- 3d Driver description -------
struct DriverDesc // -V730
{
  int zcmpfunc, acmpfunc;
  int sblend, dblend;

  int mintexw, mintexh;
  int maxtexw, maxtexh;
  int mincubesize, maxcubesize;
  int minvolsize, maxvolsize;
  int maxtexaspect; ///< 1 means texture should be square, 0 means no limit
  int maxtexcoord;
  int maxsimtex;
  int maxvertexsamplers;
  int maxclipplanes;
  int maxstreams;
  int maxstreamstr;
  int maxvpconsts;
  int maxprims, maxvertind;
  /// pixel-size UV offsets that align texture pixels to frame buffer pixels;
  /// usually .5 for DX, and 0 for OGL
  float upixofs, vpixofs;
  /// Maximum number of simultaneous render targets, will be at least 1.
  int maxSimRT;
  bool is20ArbitrarySwizzleAvailable;
  int minWarpSize, maxWarpSize;
  // When DeviceDriverCapabilities::hasVariableRateShadingTexture is set, this tells how many shading pixels are represented by each
  // texel of the texture. Screen space to tile space: ceil(float(rtWith) / variableRateTextureTileSizeX), ceil(float(rtHeight) /
  // variableRateTextureTileSizeY). Possible values as of right now are 8, 16 and 32. On DX12 X and Y are equal on Vulkan those can
  // differ. On XBOX SCARLETT these are always 8.
  unsigned variableRateTextureTileSizeX;
  unsigned variableRateTextureTileSizeY;
  // When DeviceDriverCapabilities::hasRenderPassDepthResolve is set, this variable enumerates supported resolve modes
  unsigned depthResolveModes = DepthResolveMode::DEPTH_RESOLVE_MODE_NONE;
  DeviceAttributes info;
  DeviceDriverCapabilities caps;
  DeviceDriverIssues issues;
  DeviceDriverShaderModelVersion shaderModel;
  DeviceDriverRaytraceProperties raytrace;
};

#endif
