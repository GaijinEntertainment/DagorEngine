//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driverCode.h>
#include <drv/3d/dag_shaderModelVersion.h>

// declare specific types
typedef int VPROG;
static constexpr int BAD_VPROG = -1;

typedef int FSHADER;
static constexpr int BAD_FSHADER = -1;

typedef int VDECL;
static constexpr int BAD_VDECL = -1;

typedef int PROGRAM;
static constexpr int BAD_PROGRAM = -1;

typedef int PALID;
static constexpr int BAD_PALID = -1;

typedef int VPRTYPE;
typedef unsigned long FSHTYPE;
typedef unsigned long VSDTYPE;

typedef unsigned long long GPUFENCEHANDLE;
constexpr GPUFENCEHANDLE BAD_GPUFENCEHANDLE = 0;

static constexpr size_t RT_TRANSFORM_SIZE = sizeof(float) * 12;

enum ShaderStage
{
  STAGE_CS,
  STAGE_PS,
  STAGE_VS,
  // keep as is, don't bloat other backends when they can't support it
  STAGE_MAX,

  STAGE_CS_ASYNC_STATE = STAGE_MAX, // STAGE_CS_ASYNC_STATE is only supported in DX11, and STAGE_RAYTRACE is not supported there.

  STAGE_MAX_EXT
};

enum class GpuPipeline
{
  GRAPHICS,
  ASYNC_COMPUTE,
  TRANSFER
};

// general limitations
enum
{
  MAXSAMPLERS = 16,
  MAXSTREAMS = 16,
  MAXSAMPLERS_VS = 4,
  MAX_SLI_AFRS = 4
};

// vertex stream declarations
#define VSDOP_MASK   (7 << 29)
#define VSDOP_STREAM (1 << 29)
#define VSDOP_INPUT  (2 << 29)

#define VSDT_MASK              (31 << 16)
#define VSDR_MASK              31
#define VSDS_MASK              15
#define VSDS_PER_INSTANCE_DATA 16
#define GET_VSDREG(a)          ((a)&VSDR_MASK)
#define MAKE_VSDREG(a)         (a)
#define GET_VSDSTREAM(a)       ((a)&VSDS_MASK)
#define MAKE_VSDSTREAM(a)      (a)
#define VSD_SKIPFLG            (1 << 28)
#define GET_VSDSKIP(a)         (((a) >> 16) & 15)

// does nothing, but wastes space; why do you need it?
// #define VSD_NOP 0

// end of declaration
#define VSD_END 0xFFFFFFFF

enum
{
  VDECLUSAGE_POS = 0,
  VDECLUSAGE_BLENDW,     // 1
  VDECLUSAGE_BLENDIND,   // 2
  VDECLUSAGE_NORM,       // 3
  VDECLUSAGE_PSIZE,      // 4
  VDECLUSAGE_TC,         // 5
  VDECLUSAGE_TANG,       // 6
  VDECLUSAGE_BINORM,     // 7
  VDECLUSAGE_TESSFACTOR, // 8
  VDECLUSAGE_TPOS,       // 9
  VDECLUSAGE_COLOR,      // 10
  VDECLUSAGE_FOG,        // 11
  VDECLUSAGE_DEPTH,      // 12
  VDECLUSAGE_SAMPLE,     // 13
};


#define VSTREAMSRC_NORMAL_DATA   0
#define VSTREAMSRC_INDEXED_DATA  (1 << 30)
#define VSTREAMSRC_INSTANCE_DATA (2 << 30)


enum
{
  /// Buffer can be used as a shader binding table.
  /// Should not be used directly, use get_shader_binding_table_buffer_properties
  /// to calculate all properties, including create flags.
  SBCF_USAGE_SHADER_BINDING_TABLE = 0x0001,
  /// Buffer is used as scratch space for RT structure builds
  SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE = 0x0002,
  // Create dynamic buffer
  // means data is frequently changed in it and it should handle it faster, also means
  // - buffer is CPU visible
  // - must support lock with discard/nooverwrite
  // additional logic that is mostly supported, but better be avoided
  // (allows target specific optimizations when avoided)
  // - buffer data that is locked but not written to is persistent
  // - locks with nooverwrite flags can be overlapped in one frame
  SBCF_DYNAMIC = 0x0004,
  SBCF_ZEROMEM = 0x0010, // Make sure driver has cleared the buffer (PS4, PS5)
  SBCF_INDEX32 = 0x0020, // Use 32-bit indices

