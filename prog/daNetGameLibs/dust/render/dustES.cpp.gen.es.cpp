// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dustES.cpp.inl"
ECS_DEF_PULL_VAR(dust);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dust_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 6 ro components at [2]
  {ECS_HASH("dust__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dust__width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dust__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dust__alpha"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dust__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dust__maxDensity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void dust_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dust_es_event_handler(evt
        , ECS_RW_COMP(dust_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(dust_es_event_handler_comps, "dust__speed", float)
    , ECS_RO_COMP(dust_es_event_handler_comps, "dust__width", float)
    , ECS_RO_COMP(dust_es_event_handler_comps, "dust__density", float)
    , ECS_RO_COMP(dust_es_event_handler_comps, "dust__alpha", float)
    , ECS_RO_COMP(dust_es_event_handler_comps, "dust__length", float)
    , ECS_RW_COMP(dust_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP_OR(dust_es_event_handler_comps, "dust__maxDensity", float(10))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dust_es_event_handler_es_desc
(
  "dust_es",
  "prog/daNetGameLibs/dust/render/dustES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dust_es_event_handler_all_events),
  make_span(dust_es_event_handler_comps+0, 2)/*rw*/,
  make_span(dust_es_event_handler_comps+2, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
