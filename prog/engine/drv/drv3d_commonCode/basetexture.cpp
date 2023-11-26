
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

BaseTextureImpl::BaseTextureImpl(int set_cflg, int set_type)
{
  anisotropyLevel = 1;
  addrU = TEXADDR_WRAP;
  addrV = TEXADDR_WRAP;
  addrW = TEXADDR_WRAP;

  texFilter = TEXFILTER_DEFAULT;
  mipFilter = TEXMIPMAP_DEFAULT;
  minMipLevel = 15; // 32768 texture size
  maxMipLevel = 0;
  mipLevels = 1;
  cflg = set_cflg;
  type = set_type;
  base_format = (cflg & TEXFMT_MASK);
}

int BaseTextureImpl::level_count() const { return mipLevels; }

void BaseTextureImpl::setTexName(const char *name) { setResName(name); }

int BaseTextureImpl::getinfo(TextureInfo &ti, int level) const
{
  if (level >= mipLevels)
    level = mipLevels - 1;

  ti.w = max<uint32_t>(1u, width >> level);
  ti.h = max<uint32_t>(1u, height >> level);
  switch (type)
  {
    case RES3D_CUBETEX:
      ti.d = 1;
      ti.a = 6;
      break;
    case RES3D_CUBEARRTEX:
    case RES3D_ARRTEX:
      ti.d = 1;
      ti.a = depth * (isCubeArray() ? 6 : 1);
      break;
    case RES3D_VOLTEX:
      ti.d = max<uint32_t>(1u, depth >> level);
      ti.a = 1;
      break;
    default:
      ti.d = 1;
      ti.a = 1;
      break;
  }

  ti.mipLevels = mipLevels;
  ti.resType = type;
  ti.cflg = cflg;
  return 1;
}

uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t min_size)
{
  uint32_t lev = 1;
  uint32_t minExtend = min(w, h);
  while (minExtend > min_size)
  {
    lev++;
    minExtend >>= 1;
  }
  return lev;
}

eastl::pair<int32_t, int> add_srgb_read_flag_and_count_mips(int w, int h, int32_t flg, int levels)
{
  const bool rt = (TEXCF_RTARGET & flg) != 0;
  auto fmt = TEXFMT_MASK & flg;

  if (rt && (0 != (TEXCF_SRGBWRITE & flg)) && (TEXFMT_A8R8G8B8 != fmt))
  {
    // TODO verify this requirement (RE-508)
    if (0 != (TEXCF_SRGBREAD & flg))
    {
      debug("Adding TEXCF_SRGBREAD to texture flags, because chosen format needs it");
      flg |= TEXCF_SRGBREAD;
    }
  }

  if (0 == levels)
  {
    levels = auto_mip_levels_count(w, h, rt ? 1 : 4);
    debug("Auto compute for texture mip levels yielded %d", levels);
  }

  // update format info if it had changed
  return {(flg & ~TEXFMT_MASK) | fmt, levels};
}