  // Uses fast discard codepath, allowing to more efficiently utilize mem management
  // there's few limitation - it can only be locked with discard flag, there has to be cpu write access flag
  // and it can't be unordered
  // the contents of such buffer is only valid till the end of the frame
  // the data has to be updated every frame with discard flag
  // some drivers may choose to ignore this flag altogether
  // this flag is meant to add a fast CPU->GPU path where possible thus those buffers are not GPU writable
  SBCF_FRAMEMEM = 0x0080, // Use fast discard codepath

  // was previously SBCF_PS4_ONION
  // - buffer is used for read backs from GPU and it should choose a memory layout for this kind of usage
  // On XBOX ONE/SCARLETT and PS4 this mapps to ONION buffer which results in better CPU access behavior then GARLIC
  SBCF_USAGE_READ_BACK = 0x0040, // Create CPU readable buffer in a shared memory (no sysmem copies)

  SBCF_ALIGN16 = 0x1000, // GPU buffer 16 byte alignment

  SBCF_CPU_ACCESS_MASK = 0xC000,  // CPU access flags (for staging buffer):
  SBCF_CPU_ACCESS_WRITE = 0x4000, // Host, shared or device memory
  SBCF_CPU_ACCESS_READ = 0x8000,  // Host or shared memory

  SBCF_BIND_MASK = 0x00FF0000,       // SHADERRES buffer
  SBCF_BIND_VERTEX = 0x00010000,     // VERTEX buffer
  SBCF_BIND_INDEX = 0x00020000,      // INDEX buffer
  SBCF_BIND_CONSTANT = 0x00040000,   // CONSTANT buffer
  SBCF_BIND_SHADER_RES = 0x00080000, // SHADERRES buffer
  SBCF_BIND_UNORDERED = 0x00800000,  // UNORDERED buffer

  SBCF_MISC_MASK = 0x7000000,         // Resource misc flags
  SBCF_MISC_DRAWINDIRECT = 0x1000000, // D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS
  SBCF_MISC_ALLOW_RAW = 0x2000000,    // D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
  SBCF_MISC_STRUCTURED = 0x4000000,   // D3D11_RESOURCE_MISC_BUFFER_STRUCTURED
#if _TARGET_XBOX
  SBCF_MISC_ESRAM_ONLY = 0x8000000,
#else
  SBCF_MISC_ESRAM_ONLY = 0,
#endif

  // Buffer flag sets.
  // Const buffers
  // Such buffers could be updated from time to time.
  SBCF_CB_PERSISTENT = SBCF_BIND_CONSTANT | SBCF_DYNAMIC,
  // Such buffers must be updated every frame. Because of that we don't care about its content on device reset.
  SBCF_CB_ONE_FRAME = SBCF_BIND_CONSTANT | SBCF_DYNAMIC | SBCF_FRAMEMEM,

  // UAV buffers
  // (RW)ByteAddressBuffer in shader.
  SBCF_UA_SR_BYTE_ADDRESS = SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES,
  // (RW)StructuredBuffer in shader.
  SBCF_UA_SR_STRUCTURED = SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES,
  // RWByteAddressBuffer in shader.
  SBCF_UA_BYTE_ADDRESS = SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW,
  // RWStructuredBuffer in shader.
  SBCF_UA_STRUCTURED = SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED,
  // The same as SBCF_UA_BYTE_ADDRESS but its content can be read on CPU
  SBCF_UA_BYTE_ADDRESS_READBACK = SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_USAGE_READ_BACK,
  // The same as SBCF_UA_STRUCTURED but its content can be read on CPU
  SBCF_UA_STRUCTURED_READBACK = SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED | SBCF_USAGE_READ_BACK,
  // Indirect buffer filled on GPU
  SBCF_UA_INDIRECT = SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_MISC_DRAWINDIRECT,

  // GPU RO buffers
  // Indirect buffer filled on CPU
  SBCF_INDIRECT = SBCF_MISC_DRAWINDIRECT,

  // Staging buffer
  // A buffer for data transfer on GPU
  SBCF_STAGING_BUFFER = SBCF_CPU_ACCESS_READ | SBCF_CPU_ACCESS_WRITE
};

//--- Render states ---------
enum
{
  WRAP_COORD0 = 1,
  WRAP_COORD1 = 2,
  WRAP_COORD2 = 4,
  WRAP_COORD3 = 8,
};

enum
{
  VCDEST_NONE = 0,
  VCDEST_AMB,
  VCDEST_DIFF,
  VCDEST_AMB_DIFF,
  VCDEST_EMIS,
  VCDEST_SPEC,
};

