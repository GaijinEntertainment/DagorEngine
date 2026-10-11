// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math.h>
#include <stdio.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_decl.h>
#include <math/dag_color.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_log.h>
#include <render/hdrRender.h>
#include <render/specialVision/nightVisionPostFx.h>

#define BLUR_SAMPLES 8

constexpr const char *thermal_resolve_shader = "deferred_thermal_resolve";
constexpr const char *glow_downsample_shader = "night_vision_downsample";
constexpr const char *glow_blur_shader = "night_vision_blur";
constexpr const char *glow_combine_shader = "night_vision_combine";

static int texVarId = -1;
static int glob_night_vision_downsample_texVarId = -1;
static int glob_night_vision_combine_texVarId = -1;
static int texTmVarId[BLUR_SAMPLES];

static d3d::SamplerInfo glow_sampler_info(d3d::FilterMode filter_mode)
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.filter_mode = filter_mode;
  return smpInfo;
}

static void request_glow_samplers()
{
  ShaderGlobal::set_sampler(::get_shader_variable_id("glob_night_vision_downsample_tex_samplerstate", true),
    d3d::request_sampler(glow_sampler_info(d3d::FilterMode::Point)));
  ShaderGlobal::set_sampler(::get_shader_variable_id("glob_night_vision_combine_tex_samplerstate", true),
    d3d::request_sampler(glow_sampler_info(d3d::FilterMode::Linear)));
}

void render::special_vision::set_special_vision_shader_mode(bool on)
{
  static const int special_visionVarId = ::get_shader_variable_id("special_vision", true);
  ShaderGlobal::set_int(special_visionVarId, on ? 1 : 0);
}

NightVisionPostFx::NightVisionPostFx()
{
  currentResolution = IPoint2(640, 360);
  sourceResolution = IPoint2(640, 360);
  baseResolution = IPoint2(1920, 1080);
  glowResolution = IPoint2(640, 360);
  currentTexIndex = 0;
}

NightVisionPostFx::~NightVisionPostFx() { releaseTargets(); }

bool NightVisionPostFx::init(bool full_deferred)
{
  isInitialized = false;
  currentTexIndex = 0;
  fullDeferred = full_deferred;
  applyFullscreen.init(glow_combine_shader);
  blur.init(glow_blur_shader);
  downsample.init(glow_downsample_shader);
  if (!applyFullscreen.getMat() || !blur.getMat() || !downsample.getMat())
    return false;

  texVarId = ::get_shader_variable_id("tex", true);
  glob_night_vision_downsample_texVarId = ::get_shader_variable_id("glob_night_vision_downsample_tex", true);
  glob_night_vision_combine_texVarId = ::get_shader_variable_id("glob_night_vision_combine_tex", true);

  {
    d3d::SamplerHandle smp = d3d::request_sampler(glow_sampler_info(d3d::FilterMode::Point));
    blur.getMat()->set_sampler_param(::get_shader_variable_id("tex_samplerstate", true), smp);
    blur.getMat()->set_sampler_param(::get_shader_variable_id("prevGlowTex_samplerstate", true), smp);
  }
  {
    d3d::SamplerHandle smp = d3d::request_sampler(glow_sampler_info(d3d::FilterMode::Linear));
    applyFullscreen.getMat()->set_sampler_param(::get_shader_variable_id("glowTex_samplerstate", true), smp);
  }
  if (fullDeferred)
  {
    resolveShader.init(thermal_resolve_shader);
    if (!resolveShader.getMat())
      return false;
  }

  char name[64];
  for (int i = 0; i < BLUR_SAMPLES; ++i)
  {
    snprintf(name, sizeof(name), "texTm%d", i);
    texTmVarId[i] = get_shader_variable_id(name);
  }
  isInitialized = true;
  return true;
}

void NightVisionPostFx::resolve()
{
  if (fullDeferred)
    resolveShader.render();
}

