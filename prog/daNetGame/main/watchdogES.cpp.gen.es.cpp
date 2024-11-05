#include "watchdogES.cpp.inl"
ECS_DEF_PULL_VAR(watchdog);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc watchdog_es_comps[] ={};
static void watchdog_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    watchdog_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc watchdog_es_es_desc
(
  "watchdog_es",
  "prog/daNetGame/main/watchdogES.cpp.inl",
  ecs::EntitySystemOps(watchdog_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
);