// Flags for writemask method
enum
{
  WRITEMASK_RED0 = (1U << 0),
  WRITEMASK_GREEN0 = (1U << 1),
  WRITEMASK_BLUE0 = (1U << 2),
  WRITEMASK_ALPHA0 = (1U << 3),
  WRITEMASK_RED1 = (WRITEMASK_RED0 << 4),
  WRITEMASK_RED2 = (WRITEMASK_RED1 << 4),
  WRITEMASK_RED3 = (WRITEMASK_RED2 << 4),
  WRITEMASK_RED4 = (WRITEMASK_RED3 << 4),
  WRITEMASK_RED5 = (WRITEMASK_RED4 << 4),
  WRITEMASK_RED6 = (WRITEMASK_RED5 << 4),
  WRITEMASK_RED7 = (WRITEMASK_RED6 << 4),
  WRITEMASK_GREEN1 = (WRITEMASK_GREEN0 << 4),
  WRITEMASK_GREEN2 = (WRITEMASK_GREEN1 << 4),
  WRITEMASK_GREEN3 = (WRITEMASK_GREEN2 << 4),
  WRITEMASK_GREEN4 = (WRITEMASK_GREEN3 << 4),
  WRITEMASK_GREEN5 = (WRITEMASK_GREEN4 << 4),
  WRITEMASK_GREEN6 = (WRITEMASK_GREEN5 << 4),
  WRITEMASK_GREEN7 = (WRITEMASK_GREEN6 << 4),
  WRITEMASK_BLUE1 = (WRITEMASK_BLUE0 << 4),
  WRITEMASK_BLUE2 = (WRITEMASK_BLUE1 << 4),
  WRITEMASK_BLUE3 = (WRITEMASK_BLUE2 << 4),
  WRITEMASK_BLUE4 = (WRITEMASK_BLUE3 << 4),
  WRITEMASK_BLUE5 = (WRITEMASK_BLUE4 << 4),
  WRITEMASK_BLUE6 = (WRITEMASK_BLUE5 << 4),
  WRITEMASK_BLUE7 = (WRITEMASK_BLUE6 << 4),
  WRITEMASK_ALPHA1 = (WRITEMASK_ALPHA0 << 4),
  WRITEMASK_ALPHA2 = (WRITEMASK_ALPHA1 << 4),
  WRITEMASK_ALPHA3 = (WRITEMASK_ALPHA2 << 4),
  WRITEMASK_ALPHA4 = (WRITEMASK_ALPHA3 << 4),
  WRITEMASK_ALPHA5 = (WRITEMASK_ALPHA4 << 4),
  WRITEMASK_ALPHA6 = (WRITEMASK_ALPHA5 << 4),
  WRITEMASK_ALPHA7 = (WRITEMASK_ALPHA6 << 4),
  WRITEMASK_RED = WRITEMASK_RED0 | WRITEMASK_RED1 | WRITEMASK_RED2 | WRITEMASK_RED3 | WRITEMASK_RED4 | WRITEMASK_RED5 |
                  WRITEMASK_RED6 | WRITEMASK_RED7,
  WRITEMASK_GREEN = WRITEMASK_GREEN0 | WRITEMASK_GREEN1 | WRITEMASK_GREEN2 | WRITEMASK_GREEN3 | WRITEMASK_GREEN4 | WRITEMASK_GREEN5 |
                    WRITEMASK_GREEN6 | WRITEMASK_GREEN7,
  WRITEMASK_BLUE = WRITEMASK_BLUE0 | WRITEMASK_BLUE1 | WRITEMASK_BLUE2 | WRITEMASK_BLUE3 | WRITEMASK_BLUE4 | WRITEMASK_BLUE5 |
                   WRITEMASK_BLUE6 | WRITEMASK_BLUE7,
  WRITEMASK_ALPHA = WRITEMASK_ALPHA0 | WRITEMASK_ALPHA1 | WRITEMASK_ALPHA2 | WRITEMASK_ALPHA3 | WRITEMASK_ALPHA4 | WRITEMASK_ALPHA5 |
                    WRITEMASK_ALPHA6 | WRITEMASK_ALPHA7,
  WRITEMASK_RGB = WRITEMASK_RED | WRITEMASK_GREEN | WRITEMASK_BLUE,
  WRITEMASK_ALL = 0xFFFFFFFFU,
  WRITEMASK_DEFAULT = WRITEMASK_ALL
};

