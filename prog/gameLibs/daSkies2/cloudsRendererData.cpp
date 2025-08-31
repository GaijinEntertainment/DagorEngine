// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsRendererData.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_info.h>

/*static*/ void CloudsRendererData::clear_black(ManagedTex &t)
{
  if (!t)
    return;
  TextureInfo tinfo;
  t->getinfo(tinfo, 0);
  if (tinfo.cflg & TEXCF_RTARGET)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(t.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  }
  else if (tinfo.cflg & TEXCF_UNORDERED)
    d3d::clear_rwtexf(t.getTex2D(), ZERO_PTR<float>(), 0, 0);
}

void CloudsRendererData::clearTemporalData(uint32_t gen)
{
  if (resetGen == gen)
    return;
  resetGen = gen;
  frameValid = false;
  prevCloudsColor = nullptr;
  nextCloudsColor = nullptr;
  prevCloudsWeight = nullptr;
  cloudsTextureColor = nullptr;
  cloudsBlurTextureColor = nullptr;
  cloudsTextureColor = cloudsColorPoolRT->acquire();
  clear_black(*cloudsTextureColor);

  RTarget::Ptr prevColor = cloudsColorPoolRT->acquire();
  clear_black(*prevColor);

  cloudsTextureWeight = nullptr;
  cloudsTextureWeight = cloudsWeightPoolRT->acquire();
  clear_black(*cloudsTextureWeight);

  RTarget::Ptr prevWeight = cloudsWeightPoolRT->acquire();
  clear_black(*prevWeight);
}

void CloudsRendererData::close()
{
  clouds_close_layer_is_outside.close();
  clouds_color_close.close();
  clouds_tile_distance.close();
  clouds_tile_distance_tmp.close();
  cloudsTextureDepth.close();
  cloudsBlurTextureColor = nullptr;
  cloudsTextureColor = nullptr;
  prevCloudsColor = nullptr;
  nextCloudsColor = nullptr;
  prevCloudsWeight = nullptr;
  cloudsColorPoolRT = nullptr;
  cloudsColorBlurPoolRT = nullptr;
  cloudsTextureWeight = nullptr;
  cloudsIndirectBuffer.close();
  w = h = 0;
}

void CloudsRendererData::initTiledDist(const char *prefix) // only needed when it is not 100% cloudy. todo: calc pixels count
                                                           // allocation
{
  clouds_tile_distance.close();
  clouds_tile_distance_tmp.close();
  int dw = (w + tileX - 1) / tileX, dh = (h + tileY - 1) / tileY;
  if (dw * dh < 1920 * 720 / 4 / tileX / tileY)
    return;
  uint32_t flg = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET;
  String tn;
  tn.printf(64, "%s_tile_dist", prefix);
  clouds_tile_distance = dag::create_tex(NULL, dw, dh, flg | TEXFMT_L16, 1, tn.c_str());
  tn.printf(64, "%s_tile_dist_tmp", prefix);
  clouds_tile_distance_tmp = dag::create_tex(NULL, dw, dh, flg | TEXFMT_L16, 1, tn.c_str());
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(clouds_tile_distance_tmp_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(clouds_tile_distance_samplerstateVarId, sampler);
  }
}

