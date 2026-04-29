// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <animChar/dag_animCharacter2.h>
#include <render/debugMesh.h>
#include <render/viewVecs.h>
#include <frustumCulling/frustumPlanes.h>

dafg::NodeHandle create_dynamic_mirror_prepare_node(DynamicMirrorRenderer &mirror_renderer,
  uint32_t global_flags,
  uint32_t gbuf_cnt,
  eastl::span<uint32_t> main_gbuf_fmts,
  int depth_format)
{
  return get_dynamic_mirrors_namespace().registerNode("prepare", DAFG_PP_NODE_SRC,
    [&mirror_renderer, gbuf_cnt, global_flags, depth_format,
      gbufFmts = dag::RelocatableFixedVector<uint32_t, FULL_GBUFFER_RT_COUNT, false>(main_gbuf_fmts)](dafg::Registry registry) {
      const auto mirrorResolution = get_mirror_resolution(registry);

      registry.create("mirror_prepass_gbuf_depth")
        .texture({depth_format | global_flags | TEXCF_RTARGET, mirrorResolution})
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::DEPTH_ATTACHMENT)
        .clear(make_clear_value(0.0f, 0));

      for (uint32_t i = 0; i < gbuf_cnt; ++i)
      {
        registry.create(String(0, "mirror_gbuf_%d", i))
          .texture({gbufFmts[i] | global_flags | TEXCF_RTARGET, mirrorResolution})
          .useAs(dafg::Usage::COLOR_ATTACHMENT)
          .atStage(dafg::Stage::PS)
          .clear(make_clear_value(0, 0, 0, 0));
      }


      registry.requestState().setFrameBlock("global_frame");

      registry.requestRenderPass().depthRw("mirror_prepass_gbuf_depth");

      auto mirrorActiveHndl = registry.createBlob<bool>("is_mirror_active").handle();
      auto mirrorCameraHndl = registry.createBlob<DynamicMirrorRenderer::CameraData>("mirror_camera").handle();
      auto mirrorVarsHndl = registry.createBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

      return [&mirror_renderer, mirrorVarsHndl, mirrorResolution, mirrorCameraHndl, mirrorActiveHndl]() {
        const auto resolution = mirrorResolution.get();

        // These must be set before early exit, before otherwise zero division can happen when the shader vars are evaluated
        mirrorVarsHndl.ref().gbufferViewSize = IPoint4(resolution.x, resolution.y, 0, 0);
        mirrorVarsHndl.ref().screenPosToTexcoord = Color4(1.f / resolution.x, 1.f / resolution.y, 0, 0);

        mirror_renderer.setLatestResolution(resolution);
        if (!mirror_renderer.hasMirrorSet())
          return;

        auto cameraData = mirror_renderer.getCameraData();
        auto animcharMirrorData = mirror_renderer.getAnimcharMirrorData();
        if (cameraData == nullptr || animcharMirrorData == nullptr)
          return;
        mirrorActiveHndl.ref() = true;
        mirrorCameraHndl.ref() = *cameraData;
        mirrorVarsHndl.ref().frustum = cameraData->frustum;
        mirrorVarsHndl.ref().viewVecs = calc_view_vecs(cameraData->viewTm, cameraData->projTm);

#if DAGOR_DBGLEVEL > 0
        debug_mesh::activate_mesh_coloring_master_override();
#endif
      };
    });
}