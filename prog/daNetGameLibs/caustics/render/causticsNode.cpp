// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "caustics.h"

#include <render/viewVecs.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/world/bvh.h>

#include <EASTL/array.h>

#define CAUSTICS_VARS    \
  VAR(caustics_options)  \
  VAR(caustics_hero_pos) \
  VAR(caustics_world_scale)

#define VAR(a) static int a##VarId = -1;
CAUSTICS_VARS
#undef VAR

eastl::array<dafg::NodeHandle, 2> makeCausticsNode()
{
  auto perCameraResNode = dafg::register_node("caustics_per_camera_res_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    float multiplier = is_rr_enabled() ? 1.0f : 0.5f;
    registry.createTexture2d("caustics_tex", {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view", multiplier)});
  });

  auto renderNode = dafg::register_node("caustics_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
#define VAR(a) a##VarId = get_shader_variable_id(#a);
    CAUSTICS_VARS
#undef VAR

    if (is_rr_enabled())
    {
      registry.read("gbuf_1").texture().atStage(dafg::Stage::PS).bindToShaderVar("normals_for_caustics");
      registry.read("gbuf_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("normals_for_caustics_samplerstate")
        .bindToShaderVar("depth_for_caustics_samplerstate");
      registry.allowAsyncPipelines()
        .requestRenderPass()
        .depthRoAndBindToShaderVars("gbuf_depth", {"depth_for_caustics"})
        .color({"caustics_tex"});
    }
    else
    {
      registry.readTexture("close_depth").atStage(dafg::Stage::PS).bindToShaderVar("depth_for_caustics");
      registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_for_caustics_samplerstate");
      registry.readTexture("downsampled_normals").atStage(dafg::Stage::PS).bindToShaderVar("normals_for_caustics");
      registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normals_for_caustics_samplerstate");
      registry.allowAsyncPipelines().requestRenderPass().depthRw("downsampled_depth").color({"caustics_tex"});
    }

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    return [cameraHndl, causticsPostfx = PostFxRenderer("caustics_render")](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), camera_in_camera::USE_STENCIL};
      // TODO these should be setup maybe?
      Point2 causticsHeight = Point2(5.0f, 5.0f);
      Point4 causticsHeroPos = Point4(0, -100000, 0, 0);

      CausticsSetting settings;
      queryCausticsSettings(settings);

      ShaderGlobal::set_float4(caustics_optionsVarId, 0.2f, get_shader_global_time() * settings.causticsScrollSpeed, causticsHeight.x,
        causticsHeight.y);
      ShaderGlobal::set_float4(caustics_hero_posVarId, causticsHeroPos);
      ShaderGlobal::set_float(caustics_world_scaleVarId, settings.causticsWorldScale);

      causticsPostfx.render();
    };
  });

  return {eastl::move(perCameraResNode), eastl::move(renderNode)};
}