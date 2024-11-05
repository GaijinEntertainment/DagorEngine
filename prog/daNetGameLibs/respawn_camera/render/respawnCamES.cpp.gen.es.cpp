#include "respawnCamES.cpp.inl"
ECS_DEF_PULL_VAR(respawnCam);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc respawn_cam_created_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("respawnCameraTargerPoint"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void respawn_cam_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    respawn_cam_created_es(evt
        , ECS_RO_COMP(respawn_cam_created_es_comps, "respawnCameraTargerPoint", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc respawn_cam_created_es_es_desc
(
  "respawn_cam_created_es",
  "prog/daNetGameLibs/respawn_camera/render/respawnCamES.cpp.inl",
  ecs::EntitySystemOps(nullptr, respawn_cam_created_es_all_events),
  empty_span(),
  make_span(respawn_cam_created_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc respawn_cam_destroyed_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("respawnCameraTargerPoint"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void respawn_cam_destroyed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  respawn_cam_destroyed_es(evt
        );
}
static ecs::EntitySystemDesc respawn_cam_destroyed_es_es_desc
(
  "respawn_cam_destroyed_es",
  "prog/daNetGameLibs/respawn_camera/render/respawnCamES.cpp.inl",
  ecs::EntitySystemOps(nullptr, respawn_cam_destroyed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(respawn_cam_destroyed_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc respawn_camera_target_point_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("respawnCameraLODMultiplier"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc respawn_camera_target_point_ecs_query_desc
(
  "respawn_camera_target_point_ecs_query",
  empty_span(),
  make_span(respawn_camera_target_point_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void respawn_camera_target_point_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, respawn_camera_target_point_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(respawn_camera_target_point_ecs_query_comps, "respawnCameraLODMultiplier", float)
            );

        }
    }
  );
}
