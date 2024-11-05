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
  uint32_t allocateBindlessResourceRange(uint32_t resourceType, uint32_t count);
  uint32_t resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount);
  void freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count);

  uint32_t registerBindlessSampler(id<MTLSamplerState> mtlSampler);

private:
  struct ResourceArray
  {
    uint32_t size = 0;
    eastl::vector<ValueRange<uint32_t>> freeSlotRanges;
  };
  ResourceArray textures;
  ResourceArray buffers;

  eastl::vector<id<MTLSamplerState>> samplerTable;
};

} // namespace drv3d_metal
