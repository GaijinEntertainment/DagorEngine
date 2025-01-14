// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>

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

union EncodedMetalImageType
{
  struct
  {
    MetalImageType type;
    uint8_t is_uint; // is this uint texture
    uint8_t is_int;  // is this int texture
  };
  int value = 0;
};

enum BufferType
{
  GEOM_BUFFER = 0,
  CONST_BUFFER = 1,
  STRUCT_BUFFER = 2,
  RW_BUFFER = 3,
  BINDLESS_TEXTURE_ID_BUFFER = 4,
  BINDLESS_SAMPLER_ID_BUFFER = 5,
  BINDLESS_BUFFER_ID_BUFFER = 6
};

enum BufferTypeCount
{
  GEOM_BUFFER_COUNT = 2,
  CONST_BUFFER_COUNT = 9,
  STRUCT_BUFFER_COUNT = 32,
  RW_BUFFER_COUNT = 8,
  BINDLESS_TEXTURE_ID_BUFFER_COUNT = 6,
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

static constexpr int MAX_SHADER_TEXTURES = 32;
static constexpr int MAX_SHADER_ACCELERATION_STRUCTURES = 32;

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
