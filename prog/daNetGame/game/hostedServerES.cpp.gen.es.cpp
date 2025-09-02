// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "hostedServerES.cpp.inl"
ECS_DEF_PULL_VAR(hostedServer);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc hosted_server_tracking_loaded_local_scene_entities_es_comps[] ={};
static void hosted_server_tracking_loaded_local_scene_entities_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  hosted_server_tracking_loaded_local_scene_entities_es(evt
        );
}
static ecs::EntitySystemDesc hosted_server_tracking_loaded_local_scene_entities_es_es_desc
(
  "hosted_server_tracking_loaded_local_scene_entities_es",
  "prog/daNetGame/game/hostedServerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hosted_server_tracking_loaded_local_scene_entities_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventOnLocalSceneEntitiesCreated>::build(),
  0
);