enum class XessState
{
  UNSUPPORTED_DEVICE,
  UNSUPPORTED_DRIVER,
  INIT_ERROR_UNKNOWN,
  DISABLED,
  SUPPORTED,
  READY
};

enum class Fsr2State
{
  NOT_CHECKED,
  INIT_ERROR,
  SUPPORTED,
  READY
};

enum class MtlfxUpscaleState
{
  UNSUPPORTED,
  READY
};

enum class HdrOutputMode // corresponding values in hdr_ps_output.sh
{
  SDR_ONLY = 0,
  HDR10_AND_SDR = 1,
  HDR10_ONLY = 2,
  HDR_ONLY = 3
};

enum class CSPreloaded
{
  No,
  Yes
};

/**
 * This enum defines bits signalling supported depth resolve modes.
 * Depth resolve is a functionality supported by some GAPIs which allows to resolve MSAA depth into a single sampled one.
 *
 * In the context of Vulkan, we do additional checks to determine supported depth resolve modes. Therefore, fewer modes than the
 * driver returns can be reported.
 */
enum DepthResolveMode : unsigned
{
  /**
   * Depth resolve unsupported.
   */
  DEPTH_RESOLVE_MODE_NONE = 0,
  /**
   * Use a value from the 0th sample. If depth resolve is supported, this is generally supported too.
   */
  DEPTH_RESOLVE_MODE_SAMPLE_ZERO = 1 << 0,
  /**
   * Use the average value from all samples. Not supported on iOS, rarely supported on Android Vulkan.
   */
  DEPTH_RESOLVE_MODE_AVERAGE = 1 << 1,
  /**
   * Use the smallest value from all samples. Supported on iOS, rarely supported on Android Vulkan.
   */
  DEPTH_RESOLVE_MODE_MIN = 1 << 2,
  /**
   * Same as DEPTH_RESOLVE_MODE_MIN, but the largest value is used instead.
   */
  DEPTH_RESOLVE_MODE_MAX = 1 << 3
};


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
   * \platformtable{hasVariableRateShading,c,c,c,a,c,c,c,c,c,c,r}
   */
  bool hasVariableRateShading : 1;
  /**
   * \capbrief supports shading rate textures as a source of shading rate information.
   * \someNYI
   * \platformtable{hasVariableRateShadingTexture,c,c,c,a,c,c,c,c,c,c,r}
   */
  bool hasVariableRateShadingTexture : 1;
  /**
   * \capbrief supports shader generated shading rates.
   * \someNYI
   * \platformtable{hasVariableRateShadingShaderOutput,c,c,c,a,c,c,c,c,c,c,r}
   */
  bool hasVariableRateShadingShaderOutput : 1;
  /**
   * \capbrief supports combiners for variable rate shading to select the final shading rate.
   * \someNYI
   * \platformtable{hasVariableRateShadingCombiners,c,c,c,a,c,c,c,c,c,c,r}
   */
  bool hasVariableRateShadingCombiners : 1;
  /**
   * \capbrief supports variable rate shading blocks with sizes of 4 in X and Y direction.
   * \someNYI
   * \platformtable{hasVariableRateShadingBy4,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasVariableRateShadingBy4 : 1;
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
   * \platformtable{hasTiled2DResources,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasTiled2DResources : 1;
  /**
   * \capbrief supports tiled 3D textures;
   * \platformtable{hasTiled3DResources,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasTiled3DResources : 1;
  /**
   * \capbrief supports safe read and write access for not mapped tiles of tiled resources. Such reads return 0
   * and writes are ignored.
   * \platformtable{hasTiledSafeResourcesAccess,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasTiledSafeResourcesAccess : 1;
  /**
   * \capbrief supports memory aliasing of multiple tiles.
   * \platformtable{hasTiledMemoryAliasing,c,a,c,a,c,c,c,c,c,c,r}
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
   * lines. \platformtable{hasMeshShader,c,a,c,a,c,c,c,c,c,c,r}
   */
  bool hasMeshShader : 1;
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
   * \platformtable{"hasUAVOnEveryStage",c,a,c,a,c,c,c,c,c,c,r}
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
   * \platformtable{hasNativeRayTracePipelineExpansion,c,c,c,a,r,r,c,r,r,r,r}
   */
  bool hasPersistentShaderHandles : 1;
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
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasForcedSamplerCount}
  static constexpr bool hasForcedSamplerCount = true;
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
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasAliasedTextures}
  static constexpr bool hasAliasedTextures = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasResourceHeaps}
  static constexpr bool hasResourceHeaps = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBufferOverlapCopy}
  static constexpr bool hasBufferOverlapCopy = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBufferOverlapRegionsCopy}
  static constexpr bool hasBufferOverlapRegionsCopy = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnlyForcedSampleCount}
  static constexpr bool hasUAVOnlyForcedSampleCount = true;
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
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayAccelerationStructure}
  static constexpr bool hasRayAccelerationStructure = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayQuery}
  //! \warning This is not directly supported, there is a software implementation available, see GDK documentation for details.
  static constexpr bool hasRayQuery = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasRayDispatch}
  static constexpr bool hasRayDispatch = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasIndirectRayDispatch}
  static constexpr bool hasIndirectRayDispatch = true;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasWaveOps}
  static constexpr bool hasWaveOps = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
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
  static constexpr bool hasOcclusionQuery = false;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  //! \NYI
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  //! \NYI
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  //! \NYI
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  //! \NYI
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  //! \NYI
  static constexpr bool hasVariableRateShadingBy4 = false;
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
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasMeshShader}
  static constexpr bool hasMeshShader = false;
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
  static constexpr bool hasUAVOnEveryStage = true;
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
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasBasicViewInstancing}
  static constexpr bool hasBasicViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasOptimizedViewInstancing}
  static constexpr bool hasOptimizedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAcceleratedViewInstancing}
  static constexpr bool hasAcceleratedViewInstancing = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasAsyncCompute}
  static constexpr bool hasAsyncCompute = false;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasOcclusionQuery}
  static constexpr bool hasOcclusionQuery = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasNVApi}
  static constexpr bool hasNVApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasATIApi}
  static constexpr bool hasATIApi = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShading}
  //! \NYI
  static constexpr bool hasVariableRateShading = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingTexture}
  //! \NYI
  static constexpr bool hasVariableRateShadingTexture = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingShaderOutput}
  //! \NYI
  static constexpr bool hasVariableRateShadingShaderOutput = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingCombiners}
  //! \NYI
  static constexpr bool hasVariableRateShadingCombiners = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasVariableRateShadingBy4}
  //! \NYI
  static constexpr bool hasVariableRateShadingBy4 = false;
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
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasUAVOnEveryStage}
  static constexpr bool hasUAVOnEveryStage = true;
  //! \briefconstcap{true, DeviceDriverCapabilitiesBase::hasBaseVertexSupport}
  static constexpr bool hasBaseVertexSupport = true;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::castingFullyTypedFormatsSupported}
  static constexpr bool castingFullyTypedFormatsSupported = false;
  //! \briefconstcap{false, DeviceDriverCapabilitiesBase::hasPersistentShaderHandles}
  static constexpr bool hasPersistentShaderHandles = false;
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
   * Image creation during texture reload seems to crash on some devies. Reports exist for Mali G710 and G610.
   * \note
   * Known for certain device driver combinations.
   * \note
   * - \constissue{DeviceDriverIssuesNoIssues::hasBrokenMTRecreateImage}
   * - \runtimeissue{DeviceDriverIssuesAndroid, \android}
   */
  bool hasBrokenMTRecreateImage : 1;
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
   * \baseissue{DeviceDriverIssuesBase::hasBrokenMTRecreateImage}
   **/
  static constexpr bool hasBrokenMTRecreateImage = false;
  /**
   * \brief Is constant true on \xbone, \scarlett, \ps4, \ps5, \ios, \tvos, \nswitch, \mac, \linux and \win32
   * \baseissue{DeviceDriverIssuesBase::hasBrokenSubpasses}
   **/
  static constexpr bool hasBrokenSubpasses = false;
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
};

