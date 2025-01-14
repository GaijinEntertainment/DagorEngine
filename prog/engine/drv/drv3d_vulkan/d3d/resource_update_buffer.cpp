// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_resUpdateBuffer.h>

#include "buffer_resource.h"
#include "texture.h"
#include <osApiWrappers/dag_miscApi.h>
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "resource_upload_limit.h"
#include "frontend.h"
#include "driver_config.h"
#include "device_context.h"

using namespace drv3d_vulkan;

struct ResUpdateBufferImp
{
  BaseTex *destTex;
  Image *originalImg;
  Buffer *stagingBuffer;
  VkBufferImageCopy uploadInfo;
  size_t pitch;
  size_t slicePitch;
};

d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip,
  unsigned dest_slice, unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth)
{
  G_ASSERT_RETURN(dest_base_texture, nullptr);

  BaseTex *dest_tex = cast_to_texture_base(dest_base_texture);

  TextureInfo base_ti;
  dest_tex->getinfo(base_ti, 0);

  G_ASSERT_RETURN(dest_mip < base_ti.mipLevels, nullptr);
  G_ASSERT_RETURN(dest_slice < base_ti.a, nullptr);

  FormatStore fmt = FormatStore::fromCreateFlags(base_ti.cflg & TEXFMT_MASK);

  uint32_t bx, by;
  fmt.getBytesPerPixelBlock(&bx, &by);
  auto mipW = max<uint32_t>(base_ti.w >> dest_mip, bx);
  auto mipH = max<uint32_t>(base_ti.h >> dest_mip, by);
  auto mipD = max<uint32_t>(base_ti.d >> dest_mip, 1);
  uint32_t slicePitch = fmt.calculateSlicePich(width, height);
  uint32_t size = slicePitch * depth;

  if ((offset_x % bx) || (offset_y % by) || (width % bx) || (height % by) || (offset_x >= mipW) || (offset_y >= mipH) ||
      (offset_z >= mipD) || (offset_x + width > mipW) || (offset_y + height > mipH) || (offset_z + depth > mipD) || (size == 0))
  {
    D3D_ERROR(
      "vulkan: RUB region is out of bounds: asked (%u, %u, %u)-(%u, %u, %u) for tex mip %u size (%u, %u, %u) block (%u, %u) sz "
      "%u name %s",
      offset_x, offset_y, offset_z, offset_x + width, offset_y + height, offset_z + depth, dest_mip, mipW, mipH, mipD, bx, by, size,
      dest_tex->getDeviceImage()->getDebugName());
    return nullptr;
  }

  if (!Frontend::resUploadLimit.consume(size, dest_base_texture))
    return nullptr;

  Buffer *stagingBuffer = Buffer::create(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::NONE);
  if (Globals::cfg.debugLevel)
    stagingBuffer->setDebugName(String(64, "RUB for %s", dest_tex->getDeviceImage()->getDebugName()));

  ResUpdateBufferImp *rub = (ResUpdateBufferImp *)memalloc(sizeof(ResUpdateBufferImp), tmpmem);

  rub->destTex = dest_tex;
  rub->originalImg = dest_tex->getDeviceImage();
  rub->stagingBuffer = stagingBuffer;
  rub->pitch = fmt.calculateRowPitch(width);
  rub->slicePitch = slicePitch;
  rub->uploadInfo =
    make_copy_info(fmt, dest_mip, dest_slice, 1, {uint32_t(width), uint32_t(height), uint32_t(depth)}, stagingBuffer->bufOffsetLoc(0));
  rub->uploadInfo.imageOffset.x = offset_x;
  rub->uploadInfo.imageOffset.y = offset_y;
  rub->uploadInfo.imageOffset.z = offset_z;

  return (d3d::ResUpdateBuffer *)rub;
}

