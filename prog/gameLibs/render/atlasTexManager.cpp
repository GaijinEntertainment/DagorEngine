#include <render/atlasTexManager.h>
#include <3d/dag_dynAtlas.h>
#include <3d/dag_createTex.h>
#include <3d/dag_drv3dCmd.h>
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

bool atlas_tex_manager::TexRec::initAtlas(int tex_count, dag::ConstSpan<unsigned> tex_fmt, const char *tex_name, IPoint2 tex_size,
  int mip_count)
{
  ad = new AtlasData;

  ad->atlas.init(tex_size, tex_count, 0, NULL, TEXFMT_DEFAULT | TEXCF_UPDATE_DESTINATION);
  size = tex_size;

  ad->itemData.resize(tex_count);
  ad->texcoordData.resize(tex_count);

  mipCount = mip_count;

  atlasTex.resize(tex_fmt.size());
  atlasTexFmt.resize(tex_fmt.size());

  for (uint32_t i = 0; i < tex_fmt.size(); i++)
  {
    switch (tex_fmt[i] & TEXFMT_MASK)
    {
      case TEXFMT_DXT1:
      case TEXFMT_DXT5:
      case TEXFMT_ATI1N:
      case TEXFMT_ATI2N:
      case TEXFMT_BC7:
      case TEXFMT_BC6H: compressed = true; break;
      default: break;
    }
    atlasTexFmt[i] = tex_fmt[i];
    String texName;
    texName.printf(0, "%s_%d", tex_name, i);
    atlasTex[i] = dag::create_tex(NULL, tex_size.x, tex_size.y, tex_fmt[i] | TEXCF_CLEAR_ON_CREATE, mipCount, texName);
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
  // align textures by maximum mip
  uint16_t alignSize = (1 << (mipCount - 1)) * (compressed ? 4 : 1);
  DynamicTexAtlas::ItemData *d = const_cast<DynamicTexAtlas::ItemData *>(
    ad->atlas.addItem(0, 0, max(ti.w, alignSize), max(ti.h, alignSize), atlasTexcount + 1, -1, allow_discard));
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
  uint16_t alignSize = (1 << (mipCount - 1)) * (compressed ? 4 : 1);
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
    G_ASSERTF_RETURN(max(ti.w, alignSize) == d->w && max(ti.h, alignSize) == d->h, false,
      "allocated as %dx%d, but trying to load as %dx%d (alignSize=%d)", d->w, d->h, ti.w, ti.h, alignSize);
    int destX = d->x0;
    int destY = d->y0;

    for (int i = 0; i < mipCount; i++)
    {
      if (i >= tex->level_count())
        break;

      if ((ti.w >> i) == 0 || (ti.h >> i) == 0)
        break;

      if (compressed && (((ti.w >> i) % 4 != 0) || ((ti.h >> i) % 4 != 0)))
        break;

      atlasTex[layer].getTex2D()->updateSubRegion(tex, i, 0, 0, 0, ti.w >> i, ti.h >> i, 1, i, destX, destY, 0);

      destX /= 2;
      destY /= 2;
    }
  }

  return true;
}

Point4 atlas_tex_manager::TexRec::getTc(int index) { return ad->texcoordData[index]; }