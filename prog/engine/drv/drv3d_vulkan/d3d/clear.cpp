// Copyright (C) Gaijin Games KFT.  All rights reserved.

// resource clear methods implementation
#include <drv/3d/dag_rwResource.h>
#include "texture.h"
#include "buffer.h"
#include "globals.h"
#include "device_context.h"
#include <util/dag_compilerDefs.h>

using namespace drv3d_vulkan;

#define CAST_AND_RETURN_IF_NULL(dst, src, cast, dst_type) \
  if (DAGOR_UNLIKELY(!src))                               \
    return true;                                          \
  dst_type dst = cast(src);

G_STATIC_ASSERT(sizeof(VkClearColorValue) == sizeof(unsigned) * 4);

bool d3d::clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level)
{
  CAST_AND_RETURN_IF_NULL(texture, tex, cast_to_texture_base, BaseTex *);

  Image *image = texture->getDeviceImage();
  VkClearColorValue ccv;
  memcpy(&ccv, val, sizeof(ccv));
  VkImageSubresourceRange area = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 1, face, 1);
  Globals::ctx.clearColorImage(image, area, ccv);
  return true;
}

bool d3d::clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level)
{
  CAST_AND_RETURN_IF_NULL(texture, tex, cast_to_texture_base, BaseTex *);

  Image *image = texture->getDeviceImage();
  VkClearColorValue ccv;
  memcpy(&ccv, val, sizeof(ccv));
  VkImageSubresourceRange area = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 1, face, 1);
  Globals::ctx.clearColorImage(image, area, ccv);
  return true;
}

bool d3d::clear_rwbufi(Sbuffer *buffer, const unsigned values[4])
{
  CAST_AND_RETURN_IF_NULL(vbuf, buffer, (GenericBufferInterface *), GenericBufferInterface *);
  G_ASSERT(buffer->getFlags() & SBCF_BIND_UNORDERED);

  CmdClearBufferInt cmd{vbuf->getBufferRef()};
  memcpy(cmd.values, values, sizeof(cmd.values));
  Globals::ctx.dispatchCommand(cmd);
  return true;
}

bool d3d::clear_rwbuff(Sbuffer *buffer, const float values[4])
{
  CAST_AND_RETURN_IF_NULL(vbuf, buffer, (GenericBufferInterface *), GenericBufferInterface *);
  G_ASSERT(buffer->getFlags() & SBCF_BIND_UNORDERED);

  CmdClearBufferFloat cmd{vbuf->getBufferRef()};
  memcpy(cmd.values, values, sizeof(cmd.values));
  Globals::ctx.dispatchCommand(cmd);
  return true;
}

bool d3d::clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val)
{
  CAST_AND_RETURN_IF_NULL(texture, rt.tex, cast_to_texture_base, BaseTex *);

  Image *image = texture->getDeviceImage();
  VkImageAspectFlags imgAspect = texture->getFormat().getAspektFlags();
  VkImageSubresourceRange area = makeImageSubresourceRange(imgAspect, rt.mip_level, 1, rt.layer, 1);

  if (imgAspect & VK_IMAGE_ASPECT_DEPTH_BIT)
  {
    VkClearDepthStencilValue cdsv{};
    cdsv.depth = clear_val.asDepth;
    cdsv.stencil = clear_val.asStencil;
    Globals::ctx.clearDepthStencilImage(image, area, cdsv);
  }
  else if (texture->cflg & TEXCF_RTARGET)
  {
    VkClearColorValue ccv;
    memcpy(&ccv, &clear_val, sizeof(clear_val));
    Globals::ctx.clearColorImage(image, area, ccv);
  }
  else
  {
    D3D_ERROR("Non RenderTarget texture for clear_rt, %p <%s>", rt.tex, rt.tex->getResName());
  }
  return true;
}

#undef CAST_AND_RETURN_IF_NULL