void NightVisionPostFx::releaseTargets()
{
  static int glowTexVarId = ::get_shader_variable_id("glowTex", true);
  static int prevGlowTexVarId = ::get_shader_variable_id("prevGlowTex", true);

  if (applyFullscreen.getMat())
    applyFullscreen.getMat()->set_texture_param(glowTexVarId, BAD_TEXTUREID);

  if (blur.getMat())
  {
    blur.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);
    blur.getMat()->set_texture_param(prevGlowTexVarId, BAD_TEXTUREID);
  }

  ShaderGlobal::set_texture(glob_night_vision_downsample_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(glob_night_vision_combine_texVarId, BAD_TEXTUREID);

  ShaderGlobal::set_sampler(::get_shader_variable_id("glob_night_vision_downsample_tex_samplerstate", true),
    d3d::INVALID_SAMPLER_HANDLE);
  ShaderGlobal::set_sampler(::get_shader_variable_id("glob_night_vision_combine_tex_samplerstate", true), d3d::INVALID_SAMPLER_HANDLE);

  glowTex[0].close();
  glowTex[1].close();
  tempTex.close();
  downsampleTex.close();

  state = render::special_vision::OFF;
}

void NightVisionPostFx::initTargets(int ch_state, int sizeX, int sizeY, int targetsizeX, int targetsizeY)
{
  releaseTargets();
  // actual resolution will be stretched due to screen aspect, so we store source resolution
  sourceResolution = IPoint2(sizeX, sizeY);

  float aspectStrain = ((float)targetsizeX / (float)targetsizeY) / ((float)sizeX / (float)sizeY);

  sizeX = aspectStrain > 1 ? ceilf(sizeX * aspectStrain) : sizeX;
  sizeY = aspectStrain < 1 ? ceilf(sizeY / aspectStrain) : sizeY;

  currentResolution = IPoint2(sizeX, sizeY);
  baseResolution = IPoint2(targetsizeX, targetsizeY);
  glowResolution =
    ch_state == render::special_vision::RENDER_THERMAL_VISION ? IPoint2(sizeX, sizeY) : IPoint2(targetsizeX / 3, targetsizeY / 3);

  state = ch_state;
  if (state == render::special_vision::OFF)
    return;
  if (!isInitialized || !applyFullscreen.getMat() || !blur.getMat() || !downsample.getMat())
  {
    state = render::special_vision::OFF;
    return;
  }

  request_glow_samplers();

  unsigned int flags = TEXCF_RTARGET;
  unsigned usage = d3d::USAGE_FILTER | d3d::USAGE_RTARGET;

  if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & usage) == usage)
    flags |= TEXFMT_G16R16F;
  else if ((d3d::get_texformat_usage(TEXFMT_G32R32F) & usage) == usage)
    flags |= TEXFMT_G32R32F;
  else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & usage) == usage)
    flags |= TEXFMT_A16B16G16R16F;
  else if ((d3d::get_texformat_usage(TEXFMT_A32B32G32R32F) & usage) == usage)
    flags |= TEXFMT_A32B32G32R32F;
  else
  {
    if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags |= TEXFMT_G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_G32R32F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags |= TEXFMT_G32R32F;
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags |= TEXFMT_A16B16G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_A32B32G32R32F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags |= TEXFMT_A32B32G32R32F;
    else
      logerr("nightvision falling to argb8");
  }

  glowTex[0] = dag::create_tex(NULL, glowResolution.x, glowResolution.y, flags, 1, "nightVisionGlowTex0", "nightvision");
  glowTex[1] = dag::create_tex(NULL, glowResolution.x, glowResolution.y, flags, 1, "nightVisionGlowTex1", "nightvision");
  tempTex = dag::create_tex(NULL, glowResolution.x, glowResolution.y, flags, 1, "nightVisionTempTex", "nightvision");
  if (state == render::special_vision::RENDER_NIGHT_VISION && currentResolution != baseResolution)
  {
    flags = TEXCF_RTARGET;
    if ((d3d::get_texformat_usage(TEXFMT_R16F) & usage) == usage)
      flags |= TEXFMT_R16F;
    else
      flags |= TEXFMT_R8;
    downsampleTex =
      dag::create_tex(NULL, currentResolution.x, currentResolution.y, flags, 1, "nightVisionDownsampleTex", "nightvision");
  }

  SCOPE_RENDER_TARGET;
  d3d::set_render_target({}, DepthAccess::RW, {{glowTex[0].getTex2D(), 0, 0}});
  d3d::clearview(CLEAR_TARGET, 0, 0.0f, 0);
  d3d::set_render_target({}, DepthAccess::RW, {{glowTex[1].getTex2D(), 0, 0}});
  d3d::clearview(CLEAR_TARGET, 0, 0.0f, 0);

  invalidatedPrevFrameTextures = false;
}

void NightVisionPostFx::invalidatePrevFrameTextures() { invalidatedPrevFrameTextures = true; }

