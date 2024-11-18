// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"
#include "lensFlareRenderer.h"

#include <render/daBfg/bfg.h>
#include <render/world/cameraParams.h>
#include <render/world/sunParams.h>
#include <render/world/frameGraphHelpers.h>

dabfg::NodeHandle create_lens_flare_prepare_lights_node(LensFlareRenderer *renderer)
{
  return get_lens_flare_namespace().registerNode("prepare_lights", DABFG_PP_NODE_SRC, [renderer](dabfg::Registry registry) {
    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dabfg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto hasLensFlaresHndl = registry.createBlob<int>("has_lens_flares", dabfg::History::No).handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto sunParamsHndl = registry.readBlob<SunParams>("current_sun").handle();

    auto fomShadowsHndl = bindShaderVar("fom_shadows_sin", "fom_shadows_sin").optional().handle();
    bindShaderVar("fom_shadows_cos", "fom_shadows_cos").optional();

    auto volfogHndl = registry.readBlob<OrderingToken>("volfog_token").optional().handle();

    read_gbuffer_depth(registry, dabfg::Stage::CS);

    return [renderer, cameraHndl, sunParamsHndl, hasLensFlaresHndl, fomShadowsHndl, volfogHndl]() {
      const auto &camera = cameraHndl.ref();
      const auto &sun = sunParamsHndl.ref();

      ShaderGlobal::set_int(lens_flare_has_volfogVarId, volfogHndl.get() != nullptr ? 1 : 0);
      ShaderGlobal::set_int(lens_flare_has_fom_shadowsVarId, fomShadowsHndl.get() != nullptr ? 1 : 0);

      renderer->startPreparingLights();
      renderer->collectAndPrepareECSSunFlares(camera.noJitterGlobtm, camera.cameraWorldPos, camera.viewItm.getcol(2), sun.dirToSun,
        Point3::rgb(sun.sunColor));
      bool hasFlaresToRender = renderer->endPreparingLights(camera.cameraWorldPos);
      hasLensFlaresHndl.ref() = hasFlaresToRender ? 1 : 0;
    };
  });
}