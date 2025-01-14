// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <util/dag_globDef.h>
#include <util/dag_lag.h>
#include <math/random/dag_random.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_platform.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_colorMatrix.h>
#include <render/fx/dag_demonPostFx.h>
#include <fx/dag_hdrRender.h>
#include <startup/dag_globalSettings.h>
#include <supp/dag_prefetch.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <util/dag_convar.h>

#define PROFILE_DEMONPOSTFX 0
#define HIST_USE_INSTANCING 0 // instancing works AWFULLY SLOW
#if PROFILE_DEMONPOSTFX
#include <perfMon/dag_pix.h>
#define PROFILE_START(name) perfmon::begin_named_event(name);
#define PROFILE_END()       perfmon::end_named_event();
#else
#define PROFILE_START(name)
#define PROFILE_END()
#endif // PROFILE_DEMONPOSTFX

#include <shaders/dag_DynamicShaderHelper.h>

// #include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>

static int darkThresholdVarId = -1, darkThreshold0VarId = -1, darkThreshold1VarId = -1;
static int texVarId = -1, texSzVarId = -1, texUvTransformVarId = -1;
static int glowTexVarId = -1, glowTexSzVarId = -1, glowScaleVarId = -1, hasPostFxGlowVarId = -1;
static int volfogTexVarId = -1, volfogScaleVarId = -1;
static int volFogOnVarId = -1;
static int srcTexVarId = -1;
static int vignette_multiplierGlobVarId = -1;
static int vignette_start_endGlobVarId = -1;
static int hdr_overbrightGVarId = -1;
static int texelOffsetXVarId = -1, texelOffsetYVarId = -1;
static int targetTextureSizeVarId = -1;
static int ldr_vignette_multiplierVarId = -1;
static int ldr_vignette_widthVarId = -1;
static int ldr_vignette_aspectVarId = -1;

// DoF variables
static int max_hdr_overbrightGVarId = -1;

static int colorMatrixRVarId = -1, colorMatrixGVarId = -1, colorMatrixBVarId = -1;
static int renderCombinePassGVarId = -1;

#define BLUR_SAMPLES 8
static int weight0VarId, weight1VarId;
static int weightVarId[BLUR_SAMPLES], texTmVarId[BLUR_SAMPLES];

static DemonPostFx *postFx = NULL;

static inline Color4 quadCoeffs0001(0.5f, -0.5f, 0.50001f, 0.50001f);
static inline Color4 quadCoeffs(0.5f, -0.5f, 0.5000f, 0.5000f);

