// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <ecs/render/updateStageRender.h>
#include <animChar/dag_animCharacter2.h>
#include <render/world/global_vars.h>
#include <render/world/cameraParams.h>

dafg::NodeHandle create_dynamic_mirror_prepass_node(DynamicMirrorRenderer &mirror_renderer)
{
  return get_dynamic_mirrors_namespace().registerNode("prepass", DAFG_PP_NODE_SRC, [&mirror_renderer](dafg::Registry registry) {
    auto mirrorCameraHndl = use_mirror_gbuffer_prepass(registry);

    shaders::OverrideState cockpitMirrorMaskState;
    cockpitMirrorMaskState.set(shaders::OverrideState::Z_FUNC);
    cockpitMirrorMaskState.zFunc = CMPF_ALWAYS;
    cockpitMirrorMaskState.set(shaders::OverrideState::Z_CLAMP_ENABLED);
    cockpitMirrorMaskState.set(shaders::OverrideState::CULL_NONE);

    registry.requestState().setFrameBlock("global_frame").enableOverride(cockpitMirrorMaskState);

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [&mirror_renderer, mirrorActiveHndl, mirrorCameraHndl, cameraHndl, shaderVarsHndl]() {
      if (!mirrorActiveHndl.ref())
        return;
      auto animcharMirrorData = mirror_renderer.getAnimcharMirrorData();
      if (animcharMirrorData == nullptr)
        return;
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();

      const auto cameraData = mirrorCameraHndl.get();

      Color4 originalWorldPos = ShaderGlobal::get_float4(world_view_posVarId);
      Color4 originalZnZfar = ShaderGlobal::get_float4(zn_zfarVarId);

      ShaderGlobal::set_float4(world_view_posVarId, cameraData->viewPos.x, cameraData->viewPos.y, cameraData->viewPos.z, 1);
      ShaderGlobal::set_float4(zn_zfarVarId, cameraData->prepassPersp.zn, cameraData->prepassPersp.zf, 0, 0);

      render_dynamic_mirrors(cameraHndl.ref().cameraWorldPos, *animcharMirrorData, RENDER_UNKNOWN, true,
        cameraData->mirrorRenderViewItm, TexStreamingContext(0));

      ShaderGlobal::set_float4(world_view_posVarId, originalWorldPos);
      ShaderGlobal::set_float4(zn_zfarVarId, originalZnZfar);
    };
  });
}