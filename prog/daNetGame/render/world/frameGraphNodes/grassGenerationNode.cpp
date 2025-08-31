// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/cameraInCamera.h>
#include <render/world/frameGraphHelpers.h>
#include <render/grass/grassRender.h>

#include "frameGraphNodes.h"

namespace var
{
ShaderVariableInfo reprojected_hzb("reprojected_hzb");
ShaderVariableInfo reprojected_hzb_depth_mip_count("reprojected_hzb_depth_mip_count");
ShaderVariableInfo grass_use_hzb_occlusion("grass_use_hzb_occlusion");
} // namespace var

dafg::NodeHandle makeGrassGenerationNode()
{
  const bool needSeparateGrassGenNode = renderer_has_feature(CAMERA_IN_CAMERA);
  if (!needSeparateGrassGenNode)
    return {};

  return dafg::register_node("grass_generation_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();

    auto hzbHndl = registry.readTexture("reprojected_hzb").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    registry.readBlob<int>("reprojected_hzb_mip_count").bindToShaderVar("reprojected_hzb_depth_mip_count");

    registry.createBlob<OrderingToken>("grass_generated_token", dafg::History::No);

    return [cameraHndl, hzbHndl](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

      const bool useOcclusion = camera_in_camera::is_lens_render_active();
      ShaderGlobal::set_int(var::grass_use_hzb_occlusion, useOcclusion ? 1 : 0);
      ShaderGlobal::set_texture(var::reprojected_hzb.get_var_id(), useOcclusion ? hzbHndl.view().getTexId() : BAD_TEXTUREID);

      const CameraParams &camera = cameraHndl.ref();
      const bool isMainView = multiplexing_index.subCamera == 0;

      if (isMainView)
        grass_prepare_per_camera(camera);

      grass_prepare_per_view(camera, isMainView);
    };
  });
}