DemonPostFx::DemonPostFx(const DataBlock &main_blk, const DataBlock &adaptation_blk, int target_w, int target_h,
  unsigned rtarget_flags, bool initPostfxGlow, bool initSunVolfog, bool initUIBlur) :
  paramColorMatrix(1),
  preColorMatrix(1),
  postColorMatrix(1),
  useSimpleMode(false),
  combineCallback(NULL),
  skyMaskPrepared(false),
  volFogCallback(NULL),
  eventFinished(false),
  externalSkyMaskTexId(0)
{
  const unsigned int workingFlags = d3d::USAGE_FILTER | d3d::USAGE_BLEND | d3d::USAGE_RTARGET;
  unsigned sky_fmt = TEXFMT_DEFAULT;
  if ((d3d::get_texformat_usage(TEXFMT_R11G11B10F) & workingFlags) == workingFlags)
    sky_fmt = TEXFMT_R11G11B10F;

  // This needs to be changed so we should check for d3d::get_texformat_usage and compute on all platforms.
  useCompute = d3d::should_use_compute_for_image_processing({rtarget_flags, sky_fmt});
  useSimpleMode = (::hdr_render_mode == HDR_MODE_NONE);

  hdr_overbrightGVarId = ::get_shader_glob_var_id("hdr_overbright", true);
  darkThresholdVarId = ::get_shader_variable_id("darkThreshold", true);
  darkThreshold0VarId = ::get_shader_variable_id("darkThreshold0");
  darkThreshold1VarId = ::get_shader_variable_id("darkThreshold1");

  vignetteMul = 1.0;
  vignetteStart = Point2(1, 0.7);
  vignetteEnd = Point2(1.5, 1.01);

  resSize = IPoint2(target_w, target_h);
  lowResSize.x = target_w / 4;
  lowResSize.y = target_h / 4;
  if (useCompute)
  {
    // When using compute shaders, we need to have a multiple of 8 so we can
    // use 8x8 thread groups without checking for out-of-bounds access
    lowResSize.x &= ~7;
    lowResSize.y &= ~7;
  }

  if ((!lowResSize.x || !lowResSize.y) && !useSimpleMode)
  {
    LOGWARN_CTX("switched to useSimpleMode/HDR_MODE_NONE due to lowResSize=%dx%d (target=%dx%d)", lowResSize.x, lowResSize.y, target_w,
      target_h);
    useSimpleMode = true;
    ::hdr_render_mode = HDR_MODE_NONE;
  }

  texVarId = get_shader_variable_id("tex");
  texSzVarId = get_shader_variable_id("texsz_consts");
  texUvTransformVarId = get_shader_variable_id("texUvTransform");

  colorMatrixRVarId = get_shader_variable_id("colorMatrixR", true);
  colorMatrixGVarId = get_shader_variable_id("colorMatrixG", true);
  colorMatrixBVarId = get_shader_variable_id("colorMatrixB", true);

  vignette_multiplierGlobVarId = ::get_shader_glob_var_id("vignette_multiplier", true);
  vignette_start_endGlobVarId = ::get_shader_glob_var_id("vignette_start_end", true);

  ldr_vignette_multiplierVarId = get_shader_variable_id("ldr_vignette_multiplier", true);
  ldr_vignette_widthVarId = get_shader_variable_id("ldr_vignette_width", true);
  ldr_vignette_aspectVarId = get_shader_variable_id("ldr_vignette_aspect", true);


  if (!useSimpleMode)
  {
    if (useCompute)
    {
      glowBlurXFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlurYFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlur2XFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));
      glowBlur2YFxCS.reset(new_compute_shader("demon_postfx_blur_cs", true));

      if (!glowBlurXFxCS)
        useCompute = false;
    }

    if (initSunVolfog)
    {
      lowResLumTexA = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_lowresLumA");
      d3d_err(lowResLumTexA.getTex2D());
      lowResLumTexA->texaddr(TEXADDR_MIRROR);

      lowResLumTexB = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_lowresLumB");
      d3d_err(lowResLumTexB.getTex2D());
      lowResLumTexB->texaddr(TEXADDR_MIRROR);
    }

    if (initUIBlur)
    {
      uiBlurTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | TEXCF_RTARGET, 1, "postfx_uiBlur");
      d3d_err(uiBlurTex.getTex2D());
      uiBlurTex->texaddr(TEXADDR_CLAMP);
    }

    uint32_t usageFlag = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET;

    if (initPostfxGlow)
    {
      tmpTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, sky_fmt | usageFlag, 1, "postfx_tmp");
      d3d_err(tmpTex.getTex2D());
      tmpTex->texaddr(TEXADDR_CLAMP);

      glowTex = dag::create_tex(NULL, lowResSize.x, lowResSize.y, rtarget_flags | usageFlag, 1, "postfx_glow");
      d3d_err(glowTex.getTex2D());
      glowTex->texaddr(TEXADDR_CLAMP);
    }

    const bool useAdaptation = adaptation_blk.getBool("useAdaptation", true);

    if (initSunVolfog || initPostfxGlow || lenseFlareEnabled || useAdaptation)
    {
      // low-res previous frame texture
      prevFrameLowResTex =
        dag::create_tex(NULL, lowResSize.x, lowResSize.y, rtarget_flags | TEXCF_RTARGET, 1, "postfx_prevFrameLowRes");
      d3d_err(prevFrameLowResTex.getTex2D());
      prevFrameLowResTex->texaddr(TEXADDR_CLAMP);
    }

    glowBlurXFx.init("demon_postfx_blur");
    glowBlurYFx.init("demon_postfx_blur");
    glowBlur2XFx.init("demon_postfx_blur");
    glowBlur2YFx.init("demon_postfx_blur");
    radialBlur1Fx.init("demon_postfx_blur");
    radialBlur2Fx.init("demon_postfx_blur");
    downsample4x.init("downsample4x");

    glowTexSzVarId = get_shader_variable_id("glow_texsz_consts");
    targetTextureSizeVarId = get_shader_variable_id("targetTextureSize", true);
    glowTexVarId = get_shader_variable_id("glowTex");
    glowScaleVarId = get_shader_variable_id("glowScale");
    hasPostFxGlowVarId = get_shader_variable_id("hasPostFxGlow");
    volfogTexVarId = get_shader_variable_id("volfogTex");
    volfogScaleVarId = get_shader_variable_id("volfogScale");
    volFogOnVarId = get_shader_variable_id("volFogOn", true);
    srcTexVarId = get_shader_variable_id("srcTex");
    texelOffsetXVarId = ::get_shader_variable_id("texelOffsetX", true);
    texelOffsetYVarId = ::get_shader_variable_id("texelOffsetY", true);

    weight0VarId = get_shader_variable_id("weight0");
    weight1VarId = get_shader_variable_id("weight1");
    char name[64];
    for (int i = 0; i < BLUR_SAMPLES; ++i)
    {
      snprintf(name, sizeof(name), "weight%d", i);
      weightVarId[i] = get_shader_variable_id(name);
      snprintf(name, sizeof(name), "texTm%d", i);
      texTmVarId[i] = get_shader_variable_id(name);
    }

    Color4 quad_coefs = quadCoeffs;

    // init glow blur
    glowBlurXFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlurYFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlur2XFx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    glowBlur2YFx.getMat()->set_color4_param(texSzVarId, quad_coefs);

    // init radial blur
    radialBlur1Fx.getMat()->set_color4_param(texSzVarId, quad_coefs);
    radialBlur2Fx.getMat()->set_color4_param(texSzVarId, quad_coefs);
  }

  if (!updateSettings(main_blk))
  {
    G_ASSERT(0 && "updateSettings() must return true in ctor!");
  }

  combineFx.init(useSimpleMode ? "demon_postfx_combine_simple" : "demon_postfx_combine");

  postFx = this;
  max_hdr_overbrightGVarId = ::get_shader_glob_var_id("max_hdr_overbright", true);
  ShaderGlobal::set_real_fast(max_hdr_overbrightGVarId, ::hdr_max_overbright);
}

