// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "physBodyES.cpp.inl"
ECS_DEF_PULL_VAR(physBody);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_phys_body_transform_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys_body"), ecs::ComponentTypeInfo<physbody_t>()}
};
static void update_phys_body_transform_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    update_phys_body_transform_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(update_phys_body_transform_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(update_phys_body_transform_es_comps, "phys_body", physbody_t)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_phys_body_transform_es_es_desc
(
  "update_phys_body_transform_es",
  "prog/gameLibs/ecs/phys/physBodyES.cpp.inl",
  ecs::EntitySystemOps(update_phys_body_transform_es_all),
  make_span(update_phys_body_transform_es_comps+0, 1)/*rw*/,
  make_span(update_phys_body_transform_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
