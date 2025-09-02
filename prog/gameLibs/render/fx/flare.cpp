// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/flare.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_renderTarget.h>

#define GLOBAL_VARS_LIST     \
  VAR(flare_enabled)         \
  VAR(flare_use_covering)    \
  VAR(flareSrc)              \
  VAR(flareSrc_samplerstate) \
  VAR(flareSrcGlob)          \
  VAR(texelOffset)           \
  VAR(duv1duv2)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
}

void Flare::init(const Point2 low_res_size, const char *lense_covering_tex_name, const char *lense_radial_tex_name)
{
  if (flareBlur)
    return; // already initialized

  init_shader_vars();

  flareCoveringTex = dag::get_tex_gameres(lense_covering_tex_name, "flareLenseCovering");
  flareColorTex = dag::get_tex_gameres(lense_radial_tex_name, "flareLenseRadial");
  ShaderGlobal::set_sampler(::get_shader_variable_id("flareLenseRadial_samplerstate", true),
    get_texture_separate_sampler(flareColorTex.getTexId()));

  // LENSE FLARES
  int flareWidth = low_res_size.x, flareHeight = low_res_size.y; // assume quarter resolution
  debug("init lense flare %dx%d", flareWidth, flareHeight);

  unsigned flare_fmt = TEXFMT_DEFAULT;
  const unsigned int workingFlags = d3d::USAGE_RTARGET;
  if ((d3d::get_texformat_usage(TEXFMT_R11G11B10F) & workingFlags) == workingFlags)
    flare_fmt = TEXFMT_R11G11B10F;

  flareRTPool = RTargetPool::get(flareWidth, flareHeight, flare_fmt | TEXCF_RTARGET, 1);

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    flareTexSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(::get_shader_variable_id("flareSrcGlob_samplerstate", true), flareTexSampler);
  }
  flareDownsample = new PostFxRenderer();
  flareDownsample->init("flare_downsample");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    flareDownsample->getMat()->set_sampler_param(flareSrc_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  flareFeature = new PostFxRenderer();
  flareFeature->init("flare_feature");
  flareBlur = new PostFxRenderer();
  flareBlur->init("flare_blur");

  // becuase of too many intervals in postfx.sh, flare and dust tex flags are controlled manually
  ShaderGlobal::set_int(flare_enabledVarId, 1);
  ShaderGlobal::set_int(flare_use_coveringVarId, flareCoveringTex.getTexId() != BAD_TEXTUREID);
}


void Flare::apply(Texture *src_tex, TEXTUREID src_id)
{
  if (!flareBlur)
  {
    if (VariableMap::isVariablePresent(flareSrcGlobVarId))
      ShaderGlobal::set_texture(flareSrcGlobVarId, BAD_TEXTUREID); // to disable shader behaviour
    return;
  }

  int duv3duv4VarId = duv1duv2VarId;
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  TIME_D3D_PROFILE(lenseFlare);

  Color4 texelOffset;
  int targetWidth, targetHeight;

  RTarget::Ptr tmpFlareTex = flareRTPool->acquire();

  if (flareTex == nullptr)
  {
    flareTex = flareRTPool->acquire();
  }

  // downsample
  {
    d3d::resource_barrier({src_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::set_render_target(tmpFlareTex->getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);

    // compute texel offset
    d3d::get_target_size(targetWidth, targetHeight);
    texelOffset = Color4(0.5f, 0.5f, 1.f / targetWidth, 1.f / targetHeight);

    // texture is same size, don't need to downsample

    flareDownsample->getMat()->set_color4_param(texelOffsetVarId, texelOffset);
    flareDownsample->getMat()->set_texture_param(flareSrcVarId, src_id);
    flareDownsample->render();
    flareDownsample->getMat()->set_texture_param(flareSrcVarId, BAD_TEXTUREID);
    d3d::resource_barrier({tmpFlareTex->getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // feature
  {
    TIME_D3D_PROFILE(feature);
    d3d::set_render_target(flareTex->getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    flareFeature->getMat()->set_color4_param(texelOffsetVarId, texelOffset);
    flareFeature->getMat()->set_texture_param(flareSrcVarId, tmpFlareTex->getTexId());
    flareFeature->getMat()->set_sampler_param(flareSrc_samplerstateVarId, flareTexSampler);
    flareFeature->render();
    flareFeature->getMat()->set_texture_param(flareSrcVarId, BAD_TEXTUREID);
    d3d::resource_barrier({flareTex->getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }


  // blur (horizontal)
  {
    float du, dv;

    // Phase 1. horizontal. flareTex_1 -> flareTex_0
    d3d::set_render_target(tmpFlareTex->getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    du = 1.0f / targetWidth;
    dv = 0.0f;
    flareBlur->getMat()->set_color4_param(duv1duv2VarId, Color4(du * 1.0f, dv * 1.0f, du * 2.0f, dv * 2.0f));
    flareBlur->getMat()->set_color4_param(duv3duv4VarId, Color4(du * 3.0f, dv * 3.0f, du * 4.0f, dv * 4.0f));
    flareBlur->getMat()->set_color4_param(texelOffsetVarId, texelOffset);
    flareBlur->getMat()->set_texture_param(flareSrcVarId, flareTex->getTexId());
    flareBlur->getMat()->set_sampler_param(flareSrc_samplerstateVarId, flareTexSampler);
    flareBlur->render();
    d3d::resource_barrier({tmpFlareTex->getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    // Phase 2. vertical. flareTex_0 -> flareTex_1
    d3d::set_render_target(flareTex->getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    du = 0.0f;
    dv = 1.0f / targetHeight;
    flareBlur->getMat()->set_color4_param(duv1duv2VarId, Color4(du * 1.0f, dv * 1.0f, du * 2.0f, dv * 2.0f));
    flareBlur->getMat()->set_color4_param(duv3duv4VarId, Color4(du * 3.0f, dv * 3.0f, du * 4.0f, dv * 4.0f));
    flareBlur->getMat()->set_texture_param(flareSrcVarId, tmpFlareTex->getTexId());
    flareBlur->render();
  }
  // blend back (additive)
  ShaderGlobal::set_texture(flareSrcGlobVarId, flareTex->getTexId());
  d3d::resource_barrier({flareTex->getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  flareBlur->getMat()->set_texture_param(flareSrcVarId, BAD_TEXTUREID);
  tmpFlareTex = nullptr;
  /*
  {
    TIME_D3D_PROFILE(blendback);
    d3d::set_render_target(target_tex, 0, false);
    flareBlend->getMat()->set_color4_param(::get_shader_variable_id("texelOffset"), texelOffset);
    flareBlend->render();
  }*/
  d3d::set_render_target(prevRt);
}

void Flare::toggleEnabled(bool enabled) { ShaderGlobal::set_int(flare_enabledVarId, enabled); }

void Flare::toggleCovering(bool enabled)
{
  if (flareCoveringTex)
  {
    flareCoveringTex.setVar();
    ShaderGlobal::set_int(flare_use_coveringVarId, enabled);
  }
}

void Flare::close()
{
  releaseRTs();

  ShaderGlobal::set_int(flare_enabledVarId, 0);
  ShaderGlobal::set_int(flare_use_coveringVarId, 0);

  flareCoveringTex.close();
  flareColorTex.close();

  del_it(flareDownsample);
  del_it(flareFeature);
  del_it(flareBlur);
}

void Flare::releaseRTs()
{
  ShaderGlobal::set_texture(flareSrcGlobVarId, BAD_TEXTUREID);
  flareTex = nullptr;
}