d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex(BaseTexture *dest_base_texture, int dest_mip, int dest_slice)
{
  G_ASSERT(dest_base_texture);
  BaseTex *dest_tex = cast_to_texture_base(dest_base_texture);

  TextureInfo base_ti;
  dest_tex->getinfo(base_ti, 0);

  uint16_t level_w = max<uint16_t>(1u, base_ti.w >> dest_mip);
  uint16_t level_h = max<uint16_t>(1u, base_ti.h >> dest_mip);

  FormatStore fmt = FormatStore::fromCreateFlags(base_ti.cflg & TEXFMT_MASK);

  uint32_t size = fmt.calculateSlicePich(level_w, level_h);
  G_ASSERT(size != 0);

  if (!Frontend::resUploadLimit.consume(size, dest_base_texture))
    return nullptr;

  Buffer *stagingBuffer = Buffer::create(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::TEMP);
  if (Globals::cfg.debugLevel)
    stagingBuffer->setDebugName(String(64, "RUB for %s", dest_tex->getDeviceImage()->getDebugName()));

  ResUpdateBufferImp *rub = (ResUpdateBufferImp *)memalloc(sizeof(ResUpdateBufferImp), tmpmem);

  rub->destTex = dest_tex;
  rub->originalImg = dest_tex->getDeviceImage();
  rub->stagingBuffer = stagingBuffer;
  rub->pitch = fmt.calculateRowPitch(level_w);
  rub->slicePitch = size;

  switch (dest_tex->restype())
  {
    case RES3D_VOLTEX:
    {
      rub->uploadInfo = make_copy_info(fmt, dest_mip, 0, 1, {base_ti.w, base_ti.h, 1}, stagingBuffer->bufOffsetLoc(0));
      rub->uploadInfo.imageOffset.z = dest_slice;
      break;
    }
    default:
    {
      rub->uploadInfo = make_copy_info(fmt, dest_mip, dest_slice, 1, {base_ti.w, base_ti.h, 1}, stagingBuffer->bufOffsetLoc(0));
      break;
    }
  }

  return (d3d::ResUpdateBuffer *)rub;
}

void d3d::release_update_buffer(d3d::ResUpdateBuffer *&rub)
{
  if (ResUpdateBufferImp *&rub_imp = reinterpret_cast<ResUpdateBufferImp *&>(rub))
  {
    drv3d_vulkan::Globals::ctx.destroyBuffer(rub_imp->stagingBuffer);
    memfree(rub_imp, tmpmem); // release update buffer
    rub = nullptr;
  }
}

char *d3d::get_update_buffer_addr_for_write(d3d::ResUpdateBuffer *rub)
{
  if (ResUpdateBufferImp *rub_imp = reinterpret_cast<ResUpdateBufferImp *>(rub))
  {
    G_ASSERT(rub_imp->stagingBuffer->hasMappedMemory());
    return reinterpret_cast<char *>(rub_imp->stagingBuffer->ptrOffsetLoc(0));
  }
  return nullptr;
}

size_t d3d::get_update_buffer_size(d3d::ResUpdateBuffer *rub)
{
  return rub ? ((ResUpdateBufferImp *)rub)->stagingBuffer->getBlockSize() : 0;
}

size_t d3d::get_update_buffer_pitch(d3d::ResUpdateBuffer *rub) { return rub ? ((ResUpdateBufferImp *)rub)->pitch : 0; }

size_t d3d::get_update_buffer_slice_pitch(d3d::ResUpdateBuffer *rub)
{
  return rub ? reinterpret_cast<ResUpdateBufferImp *>(rub)->slicePitch : 0;
}

bool d3d::update_texture_and_release_update_buffer(d3d::ResUpdateBuffer *&rub)
{
  if (!rub)
    return false;

  ResUpdateBufferImp *&rub_imp = reinterpret_cast<ResUpdateBufferImp *&>(rub);

  rub_imp->stagingBuffer->markNonCoherentRangeLoc(0, rub_imp->stagingBuffer->getBlockSize(), true);

  if (rub_imp->destTex->tex.image != rub_imp->originalImg)
    D3D_ERROR("vulkan: image changed between RUB allocation and update. original %p:%s current %p:%s", rub_imp->originalImg,
      rub_imp->originalImg ? rub_imp->originalImg->getDebugName() : "<null>", rub_imp->destTex->tex.image,
      rub_imp->destTex->tex.image ? rub_imp->destTex->tex.image->getDebugName() : "<null>");

  drv3d_vulkan::Globals::ctx.copyBufferToImage(rub_imp->stagingBuffer, rub_imp->destTex->tex.image, 1, &rub_imp->uploadInfo, true);

  d3d::release_update_buffer(rub);
  return true;
}
