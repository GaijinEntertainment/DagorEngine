#include <3d/dag_drv3d.h>
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

#define GLOBAL_VARS_LIST               \
  VAR(dof_frame_tex)                   \
  VAR(dof_near_layer)                  \
  VAR(dof_far_layer)                   \
  VAR(dof_rt_size)                     \
  VAR(dof_downsampled_frame_tex)       \
  VAR(dof_downsampled_close_depth_tex) \
  VAR(dof_gather_iteration)            \
  VAR(dof_focus_params)                \
  VAR(dof_focus_linear_params)         \
  VAR(num_dof_taps)                    \
  VAR(dof_focus_far_mode)              \
  VAR(dof_focus_near_mode)             \
  VAR(simplified_dof)                  \
  VAR(dof_coc_history)

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
  if (!dof_near_layer[0].getTex2D())
    return;
  for (ResizableTex &holder : dof_near_layer)
    holder.close();
  for (int i = 0; i < dof_coc.size(); i++)
    dof_coc[i].close();
  dof_coc_history.close();
}

void DepthOfFieldPS::initNear()
{
  if (dof_near_layer[0].getTex2D() && useCoCAccumulation == bool(dof_coc_history))
    return;
  uint32_t flg = TEXCF_RTARGET;
  for (uint32_t i = 0; i < dof_near_layer.size(); ++i)
  {
    String name(128, "dof_near_layer%d", i);
    dof_near_layer[i] = dag::create_tex(NULL, originalWidth, originalHeight, flg | TEXFMT_A16B16G16R16F, 1, name);
    if (dof_near_layer[i].getTex2D())
      dof_near_layer[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  for (int i = 0; i < dof_coc.size(); i++)
  {
    int wd = 1 << i;
    String name(128, "dof_coc%d", i);
    dof_coc[i] = dag::create_tex(NULL, (originalWidth + wd - 1) / wd, (originalHeight + wd - 1) / wd, flg | TEXFMT_R8, 1, name);
    if (dof_coc[i].getTex2D())
      dof_coc[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  if (useCoCAccumulation && !useSimplifiedRendering)
  {
    dof_coc_history = dag::create_tex(NULL, originalWidth, originalHeight, flg | TEXFMT_R8, 1, "dof_coc_history");
    if (dof_coc_history.getTex2D())
      dof_coc_history.getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  else
  {
    dof_coc_history.close();
  }
  if (originalWidth != width || originalHeight != height)
    changeNearResolution();
}

void DepthOfFieldPS::changeNearResolution()
{
  if (!dof_near_layer[0].getTex2D())
    return;
  for (ResizableTex &holder : dof_near_layer)
  {
    holder.resize(width, height);
    holder.getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  for (int i = 0; i < dof_coc.size(); i++)
  {
    int wd = 1 << i;
    dof_coc[i].resize((width + wd - 1) / wd, (height + wd - 1) / wd);
    dof_coc[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }
}

void DepthOfFieldPS::closeFar()
{
  if (!dof_far_layer[0].getTex2D())
    return;
  for (ResizableTex &holder : dof_far_layer)
    holder.close();
  dof_max_coc_far.close();
}

void DepthOfFieldPS::initFar()
{
  if (dof_far_layer[0].getTex2D())
    return;
  uint32_t flg = TEXCF_RTARGET;
  for (uint32_t i = 0; i < dof_far_layer.size(); ++i)
  {
    String name(128, "dof_far_layer%d", i);
    dof_far_layer[i] = dag::create_tex(NULL, originalWidth, originalHeight, flg | TEXFMT_A16B16G16R16F, 1, name);
    if (dof_far_layer[i].getTex2D())
      dof_far_layer[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }

  dof_max_coc_far = dag::create_tex(NULL, originalWidth, originalHeight, flg | TEXFMT_L8, 1, "dof_max_coc_far");
  if (dof_max_coc_far.getTex2D())
    dof_max_coc_far.getTex2D()->texaddr(TEXADDR_CLAMP);
  if (originalWidth != width || originalHeight != height)
    changeFarResolution();
}

void DepthOfFieldPS::changeFarResolution()
{
  if (!dof_far_layer[0].getTex2D())
    return;
  for (ResizableTex &holder : dof_far_layer)
  {
    holder.resize(width, height);
    holder.getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  dof_max_coc_far.resize(width, height);
  dof_max_coc_far.getTex2D()->texaddr(TEXADDR_CLAMP);
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

  // dof_coc_temp.set(d3d::create_tex(NULL, width, height, 1, TEXFMT_R16G16F|TEXCF_RTARGET, "dof_coc_temp"), "dof_coc_temp");
  ShaderGlobal::set_color4(dof_rt_sizeVarId, width, height, 1. / width, 1. / height);
  dofTile.init("dof_tile");
  dofGather.init("dof_gather");
  dofComposite.init("dof_composite");
  if (dof_far_layer[0].getTex2D())
  {
    closeFar();
    initFar();
  }
  if (dof_near_layer[0].getTex2D())
  {
    closeNear();
    initNear();
  }
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

  ShaderGlobal::set_texture(dof_far_layerVarId, hasFarDof ? dof_far_layer[0].getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_texture(dof_near_layerVarId, hasNearDof ? dof_near_layer[0].getTexId() : BAD_TEXTUREID);

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
    if (hasNearDof)
    {
      d3d::set_render_target(dof_near_layer[0].getTex2D(), 0);
      d3d::set_render_target(1, dof_coc[0].getTex2D(), 0);
      d3d::set_render_target(2, dof_far_layer[0].getTex2D(), 0);
      d3d::set_render_target(3, dof_max_coc_far.getTex2D(), 0);
    }
    else
    {
      d3d::set_render_target(dof_far_layer[0].getTex2D(), 0);
      d3d::set_render_target(1, dof_max_coc_far.getTex2D(), 0);
    }
    dofDownscale.render();
    if (hasNearDof)
    {
      d3d::resource_barrier({dof_near_layer[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({dof_coc[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (dof_far_layer[0].getTex2D())
        d3d::resource_barrier({dof_far_layer[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    else
    {
      d3d::resource_barrier({dof_far_layer[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
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
      dof_coc[i - 1].getTex2D()->texfilter(TEXFILTER_POINT);
      d3d::settex(0, dof_coc[i - 1].getTex2D());
      dofTile.render();
      d3d::resource_barrier({dof_coc[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  }
  Point2 scale(1. / width, 1. / height);
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

    d3d::set_render_target(hasNearDof ? dof_near_layer[1].getTex2D() : NULL, 0);
    d3d::set_render_target(1, hasFarDof ? dof_far_layer[1].getTex2D() : NULL, 0);
    // d3d::set_render_target(2, dof_coc_temp.getTex2D(), 0, false);

    if (dof_coc.back().getTex2D())
      dof_coc.back().getTex2D()->texfilter(TEXFILTER_POINT);

    d3d::settex(0, dof_near_layer[0].getTex2D());
    d3d::settex(1, dof_far_layer[0].getTex2D());
    d3d::settex(2, dof_coc.back().getTex2D());
    d3d::settex(3, dof_max_coc_far.getTex2D());

    ShaderGlobal::set_int(num_dof_tapsVarId, nSquareTapsSide * nSquareTapsSide);
    ShaderGlobal::set_int(dof_gather_iterationVarId, 0);
    dofGather.render();
    if (hasNearDof)
      d3d::resource_barrier({dof_near_layer[1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (hasFarDof)
      d3d::resource_barrier({dof_far_layer[1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
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

    d3d::set_render_target(hasNearDof ? dof_near_layer[0].getTex2D() : NULL, 0);
    d3d::set_render_target(1, hasFarDof ? dof_far_layer[0].getTex2D() : NULL, 0);
    // d3d::set_render_target(2, dof_coc[0].getTex2D(), 0, false);

    if (dof_coc.back().getTex2D())
      dof_coc.back().getTex2D()->texfilter(TEXFILTER_POINT);

    d3d::settex(0, dof_near_layer[1].getTex2D());
    d3d::settex(1, dof_far_layer[1].getTex2D());
    d3d::settex(2, dof_coc.back().getTex2D());
    d3d::settex(3, dof_max_coc_far.getTex2D());
    ShaderGlobal::set_int(num_dof_tapsVarId, nSquareTapsSide * nSquareTapsSide);
    ShaderGlobal::set_int(dof_gather_iterationVarId, 1);
    dofGather.render();
    if (hasNearDof)
      d3d::resource_barrier({dof_near_layer[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (hasFarDof)
      d3d::resource_barrier({dof_far_layer[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  if (useCoCAccumulation && !useSimplifiedRendering && dof_coc_history && dof_coc[0])
  {
    eastl::swap(dof_coc_history, dof_coc[0]);
    dof_coc_history->texfilter(TEXFILTER_SMOOTH);
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