void NightVisionPostFx::applySettings(float ghosting, float thermal_ghosting, Point3 nv_baseColor, Point3 nv_midColor,
  Point3 nv_brightColor, float nv_lightMultiplier, float nv_noiseFactor, bool black_is_hot)
{
  static int glowGhostingVarId = ::get_shader_variable_id("glowGhosting", true);
  static int nightVisionBaseColorVarId = ::get_shader_variable_id("nightVisionBaseColor", true);
  static int nightVisionMidColorVarId = ::get_shader_variable_id("nightVisionMidColor", true);
  static int nightVisionBrightColorVarId = ::get_shader_variable_id("nightVisionBrightColor", true);
  if (state == render::special_vision::RENDER_NIGHT_VISION)
  {
    static int nightLightMultiplierVarId = ::get_shader_variable_id("night_light_multiplier", true);
    static int noiseFactorVarId = ::get_shader_variable_id("noiseFactor", true);

    if (!fullDeferred)
    {
      nv_lightMultiplier *= 0.7f;
      nv_noiseFactor *= 0.3f;
    }
    ShaderGlobal::set_float(nightLightMultiplierVarId, nv_lightMultiplier);
    ShaderGlobal::set_float(glowGhostingVarId, ghosting);
    ShaderGlobal::set_float(noiseFactorVarId, nv_noiseFactor);
    ShaderGlobal::set_float4(nightVisionBaseColorVarId, nv_baseColor.x, nv_baseColor.y, nv_baseColor.z, 0);
    ShaderGlobal::set_float4(nightVisionMidColorVarId, nv_midColor.x, nv_midColor.y, nv_midColor.z, 0);
    ShaderGlobal::set_float4(nightVisionBrightColorVarId, nv_brightColor.x, nv_brightColor.y, nv_brightColor.z, 0);
  }
  else if (state == render::special_vision::RENDER_THERMAL_VISION)
  {
    static int blackIsHotVarId = ::get_shader_variable_id("blackIsHot", true);
    ShaderGlobal::set_int(blackIsHotVarId, black_is_hot ? 1 : 0);
    ShaderGlobal::set_float(glowGhostingVarId, thermal_ghosting);
    ShaderGlobal::set_float4(nightVisionBaseColorVarId, nv_baseColor.x, nv_baseColor.y, nv_baseColor.z, 0);
    ShaderGlobal::set_float4(nightVisionMidColorVarId, nv_midColor.x, nv_midColor.y, nv_midColor.z, 0);
    ShaderGlobal::set_float4(nightVisionBrightColorVarId, nv_brightColor.x, nv_brightColor.y, nv_brightColor.z, 0);
  }
}

