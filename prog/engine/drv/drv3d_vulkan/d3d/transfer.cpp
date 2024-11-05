// Copyright (C) Gaijin Games KFT.  All rights reserved.

// user provided GPU sync logic implementation
#include <drv/3d/dag_texture.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"
#include "texture.h"

using namespace drv3d_vulkan;

bool d3d::stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  TextureInterfaceBase *srcTex = cast_to_texture_base(src);
  TextureInterfaceBase *dstTex = cast_to_texture_base(dst);

  Image *srcImg = nullptr;
  Image *dstImg = nullptr;

  if (srcTex)
    srcImg = srcTex->getDeviceImage();

  if (dstTex)
    dstImg = dstTex->getDeviceImage();

  VkImageBlit blit;
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;

  if (rsrc)
  {
    blit.srcOffsets[0].x = rsrc->left;
    blit.srcOffsets[0].y = rsrc->top;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = rsrc->right;
    blit.srcOffsets[1].y = rsrc->bottom;
    blit.srcOffsets[1].z = 1;
  }
  else if (srcTex)
  {
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1] = toOffset(srcTex->getMipmapExtent(0));
    if (blit.dstOffsets[1].z < 1)
      blit.dstOffsets[1].z = 1;
  }
  else
  {
    VkExtent2D size = Globals::swapchain.getMode().extent;
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = size.width;
    blit.srcOffsets[1].y = size.height;
    blit.srcOffsets[1].z = 1;
  }

  if (rdst)
  {
    blit.dstOffsets[0].x = rdst->left;
    blit.dstOffsets[0].y = rdst->top;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = rdst->right;
    blit.dstOffsets[1].y = rdst->bottom;
    blit.dstOffsets[1].z = 1;
  }
  else if (dstTex)
  {
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1] = toOffset(dstTex->getMipmapExtent(0));
    if (blit.dstOffsets[1].z < 1)
      blit.dstOffsets[1].z = 1;
  }
  else
  {
    VkExtent2D size = Globals::swapchain.getMode().extent;
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = size.width;
    blit.dstOffsets[1].y = size.height;
    blit.dstOffsets[1].z = 1;
  }

  Globals::ctx.blitImage(srcImg, dstImg, blit, /*whole_subres*/ rdst == nullptr);
  return true;
}
