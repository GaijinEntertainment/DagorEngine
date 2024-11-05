//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>

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

struct ResourceDumpInfo
{

  int memSize;
  uint64_t gpuAddress;
  resource_dump_types::ResourceType type;
  union
  {
    struct
    {
      const char *name;
      int flags, totalDiscards, currentDiscard, id, sysCopySize;
      uint64_t offset, mapAddress, gpuAddress;
    } buffer;

    struct
    {
      const char *name;
      int resX, resY, mip, depth, layers;
      uint32_t flags, formatFlags;
      resource_dump_types::TextureTypes texType;
      bool isColor;

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
    } texture;

    struct
    {
      bool top;
    } rtAcceleration;
  };

  ResourceDumpInfo(resource_dump_types::ResourceType type, int mem_size, uint64_t gpu_address, bool top) :
    memSize(mem_size), gpuAddress(gpu_address), type(type), rtAcceleration({top})
  {}

  ResourceDumpInfo(resource_dump_types::ResourceType type, const char *name, int mem_size, int sys_copy_size, int flags,
    int current_discard, int total_discards, int id, uint64_t offset, uint64_t map_address, uint64_t gpu_address) :
    type(type),
    gpuAddress(gpu_address),
    memSize(mem_size),
    buffer({name, flags, total_discards, current_discard, id, sys_copy_size, offset, map_address})
  {}

  ResourceDumpInfo(resource_dump_types::ResourceType type, const char *name, int mem_size, resource_dump_types::TextureTypes tex_type,
    int res_x, int res_y, int depth, int layers, int mip, uint32_t format_flags, bool is_color, uint32_t flags, uint64_t gpu_address) :
    memSize(mem_size),
    gpuAddress(gpu_address),
    type(type),
    texture({name, res_x, res_y, mip, depth, layers, flags, format_flags, tex_type, is_color})
  {}
};