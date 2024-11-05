// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <imgui/imgui.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>

class BaseTexture;

namespace PropPanel
{

class ImageHelper
{
public:
  ~ImageHelper() { release(); }

  void release();

  TEXTUREID loadIcon(const char *filename);

  static ImTextureID TextureIdToImTextureId(TEXTUREID id) { return (ImTextureID)((unsigned)id); }

private:
  BaseTexture *createTextureFromBuffer(const char *filename, const uint32_t *image_data, int width, int height);

  eastl::vector_map<eastl::string, TEXTUREID> loadedTextures;
};

extern ImageHelper image_helper;

} // namespace PropPanel