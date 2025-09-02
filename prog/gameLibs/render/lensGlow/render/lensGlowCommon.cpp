// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <drv/3d/dag_tex3d.h>

#include "lensGlowCommon.h"

#define LENS_GLOW_VARS_LIST   \
  VAR(lens_glow_enabled)      \
  VAR(lens_glow_tex)          \
  VAR(lens_glow_tint)         \
  VAR(lens_glow_flare_params) \
  VAR(lens_glow_tex_params)   \
  VAR(lens_glow_bloom_params)

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
LENS_GLOW_VARS_LIST
#undef VAR

void validate_shadervars()
{
#define VAR(a)     \
  if (!(a##VarId)) \
    logerr("mandatory shader variable is missing: %s", #a);
  LENS_GLOW_VARS_LIST
#undef VAR
}

void apply_lens_glow_settings(bool enabled, const IPoint2 &display_resolution, float lens_glow_config__bloom_offset,
  float lens_glow_config__bloom_multiplier, float lens_glow_config__lens_flare_offset, float lens_glow_config__lens_flare_multiplier,
  const ecs::string &texture_name, float lens_glow_config__texture_intensity_multiplier, const Point3 &lens_glow_config__texture_tint)
{
  if (!enabled || texture_name.empty() || lens_glow_config__texture_intensity_multiplier <= 0)
  {
    ShaderGlobal::set_int(lens_glow_enabledVarId, 0);
    ShaderGlobal::set_texture(lens_glow_texVarId, BAD_TEXTUREID);
    return;
  }
  validate_shadervars();
  TEXTUREID texId = ::get_tex_gameres(texture_name.c_str(), true);
  if (texId == BAD_TEXTUREID)
  {
    logerr("Could not find lens glow texture: %s", texture_name);
    ShaderGlobal::set_int(lens_glow_enabledVarId, 0);
    ShaderGlobal::set_texture(lens_glow_texVarId, BAD_TEXTUREID);
    return;
  }
  if (BaseTexture *tex = D3dResManagerData::getD3dTex<D3DResourceType::TEX>(texId))
  {
    TextureInfo texInfo;
    tex->getinfo(texInfo);
    Point2 tc_scaling = Point2(float(display_resolution.x) / display_resolution.y * float(texInfo.h) / texInfo.w, 1);
    tc_scaling /= max(tc_scaling.x, tc_scaling.y);
    ShaderGlobal::set_color4(lens_glow_tex_paramsVarId, tc_scaling.x, tc_scaling.y, 0, 0);
  }
  ShaderGlobal::set_int(lens_glow_enabledVarId, 1);
  ShaderGlobal::set_texture(lens_glow_texVarId, texId);
  ShaderGlobal::set_color4(lens_glow_tintVarId, lens_glow_config__texture_tint * lens_glow_config__texture_intensity_multiplier, 0);
  ShaderGlobal::set_color4(lens_glow_flare_paramsVarId, lens_glow_config__lens_flare_offset, lens_glow_config__lens_flare_multiplier,
    0, 0);
  ShaderGlobal::set_color4(lens_glow_bloom_paramsVarId, lens_glow_config__bloom_offset, lens_glow_config__bloom_multiplier, 0, 0);
  ::release_managed_tex(texId); // ShaderGlobal holds the reference now
}

ecs::string pick_best_texture(const ecs::Array &texture_variants, const IPoint2 &display_resolution)
{
  if (texture_variants.empty())
    return "";
  if (display_resolution.x * display_resolution.y == 0)
    return "";

  float bestTexStretch = 1;
  int bestExcessTexArea = 0;
  ecs::string bestTexture;
  for (const auto &variant : texture_variants)
  {
    const ecs::Object &variantObj = variant.get<ecs::Object>();
    IPoint2 targetRes = IPoint2(0, 0);
    ecs::string textureName;

    if (auto value = variantObj.getNullable<IPoint2>(ECS_HASH("target_res")))
      targetRes = *value;
    if (auto value = variantObj.getNullable<ecs::string>(ECS_HASH("texture")))
      textureName = *value;

    if (targetRes.x * targetRes.y == 0)
    {
      logerr("Lens glow texture (%s) has invalid/missing target resolution. Specify `target_res:i2=...` in blk", textureName);
      continue;
    }

    if (textureName.empty())
    {
      logerr("Lens glow texture (target resolution = %dx%d) has invalid/missing texture name. Specify `texture:t=...` in blk",
        targetRes.x, targetRes.y);
      continue;
    }

    float stretch = max(float(display_resolution.x) / targetRes.x, float(display_resolution.y) / targetRes.y);
    int excessTexArea = targetRes.x * targetRes.y - display_resolution.x * display_resolution.y;

    if (bestTexture.empty() || (bestTexStretch > 1 && stretch < bestTexStretch) || (stretch <= 1 && excessTexArea < bestExcessTexArea))
    {
      bestTexture = textureName;
      bestTexStretch = stretch;
      bestExcessTexArea = excessTexArea;
    }
  }

  return bestTexture;
}
