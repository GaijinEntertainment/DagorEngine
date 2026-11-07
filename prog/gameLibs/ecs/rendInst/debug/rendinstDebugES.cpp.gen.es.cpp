// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "rendinstDebugES.cpp.inl"
ECS_DEF_PULL_VAR(rendinstDebug);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc show_rigen_cells_debug_es_comps[] ={};
static void show_rigen_cells_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    show_rigen_cells_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc show_rigen_cells_debug_es_es_desc
(
  "show_rigen_cells_debug_es",
  "prog/gameLibs/ecs/rendInst/debug/rendinstDebugES.cpp.inl",
  ecs::EntitySystemOps(show_rigen_cells_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
