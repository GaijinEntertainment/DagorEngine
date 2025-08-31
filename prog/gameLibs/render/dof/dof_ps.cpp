// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <shaders/dag_shaders.h>
#include <render/dof/dof_ps.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <util/dag_console.h>

#define GLOBAL_VARS_LIST                      \
  VAR(dof_frame_tex)                          \
  VAR(dof_frame_tex_samplerstate)             \
  VAR(dof_near_layer)                         \
  VAR(dof_near_layer_samplerstate)            \
  VAR(dof_far_layer)                          \
  VAR(dof_far_layer_samplerstate)             \
  VAR(dof_rt_size)                            \
  VAR(dof_downsampled_frame_tex)              \
  VAR(dof_downsampled_frame_tex_samplerstate) \
  VAR(dof_downsampled_close_depth_tex)        \
  VAR(dof_gather_iteration)                   \
  VAR(dof_focus_params)                       \
  VAR(dof_focus_linear_params)                \
  VAR(num_dof_taps)                           \
  VAR(dof_focus_far_mode)                     \
  VAR(dof_focus_near_mode)                    \
  VAR(simplified_dof)                         \
  VAR(dof_coc_history)                        \
  VAR(dof_coc_history_samplerstate)           \
  VAR(dof_blend_depth_tex_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static float ngon_rad(float theta, float n) { return cosf(PI / n) / cosf(theta - (2 * PI / n) * floorf((n * theta + PI) / (2 * PI))); }

// Shirley's concentric mapping
static Point4 to_unit_disk(const Point2 &origin, float blades, float kernelSize, const Point2 &scale)
{
  const float max_size = 8;
  const float min_size = 1;
  float normalizedStops = clamp((kernelSize - min_size) / (max_size - min_size), 0.0f, 1.0f);

  float phi;
  float r;
  const float a = 2 * origin.x - 1;
  const float b = 2 * origin.y - 1;
  if (fabsf(a) > fabsf(b)) // Use squares instead of absolute values
  {
    r = a;
    phi = (PI / 4.0f) * (b / (a + 1e-6f));
  }
  else
  {
    r = b;
    phi = (PI / 2.0f) - (PI / 4.0f) * (a / (b + 1e-6f));
  }

  float rr = r * powf(ngon_rad(phi, blades), normalizedStops);
  rr = fabsf(rr) * (rr > 0 ? 1.0f : -1.0f);

  // normalizedStops *= -0.4f * PI;
  Point2 offset(rr * cosf(phi + normalizedStops), rr * sinf(phi + normalizedStops));
  return Point4(offset.x * scale.x, offset.y * scale.y, offset.x, offset.y);
}

// Shirley's concentric mapping
static Point4 to_unit_disk(const Point2 &origin, const Point2 &scale)
{
  float phi;
  float r;
  const float a = 2 * origin.x - 1;
  const float b = 2 * origin.y - 1;
  if (fabsf(a) > fabsf(b)) // Use squares instead of absolute values
  {
    r = a;
    phi = (PI / 4.0f) * (b / (a + 1e-6f));
  }
  else
  {
    r = b;
    phi = (PI / 2.0f) - (PI / 4.0f) * (a / (b + 1e-6f));
  }
  Point2 offset(r * cosf(phi), r * sinf(phi));
  return Point4(offset.x * scale.x, offset.y * scale.y, offset.x, offset.y);
}

DepthOfFieldPS::~DepthOfFieldPS()
{
  ShaderGlobal::set_texture(dof_downsampled_frame_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(dof_frame_texVarId, BAD_TEXTUREID);
  setOn(false);
}

void DepthOfFieldPS::closeNear()
{
  if (!useNearDof)
    return;
  dof_near_layer = nullptr;
  for (int i = 0; i < dof_coc.size(); i++)
    dof_coc[i].close();
  dof_coc_history.close();
  useNearDof = false;
}

void DepthOfFieldPS::initNear()
{
  if (useNearDof && useCoCAccumulation == bool(dof_coc_history))
    return;
  uint32_t flg = TEXCF_RTARGET;
  ShaderGlobal::set_sampler(dof_near_layer_samplerstateVarId, clampSampler);
  for (int i = 0; i < dof_coc.size(); i++)
  {
    int wd = 1 << i;
    String name(128, "dof_coc%d", i);
    dof_coc[i] = dag::create_tex(NULL, (originalWidth + wd - 1) / wd, (originalHeight + wd - 1) / wd, flg | TEXFMT_R8, 1, name);
  }
  if (useCoCAccumulation && !useSimplifiedRendering)
  {
    dof_coc_history = dag::create_tex(NULL, originalWidth, originalHeight, flg | TEXFMT_R8, 1, "dof_coc_history");
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(dof_coc_history_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  else
  {
    dof_coc_history.close();
  }
  useNearDof = true;
  if (originalWidth != width || originalHeight != height)
    changeNearResolution();
}

void DepthOfFieldPS::changeNearResolution()
{
  if (!useNearDof)
    return;
  if (dof_near_layer)
    dof_near_layer->resize(width, height);
  for (int i = 0; i < dof_coc.size(); i++)
  {
    int wd = 1 << i;
    dof_coc[i].resize((width + wd - 1) / wd, (height + wd - 1) / wd);
  }
}

void DepthOfFieldPS::closeFar()
{
  if (!useFarDof)
    return;
  dof_far_layer = nullptr;
  dof_max_coc_far.close();
  useFarDof = false;
}

void DepthOfFieldPS::initFar()
{
  if (useFarDof)
    return;
  ShaderGlobal::set_sampler(dof_far_layer_samplerstateVarId, clampSampler);
  uint32_t flg = TEXCF_RTARGET;

  dof_max_coc_far = dag::create_tex(NULL, width, height, flg | TEXFMT_R8, 1, "dof_max_coc_far");
  useFarDof = true;
}

void DepthOfFieldPS::changeFarResolution()
{
  if (!useFarDof)
    return;

  if (dof_far_layer != nullptr)
    dof_far_layer->resize(width, height);
  dof_max_coc_far.resize(width, height);
}

void DepthOfFieldPS::setOn(bool on_)
{
  if (on != on_)
  {
    on = on_;
    ShaderGlobal::set_int(dof_focus_near_modeVarId, 0);
    ShaderGlobal::set_int(dof_focus_far_modeVarId, 0);
    ShaderGlobal::set_texture(dof_far_layerVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(dof_near_layerVarId, BAD_TEXTUREID);
  }
}

void DepthOfFieldPS::init(int w, int h)
{
  if (originalWidth == w / 2 && originalHeight == h / 2)
    return;

#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
  width = w / 2;
  height = h / 2;
  originalWidth = width;
  originalHeight = height;
  dofDownscale.init("dof_downsample");

  if (dofLayerRTPool != nullptr)
  {
    dof_far_layer = nullptr;
    dof_near_layer = nullptr;
    dofLayerRTPool = nullptr;
  }

  dofLayerRTPool = ResizableRTargetPool::get(originalWidth, originalHeight, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1);
  // dof_coc_temp.set(d3d::create_tex(NULL, width, height, 1, TEXFMT_R16G16F|TEXCF_RTARGET, "dof_coc_temp"), "dof_coc_temp");
  ShaderGlobal::set_color4(dof_rt_sizeVarId, width, height, 1. / width, 1. / height);
  dofTile.init("dof_tile");
  dofGather.init("dof_gather");
  dofComposite.init("dof_composite");
  if (useFarDof)
  {
    closeFar();
    initFar();
  }
  if (useNearDof)
  {
    closeNear();
    initNear();
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampPointSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(dof_blend_depth_tex_samplerstateVarId, clampPointSampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampSampler = d3d::request_sampler(smpInfo);
  }
  {
    ShaderGlobal::set_sampler(dof_downsampled_frame_tex_samplerstateVarId, clampSampler);
    ShaderGlobal::set_sampler(dof_frame_tex_samplerstateVarId, clampSampler);
  }

  ShaderGlobal::set_sampler(::get_shader_variable_id("dof_downsampled_close_depth_tex_samplerstate", true), d3d::request_sampler({}));
}

void DepthOfFieldPS::changeResolution(int w, int h)
{
  width = w / 2;
  height = h / 2;
  ShaderGlobal::set_color4(dof_rt_sizeVarId, width, height, 1. / width, 1. / height);
  changeNearResolution();
  changeFarResolution();
}


#define MAX_COC 4.0f

static inline float device_hyper_z(float linDepth, float zn, float zf) // reverse depth
{
  return (zn * zf / (zf - zn)) / linDepth - zn / (zf - zn);
}

static inline void device_reversed_depth_from_inv_depth_scale_bias(float &hyper_scale, float &hyper_bias, float scale, float bias,
  float zn, float zf) // reversed depth
{
  // return (zn * zf/(zf-zn)) / linDepth - zn/(zf-zn);
  // hd = (zn * zf/(zf-zn)) *invDepth - zn/(zf-zn);
  // invDepth = (hd+zn/(zf-zn))/(zn * zf/(zf-zn))
  // scale*invDepth + bias = scale*(hd+zn/(zf-zn))/(zn * zf/(zf-zn)) + bias
  // hd*scale/(zn * zf/(zf-zn))
  hyper_scale = scale * (zf - zn) / (zn * zf);
  hyper_bias = bias + scale / zf;
}

void DepthOfFieldPS::perform(const TextureIDPair &frame, const TextureIDPair &close_depth, float zn, float zf, float fov_scale)
{
  if (!height || !frame.getTex2D())
    return;
  if (!on)
  {
    ShaderGlobal::set_int(dof_focus_near_modeVarId, 0);
    ShaderGlobal::set_int(dof_focus_far_modeVarId, 0);
    ShaderGlobal::set_texture(dof_far_layerVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(dof_near_layerVarId, BAD_TEXTUREID);
    return;
  }
  fov_scale = max(fov_scale, 1e-6f);
  fov_scale *= fov_scale; // that is correct if focus plane is >> focalLength, which is usually true. (especially when focus to
                          // infinity)

  if (cFocus.hasCustomFocalLength())
    fov_scale = 1.0f; // ignore fov

  const float near_z_check = minCheckDist > 0.f ? max(zn, minCheckDist) : zn;
  // Focal Length is of max 20 centimeters (0.05 mm lense focal for 24 mm film) * magnification, which is typicay less than 4.
  // If we use tele lense, than it can be up to meter, but focus plane will be at infinity.
  const bool hasNearDof = cFocus.hasNearDof(near_z_check, 0.25f / height / fov_scale),
             hasFarDof = cFocus.hasFarDof(zf, 0.5f / height / fov_scale);

  cFocus = nFocus;
  if (!hasNearDof)
    closeNear();
  else
    initNear();
  if (!hasFarDof)
    closeFar();
  else
    initFar();

  // cFocus.setNearDof(dof_focus_near_start.get(), dof_focus_near_end.get(), dof_near_blur.get()*dof_blur_amount.get()/100);
  // cFocus.setFarDof(dof_focus_far_start.get(), dof_focus_far_end.get(), dof_far_blur.get()*dof_blur_amount.get()/100);
  // debug("calcNearCoc(%f) = %f, threshold=%f", near_z_check, cFocus.calcNearCoc(near_z_check), 0.25f/height/fov_scale);
  if (!hasNearDof && !hasFarDof)
  {
    ShaderGlobal::set_int(dof_focus_near_modeVarId, 0);
    ShaderGlobal::set_int(dof_focus_far_modeVarId, 0);
    ShaderGlobal::set_texture(dof_far_layerVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(dof_near_layerVarId, BAD_TEXTUREID);
    return;
  }

  performedNearDof = hasNearDof;
  performedFarDof = hasFarDof;

  TIME_D3D_PROFILE(dof)
  SCOPE_RENDER_TARGET;

  const float fkernelSize = bokehShapeKernelSize;
  const float fNumApertureSides = bokehShapeApertureBlades;
  float cocNearScale, cocNearBias, cocFarScale, cocFarBias;
  cFocus.getFarCoCParams(cocFarScale, cocFarBias);
  cFocus.getNearCoCParams(cocNearScale, cocNearBias);
  // convert to use with hyperz
  device_reversed_depth_from_inv_depth_scale_bias(cocFarScale, cocFarBias, cocFarScale, cocFarBias, zn, zf);     // reversed depth
  device_reversed_depth_from_inv_depth_scale_bias(cocNearScale, cocNearBias, cocNearScale, cocNearBias, zn, zf); // reversed depth
  if (!hasNearDof)
    cocNearScale = cocNearBias = 0;
  if (!hasFarDof)
    cocFarScale = cocFarBias = 0;
  cocNearScale *= fov_scale;
  cocNearBias *= fov_scale; // use fov
  cocFarScale *= fov_scale;
  cocFarBias *= fov_scale; // use fov

  cocNearScale *= height / MAX_COC;
  cocNearBias *= height / MAX_COC; // convert to normalized pixels
  cocFarScale *= height / MAX_COC;
  cocFarBias *= height / MAX_COC; // convert to normalized pixels

  ShaderGlobal::set_color4(dof_focus_paramsVarId, cocNearScale, cocNearBias, cocFarScale, cocFarBias);

  ShaderGlobal::set_int(simplified_dofVarId, useSimplifiedRendering);
  float nearLinearMin, nearLinearMax, farLinearMin, farLinearMax;
  cFocus.getNearLinear(nearLinearMin, nearLinearMax);
  cFocus.getFarLinear(farLinearMin, farLinearMax);
  bool nearIsLinear = nearLinearMin != 0.0f && nearLinearMin != nearLinearMax && useLinearBlend;
  bool farIsLinear = farLinearMin != 0.0f && farLinearMin != farLinearMax && useLinearBlend;
  ShaderGlobal::set_color4(dof_focus_linear_paramsVarId, 1.0f / max(nearLinearMax - nearLinearMin, 1e-9f),
    -nearLinearMin / max(nearLinearMax - nearLinearMin, 1e-9f), 1.0f / max(farLinearMax - farLinearMin, 1e-9f),
    -farLinearMin / max(farLinearMax - farLinearMin, 1e-9f));

  ShaderGlobal::set_int(dof_focus_near_modeVarId, hasNearDof ? (nearIsLinear ? 2 : 1) : 0);
  ShaderGlobal::set_int(dof_focus_far_modeVarId, hasFarDof ? (farIsLinear ? 2 : 1) : 0);

  ShaderGlobal::set_texture(dof_coc_historyVarId,
    (useCoCAccumulation && !useSimplifiedRendering) ? dof_coc_history.getTexId() : BAD_TEXTUREID);

  // 1st downscale stage
  TextureInfo info;
  frame.getTex2D()->getinfo(info, 0);
  if (info.w == width && info.h == height)
  {
    ShaderGlobal::set_texture(dof_downsampled_frame_texVarId, frame.getId());
    ShaderGlobal::set_texture(dof_frame_texVarId, BAD_TEXTUREID);
  }
  else
  {
    ShaderGlobal::set_texture(dof_downsampled_frame_texVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(dof_frame_texVarId, frame.getId());
  }
  ShaderGlobal::set_texture(dof_downsampled_close_depth_texVarId, close_depth.getId());

  {
    TIME_D3D_PROFILE(dof_downscale);
    if (useNearDof && dof_near_layer == nullptr)
    {
      dof_near_layer = dofLayerRTPool->acquire();
      dof_near_layer->resize(width, height);
    }
    if (useFarDof && dof_far_layer == nullptr)
    {
      dof_far_layer = dofLayerRTPool->acquire();
      dof_far_layer->resize(width, height);
    }

    ShaderGlobal::set_texture(dof_near_layerVarId, useNearDof ? dof_near_layer->getTexId() : BAD_TEXTUREID);
    ShaderGlobal::set_texture(dof_far_layerVarId, useFarDof ? dof_far_layer->getTexId() : BAD_TEXTUREID);

    if (hasNearDof)
    {
      d3d::set_render_target(useNearDof ? dof_near_layer->getTex2D() : nullptr, 0);
      d3d::set_render_target(1, dof_coc[0].getTex2D(), 0);
      d3d::set_render_target(2, useFarDof ? dof_far_layer->getTex2D() : nullptr, 0);
      d3d::set_render_target(3, dof_max_coc_far.getTex2D(), 0);
    }
    else
    {
      G_ASSERT(useFarDof);
      d3d::set_render_target(dof_far_layer->getTex2D(), 0);
      d3d::set_render_target(1, dof_max_coc_far.getTex2D(), 0);
    }
    dofDownscale.render();
    if (hasNearDof)
    {
      d3d::resource_barrier({dof_near_layer->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({dof_coc[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (useFarDof)
        d3d::resource_barrier({dof_far_layer->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    else
    {
      d3d::resource_barrier({dof_far_layer->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (dof_max_coc_far.getTex2D())
      d3d::resource_barrier({dof_max_coc_far.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // 2nd downscale stage (tile min CoC)
  if (hasNearDof)
  {
    TIME_D3D_PROFILE(dof_tiles);
    for (int i = 1; i < dof_coc.size(); i++)
    {
      d3d::set_render_target(dof_coc[i].getTex2D(), 0);
      d3d::set_tex(STAGE_PS, 0, dof_coc[i - 1].getTex2D());
      d3d::set_sampler(STAGE_PS, 0, clampPointSampler);
      dofTile.render();
      d3d::resource_barrier({dof_coc[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  }
  Point2 scale(1. / width, 1. / height);

  ResizableRTarget::Ptr tmpNearDof, tmpFarDof;
  if (hasNearDof)
  {
    tmpNearDof = dofLayerRTPool->acquire();
    tmpNearDof->resize(width, height);
  }
  if (hasFarDof)
  {
    tmpFarDof = dofLayerRTPool->acquire();
    tmpFarDof->resize(width, height);
  }

  // 1st gather pass
  {
    TIME_D3D_PROFILE(dof_far_near_0)
    constexpr int nSquareTapsSide = 7;

    Point4 vTaps[nSquareTapsSide * nSquareTapsSide];
    if (useSimplifiedRendering)
    {
      constexpr int simplifiedDofTaps = 4;
      float fRecipTaps = 1.0f / (simplifiedDofTaps - 1.0f);
      for (int y = 0; y < simplifiedDofTaps; ++y)
        for (int x = 0; x < simplifiedDofTaps; ++x)
          vTaps[x + y * simplifiedDofTaps] =
            to_unit_disk(Point2(x * fRecipTaps, y * fRecipTaps), ((x > 0 && x < 3 && y > 0 && y < 3) ? 2 : 1.5) * scale);
    }
    else
    {
      float fRecipTaps = 1.0f / (nSquareTapsSide - 1.0f);
      for (int y = 0; y < nSquareTapsSide; ++y)
        for (int x = 0; x < nSquareTapsSide; ++x)
          vTaps[x + y * nSquareTapsSide] =
            to_unit_disk(Point2(x * fRecipTaps, y * fRecipTaps), fNumApertureSides, fkernelSize, 2 * scale);
    }
    d3d::set_ps_const(6, &vTaps[0].x, nSquareTapsSide * nSquareTapsSide);

    d3d::set_render_target(hasNearDof ? tmpNearDof->getTex2D() : NULL, 0);
    d3d::set_render_target(1, hasFarDof ? tmpFarDof->getTex2D() : NULL, 0);
    // d3d::set_render_target(2, dof_coc_temp.getTex2D(), 0, false);

    d3d::settex(0, hasNearDof ? dof_near_layer->getTex2D() : NULL);
    d3d::set_sampler(STAGE_PS, 0, clampSampler);
    d3d::settex(1, hasFarDof ? dof_far_layer->getTex2D() : NULL);
    d3d::set_sampler(STAGE_PS, 1, clampSampler);
    d3d::set_tex(STAGE_PS, 2, dof_coc.back().getTex2D());
    d3d::set_sampler(STAGE_PS, 2, clampPointSampler);
    d3d::set_tex(STAGE_PS, 3, dof_max_coc_far.getTex2D());
    d3d::set_sampler(STAGE_PS, 3, clampSampler);

    ShaderGlobal::set_int(num_dof_tapsVarId, nSquareTapsSide * nSquareTapsSide);
    ShaderGlobal::set_int(dof_gather_iterationVarId, 0);
    dofGather.render();
    if (hasNearDof)
      d3d::resource_barrier({tmpNearDof->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (hasFarDof)
      d3d::resource_barrier({tmpFarDof->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // 2nd gather iteration
  {
    TIME_D3D_PROFILE(dof_far_near_1)
    constexpr int nSquareTapsSide = 3;
    Point4 vTaps[nSquareTapsSide * nSquareTapsSide];
    if (useSimplifiedRendering)
    {
      constexpr int simplifiedDofTaps = 2;
      float fRecipTaps = 1.0f / (simplifiedDofTaps - 1.0f);
      for (int y = 0; y < simplifiedDofTaps; ++y)
        for (int x = 0; x < simplifiedDofTaps; ++x)
          vTaps[x + y * simplifiedDofTaps] = to_unit_disk(Point2(x * fRecipTaps, y * fRecipTaps), (2 * 0.15) * scale);
    }
    else
    {
      float fRecipTaps = 1.0f / (nSquareTapsSide - 1.0f);
      for (int y = 0; y < nSquareTapsSide; ++y)
        for (int x = 0; x < nSquareTapsSide; ++x)
          vTaps[x + y * nSquareTapsSide] =
            to_unit_disk(Point2(x * fRecipTaps, y * fRecipTaps), fNumApertureSides, fkernelSize, (2 * 0.15) * scale);
    }
    d3d::set_ps_const(6, &vTaps[0].x, nSquareTapsSide * nSquareTapsSide);

    d3d::set_render_target(hasNearDof ? dof_near_layer->getTex2D() : NULL, 0);
    d3d::set_render_target(1, hasFarDof ? dof_far_layer->getTex2D() : NULL, 0);
    // d3d::set_render_target(2, dof_coc[0].getTex2D(), 0, false);

    d3d::settex(0, tmpNearDof ? tmpNearDof->getTex2D() : nullptr);
    d3d::set_sampler(STAGE_PS, 0, clampSampler);
    d3d::settex(1, tmpFarDof ? tmpFarDof->getTex2D() : nullptr);
    d3d::set_sampler(STAGE_PS, 1, clampSampler);
    d3d::set_tex(STAGE_PS, 2, dof_coc.back().getTex2D());
    d3d::set_sampler(STAGE_PS, 2, clampPointSampler);
    d3d::set_tex(STAGE_PS, 3, dof_max_coc_far.getTex2D());
    d3d::set_sampler(STAGE_PS, 3, clampSampler);
    ShaderGlobal::set_int(num_dof_tapsVarId, nSquareTapsSide * nSquareTapsSide);
    ShaderGlobal::set_int(dof_gather_iterationVarId, 1);
    dofGather.render();
    if (hasNearDof)
      d3d::resource_barrier({dof_near_layer->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (hasFarDof)
      d3d::resource_barrier({dof_far_layer->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  if (useCoCAccumulation && !useSimplifiedRendering && dof_coc_history && dof_coc[0])
  {
    TextureInfo histInfo;
    dof_coc_history.getTex2D()->getinfo(histInfo, 0);
    TextureInfo cocInfo;
    dof_coc[0].getTex2D()->getinfo(cocInfo, 0);
    if (histInfo.w != cocInfo.w || histInfo.h != cocInfo.h)
    {
      dof_coc_history.resize(cocInfo.w, cocInfo.h);
    }
    eastl::swap(dof_coc_history, dof_coc[0]);
  }
}

void DepthOfFieldPS::apply()
{
  if (!height)
    return;
  if (!performedNearDof && !performedFarDof)
    return;
  // Final composition
  TIME_D3D_PROFILE(dof_composite)
  dofComposite.render();
}

void DepthOfFieldPS::setBlendDepthTex(TEXTUREID tex_id)
{
  static int dof_blend_depth_texVarId = get_shader_variable_id("dof_blend_depth_tex", true);
  ShaderGlobal::set_texture(dof_blend_depth_texVarId, tex_id);
}

void DepthOfFieldPS::releaseRTs()
{
  if (dof_far_layer)
  {
    ShaderGlobal::set_texture(dof_far_layerVarId, BAD_TEXTUREID);
    dof_far_layer = nullptr;
  }
  if (dof_near_layer)
  {
    ShaderGlobal::set_texture(dof_near_layerVarId, BAD_TEXTUREID);
    dof_near_layer = nullptr;
  }
}
