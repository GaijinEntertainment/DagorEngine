//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_bindump_ext.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
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
  INVALID_STAGE = 0xFFFF,
};

enum SpecialConstantType
{
  SC_DRAW_ID = 1,
};

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
  uint8_t unusedBytes[3];
};

inline bool is_compatible(const ShaderDeviceRequirement &base, const ShaderDeviceRequirement &compare)
{
  // version is packed lower 4 bytes are minor and upper 4 bytes is major, so simple compare should work correctly.
  return base.shaderModel >= compare.shaderModel &&
         // when any feature bit is set by compare that base does not has, its not compatible.
         0 == ((~base.shaderFeatureFlagsLow) & compare.shaderFeatureFlagsLow) &&
         0 == ((~base.shaderFeatureFlagsHigh) & compare.shaderFeatureFlagsHigh);
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
         0 == (feature_mask_high & ((~base.shaderFeatureFlagsHigh) & compare.shaderFeatureFlagsHigh));
}

struct ShaderHeader
{
  uint16_t shaderType;
  // input primitive for tesselation stages
  uint16_t inputPrimitive;
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

  // @TODO: proper padding zeroeing
  uint8_t pad_[4 - MAX_U_REGISTERS % 4];

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

struct ShaderHeaderCompileResult
{
  ShaderHeader header = {};
  ComputeShaderInfo computeShaderInfo = {};
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
  uint32_t max_const_count, void *dxc_lib);

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

enum class StoredShaderType
{
  singleShader,
  combinedVertexShader,
  meshShader,
};

BINDUMP_BEGIN_LAYOUT(ShaderContainer)
  BINDUMP_USING_EXTENSION()
  StoredShaderType type;
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

} // namespace dxil
