#include "scriptsDebugES.cpp.inl"
ECS_DEF_PULL_VAR(scriptsDebug);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc display_das_errors_es_comps[] ={};
static void display_das_errors_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    display_das_errors_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc display_das_errors_es_es_desc
(
  "display_das_errors_es",
  "prog/daNetGame/game/scriptsDebugES.cpp.inl",
  ecs::EntitySystemOps(display_das_errors_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"dasDebug",nullptr,"*");
