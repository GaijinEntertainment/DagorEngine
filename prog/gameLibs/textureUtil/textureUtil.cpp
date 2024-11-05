// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textureUtil/textureUtil.h"

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint4.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>

bool texture_util::rotate_texture(TEXTUREID source_texture_id, BaseTexture *dest_texture, TextureRotation rotation)
{
  PostFxRenderer postFxRenderer;
  postFxRenderer.init("texture_util_rotate");
  if (!postFxRenderer.getElem())
    return false;

  int sourceTexVarId = ::get_shader_variable_id("texture_util_source_texture", /*is_optional*/ true);
  if (sourceTexVarId < 0)
    return false;
  ShaderGlobal::set_texture(sourceTexVarId, source_texture_id);

  alignas(16) IPoint4 rot((int)rotation, 0, 0, 0);
  d3d::set_vs_const(10, (const float *)&rot, 1);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target(dest_texture, 0);

  postFxRenderer.render();

  return true;
}

static bool stitch_textures_impl(dag::ConstSpan<BaseTexture *> source_textures, BaseTexture *dest_texture, bool is_vertical)
{
  if (source_textures.empty() || !dest_texture)
    return false;

  TextureInfo texInfo;
  int offset = 0;
  for (size_t i = 0; i < source_textures.size(); ++i)
  {
    if (!source_textures[i])
      return false;
    source_textures[i]->getinfo(texInfo);

    if (!dest_texture->updateSubRegion(source_textures[i], 0, 0, 0, 0, texInfo.w, texInfo.h, 1, 0, is_vertical ? 0 : offset,
          is_vertical ? offset : 0, 0))
      return false;
    offset += is_vertical ? texInfo.h : texInfo.w;
  }

  return true;
}

bool texture_util::stitch_textures_horizontal(dag::ConstSpan<BaseTexture *> source_textures, BaseTexture *dest_texture)
{
  return stitch_textures_impl(source_textures, dest_texture, false);
}

bool texture_util::stitch_textures_vertical(dag::ConstSpan<BaseTexture *> source_textures, BaseTexture *dest_texture)
{
  return stitch_textures_impl(source_textures, dest_texture, true);
}

TexPtr texture_util::stitch_textures_horizontal(dag::ConstSpan<TEXTUREID> source_texture_ids, const char *dest_tex_name, int flags)
{
  if (source_texture_ids.empty())
    return TexPtr();

  Tab<BaseTexture *> source_textures;
  TextureInfo texInfo;
  int srcFormat = TEXFMT_DEFAULT;
  int destWidth = 0, destHeight = 0;
  for (size_t i = 0; i < source_texture_ids.size(); ++i)
  {
    BaseTexture *tex = acquire_managed_tex(source_texture_ids[i]);
    if (!tex || get_managed_res_cur_tql(source_texture_ids[i]) == TQL_stub)
    {
      for (size_t j = 0; j < source_textures.size(); ++j)
        release_managed_tex(source_texture_ids[j]);
      return TexPtr();
    }
    source_textures.push_back(tex);
    tex->getinfo(texInfo);

    int iterFormat = texInfo.cflg & TEXFMT_MASK;
    if ((i > 0) && (srcFormat != iterFormat))
    {
      logerr("stitch_textures_horizontal: different src texture formats %s and %s, data can be corrupted!",
        get_tex_format_name(srcFormat), get_tex_format_name(iterFormat));
    }
    else
      srcFormat = iterFormat;

    destWidth += texInfo.w;
    destHeight = max(destHeight, (int)texInfo.h);
  }

  int targetFormat = flags & TEXFMT_MASK;
  if ((srcFormat != targetFormat) && (targetFormat == TEXFMT_DEFAULT))
  {
    debug("stitch_textures_horizontal: updating target format from default(%s) to %s", get_tex_format_name(flags),
      get_tex_format_name(srcFormat));
    flags = (flags & ~TEXFMT_MASK) | srcFormat;
  }

  TexPtr combinedTex =
    dag::create_tex(nullptr, destWidth, destHeight, flags | TEXCF_CLEAR_ON_CREATE | TEXCF_UPDATE_DESTINATION, 1, dest_tex_name);
  G_ASSERT(combinedTex);
  if (combinedTex)
    texture_util::stitch_textures_horizontal(source_textures, combinedTex.get());

  for (int j = 0; j < source_textures.size(); ++j)
    release_managed_tex(source_texture_ids[j]);
  return combinedTex;
}
