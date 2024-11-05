//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <physMap/physMatSwRenderer.h>
#include <image/dag_texPixel.h>
#include <drv/3d/dag_texture.h>

template <int Width, int Height>
class RenderDecalMaterialsWithRT : public RenderDecalMaterials<Width, Height>
{
public:
  enum
  {
    WIDTH = Width,
    HEIGHT = Height
  };

  RenderDecalMaterialsWithRT() { renderTarget = NULL; }
  virtual ~RenderDecalMaterialsWithRT() { clear(); }

  void clear() { del_d3dres(renderTarget); }

  void renderPhysMap(const PhysMap &phys_map, const BBox2 &region, bool apply_decals = true)
  {
    if (!renderTarget)
      renderTarget = d3d::create_tex(NULL, WIDTH, HEIGHT, TEXFMT_A8R8G8B8 | TEXCF_DYNAMIC, 1, "decalRT");
    RenderDecalMaterials<Width, Height>::renderPhysMap(phys_map, region, apply_decals);
  }

  Texture *getRT() { return renderTarget; }

  void updateRT(const PhysMap &phys_map, bool remap_to_unique = false)
  {
    TexPixel32 *pixels = NULL;
    static const uint32_t colorMap[] = {
      0xff00ff00, // green
      0xff0000ff, // blue
      0xffffff00, // yellow
      0xffff00ff, // magenta
      0xffffffff, // white
      0xff00ffff, // cyan
      0xff808080, // gray
      0xffff0000, // red
    };
    int strideBytes;

    Tab<uint32_t> matColors(framemem_ptr());
    matColors.resize(PhysMat::physMatCount());
    for (int i = 0; i < matColors.size(); ++i)
    {
      if (remap_to_unique)
      {
        int matIdx = find_value_idx(phys_map.materials, i);
        matColors[i] = matIdx >= 0 ? colorMap[matIdx % countof(colorMap)] : 0xff000000;
      }
      else
        matColors[i] = colorMap[i % countof(colorMap)];
    }

    if (renderTarget->lockimg((void **)&pixels, strideBytes, 0, TEXLOCK_WRITE))
    {
      for (int y = 0; y < HEIGHT; ++y, pixels = (TexPixel32 *)(((uint8_t *)pixels) + strideBytes))
        for (int x = 0; x < WIDTH; ++x)
        {
          if (!RenderDecalMaterials<Width, Height>::checkPixel(x, y))
            pixels[x].u = 0xff000000;
          else
            pixels[x].u = matColors[RenderDecalMaterials<Width, Height>::materials[y * WIDTH + x]];
        }
      renderTarget->unlockimg();
    }
  }
  void updateRTDeltaHeight(const PhysMap &phys_map)
  {
    float maxHt = 0.f;
    for (int i = 0; i < phys_map.materials.size(); ++i)
    {
      const PhysMat::MaterialData &mat = PhysMat::getMaterial(phys_map.materials[i]);
      maxHt = max(maxHt, mat.deformableWidth);
    }
    int strideBytes;
    TexPixel32 *pixels = NULL;
    if (renderTarget->lockimg((void **)&pixels, strideBytes, 0, TEXLOCK_WRITE))
    {
      if (maxHt < 0.01)
        for (int y = 0; y < HEIGHT; ++y, pixels = (TexPixel32 *)(((uint8_t *)pixels) + strideBytes))
          memset(pixels, 0, WIDTH * sizeof(TexPixel32));
      else
      {
        float invMaxHt = 255.f / maxHt;
        for (int y = 0; y < HEIGHT; ++y, pixels = (TexPixel32 *)(((uint8_t *)pixels) + strideBytes))
          for (int x = 0; x < WIDTH; ++x)
          {
            if (!RenderDecalMaterials<Width, Height>::checkPixel(x, y))
              pixels[x].u = 0;
            else
            {
              int matId = RenderDecalMaterials<Width, Height>::materials[y * WIDTH + x];
              uint8_t thickness = uint8_t(PhysMat::getMaterial(matId).deformableWidth * invMaxHt);
              pixels[x].u = thickness | (thickness << 8) | (thickness << 16) | (0xFF << 24);
            }
          }
      }
    }
    renderTarget->unlockimg();
  }

protected:
  Texture *renderTarget;
};
