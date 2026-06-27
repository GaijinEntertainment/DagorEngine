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
  // when set, the caller writes here and update() memcpys it into stagingBuffer: a kernel fread
  // cannot write host-visible GPU memory on some stacks (WSL2 dxgkrnl), but a user memcpy can.
  void *cpuShadow;
};

} // namespace drv3d_vulkan
