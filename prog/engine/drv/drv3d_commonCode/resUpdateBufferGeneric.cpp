// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resUpdateBufferGeneric.h"
#include "drv_log_defs.h"
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>

rubgeneric::ResUpdateBuffer *rubgeneric::allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip,
  unsigned dest_slice, unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth)
{
  TextureInfo ti;
  dest_base_texture->getinfo(ti, dest_mip);

  int cflg = (ti.cflg & (TEXFMT_MASK | TEXCF_SRGBWRITE | TEXCF_SRGBREAD)) | TEXCF_SYSMEM | TEXCF_WRITEONLY;

  unsigned texFmt = ti.cflg & TEXFMT_MASK;
  const TextureFormatDesc &fmtInfo = get_tex_format_desc(texFmt);

  uint32_t targetWidth = ti.w;
  uint32_t targetHeight = ti.h;
  uint32_t targetDepth = ti.d;
  if (fmtInfo.isBlockFormat)
  {
    G_ASSERT_RETURN(0 == offset_x % fmtInfo.elementWidth, nullptr);
    G_ASSERT_RETURN(0 == offset_y % fmtInfo.elementHeight, nullptr);
    G_ASSERT_RETURN(0 == width % fmtInfo.elementWidth, nullptr);
    G_ASSERT_RETURN(0 == height % fmtInfo.elementHeight, nullptr);
    // getinfo may yield incorrectly calculated values for block compressed formats
    targetWidth = max<uint32_t>(targetWidth, fmtInfo.elementWidth);
    targetHeight = max<uint32_t>(targetHeight, fmtInfo.elementHeight);
  }

  G_ASSERT_RETURN(offset_x < targetWidth, nullptr);
  G_ASSERT_RETURN(offset_y < targetHeight, nullptr);
  G_ASSERT_RETURN(offset_z < targetDepth, nullptr);
  G_ASSERT_RETURN(offset_x + width <= targetWidth, nullptr);
  G_ASSERT_RETURN(offset_y + height <= targetHeight, nullptr);
  G_ASSERT_RETURN(offset_z + depth <= targetDepth, nullptr);

  bool updateDirectly = d3d::is_in_device_reset_now();
  if (d3d::get_driver_code().is(d3d::metal))
    updateDirectly = true;

  BaseTexture *stagingTex = nullptr;
  char *dst = nullptr;
  int dstPitch = 0, slicePitch = 0;
  if (updateDirectly)
  {
    stagingTex = dest_base_texture;
    bool ret = false;
    switch (ti.resType)
    {
      case RES3D_VOLTEX:
        ret = stagingTex->lockbox((void **)&dst, dstPitch, slicePitch, dest_mip, TEXLOCK_WRITE);
        G_ASSERT(slicePitch == dstPitch * ti.h / fmtInfo.elementHeight);
        break;
      case RES3D_ARRTEX:
      case RES3D_CUBETEX:
      case RES3D_CUBEARRTEX: ret = stagingTex->lockimg((void **)&dst, dstPitch, dest_slice, dest_mip, TEXLOCK_WRITE); break;
      default: ret = stagingTex->lockimg((void **)&dst, dstPitch, dest_mip, TEXLOCK_WRITE); break;
    }
    if (!ret || !dst)
    {
      if (!d3d::is_in_device_reset_now())
        D3D_ERROR("failed to lock RUB.staging %dx%d,L%d cfg=0x%x %s", ti.w, ti.h, 1, ti.cflg, dest_base_texture->getTexName());
      return nullptr;
    }
    dst += fmtInfo.bytesPerElement * offset_x / fmtInfo.elementWidth;
    dst += dstPitch * offset_y / fmtInfo.elementHeight;
    dst += slicePitch * offset_z;
  }
  else
  {
    bool voltex = (ti.resType == RES3D_VOLTEX);
    stagingTex = voltex ? d3d::create_voltex(width, height, depth, cflg, 1) : d3d::create_tex(nullptr, width, height, cflg, 1);
    if (!stagingTex)
    {
      if (!d3d::is_in_device_reset_now())
      {
        D3D_ERROR("failed to create RUB.staging %dx%d,L%d cfg=0x%x", ti.w, ti.h, 1, ti.cflg);
      }
      return nullptr;
    }
    stagingTex->allocateTex();

    if ((voltex && (!stagingTex->lockbox((void **)&dst, dstPitch, slicePitch, 0, TEXLOCK_WRITE) || !dst)) ||
        (!voltex && (!stagingTex->lockimg((void **)&dst, dstPitch, 0, TEXLOCK_WRITE) || !dst)))
    {
      if (!d3d::is_in_device_reset_now())
      {
        D3D_ERROR("failed to lock RUB.staging %dx%d,L%d cfg=0x%x %s", ti.w, ti.h, 1, ti.cflg, dest_base_texture->getTexName());
      }
      del_d3dres(stagingTex);
      return nullptr;
    }
  }
  // debug("%s: update(%s) %d:%d lock %dx%d dst=%p dstPitch=%d (slicePitch=%d)", dest_base_texture->getTexName(),
  //   stagingTex == dest_base_texture ? "direct" : "blit", dest_mip, dest_slice, ti.w, ti.h, dst, dstPitch, slicePitch);

  ResUpdateBuffer *rub = (ResUpdateBuffer *)memalloc(sizeof(ResUpdateBuffer), tmpmem);
  rub->staging = stagingTex;
  rub->pitch = dstPitch;
  rub->slicePitch = slicePitch ? slicePitch : (dstPitch * ti.h / fmtInfo.elementHeight);
  rub->ptr = dst;
  rub->size = rub->slicePitch * depth;
  rub->x = offset_x;
  rub->y = offset_y;
  rub->z = offset_z;
  rub->w = width;
  rub->h = height;
  rub->d = depth;
  rub->cflg = ti.cflg;
  rub->destTex = dest_base_texture;
  rub->destMip = dest_mip;
  rub->destSlice = dest_slice;
  return rub;
}

