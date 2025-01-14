// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/renderType.h>
#include <render/xcsm.h>
#include <render/variance.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <util/dag_globDef.h>
#include <shaders/dag_shaders.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_viewMatrix.h>
#include <math/dag_TMatrix4more.h>
#include <EASTL/utility.h>
#include <3d/dag_render.h>
#include <math/dag_math3d.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include <shaders/dag_overrideStates.h>

enum
{
  UPDATE_NONE = 0x00,
  UPDATE_SCENE = 0x01,
  UPDATE_BLUR_X = 0x02,
  UPDATE_BLUR_Y = 0x04,
  UPDATE_BLUR = 0x06,
  UPDATE_ALL = 0x07,
};

class PseudoGaussBlur
{
public:
  int texVarId, texSzVarId;
  PostFxRenderer blurXFx, blurYFx;
  PseudoGaussBlur() { texVarId = texSzVarId = -1; }
  ~PseudoGaussBlur() { clear(); }
  void clear()
  {
    blurXFx.clear();
    blurYFx.clear();
  }
  void init(const char *shaderXName, const char *shaderYName, const IPoint2 texSize);
  void render(TEXTUREID srcTexId, Texture *tempTex, TEXTUREID tempTexId, Texture *targTex, unsigned update_state);
};


void PseudoGaussBlur::init(const char *shaderXName, const char *shaderYName, const IPoint2 texSize)
{

  texVarId = ::get_shader_variable_id("tex");
  texSzVarId = ::get_shader_variable_id("texsz_consts");

  blurXFx.init(shaderXName);
  blurYFx.init(shaderYName);
  blurXFx.getMat()->set_color4_param(texSzVarId, Color4(0.5f, -0.5f, 1.0f / texSize.x, 1.0f / texSize.y));

  blurYFx.getMat()->set_color4_param(texSzVarId, Color4(0.5f, -0.5f, 1.0f / texSize.x, 1.0f / texSize.y));
  // set up weights
}

