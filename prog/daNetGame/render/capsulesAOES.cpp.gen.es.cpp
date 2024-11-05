#include "capsulesAOES.cpp.inl"
ECS_DEF_PULL_VAR(capsulesAO);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc capsules_ao_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("capsules_ao"), ecs::ComponentTypeInfo<CapsulesAOHolder>()},
//start of 1 ro components at [1]
  {ECS_HASH("capsules_ao__max_units_per_cell"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static void capsules_ao_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    capsules_ao_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(capsules_ao_es_comps, "capsules_ao", CapsulesAOHolder)
    , ECS_RO_COMP_OR(capsules_ao_es_comps, "capsules_ao__max_units_per_cell", int(32))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc capsules_ao_es_es_desc
(
  "capsules_ao_es",
  "prog/daNetGame/render/capsulesAOES.cpp.inl",
  ecs::EntitySystemOps(nullptr, capsules_ao_es_all_events),
  make_span(capsules_ao_es_comps+0, 1)/*rw*/,
  make_span(capsules_ao_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_capsules_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("capsule_approximation"), ecs::ComponentTypeInfo<ecs::SharedComponent<CapsuleApproximation>>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc get_capsules_ecs_query_desc
(
  "get_capsules_ecs_query",
  empty_span(),
  make_span(get_capsules_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_capsules_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_capsules_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_capsules_ecs_query_comps, "capsule_approximation", ecs::SharedComponent<CapsuleApproximation>)
            , ECS_RO_COMP(get_capsules_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP(get_capsules_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
