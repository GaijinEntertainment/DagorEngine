// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/atlasTexManager.h>
#include <3d/dag_dynAtlas.h>
#include <3d/dag_createTex.h>
#include <3d/dag_texPackMgr2.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastIntList.h>
#include <util/dag_texMetaData.h>
#include <util/dag_delayedAction.h>
#include <math/dag_adjpow2.h>
#include <stdio.h>


namespace atlas_tex_manager
{
typedef DynamicAtlasTexTemplate<false> DynamicTexAtlas;

struct AtlasData
{
  enum
  {
    FMT_ARGB,
    FMT_L8,
    FMT_DXT5,
    FMT_DXT,
    FMT_none
  };
  DynamicTexAtlas atlas;
  dag::Vector<DynamicTexAtlas::ItemData *> itemData;
  dag::Vector<Point4> texcoordData;
  IPoint2 maxAllowedPicSz = IPoint2(4096, 4096);
};
}; // namespace atlas_tex_manager


namespace atlas_tex_manager
{
static inline uint32_t str_hash_fnv1(const char *fn)
{
  uint32_t c, result = 2166136261U; // We implement an FNV-like string hash
  for (; (c = *fn) != 0; fn++)
    result = (result * 16777619) ^ c;
  return result;
}
}; // namespace atlas_tex_manager

