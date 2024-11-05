// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "postFxManager.h"
#define INSIDE_RENDERER 1 // fixme: move to jam
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>
#include "private_worldRenderer.h"
#include <render/fx/postFxEvent.h>
#include <render/fx/bloom_ps.h>
#include "render/world/global_vars.h"
#include <render/dof/dof_ps.h>
#include "render/deferredRenderer.h"
#include <util/dag_convar.h>
#include <render/genericLUT/colorGradingLUT.h>
#include <render/genericLUT/fullColorGradingLUT.h>
#include <render/hdrRender.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderBlock.h>
#include <image/dag_texPixel.h>
#include <render/world/postfxRender.h>

// for ECS events
#include "render/renderEvent.h"
#include <ecs/core/entityManager.h>

CONSOLE_BOOL_VAL("render", buildLUTEachFrame, false);
CONSOLE_BOOL_VAL("render", adaptationIlluminance, false);

PostFxManager::~PostFxManager() = default;
PostFxManager::PostFxManager() = default;

void PostFxManager::close()
{
  gradingLUT.reset();
  fullGradingLUT.reset();
  if (fireOnScreenTexture != BAD_TEXTUREID)
    release_managed_tex(fireOnScreenTexture);
  if (smokeBlackoutTexture != BAD_TEXTUREID)
    release_managed_tex(smokeBlackoutTexture);
  smokeBlackoutTexture = BAD_TEXTUREID;
  fireOnScreenTexture = BAD_TEXTUREID;
}

void PostFxManager::init(const WorldRenderer &world_renderer)
{
  if (hdrrender::is_hdr_enabled())
    gradingLUT = eastl::make_unique<ColorGradingLUT>(true);

  int lutSize = world_renderer.isLowResLUT() ? 16 : 32;
  fullGradingLUT = eastl::make_unique<FullColorGradingTonemapLUT>(hdrrender::is_hdr_enabled(), lutSize);

  int usePostfxVarId = ::get_shader_variable_id("use_postfx", true);
  if (usePostfxVarId != -1)
    ShaderGlobal::set_int(usePostfxVarId, world_renderer.hasFeature(FeatureRenderFlags::POSTFX));

  if (!world_renderer.hasFeature(FeatureRenderFlags::POSTFX))
  {
    debug("POSTFX disabled by render feature flag");
    G_ASSERTF(!world_renderer.hasFeature(FeatureRenderFlags::GPU_RESIDENT_ADAPTATION),
      "GPU_RESIDENT_ADAPTATION must be disabled, when POSTFX is disabled");
    G_ASSERTF(!world_renderer.hasFeature(FeatureRenderFlags::BLOOM), "BLOOM must be disabled, when POSTFX is disabled");
    G_ASSERTF(world_renderer.hasFeature(FeatureRenderFlags::FORWARD_RENDERING),
      "FORWARD_RENDERING must be enabled, when POSTFX is disabled");

    return;
  }

  g_entity_mgr->broadcastEventImmediate(ReloadPostFx());

  fireOnScreenTexture = ::get_tex_gameres("flamelet_atlas_8x8_a");
  ShaderGlobal::set_texture(::get_shader_variable_id("fire_on_screen_tex", true), fireOnScreenTexture);
  smokeBlackoutTexture = ::get_tex_gameres("smoke_puff_anim_loop_fluid_sparse_8x8_a");
  ShaderGlobal::set_texture(::get_shader_variable_id("smoke_blackout_tex", true), smokeBlackoutTexture);
}


void PostFxManager::setResolution(
  const IPoint2 &postfx_resolution, const IPoint2 &rendering_resolution, const WorldRenderer &world_renderer)
{
  G_ASSERT(postfx_resolution.x >= rendering_resolution.x && postfx_resolution.y >= rendering_resolution.y);
  G_UNUSED(rendering_resolution); // used in assert
  G_UNUSED(postfx_resolution);    // used in assert
  // base size is screen size to allow using MRT in postfx shader when sampling is not 1:1

  // since swapchain changed here we check if HDR got enabled or not and adjust tonemap accordingly
  if (!!gradingLUT != hdrrender::is_hdr_enabled())
    gradingLUT = hdrrender::is_hdr_enabled() ? eastl::make_unique<ColorGradingLUT>(true) : decltype(gradingLUT){};

  int lutSize = world_renderer.isLowResLUT() ? 16 : 32;
  if (!fullGradingLUT || fullGradingLUT->hasHDR() != hdrrender::is_hdr_enabled())
  {
    fullGradingLUT.reset();
    fullGradingLUT = eastl::make_unique<FullColorGradingTonemapLUT>(hdrrender::is_hdr_enabled(), lutSize);
  }
}


void WorldRenderer::beforeDrawPostFx()
{
  if (postfx.gradingLUT)
  {
    if (buildLUTEachFrame.get())
      postfx.gradingLUT->requestUpdate();
    postfx.gradingLUT->render();
  }
  if (postfx.fullGradingLUT)
  {
    if (buildLUTEachFrame.get())
      postfx.fullGradingLUT->requestUpdate();
    postfx.fullGradingLUT->render();
  }
}

