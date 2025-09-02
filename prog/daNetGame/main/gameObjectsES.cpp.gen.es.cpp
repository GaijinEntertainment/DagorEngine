#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t hierarchy_transform_get_type();
static ecs::LTComponentList hierarchy_transform_component(ECS_HASH("hierarchy_transform"), hierarchy_transform_get_type(), "prog/daNetGame/main/gameObjectsES.cpp.inl", "game_objects_events_es_event_handler", 0);
static constexpr ecs::component_t hierarchy_unresolved_id_get_type();
static ecs::LTComponentList hierarchy_unresolved_id_component(ECS_HASH("hierarchy_unresolved_id"), hierarchy_unresolved_id_get_type(), "prog/daNetGame/main/gameObjectsES.cpp.inl", "game_objects_events_es_event_handler", 0);
static constexpr ecs::component_t hierarchy_unresolved_parent_id_get_type();
static ecs::LTComponentList hierarchy_unresolved_parent_id_component(ECS_HASH("hierarchy_unresolved_parent_id"), hierarchy_unresolved_parent_id_get_type(), "prog/daNetGame/main/gameObjectsES.cpp.inl", "game_objects_events_es_event_handler", 0);
static constexpr ecs::component_t initialTransform_get_type();
static ecs::LTComponentList initialTransform_component(ECS_HASH("initialTransform"), initialTransform_get_type(), "prog/daNetGame/main/gameObjectsES.cpp.inl", "game_objects_events_es_event_handler", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/main/gameObjectsES.cpp.inl", "game_objects_events_es_event_handler", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gameObjectsES.cpp.inl"
ECS_DEF_PULL_VAR(gameObjects);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc game_objects_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void game_objects_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    game_objects_es(*info.cast<UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(game_objects_es_comps, "game_objects", GameObjects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc game_objects_es_es_desc
(
  "game_objects_es",
  "prog/daNetGame/main/gameObjectsES.cpp.inl",
  ecs::EntitySystemOps(game_objects_es_all),
  empty_span(),
  make_span(game_objects_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render,server");
static constexpr ecs::ComponentDesc game_objects_bspheres_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("collision_debug"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void game_objects_bspheres_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    game_objects_bspheres_es(*info.cast<UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc game_objects_bspheres_es_es_desc
(
  "game_objects_bspheres_es",
  "prog/daNetGame/main/gameObjectsES.cpp.inl",
  ecs::EntitySystemOps(game_objects_bspheres_es_all),
  empty_span(),
  empty_span(),
  make_span(game_objects_bspheres_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc game_objects_events_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()},
//start of 3 ro components at [1]
  {ECS_HASH("game_objects__seed"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("game_objects__syncCreation"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("game_objects__createChance"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void game_objects_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<EventGameObjectsOptimize>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      game_objects_events_es_event_handler(static_cast<const EventGameObjectsOptimize&>(evt)
            , ECS_RW_COMP(game_objects_events_es_event_handler_comps, "game_objects", GameObjects)
      );
    while (++comp != compE);
  } else if (evt.is<EventGameObjectsCreated>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      game_objects_events_es_event_handler(static_cast<const EventGameObjectsCreated&>(evt)
            , ECS_RW_COMP(game_objects_events_es_event_handler_comps, "game_objects", GameObjects)
      , ECS_RO_COMP_PTR(game_objects_events_es_event_handler_comps, "game_objects__seed", int)
      , ECS_RO_COMP_PTR(game_objects_events_es_event_handler_comps, "game_objects__syncCreation", ecs::Object)
      , ECS_RO_COMP(game_objects_events_es_event_handler_comps, "game_objects__createChance", ecs::Object)
      );
    while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <%s> in game_objects_events_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc game_objects_events_es_event_handler_es_desc
(
  "game_objects_events_es",
  "prog/daNetGame/main/gameObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_objects_events_es_event_handler_all_events),
  make_span(game_objects_events_es_event_handler_comps+0, 1)/*rw*/,
  make_span(game_objects_events_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsCreated,
                       EventGameObjectsOptimize>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc game_objects_client_events_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void game_objects_client_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventGameObjectsOptimize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    game_objects_client_events_es_event_handler(static_cast<const EventGameObjectsOptimize&>(evt)
        , ECS_RW_COMP(game_objects_client_events_es_event_handler_comps, "game_objects", GameObjects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc game_objects_client_events_es_event_handler_es_desc
(
  "game_objects_client_events_es",
  "prog/daNetGame/main/gameObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_objects_client_events_es_event_handler_all_events),
  make_span(game_objects_client_events_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsOptimize>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc ladder_optimize_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void ladder_optimize_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventGameObjectsOptimize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ladder_optimize_es_event_handler(static_cast<const EventGameObjectsOptimize&>(evt)
        , ECS_RW_COMP(ladder_optimize_es_event_handler_comps, "game_objects", GameObjects)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ladder_optimize_es_event_handler_es_desc
(
  "ladder_optimize_es",
  "prog/daNetGame/main/gameObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ladder_optimize_es_event_handler_all_events),
  make_span(ladder_optimize_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsOptimize>::build(),
  0
);
static constexpr ecs::component_t hierarchy_transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
static constexpr ecs::component_t hierarchy_unresolved_id_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t hierarchy_unresolved_parent_id_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t initialTransform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
