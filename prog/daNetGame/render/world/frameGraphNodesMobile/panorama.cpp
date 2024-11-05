// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "nodes.h"

#include <render/skies.h>
#include <render/world/private_worldRenderer.h>
#include <render/world/worldRendererQueries.h>

#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_renderPass.h>

extern ConVarT<bool, false> sky_is_unreachable;

static bool should_render_sky_and_clouds() { return !has_custom_sky_render(CustomSkyRequest::RENDER_IF_EXISTS); }

static void apply(
  DngSkies &skies, SkiesData *main_pov_data, const CameraParams &camera, const bool below_clouds, const TEXTUREID depth_tex_id)
{
  FRAME_LAYER_GUARD(-1);
  if (should_render_sky_and_clouds())
  {
    const bool infinite = sky_is_unreachable && below_clouds;
    const TMatrix &viewTm = camera.viewTm;
    const TMatrix4 &projTm = camera.jitterProjTm;
    const Driver3dPerspective &persp = camera.jitterPersp;
    AuroraBorealis *aurora = nullptr;

    skies.renderSky(main_pov_data, viewTm, projTm, persp, true, aurora);
    skies.renderClouds(infinite, UniqueTex{}, depth_tex_id, main_pov_data, viewTm, projTm);
  }
}

dabfg::NodeHandle mk_panorama_prepare_mobile_node()
{
  return dabfg::register_node("panorama_prepare_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_water_node");
    registry.requestState().setFrameBlock("global_frame");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();
    return [cameraHndl, belowCloudsHndl] {
      auto *skies = get_daskies();
      if (!skies || !skies->panoramaEnabled() || !should_render_sky_and_clouds())
        return;

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();
      const bool below_clouds = belowCloudsHndl.ref();

      SCENE_LAYER_GUARD(-1);
      FRAME_LAYER_GUARD(-1);
      const bool infinite = sky_is_unreachable && below_clouds;
      const DPoint3 &origin = dpoint3(camera.viewItm.getcol(3));
      const DPoint3 &dir = dpoint3(camera.viewItm.getcol(2));
      const uint32_t render_sun_moon = 3;

      SkiesData *data = wr.main_pov_data;
      const TMatrix &viewTm = camera.viewTm;
      const TMatrix4 &projTm = camera.jitterProjTm;
      const bool updateSky = true;
      const bool fixedOffset = false;
      const float altitudeTolerance = 20.0f;
      AuroraBorealis *aurora = nullptr;

      G_ASSERT(data);
      skies->prepareSkyAndClouds(infinite, origin, dir, render_sun_moon, UniqueTex{}, UniqueTex{}, data, viewTm, projTm, updateSky,
        fixedOffset, altitudeTolerance, aurora);
    };
  });
}

dabfg::NodeHandle mk_panorama_apply_mobile_node()
{
  return dabfg::register_node("panorama_apply_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.rename("target_after_resolve", "target_after_panorama_apply", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::UNKNOWN)
      .useAs(dabfg::Usage::UNKNOWN);
    auto depthHndl =
      registry.read("depth_after_opaque").texture().atStage(dabfg::Stage::UNKNOWN).useAs(dabfg::Usage::UNKNOWN).handle();

    registry.requestState().setFrameBlock("global_frame");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();
    return [depthHndl, cameraHndl, belowCloudsHndl,
             enableDiscardByDepthVarId = get_shader_variable_id("panorama_discard_by_subpass_depth")] {
      d3d::next_subpass();

      auto *skies = get_daskies();
      if (!skies || !skies->panoramaEnabled())
        return;
      ShaderGlobal::set_int(enableDiscardByDepthVarId, 1);
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      apply(*skies, wr.main_pov_data, cameraHndl.ref(), belowCloudsHndl.ref(), depthHndl.d3dResId());
      ShaderGlobal::set_int(enableDiscardByDepthVarId, 0);
    };
  });
}

dabfg::NodeHandle mk_panorama_apply_forward_node()
{
  return dabfg::register_node("panorama_apply_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto depthHndl = registry.rename("depth_opaque_dynamic", "depth_after_opaque", dabfg::History::No)
                       .texture()
                       .atStage(dabfg::Stage::UNKNOWN)
                       .useAs(dabfg::Usage::UNKNOWN)
                       .handle();

    registry.requestRenderPass()
      .color({registry.rename("target_opaque_dynamic", "target_after_panorama_apply", dabfg::History::No).texture()})
      .depthRw("depth_after_opaque");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();
    return [depthHndl, cameraHndl, belowCloudsHndl] {
      auto *skies = get_daskies();
      if (!skies || !skies->panoramaEnabled())
        return;
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      apply(*skies, wr.main_pov_data, cameraHndl.ref(), belowCloudsHndl.ref(), depthHndl.d3dResId());
    };
  });
}
