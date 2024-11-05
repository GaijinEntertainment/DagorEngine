// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/aimRender.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/shaderVar.h>
#include <render/daBfg/bfg.h>

template <typename Callable>
inline static void query_aim_data_ecs_query(Callable c);

template <typename Callable>
inline static void check_if_lens_renderer_enabled_globally_ecs_query(Callable c);

static bool prepare_aim_render(AimRenderingData &aimData)
{
  bool gathered = false;
  query_aim_data_ecs_query([&](ECS_REQUIRE(eastl::true_type camera__active) int aim_data__lensNodeId, bool aim_data__farDofEnabled,
                             bool aim_data__lensRenderEnabled) {
    aimData.farDofEnabled = aim_data__farDofEnabled;
    aimData.lensRenderEnabled = aim_data__lensRenderEnabled && aim_data__lensNodeId >= 0;
    gathered = true;
  });
  return gathered;
}

AimRenderingData get_aim_rendering_data()
{
  AimRenderingData r;
  prepare_aim_render(r);
  return r;
}

bool lens_renderer_enabled_globally()
{
  bool enabled = false;
  check_if_lens_renderer_enabled_globally_ecs_query(
    [&enabled](bool lens_renderer_enabled_globally) { enabled = enabled || lens_renderer_enabled_globally; });
  return enabled;
}

dabfg::NodeHandle makeGenAimRenderingDataNode()
{
  return dabfg::register_node("setup_aim_rendering_data", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto aimRenderDataHandle = registry.createBlob<AimRenderingData>("aim_render_data", dabfg::History::No).handle();
    registry.multiplex(dabfg::multiplexing::Mode::None);
    return [aimRenderDataHandle]() {
      auto &ard = aimRenderDataHandle.ref();
      if (!prepare_aim_render(ard))
        ard = AimRenderingData();
    };
  });
}