#if _TARGET_XBOXONE
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_SCARLETT
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS
using DeviceDriverIssues = DeviceDriverIssuesNoIssues;
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
};

//--- 3d Driver description -------
struct Driver3dDesc // -V730
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
  DeviceDriverCapabilities caps;
  DeviceDriverIssues issues;
  DeviceDriverShaderModelVersion shaderModel;
  DeviceDriverRaytraceProperties raytrace;
};

//--- include defines specific to target 3d -------
#include <drv/3d/dag_consts_base.h>
#if _TARGET_C1 | _TARGET_C2

#endif

enum
{
  DRV3D_FRAMERATE_LIMITED_BY_NOTHING = 0x0,
  DRV3D_FRAMERATE_LIMITED_BY_REPLAY_WAIT = 0x1,
  DRV3D_FRAMERATE_LIMITED_BY_REPLAY_UNDERFEED = 0x2,
  DRV3D_FRAMERATE_LIMITED_BY_GPU_UTILIZATION = 0x4,
};

/**
 * \brief Combiners for VRS for vertex stage and pixel shader state.
 * \details Should the device support DeviceDriverCapabilities::hasVariableRateShadingCombiners,
 * then each combiner can be used for any stage in any combination. When
 * DeviceDriverCapabilities::hasVariableRateShadingCombiners is not supported then the
 * following restrictions apply:
 * - vertex state can only be PASSTHROUGH
 * - if DeviceDriverCapabilities::hasVariableRateShadingShaderOutput is supported then vertex state
 *   can additionally be OVERRIDE
 * - pixel state can only be PASSTHROUGH
 * - if DeviceDriverCapabilities::hasVariableRateShadingTexture is supported then pixel state
 *   can additionally be OVERRIDE
 */
