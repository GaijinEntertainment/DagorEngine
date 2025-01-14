// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../texMgrData.h"
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_info.h>
#include "uniTexCrd.h"
#include "genmip.h"
#include <3d/tql.h>
#include <3d/ddsFormat.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>

// #define RMGR_TRACE debug  // trace resource manager's DDSx loads
#ifndef RMGR_TRACE
#define RMGR_TRACE(...) ((void)0)
#endif

#define TEX_NAME(TEX, TID) TEX ? TEX->getTexName() : get_managed_texture_name(TID)

using texmgr_internal::RMGR;

static void convert_bc7_to_dxt5(void *ref_data, int width, int height, int faces);
static bool convert_16b_to_argb(IGenLoad &crd, char *dst, int dst_pitch, int mip_level, int mip_data_pitch, int mip_data_lines,
  unsigned d3d_fmt);
static bool convert_dxt_to_argb(IGenLoad &crd, char *dst, int dst_pitch, int mip_data_sz, int mip_data_w, int mip_data_h,
  unsigned d3d_fmt);

#include "genmip.inc.cpp"

static bool load_ddsx_slice(IGenLoad &crd, char *dst, int dst_pitch, int mip_level, int mip_data_pitch, int mip_data_lines,
  int mip_data_w, int mip_data_h, unsigned d3d_fmt, int cflg, BaseTexture *t)
{
  if (dst_pitch == 0)
  {
    crd.seekrel(mip_data_pitch * mip_data_lines);
    return true;
  }
  if (
    (cflg & TEXFMT_MASK) == TEXFMT_A8R8G8B8 && (d3d_fmt == D3DFMT_A4R4G4B4 || d3d_fmt == D3DFMT_X4R4G4B4 || d3d_fmt == D3DFMT_R5G6B5))
    return convert_16b_to_argb(crd, dst, dst_pitch, mip_level, mip_data_pitch, mip_data_lines, d3d_fmt);

  if ((cflg & TEXFMT_MASK) == TEXFMT_A8R8G8B8 && (d3d_fmt == D3DFMT_DXT1 || d3d_fmt == D3DFMT_DXT5))
    return convert_dxt_to_argb(crd, dst, dst_pitch, mip_data_pitch * mip_data_lines, mip_data_w, mip_data_h, d3d_fmt);

  G_ASSERTF_RETURN(dst_pitch >= mip_data_pitch, false, "dst_pitch=%d < mip_data_pitch=%d (mip=%d %dx%d, %db x %dr) d3dFormat=0x%x %s",
    dst_pitch, mip_data_pitch, mip_level, mip_data_w, mip_data_h, mip_data_pitch, mip_data_lines, d3d_fmt, t->getTexName());
  G_UNUSED(t);

  if ((cflg & TEXFMT_MASK) == TEXFMT_DXT5 && d3d_fmt == _MAKE4C('BC7 '))
  {
    if (dst_pitch == mip_data_pitch)
    {
      crd.readExact(dst, mip_data_pitch * mip_data_lines);
      convert_bc7_to_dxt5(dst, mip_data_w, mip_data_h, 1);
    }
    else
    {
      char *tmpdst = (char *)memalloc(mip_data_pitch * mip_data_lines, tmpmem);
      crd.readExact(tmpdst, mip_data_pitch * mip_data_lines);
      convert_bc7_to_dxt5(tmpdst, mip_data_w, mip_data_h, 1);
      if (dst_pitch > mip_data_pitch)
        for (char *src = tmpdst, *src_e = src + mip_data_lines * mip_data_pitch; src < src_e; dst += dst_pitch, src += mip_data_pitch)
          memcpy(dst, src, mip_data_pitch);
      memfree(tmpdst, tmpmem);
    }
    return true;
  }

  if (dst_pitch == mip_data_pitch)
    crd.readExact(dst, mip_data_pitch * mip_data_lines);
  else if (dst_pitch > mip_data_pitch)
    for (int hi = mip_data_lines; hi > 0; hi--, dst += dst_pitch)
      crd.readExact(dst, mip_data_pitch);
  return true;
}

