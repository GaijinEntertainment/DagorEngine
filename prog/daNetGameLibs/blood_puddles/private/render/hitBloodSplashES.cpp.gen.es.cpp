#include "hitBloodSplashES.cpp.inl"
ECS_DEF_PULL_VAR(hitBloodSplash);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc blood_puddles_dark_zone_shredder_es_event_handler_comps[] ={};
static void blood_puddles_dark_zone_shredder_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  blood_puddles_dark_zone_shredder_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc blood_puddles_dark_zone_shredder_es_event_handler_es_desc
(
  "blood_puddles_dark_zone_shredder_es",
  "prog/daNetGameLibs/blood_puddles/private/render/hitBloodSplashES.cpp.inl",
  ecs::EntitySystemOps(nullptr, blood_puddles_dark_zone_shredder_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<eastl::integral_constant<ecs::event_type_t,
                       str_hash_fnv1("EventOnMovingZoneStarted")>>::build(),
  0
,"server",nullptr,"*");
static constexpr ecs::ComponentDesc dark_zones_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dark_zone_shredder__start"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc dark_zones_ecs_query_desc
(
  "dark_zones_ecs_query",
  empty_span(),
  make_span(dark_zones_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void dark_zones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, dark_zones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(dark_zones_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(dark_zones_ecs_query_comps, "sphere_zone__radius", float)
            , ECS_RO_COMP(dark_zones_ecs_query_comps, "dark_zone_shredder__start", float)
            );

        }while (++comp != compE);
    }
  );
}
