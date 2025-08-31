// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "stormES.cpp.inl"
ECS_DEF_PULL_VAR(storm);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc storm_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("effect__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 6 ro components at [3]
  {ECS_HASH("storm__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("storm__width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("storm__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("storm__alpha"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("storm__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("storm__maxDensity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void storm_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    storm_es_event_handler(evt
        , ECS_RW_COMP(storm_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(storm_es_event_handler_comps, "storm__speed", float)
    , ECS_RO_COMP(storm_es_event_handler_comps, "storm__width", float)
    , ECS_RO_COMP(storm_es_event_handler_comps, "storm__density", float)
    , ECS_RO_COMP(storm_es_event_handler_comps, "storm__alpha", float)
    , ECS_RO_COMP(storm_es_event_handler_comps, "storm__length", float)
    , ECS_RW_COMP(storm_es_event_handler_comps, "effect__offset", Point3)
    , ECS_RW_COMP(storm_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP_OR(storm_es_event_handler_comps, "storm__maxDensity", float(10))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc storm_es_event_handler_es_desc
(
  "storm_es",
  "prog/daNetGameLibs/storm/render/stormES.cpp.inl",
  ecs::EntitySystemOps(nullptr, storm_es_event_handler_all_events),
  make_span(storm_es_event_handler_comps+0, 3)/*rw*/,
  make_span(storm_es_event_handler_comps+3, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*","bound_camera_effect_es");
static constexpr ecs::ComponentDesc get_wind_strength_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("wind__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__dir"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_wind_strength_ecs_query_desc
(
  "get_wind_strength_ecs_query",
  empty_span(),
  make_span(get_wind_strength_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_wind_strength_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_wind_strength_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_wind_strength_ecs_query_comps, "wind__strength", float)
            , ECS_RO_COMP(get_wind_strength_ecs_query_comps, "wind__dir", float)
            );

        }while (++comp != compE);
    }
  );
}
