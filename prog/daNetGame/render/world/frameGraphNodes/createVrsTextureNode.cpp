// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/motionVectorAccess.h>
#include <shaders/dag_computeShaders.h>
#include <ioSys/dag_dataBlock.h>

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
  auto &drvDesc = d3d::get_driver_desc();

  static ShaderVariableInfo has_variable_rate_shading_by_4("has_variable_rate_shading_by_4");
  has_variable_rate_shading_by_4.set_int(drvDesc.caps.hasVariableRateShadingBy4);

  static ShaderVariableInfo variable_rate_shading_tile_size("variable_rate_shading_tile_size");
  variable_rate_shading_tile_size.set_int(drvDesc.variableRateTextureTileSizeX);
}

static ShaderVariableInfo motion_vrs_strength("motion_vrs_strength", true);

eastl::fixed_vector<dafg::NodeHandle, 4> makeCreateVrsTextureNode(bool force_dummy_nodes)
{
  const bool vrsSupported = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
  if (!vrsSupported)
    return {};

  eastl::fixed_vector<dafg::NodeHandle, 4> result;

  if (!can_use_motion_vrs() || force_dummy_nodes)
  {
    result.push_back(dafg::register_node("create_vrs_texture_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create(VRS_RATE_TEXTURE_NAME)
        .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dafg::Stage::COMPUTE)
        .useAs(dafg::Usage::SHADER_RESOURCE)
        .clear(make_clear_value(0, 0, 0, 0));
    }));
    result.push_back(dafg::register_node("create_shading_vrs_texture", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create(SHADING_VRS_RATE_TEXTURE_NAME)
        .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dafg::Stage::COMPUTE)
        .useAs(dafg::Usage::SHADER_RESOURCE)
        .clear(make_clear_value(0, 0, 0, 0));
    }));

    return result;
  }

  result.push_back(dafg::register_node("gen_downsampled_luminance", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);
    registry.create("downsampled_luminance")
      .texture({TEXFMT_R8G8B8A8 | TEXCF_UNORDERED, halfMainViewResolution})
      .withHistory(dafg::History::ClearZeroOnFirstFrame)
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
      return registry.create(name)
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
    makeStatisticsTex("tile_luma_min", TEXFMT_R32UI, 0xFFFFFFFF);
    makeStatisticsTex("tile_luma_max", TEXFMT_R32UI, 0);

    registry.historyFor("close_depth").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("prev_close_depth");
    registry.historyFor("downsampled_luminance").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("prev_downsampled_luminance");
    registry.historyFor("downsampled_motion_vectors_tex")
      .texture()
      .atStage(dafg::Stage::COMPUTE)
      .optional()
      .bindToShaderVar("prev_downsampled_motion_vecs");

    setup_driver_cap_shadervars();

    const auto halfMainViewResolution = registry.getResolution<2>("main_view", 0.5f);
    const auto mainViewResolution = registry.getResolution<2>("main_view");
    const auto postFxResolution = registry.getResolution<2>("post_fx");

    auto camHndl = registry.read("current_camera").blob<CameraParams>().handle();
    auto prevCamHndl = registry.historyFor("current_camera").blob<CameraParams>().handle();

    return [halfMainViewResolution, mainViewResolution, postFxResolution, camHndl, prevCamHndl,
             shader = ComputeShader("gen_motion_statistics")]() {
      if (motion_vrs_strength.get_float() == 0)
        return;

      const float frameUpscaleFactor = float(mainViewResolution.get().x) / float(postFxResolution.get().x);

      static ShaderVariableInfo frame_upscale_factor("frame_upscale_factor", true);
      frame_upscale_factor.set_float(frameUpscaleFactor);

      static ShaderVariableInfo zn_zfar("zn_zfar");
      zn_zfar.set_float4(camHndl.ref().znear, camHndl.ref().zfar, 0, 0);

      motion_vector_access::set_reprojection_params_prev_to_curr(dng_to_mva(camHndl.ref()), dng_to_mva(prevCamHndl.ref()));

      auto [w, h] = halfMainViewResolution.get();
      shader.dispatchThreads(w, h, 1);
    };
  }));

  result.push_back(dafg::register_node("create_vrs_texture_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto tilesResolution = registry.getResolution<2>("texel_per_vrs_tile");
    registry.create(VRS_RATE_TEXTURE_NAME)
      .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, tilesResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("vrs_rate_tex");

    registry.read("tile_motion_x_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_motion_y_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_squares_sum").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_motion_sample_count").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_min").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("tile_luma_max").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();

    // Driver caps don't change at runtime, but can change after a device reset
    // & migration to igpu, so leave them inside the declaration callback.
    setup_driver_cap_shadervars();

    auto tileRes = registry.getResolution<2>("texel_per_vrs_tile");

    return [tileRes, shader = ComputeShader("gen_motion_vrs")]() {
      // Leave a way to turn it off via a config update in case issues come up.
      if (motion_vrs_strength.get_float() == 0)
        return;

      auto [w, h] = tileRes.get();
      shader.dispatchThreads(w, h, 1);
    };
  }));

  result.push_back(dafg::register_node("create_shading_vrs_texture", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto tilesResolution = registry.getResolution<2>("texel_per_vrs_tile");
    registry.create(SHADING_VRS_RATE_TEXTURE_NAME)
      .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, tilesResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("vrs_rate_tex");

    registry.read("close_depth").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar();
    registry.read("downsampled_motion_vectors_tex").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar().optional();

    // Driver caps don't change at runtime, but can change after a device reset
    // & migration to igpu, so leave them inside the declaration callback.
    setup_driver_cap_shadervars();

    registry.dispatchThreads("gen_shading_motion_vrs")
      .x<&IPoint2::x>(registry.getResolution<2>("texel_per_vrs_tile"))
      .y<&IPoint2::y>(registry.getResolution<2>("texel_per_vrs_tile"))
      .z(1);
  }));

  return result;
}
