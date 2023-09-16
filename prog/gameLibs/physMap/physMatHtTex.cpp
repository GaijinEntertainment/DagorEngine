#include <physMap/physMap.h>
#include <physMap/physMatHtTex.h>
#include <physMap/physMatSwRenderer.h>
#include <scene/dag_physMat.h>
#include <math/dag_bounds3.h>
#include <generic/dag_smallTab.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <convert/fastDXT/fastDXT.h>

static constexpr int TEXSIZE = 2048;

class PhysMapTexData
{
public:
  SmallTab<uint8_t, TmpmemAlloc> texData;
  uint32_t texFmt;
  PhysMapTexData(uint32_t fmt) : texFmt(fmt)
  {
    if (fmt == TEXFMT_L8)
      clear_and_resize(texData, TEXSIZE * TEXSIZE);
    else if (fmt == TEXFMT_DXT1 || fmt == TEXFMT_ATI1N)
      clear_and_resize(texData, TEXSIZE * TEXSIZE / 2);
  }
};

void destroy_phys_map_tex_data(PhysMapTexData *data) { del_it(data); }

Texture *create_phys_map_ht_tex(PhysMapTexData *data)
{
  if (!data)
    return NULL;
  Texture *tex = d3d::create_tex(NULL, TEXSIZE, TEXSIZE, data->texFmt, 1, "physMatHt");
  if (!tex)
  {
    logerr("can not create physmatht texture");
    return NULL;
  }
  int strideBytes;
  uint8_t *pixels = NULL;
  if (!tex->lockimg((void **)&pixels, strideBytes, 0, TEXLOCK_WRITE | TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_DELSYSMEMCOPY)) //
  {
    logerr("can not lock physmatht  texture");
    del_d3dres(tex);
    return NULL;
  }
  const uint8_t *srcPixels = data->texData.data();
  if (data->texFmt == TEXFMT_DXT1 || data->texFmt == TEXFMT_ATI1N)
  {
    for (int y = 0; y < TEXSIZE; y += 4, pixels += strideBytes, srcPixels += TEXSIZE * 2)
      memcpy(pixels, srcPixels, TEXSIZE * 2);
  }
  else
  {
    for (int y = 0; y < TEXSIZE; ++y, pixels += strideBytes, srcPixels += TEXSIZE)
      memcpy(pixels, srcPixels, TEXSIZE);
  }
  tex->unlockimg();
  tex->texaddr(TEXADDR_BORDER);
  tex->texbordercolor(0);
  return tex;
}


#define FILL_TWO_TEXELS_WITH_0 2

