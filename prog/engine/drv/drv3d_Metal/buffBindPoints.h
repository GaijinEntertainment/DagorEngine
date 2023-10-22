
#pragma once

#include <debug/dag_assert.h>

namespace drv3d_metal
{
enum BufferType
{
  GEOM_BUFFER = 0,
  CONST_BUFFER = 1,
  STRUCT_BUFFER = 2,
  RW_BUFFER = 3
};

enum BufferTypeCount
{
  GEOM_BUFFER_COUNT = 2,
  CONST_BUFFER_COUNT = 9,
  STRUCT_BUFFER_COUNT = 32,
  RW_BUFFER_COUNT = 8
};

enum BUFFERBINDPOINT
{
  GEOM_BUFFER_POINT = 0,
  CONST_BUFFER_POINT = GEOM_BUFFER_POINT + GEOM_BUFFER_COUNT + 1,
  STRUCT_BUFFER_POINT = CONST_BUFFER_POINT + CONST_BUFFER_COUNT,
  RW_BUFFER_POINT = STRUCT_BUFFER_POINT + STRUCT_BUFFER_COUNT,
  BUFFER_POINT_COUNT = RW_BUFFER_POINT + RW_BUFFER_COUNT
};

static constexpr int BIND_POINT = GEOM_BUFFER_COUNT;

static constexpr int MAX_SHADER_TEXTURES = 32;

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
  }

  return slot;
}
} // namespace drv3d_metal
