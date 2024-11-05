// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "caustics.h"

#include <render/viewVecs.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/world/cameraParams.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_renderTarget.h>

CONSOLE_FLOAT_VAL("render", causticsScrollSpeed, 0.75);
CONSOLE_FLOAT_VAL("render", causticsWorldScale, 0.25);

#define CAUSTICS_VARS        \
  VAR(caustics_texture_size) \
  VAR(caustics_options)      \
  VAR(caustics_hero_pos)     \
  VAR(caustics_world_scale)

#define VAR(a) static int a##VarId = -1;
CAUSTICS_VARS
#undef VAR

dabfg::NodeHandle makeCausticsNode()
{
  return dabfg::register_node("caustics_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
#define VAR(a) a##VarId = get_shader_variable_id(#a);
    CAUSTICS_VARS
#undef VAR

    auto causticsTexHndl =
      registry
        .createTexture2d("caustics_tex", dabfg::History::No, {TEXFMT_L8 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5)})
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .atStage(dabfg::Stage::POST_RASTER)
        .handle();
    registry.create("caustics_tex_samplerstate", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler({}));
    registry.readTexture("close_depth").atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");
    registry.readTexture("downsampled_normals").atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_normals");
    registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [causticsTexHndl, cameraHndl, causticsPostfx = PostFxRenderer("caustics_render")]() {
      // TODO these should be setup maybe?
      Point2 causticsHeight = Point2(5.0f, 5.0f);
      Point4 causticsHeroPos = Point4(0, -100000, 0, 0);

      ShaderGlobal::set_color4(caustics_optionsVarId, 0.2f, get_shader_global_time() * causticsScrollSpeed, causticsHeight.x,
        causticsHeight.y);
      ShaderGlobal::set_color4(caustics_hero_posVarId, causticsHeroPos);
      ShaderGlobal::set_real(caustics_world_scaleVarId, causticsWorldScale);
      TextureInfo info;
      causticsTexHndl.get()->getinfo(info);
      ShaderGlobal::set_color4(caustics_texture_sizeVarId, info.w, info.h);
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);

      d3d::set_render_target({nullptr, 0}, DepthAccess::RW, {{causticsTexHndl.get(), 0}});
      causticsPostfx.render();
    };
  });
}