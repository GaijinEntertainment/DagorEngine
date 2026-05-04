// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "burntGrassNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>

#include "burntGrassRenderer.h"

dafg::NodeHandle create_burnt_grass_prepare_node(BurntGrassRenderer *renderer)
{
  return get_burnt_grass_namespace().registerNode("prepare", DAFG_PP_NODE_SRC, [renderer](dafg::Registry registry) {
    registry.createBlob<OrderingToken>("burnt_grass_prepared_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [renderer, cameraHndl, burnt_grass_enabledVarId = ::get_shader_variable_id("burnt_grass_enabled", true)]() {
      bool hasBurntGrass = false;
      if (renderer->hasAnyFire())
      {
        auto eyePos = cameraHndl.ref().cameraWorldPos;
        hasBurntGrass = renderer->prepare(Point2(eyePos.x, eyePos.z));
      }

      ShaderGlobal::set_int(burnt_grass_enabledVarId, hasBurntGrass ? 1 : 0);
    };
  });
}