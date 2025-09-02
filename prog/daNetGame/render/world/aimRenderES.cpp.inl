// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/aimRender.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/shaderVar.h>
#include <render/daFrameGraph/daFG.h>

template <typename Callable>
inline static void query_aim_data_ecs_query(Callable c);

template <typename Callable>
inline static void check_if_lens_renderer_enabled_globally_ecs_query(Callable c);

static bool prepare_aim_render(AimRenderingData &aimData)
{
  bool gathered = false;
  query_aim_data_ecs_query([&](ECS_REQUIRE(eastl::true_type camera__active) int aim_data__lensNodeId,
                             int aim_data__lensCollisionNodeId, float aim_data__lensBoundingSphereRadius, bool aim_data__farDofEnabled,
                             bool aim_data__lensRenderEnabled, const ecs::EntityId aim_data__entityWithScopeLensEid) {
    aimData.farDofEnabled = aim_data__farDofEnabled;
    aimData.lensRenderEnabled = aim_data__lensRenderEnabled && aim_data__lensNodeId >= 0;
    aimData.entityWithScopeLensEid = aim_data__entityWithScopeLensEid;
    aimData.lensNodeId = aim_data__lensNodeId;
    aimData.lensCollisionNodeId = aim_data__lensCollisionNodeId;
    aimData.lensBoundingSphereRadius = aim_data__lensBoundingSphereRadius;
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

dafg::NodeHandle makeGenAimRenderingDataNode()
{
  return dafg::register_node("setup_aim_rendering_data", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto aimRenderDataHandle = registry.createBlob<AimRenderingData>("aim_render_data", dafg::History::No).handle();
    registry.multiplex(dafg::multiplexing::Mode::None);
    return [aimRenderDataHandle]() {
      auto &ard = aimRenderDataHandle.ref();
      if (!prepare_aim_render(ard))
        ard = AimRenderingData();
    };
  });
}
