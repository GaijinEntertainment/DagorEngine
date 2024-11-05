// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "imageHelper.h"
#include <propPanel/c_util.h>
#include <3d/dag_lockTexture.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <util/dag_texMetaData.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include <stb/stb_image.h>

namespace PropPanel
{

ImageHelper image_helper;

void ImageHelper::release()
{
  for (auto i = loadedTextures.begin(); i != loadedTextures.end(); ++i)
    release_managed_tex(i->second);

  loadedTextures.clear();
}

BaseTexture *ImageHelper::createTextureFromBuffer(const char *filename, const uint32_t *image_data, int width, int height)
{
  BaseTexture *texture = d3d::create_tex(nullptr, width, height, TEXFMT_A8R8G8B8, 1, filename);
  if (!texture)
    return nullptr;

  TextureMetaData textureMetaData;
  apply_gen_tex_props(texture, textureMetaData);

  LockedImage2DView<uint32_t> lockedTexture = lock_texture<uint32_t>(texture, 0, TEXLOCK_WRITE);
  if (!lockedTexture)
  {
    del_d3dres(texture);
    return nullptr;
  }

  for (int i = 0; i < height; ++i)
    memcpy(lockedTexture.get() + (lockedTexture.getByteStride() * i), image_data + (width * i), width * sizeof(uint32_t));

  return texture;
}

TEXTUREID ImageHelper::loadIcon(const char *filename)
{
  if (!filename || !filename[0])
    return BAD_TEXTUREID;

  auto i = loadedTextures.find(filename);
  if (i != loadedTextures.end())
  {
    TEXTUREID textureId = i->second;
    return textureId;
  }

  const String bmp(256, "%s%s%s", p2util::get_icon_path(), filename, ".bmp");
  int width, height, components;
  uint32_t *imageData = (uint32_t *)stbi_load(bmp, &width, &height, &components, 4);
  if (!imageData)
  {
    logdbg("Failed to load image '%s'.", bmp.c_str());
    return BAD_TEXTUREID;
  }

  const uint32_t transparentColor = imageData[0]; // In BMPs the first pixel defines the transparent color.
  uint32_t *currentPixel = imageData;
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
    {
      uint8_t *currentPixelByte = (uint8_t *)currentPixel;

      if (*currentPixel == transparentColor)
        currentPixelByte[3] = 0;

      eastl::swap(currentPixelByte[0], currentPixelByte[2]); // Swap red and blue.
      ++currentPixel;
    }

  BaseTexture *texture = createTextureFromBuffer(bmp, imageData, width, height);
  stbi_image_free(imageData);
  if (!texture)
  {
    logerr("Failed to create texture from stb image '%s'.", bmp.c_str());
    return BAD_TEXTUREID;
  }

  // The icons look more crisp with nearest neighbor filtering.
  texture->texfilter(TEXFILTER_POINT);

  TEXTUREID textureId = register_managed_tex(String(64, "propPanel_%s", filename), texture);
  TextureMetaData textureMetaData;
  textureMetaData.texFilterMode = TextureMetaData::FILT_POINT;
  set_texture_separate_sampler(textureId, get_sampler_info(textureMetaData));
  G_ASSERT(textureId != BAD_TEXTUREID);
  loadedTextures.insert(eastl::make_pair<eastl::string, TEXTUREID>(eastl::string(filename), textureId));

  return textureId;
}

} // namespace PropPanel