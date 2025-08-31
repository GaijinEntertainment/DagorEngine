// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_renderAnimCharIconBase.h>
#include <animChar/dag_animCharacter2.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <ioSys/dag_dataBlock.h>
#include <render/deferredRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include "world/global_vars.h"
#include <3d/dag_render.h> //for global state

#include "world/dynModelRenderPass.h"
#include "world/dynModelRenderer.h"
#define INSIDE_RENDERER 1
#include "world/private_worldRenderer.h"
#include "world/wrDispatcher.h"
#include <ecs/render/renderPasses.h>
#include "EASTL/string.h"
#include "math/dag_TMatrix.h"

using namespace dynmodel_renderer;
extern ShaderBlockIdHolder dynamicSceneBlockId;
extern ShaderBlockIdHolder dynamicSceneTransBlockId;

class RenderAnimCharIcon : public RenderAnimCharIconBase
{
protected:
  virtual bool renderIconAnimChars(const IconAnimcharWithAttachments &iconAnimchars,
    Texture *to,
    const Driver3dPerspective &persp,
    int x,
    int y,
    int w,
    int h,
    int dstw,
    int dsth,
    const DataBlock &info) override;
  virtual bool needsResolveRenderTarget() override { return !isForward; }
  virtual bool ensureDim(int w, int h) override;
  virtual void beforeRender(const DataBlock &info) override;
  virtual void afterRender() override;
  virtual void setSunLightParams(const Point4 &lightDir, const Point4 &lightColor) override;

  bool fullDeferred = true;
  bool mobileDeferred = false;
  bool isForward = false;
};

#define GLOBAL_VARS_ANIMCHAR_ICONS_LIST \
  VAR(wetness_params)                   \
  VAR(icon_outline_color)               \
  VAR(icon_silhouette_color)            \
  VAR(icon_silhouette_has_shadow)       \
  VAR(icon_silhouette_min_shadow)       \
  VAR(icon_albedo_gbuf)                 \
  VAR(icon_albedo_gbuf_samplerstate)    \
  VAR(icon_normal_gbuf)                 \
  VAR(icon_normal_gbuf_samplerstate)    \
  VAR(icon_material_gbuf)               \
  VAR(icon_material_gbuf_samplerstate)  \
  VAR(icon_depth_gbuf)                  \
  VAR(icon_depth_gbuf_samplerstate)     \
  VAR(icon_lights_count)                \
  VAR(icon_light_dirs)                  \
  VAR(icon_light_colors)                \
  VAR(view_scale_ofs)                   \
  VAR(projtm_psf_0)                     \
  VAR(projtm_psf_1)                     \
  VAR(projtm_psf_2)                     \
  VAR(projtm_psf_3)                     \
  VAR(render_to_icon)                   \
  VAR(background_alpha)                 \
  VAR(animchar_icons_final_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_ANIMCHAR_ICONS_LIST
#undef VAR


bool RenderAnimCharIcon::ensureDim(int w, int h)
{
  unsigned texFmt[3] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
  uint32_t expectedRtNum = countof(texFmt);
  fullDeferred = true;
  mobileDeferred = false;

  const IRenderWorld *irw = get_world_renderer();
  if (irw == nullptr)
    return false;

  const WorldRenderer &wr = *(WorldRenderer *)irw;
  if (!wr.hasFeature(FeatureRenderFlags::FULL_DEFERRED))
  {
    fullDeferred = false;
    if (wr.hasFeature(FeatureRenderFlags::MOBILE_DEFERRED))
    {
      mobileDeferred = true;
      isForward = false;
      if (wr.hasFeature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS))
      {
        expectedRtNum = 2;
        texFmt[0] = TEXFMT_R8G8B8A8;
        texFmt[1] = TEXFMT_R8G8;
      }
      else
      {
        expectedRtNum = 3;
        texFmt[0] = TEXFMT_R8G8B8A8;
        texFmt[1] = TEXFMT_R8G8B8A8;
        texFmt[2] = TEXFMT_R8G8;
      }
    }
    else if (wr.hasFeature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      isForward = true;
      expectedRtNum = 1;
      texFmt[0] = TEXFMT_R11G11B10F;
    }
    else
    {
      expectedRtNum = 2;
      texFmt[0] = TEXFMT_R11G11B10F;
      texFmt[1] = TEXFMT_R11G11B10F;
    }
  }
  if (target && target->getWidth() >= w && target->getHeight() >= h && expectedRtNum == target->getRtNum())
    return true;

  // Release previous target
  target.reset();

  target =
    eastl::make_unique<DeferredRenderTarget>("deferred_immediate_resolve", fullDeferred ? "animchar_icons" : "animchar_icons_thin_g",
      w, h, DeferredRT::StereoMode::MonoOrMultipass, 0, expectedRtNum, texFmt, TEXFMT_DEPTH32);
  d3d::SamplerHandle gbufferSampler = d3d::request_sampler({});

  finalTarget.close();
  if (!isForward)
  {
    finalTarget = dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "animchar_icons_final");
    finalAA.init("simple_gui_tonemap");
  }

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_ANIMCHAR_ICONS_LIST
#undef VAR

  ShaderGlobal::set_sampler(icon_albedo_gbuf_samplerstateVarId, gbufferSampler);
  ShaderGlobal::set_sampler(icon_normal_gbuf_samplerstateVarId, gbufferSampler);
  ShaderGlobal::set_sampler(icon_material_gbuf_samplerstateVarId, gbufferSampler);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle gbufferDepthSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(icon_depth_gbuf_samplerstateVarId, gbufferDepthSampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(animchar_icons_final_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  return true;
}

static void set_view_proj_shaders(TMatrix4 projTm, TMatrix4 globtm, const Point3 &world_view_pos)
{
  projTm = projTm.transpose();
  ShaderGlobal::set_color4(projtm_psf_0VarId, Color4(projTm[0]));
  ShaderGlobal::set_color4(projtm_psf_1VarId, Color4(projTm[1]));
  ShaderGlobal::set_color4(projtm_psf_2VarId, Color4(projTm[2]));
  ShaderGlobal::set_color4(projtm_psf_3VarId, Color4(projTm[3]));
  globtm = globtm.transpose();
  ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtm[3]));
  ShaderGlobal::set_color4(world_view_posVarId, world_view_pos.x, world_view_pos.y, world_view_pos.z, 1);
}