void NightVisionPostFx::apply(Texture *src_tex, Texture *dest_tex)
{
  if (!isInitialized || !src_tex || !hasTargets())
    return;

  TIME_D3D_PROFILE(night_vision_postfx);

  d3d::resource_barrier({src_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  Driver3dRenderTarget rt;
  d3d::get_render_target(rt);

  if (invalidatedPrevFrameTextures)
  {
    d3d::clear_rt({glowTex[1 - currentTexIndex].getTex2D(), 0}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
    invalidatedPrevFrameTextures = false;
  }

  static int glowTexVarId = ::get_shader_variable_id("glowTex", true);
  static int prevGlowTexVarId = ::get_shader_variable_id("prevGlowTex", true);
  static int tcStepsXYVarId = ::get_shader_variable_id("tcStepsXY", true);
  static int downsampleModeVarId = ::get_shader_variable_id("downsampleMode", true);

  ShaderGlobal::set_float4(tcStepsXYVarId, 1.0f / (float)baseResolution.x, 1.0f / (float)baseResolution.y,
    ceilf((float)baseResolution.x / (float)glowResolution.x), ceilf((float)baseResolution.y / (float)glowResolution.y));

  d3d::set_render_target({}, DepthAccess::RW, {{glowTex[currentTexIndex].getTex2D(), 0, 0}});

  ShaderGlobal::set_texture(glob_night_vision_downsample_texVarId, src_tex);
  ShaderGlobal::set_int(downsampleModeVarId, 1);
  downsample.render();
  ShaderGlobal::set_texture(glob_night_vision_downsample_texVarId, BAD_TEXTUREID);
  d3d::resource_barrier({glowTex[currentTexIndex].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  static int blurStageVarId = ::get_shader_variable_id("blur_stage", true);
  ShaderGlobal::set_int(blurStageVarId, 0);
  static int blurScaleVarId = ::get_shader_variable_id("blur_scale", true);
  ShaderGlobal::set_int(blurScaleVarId, 0);

  float blurPixelSizeX = 1.0f / (float)currentResolution.x;
  float blurPixelSizeY = 1.0f / (float)currentResolution.y;

  blur.getMat()->set_texture_param(prevGlowTexVarId, glowTex[1 - currentTexIndex].getTexId());

  for (int k = 0; k < 3; k++)
  {
    TIME_D3D_PROFILE(blur_pass);
    float mult;

    // blur horisontally
    mult = 1;
    for (int i = 0; i < BLUR_SAMPLES; i += 2)
    {
      blur.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, mult * blurPixelSizeX, 0));
      blur.getMat()->set_color4_param(texTmVarId[i + 1], Color4(1, 1, -mult * blurPixelSizeX, 0));
      mult++;
    }

    d3d::set_render_target({}, DepthAccess::RW, {{tempTex.getTex2D(), 0, 0}});
    blur.getMat()->set_texture_param(texVarId, glowTex[currentTexIndex].getTexId());
    blur.render();

    d3d::resource_barrier({tempTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    ShaderGlobal::set_int(blurStageVarId, (k < 2 || (dest_tex != NULL)) ? 1 : 2);

    // blur vertically
    mult = 1;
    for (int i = 0; i < BLUR_SAMPLES; i += 2)
    {
      blur.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, 0, mult * blurPixelSizeY));
      blur.getMat()->set_color4_param(texTmVarId[i + 1], Color4(1, 1, 0, -mult * blurPixelSizeY));
      mult++;
    }

    d3d::set_render_target({}, DepthAccess::RW, {{glowTex[currentTexIndex].getTex2D(), 0, 0}});
    blur.getMat()->set_texture_param(texVarId, tempTex.getTexId());
    blur.render();
    d3d::resource_barrier({glowTex[currentTexIndex].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    ShaderGlobal::set_int(blurScaleVarId, 1);
  }
  blur.getMat()->set_texture_param(prevGlowTexVarId, BAD_TEXTUREID);
  blur.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);

  // final apply

  d3d::set_render_target();

  if (state == render::special_vision::RENDER_NIGHT_VISION)
  {
    if (!downsampleTex.getTex2D() || baseResolution == currentResolution)
    {
      ShaderGlobal::set_texture(glob_night_vision_combine_texVarId, src_tex);
    }
    else
    {
      TIME_D3D_PROFILE(downsample_pass);
      ShaderGlobal::set_float4(tcStepsXYVarId, 1.0f / (float)baseResolution.x, 1.0f / (float)baseResolution.y,
        ceilf((float)baseResolution.x / (float)currentResolution.x), ceilf((float)baseResolution.y / (float)currentResolution.y));

      d3d::set_render_target({}, DepthAccess::RW, {{downsampleTex.getTex2D(), 0, 0}});

      ShaderGlobal::set_texture(glob_night_vision_downsample_texVarId, src_tex);
      ShaderGlobal::set_int(downsampleModeVarId, 0);
      downsample.render();
      d3d::resource_barrier({downsampleTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      ShaderGlobal::set_texture(glob_night_vision_downsample_texVarId, BAD_TEXTUREID);

      ShaderGlobal::set_texture(glob_night_vision_combine_texVarId, downsampleTex.getBaseTex());
    }
    applyFullscreen.getMat()->set_texture_param(glowTexVarId, glowTex[currentTexIndex].getTexId());
  }
  else
    ShaderGlobal::set_texture(glob_night_vision_combine_texVarId, glowTex[currentTexIndex].getTex2D());

  if (dest_tex)
    d3d::set_render_target({}, DepthAccess::RW, {{dest_tex, 0, 0}});
  else
    hdrrender::set_render_target();

  TIME_D3D_PROFILE(apply_pass);

  if (!fullDeferred && state == render::special_vision::RENDER_NIGHT_VISION)
    d3d::set_srgb_backbuffer_write(true);
  applyFullscreen.render();
  if (!fullDeferred && state == render::special_vision::RENDER_NIGHT_VISION)
    d3d::set_srgb_backbuffer_write(false);

  currentTexIndex = (currentTexIndex + 1) % 2;

  ShaderGlobal::set_texture(glob_night_vision_combine_texVarId, BAD_TEXTUREID);
  applyFullscreen.getMat()->set_texture_param(glowTexVarId, BAD_TEXTUREID);

  d3d::set_render_target(rt);
}
