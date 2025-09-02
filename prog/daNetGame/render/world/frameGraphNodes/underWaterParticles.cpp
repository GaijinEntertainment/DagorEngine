// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/fx.h>
#include <render/rendererFeatures.h>
#include <render/daFrameGraph/daFG.h>

#include "frameGraphNodes.h"

dafg::NodeHandle makeUnderWaterParticlesNode()
{
  return dafg::register_node("under_water_particles_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("under_water_fog_node");
    registry.requestState().setFrameBlock("global_frame");

    const bool isForward = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING);
    const char *depthTex = isForward ? "depth_for_transparent_effects" : "depth_for_transparency";
    // Dirty, dirty hack to get the resolution of this resource.
    auto depthHndl = registry.readTexture(depthTex).atStage(dafg::Stage::UNKNOWN).useAs(dafg::Usage::UNKNOWN).handle();
    registry.requestRenderPass().color({"target_for_transparency"}).depthRoAndBindToShaderVars(depthTex, {"effects_depth_tex"});
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("effects_depth_tex_samplerstate");

    registry.readTexture("fom_shadows_sin").atStage(dafg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_sin").optional();
    registry.readTexture("fom_shadows_cos").atStage(dafg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    int dafx_use_underwater_fogVarId = ::get_shader_variable_id("dafx_use_underwater_fog");

    auto underwaterHndl = registry.readBlob<bool>("is_underwater").handle();
    return [underwaterHndl, depthHndl, dafx_use_underwater_fogVarId] {
      if (!underwaterHndl.ref())
        return;

      ::ShaderGlobal::set_int(dafx_use_underwater_fogVarId, 1);
      acesfx::setDepthTex(depthHndl.view().getTex2D());
      acesfx::renderUnderwater();
      ::ShaderGlobal::set_int(dafx_use_underwater_fogVarId, 0);
    };
  });
}
