// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include "waterEffects.h"
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <frustumCulling/frustumPlanes.h>

dabfg::NodeHandle makeWaterEffectsPrepareNode()
{
  return dabfg::register_node("water_effects_prepare_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.multiplex(dabfg::multiplexing::Mode::None);
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);
    const auto mainViewRes = registry.getResolution<2>("main_view");
    registry.createTexture2d("wfx_hmap", dabfg::History::No, {TEXFMT_R16F | TEXCF_RTARGET, mainViewRes});
    registry.createTexture2d("wfx_normals", dabfg::History::No, {TEXFMT_R8G8 | TEXCF_RTARGET, mainViewRes});
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      registry.create("wfx_normals_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("wfx_hmap_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    }
  });
}

dabfg::NodeHandle makeWaterEffectsNode(WaterEffects &water_effects)
{
  return dabfg::register_node("water_effects_node", DABFG_PP_NODE_SRC, [&water_effects](dabfg::Registry registry) {
    // TODO: registrar.requestMultiplexing(dabfg::multiplexing::Mode::None);
    // We cannot set multiplexing now, because there is no support for requesting resources from higher multiplex.
    registry.orderMeAfter("water_effects_prepare_node");
    registry.orderMeAfter("acesfx_update_node");
    registry.orderMeBefore("prepare_water_node");

    registry.readTexture("fom_shadows_sin").atStage(dabfg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_sin").optional();
    registry.readTexture("fom_shadows_cos").atStage(dabfg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();
    registry.allowAsyncPipelines();

    auto waterHeightmapHndl =
      registry.modifyTexture("wfx_hmap").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).optional().handle();
    auto waterNormalHndl =
      registry.modifyTexture("wfx_normals").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).optional().handle();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto isUnderwaterHndl = registry.readBlob<bool>("is_underwater").handle();
    use_jitter_frustum_plane_shader_vars(registry);


    return [&water_effects, waterHeightmapHndl, waterNormalHndl, cameraHndl, isUnderwaterHndl](
             dabfg::multiplexing::Index multiplexing_index) {
      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      if (!isFirstIteration)
        return;

      ManagedTexView waterHeightmap = waterHeightmapHndl.view();
      ManagedTexView waterNormal = waterNormalHndl.view();

      // TODO: remove SCOPE_VIEW_PROJ_MATRIX and grs_cur_view here
      DagorCurView savedView = ::grs_cur_view;
      SCOPE_VIEW_PROJ_MATRIX;

      if (waterHeightmap && waterNormal)
      {
        water_effects.render(*get_water(), cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm, cameraHndl.ref().jitterPersp,
          waterHeightmap, waterNormal, isUnderwaterHndl.ref());
      }

      ::grs_cur_view = savedView;
      water_effects.updateWaterProjectedFx();
      water_effects.useWaterTextures();
    };
  });
}