static TexLoadRes load_tex_data_2d(BaseTexture *tex, TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr, IGenLoad &crd,
  int start_lev, int rd_lev, int skip_lev, unsigned cflg, int q_id, int dest_slice, on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb)
{
  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
  {
    uint8_t *baseTexData = NULL;
    int baseTexFmt = 0;

    if (hdr.flags & hdr.FLG_NEED_PAIRED_BASETEX)
    {
      G_ASSERTF(hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_DXT5, "%s: hdr.d3dFormat=0x%x", TEX_NAME(tex, tid),
        hdr.d3dFormat);
      G_ASSERTF(paired_tid != BAD_TEXTUREID && RMGR.toIndex(paired_tid) >= 0 && RMGR.getBaseTexRc(paired_tid) > 0,
        "%s: paired_tid=0x%x btRc=%d", TEX_NAME(tex, tid), paired_tid, RMGR.getBaseTexRc(paired_tid));

      auto *base_data = RMGR.getTexBaseData(paired_tid);
      dag::Span<uint8_t> ddsx_data(base_data ? base_data->data : nullptr, base_data ? base_data->sz : 0);

#if DAGOR_DBGLEVEL > 0
      debug("%p(%s) needs 0x%x(%s) (%p,%d)", tex, TEX_NAME(tex, tid), paired_tid, get_managed_texture_name(paired_tid),
        ddsx_data.data(), data_size(ddsx_data));
#endif

      G_ASSERTF_RETURN(base_data, TexLoadRes::ERR, "%s needs %s", TEX_NAME(tex, tid), get_managed_texture_name(paired_tid));
      G_ASSERTF(base_data->hdr.d3dFormat == D3DFMT_DXT1 || base_data->hdr.d3dFormat == D3DFMT_DXT5,
        "%s: hdr.d3dFormat=0x%x and base_data->hdr.d3dFormat=0x%x (%s)", TEX_NAME(tex, tid), hdr.d3dFormat, base_data->hdr.d3dFormat,
        RMGR.getName(paired_tid.index()));
      G_ASSERTF(base_data->hdr.w == hdr.w || base_data->hdr.h == hdr.h, "%s: hdr=%dx%d and base_data->hdr=%dx%d (%s)",
        TEX_NAME(tex, tid), hdr.w, hdr.h, base_data->hdr.w, base_data->hdr.h, RMGR.getName(paired_tid.index()));

      baseTexData = ddsx_data.data();
      baseTexFmt = base_data->hdr.d3dFormat;
    }

    int q_type = DXT_ALGORITHM_EXCELLENT;
    if (q_id == 1)
      q_type = DXT_ALGORITHM_PRODUCTION;
    else if (q_id == 2)
      q_type = DXT_ALGORITHM_PRECISE;

    AutoAcquireMem mem;
    if (hdr.flags & hdr.FLG_GENMIP_BOX)
    {
      // debug("genmip BOX %d", q_id);
      if (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_DXT5 || hdr.d3dFormat == D3DFMT_DXT3)
      {
        BoxFilter filter;
        return create_dxt_mip_chain(tex, tid, baseTexData, baseTexFmt, hdr, crd, hdr.d3dFormat, start_lev, rd_lev, skip_lev,
          dest_slice, filter, q_type);
      }
      else if (hdr.d3dFormat == D3DFMT_A8R8G8B8 || hdr.d3dFormat == D3DFMT_A8B8G8R8 || hdr.d3dFormat == D3DFMT_X8R8G8B8)
      {
        BoxFilter filter;
        return create_rgba8_mip_chain(tex, hdr, crd, start_lev, rd_lev, skip_lev, dest_slice, filter);
      }
    }
    else
    {
      // debug("genmip KZR %d", q_id);
      if (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_DXT5 || hdr.d3dFormat == D3DFMT_DXT3)
      {
        KaiserFilter filter(4.0f, 1.0f);
        return create_dxt_mip_chain(tex, tid, baseTexData, baseTexFmt, hdr, crd, hdr.d3dFormat, start_lev, rd_lev, skip_lev,
          dest_slice, filter, q_type);
      }
      else if (hdr.d3dFormat == D3DFMT_A8R8G8B8 || hdr.d3dFormat == D3DFMT_A8B8G8R8 || hdr.d3dFormat == D3DFMT_X8R8G8B8)
      {
        KaiserFilter filter(4.0f, 1.0f);
        return create_rgba8_mip_chain(tex, hdr, crd, start_lev, rd_lev, skip_lev, dest_slice, filter);
      }
    }
    logerr("%s: unsupported hdr.d3dFormat=0x%08X for GENMIP", TEX_NAME(tex, tid), hdr.d3dFormat);
    return TexLoadRes::ERR;
  }

  DDSX_LD_ITERATE_MIPS_START(crd, hdr, skip_lev, start_lev, rd_lev, 1, 1)
  d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, tex_level, dest_slice);
  if (!rub)
    return TexLoadRes::ERR_RUB;
  if (!load_ddsx_slice(crd, d3d::get_update_buffer_addr_for_write(rub), d3d::get_update_buffer_pitch(rub), src_level, mip_data_pitch,
        mip_data_lines, mip_data_w, mip_data_h, hdr.d3dFormat, cflg, tex))
  {
    d3d::release_update_buffer(rub);
    return TexLoadRes::ERR;
  }
  if (on_tex_slice_loaded_cb)
    (*on_tex_slice_loaded_cb)();
  if (!d3d::update_texture_and_release_update_buffer(rub))
  {
    d3d::release_update_buffer(rub);
    return TexLoadRes::ERR;
  }
  DDSX_LD_ITERATE_MIPS_END()

  return TexLoadRes::OK;
}