void PseudoGaussBlur::render(TEXTUREID srcTexId, Texture *tempTex, TEXTUREID tempTexId, Texture *targTex, unsigned update_state)
{
  if (!blurXFx.getMat())
    return;
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  if (update_state & UPDATE_BLUR_X)
  {
    d3d::set_render_target(tempTex, 0);
    blurXFx.getMat()->set_texture_param(texVarId, srcTexId);
    blurXFx.render();
    blurXFx.getMat()->set_texture_param(texVarId, BAD_TEXTUREID);
  }
  if (update_state & UPDATE_BLUR_Y)
  {
    d3d::set_render_target(targTex, 0);
    tempTex->texaddr(TEXADDR_CLAMP);
    blurYFx.getMat()->set_texture_param(texVarId, tempTexId);
    blurYFx.render();
    tempTex->texaddr(TEXADDR_BORDER);
    d3d::resource_barrier({targTex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
}

Variance::Variance() : vsm_shadowmapVarId(-1)
{
  blur = NULL;
  temp_tex = targ_tex = dest_tex = NULL;
  temp_wrtex = targ_wrtex = NULL;
  temp_texId = targ_texId = dest_texId = BAD_TEXTUREID;
  updateBox.setempty();
  updateLightDir.zero();
  updateShadowDist = 0.f;
}

static int vsm_positiveVarId = -1;
void Variance::init(int w, int h, VsmType vsmTypeIn)
{
  vsmType = vsmTypeIn;
  close();
  shaders::OverrideState common;
  common.set(shaders::OverrideState::Z_FUNC | shaders::OverrideState::CULL_NONE);
  common.zFunc = CMPF_LESSEQUAL;
  {
    shaders::OverrideState state = common;
    state.colorWr = 0;
    depthOnlyOverride = shaders::overrides::create(state);
  }

  {
    shaders::OverrideState state = common;
    state.set(shaders::OverrideState::BLEND_OP | shaders::OverrideState::Z_WRITE_DISABLE | shaders::OverrideState::BLEND_SRC_DEST);
    state.blendOp = BLENDOP_MIN;
    state.sblend = BLEND_ONE;
    state.dblend = BLEND_ONE;
    blendOverride = shaders::overrides::create(state);
  }

  width = w;
  height = h;
  blur = new PseudoGaussBlur();
  blur->init("vsm_shadow_blur_x", "vsm_shadow_blur_y", IPoint2(w, h));
  debug("initing vsm %dx%d", w, h);
  unsigned int flags_vsm = TEXCF_RTARGET;
  unsigned usage = d3d::USAGE_FILTER | d3d::USAGE_RTARGET;
  if (vsmType == VSM_BLEND)
    usage = d3d::USAGE_BLEND;
  if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & usage) == usage)
    flags_vsm |= TEXFMT_G16R16F;
  else if ((d3d::get_texformat_usage(TEXFMT_G32R32F) & usage) == usage)
    flags_vsm |= TEXFMT_G32R32F;
  else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & usage) == usage)
    flags_vsm |= TEXFMT_A16B16G16R16F;
  else if ((d3d::get_texformat_usage(TEXFMT_A32B32G32R32F) & usage) == usage)
    flags_vsm |= TEXFMT_A32B32G32R32F;
  else
  {
    if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags_vsm |= TEXFMT_G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_G32R32F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags_vsm |= TEXFMT_G32R32F;
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags_vsm |= TEXFMT_A16B16G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_A32B32G32R32F) & d3d::USAGE_RTARGET) == d3d::USAGE_RTARGET)
      flags_vsm |= TEXFMT_A32B32G32R32F;
    else
      logerr("vsm falling to argb8");
  }

  temp_tex = d3d::create_tex(NULL, w, h, flags_vsm, 1, "temp_vsm_tex");
  d3d_err(temp_tex);
  targ_tex = d3d::create_tex(NULL, w, h, flags_vsm, 1, "targ_vsm_tex");
  d3d_err(targ_tex);

  temp_wrtex = temp_tex;
  targ_wrtex = targ_tex;

  unsigned int flags = TEXCF_RTARGET;
  // although depth it is completely working on DirectX,  it will take more memory (since it is 32 bit, instead of 16 bit)
  /*if (hwDepthSupported && d3d::check_texformat(flags|TEXFMT_DEPTH16))
    flags |= TEXFMT_DEPTH16;
  else */
  if (vsmType == VSM_HW)
  {
    if (d3d::check_texformat(flags | TEXFMT_DEPTH16))
      flags |= TEXFMT_DEPTH16;
    else if (d3d::check_texformat(flags | TEXFMT_DEPTH24))
      flags |= TEXFMT_DEPTH24;
    else
    {
      logerr("hw depth buffer unsupported");
      close();
      return;
    }
  }
  else
  {
    if (d3d::check_texformat(flags | TEXFMT_R16F))
      flags |= TEXFMT_R16F;
    else if (d3d::check_texformat(flags | TEXFMT_R32F))
      flags |= TEXFMT_R32F;
  }

  if (vsmType != VSM_BLEND)
  {
    dest_tex = d3d::create_tex(NULL, w, h, flags, 1, "dest_vsm_tex");
    d3d_err(dest_tex);
  }
  else
  {
    dest_tex = NULL;
    dest_texId = BAD_TEXTUREID;
  }

  debug("VSM hwDepthBuffer = %d hwDepth16bit=%d", vsmType == VSM_HW, (flags & TEXFMT_MASK) == TEXFMT_DEPTH16);
  if (dest_tex)
  {
    dest_tex->texfilter(TEXFILTER_POINT);
    dest_tex->texmipmap(TEXMIPMAP_POINT);

    dest_texId = ::register_managed_tex(String(64, "vsm_dest_tex@shadowTex"), dest_tex);
    dest_tex->texaddr(TEXADDR_CLAMP);
  }
  temp_texId = ::register_managed_tex(String(64, "vsm_temp_tex@shadowTex"), temp_tex);
  temp_tex->texaddr(TEXADDR_BORDER);
  targ_texId = ::register_managed_tex(String(64, "vsm_targ_tex@shadowTex"), targ_tex);
  targ_tex->texaddr(TEXADDR_BORDER);
  temp_tex->texbordercolor(0xFFFFFFFF);
  targ_tex->texbordercolor(0xFFFFFFFF);

  /*Driver3dRenderTarget oldrt;
  d3d::get_render_target(oldrt);
  d3d::set_render_target(targ_tex, 0, false);
  d3d::clearview(CLEAR_TARGET,0xFFFFFFFF,1,0);
  d3d::set_render_target(oldrt);*/


  vsm_shadowmapVarId = ::get_shader_glob_var_id("vsm_shadowmap", true);
  ShaderGlobal::set_texture_fast(vsm_shadowmapVarId, BAD_TEXTUREID);
  vsmShadowProjXVarId = ::get_shader_glob_var_id("vsm_shadow_tm_x", true);
  vsmShadowProjYVarId = ::get_shader_glob_var_id("vsm_shadow_tm_y", true);
  vsmShadowProjZVarId = ::get_shader_glob_var_id("vsm_shadow_tm_z", true);
  vsmShadowProjWVarId = ::get_shader_glob_var_id("vsm_shadow_tm_w", true);
  vsm_hw_filterVarId = ::get_shader_glob_var_id("vsm_hw_filter", true);
  vsm_shadow_tex_sizeVarId = ::get_shader_glob_var_id("vsm_shadow_tex_size", true);
  vsm_positiveVarId = ::get_shader_glob_var_id("vsm_positive", true);
  bool filter = (d3d::get_texformat_usage(flags_vsm) & d3d::USAGE_FILTER);
  ShaderGlobal::set_int_fast(vsm_hw_filterVarId, filter ? 1 : 0);

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;

  // todo: check on non- hw filter supported!
  if (!filter)
  {
    temp_tex->texfilter(TEXFILTER_POINT); ///< just to be sure, that result still valid
    temp_tex->texmipmap(TEXMIPMAP_POINT);
    targ_tex->texfilter(TEXFILTER_POINT); ///< just to be sure, that result still valid
    targ_tex->texmipmap(TEXMIPMAP_POINT);
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.mip_map_mode = d3d::MipMapMode::Point;
    debug("non filtered vsm map");
  }
  ShaderGlobal::set_sampler(::get_shader_glob_var_id("vsm_shadowmap_samplerstate", true), d3d::request_sampler(smpInfo));


  ShaderGlobal::set_color4_fast(vsm_shadow_tex_sizeVarId, Color4(width, height, 1.0 / width, 1.0 / height));
  light_full_update_threshold = 0.04;
  light_update_threshold = 0.005;
  box_full_update_threshold = 50 * 50;
  box_update_threshold = 20 * 20;
  enable_vsm_treshold = 0.8;
  oldLightDir = Point3(0, 0, 0);
  oldBox.setempty();
  forceUpdate();
}