rubgeneric::ResUpdateBuffer *rubgeneric::allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice)
{
  TextureInfo ti;
  dest_tex->getinfo(ti, dest_mip);
  int cflg = (ti.cflg & (TEXFMT_MASK | TEXCF_SRGBWRITE | TEXCF_SRGBREAD)) | TEXCF_SYSMEM | TEXCF_WRITEONLY;
  unsigned tex_fmt = ti.cflg & TEXFMT_MASK;
  bool is_block_fmt = tex_fmt == TEXFMT_DXT1 || tex_fmt == TEXFMT_DXT3 || tex_fmt == TEXFMT_DXT5 || tex_fmt == TEXFMT_BC7 ||
                      tex_fmt == TEXFMT_BC6H || tex_fmt == TEXFMT_ATI1N || tex_fmt == TEXFMT_ATI2N;

  if (is_block_fmt && (ti.w < 4 || ti.h < 4))
  {
    ti.w = max<uint16_t>(ti.w, 4);
    ti.h = max<uint16_t>(ti.h, 4);
  }

  bool update_directly = d3d::is_in_device_reset_now();
  if (d3d::get_driver_code().is(d3d::metal))
    update_directly = true;

  BaseTexture *staging_tex = nullptr;
  char *dst = nullptr;
  int dst_pitch = 0, slice_pitch = 0;
  if (update_directly)
  {
    staging_tex = dest_tex;
    bool ret = false;
    switch (dest_tex->restype())
    {
      case RES3D_VOLTEX:
        ret = staging_tex->lockbox((void **)&dst, dst_pitch, slice_pitch, dest_mip, TEXLOCK_WRITE);
        G_ASSERT(slice_pitch == dst_pitch * (is_block_fmt ? ti.h / 4 : ti.h));
        break;
      case RES3D_ARRTEX:
      case RES3D_CUBETEX:
      case RES3D_CUBEARRTEX: ret = staging_tex->lockimg((void **)&dst, dst_pitch, dest_slice, dest_mip, TEXLOCK_WRITE); break;
      default: ret = staging_tex->lockimg((void **)&dst, dst_pitch, dest_mip, TEXLOCK_WRITE); break;
    }
    if (!ret || !dst)
    {
      if (!d3d::is_in_device_reset_now())
        D3D_ERROR("failed to lock RUB.staging %dx%d,L%d cfg=0x%x %s", ti.w, ti.h, 1, ti.cflg, dest_tex->getTexName());
      return nullptr;
    }
    if (RES3D_VOLTEX == dest_tex->restype())
    {
      dst += slice_pitch * dest_slice;
    }
  }
  else
  {
    bool voltex = (dest_tex->restype() == RES3D_VOLTEX);
    staging_tex = voltex ? d3d::create_voltex(ti.w, ti.h, 1, cflg, 1) : d3d::create_tex(nullptr, ti.w, ti.h, cflg, 1);
    if (!staging_tex)
    {
      if (!d3d::is_in_device_reset_now())
        D3D_ERROR("failed to create RUB.staging %dx%d,L%d cfg=0x%x", ti.w, ti.h, 1, ti.cflg);
      return nullptr;
    }
    staging_tex->allocateTex();

    if ((voltex && (!staging_tex->lockbox((void **)&dst, dst_pitch, slice_pitch, 0, TEXLOCK_WRITE) || !dst)) ||
        (!voltex && (!staging_tex->lockimg((void **)&dst, dst_pitch, 0, TEXLOCK_WRITE) || !dst)))
    {
      if (!d3d::is_in_device_reset_now())
        D3D_ERROR("failed to lock RUB.staging %dx%d,L%d cfg=0x%x %s", ti.w, ti.h, 1, ti.cflg, dest_tex->getTexName());
      del_d3dres(staging_tex);
      return nullptr;
    }
  }
  // debug("%s: update(%s) %d:%d lock %dx%d dst=%p dst_pitch=%d (slice_pitch=%d)", dest_tex->getTexName(),
  //   staging_tex == dest_tex ? "direct" : "blit", dest_mip, dest_slice, ti.w, ti.h, dst, dst_pitch, slice_pitch);

  ResUpdateBuffer *rub = (ResUpdateBuffer *)memalloc(sizeof(ResUpdateBuffer), tmpmem);
  rub->staging = staging_tex;
  rub->pitch = dst_pitch;
  rub->slicePitch = dst_pitch * (is_block_fmt ? ti.h / 4 : ti.h);
  rub->ptr = dst;
  rub->size = rub->slicePitch;
  rub->x = 0;
  rub->y = 0;
  rub->z = (dest_tex->restype() == RES3D_VOLTEX) ? dest_slice : 0;
  rub->w = ti.w;
  rub->h = ti.h;
  rub->d = 1;
  rub->cflg = ti.cflg;
  rub->destTex = dest_tex;
  rub->destMip = dest_mip;
  rub->destSlice = (dest_tex->restype() == RES3D_VOLTEX) ? 0 : dest_slice;
  return rub;
}