void DemonPostFx::closeLenseFlare() { lensFlare.close(); }

void DemonPostFx::initLenseFlare(const char *lense_covering_tex_name, const char *lense_radial_tex_name)
{
  if (::hdr_render_mode != HDR_MODE_FLOAT)
    return;
  lensFlare.init(lowResSize, lense_covering_tex_name, lense_radial_tex_name);
}

bool DemonPostFx::updateSettings(const DataBlock &main_blk)
{
  if (useSimpleMode != (::hdr_render_mode == HDR_MODE_NONE))
    return false;

  showAlpha = main_blk.getBool("showAlpha", false);

  glowEdgeK = main_blk.getReal("glowEdgeK", 0.1f);
  glowFadeK = main_blk.getReal("glowFadeK", 1);

  glowAngle[0] = main_blk.getReal("glowAngleU", 45);
  glowAngle[1] = main_blk.getReal("glowAngleV", 135);

  current.glowRadius = main_blk.getReal("glowRadius", 0.05f);

  current.volfogFade = main_blk.getReal("volfogFade", 0.005f);
  current.volfogRange = main_blk.getReal("volfogRange", 7.0f);
  current.volfogMul = main_blk.getReal("volfogMul", 1.0f);
  current.volfogColor = color3(main_blk.getE3dcolor("volfogColor", E3DCOLOR(255, 255, 255)));
  current.volfogMaxAngle = DegToRad(main_blk.getReal("volfogMaxAngle", 70));
  volfogMaxAngleCos = cosf(current.volfogMaxAngle);

  screenAspect = main_blk.getReal("screenAspect", 0.75f);

  current.hdrDarkThreshold = main_blk.getReal("hdrDarkThreshold", 1.5f);
  current.hdrGlowMul = main_blk.getReal("hdrGlowMul", 1.0f);
  current.hdrGlowPower = main_blk.getReal("hdrGlowPower", 1.0f);

  current.setSunDir(dir_from_polar(DegToRad(90 - main_blk.getReal("sunAzimuth", 0)), DegToRad(main_blk.getReal("sunZenith", 45))));

  // init color matrix
  const DataBlock &cmBlk = *main_blk.getBlockByNameEx("colorMatrix");

  current.hueShift = cmBlk.getReal("hue", 0);
  current.saturationColor = color3(cmBlk.getE3dcolor("saturationColor", E3DCOLOR(255, 255, 255)));
  current.saturation = cmBlk.getReal("saturation", 100.0f);
  current.grayColor = color3(cmBlk.getE3dcolor("grayColor", E3DCOLOR(255, 255, 255)));
  current.contrastColor = color3(cmBlk.getE3dcolor("contrastColor", E3DCOLOR(255, 255, 255)));
  current.contrast = cmBlk.getReal("contrast", 100.0f);
  current.contrastPivotColor = color3(cmBlk.getE3dcolor("contrastPivotColor", E3DCOLOR(255, 255, 255)));
  current.contrastPivot = cmBlk.getReal("contrastPivot", 50.0f);
  current.brightnessColor = color3(cmBlk.getE3dcolor("brightnessColor", E3DCOLOR(255, 255, 255)));
  current.brightness = cmBlk.getReal("brightness", 100.0f);

  recalcColorMatrix();
  recalcGlow();

  return true;
}

DemonPostFx::~DemonPostFx()
{
  if (combineFx.getMat())
    combineFx.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);
  if (downsample4x.getMat())
    downsample4x.getMat()->set_texture_param(srcTexVarId, BAD_TEXTUREID);

  postFx = NULL;

  glowBlur2YFx.clear();
  glowBlur2XFx.clear();
  glowBlurYFx.clear();
  glowBlurXFx.clear();
  glowBlur2YFxCS.reset();
  glowBlur2XFxCS.reset();
  glowBlurYFxCS.reset();
  glowBlurXFxCS.reset();
  radialBlur1Fx.clear();
  radialBlur2Fx.clear();
  combineFx.clear();
  downsample4x.clear();
  combineCallback = NULL;

  prevFrameLowResTex.close();
  glowTex.close();
  tmpTex.close();
  uiBlurTex.close();
  lowResLumTexB.close();
  lowResLumTexA.close();

  // FLARE
  closeLenseFlare();
}

