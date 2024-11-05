#include "dbgDrawPhysWorldES.cpp.inl"
ECS_DEF_PULL_VAR(dbgDrawPhysWorld);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc physdbg_render_world_es_comps[] ={};
static void physdbg_render_world_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    physdbg_render_world_es(*info.cast<UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc physdbg_render_world_es_es_desc
(
  "physdbg_render_world_es",
  "prog/daNetGame/phys/dbgDrawPhysWorldES.cpp.inl",
  ecs::EntitySystemOps(physdbg_render_world_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