void Variance::close()
{
  setOff();
  del_it(blur);
  ShaderGlobal::set_texture_fast(vsm_shadowmapVarId, BAD_TEXTUREID);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(temp_texId, temp_tex);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(targ_texId, targ_tex);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(dest_texId, dest_tex);
  shaders::overrides::destroy(blendOverride);
  shaders::overrides::destroy(depthOnlyOverride);
}

void Variance::endShadowMap()
{
  if (update_state == UPDATE_NONE)
    return;
  if (update_state & UPDATE_SCENE)
  {
    shaders::overrides::reset();
  }
  if ((update_state & UPDATE_BLUR))
  {
    ShaderGlobal::set_int_fast(vsm_positiveVarId, vsmType == VSM_HW ? 1 : 0);
    TIME_D3D_PROFILE(blurVsm);
    if (dest_tex)
    {
      blur->render(dest_texId, temp_wrtex, temp_texId, targ_wrtex, update_state);
    }
    else
    {
      update_state |= UPDATE_BLUR; // we have to blur both stages in the same time, since both textures become invalid
      // blur->render(targ_texId, temp_wrtex, temp_texId, targ_wrtex, update_state);
      temp_tex->texaddr(TEXADDR_CLAMP);
      blur->render(temp_texId, targ_wrtex, targ_texId, temp_wrtex, update_state);
      temp_tex->texaddr(TEXADDR_BORDER);
      eastl::swap(temp_wrtex, targ_wrtex);
      eastl::swap(temp_tex, targ_tex);
      eastl::swap(temp_texId, targ_texId);
    }
  }

  d3d::set_render_target(oldrt);
  if (update_state & UPDATE_BLUR_Y)
  {
    ShaderGlobal::set_color4_fast(vsmShadowProjXVarId,
      Color4(shadowProjMatrix.m[0][0], shadowProjMatrix.m[1][0], shadowProjMatrix.m[2][0], shadowProjMatrix.m[3][0]));

    ShaderGlobal::set_color4_fast(vsmShadowProjYVarId,
      Color4(shadowProjMatrix.m[0][1], shadowProjMatrix.m[1][1], shadowProjMatrix.m[2][1], shadowProjMatrix.m[3][1]));
    /*if (hwDepthBuffer)//we now also rescale to -1..1,  but in shader
      ShaderGlobal::set_color4_fast(vsmShadowProjZVarId, Color4(
        shadowProjMatrix.m[0][2],
        shadowProjMatrix.m[1][2],
        shadowProjMatrix.m[2][2],
        shadowProjMatrix.m[3][2]));
    else*/
    ShaderGlobal::set_color4_fast(vsmShadowProjZVarId, Color4(shadowProjMatrix.m[0][2] * 2.0, shadowProjMatrix.m[1][2] * 2.0,
                                                         shadowProjMatrix.m[2][2] * 2.0, shadowProjMatrix.m[3][2] * 2.0 - 1.0));
    ShaderGlobal::set_color4_fast(vsmShadowProjWVarId,
      Color4(shadowProjMatrix.m[0][3], shadowProjMatrix.m[1][3], shadowProjMatrix.m[2][3], shadowProjMatrix.m[3][3]));
  }
  // lazy - each frame render only one rtarget

  if (update_state & UPDATE_BLUR_Y)
    update_state = UPDATE_NONE;
  else if (update_state & UPDATE_BLUR_X)
    update_state = UPDATE_BLUR_Y;
  else if (update_state & UPDATE_SCENE)
    update_state = UPDATE_BLUR_X;
  ShaderGlobal::set_texture(vsm_shadowmapVarId, targ_texId);
  ShaderGlobal::set_color4(vsm_shadow_tex_sizeVarId, width, height, 1.0 / width, 1.0 / height);
  d3d_set_view_proj(savedViewProj);
}

