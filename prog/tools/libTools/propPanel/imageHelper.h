// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/propPanel.h>
#include <gui/dag_imguiUtil.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_resId.h>
#include <util/dag_string.h>
#include <imgui/imgui.h>
#include <EASTL/vector_map.h>
#include <3d/dag_resMgr.h>

namespace PropPanel
{

class ImageHelper
{
public:
  ImageHelper();
  ~ImageHelper() { release(); }

  void release();
  void reloadAllIcons();

  // filename: name of the icon without extension. E.g.: "close_editor".
  // size: the icon will be loaded using this size for both width and height. If size is <= 0 then
  //       ImguiHelper::getFontSizedIconSize().x will be used.
  IconId loadIcon(const char *filename, int size = 0);

  TEXTUREID getTextureIdFromIconId(IconId icon_id) { return textures[(int)icon_id]; }
  ImTextureID getImTextureIdFromIconId(IconId icon_id) { return TextureIdToImTextureId(getTextureIdFromIconId(icon_id)); }

  static ImTextureID TextureIdToImTextureId(TEXTUREID id)
  {
    return ImGuiDagor::EncodeTexturePtr<ImTextureID>(D3dResManagerData::getBaseTex(id));
  }

private:
  static TEXTUREID loadIconFile(const char *filename, int size);

  eastl::vector_map<String, IconId> textureMap; // key format: icon_name_without_extension:size
  dag::Vector<TEXTUREID> textures;              // indexed with IconId
  String tempBuffer;
};

extern ImageHelper image_helper;

} // namespace PropPanel