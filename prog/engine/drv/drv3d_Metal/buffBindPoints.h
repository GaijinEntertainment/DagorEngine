// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <drv/shadersMetaData/metal/register_limits.h>

namespace drv3d_metal
{

enum class MetalImageType : uint8_t
{
  Tex2D = 0,
  Tex2DArray = 1,
  Tex2DDepth = 2,
  TexCube = 3,
  Tex3D = 4,
  TexBuffer = 5,
  TexCubeArray = 6,
  Count
};

enum
{
  // STAGE_CS, STAGE_PS, STAGE_VS
  // artificial stage to be able to bind STAGE_VS resources to mesh and object shaders
  STAGE_MS = 3,
  STAGE_OS = 4,
  STAGE_TOTAL
};

namespace ShaderFlags
{
enum
{
  HasSamplerBiases = 1 << 0,
  HasImmediateConstants = 1 << 1
};
}

union EncodedMetalImageType
{
  struct
  {
    MetalImageType type;
    uint8_t is_uint; // is this uint texture
    uint8_t is_int;  // is this int texture
    // set for a UAV texture the shader reads or writes, both clear when unknown
    uint8_t is_read : 1;
    uint8_t is_written : 1;
  };
  int value = 0;
};
static_assert(sizeof(EncodedMetalImageType) == 4, "stored as an int in the shader header");

enum BufferType : uint8_t
{
  GEOM_BUFFER = 0,
  CONST_BUFFER = 1,
  STRUCT_BUFFER = 2,
  RW_BUFFER = 3,
  BINDLESS_TEXTURE_ID_BUFFER = 4,
  BINDLESS_SAMPLER_ID_BUFFER = 5,
  BINDLESS_BUFFER_ID_BUFFER = 6
};

union EncodedBufferRemap
{
  enum class RemapType : uint8_t
  {
    Invalid = 0,
    Buffer,
    Texture,
    Sampler
  };

  struct
  {
    MetalImageType texture_type;
    BufferType buffer_type;
    RemapType remap_type;
    int8_t slot;
  };
  int value;

  EncodedBufferRemap()
  {
    texture_type = MetalImageType::Count;
    buffer_type = BufferType::GEOM_BUFFER;
    remap_type = RemapType::Invalid;
    slot = -1;
  }
};
static_assert(sizeof(EncodedBufferRemap) == 4, "this is important");

enum BufferTypeCount
{
  GEOM_BUFFER_COUNT = 2,
  CONST_BUFFER_COUNT = metal::MAX_B_REGISTERS,
  STRUCT_BUFFER_COUNT = metal::MAX_T_REGISTERS,
  RW_BUFFER_COUNT = metal::MAX_U_REGISTERS,
  BINDLESS_TEXTURE_ID_BUFFER_COUNT = 5,
  BINDLESS_SAMPLER_ID_BUFFER_COUNT = 3,
  BINDLESS_BUFFER_ID_BUFFER_COUNT = 3,

  GEOM_BUFFER_MASK = ((1ull << GEOM_BUFFER_COUNT) - 1) << 0ull
};

enum BUFFERBINDPOINT
{
  GEOM_BUFFER_POINT = 0,
  CONST_BUFFER_POINT = GEOM_BUFFER_POINT + GEOM_BUFFER_COUNT + 1,
  STRUCT_BUFFER_POINT = CONST_BUFFER_POINT + CONST_BUFFER_COUNT,
  RW_BUFFER_POINT = STRUCT_BUFFER_POINT + STRUCT_BUFFER_COUNT,
  BINDLESS_TEXTURE_ID_BUFFER_POINT = RW_BUFFER_POINT + RW_BUFFER_COUNT,
  BINDLESS_SAMPLER_ID_BUFFER_POINT = BINDLESS_TEXTURE_ID_BUFFER_POINT + BINDLESS_TEXTURE_ID_BUFFER_COUNT,
  BINDLESS_BUFFER_ID_BUFFER_POINT = BINDLESS_SAMPLER_ID_BUFFER_POINT + BINDLESS_SAMPLER_ID_BUFFER_COUNT,
  BUFFER_POINT_COUNT = BINDLESS_BUFFER_ID_BUFFER_POINT + BINDLESS_BUFFER_ID_BUFFER_COUNT
};

static constexpr int BIND_POINT = GEOM_BUFFER_COUNT;
static constexpr int IMMEDIATE_BIND_SLOT = BIND_POINT + 2;                // backend bind slot
static constexpr int IMMEDIATE_BIND_POINT = metal::IMMEDIATE_CB_REGISTER; // hlsl slot

static constexpr int MAX_SHADER_TEXTURES = metal::MAX_T_REGISTERS;

// first MAX_SHADER_TEXTURES is ordinary textures
// second MAX_SHADER_TEXTURES are uav textures
static constexpr int MAX_STAGE_TEXTURES = MAX_SHADER_TEXTURES * 2;
static constexpr int MAX_SHADER_ACCELERATION_STRUCTURES = metal::MAX_T_REGISTERS;

#define BIND_POINT_STR "2"

inline int RemapBufferSlot(BufferType bf_type, int slot)
{
  switch (bf_type)
  {
    case GEOM_BUFFER:
    {
      G_ASSERT(slot < GEOM_BUFFER_COUNT);
      slot += GEOM_BUFFER;
      break;
    }
    case CONST_BUFFER:
    {
      G_ASSERT(slot < CONST_BUFFER_COUNT);
      slot += CONST_BUFFER_POINT;
      break;
    }
    case STRUCT_BUFFER:
    {
      G_ASSERT(slot < STRUCT_BUFFER_COUNT);
      slot += STRUCT_BUFFER_POINT;
      break;
    }
    case RW_BUFFER:
    {
      G_ASSERT(slot < RW_BUFFER_COUNT);
      slot += RW_BUFFER_POINT;
      break;
    }
    case BINDLESS_TEXTURE_ID_BUFFER:
    {
      G_ASSERT(slot < BINDLESS_TEXTURE_ID_BUFFER_COUNT);
      slot += BINDLESS_TEXTURE_ID_BUFFER_POINT;
      break;
    }
    case BINDLESS_SAMPLER_ID_BUFFER:
    {
      G_ASSERT(slot < BINDLESS_SAMPLER_ID_BUFFER_COUNT);
      slot += BINDLESS_SAMPLER_ID_BUFFER_POINT;
      break;
    }
    case BINDLESS_BUFFER_ID_BUFFER:
    {
      G_ASSERT(slot < BINDLESS_BUFFER_ID_BUFFER_COUNT);
      slot += BINDLESS_BUFFER_ID_BUFFER_POINT;
      break;
    }
  }

  return slot;
}
} // namespace drv3d_metal