#include <perfMon/dag_statDrv.h>
void DemonPostFx::applyLenseFlare(const UniqueTex &src_tex)
{
  if (!lenseFlareEnabled)
    return;
  lensFlare.apply(src_tex.getTex2D(), src_tex.getTexId());
}

void DemonPostFx::calcGlowGraphics()
{
  {
    TIME_D3D_PROFILE(glowBlurXFx);
    d3d::resource_barrier({prevFrameLowResTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    // glow
    d3d::set_render_target(tmpTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlurXFx.getMat()->set_color4_param(weight0VarId, current.hdrGlowMul * glowWeights0X);
    glowBlurXFx.getMat()->set_color4_param(weight1VarId, current.hdrGlowMul * glowWeights1X);
    glowBlurXFx.getMat()->set_color4_param(darkThreshold0VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights0X);
    glowBlurXFx.getMat()->set_color4_param(darkThreshold1VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights1X);
    glowBlurXFx.getMat()->set_texture_param(texVarId, prevFrameLowResTex.getTexId());
    glowBlurXFx.render();
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlurYFx);
    d3d::set_render_target(glowTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlurYFx.getMat()->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlurYFx.render();
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2XFx);
    d3d::set_render_target(tmpTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlur2XFx.getMat()->set_texture_param(texVarId, glowTex.getTexId());
    glowBlur2XFx.render();
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2YFx);
    d3d::set_render_target(glowTex.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    glowBlur2YFx.getMat()->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlur2YFx.render();
  }
}

void DemonPostFx::calcGlowCompute()
{
  {
    TIME_D3D_PROFILE(glowBlurXFx_cs);
    d3d::resource_barrier({prevFrameLowResTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    // glow
    d3d::set_rwtex(STAGE_CS, 0, tmpTex.getTex2D(), 0, 0);
    glowBlurXFxCS->set_color4_param(weight0VarId, current.hdrGlowMul * glowWeights0X);
    glowBlurXFxCS->set_color4_param(weight1VarId, current.hdrGlowMul * glowWeights1X);
    glowBlurXFxCS->set_color4_param(darkThreshold0VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights0X);
    glowBlurXFxCS->set_color4_param(darkThreshold1VarId, -(current.hdrGlowMul * current.hdrDarkThreshold) * glowWeights1X);
    glowBlurXFxCS->set_texture_param(texVarId, prevFrameLowResTex.getTexId());
    glowBlurXFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlurXFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlurYFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, glowTex.getTex2D(), 0, 0);
    glowBlurYFxCS->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlurYFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlurYFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2XFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, tmpTex.getTex2D(), 0, 0);
    glowBlur2XFxCS->set_texture_param(texVarId, glowTex.getTexId());
    glowBlur2XFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlur2XFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(glowBlur2YFx_cs);
    d3d::set_rwtex(STAGE_CS, 0, glowTex.getTex2D(), 0, 0);
    glowBlur2YFxCS->set_texture_param(texVarId, tmpTex.getTexId());
    glowBlur2YFxCS->set_color4_param(targetTextureSizeVarId, Color4(lowResSize.x, lowResSize.y, 0, 0));
    glowBlur2YFxCS->dispatchThreads(lowResSize.x, lowResSize.y, 1);
    d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
}

void DemonPostFx::recalcColorMatrix()
{
  paramColorMatrix = ::make_hue_shift_color_matrix(current.hueShift);

  paramColorMatrix *= ::make_saturation_color_matrix(current.saturationColor * (current.saturation / 100), current.grayColor);

  paramColorMatrix *= ::make_contrast_color_matrix(current.contrastColor * (current.contrast / 100),
    current.contrastPivotColor * (current.contrastPivot / 100));

  paramColorMatrix *= ::make_brightness_color_matrix(current.brightnessColor * (current.brightness / 100));
}


void DemonPostFx::recalcGlow()
{
  if (useSimpleMode)
    return;
  float weights[BLUR_SAMPLES];
  for (int i = 0; i < BLUR_SAMPLES; ++i)
    weights[i] = powf(current.volfogFade, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES)) / BLUR_SAMPLES;

  radialBlur1Fx.getMat()->set_color4_param(weight0VarId, Color4(weights));
  radialBlur1Fx.getMat()->set_color4_param(weight1VarId, Color4(weights + 4));

  for (int i = 0; i < BLUR_SAMPLES; ++i)
    weights[i] = powf(current.volfogFade, i / real(BLUR_SAMPLES)) / BLUR_SAMPLES;

  radialBlur2Fx.getMat()->set_color4_param(weight0VarId, Color4(weights));
  radialBlur2Fx.getMat()->set_color4_param(weight1VarId, Color4(weights + 4));

  real u0 = cosf(DegToRad(-glowAngle[0]));
  real v0 = sinf(DegToRad(-glowAngle[0]));
  real u1 = cosf(DegToRad(-glowAngle[1]));
  real v1 = sinf(DegToRad(-glowAngle[1]));

  real glowWt[BLUR_SAMPLES / 2];
  real sum = 0;

  for (int i = 0; i < BLUR_SAMPLES / 2; ++i)
  {
    real t = real(i) / (BLUR_SAMPLES / 2);
    real w = (glowFadeK >= 0 ? powf(1 - t, glowFadeK) : 1 - powf(t, -glowFadeK));
    w = w * (1 - glowEdgeK) + glowEdgeK;
    glowWt[i] = w;
    sum += w;
  }

  if (!float_nonzero(sum))
    sum = 1;

  for (int i = 0; i < BLUR_SAMPLES / 2; ++i)
    glowWt[i] /= sum * 2;

  float weightsX[BLUR_SAMPLES], weightsX2[BLUR_SAMPLES];
  for (int i = 0; i < BLUR_SAMPLES; ++i)
  {
    real w = glowWt[i < BLUR_SAMPLES / 2 ? BLUR_SAMPLES / 2 - 1 - i : i - BLUR_SAMPLES / 2];

    real o1 = (i - (BLUR_SAMPLES - 1) * 0.5f) / ((BLUR_SAMPLES - 1) * 0.5f);
    Point2 o = Point2(o1 / (float)lowResSize.x, o1 / (float)lowResSize.y);
    o *= 2.f;

    real w1 = 1.0 / BLUR_SAMPLES; // use box filter for first iteration

    weightsX[i] = w1;
    weightsX2[i] = w;

    glowBlurXFx.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u0, o.y * v0));

    glowBlurYFx.getMat()->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u1, o.y * v1));

    glowBlur2XFx.getMat()->set_color4_param(texTmVarId[i],
      Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u0, o.y * BLUR_SAMPLES * 0.5f * v0));

    glowBlur2YFx.getMat()->set_color4_param(texTmVarId[i],
      Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u1, o.y * BLUR_SAMPLES * 0.5f * v1));

    if (useCompute)
    {
      glowBlurXFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u0, o.y * v0));

      glowBlurYFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * u1, o.y * v1));

      glowBlur2XFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u0, o.y * BLUR_SAMPLES * 0.5f * v0));

      glowBlur2YFxCS->set_color4_param(texTmVarId[i], Color4(1, 1, o.x * BLUR_SAMPLES * 0.5f * u1, o.y * BLUR_SAMPLES * 0.5f * v1));
    }
  }
  glowWeights0X = Color4(weightsX);
  glowWeights1X = Color4(weightsX + 4);
  glowBlurXFx.getMat()->set_int_param(darkThresholdVarId, 1);
  glowBlurXFx.getMat()->set_color4_param(weight0VarId, glowWeights0X * current.hdrGlowMul);
  glowBlurXFx.getMat()->set_color4_param(weight1VarId, glowWeights1X * current.hdrGlowMul);
  glowBlurYFx.getMat()->set_color4_param(weight0VarId, glowWeights0X);
  glowBlurYFx.getMat()->set_color4_param(weight1VarId, glowWeights1X);

  glowBlur2XFx.getMat()->set_color4_param(weight0VarId, Color4(weightsX2));
  glowBlur2XFx.getMat()->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

  glowBlur2YFx.getMat()->set_color4_param(weight0VarId, Color4(weightsX2));
  glowBlur2YFx.getMat()->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

  if (useCompute)
  {
    glowBlurXFxCS->set_int_param(darkThresholdVarId, 1);
    glowBlurXFxCS->set_color4_param(weight0VarId, glowWeights0X * current.hdrGlowMul);
    glowBlurXFxCS->set_color4_param(weight1VarId, glowWeights1X * current.hdrGlowMul);
    glowBlurYFxCS->set_color4_param(weight0VarId, glowWeights0X);
    glowBlurYFxCS->set_color4_param(weight1VarId, glowWeights1X);

    glowBlur2XFxCS->set_color4_param(weight0VarId, Color4(weightsX2));
    glowBlur2XFxCS->set_color4_param(weight1VarId, Color4(weightsX2 + 4));

    glowBlur2YFxCS->set_color4_param(weight0VarId, Color4(weightsX2));
    glowBlur2YFxCS->set_color4_param(weight1VarId, Color4(weightsX2 + 4));
  }
}