static TexLoadRes load_ddsx_faces_2d(BaseTexture *tex, TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr, IGenLoad &crd,
  int faces, int start_lev, int rd_lev, int skip_lev, unsigned cflg, int q_id, on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb)
{
  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER)) // generating mips
  {
    G_ASSERTF_RETURN(faces == 1, TexLoadRes::ERR, "%s: faces=%d", TEX_NAME(tex, tid), faces);
    return load_tex_data_2d(tex, tid, paired_tid, hdr, crd, start_lev, rd_lev, skip_lev, cflg, q_id, 0, on_tex_slice_loaded_cb);
  }

  if (hdr.flags & hdr.FLG_CUBTEX)
  {
    G_ASSERTF_RETURN(faces == 6, TexLoadRes::ERR, "%s: faces=%d for FLG_CUBTEX", TEX_NAME(tex, tid), faces);

    DDSX_LD_ITERATE_MIPS_START(crd, hdr, skip_lev, start_lev, rd_lev, 1, faces)
    for (int face_idx = 0; face_idx < mip_data_depth; face_idx++)
    {
      d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, tex_level, face_idx);
      if (!rub)
        return TexLoadRes::ERR_RUB;
      if (!load_ddsx_slice(crd, d3d::get_update_buffer_addr_for_write(rub), d3d::get_update_buffer_pitch(rub), src_level,
            mip_data_pitch, mip_data_lines, mip_data_w, mip_data_h, hdr.d3dFormat, cflg, tex))
      {
        d3d::release_update_buffer(rub);
        return TexLoadRes::ERR;
      }
      if (on_tex_slice_loaded_cb)
        (*on_tex_slice_loaded_cb)();
      if (!d3d::update_texture_and_release_update_buffer(rub))
      {
        d3d::release_update_buffer(rub);
        return TexLoadRes::ERR;
      }
    }
    DDSX_LD_ITERATE_MIPS_END()
    return TexLoadRes::OK;
  }

  return load_tex_data_2d(tex, tid, paired_tid, hdr, crd, start_lev, rd_lev, skip_lev, cflg, q_id, 0, on_tex_slice_loaded_cb);
}

