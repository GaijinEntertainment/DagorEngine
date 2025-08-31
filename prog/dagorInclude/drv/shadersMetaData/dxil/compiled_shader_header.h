//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_bindump_ext.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/span.h>
#include <EASTL/functional.h>
#include <EASTL/optional.h>
#include <shaders/dag_shaderHash.h>

// some headers define DOMAIN as macro which collides with enum names
#if defined(DOMAIN)
#undef DOMAIN
#endif

namespace dxil
{

using HashValue = ShaderHashValue;

// DX12 Baseline limit
// 14 is the max until Tier 3 HW
constexpr uint32_t MAX_B_REGISTERS = 14;
constexpr uint32_t MAX_T_REGISTERS = 32;
constexpr uint32_t MAX_S_REGISTERS = MAX_T_REGISTERS;
constexpr uint32_t MAX_U_REGISTERS = 13;
constexpr uint32_t ROOT_CONSTANT_BUFFER_REGISTER_INDEX = 8;
constexpr uint32_t ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET = 1;
constexpr uint32_t SPECIAL_CONSTANTS_REGISTER_INDEX = 7;
constexpr uint32_t DRAW_ID_REGISTER_SPACE = 1;
constexpr uint32_t MAX_UNBOUNDED_REGISTER_SPACES = 8;

constexpr uint32_t REGULAR_RESOURCES_SPACE_INDEX = 0;

constexpr uint32_t BINDLESS_REGISTER_INDEX = 0;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_COUNT = 2;
constexpr uint32_t BINDLESS_SAMPLERS_SPACE_OFFSET = 1;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_COUNT = 30;
constexpr uint32_t BINDLESS_RESOURCES_SPACE_OFFSET = 1;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_BITS_SHIFT = 0;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_BITS_SHIFT = BINDLESS_SAMPLERS_SPACE_BITS_SHIFT + BINDLESS_SAMPLERS_SPACE_COUNT;

constexpr uint32_t BINDLESS_SAMPLERS_SPACE_BITS_MASK = ((1u << BINDLESS_SAMPLERS_SPACE_COUNT) - 1)
                                                       << BINDLESS_SAMPLERS_SPACE_BITS_SHIFT;

constexpr uint32_t BINDLESS_RESOURCES_SPACE_BITS_MASK = ((1u << BINDLESS_RESOURCES_SPACE_COUNT) - 1)
                                                        << BINDLESS_RESOURCES_SPACE_BITS_SHIFT;

/// This is a hard limit, total shader record size with shder header can not exceed this value.
constexpr uint32_t MAX_SHADER_BINDING_TABLE_STRIDE = 4096;
/// This is the header size for a shader, this counts against MAX_SHADER_BINDING_TABLE_STRIDE limit.
constexpr uint32_t SHADER_BINDING_TABLE_SHADER_HEADER_SIZE = 32;
/// This is the upper limit of constant data in a shader record, this also includes resource references.
constexpr uint32_t MAX_SHADER_RECORD_CONSTANT_SIZE_IN_BYTES =
  MAX_SHADER_BINDING_TABLE_STRIDE - SHADER_BINDING_TABLE_SHADER_HEADER_SIZE;
/// Smallest unit of a constant in a shader record.
constexpr uint32_t MAX_SHADER_RECORD_CONSTANT_DWORD_COUNT = MAX_SHADER_RECORD_CONSTANT_SIZE_IN_BYTES / 4;
/// Absolute total limit of resources that can be referenced in a shader record, when space needed for constants 0.
constexpr uint32_t MAX_SHADER_RECORD_RESOURCE_COUNT = MAX_SHADER_RECORD_CONSTANT_SIZE_IN_BYTES / sizeof(uint64_t);

inline constexpr uint32_t MAX_SEMANTIC_NAME_SIZE = 32;

enum class ShaderStage : uint32_t
{
  VERTEX,
  PIXEL,
  GEOMETRY,
  DOMAIN,
  HULL,
  COMPUTE,
  // mesh pipeline
  MESH,
  AMPLIFICATION,
  // ray tracing
  RAY_GEN,
  INTERSECTION,
  ANY_HIT,
  CLOSEST_HIT,
  MISS,
  // technically generic but only supported on ray tracing for now
  CALLABLE,
  // work graphs (not used yet)
  NODE,
  // meta type for library may be used in some special cases
  LIBRARY,
  INVALID_STAGE = 0xFF,
};

enum SpecialConstantType
{
  SC_DRAW_ID = 1u << 0,
  /// shader uses NV extension hacks
  /// Those are enabled by a special UAV resource
  /// We encode this with this bit and the slot and space index is hardcoded, see extension::nvidia namespace
  SC_NVIDIA_EXTENSION = 1u << 1,
  /// Same as SC_NVIDIA_EXTENSION
  /// It is mutual exclusive to SC_NVIDIA_EXTENSION
  /// Slot and space encoding is located in extension::amd
  SC_AMD_EXTENSION = 1u << 2,
};

namespace extension
{
namespace nvidia
{
const uint32_t register_space_index = 99;
const uint32_t register_index = 0;
} // namespace nvidia
namespace amd
{
// default used by AMD, could be overridden, but why bother...
const uint32_t register_space_index = 0x7FFF0ADE;
const uint32_t register_index = 0;
} // namespace amd
} // namespace extension

struct ShaderResourceUsageTable
{
  uint32_t tRegisterUseMask = 0;
  uint32_t sRegisterUseMask = 0;
  uint32_t bindlessUsageMask = 0;
  uint16_t bRegisterUseMask = 0;
  uint16_t uRegisterUseMask = 0;
  uint8_t rootConstantDwords = 0;
  uint8_t specialConstantsMask = 0;
  uint16_t _resv = 0;
};

inline bool operator==(const ShaderResourceUsageTable &l, const ShaderResourceUsageTable &r)
{
  return l.tRegisterUseMask == r.tRegisterUseMask && l.sRegisterUseMask == r.sRegisterUseMask &&
         l.bindlessUsageMask == r.bindlessUsageMask && l.bRegisterUseMask == r.bRegisterUseMask &&
         l.rootConstantDwords == r.rootConstantDwords && l.uRegisterUseMask == r.uRegisterUseMask &&
         l.specialConstantsMask == r.specialConstantsMask;
}

inline bool operator!=(const ShaderResourceUsageTable &l, const ShaderResourceUsageTable &r) { return !(l == r); }

inline bool any_registers_used(const ShaderResourceUsageTable &srut)
{
  return 0 != (srut.bRegisterUseMask | srut.sRegisterUseMask | srut.tRegisterUseMask | srut.uRegisterUseMask | srut.bindlessUsageMask |
                srut.rootConstantDwords | srut.specialConstantsMask);
}

/// This hold the required shader model version and a set of required features.
/// The driver should avoid loading shaders with requirements the device can
/// not provide, otherwise the pipeline creation may fail.
struct ShaderDeviceRequirement
{
  /// Low part returned by ID3D12ShaderReflection::GetRequiresFlags
  uint32_t shaderFeatureFlagsLow;
  /// High part returned by ID3D12ShaderReflection::GetRequiresFlags
  uint32_t shaderFeatureFlagsHigh;
  /// Lower 8 bits of D3D12_SHADER_DESC::Version
  uint8_t shaderModel;
  /// see ExtensionVendor
  uint8_t extensionVendor;
  /// see VendorExtensionBits
  uint16_t vendorExtensionMask;
};

enum ExtensionVendor : uint8_t
{
  NoExtensionsUsed,
  NVIDIA,
  AMD,
};

enum VendorExtensionBits : uint16_t
{
  // NVIDIA_* only valid when ShaderDeviceRequirement::extensionVendor equals ExtensionVendor::NVidia
  NVIDIA_DXR_SHADER_EXECUTION_REORDER = 1u << 0,
  NVIDIA_DXR_MICRO_MAP = 1u << 1,
  NVIDIA_WAVE_MULTI_PREFIX = 1u << 2,
  NVIDIA_TEXTURE_FOOTPRINT = 1u << 3,
  NVIDIA_VARIABLE_RATE_SHADING = 1u << 4,
  NVIDIA_FP16_ATOMICS = 1u << 5,
  NVIDIA_FP32_ATOMICS = 1u << 6,
  NVIDIA_U64_ATOMICS = 1u << 7,
  NVIDIA_SPECIAL_REGISTER = 1u << 8,
  NVIDIA_WARP_VOTE = 1u << 9,
  NVIDIA_WARP_SHUFFLE = 1u << 10,
  NVIDIA_LANE_ID = 1u << 11,
  NVIDIA_WAVE_MATCH = 1u << 12,
  NVIDIA_DXR_CLUSTER_GEOMETRY = 1u << 13,
  NVIDIA_DXR_LINEAR_SWEPT_SPHERE = 1u << 14,
  // AMD_* only valid when ShaderDeviceRequirement::extensionVendor equals ExtensionVendor::AMD
  // ReadFirstLane, ReadLane, LaneID, Swizzle, Ballot, MBCount, Med3, Barycentrics
  AMD_INTRISICS_16 = 1u << 0,
  // WaveReduce, WaveScan
  AMD_INTRISICS_17 = 1u << 1,
  // DrawIndex, AtomicU64
  AMD_INTRISICS_19 = 1u << 2,
  AMD_GET_BASE_VERTEX = 1u << 3,
  AMD_GET_BASE_INSTANCE = 1u << 4,
  AMD_FLOAT_CONVERSION = 1u << 5,
  AMD_READ_LINE_AT = 1u << 6,
  AMD_DXR_RAY_HIT_TOKEN = 1u << 7,
  AMD_SHADER_CLOCK = 1u << 8,
  AMD_GET_WAVE_SIZE = 1u << 9,
};

inline bool is_compatible(const ShaderDeviceRequirement &base, const ShaderDeviceRequirement &compare)
{
  // version is packed lower 4 bytes are minor and upper 4 bytes is major, so simple compare should work correctly.
  return base.shaderModel >= compare.shaderModel &&
         // when any feature bit is set by compare that base does not has, its not compatible.
         0 == ((~base.shaderFeatureFlagsLow) & compare.shaderFeatureFlagsLow) &&
         0 == ((~base.shaderFeatureFlagsHigh) & compare.shaderFeatureFlagsHigh) &&
         // either when no extension vendor is needed than we don't care about vendor extensions at all
         // or we have to check the extension mask the same like the feature mask
         ((ExtensionVendor::NoExtensionsUsed == compare.extensionVendor) ||
           ((base.extensionVendor == compare.extensionVendor) && (0 == ((~base.vendorExtensionMask) & compare.vendorExtensionMask))));
}

// Allows for masking off feature flags that should not be taken into account
inline bool is_compatible(const ShaderDeviceRequirement &base, const ShaderDeviceRequirement &compare, uint64_t feature_mask)
{
  const uint32_t feature_mask_high = static_cast<uint32_t>(feature_mask >> 32);
  const uint32_t feature_mask_low = static_cast<uint32_t>(feature_mask);
  // version is packed lower 4 bytes are minor and upper 4 bytes is major, so simple compare should work correctly.
  return base.shaderModel >= compare.shaderModel &&
         // when any feature bit is set by compare that base does not has, its not compatible.
         0 == (feature_mask_low & ((~base.shaderFeatureFlagsLow) & compare.shaderFeatureFlagsLow)) &&
         0 == (feature_mask_high & ((~base.shaderFeatureFlagsHigh) & compare.shaderFeatureFlagsHigh)) &&
         ((ExtensionVendor::NoExtensionsUsed == base.extensionVendor) ||
           ((base.extensionVendor == compare.extensionVendor) && (0 == ((~base.vendorExtensionMask) & compare.vendorExtensionMask))));
}

struct ShaderHeader
{
  uint32_t maxConstantCount;
  uint32_t bonesConstantsUsed;
  ShaderResourceUsageTable resourceUsageTable;

