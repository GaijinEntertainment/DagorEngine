// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gameResES.cpp.inl"
ECS_DEF_PULL_VAR(gameRes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc cancel_loading_ecs_jobs_es_comps[] ={};
static void cancel_loading_ecs_jobs_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerBeforeClear>());
  cancel_loading_ecs_jobs_es(static_cast<const ecs::EventEntityManagerBeforeClear&>(evt)
        );
}
static ecs::EntitySystemDesc cancel_loading_ecs_jobs_es_es_desc
(
  "cancel_loading_ecs_jobs_es",
  "prog/gameLibs/ecs/gameres/gameResES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cancel_loading_ecs_jobs_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear>::build(),
  0
,nullptr,nullptr,"*");