static void unlock_update_buffer(rubgeneric::ResUpdateBuffer *rub)
{
  if (!rub || !rub->staging || !rub->ptr)
    return;
  bool voltex = (rub->staging->restype() == RES3D_VOLTEX);
  if ((voltex && !rub->staging->unlockbox()) || (!voltex && !rub->staging->unlockimg()))
    if (!d3d::is_in_device_reset_now())
      D3D_ERROR("failed to unlock RUB.staging");
  rub->ptr = nullptr;
  rub->size = 0;
}
void rubgeneric::release_update_buffer(rubgeneric::ResUpdateBuffer *&rub)
{
  if (rub)
  {
    unlock_update_buffer(rub);
    if (rub->staging == rub->destTex)
      rub->staging = nullptr;
    else
      del_d3dres(rub->staging);

    rub->ptr = nullptr;
    rub->size = 0;
    rub->pitch = 0;
    rub->destTex = nullptr;
    rub->destMip = rub->destSlice = 0;
    memfree(rub, tmpmem); // release update buffer
    rub = nullptr;
  }
}

bool rubgeneric::update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub)
{
  if (!src_rub)
    return false;

  unlock_update_buffer(src_rub);
  if (src_rub->staging != src_rub->destTex)
  {
    G_ASSERT_RETURN(src_rub->staging, false);

    bool voltex = (src_rub->staging->restype() == RES3D_VOLTEX);
    BaseTexture *dest = src_rub->destTex;
    int dest_mip = src_rub->destMip;
    int dest_slice = src_rub->destSlice;
    G_ASSERTF_RETURN((dest->restype() == RES3D_VOLTEX) == voltex, false, "dest->type=%d staging->type=%d", dest->restype(),
      src_rub->staging->restype());
    if (!dest->updateSubRegion(/*src*/ src_rub->staging, 0, 0, 0, 0, /*size*/ src_rub->w, src_rub->h, src_rub->d,
          /*dest*/ dest->calcSubResIdx(dest_mip, dest_slice), src_rub->x, src_rub->y, src_rub->z))
      if (!d3d::is_in_device_reset_now())
        D3D_ERROR("%s: updateSubRegion(%dx%d) -> %d:%d failed", dest->getTexName(), src_rub->w, src_rub->h, dest_mip, dest_slice);
  }
  release_update_buffer(src_rub);
  return true;
}
