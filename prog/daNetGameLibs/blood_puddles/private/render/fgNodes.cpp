// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fgNodes.h"

#include <blood_puddles/public/render/bloodPuddles.h>

#include <shaders/dag_postFxRenderer.h>

#include <render/world/defaultVrsSettings.h>
#include <render/world/frameGraphHelpers.h>

dabfg::NodeHandle make_blood_accumulation_prepass_node()
{
  auto ns = dabfg::root() / "opaque" / "statics";
  return ns.registerNode("blood_decals_accumulation", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto bldBuf0Hndl = registry.createTexture2d("blood_acc_buf0", dabfg::History::No,
      {TEXCF_RTARGET | TEXFMT_R8G8B8A8, registry.getResolution<2>("main_view", 1.0)});
    auto bloodNormalsHndl = registry.createTexture2d("blood_acc_normals", dabfg::History::No,
      {TEXCF_RTARGET | TEXFMT_R8G8, registry.getResolution<2>("main_view", 1.0)});

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto pass = registry.requestRenderPass()
                  .clear(bldBuf0Hndl, make_clear_value(1.0f, 1.0f, 1.0f, 1.0f))
                  .clear(bloodNormalsHndl, make_clear_value(0, 0, 0, 0))
                  .depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"})
                  .color({bldBuf0Hndl, bloodNormalsHndl});

    auto state = registry.requestState().setFrameBlock("global_frame");

    use_default_vrs(pass, state);

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_accumulation", nullptr, 0, "blood_puddles_accumulation", true);

    return [cameraHndl, shHolder = std::move(shHolder)] {
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr)
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
    };
  });
}

dabfg::NodeHandle make_blood_resolve_node()
{
  auto ns = dabfg::root() / "opaque" / "statics";
  return ns.registerNode("blood_puddles_resolve", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto pass = render_to_gbuffer_but_sample_depth(registry);

    registry.readTexture("blood_acc_buf0").atStage(dabfg::Stage::PS).bindToShaderVar("blood_acc_buf0");
    registry.readTexture("blood_acc_normals").atStage(dabfg::Stage::PS).bindToShaderVar("blood_acc_normal");

    auto state = registry.requestState().setFrameBlock("global_frame");

    use_default_vrs(pass, state);

    PostFxRenderer shHolder("blood_puddles_resolve");

    return [shHolder = std::move(shHolder)] { shHolder.render(); };
  });
}

dabfg::NodeHandle make_blood_forward_node()
{
  auto ns = dabfg::root() / "opaque" / "statics";
  return ns.registerNode("blood_puddles_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto pass = render_to_gbuffer_but_sample_depth(registry);

    auto state = registry.requestState().setFrameBlock("global_frame").allowWireframe();

    use_default_vrs(pass, state);

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
