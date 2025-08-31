// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "imageHelper.h"
#include <propPanel/c_util.h>
#include <propPanel/propPanel.h>
#include <propPanel/imguiHelper.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>

namespace PropPanel
{

ImageHelper image_helper;

ImageHelper::ImageHelper()
{
  G_STATIC_ASSERT((int)IconId::Invalid == 0);
  textures.push_back(BAD_TEXTUREID);
}

void ImageHelper::release()
{
  for (auto i = textures.begin(); i != textures.end(); ++i)
    if (*i != BAD_TEXTUREID)
      release_managed_tex(*i);

  textures.clear();
  textureMap.clear();
}

void ImageHelper::reloadAllIcons()
{
  for (auto i = textureMap.begin(); i != textureMap.end(); ++i)
  {
    const IconId iconId = i->second;
    TEXTUREID &textureId = textures[(int)iconId];
    if (textureId != BAD_TEXTUREID)
      release_managed_tex(textureId);
    textureId = loadIconFile(i->first.c_str());
  }
}

TEXTUREID ImageHelper::loadIconFile(const char *filename)
{
  String iconPath(256, "%s%s.tga", p2util::get_icon_path(), filename);
  const char *iconFallbackPath = p2util::get_icon_fallback_path();
  if (iconFallbackPath && *iconFallbackPath && !dd_file_exist(iconPath))
    iconPath.printf(256, "%s%s.tga", iconFallbackPath, filename);
  const ImVec2 fontSizedIconSize = ImguiHelper::getFontSizedIconSize();
  iconPath.aprintf(0, ":%d:%d:K", fontSizedIconSize.x, fontSizedIconSize.y);

  TEXTUREID textureId = add_managed_texture(iconPath);
  if (textureId == BAD_TEXTUREID)
    return textureId;

  if (!acquire_managed_tex(textureId))
  {
    release_managed_tex(textureId);
    return BAD_TEXTUREID;
  }

  TextureMetaData textureMetaData;
  textureMetaData.texFilterMode = TextureMetaData::FILT_POINT;
  set_texture_separate_sampler(textureId, get_sampler_info(textureMetaData));

  return textureId;
}

IconId ImageHelper::loadIcon(const char *filename)
{
  if (!filename || !filename[0])
    return IconId::Invalid;

  auto i = textureMap.find(filename);
  if (i != textureMap.end())
    return i->second;

  TEXTUREID textureId = loadIconFile(filename);
  if (textureId == BAD_TEXTUREID)
    return IconId::Invalid;

  G_ASSERT(!textures.empty()); // The first item is BAD_TEXTUREID for IconId.
  const IconId iconId = (IconId)textures.size();
  textures.push_back(textureId);

  textureMap.insert(eastl::pair<eastl::string, IconId>(eastl::string(filename), iconId));

  return iconId;
}

} // namespace PropPanel