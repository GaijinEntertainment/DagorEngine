// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/debugGbuffer.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>


void set_up_show_gbuffer_entity()
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("show_gbuffer")));
  if (!shouldRenderGbufferDebug())
    return;
  ecs::ComponentsInitializer init;
  init[ECS_HASH("showGbufferNode")] = dafg::register_node("show_gbuffer_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("post_fx_node");
    read_gbuffer(registry);
    registry.readTexture("motion_vecs_after_transparency").atStage(dafg::Stage::PS).bindToShaderVar("motion_gbuf").optional();
    registry.readTexture("upscale_sampling_tex").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("upscale_sampling_tex").optional();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.readTexture("ssao_tex").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("ssao_tex").optional();
    registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
    registry.readTexture("ssr_target").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("ssr_target").optional();
    registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_target_samplerstate").optional();
    registry.requestRenderPass().color({"frame_with_debug"});
    auto gdepth = registry.readTexture("depth_for_postfx").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("depth_gbuf").handle();
    return [debugRenderer = PostFxRenderer(DEBUG_RENDER_GBUFFER_SHADER_NAME),
             debugVecShader = DynamicShaderHelper(DEBUG_RENDER_GBUFFER_WITH_VECTORS_SHADER_NAME), gdepth]() {
      debug_render_gbuffer(debugRenderer, gdepth.view().getTex2D());
      debug_render_gbuffer_with_vectors(debugVecShader, gdepth.view().getTex2D());
    };
  });
  g_entity_mgr->createEntityAsync("show_gbuffer", eastl::move(init));
}
