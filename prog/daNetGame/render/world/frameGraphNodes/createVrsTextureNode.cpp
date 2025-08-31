// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/motionVectorAccess.h>
#include <shaders/dag_computeShaders.h>

static motion_vector_access::CameraParams dng_to_mva(const CameraParams &dngCam)
{
  return motion_vector_access::CameraParams{
    .view = dngCam.viewTm,
    .viewInverse = dngCam.viewItm,
    .projectionNoJitter = dngCam.noJitterProjTm,
    .position = dngCam.cameraWorldPos,
    .znear = dngCam.znear,
    .zfar = dngCam.zfar,
  };
}

static void setup_driver_cap_shadervars()
{
  const Driver3dDesc &drv = d3d::get_driver_desc();

  static ShaderVariableInfo has_variable_rate_shading_by_4("has_variable_rate_shading_by_4");
  has_variable_rate_shading_by_4.set_int(drv.caps.hasVariableRateShadingBy4);

  static ShaderVariableInfo variable_rate_shading_tile_size("variable_rate_shading_tile_size");
  variable_rate_shading_tile_size.set_int(drv.variableRateTextureTileSizeX);
}

static ShaderVariableInfo motion_vrs_strength("motion_vrs_strength", true);

eastl::fixed_vector<dafg::NodeHandle, 3> makeCreateVrsTextureNode()
{
  const bool vrsSupported = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
  if (!vrsSupported)
    return {};

  eastl::fixed_vector<dafg::NodeHandle, 3> result;

  if (!can_use_motion_vrs())
  {
    result.push_back(dafg::register_node("create_vrs_texture_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create(VRS_RATE_TEXTURE_NAME, dafg::History::No)
        .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dafg::Stage::COMPUTE)
        .useAs(dafg::Usage::SHADER_RESOURCE);
    }));

    return result;
  }

  result.push_back(dafg::register_node("gen_downsampled_luminance", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);
    registry.create("downsampled_luminance", dafg::History::ClearZeroOnFirstFrame)
      .texture({TEXFMT_R8 | TEXCF_UNORDERED, halfMainViewResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("downsampled_luminance");

    registry.read("frame_after_postfx").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("final_frame");

    return [halfMainViewResolution, shader = ComputeShader("gen_downsampled_luminance")]() {
      auto [w, h] = halfMainViewResolution.get();
      shader.dispatchThreads(w, h, 1);
    };
  }));

  result.push_back(dafg::register_node("gen_tile_motion_statistics", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto makeStatisticsTex = [&](const char *name, uint32_t fmt, uint32_t init) {
      return registry.create(name, dafg::History::No)
        .texture(dafg::Texture2dCreateInfo{fmt | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dafg::Stage::COMPUTE)
        .clear(make_clear_value(init, 0u, 0u, 0u))
        .bindToShaderVar()
        .handle();
    };

    // all of these are AT LEAST in 1/8 resolution so it's ok to have so many of them
    makeStatisticsTex("tile_motion_x_sum", TEXFMT_R32SI, 0);
    makeStatisticsTex("tile_motion_y_sum", TEXFMT_R32SI, 0);
    makeStatisticsTex("tile_luma_sum", TEXFMT_R32UI, 0);
    makeStatisticsTex("tile_luma_squares_sum", TEXFMT_R32UI, 0);
    makeStatisticsTex("tile_motion_sample_count", TEXFMT_R32UI, 0);

    registry.historyFor("close_depth").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("prev_close_depth");
    registry.historyFor("downsampled_luminance").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("prev_downsampled_luminance");

    setup_driver_cap_shadervars();

    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);

    auto camHndl = registry.read("current_camera").blob<CameraParams>().handle();
    auto prevCamHndl = registry.historyFor("current_camera").blob<CameraParams>().handle();

    return [halfMainViewResolution, camHndl, prevCamHndl, shader = ComputeShader("gen_motion_statistics")]() {
      if (motion_vrs_strength.get_real() == 0)
        return;

      static ShaderVariableInfo zn_zfar("zn_zfar");
      zn_zfar.set_color4(camHndl.ref().znear, camHndl.ref().zfar, 0, 0);

      motion_vector_access::set_reprojection_params_prev_to_curr(dng_to_mva(camHndl.ref()), dng_to_mva(prevCamHndl.ref()));

      auto [w, h] = halfMainViewResolution.get();
      shader.dispatchThreads(w, h, 1);
    };
  }));

  result.push_back(dafg::register_node("create_vrs_texture_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto tilesResolution = registry.getResolution<2>("texel_per_vrs_tile");
    registry.create(VRS_RATE_TEXTURE_NAME, dafg::History::No)
      .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, tilesResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("vrs_rate_tex");

    registry.read("tile_motion_x_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_motion_y_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_squares_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_motion_sample_count").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();

    // Driver caps don't change at runtime, but can change after a device reset
    // & migration to igpu, so leave them inside the declaration callback.
    setup_driver_cap_shadervars();

    auto tileRes = registry.getResolution<2>("texel_per_vrs_tile");

    return [tileRes, shader = ComputeShader("gen_motion_vrs")]() {
      // Leave a way to turn it off via a config update in case issues come up.
      if (motion_vrs_strength.get_real() == 0)
        return;

      auto [w, h] = tileRes.get();
      shader.dispatchThreads(w, h, 1);
    };
  }));

  return result;
}
