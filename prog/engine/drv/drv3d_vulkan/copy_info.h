// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <dag/dag_vector.h>
#include "vulkan_api.h"

namespace drv3d_vulkan
{

class Buffer;
class Image;

struct BufferFlushInfo
{
  Buffer *buffer = nullptr;
  uint32_t offset = 0;
  uint32_t range = 0;
};

struct BufferCopyInfo
{
  Buffer *src;
  Buffer *dst;
  uint32_t copyIndex;
  uint32_t copyCount;

  static void optimizeBufferCopies(dag::Vector<BufferCopyInfo> &info, dag::Vector<VkBufferCopy> &copies);
};

struct ImageCopyInfo
{
  Image *image;
  Buffer *buffer;
  uint32_t copyIndex;
  uint32_t copyCount;

  static void deduplicate(dag::Vector<ImageCopyInfo> &info, dag::Vector<VkBufferImageCopy> &copies);
};

} // namespace drv3d_vulkan
