// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_texture.h>
#include <3d/dag_resPtr.h>
#include <render/renderEvent.h>
#include <render/resolution.h>
#include <render/hdrRender.h>
#include <render/world/wrDispatcher.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/samplerHandle.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderVariableInfo.h>

#include <render/fx/blurredUI.h>


ECS_DECLARE_RELOCATABLE_TYPE(BlurredUI)
ECS_REGISTER_RELOCATABLE_TYPE(BlurredUI, nullptr)
ECS_DECLARE_RELOCATABLE_TYPE(TEXTUREID)
ECS_REGISTER_RELOCATABLE_TYPE(TEXTUREID, nullptr)

CONSOLE_INT_VAL("render", ui_bloom_mip, -1.0, -1, 1);

namespace var
{
static ShaderVariableInfo ui_tex("ui_tex", true);
}

ECS_TAG(render)
static void set_resolution_blurred_ui_manager_es(const SetResolutionEvent &evt,
  BlurredUI &blurred_ui__manager,
  const uint32_t blurred_ui__levels,
  TEXTUREID &blurred_ui__texid,
  d3d::SamplerHandle &blurred_ui__smp,
  TEXTUREID &blurred_ui_sdr__texid,
  d3d::SamplerHandle &blurred_ui_sdr__smp)
{
  if (evt.type != SetResolutionEvent::Type::SETTINGS_CHANGED)
    return;

  blurred_ui__manager.init(evt.displayResolution.x, evt.displayResolution.y, blurred_ui__levels, TextureIDPair());
  blurred_ui_sdr__texid = BAD_TEXTUREID;
  blurred_ui_sdr__smp = d3d::INVALID_SAMPLER_HANDLE;
  if (hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
  {
    blurred_ui__manager.initSdrTex(evt.displayResolution.x, evt.displayResolution.y, blurred_ui__levels);
    blurred_ui_sdr__texid = blurred_ui__manager.getFinalSdr().getId();
    blurred_ui_sdr__smp = blurred_ui__manager.getFinalSdrSampler();
  }
  blurred_ui__texid = blurred_ui__manager.getFinal().getId();
  blurred_ui__smp = blurred_ui__manager.getFinalSampler();
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_blurred_ui_manager_es(const ecs::Event &,
  BlurredUI &blurred_ui__manager,
  const uint32_t blurred_ui__levels,
  TEXTUREID &blurred_ui__texid,
  d3d::SamplerHandle &blurred_ui__smp,
  TEXTUREID &blurred_ui_sdr__texid,
  d3d::SamplerHandle &blurred_ui_sdr__smp)
{
  if (!WRDispatcher::isReadyToUse())
    return;

  IPoint2 rr = get_rendering_resolution();
  blurred_ui__manager.init(rr.x, rr.y, blurred_ui__levels, TextureIDPair());
  blurred_ui_sdr__texid = BAD_TEXTUREID;
  blurred_ui_sdr__smp = d3d::INVALID_SAMPLER_HANDLE;
  if (hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
  {
    blurred_ui__manager.initSdrTex(rr.x, rr.y, blurred_ui__levels);
    blurred_ui_sdr__texid = blurred_ui__manager.getFinalSdr().getId();
    blurred_ui_sdr__smp = blurred_ui__manager.getFinalSdrSampler();
  }
  blurred_ui__texid = blurred_ui__manager.getFinal().getId();
  blurred_ui__smp = blurred_ui__manager.getFinalSampler();
}

static UniqueTex frame_with_ui_for_blur;
static eastl::unique_ptr<PostFxRenderer> ui_blend_renderer;

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const BlurredUI &blurred_ui__manager)
static void close_blurred_ui_resources_es(const ecs::Event &)
{
  frame_with_ui_for_blur.close();
  ui_blend_renderer.reset();
}

static TextureIDPair composite_frame_with_ui(BaseTexture *frame_tex, BaseTexture *ui_tex)
{
  TextureInfo frameInfo;
  frame_tex->getinfo(frameInfo);

  bool needRecreate = !frame_with_ui_for_blur;
  if (!needRecreate)
  {
    TextureInfo compositeInfo;
    frame_with_ui_for_blur.getTex2D()->getinfo(compositeInfo);
    needRecreate = compositeInfo.w != frameInfo.w || compositeInfo.h != frameInfo.h;
  }
  if (needRecreate)
    frame_with_ui_for_blur =
      dag::create_tex(nullptr, frameInfo.w, frameInfo.h, (frameInfo.cflg & TEXFMT_MASK) | TEXCF_RTARGET, 1, "ui_blur_composite");

  d3d::stretch_rect(frame_tex, frame_with_ui_for_blur.getTex2D());

  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(frame_with_ui_for_blur.getTex2D(), 0);
    ShaderGlobal::set_texture(var::ui_tex, ui_tex);
    if (!ui_blend_renderer)
      ui_blend_renderer = eastl::make_unique<PostFxRenderer>("ui_blend");
    ui_blend_renderer->render();
    ShaderGlobal::set_texture(var::ui_tex, nullptr);
  }

  return TextureIDPair{frame_with_ui_for_blur.getTex2D(), frame_with_ui_for_blur.getTexId()};
}

ECS_TAG(render)
static void update_blurred_ui_es(const UpdateBlurredUI &evt, BlurredUI &blurred_ui__manager)
{
  if (!WRDispatcher::isReadyToUse())
    return;

  const ManagedTex &targetFrame = WRDispatcher::getFinalTargetFrame();
  G_ASSERT(d3d::get_driver_code().is(d3d::stub) || targetFrame.getTexId() != BAD_TEXTUREID);
  BaseTexture *uiTex = evt.uiTex;

  TextureIDPair targetPair =
    uiTex ? composite_frame_with_ui(targetFrame.getTex2D(), uiTex) : TextureIDPair{targetFrame.getTex2D(), targetFrame.getTexId()};

  blurred_ui__manager.updateFinalBlurred(targetPair, evt.begin, evt.end, evt.max_mip);

  if (ui_bloom_mip.get() >= 0)
    blurred_ui__manager.getFinal().getTex2D()->texmiplevel(ui_bloom_mip.get(), ui_bloom_mip.get());
}