static TexLoadRes load_ddsx_faces_3d(BaseTexture *tex, const ddsx::Header &hdr, IGenLoad &crd, int start_lev, int rd_lev, int skip_lev,
  unsigned cflg, on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb)
{
  DDSX_LD_ITERATE_MIPS_START(crd, hdr, skip_lev, start_lev, rd_lev, hdr.depth, 1)
  {
    const bool update_directly =
      d3d::get_driver_code().is(d3d::metal) || (d3d::is_in_device_reset_now() && !d3d::get_driver_code().is(d3d::vulkan));

    d3d::ResUpdateBuffer *rub_slice = !update_directly ? nullptr : d3d::allocate_update_buffer_for_tex(tex, tex_level, 0);
    char *rub_data = rub_slice ? d3d::get_update_buffer_addr_for_write(rub_slice) : nullptr;
    for (int slice_idx = 0; slice_idx < mip_data_depth; slice_idx++)
    {
      d3d::ResUpdateBuffer *rub = rub_data ? nullptr : d3d::allocate_update_buffer_for_tex(tex, tex_level, slice_idx);
      if (!rub_data && !rub)
        return TexLoadRes::ERR_RUB;
      char *dst = rub_data ? rub_data : d3d::get_update_buffer_addr_for_write(rub);
      uint32_t dst_pitch = rub_data ? d3d::get_update_buffer_pitch(rub_slice) : d3d::get_update_buffer_pitch(rub);
      if (!load_ddsx_slice(crd, dst, dst_pitch, src_level, mip_data_pitch, mip_data_lines, mip_data_w, mip_data_h, hdr.d3dFormat, cflg,
            tex))
      {
        if (rub)
          d3d::release_update_buffer(rub);
        return TexLoadRes::ERR;
      }
      if (on_tex_slice_loaded_cb)
        (*on_tex_slice_loaded_cb)();
      if (rub_data)
        rub_data += d3d::get_update_buffer_size(rub_slice);
      else if (!d3d::update_texture_and_release_update_buffer(rub))
      {
        d3d::release_update_buffer(rub);
        return TexLoadRes::ERR;
      }
    }
    if (rub_slice)
      if (!d3d::update_texture_and_release_update_buffer(rub_slice))
      {
        d3d::release_update_buffer(rub_slice);
        return TexLoadRes::ERR;
      }
  }
  DDSX_LD_ITERATE_MIPS_END()
  return TexLoadRes::OK;
}

bool isDiffuseTextureSuitableForMipColorization(const char *name);

