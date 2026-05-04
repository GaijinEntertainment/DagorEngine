// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"

namespace drv3d_vulkan
{

struct BaseTex;
class Image;
class Buffer;

struct ResUpdateBufferImp
{
  BaseTex *destTex;
  Image *originalImg;
  Buffer *stagingBuffer;
  VkBufferImageCopy uploadInfo;
  size_t pitch;
  size_t slicePitch;
};

} // namespace drv3d_vulkan
