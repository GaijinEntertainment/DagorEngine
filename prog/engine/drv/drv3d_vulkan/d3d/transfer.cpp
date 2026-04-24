// Copyright (C) Gaijin Games KFT.  All rights reserved.

// user provided GPU sync logic implementation
#include <drv/3d/dag_texture.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"
#include "texture.h"

using namespace drv3d_vulkan;

bool d3d::stretch_rect(BaseTexture *src, BaseTexture *dst, const RectInt *rsrc, const RectInt *rdst)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  TextureInterfaceBase *srcTex = cast_to_texture_base(src);
  TextureInterfaceBase *dstTex = cast_to_texture_base(dst);

  if (!srcTex || !dstTex)
    return false;

  // this would work only if layers == depth and need special handling, drop such requests for now as they look strange on their own
  if (srcTex->pars.isVolume() ^ dstTex->pars.isVolume())
  {
    D3D_ERROR("vulkan: trying to blit volume texture with non volume one. src %s dst %s", srcTex->getName(), dstTex->getName());
    return false;
  }

  Image *srcImg = srcTex->image;
  Image *dstImg = dstTex->image;

  VkImageBlit blit;
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;

  VkExtent3D srcExt = srcTex->pars.getMipExtent(0);
  blit.srcOffsets[1] = toOffset(srcExt);
  if (blit.srcOffsets[1].z < 1)
    blit.srcOffsets[1].z = 1;

  if (rsrc)
  {
    blit.srcOffsets[0].x = rsrc->left;
    blit.srcOffsets[0].y = rsrc->top;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = min((uint32_t)rsrc->right, srcExt.width);
    blit.srcOffsets[1].y = min((uint32_t)rsrc->bottom, srcExt.height);
  }
  else
  {
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
  }

  VkExtent3D dstExt = dstTex->pars.getMipExtent(0);
  blit.dstOffsets[1] = toOffset(dstExt);
  if (blit.dstOffsets[1].z < 1)
    blit.dstOffsets[1].z = 1;

  if (rdst)
  {
    blit.dstOffsets[0].x = rdst->left;
    blit.dstOffsets[0].y = rdst->top;
    blit.dstOffsets[0].z = 0;

    // if user wants to stretch over image boundary, do that by reducing src rect size and clamping dst one
    blit.dstOffsets[1].x = min((uint32_t)rdst->right, dstExt.width);
    blit.dstOffsets[1].y = min((uint32_t)rdst->bottom, dstExt.height);

    if (rdst->right > dstExt.width)
    {
      int fullDstSz = rdst->right - rdst->left;
      int clampDstSz = blit.dstOffsets[1].x - blit.dstOffsets[0].x;
      int srcSz = blit.srcOffsets[1].x - blit.srcOffsets[0].x;
      blit.srcOffsets[1].x = blit.srcOffsets[0].x + srcSz * clampDstSz / fullDstSz;
    }

    if (rdst->bottom > dstExt.height)
    {
      int fullDstSz = rdst->bottom - rdst->top;
      int clampDstSz = blit.dstOffsets[1].y - blit.dstOffsets[0].y;
      int srcSz = blit.srcOffsets[1].y - blit.srcOffsets[0].y;
      blit.srcOffsets[1].y = blit.srcOffsets[0].y + srcSz * clampDstSz / fullDstSz;
    }
  }
  else
  {
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
  }

  Globals::ctx.dispatchCmd<CmdBlitImage>({srcImg, dstImg, blit, /*whole_subres*/ rdst == nullptr});
  return true;
}
