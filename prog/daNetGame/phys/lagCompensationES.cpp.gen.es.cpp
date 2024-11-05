#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t isAlive_get_type();
static ecs::LTComponentList isAlive_component(ECS_HASH("isAlive"), isAlive_get_type(), "prog/daNetGame/phys/lagCompensationES.cpp.inl", "", 0);
#include "lagCompensationES.cpp.inl"
ECS_DEF_PULL_VAR(lagCompensation);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc lag_compensation_update_es_comps[] ={};
static void lag_compensation_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    lag_compensation_update_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc lag_compensation_update_es_es_desc
(
  "lag_compensation_update_es",
  "prog/daNetGame/phys/lagCompensationES.cpp.inl",
  ecs::EntitySystemOps(lag_compensation_update_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"net,server");
static constexpr ecs::component_t isAlive_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
