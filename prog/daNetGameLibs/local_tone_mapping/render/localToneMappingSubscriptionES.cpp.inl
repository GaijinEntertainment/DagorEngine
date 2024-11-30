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