void Variance::setOff()
{
  if (VariableMap::isGlobVariablePresent(vsm_shadowmapVarId))
  {
    ShaderGlobal::set_texture(vsm_shadowmapVarId, BAD_TEXTUREID);
    ShaderGlobal::set_color4(vsm_shadow_tex_sizeVarId, 0, 0);
  }
  update_state = UPDATE_NONE;
  forceUpdateFrameDelay = -1;
}

bool Variance::needUpdate(const Point3 &light_dir) const
{
  float diffLight = lengthSq(light_dir - oldLightDir);
  return needUpdate() || diffLight > light_full_update_threshold;
}

bool Variance::startShadowMap(const BBox3 &in_box, const Point3 &in_light_dir_unrm, float in_shadow_dist,
  CullingInfo *out_culling_info)
{
  if (!blur)
    return false;

  d3d_get_view_proj(savedViewProj);

  Point3 in_light_dir = normalize(in_light_dir_unrm);

  int new_update_state = UPDATE_NONE;
  if (forceUpdateFrameDelay >= 0)
  {
    if (forceUpdateFrameDelay == 0)
      new_update_state = UPDATE_ALL;
    forceUpdateFrameDelay--;
  }

  updateBox = in_box;
  updateLightDir = in_light_dir;
  updateShadowDist = in_shadow_dist;

  float diffLight = lengthSq(updateLightDir - oldLightDir);
  if (diffLight > light_full_update_threshold)
    new_update_state |= UPDATE_ALL;
  else if (diffLight > light_update_threshold)
    new_update_state |= UPDATE_SCENE;

  if (!updateBox.isempty() && !oldBox.isempty())
  {
    float diffBox = lengthSq(updateBox.center() - oldBox.center());

    if (diffBox > box_full_update_threshold)
      new_update_state |= UPDATE_ALL;
    else if (diffBox > box_update_threshold)
      new_update_state |= UPDATE_SCENE;
  }
  else if (updateBox.isempty() != oldBox.isempty())
    new_update_state = UPDATE_ALL;

  if (new_update_state)
  {
    if (update_state) // if already updating and need update- force redraw
      update_state = UPDATE_ALL;
    else // old update_state is do nothing. make new one
      update_state = new_update_state;
  } // use previous update state

  if (update_state)
  {
    d3d::get_render_target(oldrt);
  }

  if (!(update_state & UPDATE_SCENE))
    return false;
  if (fabsf(updateLightDir.y) > enable_vsm_treshold)
  {
    setOff();
    return false;
  }
  else
    ShaderGlobal::set_texture_fast(vsm_shadowmapVarId, targ_texId);

  oldLightDir = updateLightDir;
  oldBox = updateBox;

  TMatrix viewTm, viewItm;
  if (!updateBox.isempty())
  {
    viewItm.setcol(3, Point3(0, 0, 0));
    view_matrix_from_look(updateLightDir, viewItm);
    viewTm = orthonormalized_inverse(viewItm);
    BBox3 spacebox = viewTm * updateBox;
    float dist = max(spacebox[1].z - spacebox[0].z + 1.0f, updateShadowDist);
    viewTm.setcol(3, Point3(0, 0, -(spacebox[0].z - 1.0f - (dist - (spacebox[1].z - spacebox[0].z + 1.0f)))));

    viewItm = orthonormalized_inverse(viewTm);

    lightProj = ::matrix_ortho_off_center_lh(spacebox[0].x, spacebox[1].x, spacebox[0].y, spacebox[1].y, 1.0f, dist);
    TMatrix4 lightViewProj = (TMatrix4(viewTm) * lightProj);
    // FIGHT ALIASING
    {
      Point4 origin;
      lightViewProj.transform(Point3(0, 0, 0), origin);
      origin.unify();

      float halfW = 0.5f * (float)width;
      float halfH = 0.5f * (float)height;

      float texCoordX = origin.x * halfW;
      float texCoordY = origin.y * halfH;

      float dx = (floorf(texCoordX) - texCoordX) / halfW;
      float dy = (floorf(texCoordY) - texCoordY) / halfH;
      TMatrix4 tRound;
      tRound.identity();
      tRound._41 = dx;
      tRound._42 = dy;

      lightProj = lightProj * tRound;
    }
    shadowProjMatrix = (TMatrix4(viewTm) * lightProj) * screen_to_tex_scale_tm_xy();
  }
  else
  {
    XCSM xcsm;
    xcsm.init(width, height, 1);
    // lightProj = vpsm.buildMatrix(NULL, 0, box);
    Split split = xcsm.getSplit(0);
    split.isWarp = false;
    split.isAlign = true;
    xcsm.setSplit(0, split);
    xcsm.prepare(savedViewProj.p.wk, savedViewProj.p.hk, savedViewProj.p.zn, updateShadowDist, viewTm, -updateLightDir,
      updateShadowDist);
    lightProj = xcsm.getSplitProj(0);
    // shadowProjMatrix = lightProj * shadowScaleMatrix;
    shadowProjMatrix = xcsm.getSplitTexTM(0);
    view_matrix_from_look(updateLightDir, viewItm);
    viewTm = orthonormalized_inverse(viewItm);

    lightProj = TMatrix4(viewItm) * lightProj;
  }
  unsigned clearFlags = 0;
  if (vsmType == VSM_BLEND)
  {
    shaders::overrides::set(blendOverride);
  }
  else
    clearFlags = CLEAR_ZBUFFER | CLEAR_STENCIL;
  if (vsmType == VSM_HW)
  {
    d3d_err(d3d::set_render_target(0, (Texture *)NULL, 0));
    d3d_err(d3d::set_depth(dest_tex, DepthAccess::RW));
    shaders::overrides::set(depthOnlyOverride);
  }
  else // VSM_BLEND
  {
    d3d_err(d3d::set_render_target(temp_tex, 0));
    clearFlags |= CLEAR_TARGET;
  }

  d3d::clearview(clearFlags, 0xFFFFFFFF, 1, 0);

  // Set new matrices.
  d3d::settm(TM_WORLD, &TMatrix4::IDENT);

  d3d::settm(TM_VIEW, viewTm);
  d3d::settm(TM_PROJ, &lightProj);

  if (out_culling_info)
  {
    out_culling_info->viewInv = viewItm;
    out_culling_info->viewProj = TMatrix4(viewTm) * lightProj;
  }
  return true;
}

void Variance::debugRenderShadowMap() {}