void PostFxManager::prepare(const TextureIDPair &currentAntiAliasedTarget,
  ManagedTexView downsampled_frame,
  ManagedTexView closeDepth,
  ManagedTexView depth,
  float zn,
  float zf,
  float hk)
{
  TIME_D3D_PROFILE(postfx_prepare)
  ShaderGlobal::set_texture(frame_texVarId, currentAntiAliasedTarget.getId());

  if (downsampled_frame)
  {
    g_entity_mgr->broadcastEventImmediate(
      RenderPostFx(TextureIDPair(downsampled_frame.getBaseTex(), downsampled_frame.getTexId()), currentAntiAliasedTarget,
        TextureIDPair(closeDepth.getBaseTex(), closeDepth.getTexId()), TextureIDPair(depth.getTex2D(), depth.getTexId()), zn, zf, hk));
  }
}

void WorldRenderer::initPostFx() { postfx.init(*this); }

void WorldRenderer::initUiBlurFallback()
{
  if (hasFeature(FeatureRenderFlags::POSTFX))
    return;

  TexImage32 image[2];
  image[0].w = image[0].h = 1;

  reinterpret_cast<E3DCOLOR *>(image)[1] = ::dgs_get_game_params()->getE3dcolor("uiBlurFallbackColor", 0x62626262);

  const char *texname = "uiBlurFallbackTex";
  uiBlurFallbackTexId = dag::create_tex(image, 1, 1, TEXFMT_A8R8G8B8 | TEXCF_LOADONCE | TEXCF_SYSTEXCOPY, 1, texname);
}

void WorldRenderer::setPostFxResolution(int w, int h)
{
  if (!hasFeature(FeatureRenderFlags::POSTFX))
    return;

  IPoint2 rr;
  getRenderingResolution(rr.x, rr.y);
  postfx.setResolution(IPoint2(w, h), rr, *this);
}

void WorldRenderer::setFadeMul(float mul) { ::setFadeMul(mul); }

static void update_damage_indicator(const ecs::Object &postFx)
{
  const Point4 damage_indicator__color = postFx.getMemberOr(ECS_HASH("damage_indicator__color"), Point4(1, 0.3, 0.3, 0));
  const float damage_indicator__size = postFx.getMemberOr(ECS_HASH("damage_indicator__size"), 1.f);
  ShaderGlobal::set_color4(::get_shader_variable_id("damage_indicator_color", true), damage_indicator__color.x,
    damage_indicator__color.y, damage_indicator__color.z, damage_indicator__color.w);
  ShaderGlobal::set_real(::get_shader_variable_id("damage_indicator_size", true), damage_indicator__size);
}

void PostFxManager::set(const ecs::Object &postFx)
{
  eastl::array<ecs::HashedConstString, 17> depricatedFields = {ECS_HASH("bloom__on"), ECS_HASH("bloom__mul"),
    ECS_HASH("bloom__threshold"), ECS_HASH("bloom__radius"), ECS_HASH("bloom__upSample"), ECS_HASH("adaptation__on"),
    ECS_HASH("adaptation__autoExposureScale"), ECS_HASH("adaptation__highPart"), ECS_HASH("adaptation__maxExposure"),
    ECS_HASH("adaptation__minExposure"), ECS_HASH("adaptation__lowPart"), ECS_HASH("adaptation__adaptDownSpeed"),
    ECS_HASH("adaptation__adaptUpSpeed"), ECS_HASH("adaptation__brightnessPerceptionLinear"),
    ECS_HASH("adaptation__brightnessPerceptionPower"), ECS_HASH("adaptation__fixedExposure"), ECS_HASH("adaptation__fadeMul")};
  for (const ecs::HashedConstString &depricatedField : depricatedFields)
    if (postFx.hasMember(depricatedField))
      logerr("PostFxManager::set: %s is depricated", depricatedField.str);

  {
    const float increaseTime = postFx.getMemberOr("smoke_blackout_effect__increaseDuration", 3.0f);
    const float decreaseTime = postFx.getMemberOr("smoke_blackout_effect__decreaseDuration", 1.0f);
    const float maxIntensity = postFx.getMemberOr("smoke_blackout_effect__maxIntensity", 1.0f);
    const float minIntensity = postFx.getMemberOr("smoke_blackout_effect__minIntensity", 1.0f);
    ShaderGlobal::set_real(::get_shader_variable_id("smoke_blackout_effect_increase_duration", true), increaseTime);
    ShaderGlobal::set_real(::get_shader_variable_id("smoke_blackout_effect_decrease_duration", true), decreaseTime);
    ShaderGlobal::set_real(::get_shader_variable_id("smoke_blackout_effect_max_intensity", true), maxIntensity);
    ShaderGlobal::set_real(::get_shader_variable_id("smoke_blackout_effect_min_intensity", true), minIntensity);
    ShaderGlobal::set_int(::get_shader_variable_id("smoke_blackout_active", true), 0);
  }

  update_damage_indicator(postFx);
}

void WorldRenderer::setPostFx(const ecs::Object &postFx) { postfx.set(postFx); }

void WorldRenderer::postFxTonemapperChanged()
{
  if (postfx.gradingLUT)
    postfx.gradingLUT->requestUpdate();
  if (postfx.fullGradingLUT)
    postfx.fullGradingLUT->requestUpdate();
}

namespace texmgr_internal
{
extern bool dbg_texq_only_stubs;
} // namespace texmgr_internal
