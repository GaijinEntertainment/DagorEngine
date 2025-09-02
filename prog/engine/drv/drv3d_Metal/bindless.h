// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_d3dResource.h>

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
  uint32_t resizeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t currentCount, uint32_t newCount);
  void freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count);

  uint32_t registerBindlessSampler(int index, float bias);

  bool empty() const { return textures2d.empty() && texturesCube.empty() && textures2dArray.empty() && buffers.empty(); }

private:
  struct ResourceArray
  {
    uint32_t size = 0;
    eastl::vector<ValueRange<uint32_t>> freeSlotRanges;

    bool empty() const { return size == 0; }
  };

  ResourceArray &getArray(D3DResourceType type)
  {
    return type == D3DResourceType::SBUF
             ? buffers
             : (type == D3DResourceType::TEX ? textures2d : (type == D3DResourceType::CUBETEX ? texturesCube : textures2dArray));
  }

  ResourceArray textures2d;
  ResourceArray texturesCube;
  ResourceArray textures2dArray;
  ResourceArray buffers;

  eastl::vector<id<MTLSamplerState>> samplerTable;
};

} // namespace drv3d_metal