static TexLoadRes load_ddsx_tex_contents(BaseTexture *tex, TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr, IGenLoad &crd,
  int q_id, int start_lev, unsigned tex_ld_lev, on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb, bool tex_props_inited)
{
  TextureInfo ti;
  tex->getinfo(ti, 0);

  int rd_lev = ti.mipLevels, skip_lev = 0;
  if (!tql::updateSkipLev(hdr, rd_lev, skip_lev, max(ti.w >> start_lev, 1), max(ti.h >> start_lev, 1), max(ti.d >> start_lev, 1)))
    return TexLoadRes::OK;

  if (tex_ld_lev > 1)
  {
    unsigned tex_lev = get_log2i(max(max(ti.w, ti.h), ti.d));
    if (tex_lev > tex_ld_lev && rd_lev + start_lev > tex_lev - tex_ld_lev)
      rd_lev = tex_lev - tex_ld_lev - start_lev; // limit rd_level to mips that are not already loaded to texture
  }

  RMGR_TRACE("tex=%p(%s) cflg=0x%08x type=%d %dx%dx%d,L%d fmt=0x%x (to read to %dx%dx%d,L%d)", tex, TEX_NAME(tex, tid), ti.cflg,
    tex ? tex->restype() : RES3D_TEX, max(hdr.w >> skip_lev, 1), max(hdr.h >> skip_lev, 1), max<int>(hdr.depth >> skip_lev, ti.a),
    rd_lev, hdr.d3dFormat, max(ti.w >> start_lev, 1), max(ti.h >> start_lev, 1), max<int>(ti.d >> start_lev, ti.a),
    min(rd_lev, ti.mipLevels - start_lev));
  if (start_lev + rd_lev > ti.mipLevels)
    rd_lev = ti.mipLevels - start_lev;

  UnifiedTexGenLoad ucrd(crd, hdr);
  tex->allocateTex();
  // NOTE: tex_props_inited is used to only read & set header data once when
  // loading a texture gradually (e.g. TQ/BQ first, then HQ/UHQ after some frames)
  // because we do "live patching" of a texture that is already being used for rendering
  // and calling texaddru or other modifying stuff on it would be a data race.
  if (!tex_props_inited && tex->isSamplerEnabled())
  {
    tex->texaddru(hdr.getAddrU());
    tex->texaddrv(hdr.getAddrV());
  }

  if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && tid != BAD_TEXTUREID)
    RMGR.incBaseTexRc(tid);

    // force uncompressed texture for mip vizualisation
#if DAGOR_DBGLEVEL > 0
  if (isDiffuseTextureSuitableForMipColorization(tex->getTexName()) && tex->restype() == RES3D_TEX)
  {
    uint32_t colors[4] = {0x800000ff, 0x8000ff00, 0x80ff0000, 0x8000ffff};
    for (int mip = 0; mip < tex->level_count(); ++mip)
    {
      char *ptr = nullptr;
      int stride = 0;

      TextureInfo info;
      tex->getinfo(info, mip);

      tex->lockimg((void **)&ptr, stride, mip, TEXLOCK_DEFAULT);
      G_ASSERT((stride & 3) == 0);
      for (uint32_t h = 0; h < info.h; ++h)
      {
        uint32_t *img = (uint32_t *)(ptr + h * stride);
        for (uint32_t w = 0; w < info.w; ++w)
          *img++ = colors[mip > 3 ? 3 : mip];
      }

      tex->unlockimg();
    }
    return TexLoadRes::OK;
  }
#endif

  TexLoadRes loadStatus = TexLoadRes::ERR;
  switch (tex->restype())
  {
    case RES3D_TEX:
    case RES3D_CUBETEX:
      loadStatus =
        load_ddsx_faces_2d(tex, tid, paired_tid, hdr, *ucrd, ti.a, start_lev, rd_lev, skip_lev, ti.cflg, q_id, on_tex_slice_loaded_cb);
      break;

    case RES3D_VOLTEX:
      loadStatus = load_ddsx_faces_3d(tex, hdr, *ucrd, start_lev, rd_lev, skip_lev, ti.cflg, on_tex_slice_loaded_cb);
      break;
  }
  if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && tid != BAD_TEXTUREID)
    RMGR.decBaseTexRc(tid);
  return loadStatus;
}