void CloudsRendererData::setVars()
{
  const int dw = bool(clouds_tile_distance) ? (w + tileX - 1) / tileX : 0;
  const int dh = dw != 0 ? (h + tileY - 1) / tileY : 0;
  G_ASSERT(w > 0 && h > 0);
  ShaderGlobal::set_int4(clouds_tiled_resVarId, dw, dh, 0, 0);
  ShaderGlobal::set_int4(clouds2_resolutionVarId, w, h, lowresCloseClouds ? w / 2 : w, lowresCloseClouds ? h / 2 : h);
  ShaderGlobal::set_texture(clouds_colorVarId, cloudsBlurTextureColor ? cloudsBlurTextureColor->getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_texture(clouds_color_closeVarId, clouds_color_close);
  ShaderGlobal::set_texture(clouds_tile_distanceVarId, clouds_tile_distance);
  ShaderGlobal::set_texture(clouds_tile_distance_tmpVarId, clouds_tile_distance_tmp);
  ShaderGlobal::set_texture(clouds_depthVarId, cloudsTextureDepth);
  ShaderGlobal::set_buffer(clouds_close_layer_is_outsideVarId, clouds_close_layer_is_outside);
}

void CloudsRendererData::init(int width, int height, const char *prefix, bool can_be_in_clouds, CloudsResolution clouds_resolution,
  bool use_blurred_clouds)
{
  ShaderGlobal::set_int(clouds_use_fullresVarId, (clouds_resolution == CloudsResolution::ForceFullresClouds));
  const bool changedSize = (w != width || h != height);

  // note: we also need depth (see CloudsRenderer::render) to decide if can be in clouds, but it's pass dependent
  const bool changedCanBeInClouds = (can_be_in_clouds != bool(clouds_color_close)) || changedSize;
  if (!changedSize && !changedCanBeInClouds)
    return;
  if (d3d::get_driver_desc().shaderModel < 5.0_sm)
    useCompute = taaUseCompute = false;
  if (!width)
  {
    close();
    return;
  }

  w = width;
  h = height;
  frameValid = false;
  resetGen = 0;
  lowresCloseClouds =
    clouds_resolution != CloudsResolution::FullresCloseClouds && clouds_resolution != CloudsResolution::ForceFullresClouds;
  uint32_t fmt = TEXFMT_A16B16G16R16F;
  if (useCompute && d3d::get_driver_desc().issues.hasBrokenComputeFormattedOutput)
    fmt = TEXFMT_A32B32G32R32F;
  uint32_t rtflg = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET; //
  String tn;
  if (d3d::get_driver_desc().shaderModel >= 5.0_sm && d3d::get_driver_desc().caps.hasIndirectSupport && !cloudsIndirectBuffer)
  {
    tn.printf(64, "%s_clouds_indirect", prefix);
    cloudsIndirectBuffer = dag::create_sbuffer(4, CLOUDS_APPLY_COUNT * 4, SBCF_UA_INDIRECT, 0, tn.c_str());
  }
  if (changedSize)
  {
    uint32_t taaRtflg = taaUseCompute ? TEXCF_UNORDERED : TEXCF_RTARGET; //
    cloudsTextureColor = nullptr;
    prevCloudsColor = nullptr;
    nextCloudsColor = nullptr;
    prevCloudsWeight = nullptr;
    cloudsBlurTextureColor = nullptr;
    cloudsTextureWeight = nullptr;
    cloudsTextureDepth.close();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(clouds_prev_taa_weight_samplerstateVarId, sampler);
      ShaderGlobal::set_sampler(clouds_color_samplerstateVarId, sampler);
      ShaderGlobal::set_sampler(clouds_color_prev_samplerstateVarId, sampler);
      smpInfo.filter_mode = d3d::FilterMode::Point;
      sampler = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(clouds_depth_samplerstateVarId, sampler);
    }

    cloudsWeightPoolRT = RTargetPool::get(w, h, taaRtflg | TEXFMT_R8, 1);

    cloudsColorPoolRT = RTargetPool::get(w, h, taaRtflg | fmt, 1);
    cloudsColorBlurPoolRT = RTargetPool::get(w, h, rtflg | fmt, 1);

    tn.printf(64, "%s_clouds_depth", prefix);
    cloudsTextureDepth.close();
    cloudsTextureDepth = dag::create_tex(NULL, w, h, rtflg | TEXFMT_L16, 1, tn.c_str());
    initTiledDist(prefix);
  }
  if (changedCanBeInClouds)
  {
    clouds_color_close.close();
    clouds_close_layer_is_outside.close();
    if (can_be_in_clouds)
    {
      tn.printf(64, "%s_clouds_close", prefix);
      clouds_color_close = dag::create_tex(NULL, lowresCloseClouds ? w / 2 : w, lowresCloseClouds ? h / 2 : h,
        rtflg | fmt | TEXCF_CLEAR_ON_CREATE, 1, tn.c_str());
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        ShaderGlobal::set_sampler(clouds_color_close_samplerstateVarId, d3d::request_sampler(smpInfo));
      }
    }
  }

  tn.printf(64, "%s_close_layer_is_outside", prefix);

  if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
  {
    if (!clouds_close_layer_is_outside.getBuf())
      clouds_close_layer_is_outside = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2, tn.c_str(), dag::buffers::Init::Zero);
  } // NOTE: we don't use clouds_close_layer_is_outside optimization without compute shaders

  useBlurredClouds = use_blurred_clouds;
}

void CloudsRendererData::update(const DPoint3 &dir, const DPoint3 &origin)
{
  double offset = dir * (origin - cloudsCameraOrigin);
  currentCloudsOffset += offset;
  cloudsCameraOrigin = origin;
}
