// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <animChar/dag_animCharacter2.h>
#include <render/skies.h>
#include <render/world/worldRendererQueries.h>
#include <util/dag_convar.h>
#include <render/daFrameGraph/daFG.h>

extern ConVarT<bool, false> sky_is_unreachable;
CONSOLE_FLOAT_VAL("render", mirror_sky_prepare_altitude_tolerance, 100.0f);

dafg::NodeHandle create_dynamic_mirror_envi_node(DynamicMirrorRenderer &mirror_renderer)
{
  return get_dynamic_mirrors_namespace().registerNode("render_envi", DAFG_PP_NODE_SRC, [&mirror_renderer](dafg::Registry registry) {
    registry.requestRenderPass().depthRoAndBindToShaderVars("mirror_gbuf_depth", {"depth_gbuf"}).color({"mirror_texture"});
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto mirrorCameraHndl = registry.readBlob<DynamicMirrorRenderer::CameraData>("mirror_camera").handle();

    return [mirrorActiveHndl, mirrorCameraHndl, &mirror_renderer]() {
      if (!mirrorActiveHndl.ref())
        return;
      auto skies = get_daskies();
      if (skies == nullptr)
        return;
      auto skiesData = mirror_renderer.getSkiesData();
      if (DAGOR_UNLIKELY(skiesData == nullptr))
        return;
      const auto cameraData = mirrorCameraHndl.get();


      if (DAGOR_LIKELY(!try_render_custom_sky(cameraData->viewTm, cameraData->projTm, cameraData->persp)))
        skies->renderEnvi(sky_is_unreachable, dpoint3(cameraData->viewItm.getcol(3)), dpoint3(cameraData->viewItm.getcol(2)), 2,
          UniqueTex{}, UniqueTex{}, nullptr, skiesData, cameraData->viewTm, cameraData->projTm, cameraData->persp, UpdateSky::On, true,
          mirror_sky_prepare_altitude_tolerance.get());
    };
  });
}
