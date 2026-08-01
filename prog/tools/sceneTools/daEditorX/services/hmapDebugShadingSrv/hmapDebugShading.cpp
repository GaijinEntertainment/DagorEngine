// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmapDebugShading.h"

#include <EditorCore/ec_IEditorCore.h>
#include <drv/3d/dag_texture.h>
#include <de3_hmapDebugShadingGradients.h>

#include <math/dag_color.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

void HeightmapDebugShading::update(const HeightmapColorGradient *gradient_to_show) { updateGradientShader(gradient_to_show); }

E3DCOLOR HeightmapDebugShading::getGradientColor(const HeightmapColorGradient &gradient, float height)
{
  const int keyCount = gradient.keys.size();
  if (keyCount == 0)
    return E3DCOLOR(255, 255, 255, 255);

  if (height <= gradient.keys.front().height || keyCount == 1)
    return gradient.keys.front().color;

  for (int i = 0; i < (keyCount - 1); ++i)
  {
    const HeightmapColorGradient::Key &nextKey = gradient.keys[i + 1];
    if (height < nextKey.height)
    {
      const HeightmapColorGradient::Key &key = gradient.keys[i];
      const float range = nextKey.height - key.height;
      if (range > 0.0f)
        return e3dcolor(lerp(color4(key.color), color4(nextKey.color), (height - key.height) / range));
      else
        return key.color;
    }
  }

  return gradient.keys.back().color;
}

void HeightmapDebugShading::updateGradientdTexture(const HeightmapColorGradient &gradient, float min_height, float max_height)
{
  if (!gradientTexture.getTex())
  {
    gradientTexture.set(
      d3d::create_tex(nullptr, TEXTURE_WIDTH, 1, TEXFMT_A8R8G8B8 | TEXCF_DYNAMIC | TEXCF_SRGBREAD, 1, "gradientLandTex"));
    G_ASSERT_RETURN(gradientTexture.getTex(), );
  }

  void *lockedTexture = nullptr;
  int stride = 0;
  if (!gradientTexture.getTex()->lockimg(&lockedTexture, stride, 0, TEXLOCK_WRITE))
    return;

  E3DCOLOR *colors = static_cast<E3DCOLOR *>(lockedTexture);
  for (int i = 0; i < TEXTURE_WIDTH; ++i, ++colors)
  {
    const float height = lerp(min_height, max_height, static_cast<float>(i) / (TEXTURE_WIDTH - 1));
    *colors = getGradientColor(gradient, height);
  }
  gradientTexture.getTex()->unlockimg();
}

void HeightmapDebugShading::updateGradientShader(const HeightmapColorGradient *gradient_to_show)
{
  static int heightmap_show_gradient_land_gvid = dagGeom->getShaderVariableId("heightmap_show_gradient_land");
  static int heightmap_gradient_land_params_gvid = dagGeom->getShaderVariableId("heightmap_gradient_land_params");
  static int heightmap_gradient_land_tex_gvid = dagGeom->getShaderVariableId("heightmap_gradient_land_tex");

  if (gradient_to_show)
  {
    const float minHeight = gradient_to_show->keys.front().height;
    const float maxHeight = gradient_to_show->keys.back().height;
    updateGradientdTexture(*gradient_to_show, minHeight, maxHeight);

    dagGeom->shaderGlobalSetInt(heightmap_show_gradient_land_gvid, 1);
    dagGeom->shaderGlobalSetTexture(heightmap_gradient_land_tex_gvid, gradientTexture.getId());

    const float range = maxHeight - minHeight;
    const float invRange = range > 0.0f ? (1.0f / range) : 0.0f;
    dagGeom->shaderGlobalSetColor4(heightmap_gradient_land_params_gvid, Color4(minHeight, invRange, 0, 0));
  }
  else
  {
    dagGeom->shaderGlobalSetInt(heightmap_show_gradient_land_gvid, 0);
    dagGeom->shaderGlobalSetTexture(heightmap_gradient_land_tex_gvid, BAD_TEXTUREID);
  }
}
