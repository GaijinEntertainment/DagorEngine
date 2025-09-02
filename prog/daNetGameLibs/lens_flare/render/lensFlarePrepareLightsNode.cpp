// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"

#include <render/lensFlare/render/lensFlareRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraInCamera.h>
#include <render/world/cameraParams.h>
#include <render/world/sunParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>
#include <render/world/shadowsManager.h>
#include <render/clusteredLights.h>
#include <render/skies.h>

dafg::NodeHandle create_lens_flare_prepare_lights_node(LensFlareRenderer *renderer)
{
  return get_lens_flare_namespace().registerNode("prepare_lights", DAFG_PP_NODE_SRC, [renderer](dafg::Registry registry) {
    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto hasLensFlaresHndl = registry.createBlob<int>("has_lens_flares", dafg::History::No).handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto sunParamsHndl = registry.readBlob<SunParams>("current_sun").handle();

    auto fomShadowsHndl = bindShaderVar("fom_shadows_sin", "fom_shadows_sin").optional().handle();
    bindShaderVar("fom_shadows_cos", "fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    use_volfog(registry, dafg::Stage::CS);

    read_gbuffer_depth(registry, dafg::Stage::CS);
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::CS).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

    return [renderer, cameraHndl, sunParamsHndl, hasLensFlaresHndl, fomShadowsHndl,
             lens_flare_perpare_globtmVarId = get_shader_glob_var_id("lens_flare_perpare_globtm")]() {
      if (camera_in_camera::is_lens_render_active())
      {
        hasLensFlaresHndl.ref() = 0;
        return;
      }

      const auto &camera = cameraHndl.ref();
      const auto &sun = sunParamsHndl.ref();

      const auto &lights = WRDispatcher::getClusteredLights();

      ShaderGlobal::set_float4x4(lens_flare_perpare_globtmVarId, camera.noJitterGlobtm);
      ShaderGlobal::set_int(lens_flare_prepare_has_fom_shadowsVarId, fomShadowsHndl.get() != nullptr ? 1 : 0);

      renderer->startPreparingLights();
      if (get_daskies() && !get_daskies()->getMoonIsPrimary())
        renderer->collectAndPrepareECSFlares_Sun(camera.viewItm.getcol(2), camera.noJitterGlobtm, sun.visualSunDirFromCameraPos,
          Point3::rgb(sun.sunColor));
      renderer->collectAndPrepareECSFlares_PointFlares(camera.cameraWorldPos, camera.viewItm.getcol(2), camera.noJitterFrustum);
      renderer->collectAndPrepareECSFlares_DynamicLights();
      bool hasFlaresToRender =
        renderer->endPreparingLights(camera.cameraWorldPos, camera.viewItm.getcol(2), lights.getVisibleClusteredOmniCount(),
          lights.getVisibleClusteredSpotsCount(), WRDispatcher::getShadowInfoProvider().getShadowFramesCount());
      hasLensFlaresHndl.ref() = hasFlaresToRender ? 1 : 0;
    };
  });
}