enum class VariableRateShadingCombiner
{
  /// selects rate value from previous stage
  VRS_PASSTHROUGH,
  /// selects rate from this stage
  VRS_OVERRIDE,
  /// selects min of previous and this stage
  VRS_MIN,
  /// selects max of previous and this stage
  VRS_MAX,
  /// adds values of previous and this stage and clamps it to max possible value
  VRS_SUM
};

// Resource barriers
// Usually RW targets are implicitly transitioned to and need no explicit barrier.
// Details can be found here https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers
enum ResourceBarrier : int
{
  // For resources with color formats the target is render target and for resources with depth
  // stencil format it is depth stencil target. If resource has a depth/stencil format and paired
  // with RB_RO_SRV then it indicates use as a constant depth stencil target.
  // See RB_RO_CONSTANT_DEPTH_STENCIL_TARGET and RB_RW_DEPTH_STENCIL_TARGET as convenient aliases.
  RB_RW_RENDER_TARGET = 1u << 0,
  // Must be paired with RB_STAGE_* of stages where its used.
  RB_RW_UAV = 1u << 1,
  RB_RW_COPY_DEST = 1u << 2,
  RB_RW_BLIT_DEST = 1u << 3,

  // Must be paired with RB_STAGE_* of stages where its used.
  RB_RO_SRV = 1u << 4,
  // Must be paired with RB_STAGE_* of stages where its used.
  RB_RO_CONSTANT_BUFFER = 1u << 5,
  RB_RO_VERTEX_BUFFER = 1u << 6,
  RB_RO_INDEX_BUFFER = 1u << 7,
  RB_RO_INDIRECT_BUFFER = 1u << 8,
  RB_RO_VARIABLE_RATE_SHADING_TEXTURE = 1u << 9,
  RB_RO_COPY_SOURCE = 1u << 10,
  RB_RO_BLIT_SOURCE = 1u << 11,
  // For buffers used to build/update TLAS/BLAS
  RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE = 1u << 12,

  RB_FLAG_RELEASE_PIPELINE_OWNERSHIP = 1u << 13,
  RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP = 1u << 14,

  RB_FLAG_SPLIT_BARRIER_BEGIN = 1u << 15,
  RB_FLAG_SPLIT_BARRIER_END = 1u << 16,

  // includes vs, gs, hs and ts stages, also includes mesh shaders
  RB_STAGE_VERTEX = 1u << 17,
  RB_STAGE_PIXEL = 1u << 18,
  RB_STAGE_COMPUTE = 1u << 19,
  // covers all raytrace stages
  RB_STAGE_RAYTRACE = 1u << 20,
  // Must be paired with RB_STAGE_* of stages that have to wait for previous UAV access.
  // Must be paired with RB_SOURCE_STAGE_* of stages that need to finish before next UAV access can
  // continue.
  RB_FLUSH_UAV = 1u << 21,
  // Allows the driver to use a non preserving barrier if it can. If supported they are usually
  // faster as regular barriers. If paired with RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP, then for the
  // resource there is no RB_FLAG_RELEASE_PIPELINE_OWNERSHIP required.
  RB_FLAG_DONT_PRESERVE_CONTENT = 1u << 22,