  uint32_t sRegisterCompareUseMask;
  // for VS each bit indicates use of semantic name lookup table
  // for PS it is a RGBA mask for each of the 8 render targets
  uint32_t inOutSemanticMask;
  // Needed for null fallback, need to know which kind to use
  uint8_t tRegisterTypes[MAX_T_REGISTERS];
  uint8_t uRegisterTypes[MAX_U_REGISTERS];

  uint8_t shaderType;
  // input primitive for tesselation stages
  uint16_t inputPrimitive;

  ShaderDeviceRequirement deviceRequirement;

  friend bool operator==(const ShaderHeader &l, const ShaderHeader &r) { return 0 == (memcmp(&l, &r, sizeof(ShaderHeader))); }

  friend bool operator!=(const ShaderHeader &l, const ShaderHeader &r) { return !(l == r); }
};

struct ComputeShaderInfo
{
  uint32_t threadGroupSizeX;
  uint32_t threadGroupSizeY;
  uint32_t threadGroupSizeZ;
};

struct SemanticTableEntry
{
  uint32_t offset;
  uint32_t size;
};

struct StreamOutputComponentInfo
{
  char semanticName[MAX_SEMANTIC_NAME_SIZE];
  uint8_t semanticIndex;
  uint8_t mask : 4;
  uint8_t slot : 4; // it requires 2 bits, but we use 4 to avoid padding with possible random bits
};

struct ShaderHeaderCompileResult
{
  ShaderHeader header = {};
  ComputeShaderInfo computeShaderInfo = {};
  dag::Vector<StreamOutputComponentInfo> streamOutputComponents;
  bool isOk = true;
  eastl::string logMessage;
};

struct SemanticInfo
{
  const char *name;
  uint32_t index;
};

const SemanticInfo semantic_remap[25] = //
  {{"POSITION", 0}, {"BLENDWEIGHT", 0}, {"BLENDINDICES", 0}, {"NORMAL", 0}, {"PSIZE", 0}, {"COLOR", 0}, {"COLOR", 1},
    {"TEXCOORD", 0}, // 7
    {"TEXCOORD", 1}, {"TEXCOORD", 2}, {"TEXCOORD", 3}, {"TEXCOORD", 4}, {"TEXCOORD", 5}, {"TEXCOORD", 6}, {"TEXCOORD", 7},
    {"POSITION", 1}, {"NORMAL", 1}, {"TEXCOORD", 15}, {"TEXCOORD", 8}, {"TEXCOORD", 9}, {"TEXCOORD", 10}, {"TEXCOORD", 11},
    {"TEXCOORD", 12}, {"TEXCOORD", 13}, {"TEXCOORD", 14}};

inline const SemanticInfo *getSemanticInfoFromIndex(uint32_t index)
{
  auto at = eastl::begin(semantic_remap) + index;
  if (at >= eastl::end(semantic_remap))
    return nullptr;
  return at;
}
// NOTE: for something like TEXCOORD1 the input has to be "TEXCOOORD" for name and 1 for index
inline uint32_t getIndexFromSementicAndSemanticIndex(const char *name, uint32_t index)
{
  auto ref = eastl::find_if(eastl::begin(semantic_remap), eastl::end(semantic_remap),
    [=](const SemanticInfo &info) { return info.index == index && 0 == strcmp(name, info.name); });

  if (ref != eastl::end(semantic_remap))
  {
    return ref - eastl::begin(semantic_remap);
  }
  return ~uint32_t(0);
}

ShaderHeaderCompileResult compileHeaderFromReflectionData(ShaderStage stage, const eastl::vector<uint8_t> &reflection,
  uint32_t max_const_count, dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_components, void *dxc_lib);

// identifies simple shader blob with one shader
const uint32_t SHADER_IDENT = _MAKE4C('SX12');
const uint32_t SHADER_UNCOMPRESSED_IDENT = _MAKE4C('sx12');
// identifies a combine shader blob with a set of shader (all with different stages!)
const uint32_t COMBINED_SHADER_IDENT = _MAKE4C('SC12');
const uint32_t COMBINED_SHADER_UNCOMPRESSED_IDENT = _MAKE4C('sc12');

struct CombinedChunk
{
  uint32_t offset;
  uint32_t size;
};

enum class ChunkType : uint32_t
{
  // Section with one ShaderHeader
  SHADER_HEADER,
  // Array of bytes containing DXIL binary
  DXIL,
  // Array of bytes containing DXBC binary
  DXBC,
  // name of the shader for debugging
  // (primarily for compute, as there the system does not generate the name)
  SHADER_NAME,
  // Used internally for XBOX compilation to pass original source
  // from phase one to phase two
  SHADER_SOURCE,
};

struct ChunkHeader
{
  HashValue hash;
  ChunkType type;
  uint32_t offset;
  uint32_t size;
};

struct FileHeader
{
  uint32_t ident;
  uint32_t chunkCount;
  uint32_t chunkDataSize;
  uint32_t compressedSize;
};

// New format of shaders

BINDUMP_BEGIN_LAYOUT(Shader)
  BINDUMP_USING_EXTENSION()
  ShaderHeader shaderHeader;
  VecHolder<uint8_t> bytecode;
  Compressed<VecHolder<char>> shaderSource;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(VertexShaderPipeline)
  Layout<Shader> vertexShader;
  Ptr<Shader> hullShader;
  Ptr<Shader> domainShader;
  Ptr<Shader> geometryShader;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(MeshShaderPipeline)
  Layout<Shader> meshShader;
  Ptr<Shader> amplificationShader;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(ShaderWithStreamOutput)
  BINDUMP_USING_EXTENSION()
  VecHolder<StreamOutputComponentInfo> streamOutputComponents;
  VecHolder<uint8_t> data;
BINDUMP_END_LAYOUT()

enum class StoredShaderType : uint16_t
{
  singleShader,
  combinedVertexShader,
  meshShader,
};

struct ShaderContainerType // -V730 All members are initialized
{
  StoredShaderType shaderType = StoredShaderType::singleShader;
  uint16_t hasStreamOutput : 1 = 0;
  uint16_t pad : 15 = 0;
};

BINDUMP_BEGIN_LAYOUT(ShaderContainer)
  BINDUMP_USING_EXTENSION()
  ShaderContainerType type;
  HashValue dataHash;
  VecHolder<uint8_t> data;
BINDUMP_END_LAYOUT()

/// Properties of a individual shader in a shader library.
struct LibraryShaderProperties
{
  /// limit is MAX_SHADER_RECORD_CONSTANT_DWORD_COUNT, this is a **hard** limit by DX12
  uint32_t shaderRecordSizeInDwords;
  /// The max recursion count the shader may trigger.
  uint8_t maxRecusionDepth;
  /// Payload size of ray cast data. This can be anything and depends on the shaders in and outputs.
  uint8_t maxPayloadSizeInBytes;
  /// Attributes for geometry intersection, usually 2 x size of float, barycentric coordinates of the hit. For custom intersection
  /// shaders, this can be anything. This may be 0 for shader types that do not interact with geometry intersection.
  uint8_t maxAttributeSizeInBytes;
  /// Type of the shader, see ShaderStage
  uint8_t shaderType;
  /// Resource usage mask for this shader.
  ShaderResourceUsageTable resourceUsageTable;
  /// Requirements the device has to fulfill to be used.
  ShaderDeviceRequirement deviceRequirement;
};

/// Composition of all resource usages of the shader library.
/// As within a library, conflicts of resource definitions is not possible, we can take advantage of that and store the global resource
/// definitions for the whole library and individual shaders can pick what they need depending on their usage masks.
struct LibraryResourceInformation
{
  /// Mask that tells which sampler in a s register is a comparison sampler or a normal sampler.
  /// As they can not have conflicting entries for each slot, we can use one table for all shaders.
  uint32_t sRegisterCompareUseMask;
  /// Type table for T registers for the whole library.
  /// As they can not have conflicting entries for each slot, we can use one table for all shaders.
  uint8_t tRegisterTypes[MAX_T_REGISTERS];
  /// Type table for U registers for the whole library.
  /// As they can not have conflicting entries for each slot, we can use one table for all shaders.
  uint8_t uRegisterTypes[MAX_U_REGISTERS];
  /// Alignment padding
  uint8_t pddding[8 - ((sizeof(uint32_t) + MAX_T_REGISTERS + MAX_U_REGISTERS) % 8)];
  /// Resource usage table of the whole library.
  /// As they can not have conflicting entries for each slot, we can use one table for all shaders.
  ShaderResourceUsageTable resourceUsageTable;
};

BINDUMP_BEGIN_LAYOUT(ShaderLibraryContainer)
  BINDUMP_USING_EXTENSION()
  /// Properties of each shader in the library
  VecHolder<LibraryShaderProperties> shaderProperties;
  /// Actual DXIL binary with the shader code and meta data for DX12
  VecHolder<uint8_t> dxilBinary;
  /// Resource usage information for the library as a whole
  LibraryResourceInformation resourceUsageInfo;
  /// Hash value of the data of driverBinary
  ShaderHashValue binaryHash;
BINDUMP_END_LAYOUT()

struct LibraryShaderPropertiesCompileResult
{
  eastl::vector<LibraryShaderProperties> properties;
  eastl::vector<eastl::string> names;
  LibraryResourceInformation libInfo{};
  bool isOk = true;
  eastl::string logMessage;
};

struct FunctionExtraInfo
{
  uint32_t recursionDepth = 0;
  uint32_t maxPayLoadSizeInBytes = 0;
  uint32_t maxAttributeSizeInBytes = 0;
};

using FunctionExtraDataQuery = eastl::function<eastl::optional<FunctionExtraInfo>(ShaderStage, const eastl::string &)>;

LibraryShaderPropertiesCompileResult compileLibraryShaderPropertiesFromReflectionData(uint32_t default_playload_size_in_bytes,
  const FunctionExtraDataQuery &function_extra_data_query, eastl::span<const uint8_t> reflection, void *dxc_lib_handle);

struct NvidiaExtensionOpCodeToExtensionBitEntry
{
  // hlsl extension of codes
  uint16_t opCode = 0;
  // dxil::VendorExtensionBits::NVIDIA_* values
  uint16_t extensionBit = 0;
};
inline eastl::span<const NvidiaExtensionOpCodeToExtensionBitEntry> get_nvidia_extension_op_code_to_extension_bit_table()
{
#if !NV_SHADER_EXTN_VERSION
#define NV_SHADER_EXTN_DEFINED_HERE 1
#define NV_EXTN_OP_SHFL             1
#define NV_EXTN_OP_SHFL_UP          2
#define NV_EXTN_OP_SHFL_DOWN        3
#define NV_EXTN_OP_SHFL_XOR         4

#define NV_EXTN_OP_VOTE_ALL    5
#define NV_EXTN_OP_VOTE_ANY    6
#define NV_EXTN_OP_VOTE_BALLOT 7

#define NV_EXTN_OP_GET_LANE_ID 8
#define NV_EXTN_OP_FP16_ATOMIC 12
#define NV_EXTN_OP_FP32_ATOMIC 13

#define NV_EXTN_OP_GET_SPECIAL 19

#define NV_EXTN_OP_UINT64_ATOMIC 20

#define NV_EXTN_OP_MATCH_ANY 21

// FOOTPRINT - For Sample and SampleBias
#define NV_EXTN_OP_FOOTPRINT      28
#define NV_EXTN_OP_FOOTPRINT_BIAS 29

#define NV_EXTN_OP_GET_SHADING_RATE 30

// FOOTPRINT - For SampleLevel and SampleGrad
#define NV_EXTN_OP_FOOTPRINT_LEVEL 31
#define NV_EXTN_OP_FOOTPRINT_GRAD  32

// SHFL Generic
#define NV_EXTN_OP_SHFL_GENERIC 33

#define NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE 51
#define NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED   52

// HitObject API
#define NV_EXTN_OP_HIT_OBJECT_TRACE_RAY                      67
#define NV_EXTN_OP_HIT_OBJECT_MAKE_HIT                       68
#define NV_EXTN_OP_HIT_OBJECT_MAKE_HIT_WITH_RECORD_INDEX     69
#define NV_EXTN_OP_HIT_OBJECT_MAKE_MISS                      70
#define NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD                 71
#define NV_EXTN_OP_HIT_OBJECT_INVOKE                         72
#define NV_EXTN_OP_HIT_OBJECT_IS_MISS                        73
#define NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_ID                74
#define NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_INDEX             75
#define NV_EXTN_OP_HIT_OBJECT_GET_PRIMITIVE_INDEX            76
#define NV_EXTN_OP_HIT_OBJECT_GET_GEOMETRY_INDEX             77
#define NV_EXTN_OP_HIT_OBJECT_GET_HIT_KIND                   78
#define NV_EXTN_OP_HIT_OBJECT_GET_RAY_DESC                   79
#define NV_EXTN_OP_HIT_OBJECT_GET_ATTRIBUTES                 80
#define NV_EXTN_OP_HIT_OBJECT_GET_SHADER_TABLE_INDEX         81
#define NV_EXTN_OP_HIT_OBJECT_LOAD_LOCAL_ROOT_TABLE_CONSTANT 82
#define NV_EXTN_OP_HIT_OBJECT_IS_HIT                         83
#define NV_EXTN_OP_HIT_OBJECT_IS_NOP                         84
#define NV_EXTN_OP_HIT_OBJECT_MAKE_NOP                       85

// Micro-map API
#define NV_EXTN_OP_RT_TRIANGLE_OBJECT_POSITIONS       86
#define NV_EXTN_OP_RT_MICRO_TRIANGLE_OBJECT_POSITIONS 87
#define NV_EXTN_OP_RT_MICRO_TRIANGLE_BARYCENTRICS     88
#define NV_EXTN_OP_RT_IS_MICRO_TRIANGLE_HIT           89
#define NV_EXTN_OP_RT_IS_BACK_FACING                  90
#define NV_EXTN_OP_RT_MICRO_VERTEX_OBJECT_POSITION    91
#define NV_EXTN_OP_RT_MICRO_VERTEX_BARYCENTRICS       92

// Megageometry API
#define NV_EXTN_OP_RT_GET_CLUSTER_ID                        93
#define NV_EXTN_OP_RT_GET_CANDIDATE_CLUSTER_ID              94
#define NV_EXTN_OP_RT_GET_COMMITTED_CLUSTER_ID              95
#define NV_EXTN_OP_HIT_OBJECT_GET_CLUSTER_ID                96
#define NV_EXTN_OP_RT_CANDIDATE_TRIANGLE_OBJECT_POSITIONS   97
#define NV_EXTN_OP_RT_COMMITTED_TRIANGLE_OBJECT_POSITIONS   98
#define NV_EXTN_OP_HIT_OBJECT_GET_TRIANGLE_OBJECT_POSITIONS 99

// Linear Swept Sphere API
#define NV_EXTN_OP_RT_SPHERE_OBJECT_POSITION_AND_RADIUS             100
#define NV_EXTN_OP_RT_CANDIDATE_SPHERE_OBJECT_POSITION_AND_RADIUS   101
#define NV_EXTN_OP_RT_COMMITTED_SPHERE_OBJECT_POSITION_AND_RADIUS   102
#define NV_EXTN_OP_HIT_OBJECT_GET_SPHERE_OBJECT_POSITION_AND_RADIUS 103
#define NV_EXTN_OP_RT_LSS_OBJECT_POSITIONS_AND_RADII                104
#define NV_EXTN_OP_RT_CANDIDATE_LSS_OBJECT_POSITIONS_AND_RADII      105
#define NV_EXTN_OP_RT_COMMITTED_LSS_OBJECT_POSITIONS_AND_RADII      106
#define NV_EXTN_OP_HIT_OBJECT_GET_LSS_OBJECT_POSITIONS_AND_RADII    107
#define NV_EXTN_OP_RT_IS_SPHERE_HIT                                 108
#define NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_SPHERE                 109
#define NV_EXTN_OP_RT_COMMITTED_IS_SPHERE                           110
#define NV_EXTN_OP_HIT_OBJECT_IS_SPHERE_HIT                         111
#define NV_EXTN_OP_RT_IS_LSS_HIT                                    112
#define NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_LSS                    113
#define NV_EXTN_OP_RT_COMMITTED_IS_LSS                              114
#define NV_EXTN_OP_HIT_OBJECT_IS_LSS_HIT                            115
#define NV_EXTN_OP_RT_CANDIDATE_LSS_HIT_PARAMETER                   116
#define NV_EXTN_OP_RT_COMMITTED_LSS_HIT_PARAMETER                   117
#define NV_EXTN_OP_RT_CANDIDATE_BUILTIN_PRIMITIVE_RAY_T             118
#define NV_EXTN_OP_RT_COMMIT_NONOPAQUE_BUILTIN_PRIMITIVE_HIT        119
#endif
  static const dxil::NvidiaExtensionOpCodeToExtensionBitEntry table[] = {
    {NV_EXTN_OP_SHFL, ::dxil::VendorExtensionBits::NVIDIA_WARP_SHUFFLE},
    {NV_EXTN_OP_SHFL_UP, ::dxil::VendorExtensionBits::NVIDIA_WARP_SHUFFLE},
    {NV_EXTN_OP_SHFL_DOWN, ::dxil::VendorExtensionBits::NVIDIA_WARP_SHUFFLE},
    {NV_EXTN_OP_SHFL_XOR, ::dxil::VendorExtensionBits::NVIDIA_WARP_SHUFFLE},
    {NV_EXTN_OP_VOTE_ALL, ::dxil::VendorExtensionBits::NVIDIA_WARP_VOTE},
    {NV_EXTN_OP_VOTE_ANY, ::dxil::VendorExtensionBits::NVIDIA_WARP_VOTE},
    {NV_EXTN_OP_VOTE_BALLOT, ::dxil::VendorExtensionBits::NVIDIA_WARP_VOTE},
    {NV_EXTN_OP_GET_LANE_ID, ::dxil::VendorExtensionBits::NVIDIA_LANE_ID},
    {NV_EXTN_OP_FP16_ATOMIC, ::dxil::VendorExtensionBits::NVIDIA_FP16_ATOMICS},
    {NV_EXTN_OP_FP32_ATOMIC, ::dxil::VendorExtensionBits::NVIDIA_FP32_ATOMICS},
    {NV_EXTN_OP_GET_SPECIAL, ::dxil::VendorExtensionBits::NVIDIA_SPECIAL_REGISTER},
    {NV_EXTN_OP_UINT64_ATOMIC, ::dxil::VendorExtensionBits::NVIDIA_U64_ATOMICS},
    {NV_EXTN_OP_MATCH_ANY, ::dxil::VendorExtensionBits::NVIDIA_WAVE_MATCH},
    {NV_EXTN_OP_FOOTPRINT, ::dxil::VendorExtensionBits::NVIDIA_TEXTURE_FOOTPRINT},
    {NV_EXTN_OP_FOOTPRINT_BIAS, ::dxil::VendorExtensionBits::NVIDIA_TEXTURE_FOOTPRINT},
    {NV_EXTN_OP_GET_SHADING_RATE, ::dxil::VendorExtensionBits::NVIDIA_VARIABLE_RATE_SHADING},
    {NV_EXTN_OP_FOOTPRINT_LEVEL, ::dxil::VendorExtensionBits::NVIDIA_TEXTURE_FOOTPRINT},
    {NV_EXTN_OP_FOOTPRINT_GRAD, ::dxil::VendorExtensionBits::NVIDIA_TEXTURE_FOOTPRINT},
    {NV_EXTN_OP_SHFL_GENERIC, ::dxil::VendorExtensionBits::NVIDIA_WAVE_MULTI_PREFIX},
    {NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE, ::dxil::VendorExtensionBits::NVIDIA_VARIABLE_RATE_SHADING},
    {NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED, ::dxil::VendorExtensionBits::NVIDIA_VARIABLE_RATE_SHADING},
    {NV_EXTN_OP_HIT_OBJECT_TRACE_RAY, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_MAKE_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_MAKE_HIT_WITH_RECORD_INDEX, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_MAKE_MISS, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_INVOKE, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_IS_MISS, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_ID, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_INDEX, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_PRIMITIVE_INDEX, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_GEOMETRY_INDEX, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_HIT_KIND, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_RAY_DESC, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_ATTRIBUTES, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_GET_SHADER_TABLE_INDEX, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_LOAD_LOCAL_ROOT_TABLE_CONSTANT, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_IS_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_IS_NOP, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_HIT_OBJECT_MAKE_NOP, ::dxil::VendorExtensionBits::NVIDIA_DXR_SHADER_EXECUTION_REORDER},
    {NV_EXTN_OP_RT_TRIANGLE_OBJECT_POSITIONS, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_MICRO_TRIANGLE_OBJECT_POSITIONS, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_MICRO_TRIANGLE_BARYCENTRICS, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_IS_MICRO_TRIANGLE_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_IS_BACK_FACING, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_MICRO_VERTEX_OBJECT_POSITION, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_MICRO_VERTEX_BARYCENTRICS, ::dxil::VendorExtensionBits::NVIDIA_DXR_MICRO_MAP},
    {NV_EXTN_OP_RT_GET_CLUSTER_ID, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_RT_GET_CANDIDATE_CLUSTER_ID, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_RT_GET_COMMITTED_CLUSTER_ID, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_HIT_OBJECT_GET_CLUSTER_ID, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_RT_CANDIDATE_TRIANGLE_OBJECT_POSITIONS, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_RT_COMMITTED_TRIANGLE_OBJECT_POSITIONS, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_HIT_OBJECT_GET_TRIANGLE_OBJECT_POSITIONS, ::dxil::VendorExtensionBits::NVIDIA_DXR_CLUSTER_GEOMETRY},
    {NV_EXTN_OP_RT_SPHERE_OBJECT_POSITION_AND_RADIUS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_SPHERE_OBJECT_POSITION_AND_RADIUS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMITTED_SPHERE_OBJECT_POSITION_AND_RADIUS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_HIT_OBJECT_GET_SPHERE_OBJECT_POSITION_AND_RADIUS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_LSS_OBJECT_POSITIONS_AND_RADII, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_LSS_OBJECT_POSITIONS_AND_RADII, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMITTED_LSS_OBJECT_POSITIONS_AND_RADII, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_HIT_OBJECT_GET_LSS_OBJECT_POSITIONS_AND_RADII, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_IS_SPHERE_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_SPHERE, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMITTED_IS_SPHERE, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_HIT_OBJECT_IS_SPHERE_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_IS_LSS_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_LSS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMITTED_IS_LSS, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_HIT_OBJECT_IS_LSS_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_LSS_HIT_PARAMETER, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMITTED_LSS_HIT_PARAMETER, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_CANDIDATE_BUILTIN_PRIMITIVE_RAY_T, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
    {NV_EXTN_OP_RT_COMMIT_NONOPAQUE_BUILTIN_PRIMITIVE_HIT, ::dxil::VendorExtensionBits::NVIDIA_DXR_LINEAR_SWEPT_SPHERE},
  };
#if NV_SHADER_EXTN_DEFINED_HERE
#undef NV_SHADER_EXTN_DEFINED_HERE
#undef NV_EXTN_OP_SHFL
#undef NV_EXTN_OP_SHFL_UP
#undef NV_EXTN_OP_SHFL_DOWN
#undef NV_EXTN_OP_SHFL_XOR
#undef NV_EXTN_OP_VOTE_ALL
#undef NV_EXTN_OP_VOTE_ANY
#undef NV_EXTN_OP_VOTE_BALLOT
#undef NV_EXTN_OP_GET_LANE_ID
#undef NV_EXTN_OP_FP16_ATOMIC
#undef NV_EXTN_OP_FP32_ATOMIC
#undef NV_EXTN_OP_GET_SPECIAL
#undef NV_EXTN_OP_UINT64_ATOMIC
#undef NV_EXTN_OP_MATCH_ANY
#undef NV_EXTN_OP_FOOTPRINT
#undef NV_EXTN_OP_FOOTPRINT_BIAS
#undef NV_EXTN_OP_GET_SHADING_RATE
#undef NV_EXTN_OP_FOOTPRINT_LEVEL
#undef NV_EXTN_OP_FOOTPRINT_GRAD
#undef NV_EXTN_OP_SHFL_GENERIC
#undef NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE
#undef NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED
#undef NV_EXTN_OP_HIT_OBJECT_TRACE_RAY
#undef NV_EXTN_OP_HIT_OBJECT_MAKE_HIT
#undef NV_EXTN_OP_HIT_OBJECT_MAKE_HIT_WITH_RECORD_INDEX
#undef NV_EXTN_OP_HIT_OBJECT_MAKE_MISS
#undef NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD
#undef NV_EXTN_OP_HIT_OBJECT_INVOKE
#undef NV_EXTN_OP_HIT_OBJECT_IS_MISS
#undef NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_ID
#undef NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_INDEX
#undef NV_EXTN_OP_HIT_OBJECT_GET_PRIMITIVE_INDEX
#undef NV_EXTN_OP_HIT_OBJECT_GET_GEOMETRY_INDEX
#undef NV_EXTN_OP_HIT_OBJECT_GET_HIT_KIND
#undef NV_EXTN_OP_HIT_OBJECT_GET_RAY_DESC
#undef NV_EXTN_OP_HIT_OBJECT_GET_ATTRIBUTES
#undef NV_EXTN_OP_HIT_OBJECT_GET_SHADER_TABLE_INDEX
#undef NV_EXTN_OP_HIT_OBJECT_LOAD_LOCAL_ROOT_TABLE_CONSTANT
#undef NV_EXTN_OP_HIT_OBJECT_IS_HIT
#undef NV_EXTN_OP_HIT_OBJECT_IS_NOP
#undef NV_EXTN_OP_HIT_OBJECT_MAKE_NOP
#undef NV_EXTN_OP_RT_TRIANGLE_OBJECT_POSITIONS
#undef NV_EXTN_OP_RT_MICRO_TRIANGLE_OBJECT_POSITIONS
#undef NV_EXTN_OP_RT_MICRO_TRIANGLE_BARYCENTRICS
#undef NV_EXTN_OP_RT_IS_MICRO_TRIANGLE_HIT
#undef NV_EXTN_OP_RT_IS_BACK_FACING
#undef NV_EXTN_OP_RT_MICRO_VERTEX_OBJECT_POSITION
#undef NV_EXTN_OP_RT_MICRO_VERTEX_BARYCENTRICS
#undef NV_EXTN_OP_RT_GET_CLUSTER_ID
#undef NV_EXTN_OP_RT_GET_CANDIDATE_CLUSTER_ID
#undef NV_EXTN_OP_RT_GET_COMMITTED_CLUSTER_ID
#undef NV_EXTN_OP_HIT_OBJECT_GET_CLUSTER_ID
#undef NV_EXTN_OP_RT_CANDIDATE_TRIANGLE_OBJECT_POSITIONS
#undef NV_EXTN_OP_RT_COMMITTED_TRIANGLE_OBJECT_POSITIONS
#undef NV_EXTN_OP_HIT_OBJECT_GET_TRIANGLE_OBJECT_POSITIONS
#undef NV_EXTN_OP_RT_SPHERE_OBJECT_POSITION_AND_RADIUS
#undef NV_EXTN_OP_RT_CANDIDATE_SPHERE_OBJECT_POSITION_AND_RADIUS
#undef NV_EXTN_OP_RT_COMMITTED_SPHERE_OBJECT_POSITION_AND_RADIUS
#undef NV_EXTN_OP_HIT_OBJECT_GET_SPHERE_OBJECT_POSITION_AND_RADIUS
#undef NV_EXTN_OP_RT_LSS_OBJECT_POSITIONS_AND_RADII
#undef NV_EXTN_OP_RT_CANDIDATE_LSS_OBJECT_POSITIONS_AND_RADII
#undef NV_EXTN_OP_RT_COMMITTED_LSS_OBJECT_POSITIONS_AND_RADII
#undef NV_EXTN_OP_HIT_OBJECT_GET_LSS_OBJECT_POSITIONS_AND_RADII
#undef NV_EXTN_OP_RT_IS_SPHERE_HIT
#undef NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_SPHERE
#undef NV_EXTN_OP_RT_COMMITTED_IS_SPHERE
#undef NV_EXTN_OP_HIT_OBJECT_IS_SPHERE_HIT
#undef NV_EXTN_OP_RT_IS_LSS_HIT
#undef NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_LSS
#undef NV_EXTN_OP_RT_COMMITTED_IS_LSS
#undef NV_EXTN_OP_HIT_OBJECT_IS_LSS_HIT
#undef NV_EXTN_OP_RT_CANDIDATE_LSS_HIT_PARAMETER
#undef NV_EXTN_OP_RT_COMMITTED_LSS_HIT_PARAMETER
#undef NV_EXTN_OP_RT_CANDIDATE_BUILTIN_PRIMITIVE_RAY_T
#undef NV_EXTN_OP_RT_COMMIT_NONOPAQUE_BUILTIN_PRIMITIVE_HIT
#endif
  return {table};
}
} // namespace dxil

// TODO: move this somewhere else...
namespace shader_layout
{
BINDUMP_BEGIN_LAYOUT(ShaderLibraryContainer)
  BINDUMP_USING_EXTENSION()
  /// Driver independent name table, shader in the name table at position N should be the shader of that name from source in the shader
  /// binary. This table is required to be sorted, so that the user can use efficient binary searches.
  VecHolder<StrHolder> shaderNames;
  /// Driver specific shader library
  VecHolder<uint8_t> driverBinary;
BINDUMP_END_LAYOUT()
} // namespace shader_layout
