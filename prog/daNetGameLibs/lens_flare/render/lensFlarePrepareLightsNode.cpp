// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"

#include <render/lensFlare/render/lensFlareRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraInCamera.h>
#include <render/world/cameraParams.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/world/sunParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>
#include <render/world/shadowsManager.h>
#include <render/clusteredLights.h>
#include <render/skies.h>

dafg::NodeHandle create_lens_flare_prepare_lights_node(LensFlareRenderer *renderer, const int view_id)
{
  char name[] = "prepare_lights#";
  name[eastl::size(name) - 2] = '0' + view_id;

  return get_lens_flare_namespace().registerNode(name, DAFG_PP_NODE_SRC, [renderer, view_id](dafg::Registry registry) {
    auto bindShaderVar = [&](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    const bool isMainView = view_id == 0 || !renderer_has_feature(CAMERA_IN_CAMERA);

    auto hasLensFlaresHndl = isMainView ? registry.modifyBlob<int>("has_lens_flares_view_0").handle()
                                        : registry.renameBlob<int>("has_lens_flares_view_0", "has_lens_flares_view_1").handle();

    const char *cameraParamsName = isMainView ? "current_camera" : "lens_area_camera";
    auto camera = registry.readBlob<CameraParams>(cameraParamsName);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    auto sunParamsHndl = registry.readBlob<SunParams>("current_sun").handle();
    auto mainViewResolution = registry.getResolution<2>("main_view");
    auto mainViewOnlyHndl = registry.readBlob<bool>("main_view_only").handle();

    auto fomShadowsHndl = bindShaderVar("fom_shadows_sin", "fom_shadows_sin").optional().handle();
    bindShaderVar("fom_shadows_cos", "fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    use_volfog(registry, dafg::Stage::CS);

    read_gbuffer_depth(registry, dafg::Stage::CS);
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::CS).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

    return [mainViewOnlyHndl, isMainView, renderer, cameraHndl, sunParamsHndl, hasLensFlaresHndl, fomShadowsHndl, mainViewResolution,
             lens_flare_perpare_globtmVarId = get_shader_glob_var_id("lens_flare_perpare_globtm"),
             lens_flare_prepare_screen_aspect_ratioVarId = get_shader_variable_id("lens_flare_prepare_screen_aspect_ratio")]() {
      if (mainViewOnlyHndl.ref() && !isMainView)
      {
        hasLensFlaresHndl.ref() = false;
        return;
      }
      const auto &camera = cameraHndl.ref();

      camera_in_camera::ApplyPostfxState camcam{dafg::multiplexing::Index{0, 0, 0, isMainView ? 0U : 1U}, camera};

      const auto &sun = sunParamsHndl.ref();
      auto [renderingWidth, renderingHeight] = mainViewResolution.get();

      const auto &lights = WRDispatcher::getClusteredLights();

      ShaderGlobal::set_float4x4(lens_flare_perpare_globtmVarId, camera.noJitterGlobtm);
      ShaderGlobal::set_int(lens_flare_prepare_has_fom_shadowsVarId, fomShadowsHndl.get() != nullptr ? 1 : 0);
      ShaderGlobal::set_float(lens_flare_prepare_screen_aspect_ratioVarId, float(renderingWidth) / float(renderingHeight));

      const Occlusion *occlusion = camera.jobsMgr->getOcclusion();

      renderer->startPreparingLights();
      if (get_daskies() && !get_daskies()->getMoonIsPrimary())
        renderer->collectAndPrepareECSFlares_Sun(camera.viewItm, camera.noJitterGlobtm, sun.visualSunDirFromCameraPos,
          Point3::rgb(sun.sunColor), occlusion);
      renderer->collectAndPrepareECSFlares_PointFlares(camera.cameraWorldPos, camera.viewItm.getcol(2), camera.noJitterFrustum,
        occlusion);

      if (!camera_in_camera::is_lens_render_active())
        renderer->collectAndPrepareECSFlares_DynamicLights();

      bool hasFlaresToRender =
        renderer->endPreparingLights(camera.cameraWorldPos, camera.viewItm.getcol(2), lights.getVisibleClusteredOmniCount(),
          lights.getVisibleClusteredSpotsCount(), WRDispatcher::getShadowInfoProvider().getShadowFramesCount());

      hasLensFlaresHndl.ref() |= hasFlaresToRender ? 1 : 0;
    };
  });
}