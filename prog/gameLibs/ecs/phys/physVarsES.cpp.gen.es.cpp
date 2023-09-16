#include "physVarsES.cpp.inl"
ECS_DEF_PULL_VAR(physVars);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc phys_vars_fixed_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_vars"), ecs::ComponentTypeInfo<PhysVars>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys_vars__fixedInit"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void phys_vars_fixed_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_vars_fixed_init_es_event_handler(evt
        , ECS_RW_COMP(phys_vars_fixed_init_es_event_handler_comps, "phys_vars", PhysVars)
    , ECS_RO_COMP(phys_vars_fixed_init_es_event_handler_comps, "phys_vars__fixedInit", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_vars_fixed_init_es_event_handler_es_desc
(
  "phys_vars_fixed_init_es",
  "prog/gameLibs/ecs/phys/physVarsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_vars_fixed_init_es_event_handler_all_events),
  make_span(phys_vars_fixed_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(phys_vars_fixed_init_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,"anim_phys_init_es");
static constexpr ecs::ComponentDesc phys_vars_random_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_vars"), ecs::ComponentTypeInfo<PhysVars>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys_vars__randomInit"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void phys_vars_random_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_vars_random_init_es_event_handler(evt
        , ECS_RW_COMP(phys_vars_random_init_es_event_handler_comps, "phys_vars", PhysVars)
    , ECS_RO_COMP(phys_vars_random_init_es_event_handler_comps, "phys_vars__randomInit", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_vars_random_init_es_event_handler_es_desc
(
  "phys_vars_random_init_es",
  "prog/gameLibs/ecs/phys/physVarsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_vars_random_init_es_event_handler_all_events),
  make_span(phys_vars_random_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(phys_vars_random_init_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,"anim_phys_init_es");
