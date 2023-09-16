//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

enum class ShaderStage : uint32_t
{
  VERTEX,
  PIXEL,
  GEOMETRY,
  DOMAIN,
  HULL,
  COMPUTE,
  MESH,
  AMPLIFICATION,
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

const SemanticInfo *getSemanticInfoFromIndex(uint32_t index);
// NOTE: for something like TEXCOORD1 the input has to be "TEXCOOORD" for name and 1 for index
uint32_t getIndexFromSementicAndSemanticIndex(const char *name, uint32_t index);

ShaderHeaderCompileResult compileHeaderFromReflectionData(ShaderStage stage, const eastl::vector<uint8_t> &reflection,
  uint32_t max_const_count, uint32_t bone_const_used, void *dxc_lib);

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

} // namespace dxil
