#include "lagCatcherES.cpp.inl"
ECS_DEF_PULL_VAR(lagCatcher);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc lagcatcher_update_es_comps[] ={};
static void lagcatcher_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    lagcatcher_update_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc lagcatcher_update_es_es_desc
(
  "lagcatcher_update_es",
  "prog/daNetGame/main/dedicated/lagCatcherES.cpp.inl",
  ecs::EntitySystemOps(lagcatcher_update_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
//static constexpr ecs::ComponentDesc lagcatcher_level_loaded_es_event_handler_comps[] ={};
static void lagcatcher_level_loaded_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  lagcatcher_level_loaded_es_event_handler(static_cast<const EventLevelLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc lagcatcher_level_loaded_es_event_handler_es_desc
(
  "lagcatcher_level_loaded_es",
  "prog/daNetGame/main/dedicated/lagCatcherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lagcatcher_level_loaded_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
//static constexpr ecs::ComponentDesc lagcatcher_stop_on_clear_es_comps[] ={};
static void lagcatcher_stop_on_clear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerBeforeClear>());
  lagcatcher_stop_on_clear_es(static_cast<const ecs::EventEntityManagerBeforeClear&>(evt)
        );
}
static ecs::EntitySystemDesc lagcatcher_stop_on_clear_es_es_desc
(
  "lagcatcher_stop_on_clear_es",
  "prog/daNetGame/main/dedicated/lagCatcherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lagcatcher_stop_on_clear_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear>::build(),
  0
);
//static constexpr ecs::ComponentDesc lagcatcher_on_scene_clear_es_event_handler_comps[] ={};
static void lagcatcher_on_scene_clear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerAfterClear>());
  lagcatcher_on_scene_clear_es_event_handler(static_cast<const ecs::EventEntityManagerAfterClear&>(evt)
        );
}
static ecs::EntitySystemDesc lagcatcher_on_scene_clear_es_event_handler_es_desc
(
  "lagcatcher_on_scene_clear_es",
  "prog/daNetGame/main/dedicated/lagCatcherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lagcatcher_on_scene_clear_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerAfterClear>::build(),
  0
);
