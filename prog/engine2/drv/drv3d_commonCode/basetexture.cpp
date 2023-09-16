
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