PhysMapTexData *render_phys_map_ht_data(const PhysMap &phys_map, const BBox2 &region, float &maxHt, float ht_scale, bool apply_decals)
{
  uint32_t texFmt = TEXFMT_ATI1N;
  if (!(d3d::get_texformat_usage(texFmt) & d3d::USAGE_VERTEXTEXTURE))
  {
    if (!(d3d::get_texformat_usage(TEXFMT_L8) & d3d::USAGE_VERTEXTEXTURE))
    {
      logerr("L8 is not supported as vertex texture");
      return NULL;
    }
    else
      texFmt = TEXFMT_L8;
  }
  maxHt = 0.f;
  for (int i = 0; i < phys_map.materials.size(); ++i)
  {
    const PhysMat::MaterialData &mat = PhysMat::getMaterial(phys_map.materials[i]);
    maxHt = max(maxHt, mat.deformableWidth);
  }
  if (maxHt < 0.01f)
    return NULL;

  RenderDecalMaterials<TEXSIZE, TEXSIZE> *physMap = new RenderDecalMaterials<TEXSIZE, TEXSIZE>;
  physMap->renderPhysMap(phys_map, region, apply_decals);
  SmallTab<uint8_t, TmpmemAlloc> materialThickness;
  clear_and_resize(materialThickness, PhysMat::physMatCount());
  float invMaxHtScaled = clamp(ht_scale, 0.f, 1.f) * (255.5f / maxHt);
  for (int i = 0; i < materialThickness.size(); ++i)
  {
    materialThickness[i] = clamp(int(floorf(PhysMat::getMaterial(i).deformableWidth * invMaxHtScaled)), 0, 255);
  }
  PhysMapTexData *physMapData = new PhysMapTexData(texFmt);

  uint8_t *pixels = physMapData->texData.data();
  uint8_t horline[6][TEXSIZE + 2];
  memset(horline, 0, sizeof(horline));
#if !FILL_TWO_TEXELS_WITH_0
  for (int x = 0; x < TEXSIZE; x++)
  {
    int matId = physMap->getMaterials()[x];
    horline[5][x] = matId >= materialThickness.size() ? 0 : materialThickness[matId];
  }
#endif
  for (int y = 0; y < TEXSIZE; y += 4)
  {
    memcpy(horline[0], horline[4], TEXSIZE + 2);
    memcpy(horline[1], horline[5], TEXSIZE + 2);

    for (int line = 0, id = (y + 1) * TEXSIZE + 1; line < (y < TEXSIZE - 4 ? 4 : 3); line++, id += 2 * FILL_TWO_TEXELS_WITH_0)
    {
      for (int x = FILL_TWO_TEXELS_WITH_0; x < TEXSIZE - FILL_TWO_TEXELS_WITH_0; x++, id++)
      {
        int matId = physMap->getMaterials()[id];
        uint8_t thick = matId >= materialThickness.size() ? 0 : materialThickness[matId];
        horline[line + 2][x + 1] = thick;
      }
    }
#if FILL_TWO_TEXELS_WITH_0
    if (y == 0)
      memset(horline[2], 0, TEXSIZE + 2);
#endif

    if (y == TEXSIZE - 4)
    {
#if FILL_TWO_TEXELS_WITH_0
      memset(horline[3], 0, TEXSIZE + 2);
#endif
      memset(horline[4], 0, TEXSIZE + 2);
      memset(horline[5], 0, TEXSIZE + 2);
    }

    const uint8_t *src = horline[1] + 1;
    if (texFmt == TEXFMT_L8)
    {
      for (int line = 0; line < 4; ++line, src += 2)
        for (int x = 0; x < TEXSIZE; x++, pixels++, src++)
        {
          int pixel = src[0] * 12 + (src[-1] + src[1] + src[-(TEXSIZE + 2)] + src[TEXSIZE + 2]) * 4 +
                      (src[-(TEXSIZE + 2) - 1] + src[-(TEXSIZE + 2) + 1] + src[(TEXSIZE + 2) - 1] + src[(TEXSIZE + 2) + 1]);
          *pixels = (pixel + 15) >> 5;
        }
    }
    else if (texFmt == TEXFMT_ATI1N)
    {
      for (int x = 0; x < TEXSIZE; x += 4, src += 4, pixels += 8)
      {
        alignas(16) uint8_t block[16];
        uint8_t minV = 255, maxV = 0;
        const uint8_t *src2 = src;
        for (int bi = 0, j = 0; j < 4; ++j, src2 += TEXSIZE + 2 - 4)
        {
          for (int i = 0; i < 4; ++i, ++bi, ++src2)
          {
            int pixel = src2[0] * 12 + (src2[-1] + src2[1] + src2[-(TEXSIZE + 2)] + src2[TEXSIZE + 2]) * 4 +
                        (src2[-(TEXSIZE + 2) - 1] + src2[-(TEXSIZE + 2) + 1] + src2[(TEXSIZE + 2) - 1] + src2[(TEXSIZE + 2) + 1]);
            uint8_t thick = (pixel + 15) >> 5;
            block[bi] = thick;
            maxV = max(maxV, thick);
            minV = min(minV, thick);
          }
        }
        fastDXT::write_BC4_block(block, minV, maxV, pixels);
      }
    }
  }
  del_it(physMap);
  return physMapData;
}
