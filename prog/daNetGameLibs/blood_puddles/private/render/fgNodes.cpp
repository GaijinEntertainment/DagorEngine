// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fgNodes.h"

#include <blood_puddles/public/render/bloodPuddles.h>

#include <shaders/dag_postFxRenderer.h>

#include <render/world/defaultVrsSettings.h>
#include <render/world/frameGraphHelpers.h>

dafg::NodeHandle make_blood_accumulation_prepass_node()
{
  auto ns = dafg::root() / "opaque" / "statics";
  return ns.registerNode("blood_decals_accumulation", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto bldBuf0Hndl = registry
                         .createTexture2d("blood_acc_buf0", dafg::History::No,
                           {TEXCF_RTARGET | TEXFMT_R8G8B8A8, registry.getResolution<2>("main_view", 1.0)})
                         .clear(make_clear_value(1.0f, 1.0f, 1.0f, 1.0f));
    auto bloodNormalsHndl = registry
                              .createTexture2d("blood_acc_normals", dafg::History::No,
                                {TEXCF_RTARGET | TEXFMT_R8G8, registry.getResolution<2>("main_view", 1.0)})
                              .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.requestRenderPass()
      .depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"})
      .color({bldBuf0Hndl, bloodNormalsHndl})
      .vrsRate(VRS_RATE_TEXTURE_NAME);

    registry.requestState().setFrameBlock("global_frame");

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_accumulation", nullptr, 0, "blood_puddles_accumulation", true);

    return [cameraHndl, shHolder = std::move(shHolder)] {
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr)
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
    };
  });
}

dafg::NodeHandle make_blood_resolve_node()
{
  auto ns = dafg::root() / "opaque" / "statics";
  return ns.registerNode("blood_puddles_resolve", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    registry.readTexture("blood_acc_buf0").atStage(dafg::Stage::PS).bindToShaderVar("blood_acc_buf0");
    registry.readTexture("blood_acc_normals").atStage(dafg::Stage::PS).bindToShaderVar("blood_acc_normal");

    registry.requestState().setFrameBlock("global_frame");

    PostFxRenderer shHolder("blood_puddles_resolve");

    return [shHolder = std::move(shHolder)] { shHolder.render(); };
  });
}

dafg::NodeHandle make_blood_forward_node()
{
  auto ns = dafg::root() / "opaque" / "statics";
  return ns.registerNode("blood_puddles_forward", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_forward", nullptr, 0, "blood_puddles_forward", true);

    return [cameraHndl, shHolder = std::move(shHolder)] {
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr)
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
    };
  });
}
