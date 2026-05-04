// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include "image_resource.h"
#include "buffer_resource.h"
#include "buffer_ref.h"
#include "vulkan_api.h"

namespace drv3d_vulkan
{

struct CmdCopyBufferToImageOrdered
{
  Image *dst;
  Buffer *src;
  VkBufferImageCopy region;
};

struct CmdCopyBuffer
{
  BufferRef source;
  BufferRef dest;
  uint32_t sourceOffset;
  uint32_t destOffset;
  uint32_t dataSize;
};

struct CmdFillBuffer
{
  BufferRef buffer;
  uint32_t value;
};

struct CmdClearDepthStencilTexture
{
  Image *image;
  VkImageSubresourceRange area;
  VkClearDepthStencilValue value;
};

struct CmdClearColorTexture
{
  Image *image;
  VkImageSubresourceRange area;
  VkClearColorValue value;
};

struct CmdCopyImage
{
  Image *src;
  Image *dst;
  uint32_t srcMip;
  uint32_t dstMip;
  uint32_t mipCount;
  uint32_t regionCount;
  uint32_t regionIndex;
};

struct CmdBlitImage
{
  Image *src;
  Image *dst;
  VkImageBlit region;
  bool whole_subres;
};

struct CmdGenerateMipmaps
{
  Image *img;
};

struct CmdResolveMultiSampleImage
{
  Image *src;
  Image *dst;
};

struct CmdCaptureScreen
{
  Buffer *buffer;
  Image *colorImage;
  VkFormat *imageVkFormat;
};

} // namespace drv3d_vulkan
