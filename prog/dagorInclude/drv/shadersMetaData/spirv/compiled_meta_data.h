//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_strUtil.h>
#include <generic/dag_tab.h>
#include <hash/sha1.h>
#include <vulkan/vulkan.h>

// contains the layout of compiled shaders as spir-v
namespace spirv
{

constexpr uint32_t HEADER_MAGIC_VER = 0x73680002;

struct HashValue
{
  typedef uint8_t ValueType[20];
  ValueType value;

  void clear()
  {
    for (uint8_t &i : value)
      i = 0;
  }

  friend inline bool operator==(const HashValue &l, const HashValue &r)
  {
    G_STATIC_ASSERT(sizeof(HashValue) == sizeof(ValueType));
    G_STATIC_ASSERT(20 == sizeof(ValueType));
    return memcmp(l.value, r.value, sizeof(ValueType)) == 0;
  }

  friend inline bool operator!=(const HashValue &l, const HashValue &r) { return !(l == r); }

  template <typename T>
  static inline HashValue calculate(dag::ConstSpan<T> data)
  {
    HashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data.data()), static_cast<int>(data_size(data)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  template <typename T>
  static inline HashValue calculate(const T *data, size_t count)
  {
    HashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data), static_cast<int>(count * sizeof(T)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  void convertToString(char *buffer, size_t size) const
  {
    G_ASSERT(size > (sizeof(HashValue) * 2));
    data_to_str_hex_buf(buffer, size, value, sizeof(value));
  }
};

namespace desktop
{
// some old nvidia drivers does have limit of 4, but they are too old
const uint32_t MAX_DESCRIPTOR_SET_COUNT = 8;
// limited by intel
const uint32_t MAX_SAMPLED_IMAGES_PER_STAGE = 64;
// limited by nvidia, even intel has 64 slots
const uint32_t MAX_STORAGE_BUFFERS_PER_STAGE = 16;
// limited by intel
const uint32_t MAX_STORAGE_IMAGES_PER_STAGE = 64;
// limited by nvidia, even intel has 64 slots
const uint32_t MAX_UNIFORM_BUFFERS_PER_STAGE = 12;
// limited by intel
const uint32_t MAX_COMBINED_RESOURCES_PER_STAGE = 128;
// common limit on most hardware
const uint32_t MAX_COLOR_ATTACHMENTS = 8;
} // namespace desktop

namespace mobile
{
// mali and adreno are limited to 4
const uint32_t MAX_DESCRIPTOR_SET_COUNT = 4;
// limited by mali (max sampled images) and adreno (max samplers)
const uint32_t MAX_SAMPLED_IMAGES_PER_STAGE = 16;
// limited by mali and adreno
const uint32_t MAX_STORAGE_BUFFERS_PER_STAGE = 4;
// limited by mali and adreno
const uint32_t MAX_STORAGE_IMAGES_PER_STAGE = 4;
// limited by mali and nvidia
const uint32_t MAX_UNIFORM_BUFFERS_PER_STAGE = 12;
// limited by mali
const uint32_t MAX_COMBINED_RESOURCES_PER_STAGE = 44;
// limited by mali
const uint32_t MAX_COLOR_ATTACHMENTS = 4;
} // namespace mobile

namespace platform
{
// TODO: if we support mobile with vulkan than this
//       needs to be conditionally use mobile instead
using namespace desktop;
} // namespace platform

// do not set above 32! this directly maps to header.*RegisterUseMask 32bit field!
//  limit for b register entries, this is for the renderer to store the bindings
const uint32_t B_REGISTER_INDEX_MAX = 8;
// limit for t register entries, this is for the renderer to store the bindings
const uint32_t T_REGISTER_INDEX_MAX = 32;
// limit for s register entries, this is for the renderer to store the bindings
// it's the same as for t registers, since we use combined imaged samplers
const uint32_t S_REGISTER_INDEX_MAX = T_REGISTER_INDEX_MAX;
// limit for u register entries, this is for the renderer to store the bindings
const uint32_t U_REGISTER_INDEX_MAX = 13;
// limit for t input attachment entries
const uint32_t T_INPUT_ATTACHMENT_INDEX_MAX = 8;

// total combination of b, t and u register entries
const uint32_t REGISTER_ENTRIES = B_REGISTER_INDEX_MAX + T_REGISTER_INDEX_MAX + U_REGISTER_INDEX_MAX;

const uint32_t WORK_GROUP_SIZE_X_CONSTANT_ID = 1;
const uint32_t WORK_GROUP_SIZE_Y_CONSTANT_ID = 2;
const uint32_t WORK_GROUP_SIZE_Z_CONSTANT_ID = 3;

const uint32_t B_CONST_BUFFER_OFFSET = 0;

const uint32_t T_SAMPLED_IMAGE_OFFSET = B_CONST_BUFFER_OFFSET + B_REGISTER_INDEX_MAX;
const uint32_t T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET = T_SAMPLED_IMAGE_OFFSET + T_REGISTER_INDEX_MAX;
const uint32_t T_BUFFER_SAMPLED_IMAGE_OFFSET = T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET + T_REGISTER_INDEX_MAX;
const uint32_t T_BUFFER_OFFSET = T_BUFFER_SAMPLED_IMAGE_OFFSET + T_REGISTER_INDEX_MAX;
const uint32_t T_INPUT_ATTACHMENT_OFFSET = T_BUFFER_OFFSET + T_REGISTER_INDEX_MAX;

const uint32_t U_IMAGE_OFFSET = T_INPUT_ATTACHMENT_OFFSET + T_INPUT_ATTACHMENT_INDEX_MAX;
const uint32_t U_BUFFER_IMAGE_OFFSET = U_IMAGE_OFFSET + U_REGISTER_INDEX_MAX;
const uint32_t U_BUFFER_OFFSET = U_BUFFER_IMAGE_OFFSET + U_REGISTER_INDEX_MAX;

// like dx12RT acceleration structure are put in t namespace for now
const uint32_t T_ACCELERATION_STRUCTURE_OFFSET = U_BUFFER_OFFSET + U_REGISTER_INDEX_MAX;

const uint32_t TOTAL_REGISTER_COUNT = T_ACCELERATION_STRUCTURE_OFFSET + T_REGISTER_INDEX_MAX;

const uint32_t FALLBACK_TO_C_GLOBAL_BUFFER = 0x00;
const uint32_t MISSING_CONST_BUFFER_INDEX = 0x01;
const uint32_t MISSING_SAMPLED_IMAGE_2D_INDEX = 0x02;
const uint32_t MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX = 0x03;
const uint32_t MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX = 0x04;
const uint32_t MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX = 0x05;
const uint32_t MISSING_SAMPLED_IMAGE_3D_INDEX = 0x06;
const uint32_t MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX = 0x07;
const uint32_t MISSING_SAMPLED_IMAGE_CUBE_INDEX = 0x08;
const uint32_t MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX = 0x09;
const uint32_t MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX = 0x0A;
const uint32_t MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX = 0x0B;
const uint32_t MISSING_BUFFER_SAMPLED_IMAGE_INDEX = 0x0C;
const uint32_t MISSING_BUFFER_INDEX = 0x0D;
const uint32_t MISSING_STORAGE_IMAGE_2D_INDEX = 0x0E;
const uint32_t MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX = 0x0F;
const uint32_t MISSING_STORAGE_IMAGE_3D_INDEX = 0x10;
const uint32_t MISSING_STORAGE_IMAGE_CUBE_INDEX = 0x11;
const uint32_t MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX = 0x12;
const uint32_t MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX = 0x13;
const uint32_t MISSING_STORAGE_BUFFER_INDEX = 0x14;
const uint32_t MISSING_IS_FATAL_INDEX = 0x15;
const uint8_t INVALID_INPUT_ATTACHMENT_INDEX = 0xFF;

#define VK_DESCRIPTOR_TYPE_BEGIN_RANGE VK_DESCRIPTOR_TYPE_SAMPLER
#define VK_DESCRIPTOR_TYPE_END_RANGE   VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
#define VK_DESCRIPTOR_TYPE_RANGE_SIZE  (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT - VK_DESCRIPTOR_TYPE_SAMPLER + 1)

// plus one for raytrace acceleration structures
const uint32_t SHADER_HEADER_DECRIPTOR_COUNT_SIZE = VK_DESCRIPTOR_TYPE_RANGE_SIZE + 1;

inline uint32_t descrior_type_to_index(VkDescriptorType type)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  if (type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
    return uint32_t(VK_DESCRIPTOR_TYPE_END_RANGE) + 1;
#endif
  return uint32_t(type);
}

inline VkDescriptorType descriptor_index_to_type(uint32_t index)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  if (index == (uint32_t(VK_DESCRIPTOR_TYPE_END_RANGE) + 1))
    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
#endif
  return VkDescriptorType(index);
}

struct ShaderHeader
{
  uint32_t verMagic;
  VkDescriptorType descriptorTypes[REGISTER_ENTRIES];
  VkImageViewType imageViewTypes[REGISTER_ENTRIES];
  VkDescriptorPoolSize descriptorCounts[SHADER_HEADER_DECRIPTOR_COUNT_SIZE];
  uint32_t maxConstantCount;
  uint32_t bonesConstantsUsed;
  uint32_t tRegisterUseMask;
  uint32_t uRegisterUseMask;
  uint32_t bRegisterUseMask;
  uint32_t inputMask;
  uint32_t outputMask;
  uint8_t inputAttachmentIndex[REGISTER_ENTRIES];
  uint8_t registerIndex[REGISTER_ENTRIES];
  uint8_t imageCheckIndices[REGISTER_ENTRIES];
  uint8_t bufferCheckIndices[REGISTER_ENTRIES];
  uint8_t bufferViewCheckIndices[T_REGISTER_INDEX_MAX];
  uint8_t constBufferCheckIndices[B_REGISTER_INDEX_MAX];
  uint8_t accelerationStructureCheckIndices[T_REGISTER_INDEX_MAX];
  uint8_t missingTableIndex[REGISTER_ENTRIES];
  uint8_t inputAttachmentCount;
  uint8_t imageCount;
  uint8_t bufferCount;
  uint8_t bufferViewCount;
  uint8_t constBufferCount;
  uint8_t accelerationStructursCount;
  uint8_t descriptorCountsCount;
  uint8_t registerCount;
};

inline bool operator==(const ShaderHeader &l, const ShaderHeader &r) { return 0 == (memcmp(&l, &r, sizeof(ShaderHeader))); }

inline bool operator!=(const ShaderHeader &l, const ShaderHeader &r) { return !(l == r); }

struct ComputeShaderInfo
{
  uint32_t threadGroupSizeX;
  uint32_t threadGroupSizeY;
  uint32_t threadGroupSizeZ;
};

namespace graphics
{
namespace vertex
{
const uint32_t REGISTERS_SET_INDEX = 0;
}
namespace fragment
{
const uint32_t REGISTERS_SET_INDEX = 1;
}
namespace geometry
{
const uint32_t REGISTERS_SET_INDEX = 2;
}
namespace control
{
const uint32_t REGISTERS_SET_INDEX = 3;
}
namespace evaluation
{
const uint32_t REGISTERS_SET_INDEX = 4;
}
const uint32_t MAX_SETS = 5;
} // namespace graphics

namespace compute
{
const uint32_t REGISTERS_SET_INDEX = 0;

const uint32_t MAX_SETS = 1;
} // namespace compute

namespace raytrace
{
// for now we munch everything into one set
const uint32_t REGISTERS_SET_INDEX = 0;
const uint32_t MAX_SETS = 1;
} // namespace raytrace

namespace bindless
{
constexpr uint32_t MAX_SETS = 3;

constexpr uint32_t FIRST_DESCRIPTOR_SET_ACTUAL_INDEX = 0;
constexpr uint32_t TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX = FIRST_DESCRIPTOR_SET_ACTUAL_INDEX;
constexpr uint32_t SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX = TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX + 1;
constexpr uint32_t BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX = SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX + 1;
constexpr uint32_t MAX_DESCRIPTOR_SET_ACTUAL_INDEX = BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX;

constexpr uint32_t FIRST_DESCRIPTOR_SET_META_INDEX = 100;
constexpr uint32_t TEXTURE_DESCRIPTOR_SET_META_INDEX = FIRST_DESCRIPTOR_SET_META_INDEX + TEXTURE_DESCRIPTOR_SET_ACTUAL_INDEX;
constexpr uint32_t SAMPLER_DESCRIPTOR_SET_META_INDEX = FIRST_DESCRIPTOR_SET_META_INDEX + SAMPLER_DESCRIPTOR_SET_ACTUAL_INDEX;
constexpr uint32_t BUFFER_DESCRIPTOR_SET_META_INDEX = FIRST_DESCRIPTOR_SET_META_INDEX + BUFFER_DESCRIPTOR_SET_ACTUAL_INDEX;
constexpr uint32_t MAX_DESCRIPTOR_SET_META_INDEX = BUFFER_DESCRIPTOR_SET_META_INDEX;

} // namespace bindless

// required to be 256 byte aligned on nvidia
const uint32_t ATOMIC_COUNTER_REGION_SIZE = 256;

const uint32_t SPIR_V_BLOB_IDENT = _MAKE4C('SVz3');
const uint32_t SPIR_V_BLOB_IDENT_UNCOMPRESSED = _MAKE4C('SVu3');
const uint32_t SPIR_V_COMBINED_BLOB_IDENT = _MAKE4C('SVc1');

struct CombinedChunk
{
  VkShaderStageFlagBits stage;
  uint32_t offset;
  uint32_t size;
};

enum class ChunkType : uint32_t
{
  // contains ShaderHeader
  SHADER_HEADER,
  // spir-v compressed into smol-v format (probably not usable with extension)
  SMOL_V,
  // spir-v tools compression system, likely to be compatible with extensions
  MARK_V, // unused
          // raw spir-v (big!)
  SPIR_V, // fall-back if smol-v fails
          // human readable spir-v
  SPIR_V_DISASSEMBLY,
  // if hlsl->dxbc->glsl->spir-v toolchain is used, then this contains human readable dxbc
  HLSL_DISASSEMBLY,
  // if hlsl->dxbc->glsl->spir-v toolchain is used, then this contains human readable glsl
  RECONSTRUCTED_GLSL,
  // contains from spir-v reconstructed hlsl that is compiled down dxbc, in human readable form
  RECONSTRUCTED_HLSL_DISASSEMBLY,   // unsupported right now
                                    // xdiff of RECONSTRUCTED_HLSL_DISASSEMBLY and HLSL_DISASSEMBLY
  HLSL_AND_RECONSTRUCTED_HLSL_XDIF, // unsupported right now
                                    // that what the shader compiler gets
  UNPROCESSED_HLSL,
  // Name of the shader for better debug info, should be always exported by the compiler
  SHADER_NAME,
};

// Some notes:
// The shader compiler has to write out the variants in priority order for
// the backend to select. This simplifies the loading at the backend
// as it has only to find the first match to find its preferred variant.
enum ExtensionBits
{
  // for future use, if/when we support shader extensions (and with it multiple variants of the same
  // shader)
};

struct ChunkHeader
{
  HashValue hash;
  ChunkType type;
  uint32_t extensionBits;
  uint32_t offset;
  uint32_t size;
};
// file layout:
// Tab<ChunkHeader> chunks;
// Tab<uint8_t> chunkData;

inline ChunkHeader &add_chunk(Tab<ChunkHeader> &chunks, Tab<uint8_t> &data, ChunkType type, uint32_t extension_bits, const char *chunk,
  uint32_t count)
{
  ChunkHeader header;
  header.hash = HashValue::calculate(chunk, count);
  header.type = type;
  header.extensionBits = extension_bits;
  header.offset = data.size();
  header.size = count;
  append_items(data, count, reinterpret_cast<const uint8_t *>(chunk));
  // ensure 0 termination
  data.push_back(0);
  // align to next 4 bytes
  const uint32_t zero = 0;
  append_items(data, (4 - (data.size() & 3)) & 3, reinterpret_cast<const uint8_t *>(&zero));
  return chunks.push_back(header);
}

template <typename T>
inline ChunkHeader &add_chunk(Tab<ChunkHeader> &chunks, Tab<uint8_t> &data, ChunkType type, uint32_t extension_bits, const T *chunk,
  uint32_t count)
{
  ChunkHeader header;
  header.hash = HashValue::calculate(chunk, count);
  header.type = type;
  header.extensionBits = extension_bits;
  header.offset = data.size();
  header.size = sizeof(T) * count;
  append_items(data, sizeof(T) * count, reinterpret_cast<const uint8_t *>(chunk));
  // align to next 4 bytes
  const uint32_t zero = 0;
  append_items(data, (4 - (data.size() & 3)) & 3, reinterpret_cast<const uint8_t *>(&zero));
  return chunks.push_back(header);
}
template <typename T>
inline ChunkHeader &add_chunk(Tab<ChunkHeader> &chunks, Tab<uint8_t> &data, ChunkType type, uint32_t extension_bits, const T &chunk)
{
  return add_chunk(chunks, data, type, extension_bits, &chunk, 1);
}

inline ChunkHeader &add_chunk(Tab<ChunkHeader> &chunks, Tab<uint8_t> &data, uint32_t extension_bits, const ShaderHeader &chunk)
{
  return add_chunk(chunks, data, ChunkType::SHADER_HEADER, extension_bits, chunk);
}

inline const ChunkHeader *find_chunk(const Tab<ChunkHeader> &chunks, ChunkType type, uint32_t extension_mask)
{
  for (auto &&chunk : chunks)
  {
    if (type != chunk.type)
      continue;

    if (chunk.extensionBits != (extension_mask & chunk.extensionBits))
      continue;

    return &chunk;
  }
  return nullptr;
}

// sole purpose of this is to trigger the static asserts here, this header is used
// in tools and engine/drv
inline void do_trigger_static_assert_if_not_matching()
{
  // if one of those goes off, then the limit in question hit a hard hardware limit
  G_STATIC_ASSERT(REGISTER_ENTRIES <= platform::MAX_COMBINED_RESOURCES_PER_STAGE);
  G_STATIC_ASSERT(graphics::MAX_SETS <= platform::MAX_DESCRIPTOR_SET_COUNT);
  G_STATIC_ASSERT(compute::MAX_SETS <= platform::MAX_DESCRIPTOR_SET_COUNT);
  // have to bump register index type to next bigger int if this complains
  G_STATIC_ASSERT(TOTAL_REGISTER_COUNT < 256);
}
} // namespace spirv
