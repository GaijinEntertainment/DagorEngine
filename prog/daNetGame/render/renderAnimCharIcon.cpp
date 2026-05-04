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


#include <render/rendererFeatures.h>
#include <render/world/shadowsManager.h>
#include "world/dynModelRenderPass.h"
#include "world/dynModelRenderer.h"
#include "world/wrDispatcher.h"
#include <ecs/render/renderPasses.h>
#include "EASTL/string.h"
#include "math/dag_TMatrix.h"

using namespace dynmodel_renderer;
extern ShaderBlockIdHolder dynamicSceneBlockId;
extern ShaderBlockIdHolder dynamicSceneTransBlockId;
namespace var
{
static ShaderVariableInfo wetness_params("wetness_params", true);
static ShaderVariableInfo icon_outline_color("icon_outline_color", true);
static ShaderVariableInfo icon_silhouette_color("icon_silhouette_color", true);
static ShaderVariableInfo icon_silhouette_has_shadow("icon_silhouette_has_shadow", true);
static ShaderVariableInfo icon_silhouette_min_shadow("icon_silhouette_min_shadow", true);
static ShaderVariableInfo icon_albedo_gbuf("icon_albedo_gbuf", true);
static ShaderVariableInfo icon_albedo_gbuf_samplerstate("icon_albedo_gbuf_samplerstate", true);
static ShaderVariableInfo icon_normal_gbuf("icon_normal_gbuf", true);
static ShaderVariableInfo icon_normal_gbuf_samplerstate("icon_normal_gbuf_samplerstate", true);
static ShaderVariableInfo icon_material_gbuf("icon_material_gbuf", true);
static ShaderVariableInfo icon_material_gbuf_samplerstate("icon_material_gbuf_samplerstate", true);
static ShaderVariableInfo icon_depth_gbuf("icon_depth_gbuf", true);
static ShaderVariableInfo icon_depth_gbuf_samplerstate("icon_depth_gbuf_samplerstate", true);
static ShaderVariableInfo view_scale_ofs("view_scale_ofs", true);
static ShaderVariableInfo projtm_psf_0("projtm_psf_0", true);
static ShaderVariableInfo projtm_psf_1("projtm_psf_1", true);
static ShaderVariableInfo projtm_psf_2("projtm_psf_2", true);
static ShaderVariableInfo projtm_psf_3("projtm_psf_3", true);
static ShaderVariableInfo render_to_icon("render_to_icon", true);
static ShaderVariableInfo background_alpha("background_alpha", true);
static ShaderVariableInfo animchar_icons_final_samplerstate("animchar_icons_final_samplerstate", true);
}; // namespace var

class RenderAnimCharIcon : public RenderAnimCharIconBase
{
protected:
  virtual bool renderIconAnimChars(
    const IconAnimcharRenderingContext &ctx, const IconSSAA &icon_ssaa, const Driver3dPerspective &persp) override;
  virtual bool needsResolveRenderTarget() override { return true; }
  virtual void ensureDim(int w, int h) override;
  virtual void onBeforeRender(const DataBlock &info) override;
  virtual void onAfterRender() override;
  virtual void setSunLightParams(const Point4 &lightDir, const Point4 &lightColor) override;

  bool fullDeferred = true;
};


void RenderAnimCharIcon::ensureDim(int w, int h)
{
  unsigned texFmt[3] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
  uint32_t expectedRtNum = countof(texFmt);
  fullDeferred = true;

  if (!renderer_has_feature(FeatureRenderFlags::FULL_DEFERRED))
  {
    fullDeferred = false;
    expectedRtNum = 2;
    texFmt[0] = TEXFMT_R11G11B10F;
    texFmt[1] = TEXFMT_R11G11B10F;
  }
  if (target && target->getWidth() >= w && target->getHeight() >= h && expectedRtNum == target->getRtNum())
    return;

  // Release previous target
  target.reset();

  target =
    eastl::make_unique<DeferredRenderTarget>("deferred_immediate_resolve", fullDeferred ? "animchar_icons" : "animchar_icons_thin_g",
      w, h, DeferredRT::StereoMode::MonoOrMultipass, 0, expectedRtNum, texFmt, TEXFMT_DEPTH32);
  d3d::SamplerHandle gbufferSampler = d3d::request_sampler({});

  finalTarget.close();
  finalTargetAA.close();

  finalTarget = dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "animchar_icons_final");
  finalTargetAA = dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "animchar_icons_finalAA");
  finalAA.init("simple_gui_tonemap");
  finalSharpen.init("simple_gui_sharpen");

  ShaderGlobal::set_sampler(var::icon_albedo_gbuf_samplerstate, gbufferSampler);
  ShaderGlobal::set_sampler(var::icon_normal_gbuf_samplerstate, gbufferSampler);
  ShaderGlobal::set_sampler(var::icon_material_gbuf_samplerstate, gbufferSampler);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle gbufferDepthSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(var::icon_depth_gbuf_samplerstate, gbufferDepthSampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(var::animchar_icons_final_samplerstate, d3d::request_sampler(smpInfo));
  }
  return;
}

