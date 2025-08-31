// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "basetexture.h"


bool BaseTextureImpl::isSameFormat(int cflg1, int cflg2)
{
  if ((cflg1 & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == (cflg2 & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) &&
      (cflg1 & TEXFMT_MASK) == (cflg2 & TEXFMT_MASK))
  {
    return true;
  }
  return false;
}

BaseTextureImpl::BaseTextureImpl(uint32_t cflg, D3DResourceType type) :
  minMipLevel{15}, // 32768 texture size
  maxMipLevel{0},
  mipLevels{1},
  cflg{cflg},
  type{type},
  base_format{cflg & TEXFMT_MASK}
{}

int BaseTextureImpl::level_count() const { return mipLevels; }

void BaseTextureImpl::setTexName(const char *name) { setName(name); }

int BaseTextureImpl::getinfo(TextureInfo &ti, int level) const
{
  if (level >= mipLevels)
    level = mipLevels - 1;

  ti.w = max<uint32_t>(1u, width >> level);
  ti.h = max<uint32_t>(1u, height >> level);
  switch (type)
  {
    case D3DResourceType::TEX:
      ti.d = 1;
      ti.a = 1;
      break;
    case D3DResourceType::CUBETEX:
      ti.d = 1;
      ti.a = 6;
      break;
    case D3DResourceType::VOLTEX:
      ti.d = max<uint32_t>(1u, depth >> level);
      ti.a = 1;
      break;
    case D3DResourceType::ARRTEX:
    case D3DResourceType::CUBEARRTEX:
      ti.d = 1;
      ti.a = depth * (isCubeArray() ? 6 : 1);
      break;
    case D3DResourceType::SBUF: G_ASSERT(false); break;
  }

  ti.mipLevels = mipLevels;
  ti.type = type;
  ti.cflg = cflg;
  return 1;
}

uint32_t auto_mip_levels_count(uint32_t w, uint32_t min_size)
{
  uint32_t lev = 1;
  uint32_t minExtend = w;
  while (minExtend > min_size)
  {
    lev++;
    minExtend >>= 1;
  }
  return lev;
}

uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t min_size) { return auto_mip_levels_count(min(w, h), min_size); }

uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t d, uint32_t min_size)
{
  return auto_mip_levels_count(min(min(w, h), d), min_size);
}

int count_mips_if_needed(int w, int h, int32_t flg, int levels)
{
  if (0 == levels)
  {
    const bool rt = (TEXCF_RTARGET & flg) != 0;
    levels = auto_mip_levels_count(w, h, rt ? 1 : 4);
    debug("Auto compute for texture mip levels yielded %d", levels);
  }

  return levels;
}
