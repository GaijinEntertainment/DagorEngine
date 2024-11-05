// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <drv/3d/dag_resId.h>
#include <render/renderEvent.h>
#include <render/hdrRender.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

#include <render/fx/blurredUI.h>


ECS_DECLARE_RELOCATABLE_TYPE(BlurredUI)
ECS_REGISTER_RELOCATABLE_TYPE(BlurredUI, nullptr)
ECS_DECLARE_RELOCATABLE_TYPE(TEXTUREID)
ECS_REGISTER_RELOCATABLE_TYPE(TEXTUREID, nullptr)

CONSOLE_INT_VAL("render", ui_bloom_mip, -1.0, -1, 1);

ECS_TAG(render)
static void set_resolution_blurred_ui_manager_es(const SetResolutionEvent &evt,
  BlurredUI &blurred_ui__manager,
  const uint32_t blurred_ui__levels,
  TEXTUREID &blurred_ui__texid,
  TEXTUREID &blurred_ui_sdr__texid)
{
  if (evt.type != SetResolutionEvent::Type::SETTINGS_CHANGED)
    return;

  blurred_ui__manager.init(evt.displayResolution.x, evt.displayResolution.y, blurred_ui__levels, TextureIDPair());
  blurred_ui_sdr__texid = BAD_TEXTUREID;
  if (hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
  {
    blurred_ui__manager.initSdrTex(evt.displayResolution.x, evt.displayResolution.y, blurred_ui__levels);
    blurred_ui_sdr__texid = blurred_ui__manager.getFinalSdr().getId();
  }
  blurred_ui__texid = blurred_ui__manager.getFinal().getId();
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_blurred_ui_manager_es(const ecs::Event &,
  BlurredUI &blurred_ui__manager,
  const uint32_t blurred_ui__levels,
  TEXTUREID &blurred_ui__texid,
  TEXTUREID &blurred_ui_sdr__texid)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer(); // TODO: remove this WR dependency
  if (!renderer)
    return;

  IPoint2 rr;
  renderer->getRenderingResolution(rr.x, rr.y);
  blurred_ui__manager.init(rr.x, rr.y, blurred_ui__levels, TextureIDPair());
  blurred_ui_sdr__texid = BAD_TEXTUREID;
  if (hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
  {
    blurred_ui__manager.initSdrTex(rr.x, rr.y, blurred_ui__levels);
    blurred_ui_sdr__texid = blurred_ui__manager.getFinalSdr().getId();
  }
  blurred_ui__texid = blurred_ui__manager.getFinal().getId();
}

ECS_TAG(render)
static void update_blurred_ui_es(const UpdateBlurredUI &evt, BlurredUI &blurred_ui__manager)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer(); // TODO: remove this WR dependency
  if (!renderer)
    return;

  const ManagedTex &targetFrame = renderer->getFinalTargetTex(); // to be obtained through entity which has targetFrame component
  TextureIDPair targetPair{targetFrame.getTex2D(), targetFrame.getTexId()};
  G_ASSERT(d3d::get_driver_code().is(d3d::stub) || targetPair.getId() != BAD_TEXTUREID);

  blurred_ui__manager.updateFinalBlurred(targetPair, evt.begin, evt.end, evt.max_mip);

  if (ui_bloom_mip.get() >= 0)
    blurred_ui__manager.getFinal().getTex2D()->texmiplevel(ui_bloom_mip.get(), ui_bloom_mip.get());
}
