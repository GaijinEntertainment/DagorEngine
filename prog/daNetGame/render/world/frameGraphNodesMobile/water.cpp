// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "nodes.h"

#include <render/world/global_vars.h>
#include <render/world/private_worldRenderer.h>
#include <main/water.h>

#include <render/deferredRenderer.h>
#include <render/viewVecs.h>

#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_renderTarget.h>


dabfg::NodeHandle mk_water_prepare_mobile_node()
{
  return dabfg::register_node("prepare_water_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("frame_data_setup_mobile");
    registry.requestState().setFrameBlock("global_frame");
    return [] { ShaderGlobal::set_real(water_depth_aboveVarId, -get_waterlevel_for_camera_pos()); };
  });
}

dabfg::NodeHandle mk_water_mobile_node()
{
  return dabfg::register_node("water_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.rename("target_after_panorama_apply", "target_after_water", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::COLOR_ATTACHMENT);
    registry.read("depth_after_opaque").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::UNKNOWN);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();
    registry.requestState().setFrameBlock("global_frame");
    return [cameraHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      const auto &camera = cameraHndl.ref();

      wr.renderWater(camera.viewItm, WorldRenderer::DistantWater::No, false);
    };
  });
}

dabfg::NodeHandle mk_under_water_fog_mobile_node()
{
  return dabfg::register_node("under_water_fog", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    auto targetHndl = registry.rename("target_for_transparency", "target_after_under_water_fog", dabfg::History::No)
                        .texture()
                        .atStage(dabfg::Stage::PS)
                        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                        .handle();
    auto depthHndl = registry.read("depth_for_transparent_effects")
                       .texture()
                       .atStage(dabfg::Stage::PS)
                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                       .handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [targetHndl, depthHndl, cameraHndl, depth_gbufVarId = ::get_shader_variable_id("depth_gbuf")] {
      ShaderGlobal::set_texture(depth_gbufVarId, depthHndl.view());
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (wr.water && is_underwater())
      {
        // todo: render underwater fog with volumeLoight if it is on
        TIME_D3D_PROFILE(under_water_fog);
        d3d::set_render_target(targetHndl.get(), 0);
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
        wr.underWater.render();
        d3d::set_depth(depthHndl.view().getTex2D(), DepthAccess::RW);
      }
    };
  });
}