  // same as RB_STAGE_* but to indicate last stage that needs to finish execution before UAV can
  // continue with a RB_FLUSH_UAV barrier.
  RB_SOURCE_STAGE_VERTEX = 1u << 23,
  RB_SOURCE_STAGE_PIXEL = 1u << 24,
  RB_SOURCE_STAGE_COMPUTE = 1u << 25,
  RB_SOURCE_STAGE_RAYTRACE = 1u << 26,
  // When we have two resource A and B alias, then when A was used and B should be used, then a
  // alias pair need to be generated for A and B before B can be used.
  //
  // RB_ALIAS_FROM has to reference resource A and has to immediately be followed by RB_ALIAS_TO
  // referencing resource B. After that resource B has to be initialized, with with a copy or a
  // clear, discard clear can be used, should the initial content of B not matter.
  //
  // NOTE: This kind of barrier is only needed for textures are render / depth stencil targets,
  //       normal textures should work without it.
  // NOTE: This barrier is per resource, subresource offset and range should encapsulate this (eg
  //       both 0).
  // NOTE: Aliasing barriers can not be paired with other barriers.
  // NOTE: This barrier does also a implicit RB_RW_RENDER_TARGET or RB_RW_UAV depending on the
  //       resource usage flags and the gpu pipeline where this barrier is issued.
  RB_ALIAS_FROM = 1u << 27,
  // Second part of the aliasing barrier pair, this barrier should reference the next used aliased
  // resource.
  //
  // NOTE: Referenced resource has to be associated with the resource of the previously issued
  //       RB_ALIAS_FROM barrier, by aliasing the same memory region.
  // NOTE: RB_ALIAS_TO_AND_DISCARD can be used instead to immediately discard the resource.
  RB_ALIAS_TO = 1u << 28,
  // State for buffers that are used as shader binding table for ray dispatches (direct or indirect).
  RB_RO_SHADER_BINDING_TABLE = 1u << 29,
  // This is intended to indicate that all uses of a buffer that is used as scratch space for acceleration
  // builds should be finished, so that the buffer can be reused.
  // NOTE: This is only needed to reuse a already used region of the buffer, non overlapping regions
  //       of the buffer do not affect each other, so no flush is needed.
  RB_FLUSH_RAYTRACE_ACCELERATION_BUILD_SCRATCH_USE = 1u << 30,
  // Will do the same as RB_ALIAS_TO plus discarding the resource, this initialized the resource
  // into a usable state, the context (texels) will be in a undefined state and are expected to
  // be overwritten with subsequent operations.
  //
  // NOTE: Do not use this on resources that will be initialized by other means, like clear or being
  //       copied to.
  // NOTE: This barrier does also a implicit RB_RW_RENDER_TARGET or RB_RW_UAV depending on the
  //       resource usage flags and the gpu pipeline where this barrier is issued.
  RB_ALIAS_TO_AND_DISCARD = RB_ALIAS_TO | RB_FLAG_DONT_PRESERVE_CONTENT,
  // Flushes everything to allow aliasing of any aliasing resource combination.
  // May or may not has a performance penalty compared to targeted alias barriers, may be useful
  // when multiple resources need a aliasing barrier at once, or other scenarios where alias from /
  // to are difficult to know.
  RB_ALIAS_ALL = RB_ALIAS_FROM | RB_ALIAS_TO,

  RB_SOURCE_STAGE_ALL_GRAPHICS = RB_SOURCE_STAGE_VERTEX | RB_SOURCE_STAGE_PIXEL,
  RB_SOURCE_STAGE_ALL_SHADERS = RB_SOURCE_STAGE_VERTEX | RB_SOURCE_STAGE_PIXEL | RB_SOURCE_STAGE_COMPUTE | RB_SOURCE_STAGE_RAYTRACE,

  RB_RW_DEPTH_STENCIL_TARGET = RB_RW_RENDER_TARGET,
  // Alias, needs to be paired with a stage bit
  RB_RO_CONSTANT_DEPTH_STENCIL_TARGET = RB_RW_DEPTH_STENCIL_TARGET | RB_RO_SRV,
  // NOTE: RB_RO_INDIRECT_BUFFER is intentionally not part of this, as this is the case on XBOX (X
  // and SX).
  RB_RO_GENERIC_READ_BUFFER = RB_RO_SRV | RB_RO_CONSTANT_BUFFER | RB_RO_VERTEX_BUFFER | RB_RO_INDEX_BUFFER | RB_RO_COPY_SOURCE |
                              RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE,
  // This + RB_STAGE_ALL_SHADERS is the state of static textures after upload is completed.
  RB_RO_GENERIC_READ_TEXTURE = RB_RO_SRV | RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE | RB_RO_VARIABLE_RATE_SHADING_TEXTURE,
  // This + RB_RO_GENERIC_READ_TEXTURE is the state of static textures after upload is completed.
  RB_STAGE_ALL_SHADERS = RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_STAGE_RAYTRACE,
  RB_STAGE_ALL_GRAPHICS = RB_STAGE_VERTEX | RB_STAGE_PIXEL,
  RB_NONE = 0
};