void DemonPostFx::setLenseFlareEnabled(bool enabled)
{
  lenseFlareEnabled = enabled;
  lensFlare.toggleEnabled(enabled);
}

#include <debug/dag_debug3d.h>

// downsample 4x4->1
void DemonPostFx::downsample(Texture *to, TEXTUREID src, int srcW, int srcH, const Point4 &uv_transform)
{
  TIME_D3D_PROFILE(downsample);

  // use filtered downsampling
  d3d::set_render_target(to, 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
  Color4 target_coefs = quadCoeffs0001;
  static int texelOffsetVarId = ::get_shader_variable_id("texelOffset");
  static int uvTransformVarId = ::get_shader_variable_id("uvTransform");
  if (srcTexVarId < 0)
    srcTexVarId = ::get_shader_variable_id("srcTex");
  downsample4x.getMat()->set_texture_param(srcTexVarId, src);

  float invTargX = 1.0f / srcW, invTargY = 1.0f / srcH;

  downsample4x.getMat()->set_color4_param(texelOffsetVarId,
    Color4(0.5f * invTargX + target_coefs.b, 0.5f * invTargY + target_coefs.a, invTargX * 2, invTargY * 2));

  downsample4x.getMat()->set_color4_param(uvTransformVarId, reinterpret_cast<const Color4 &>(uv_transform));

  downsample4x.render();
  downsample4x.getMat()->set_texture_param(srcTexVarId, BAD_TEXTUREID);
}

#if 0
static float getBright(int cid, int pixels, int column_pixels, int size, int threshold)
{
  if (column_pixels == 0)
    return 0.f;

  float linear = float(threshold + column_pixels - pixels) / column_pixels;
  return (linear+cid)/size;
}

static void find_min_avg_maximum(int lowThreshold, int avgThreshold,
        unsigned * __restrict histogram, int size,
        float &lowBright, float &avgBright, int &minId, int &avgId)
{
  unsigned * __restrict pixelsPtr = histogram;
  avgId = minId = size-1;
  for (int i = 0, pixels = 0; i < size; ++i, pixelsPtr++)
  {
    PREFETCH_DATA(64, pixelsPtr);
    pixels += *pixelsPtr;
    if (pixels > lowThreshold)
    {
      lowBright = getBright(i, pixels, *pixelsPtr, size, lowThreshold);
      lowThreshold = 0x7FFFFFFF;
      minId = i;
    }
    if (pixels > avgThreshold)
    {
      avgBright = getBright(i, pixels, *pixelsPtr, size, avgThreshold);
      //avgThreshold = 0x7FFFFFFF;
      avgId = i;
      break;
    }
  }
  //debug("min %d avg %d", minId, avgId);
}
#endif

float find_avg_without_outliers(unsigned *__restrict histogram, int size, float minFractionSum, float maxFractionSum,
  float currentTarget)
{
  float sumWithoutOutliers = 0, pixels = 0;
  unsigned *__restrict pixelsPtr = histogram;

  for (int i = 0; i < size; ++i, pixelsPtr++)
  {
    float localValue = *pixelsPtr;

    // remove outlier at lower end
    float sub = min(localValue, minFractionSum);
    localValue = localValue - sub;
    minFractionSum -= sub;
    maxFractionSum = max(0.f, maxFractionSum - sub);

    // remove outlier at upper end
    localValue = min(localValue, maxFractionSum);
    maxFractionSum -= localValue;

    float luminanceAtBucket = float(i) / (size - 1); // linear buckets

    sumWithoutOutliers += luminanceAtBucket * localValue;
    pixels += localValue;
  }

  return pixels < 1 ? currentTarget : sumWithoutOutliers / max(0.0001f, pixels);
}

float keyValueMul = 2;
float keyValueMax = 0;

void DemonPostFx::prepareSkyMask(const TMatrix &view_tm)
{
  if (useSimpleMode)
    return;
  Point3 sunDir = view_tm % current.sunDir;
  float sunLightK = sunDir.z;

  real minCos = volfogMaxAngleCos;
  if (minCos < 0.0004f)
    minCos = 0.0004f;

  TIME_D3D_PROFILE(prepareSkyMask);

  G_ASSERT(volFogCallback);
  if (sunLightK >= minCos && volFogCallback)
  {
    d3d::set_render_target(lowResLumTexB.getTex2D(), 0);
    volFogCallback->process(lowResSize.x, lowResSize.y, quadCoeffs);
    skyMaskPrepared = true;
  }
}

TextureIDPair DemonPostFx::downsample(Texture *input_tex, TEXTUREID input_id, const Point4 &input_uv_transform)
{
  TextureInfo info;
  input_tex->getinfo(info);
  int targtexW = info.w;
  int targtexH = info.h;
  input_tex->texfilter(TEXFILTER_LINEAR);

  d3d::resource_barrier({input_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  if (prevFrameLowResTex.getTex2D())
  {
    downsample(prevFrameLowResTex.getTex2D(), input_id, targtexW, targtexH, input_uv_transform);
    return {prevFrameLowResTex.getTex2D(), prevFrameLowResTex.getTexId()};
  }
  return {};
}

// eye -1 for mono, 0,1 for stereo
void DemonPostFx::apply(bool vr_mode, Texture *target_tex, TEXTUREID target_id, Texture *output_tex, TEXTUREID output_id,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, Texture *depth_tex, int eye, int target_layer, const Point4 &target_uv_transform,
  const RectInt *output_viewport, PreCombineFxProc pre_combine_fx_proc)
{
  G_UNREFERENCED(output_id);
#if DAGOR_DBGLEVEL == 0
  current.debugFlags = 0;
#endif
  if (::grs_draw_wire)
    d3d::setwire(0);

  SCOPE_RENDER_TARGET;

  TextureInfo info;
  target_tex->getinfo(info);
  int targtexW = info.w;
  int targtexH = info.h;
  target_tex->texfilter(TEXFILTER_LINEAR);

  real sunLightK = 0;

  if (!useSimpleMode)
  {
    // read from already downsampled tex
    if (eye < 0)
      applyLenseFlare(prevFrameLowResTex);

    // apply glow color curve
    bool hasGlow = !(current.debugFlags & current.NO_GLOW);
    if (::hdr_render_mode != HDR_MODE_FLOAT && current.hdrDarkThreshold > ::hdr_max_overbright)
      hasGlow = false;

    if (current.hdrGlowMul <= 0.0)
    {
      hasGlow = false;
    }

    ShaderGlobal::set_real_fast(hasPostFxGlowVarId, hasGlow ? 1 : 0);
    if (current.hdrGlowMul * current.hdrGlowPower > 0 && hasGlow)
    {
      if (useCompute)
        calcGlowCompute();
      else
        calcGlowGraphics();
    }
    else if (glowTex.getTex2D())
    {
      TIME_D3D_PROFILE(clearGlow);
      if (useCompute)
      {
        const float zero[4] = {0, 0, 0, 0};
        d3d::clear_rwtexf(glowTex.getTex2D(), zero, 0, 0);
      }
      else
      {
        d3d::set_render_target(glowTex.getTex2D(), 0);
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      }
    }

    // radial blur (fake volumetric light from sun)
    Point3 sunDir = view_tm % current.sunDir;
    real wk, hk;

    wk = proj_tm(0, 0);
    hk = proj_tm(1, 1);

    sunLightK = sunDir.z;

    real minCos = volfogMaxAngleCos;
    if (minCos < 0.0004f)
      minCos = 0.0004f;

    // Here we disable the radial blur based volfog emulation for VR. The method looks good enough indeed
    // but in VR, the results are simply too inconsistent between the eyes, especially when there are
    // close things on the screen, like hands or cockpit.
    float volFogBrightness = current.volfogMul * rgbsum(current.volfogColor) * (vr_mode ? 0 : 1);
    G_ASSERT(volFogCallback);
    if (sunLightK >= minCos && volFogCallback && !(current.debugFlags & current.NO_VFOG) && volFogBrightness > 0.00001f)
    {
      if (!skyMaskPrepared)
        prepareSkyMask(view_tm);

      sunLightK = (sunLightK - minCos) / (1 - minCos);
      sunLightK *= current.volfogMul;

      real sunX = 0.5f + 0.5f * sunDir.x * wk / sunDir.z;
      real sunY = 0.5f - 0.5f * sunDir.y * hk / sunDir.z;

      // perform radial blur
      lowResLumTexA->texaddr(TEXADDR_BORDER);
      lowResLumTexA->texbordercolor(0);
      lowResLumTexB->texaddr(TEXADDR_BORDER);
      lowResLumTexB->texbordercolor(0);
      d3d::set_render_target(lowResLumTexA.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES * BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexB.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }

      d3d::set_render_target(lowResLumTexB.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, (i + 1) / real(BLUR_SAMPLES * BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexA.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }

      d3d::set_render_target(lowResLumTexA.getTex2D(), 0);

      for (int i = 0; i < BLUR_SAMPLES; ++i)
      {
        real s = powf(1 / current.volfogRange, i / real(BLUR_SAMPLES));

        radialBlur1Fx.getMat()->set_color4_param(texTmVarId[i], Color4(s, s, sunX - sunX * s, sunY - sunY * s));
      }

      radialBlur1Fx.getMat()->set_texture_param(texVarId, lowResLumTexB.getTexId());
      {
        TIME_D3D_PROFILE(radialBlur1Fx);
        radialBlur1Fx.render();
      }
      lowResLumTexA->texaddr(TEXADDR_CLAMP);
      lowResLumTexB->texaddr(TEXADDR_CLAMP);
      skyMaskPrepared = false;
    }
    else
      sunLightK = 0;
  }

  Color4 quad_coefs = quadCoeffs;

  // set up combine params
  if (!useSimpleMode)
  {
    combineFx.getMat()->set_texture_param(glowTexVarId, glowTex.getTexId());
    combineFx.getMat()->set_texture_param(volfogTexVarId, lowResLumTexA.getTexId());
    combineFx.getMat()->set_color4_param(glowTexSzVarId, quad_coefs);

    float glowPower = current.hdrGlowPower;

    combineFx.getMat()->set_color4_param(glowScaleVarId, Color4(1, 1, 1) * (glowPower >= 1 ? 1 : glowPower));

    combineFx.getMat()->set_color4_param(volfogScaleVarId,
      Color4(current.volfogColor.r, current.volfogColor.g, current.volfogColor.b) * sunLightK);
    if (volFogOnVarId >= 0)
      combineFx.getMat()->set_int_param(volFogOnVarId, sunLightK * rgbsum(current.volfogColor) > 0.00001f ? 1 : 0);
  }

  if (colorMatrixRVarId >= 0)
  {
    TMatrix ctm = postColorMatrix * paramColorMatrix * preColorMatrix;

    Color4 colorMatrixR(ctm[0][0], ctm[1][0], ctm[2][0], ctm[3][0]);
    Color4 colorMatrixG(ctm[0][1], ctm[1][1], ctm[2][1], ctm[3][1]);
    Color4 colorMatrixB(ctm[0][2], ctm[1][2], ctm[2][2], ctm[3][2]);

    combineFx.getMat()->set_color4_param(colorMatrixRVarId, colorMatrixR);
    combineFx.getMat()->set_color4_param(colorMatrixGVarId, colorMatrixG);
    combineFx.getMat()->set_color4_param(colorMatrixBVarId, colorMatrixB);
  }

  // combine result to output
  if (current.debugFlags & current.NO_SCENE)
  {
    TIME_D3D_PROFILE(clearTarget);
    d3d::set_render_target(target_tex, 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  }

  if (!useSimpleMode)
  {
    if (glowTex.getTex2D())
      d3d::resource_barrier({glowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (lowResLumTexA.getTex2D())
      d3d::resource_barrier({lowResLumTexA.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  if (output_tex)
  {
    d3d::set_render_target({depth_tex, 0}, depth_tex ? DepthAccess::SampledRO : DepthAccess::RW,
      {{output_tex, 0u, output_tex->restype() == RES3D_ARRTEX ? unsigned(target_layer) : 0u}});
  }
  else
  {
    d3d::set_render_target();
    if (depth_tex)
      d3d::set_depth(depth_tex, DepthAccess::SampledRO);
  }

  if (output_viewport)
  {
    d3d::setview(output_viewport->left, output_viewport->top, output_viewport->right - output_viewport->left,
      output_viewport->bottom - output_viewport->top, 0, 1);
    d3d::setscissor(output_viewport->left, output_viewport->top, output_viewport->right - output_viewport->left,
      output_viewport->bottom - output_viewport->top);
  }

  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);

  combineFx.getMat()->set_texture_param(texVarId, target_id);
  combineFx.getMat()->set_color4_param(texUvTransformVarId, reinterpret_cast<const Color4 &>(target_uv_transform));
  int outW, outH;
  d3d::get_target_size(outW, outH);
  combineFx.getMat()->set_color4_param(texSzVarId, quadCoeffs);
  if (vignette_multiplierGlobVarId >= 0)
    ShaderGlobal::set_real_fast(vignette_multiplierGlobVarId, vignetteMul);
  if (vignette_start_endGlobVarId >= 0)
    ShaderGlobal::set_color4_fast(vignette_start_endGlobVarId, Color4(vignetteStart.x, vignetteEnd.x, vignetteStart.y, vignetteEnd.y));

  ShaderGlobal::set_real(ldr_vignette_multiplierVarId, ldrVignetteMul);
  ShaderGlobal::set_real(ldr_vignette_widthVarId, ldrVignetteWidth);
  ShaderGlobal::set_real(ldr_vignette_aspectVarId, float(targtexW) / targtexH);

  {
    if (pre_combine_fx_proc)
      pre_combine_fx_proc();

    TIME_D3D_PROFILE(combineFx);
    combineFx.render();
  }

  combineFx.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);

  if (::grs_draw_wire)
    d3d::setwire(::grs_draw_wire);
}

void DemonPostFx::delayedCombineFx(TEXTUREID textureId)
{
  // combine result to output
  ShaderGlobal::set_int_fast(renderCombinePassGVarId, 2);
  combineFx.getMat()->set_texture_param(texVarId, textureId);
  combineFx.render();
  ShaderGlobal::set_int_fast(renderCombinePassGVarId, 0);
}