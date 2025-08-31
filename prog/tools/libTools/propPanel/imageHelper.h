// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/propPanel.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_resId.h>
#include <imgui/imgui.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>

namespace PropPanel
{

class ImageHelper
{
public:
  ImageHelper();
  ~ImageHelper() { release(); }

  void release();
  void reloadAllIcons();

  IconId loadIcon(const char *filename);
  TEXTUREID getTextureIdFromIconId(IconId icon_id) { return textures[(int)icon_id]; }
  ImTextureID getImTextureIdFromIconId(IconId icon_id) { return TextureIdToImTextureId(getTextureIdFromIconId(icon_id)); }

  static ImTextureID TextureIdToImTextureId(TEXTUREID id) { return (ImTextureID)((uintptr_t)((unsigned)id)); }

private:
  static TEXTUREID loadIconFile(const char *filename);

  eastl::vector_map<eastl::string, IconId> textureMap;
  dag::Vector<TEXTUREID> textures; // indexed with IconId
};

extern ImageHelper image_helper;

} // namespace PropPanel