constexpr inline ResourceBarrier operator|(ResourceBarrier l, ResourceBarrier r)
{
  return static_cast<ResourceBarrier>(static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

constexpr inline ResourceBarrier operator&(ResourceBarrier l, ResourceBarrier r)
{
  return static_cast<ResourceBarrier>(static_cast<unsigned>(l) & static_cast<unsigned>(r));
}

constexpr inline ResourceBarrier operator^(ResourceBarrier l, ResourceBarrier r)
{
  return static_cast<ResourceBarrier>(static_cast<unsigned>(l) ^ static_cast<unsigned>(r));
}

/** \defgroup RenderPassConsts
 * @{
 */

/// \brief Bitfield of actions that happen with target at given slot and subpass
enum RenderPassTargetAction : int
{
  /// \brief No action with target will happen
  /// \details Only \b dependencyBarrier of binding will be executed for this target
  /// \warning Content of target becomes invalid if no action is supplied overall
  RP_TA_NONE = 0,

  /// \brief Loads contents of target from memory to framebuffer
  /// \note Avoid load operations on TBDR hardware
  RP_TA_LOAD_READ = 1u << 0,
  /// \brief Loads clear value to framebuffer instead of doing any memory operation
  RP_TA_LOAD_CLEAR = 1u << 1,
  /// \brief Don't care about loading contents of target (aka discard)
  /// \warning Initial content of frame buffer is underfined, make sure to handle this
  RP_TA_LOAD_NO_CARE = 1u << 2,
  /// \brief Bitmask of any load operation
  /// \note  load action is performed for each target when
  /// its being accessed for the first time in render pass
  RP_TA_LOAD_MASK = RP_TA_LOAD_NO_CARE | RP_TA_LOAD_CLEAR | RP_TA_LOAD_READ,

  /// \brief Target contents will be readed by subpass
  /// \note This corresponds to SubpassInput with SubpassLoad inside shader
  /// \warning Generic implementation uses T register with \b subpassBindingOffset instead of SubpassInput,
  /// this must be handled properly in shader code
  RP_TA_SUBPASS_READ = 1u << 3,
  /// \brief Target will be used as MSAA resolve destination of MSAA target bound in same slot
  /// \note MSAA Depth resolve is optional feature if non generic implementation is used
  /// \warning Must supply MSAA target in same slot in another binding
  /// otherwise creation on render pass will fail
  RP_TA_SUBPASS_RESOLVE = 1u << 4,
  /// \brief Target contents will be written by subpass
  RP_TA_SUBPASS_WRITE = 1u << 5,
  /// \brief Target contents will be keeped intact if it was not used in subpass (otherwise UB)
  RP_TA_SUBPASS_KEEP = 1u << 6,
  /// \brief Bitmask of any subpass operation
  RP_TA_SUBPASS_MASK = RP_TA_SUBPASS_READ | RP_TA_SUBPASS_RESOLVE | RP_TA_SUBPASS_WRITE | RP_TA_SUBPASS_KEEP,

  /// \brief Contents of framebuffer will be written to target memory
  RP_TA_STORE_WRITE = 1u << 7,
  /// \brief Contents of framebuffer will not be stored
  RP_TA_STORE_NONE = 1u << 8,
  /// \brief Don't care about target memory contents
  /// \warning Target memory contents will be left in UB state
  RP_TA_STORE_NO_CARE = 1u << 9,
  /// \brief Bitmask of any load operation
  /// \note  store action is performed once for each target on whole pass completion
  RP_TA_STORE_MASK = RP_TA_STORE_NO_CARE | RP_TA_STORE_NONE | RP_TA_STORE_WRITE
};

constexpr inline RenderPassTargetAction operator|(RenderPassTargetAction l, RenderPassTargetAction r)
{
  return static_cast<RenderPassTargetAction>(static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

/// \brief extra indexes that encode special cases of render pass description
enum RenderPassExtraIndexes : int
{
  RP_INDEX_NORMAL = 0,
  /// \brief Pseudo subpass index, that happens at end of render pass
  /// \details Used to provide store actions as well as \b dependencyBarriers for generic implementation
  RP_SUBPASS_EXTERNAL_END = -1,
  /// \brief Slot for depth/stencil
  /// \details Using this slot will bind target as depth/stencil
  RP_SLOT_DEPTH_STENCIL = -1
};

/** @}*/
