// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <ecs/render/updateStageRender.h>
#include <animChar/dag_animCharacter2.h>
#include <render/world/global_vars.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/cameraParams.h>

dafg::NodeHandle create_dynamic_mirror_render_dynamic_node()
{
  return get_dynamic_mirrors_namespace().registerNode("render_dynamic", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.setPriority(dafg::PRIO_AS_EARLY_AS_POSSIBLE);

    const auto mirrorResolution = get_mirror_resolution(registry);

    auto mirrorCameraHndl = use_mirror_gbuffer_pass(registry);

    registry.requestState().setFrameBlock("global_frame").setSceneBlock("dynamic_scene");

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [mirrorResolution, mirrorActiveHndl, mirrorCameraHndl, cameraHndl, shaderVarsHndl]() {
      if (!mirrorActiveHndl.ref())
        return;
      const auto cameraData = mirrorCameraHndl.get();
      const auto resolution = mirrorResolution.get();
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();

      Color4 originalWorldPos = ShaderGlobal::get_float4(world_view_posVarId);
      Color4 originalZnZfar = ShaderGlobal::get_float4(zn_zfarVarId);


      TexStreamingContext texStreamingCtx = TexStreamingContext(cameraData->persp, resolution.x);
      ShaderGlobal::set_float4(world_view_posVarId, cameraData->viewPos.x, cameraData->viewPos.y, cameraData->viewPos.z, 1);
      ShaderGlobal::set_float4(zn_zfarVarId, cameraData->persp.zn, cameraData->persp.zf, 0, 0);

      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      uint32_t hints = UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH;

      auto &state = dynmodel_renderer::get_immediate_state();
      {
        TIME_D3D_PROFILE(ecs_render);
        g_entity_mgr->broadcastEventImmediate(UpdateStageInfoRender(hints, cameraData->frustum, cameraData->viewItm,
          cameraData->viewTm, cameraData->projTm, cameraHndl.ref().cameraWorldPos, cameraHndl.ref().negRoundedCamPos,
          cameraHndl.ref().negRemainderCamPos, nullptr, RENDER_UNKNOWN, &state, texStreamingCtx));
      }

      ShaderGlobal::set_float4(world_view_posVarId, originalWorldPos);
      ShaderGlobal::set_float4(zn_zfarVarId, originalZnZfar);
    };
  });
}