bool RenderAnimCharIcon::renderIconAnimChars(const IconAnimcharWithAttachments &icon_animchars,
  Texture *to,
  const Driver3dPerspective &persp,
  int x,
  int y,
  int w,
  int h,
  int dstw,
  int dsth,
  const DataBlock &info)
{
  bool ret = true;

  if (isForward)
    ShaderGlobal::set_sampler(get_shader_variable_id("static_shadow_tex_samplerstate", true), d3d::request_sampler({}));

  for (size_t i = 0; i < icon_animchars.size();)
  {
    int nextI = i + 1;
    DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
    d3d::setview(0, 0, w, h, 0, 1);

    TMatrix4 projTm;
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::setpersp(persp, &projTm);

    const IconAnimchar &ia = icon_animchars[i];

    const TMatrix4 &globTm = projTm;
    extern void set_inv_globtm_to_shader(const TMatrix4 &viewTm, const TMatrix4 &projTm, bool optional);
    set_inv_globtm_to_shader(TMatrix4(TMatrix::IDENT), projTm, true);
    // AnimCharV20::prepareFrustum(false, Frustum((mat44f&)globtm));
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
    ShaderGlobal::set_int(render_to_iconVarId, 1);
    // if (!animchar->beforeRender())
    //   logerr("not visible in icon! <%s>", info.getStr("animchar"));

    set_view_proj_shaders(projTm, globTm, Point3(0, 0, 0));
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    {
      SCENE_LAYER_GUARD(dynamicSceneBlockId);
      state.process_animchar(0, ShaderMesh::STG_imm_decal, ia.animchar->getSceneInstance(), animchar_additional_data::get_null_data(),
        false);
      for (; nextI < icon_animchars.size(); ++nextI)
      {
        if (icon_animchars[nextI].shading == IconAnimchar::SAME)
          state.process_animchar(0, ShaderMesh::STG_imm_decal, icon_animchars[nextI].animchar->getSceneInstance(),
            animchar_additional_data::get_null_data(), false);
        else
          break;
      }
      if (!state.empty())
      {
        state.prepareForRender();
        const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
        if (buffer)
        {
          state.setVars(buffer->buffer.getBufId());
          SCENE_LAYER_GUARD(dynamicSceneBlockId);
          state.render(buffer->curOffset);
        }
        else
          ret = false;
      }
    }

    if (!isForward)
    {
      d3d::set_render_target(finalTarget.getTex2D(), 0);
      d3d::setview(0, 0, w, h, 0, 1);
    }
    else
    {
      if (!to)
        d3d::set_render_target();
      else
        d3d::set_render_target(to, 0);
      d3d::setview(x, y, dstw, dsth, 0, 1);
      WRDispatcher::getShadowsManager().restoreShadowSampler();
    }

    if (i == 0)
      d3d::clearview(CLEAR_TARGET, info.getE3dcolor("backgroundColor", 0), 0, 0);
    ShaderGlobal::set_color4(view_scale_ofsVarId, w / float(target->getWidth()), h / float(target->getHeight()), 0, 0);

    ShaderGlobal::set_texture(icon_albedo_gbufVarId, target->getRtId(0));
    ShaderGlobal::set_texture(icon_normal_gbufVarId, target->getRtId(1, true));
    ShaderGlobal::set_texture(icon_material_gbufVarId, target->getRtId(2, true));
    ShaderGlobal::set_texture(icon_depth_gbufVarId, target->getDepthId());
    ShaderGlobal::set_color4(icon_outline_colorVarId, ia.outline.u == 0 ? Color4(0, 0, 0, 2) : color4(ia.outline));
    ShaderGlobal::set_real(icon_silhouette_has_shadowVarId, ia.silhouetteHasShadow ? 1.0f : 0.0f);
    ShaderGlobal::set_real(icon_silhouette_min_shadowVarId, ia.silhouetteMinShadow);
    ShaderGlobal::set_color4(icon_silhouette_colorVarId,
      ia.shading == IconAnimchar::SILHOUETTE ? color4(ia.silhouette) : Color4(0, 0, 0, 2));
    d3d::resource_barrier({target->getRt(0), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getRt(1, true))
      d3d::resource_barrier({target->getRt(1), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getRt(2, true))
      d3d::resource_barrier({target->getRt(2), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({target->getDepth(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getResolveShading())
      target->getResolveShading()->render();

    ShaderGlobal::set_real(background_alphaVarId, info.getReal("backgroundAlpha", 0.5f));
    {
      d3d::set_depth(target->getDepth(), DepthAccess::RW);
      d3d::setview(0, 0, w, h, 0, 1);
      state.clear();
      SCENE_LAYER_GUARD(dynamicSceneTransBlockId);
      uint32_t stage = ShaderMesh::STG_trans;
      state.process_animchar(stage, stage, ia.animchar->getSceneInstance(), animchar_additional_data::get_null_data(), false);
      for (int j = i + 1; j < nextI; j++)
        state.process_animchar(stage, stage, icon_animchars[j].animchar->getSceneInstance(), animchar_additional_data::get_null_data(),
          false);
      if (!state.empty())
      {
        state.prepareForRender();
        const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
        if (buffer)
        {
          state.setVars(buffer->buffer.getBufId());
          state.render(buffer->curOffset);
        }
        else
          ret = false;
      }
    }
    ShaderGlobal::set_int(render_to_iconVarId, 0);
    i = nextI;
  }

  return ret;
}

float originalExposure = 1.0f;
Color4 oldWorldViewPos;
Color4 oldWetnessParams;
Color4 prevSunDir;
Color4 prevSunColor;

void RenderAnimCharIcon::beforeRender(const DataBlock &info)
{
  setLightParams(info, fullDeferred, mobileDeferred);

  oldWorldViewPos = ShaderGlobal::get_color4(world_view_posVarId);
  ShaderElement::invalidate_cached_state_block();
  oldWetnessParams = ShaderGlobal::get_color4(wetness_paramsVarId);
  ShaderGlobal::set_color4(wetness_paramsVarId, 0, -3000, 0, 0);

  originalExposure = const_frame_exposureVarId >= 0 ? ShaderGlobal::get_real(const_frame_exposureVarId) : 1;
  ShaderGlobal::set_real(const_frame_exposureVarId, 1.0f); // exposure is not needed
  ShaderGlobal::set_int(disable_dynmat_paramsVarId, 1);

  prevSunDir = ShaderGlobal::get_color4(from_sun_directionVarId);
  prevSunColor = ShaderGlobal::get_color4(sun_color_0VarId);
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);
}

void RenderAnimCharIcon::afterRender()
{
  if (!fullDeferred)
  {
    ShaderGlobal::set_color4(from_sun_directionVarId, prevSunDir);
    ShaderGlobal::set_color4(sun_color_0VarId, prevSunColor);
  }
  ShaderGlobal::set_color4(world_view_posVarId, oldWorldViewPos);
  ShaderGlobal::set_color4(wetness_paramsVarId, oldWetnessParams);

  ShaderGlobal::set_real(const_frame_exposureVarId, originalExposure); // exposure is not needed
  ShaderGlobal::set_int(disable_dynmat_paramsVarId, 0);
}

void RenderAnimCharIcon::setSunLightParams(const Point4 &lightDir, const Point4 &lightColor)
{
  ShaderGlobal::set_color4(from_sun_directionVarId, lightDir);
  ShaderGlobal::set_color4(sun_color_0VarId, lightColor);
}

static RenderAnimCharIconPF<RenderAnimCharIcon> render_animchar_icon_factory;

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(renderAnimCharIcon);
