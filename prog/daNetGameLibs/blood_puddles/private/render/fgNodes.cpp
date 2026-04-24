// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fgNodes.h"

#include <blood_puddles/public/render/bloodPuddles.h>

#include <shaders/dag_postFxRenderer.h>

#include <render/world/defaultVrsSettings.h>
#include <render/world/frameGraphHelpers.h>

eastl::array<dafg::NodeHandle, 2> make_blood_accumulation_prepass_node()
{
  auto ns = dafg::root() / "opaque" / "dynamic_rendinst";

  auto perCameraResNode = ns.registerNode("blood_per_camera_res", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createTexture2d("blood_acc_buf0", {TEXCF_RTARGET | TEXFMT_R8G8B8A8, registry.getResolution<2>("main_view", 1.0)})
      .clear(make_clear_value(1.0f, 1.0f, 1.0f, 1.0f));
    registry.createTexture2d("blood_acc_normals", {TEXCF_RTARGET | TEXFMT_R8G8, registry.getResolution<2>("main_view", 1.0)})
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
  });

  auto accNode = ns.registerNode("blood_decals_accumulation", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    registry.requestRenderPass()
      .depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"})
      .color({"blood_acc_buf0", "blood_acc_normals"})
      .vrsRate(VRS_RATE_TEXTURE_NAME);

    registry.requestState().setFrameBlock("global_frame");
    registry.allowAsyncPipelines();

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_accumulation", nullptr, 0, "blood_puddles_accumulation", true);

    return [cameraHndl, shHolder = std::move(shHolder)](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr)
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
    };
  });

  return {eastl::move(perCameraResNode), eastl::move(accNode)};
}

dafg::NodeHandle make_blood_resolve_node()
{
  bool isNormalsPacked = renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS);
  auto ns = isNormalsPacked ? dafg::root() / "opaque" / "mixing" : dafg::root() / "opaque" / "dynamic_rendinst";
  return ns.registerNode("blood_puddles_resolve", DAFG_PP_NODE_SRC, [isNormalsPacked](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    auto bloodTexNs = registry.root() / "opaque" / "dynamic_rendinst";
    bloodTexNs.readTexture("blood_acc_buf0").atStage(dafg::Stage::PS).bindToShaderVar("blood_acc_buf0");
    bloodTexNs.readTexture("blood_acc_normals").atStage(dafg::Stage::PS).bindToShaderVar("blood_acc_normal");

    registry.requestState().setFrameBlock("global_frame");

    if (isNormalsPacked)
    {
      registry.modifyBlob<OrderingToken>("blood_puddles_rendered");
      registry.readBlob<OrderingToken>("dynamic_decals_rendered");
      registry.readBlob<OrderingToken>("dagdp_decals_rendered").optional();
      registry.readBlob<OrderingToken>("envi_cover_rendered").optional();
    }
    registry.allowAsyncPipelines();

    PostFxRenderer shHolder("blood_puddles_resolve");

    return [shHolder = std::move(shHolder)] { shHolder.render(); };
  });
}

dafg::NodeHandle make_blood_forward_node()
{
  bool isNormalsPacked = renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS);
  auto ns = isNormalsPacked ? dafg::root() / "opaque" / "mixing" : dafg::root() / "opaque" / "dynamic_rendinst";
  return ns.registerNode("blood_puddles_forward", DAFG_PP_NODE_SRC, [isNormalsPacked](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    if (isNormalsPacked)
    {
      registry.modifyBlob<OrderingToken>("blood_puddles_rendered");
      registry.readBlob<OrderingToken>("dynamic_decals_rendered");
      registry.readBlob<OrderingToken>("dagdp_decals_rendered").optional();
      registry.readBlob<OrderingToken>("envi_cover_rendered").optional();
    }

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_forward", nullptr, 0, "blood_puddles_forward", true);

    return [cameraHndl, shHolder = std::move(shHolder)](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr)
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
    };
  });
}
