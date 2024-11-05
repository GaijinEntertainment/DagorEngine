#include "deferToAct.cpp.inl"
ECS_DEF_PULL_VAR(deferToAct);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc defer_to_act_es_comps[] ={};
static void defer_to_act_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    defer_to_act_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc defer_to_act_es_es_desc
(
  "defer_to_act_es",
  "prog/gameLibs/ecs/deferToAct/deferToAct.cpp.inl",
  ecs::EntitySystemOps(defer_to_act_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
