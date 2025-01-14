//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/variant.h>
#include <EASTL/string.h>

namespace resource_dump_types
{
enum class TextureTypes
{
  TEX2D,
  CUBETEX,
  VOLTEX,
  ARRTEX,
  CUBEARRTEX
};
enum class ResourceType
{
  Buffer,
  Texture,
  RtAccel,
};
} // namespace resource_dump_types


struct ResourceDumpBase
{
  uint64_t gpuAddress, heapId, heapOffset;
};

struct ResourceDumpTexture : ResourceDumpBase
{
  int resX, resY, mip, depth, layers, memSize;
  uint32_t flags, formatFlags;
  resource_dump_types::TextureTypes texType;
  bool isColor;
  eastl::string name;
  eastl::string heapNamePS;

  const char *getTypeInString() const
  {
    switch (texType)
    {
      case resource_dump_types::TextureTypes::TEX2D: return "2D";
      case resource_dump_types::TextureTypes::VOLTEX: return "3D";
      case resource_dump_types::TextureTypes::CUBETEX: return "cube map";
      case resource_dump_types::TextureTypes::CUBEARRTEX: return "cube array";
      default: return "tex array";
    }
  }
};

struct ResourceDumpBuffer : ResourceDumpBase
{
  uint64_t offset, mapAddress;
  int memSize, sysCopySize, flags, currentDiscard, currentDiscardOffset, totalDiscards, id;
  eastl::string name;
  eastl::string heapNamePS;
};

struct ResourceDumpRayTrace : ResourceDumpBase
{
  int memSize;
  bool top;
  bool canDifferentiate;
};

typedef eastl::variant<ResourceDumpTexture, ResourceDumpBuffer, ResourceDumpRayTrace> ResourceDumpInfo;