static uint8_t clear_value_astc4[16] = {
  0xFC, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static bool check_format_supported(uint32_t tex_fmt)
{
  switch (tex_fmt)
  {
    // These formats require special values for clearing into transparent black.
    // If will be needed, we would have to extract them. For now just don't support these formats.
    case TEXFMT_ASTC8:
    case TEXFMT_ASTC12: return false;
  }
  return true;
}

static bool need_clear_texture_from_cpu(uint32_t tex_fmt)
{
  switch (tex_fmt)
  {
    case TEXFMT_ASTC4:
    case TEXFMT_ASTC8:
    case TEXFMT_ASTC12: return true;
  }
  return false;
}

static void clear_texture_from_cpu(Texture *tex)
{
  if (!tex)
    return;

  TextureInfo texInfo;
  tex->getinfo(texInfo);

  uint8_t zeroValue = 0;
  int clearValueSize = 1;
  const void *clearValuePtr = &zeroValue;
  int blockHeight = 1;
  switch (texInfo.cflg & TEXFMT_MASK)
  {
    case TEXFMT_ASTC4:
      clearValueSize = sizeof(clear_value_astc4);
      clearValuePtr = clear_value_astc4;
      blockHeight = 4;
      break;
  }

  for (int mip = 0; mip < texInfo.mipLevels; ++mip)
  {
    uint8_t *data;
    int strideBytes;
    if (tex->lockimg((void **)&data, strideBytes, mip, TEXLOCK_WRITE))
    {
      int nBlocksH = texInfo.h / (blockHeight << mip);
      for (int offset = 0; offset < nBlocksH * strideBytes; offset += clearValueSize)
        memcpy(data + offset, clearValuePtr, clearValueSize);

      tex->unlockimg();
    }
  }
}

bool atlas_tex_manager::TexRec::initAtlas(int tex_count, dag::ConstSpan<unsigned> tex_fmt, const char *tex_name, IPoint2 tex_size,
  int mip_count)
{
  blockSize = IPoint2(1, 1);
  for (uint32_t i = 0; i < tex_fmt.size(); i++)
  {
    const TextureFormatDesc &fmtDesc = get_tex_format_desc(tex_fmt[i] & TEXFMT_MASK);
    blockSize.x = max(blockSize.x, int(fmtDesc.elementWidth));
    blockSize.y = max(blockSize.y, int(fmtDesc.elementHeight));

    if (!check_format_supported(tex_fmt[i] & TEXFMT_MASK))
    {
      logerr("AtlasTexManager doesn't support format 0x%08X", tex_fmt[i]);
      return false;
    }
  }

  ad = new AtlasData;

  ad->atlas.init(tex_size, tex_count, 0, NULL, TEXFMT_DEFAULT | TEXCF_UPDATE_DESTINATION);
  size = tex_size;

  ad->itemData.resize(tex_count);
  ad->texcoordData.resize(tex_count);

  atlasTex.resize(tex_fmt.size());
  atlasTexFmt.resize(tex_fmt.size());

  int maxMips = min(get_log2i(tex_size.x / blockSize.x), get_log2i(tex_size.y / blockSize.y));
  mipCount = min(mip_count, maxMips);
  // Align textures by maximum mip and take possible compression into account.
  alignment = (1 << (mipCount - 1)) * blockSize;

  for (uint32_t i = 0; i < tex_fmt.size(); i++)
  {
    String texName;
    texName.printf(0, "%s_%d", tex_name, i);
    bool clearFromCpu = need_clear_texture_from_cpu(tex_fmt[i] & TEXFMT_MASK);
    int initFlag = clearFromCpu ? TEXCF_LOADONCE : TEXCF_CLEAR_ON_CREATE;
    atlasTex[i] = dag::create_tex(NULL, tex_size.x, tex_size.y, tex_fmt[i] | initFlag, mipCount, texName);
    if (clearFromCpu)
      clear_texture_from_cpu(atlasTex[i].getTex2D());
    atlasTexFmt[i] = tex_fmt[i];
  }

  atlasTexcount = 0;
  return true;
}

void atlas_tex_manager::TexRec::resetAtlas()
{
  atlasTex.clear();
  atlasTexFmt.clear();
  if (ad)
  {
    ad->itemData.clear();
    ad->texcoordData.clear();
  }
  del_it(ad);
}

int atlas_tex_manager::TexRec::addTextureToAtlas(TextureInfo ti, Point4 &texcoords, bool allow_discard)
{
  DynamicTexAtlas::ItemData *d = const_cast<DynamicTexAtlas::ItemData *>(
    ad->atlas.addItem(0, 0, max(ti.w, uint16_t(alignment.x)), max(ti.h, uint16_t(alignment.y)), atlasTexcount + 1, -1, allow_discard));
  if (!d)
    return -1;

  ad->itemData[atlasTexcount] = d;
  texcoords.x = (float)d->x0 / (float)size.x;
  texcoords.y = (float)d->y0 / (float)size.y;
  texcoords.z = (float)(d->x0 + ti.w) / (float)size.x;
  texcoords.w = (float)(d->y0 + ti.h) / (float)size.y;
  ad->texcoordData[atlasTexcount] = texcoords;
  return atlasTexcount++;
}

bool atlas_tex_manager::TexRec::writeTextureToAtlas(Texture *tex, int atlas_index, int layer)
{
  TextureInfo ti;
  tex->getinfo(ti, 0);
  if ((ti.cflg & TEXFMT_MASK) != (atlasTexFmt[layer] & TEXFMT_MASK))
  {
    G_ASSERTF(false, "Cannot load texture '%s' to atlas with index %d. Texture Format - %d, Atlas Format - %d", tex->getTexName(),
      atlas_index, ti.cflg & TEXFMT_MASK, atlasTexFmt[layer] & TEXFMT_MASK);
    return false;
  }
  DynamicTexAtlas::ItemData *d = ad->itemData[atlas_index];

  if (d)
  {
    G_ASSERTF_RETURN(max(ti.w, uint16_t(alignment.x)) == d->w && max(ti.h, uint16_t(alignment.y)) == d->h, false,
      "allocated as %dx%d, but trying to load as %dx%d (alignment = (%d, %d))", d->w, d->h, ti.w, ti.h, alignment.x, alignment.y);

    for (int i = 0; i < mipCount; i++)
    {
      if (i >= tex->level_count())
        break;

      int destX = d->x0 >> i;
      int destY = d->y0 >> i;
      int texMipW = ti.w >> i;
      int texMipH = ti.h >> i;

      if (texMipW == 0 || texMipH == 0)
        break;

      if ((texMipW % blockSize.x != 0) || (texMipH % blockSize.y != 0))
        break;

      atlasTex[layer].getTex2D()->updateSubRegion(tex, i, 0, 0, 0, texMipW, texMipH, 1, i, destX, destY, 0);
    }
  }

  return true;
}

Point4 atlas_tex_manager::TexRec::getTc(int index) { return ad->texcoordData[index]; }