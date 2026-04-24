// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"

#include <ecs/render/updateStageRender.h>
#include <landMesh/lmeshManager.h>
#include <render/world/global_vars.h>
#include <render/world/wrDispatcher.h>
#include <render/daFrameGraph/daFG.h>


dafg::NodeHandle create_dynamic_mirror_render_ground_node()
{
  return get_dynamic_mirrors_namespace().registerNode("render_ground", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);

    registry.requestState().setFrameBlock("global_frame");

    auto mirrorCameraHndl = use_mirror_gbuffer_pass(registry);
    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [mirrorActiveHndl, mirrorCameraHndl, shaderVarsHndl]() {
      if (!mirrorActiveHndl.ref())
        return;
      auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
      auto *lmeshManager = WRDispatcher::getLandMeshManager();
      if (lmeshRenderer == nullptr || lmeshManager == nullptr)
        return;
      const auto cameraData = mirrorCameraHndl.get();
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();

      Color4 originalWorldPos = ShaderGlobal::get_float4(world_view_posVarId);
      Color4 originalZnZfar = ShaderGlobal::get_float4(zn_zfarVarId);

      ShaderGlobal::set_float4(world_view_posVarId, cameraData->viewPos.x, cameraData->viewPos.y, cameraData->viewPos.z, 1);
      ShaderGlobal::set_float4(zn_zfarVarId, cameraData->prepassPersp.zn, cameraData->prepassPersp.zf, 0, 0);

      lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH);
      lmeshRenderer->setUseHmapTankSubDiv(0);
      lmeshRenderer->forceLowQuality(true);
      lmeshRenderer->render(reinterpret_cast<const mat44f &>(cameraData->globTm), cameraData->projTm, cameraData->frustum,
        *lmeshManager, LandMeshRenderer::RENDER_WITH_CLIPMAP, cameraData->viewPos);
      lmeshRenderer->forceLowQuality(false);

      ShaderGlobal::set_float4(world_view_posVarId, originalWorldPos);
      ShaderGlobal::set_float4(zn_zfarVarId, originalZnZfar);
    };
  });
}