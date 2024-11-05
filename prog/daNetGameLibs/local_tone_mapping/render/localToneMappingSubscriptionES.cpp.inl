// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <render/renderEvent.h>
#include <render/daBfg/ecs/frameGraphNode.h>

ECS_TAG(render)
ECS_REQUIRE(const dabfg::NodeHandle &local_tone_mapping__filter_node)
static void register_local_tone_mapping_for_postfx_es(const RegisterPostfxResources &evt)
{
  (evt.get<0>().root() / "local_tone_mapping")
    .read("exposure_multiplier_filtered_tex")
    .texture()
    .atStage(dabfg::Stage::POST_RASTER)
    .bindToShaderVar("exposure_tex");
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_local_tone_mapping_samplers_node_es(const ecs::Event &, dabfg::NodeHandle &local_tone_mapping__samplers_node)
{
  local_tone_mapping__samplers_node =
    (dabfg::root() / "local_tone_mapping").registerNode("samplers", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("clamp_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    });
}