static void set_view_proj_shaders(TMatrix4 projTm, TMatrix4 globtm, const Point3 &world_view_pos)
{
  projTm = projTm.transpose();
  ShaderGlobal::set_float4(var::projtm_psf_0, Color4(projTm[0]));
  ShaderGlobal::set_float4(var::projtm_psf_1, Color4(projTm[1]));
  ShaderGlobal::set_float4(var::projtm_psf_2, Color4(projTm[2]));
  ShaderGlobal::set_float4(var::projtm_psf_3, Color4(projTm[3]));
  globtm = globtm.transpose();
  ShaderGlobal::set_float4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_float4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_float4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_float4(globtm_psf_3VarId, Color4(globtm[3]));
  ShaderGlobal::set_float4(world_view_posVarId, world_view_pos.x, world_view_pos.y, world_view_pos.z, 1);
}

bool RenderAnimCharIcon::renderIconAnimChars(
  const IconAnimcharRenderingContext &ctx, const IconSSAA &icon_ssaa, const Driver3dPerspective &persp)
{
  bool ret = true;

  for (size_t i = 0; i < ctx.iconAnimchars.size();)
  {
    int nextI = i + 1;
    DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
    d3d::setview(0, 0, icon_ssaa.w, icon_ssaa.h, 0, 1);

    TMatrix4 projTm;
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::setpersp(persp, &projTm);

    const IconAnimchar &ia = ctx.iconAnimchars[i];

    const TMatrix4 &globTm = projTm;
    extern void set_inv_globtm_to_shader(const TMatrix4 &viewTm, const TMatrix4 &projTm, bool optional);
    set_inv_globtm_to_shader(TMatrix4(TMatrix::IDENT), projTm, true);
    // AnimCharV20::prepareFrustum(false, Frustum((mat44f&)globtm));
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
    ShaderGlobal::set_int(var::render_to_icon, 1);
    // if (!animchar->beforeRender())
    //   logerr("not visible in icon! <%s>", info.getStr("animchar"));

    set_view_proj_shaders(projTm, globTm, Point3(0, 0, 0));
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    {
      SCENE_LAYER_GUARD(dynamicSceneBlockId);
      state.process_animchar(0, ShaderMesh::STG_imm_decal, ia.animchar->getSceneInstance(), animchar_additional_data::get_null_data());
      for (; nextI < ctx.iconAnimchars.size(); ++nextI)
      {
        if (ctx.iconAnimchars[nextI].shading == IconAnimchar::SAME)
          state.process_animchar(0, ShaderMesh::STG_imm_decal, ctx.iconAnimchars[nextI].animchar->getSceneInstance(),
            animchar_additional_data::get_null_data());
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

    d3d::set_render_target({}, DepthAccess::RW, {{finalTarget.getTex2D(), 0, 0}});
    d3d::setview(0, 0, icon_ssaa.w, icon_ssaa.h, 0, 1);

    if (i == 0)
      d3d::clearview(CLEAR_TARGET, ctx.info->getE3dcolor("backgroundColor", 0), 0, 0);
    ShaderGlobal::set_float4(var::view_scale_ofs, icon_ssaa.w / float(target->getWidth()), icon_ssaa.h / float(target->getHeight()), 0,
      0);

    ShaderGlobal::set_texture(var::icon_albedo_gbuf, target->getRtId(0));
    ShaderGlobal::set_texture(var::icon_normal_gbuf, target->getRtId(1, true));
    ShaderGlobal::set_texture(var::icon_material_gbuf, target->getRtId(2, true));
    ShaderGlobal::set_texture(var::icon_depth_gbuf, target->getDepthId());
    ShaderGlobal::set_float4(var::icon_outline_color, ia.outline.u == 0 ? Color4(0, 0, 0, 2) : color4(ia.outline));

    const bool shadingSilhouette = ia.shading == IconAnimchar::SILHOUETTE;
    ShaderGlobal::set_float(var::icon_silhouette_has_shadow, shadingSilhouette && ia.silhouetteHasShadow ? 1.0f : 0.0f);
    ShaderGlobal::set_float(var::icon_silhouette_min_shadow, shadingSilhouette ? ia.silhouetteMinShadow : 1.0f);
    ShaderGlobal::set_float4(var::icon_silhouette_color, shadingSilhouette ? color4(ia.silhouette) : Color4(0, 0, 0, 2));

    d3d::resource_barrier({target->getRt(0), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getRt(1, true))
      d3d::resource_barrier({target->getRt(1), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getRt(2, true))
      d3d::resource_barrier({target->getRt(2), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({target->getDepth(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    if (target->getResolveShading())
      target->getResolveShading()->render();

    ShaderGlobal::set_float(var::background_alpha, ctx.info->getReal("backgroundAlpha", 0.5f));
    {
      d3d::set_depth(target->getDepth(), DepthAccess::RW);
      d3d::setview(0, 0, icon_ssaa.w, icon_ssaa.h, 0, 1);
      state.clear();
      SCENE_LAYER_GUARD(dynamicSceneTransBlockId);
      uint32_t stage = ShaderMesh::STG_trans;
      state.process_animchar(stage, stage, ia.animchar->getSceneInstance(), animchar_additional_data::get_null_data());
      for (int j = i + 1; j < nextI; j++)
        state.process_animchar(stage, stage, ctx.iconAnimchars[j].animchar->getSceneInstance(),
          animchar_additional_data::get_null_data());
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
    ShaderGlobal::set_int(var::render_to_icon, 0);
    i = nextI;
  }

  return ret;
}

float originalExposure = 1.0f;
Color4 oldWorldViewPos;
Color4 oldWetnessParams;
Color4 prevSunDir;
Color4 prevSunColor;

void RenderAnimCharIcon::onBeforeRender(const DataBlock &info)
{
  setLightParams(info, fullDeferred);

  oldWorldViewPos = ShaderGlobal::get_float4(world_view_posVarId);
  ShaderElement::invalidate_cached_state_block();
  oldWetnessParams = ShaderGlobal::get_float4(var::wetness_params);
  ShaderGlobal::set_float4(var::wetness_params, 0, -3000, 0, 0);

  originalExposure = const_frame_exposureVarId >= 0 ? ShaderGlobal::get_float(const_frame_exposureVarId) : 1;
  ShaderGlobal::set_float(const_frame_exposureVarId, 1.0f); // exposure is not needed
  ShaderGlobal::set_int(disable_dynmat_paramsVarId, 1);

  prevSunDir = ShaderGlobal::get_float4(from_sun_directionVarId);
  prevSunColor = ShaderGlobal::get_float4(sun_color_0VarId);
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);
}

void RenderAnimCharIcon::onAfterRender()
{
  if (!fullDeferred)
  {
    ShaderGlobal::set_float4(from_sun_directionVarId, prevSunDir);
    ShaderGlobal::set_float4(sun_color_0VarId, prevSunColor);
  }
  ShaderGlobal::set_float4(world_view_posVarId, oldWorldViewPos);
  ShaderGlobal::set_float4(var::wetness_params, oldWetnessParams);

  ShaderGlobal::set_float(const_frame_exposureVarId, originalExposure); // exposure is not needed
  ShaderGlobal::set_int(disable_dynmat_paramsVarId, 0);
}

void RenderAnimCharIcon::setSunLightParams(const Point4 &lightDir, const Point4 &lightColor)
{
  ShaderGlobal::set_float4(from_sun_directionVarId, lightDir);
  ShaderGlobal::set_float4(sun_color_0VarId, lightColor);
}

static RenderAnimCharIconPF<RenderAnimCharIcon> render_animchar_icon_factory;

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(renderAnimCharIcon);
