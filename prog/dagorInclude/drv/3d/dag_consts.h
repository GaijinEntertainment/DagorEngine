//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <EASTL/type_traits.h>

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

static constexpr int MAX_STREAM_OUTPUT_SLOTS = 4;

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
#define GET_VSDREG(a)          ((a) & VSDR_MASK)
#define MAKE_VSDREG(a)         (a)
#define GET_VSDSTREAM(a)       ((a) & VSDS_MASK)
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
  // These bits are still free for use
  SBCF_UNUSED_BIT_1 = 0x0100,
  SBCF_UNUSED_BIT_2 = 0x0200,
  SBCF_UNUSED_BIT_3 = 0x0400,
  SBCF_UNUSED_BIT_4 = 0x0800,
  SBCF_UNUSED_BIT_5 = 0x2000,
  // overlaps with SBCF_BIND_MASK ->
  SBCF_UNUSED_BIT_6 = 0x100000,
  SBCF_UNUSED_BIT_7 = 0x200000,
  SBCF_UNUSED_BIT_8 = 0x400000,
  // <- overlaps with SBCF_BIND_MASK
  SBCF_UNUSED_BIT_9 = 0x40000000,
  SBCF_UNUSED_BIT_A = 0x80000000,
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

  /// Source buffers for OMM needs special alignment, without this flag, buffers may be misaligned and cause GPU hangs
  SBCF_OPACITY_MICRO_MAP_TRIANGLE_SOURCE_DATA = 0x0008,

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

  SBCF_USAGE_STREAM_OUTPUT_COUNTER = 0x10000000, // Stream output counter buffer
  SBCF_USAGE_STREAM_OUTPUT = 0x20000000,         // Stream output buffer

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

// The MacOS defines the INTEL and this fixes the compilation error.
#undef INTEL

/**
 * Dagor specific ID of the currently used GPU's vendor.
 */
enum class GpuVendor : uint8_t
{
  UNKNOWN,
  MESA,
  IMGTEC,
  AMD,
  NVIDIA,
  INTEL,
  APPLE,
  SHIM_DRIVER,
  ARM,
  QUALCOMM,
  SAMSUNG,
  HUAWEI,
  ATI = AMD,
};

/**
 * Count of the available vendors in GpuVendor enum.
 */
static constexpr uint32_t GPU_VENDOR_COUNT = eastl::to_underlying(GpuVendor::HUAWEI) + 1;


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
// Details can be found here:
// https://dagor.rtd.gaijin.lan/en/latest/api-references/dagor-render/index/resource_and_execution_barriers.html
enum ResourceBarrier : uint64_t
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

  // State for buffers that are used as stream output targets.
  RB_STREAM_OUTPUT = 1u << 31,

  // State for buffers that are used as stream output counters.
  RB_STREAM_OUTPUT_COUNTER = 1ull << 32,

  /// Input buffer for opacity micro maps have to be in this state to be ingested by opacity micro map build commands
  RB_RO_OPACITY_MICRO_MAP_BUILD_INPUT_BUFFER = 1ull << 33,
  /// Description buffer for opacity micro maps have to be in this state to be ingested by opacity micro map build commands
  RB_RO_OPACITY_MICRO_MAP_BUILD_DESCRIPTION_BUFFER = 1ull << 34,

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
  return static_cast<ResourceBarrier>(eastl::to_underlying(l) | eastl::to_underlying(r));
}

constexpr inline ResourceBarrier operator&(ResourceBarrier l, ResourceBarrier r)
{
  return static_cast<ResourceBarrier>(eastl::to_underlying(l) & eastl::to_underlying(r));
}

constexpr inline ResourceBarrier operator^(ResourceBarrier l, ResourceBarrier r)
{
  return static_cast<ResourceBarrier>(eastl::to_underlying(l) ^ eastl::to_underlying(r));
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
  /// \warning Initial content of frame buffer is undefined, make sure to handle this
  RP_TA_LOAD_NO_CARE = 1u << 2,
  /// \brief Loads contents of stencil from memory, indepentendly to main load action
  RP_TA_LOAD_STENCIL_READ = 1u << 10,
  /// \brief Load stencil clear value, indepentendly to main load action
  RP_TA_LOAD_STENCIL_CLEAR = 1u << 11,
  /// \brief Don't care about loading contents of stencil, indepentendly to main load action
  RP_TA_LOAD_STENCIL_NO_CARE = 1u << 12,
  /// \brief Bitmask of any load operation
  /// \note  load action is performed for each target when
  /// its being accessed for the first time in render pass
  RP_TA_LOAD_MASK = RP_TA_LOAD_NO_CARE | RP_TA_LOAD_CLEAR | RP_TA_LOAD_READ | RP_TA_LOAD_STENCIL_READ | RP_TA_LOAD_STENCIL_CLEAR |
                    RP_TA_LOAD_STENCIL_NO_CARE,

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
  /// \brief Target will be used as variable rate shading texture in subpass
  RP_TA_SUBPASS_VRS_READ = 1u << 13,
  /// \brief Bitmask of any subpass operation
  RP_TA_SUBPASS_MASK = RP_TA_SUBPASS_READ | RP_TA_SUBPASS_RESOLVE | RP_TA_SUBPASS_WRITE | RP_TA_SUBPASS_KEEP | RP_TA_SUBPASS_VRS_READ,

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
  RP_SLOT_DEPTH_STENCIL = -1,
  RP_SLOT_VRS_TEXTURE = -2
};

/** @}*/


namespace raytrace
{
/// Acceleration structure pool that can be used to allocate multiple TLAS and BLAS out of it.
/// For detailed information see the appropriate functions interacting with the type.
struct AccelerationStructurePoolType;
using AccelerationStructurePool = AccelerationStructurePoolType *;
inline constexpr AccelerationStructurePool InvalidAccelerationStructurePool = nullptr;
} // namespace raytrace

/// Has to be a static string, users expect that identical tag string have the same address!
using ResourceTagType = const char *;
