// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_d3dResource.h>
#include <debug/dag_assert.h>

#include "value_range.h"

#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>

#import <Metal/Metal.h>

namespace drv3d_metal
{

class Shader;
class Texture;
class Buffer;

class BindlessManager
{
public:
  uint32_t allocateBindlessResourceRange(D3DResourceType type, uint32_t count);
  void freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count);

  uint32_t registerBindlessSampler(int index, float bias);

  bool empty() const
  {
    return textures2d.empty() && texturesCube.empty() && textures2dArray.empty() && textures3d.empty() && texturesCubeArray.empty() &&
           buffers.empty();
  }

private:
  struct ResourceArray
  {
    uint32_t size = 0;
    eastl::vector<ValueRange<uint32_t>> freeSlotRanges;

    bool empty() const { return size == 0; }
  };

  ResourceArray &getArray(D3DResourceType type)
  {
    switch (type)
    {
      case D3DResourceType::SBUF: return buffers;
      case D3DResourceType::TEX: return textures2d;
      case D3DResourceType::CUBETEX: return texturesCube;
      case D3DResourceType::ARRTEX: return textures2dArray;
      case D3DResourceType::VOLTEX: return textures3d;
      case D3DResourceType::CUBEARRTEX: return texturesCubeArray;
      default: G_ASSERTF(0, "Unknown bindless array type %d", int(type)); return buffers;
    }
  }

  uint32_t allocateBindlessResourceRangeNoLock(D3DResourceType type, uint32_t count);
  void freeBindlessResourceRangeNoLock(D3DResourceType type, uint32_t index, uint32_t count);

  ResourceArray textures2d;
  ResourceArray texturesCube;
  ResourceArray textures2dArray;
  ResourceArray textures3d;
  ResourceArray texturesCubeArray;
  ResourceArray buffers;

  eastl::vector<id<MTLSamplerState>> samplerTable;
};

} // namespace drv3d_metal