static TexLoadRes load_ddsx_to_slice(BaseTexture *tex, int slice, const ddsx::Header &hdr, IGenLoad &crd, int q_id, int start_lev,
  unsigned tex_ld_lev)
{
  TextureInfo ti;
  tex->getinfo(ti, 0);

  int rd_lev = ti.mipLevels, skip_lev = 0;
  if (!tql::updateSkipLev(hdr, rd_lev, skip_lev, max(ti.w >> start_lev, 1), max(ti.h >> start_lev, 1), max(ti.d >> start_lev, 1)))
    return TexLoadRes::OK;

  if (tex_ld_lev > 1)
  {
    unsigned tex_lev = get_log2i(max(ti.w, ti.h));
    if (tex_lev > tex_ld_lev && rd_lev + start_lev > tex_lev - tex_ld_lev)
      rd_lev = tex_lev - tex_ld_lev - start_lev; // limit rd_level to mips that are not already loaded to texture
  }

  RMGR_TRACE("tex=%p(%s) cflg=0x%08x type=%d %dx%d,L%d fmt=0x%x (to read to %dx%d,L%d) to slice %d", tex, tex->getTexName(), ti.cflg,
    tex->restype(), max(hdr.w >> skip_lev, 1), max(hdr.h >> skip_lev, 1), rd_lev, hdr.d3dFormat, max(ti.w >> start_lev, 1),
    max(ti.h >> start_lev, 1), rd_lev, slice);

  UnifiedTexGenLoad ucrd(crd, hdr);
  tex->allocateTex();
  return load_tex_data_2d(tex, BAD_TEXTUREID, BAD_TEXTUREID, hdr, *ucrd, start_lev, rd_lev, skip_lev, ti.cflg, q_id, slice, nullptr);
}

#include <image/dag_dxtCompress.h>
#include <image/dag_texPixel.h>
#if _TARGET_PC_WIN
#include <convert/detex/detex.h>
static void convert_bc7_to_dxt5(void *ref_data, int width, int height, int faces)
{
  int pitch = max(width / 4, 1) * 16;

  for (int faceNo = 0; faceNo < faces; faceNo++)
  {
    TexPixel32 *image = reinterpret_cast<TexPixel32 *>(memalloc(POW2_ALIGN(width * height * 4, 16), tmpmem));

    int srcShift = 0;
    const int BLOCK_SIDE = 4;
    for (int y = 0; y < height; y += BLOCK_SIDE)
      for (int x = 0; x < width; x += BLOCK_SIDE, srcShift += BLOCK_SIDE * BLOCK_SIDE)
      {
        TexPixel32 buffer[BLOCK_SIDE * BLOCK_SIDE];
        bool result = detexDecompressBlockBPTC(reinterpret_cast<const uint8_t *>(ref_data) + srcShift, DETEX_MODE_MASK_ALL_MODES_BPTC,
          DETEX_DECOMPRESS_FLAG_ENCODE, reinterpret_cast<uint8_t *>(buffer));
        // Unknown mode or block encountered during decoding
        // Per the BC7 format spec, we must return transparent black
        if (!result)
          memset(buffer, 0, sizeof(buffer));

        for (int i = 0; i < min(height, BLOCK_SIDE); ++i)
          for (int j = 0; j < min(width, BLOCK_SIDE); ++j)
          {
            eastl::swap(buffer[i * BLOCK_SIDE + j].r, buffer[i * BLOCK_SIDE + j].b);
            image[(y + i) * width + x + j] = buffer[i * BLOCK_SIDE + j];
          }
      }

    ManualDXT(MODE_DXT5, (TexPixel32 *)image, width, height, pitch, (char *)ref_data);
    memfree(image, tmpmem);
  }
}
#else
static void convert_bc7_to_dxt5(void *, int, int, int) { logerr("bc7->dxt5 conversion shall not be needed and n/a"); }
#endif

