// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "caustics.h"

#include <render/viewVecs.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>

#include <EASTL/array.h>

#define CAUSTICS_VARS        \
  VAR(caustics_texture_size) \
  VAR(caustics_options)      \
  VAR(caustics_hero_pos)     \
  VAR(caustics_world_scale)

#define VAR(a) static int a##VarId = -1;
CAUSTICS_VARS
#undef VAR

eastl::array<dafg::NodeHandle, 2> makeCausticsNode()
{
  auto perCameraResNode = dafg::register_node("caustics_per_camera_res_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createTexture2d("caustics_tex", dafg::History::No,
      {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5)});
  });

  auto renderNode = dafg::register_node("caustics_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
#define VAR(a) a##VarId = get_shader_variable_id(#a);
    CAUSTICS_VARS
#undef VAR

    auto depthHndl =
      registry.readTexture("downsampled_depth").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    auto causticsTexHndl =
      registry.modifyTexture("caustics_tex").useAs(dafg::Usage::COLOR_ATTACHMENT).atStage(dafg::Stage::POST_RASTER).handle();

    registry.create("caustics_tex_samplerstate", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler({}));
    registry.readTexture("close_depth").atStage(dafg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");
    registry.readTexture("downsampled_normals").atStage(dafg::Stage::PS).bindToShaderVar("downsampled_normals");
    registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();
    return [depthHndl, causticsTexHndl, cameraHndl, causticsPostfx = PostFxRenderer("caustics_render")](
             const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), camera_in_camera::USE_STENCIL};
      // TODO these should be setup maybe?
      Point2 causticsHeight = Point2(5.0f, 5.0f);
      Point4 causticsHeroPos = Point4(0, -100000, 0, 0);

      CausticsSetting settings;
      queryCausticsSettings(settings);

      ShaderGlobal::set_color4(caustics_optionsVarId, 0.2f, get_shader_global_time() * settings.causticsScrollSpeed, causticsHeight.x,
        causticsHeight.y);
      ShaderGlobal::set_color4(caustics_hero_posVarId, causticsHeroPos);
      ShaderGlobal::set_real(caustics_world_scaleVarId, settings.causticsWorldScale);
      TextureInfo info;
      causticsTexHndl.get()->getinfo(info);
      ShaderGlobal::set_color4(caustics_texture_sizeVarId, info.w, info.h);

      if (camera_in_camera::is_main_view(multiplexing_index))
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);

      Texture *stencil = camera_in_camera::is_lens_render_active() ? depthHndl.view().getTex2D() : nullptr;
      d3d::set_render_target({stencil, 0}, DepthAccess::RW, {{causticsTexHndl.get(), 0}});
      causticsPostfx.render();
    };
  });

  return {eastl::move(perCameraResNode), eastl::move(renderNode)};
}