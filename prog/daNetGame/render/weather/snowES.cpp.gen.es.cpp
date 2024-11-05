#include "snowES.cpp.inl"
ECS_DEF_PULL_VAR(snow);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc snow_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>(), ecs::CDF_OPTIONAL},
//start of 6 ro components at [2]
  {ECS_HASH("snow__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("snow__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("snow__width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("snow__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("snow__alpha"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("snow__maxDensity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void snow_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    snow_es_event_handler(evt
        , ECS_RW_COMP(snow_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(snow_es_event_handler_comps, "snow__length", float)
    , ECS_RO_COMP(snow_es_event_handler_comps, "snow__speed", float)
    , ECS_RO_COMP(snow_es_event_handler_comps, "snow__width", float)
    , ECS_RO_COMP(snow_es_event_handler_comps, "snow__density", float)
    , ECS_RO_COMP(snow_es_event_handler_comps, "snow__alpha", float)
    , ECS_RW_COMP_PTR(snow_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP_OR(snow_es_event_handler_comps, "snow__maxDensity", float(10))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc snow_es_event_handler_es_desc
(
  "snow_es",
  "prog/daNetGame/render/weather/snowES.cpp.inl",
  ecs::EntitySystemOps(nullptr, snow_es_event_handler_all_events),
  make_span(snow_es_event_handler_comps+0, 2)/*rw*/,
  make_span(snow_es_event_handler_comps+2, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