static bool convert_16b_to_argb(IGenLoad &crd, char *dst, int dst_pitch, int mip_level, int mip_data_pitch, int mip_data_lines,
  unsigned d3d_fmt)
{
  G_ASSERTF_RETURN(dst_pitch >= mip_data_pitch * 2, false, "dst_pitch=%d mip_data_pitch=%d (mip=%d %dx%d)", dst_pitch, mip_data_pitch,
    mip_level, mip_data_pitch, mip_data_lines);
  G_UNUSED(mip_level);

  uint16_t *oneline = (uint16_t *)memalloc(mip_data_pitch, tmpmem);
  if (d3d_fmt == D3DFMT_R5G6B5)
    for (int hi = mip_data_lines; hi > 0; hi--, dst += dst_pitch)
    {
      crd.readExact(oneline, mip_data_pitch);
      uint32_t *dest32 = (uint32_t *)dst;
      for (const uint16_t *src16 = oneline, *src16_e = oneline + mip_data_pitch / 2; src16 < src16_e; src16++, dest32++)
        *dest32 = ((uint32_t(*src16) << 3) & 0x000000F8) | ((uint32_t(*src16) << 5) & 0x0000FC00) |
                  ((uint32_t(*src16) << 8) & 0x00F80000) | 0xFF000000;
    }
  else if (d3d_fmt == D3DFMT_A4R4G4B4 || d3d_fmt == D3DFMT_X4R4G4B4)
    for (int hi = mip_data_lines; hi > 0; hi--, dst += dst_pitch)
    {
      crd.readExact(oneline, mip_data_pitch);
      uint32_t *dest32 = (uint32_t *)dst;
      for (const uint16_t *src16 = oneline, *src16_e = oneline + mip_data_pitch / 2; src16 < src16_e; src16++, dest32++)
      {
        uint32_t p32 = (uint32_t(*src16 & 0x000F)) | (uint32_t(*src16 & 0x00F0) << 4) | (uint32_t(*src16 & 0x0F00) << 8) |
                       (uint32_t(*src16 & 0xF000) << 12);
        *dest32 = p32 | (p32 << 4);
      }
    }
  else
    memset(dst, 0, dst_pitch * mip_data_lines);
  memfree(oneline, tmpmem);
  return true;
}

static bool convert_dxt_to_argb(IGenLoad &crd, char *dst, int dst_pitch, int mip_data_sz, int mip_data_w, int mip_data_h,
  unsigned d3d_fmt)
{
  Tab<unsigned char> tmp;
  tmp.resize(mip_data_sz);
  if (!crd.readExact(tmp.data(), data_size(tmp)))
    return false;
  decompress_dxt((unsigned char *)dst, mip_data_w, mip_data_h, dst_pitch, tmp.data(), d3d_fmt == D3DFMT_DXT1);
  return true;
}

static TexLoadRes load_genmip_sysmemcopy(TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr, IGenLoad &crd, int q_id)
{
  UnifiedTexGenLoad ucrd(crd, hdr);
  if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && tid != BAD_TEXTUREID)
    RMGR.incBaseTexRc(tid);
  TexLoadRes loadStatus = load_tex_data_2d(nullptr, tid, paired_tid, hdr, *ucrd, /*start_lev*/ 0, /*rd_lev*/ 0, /*skip_lev*/ 0,
    /*cflg*/ 0, q_id, 0, nullptr);
  if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && tid != BAD_TEXTUREID)
    RMGR.decBaseTexRc(tid);
  return loadStatus;
}

void d3d_genmip_store_sysmem_copy(TEXTUREID tid, const ddsx::Header &hdr, const void *data)
{
  int idx = RMGR.toIndex(tid);
  G_ASSERTF_RETURN(idx >= 0 && RMGR.getBaseTexRc(tid) > 0, , "tid=0x%x for hdr=%dx%d, data=%p", tid, hdr.w, hdr.h, data);

  if (RMGR.hasTexBaseData(idx)) // since base data is deterministic we don't want replace it with the same data while reallocating new
                                // block
    return;
  if (!texmgr_internal::is_sys_mem_enough_to_load_basedata() && RMGR.getBaseTexRc(tid) < 1) // mem is low but baseData is not needed
    return;
  RMGR.replaceTexBaseData(idx, texmgr_internal::D3dResMgrDataFinal::BaseData::make(hdr, data));
}

void texmgr_internal::register_ddsx_load_implementation()
{
  d3d_load_ddsx_tex_contents_impl = &load_ddsx_tex_contents;
  d3d_load_ddsx_to_slice = &load_ddsx_to_slice;
  texmgr_internal::d3d_load_genmip_sysmemcopy = &load_genmip_sysmemcopy;
}
