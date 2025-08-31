// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "zoneForceFieldES.cpp.inl"
ECS_DEF_PULL_VAR(zoneForceField);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gather_spheres_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("zone_force_field"), ecs::ComponentTypeInfo<ZoneForceFieldRenderer>()}
};
static void gather_spheres_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gather_spheres_es_event_handler(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(gather_spheres_es_event_handler_comps, "zone_force_field", ZoneForceFieldRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gather_spheres_es_event_handler_es_desc
(
  "gather_spheres_es",
  "prog/daNetGameLibs/zone_force_field/render/zoneForceFieldES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gather_spheres_es_event_handler_all_events),
  make_span(gather_spheres_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc zone_reset_on_new_level_es_event_handler_comps[] ={};
static void zone_reset_on_new_level_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  zone_reset_on_new_level_es_event_handler(static_cast<const EventLevelLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc zone_reset_on_new_level_es_event_handler_es_desc
(
  "zone_reset_on_new_level_es",
  "prog/daNetGameLibs/zone_force_field/render/zoneForceFieldES.cpp.inl",
  ecs::EntitySystemOps(nullptr, zone_reset_on_new_level_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc update_transparent_partition_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("partition_sphere"), ecs::ComponentTypeInfo<PartitionSphere>()}
};
static ecs::CompileTimeQueryDesc update_transparent_partition_ecs_query_desc
(
  "update_transparent_partition_ecs_query",
  make_span(update_transparent_partition_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_transparent_partition_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_transparent_partition_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_transparent_partition_ecs_query_comps, "partition_sphere", PartitionSphere)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc local_player_immune_to_forcefield_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("possessed"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("is_local"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc local_player_immune_to_forcefield_ecs_query_desc
(
  "local_player_immune_to_forcefield_ecs_query",
  empty_span(),
  make_span(local_player_immune_to_forcefield_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void local_player_immune_to_forcefield_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, local_player_immune_to_forcefield_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(local_player_immune_to_forcefield_ecs_query_comps, "possessed", ecs::EntityId)
            , ECS_RO_COMP(local_player_immune_to_forcefield_ecs_query_comps, "is_local", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc zone_force_field_render_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("zone_pos_radius"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("forcefieldFog__fadeSlowness"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("forcefieldFog__fadeLimit"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("forcefieldAppliesFog"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [6]
  {ECS_HASH("render_forcefield"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc zone_force_field_render_ecs_query_desc
(
  "zone_force_field_render_ecs_query",
  empty_span(),
  make_span(zone_force_field_render_ecs_query_comps+0, 6)/*ro*/,
  make_span(zone_force_field_render_ecs_query_comps+6, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void zone_force_field_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, zone_force_field_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(zone_force_field_render_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(zone_force_field_render_ecs_query_comps, "sphere_zone__radius", float)
            , ECS_RO_COMP(zone_force_field_render_ecs_query_comps, "zone_pos_radius", ShaderVar)
            , ECS_RO_COMP_OR(zone_force_field_render_ecs_query_comps, "forcefieldFog__fadeSlowness", float(16.0f))
            , ECS_RO_COMP_OR(zone_force_field_render_ecs_query_comps, "forcefieldFog__fadeLimit", float(0.4f))
            , ECS_RO_COMP_OR(zone_force_field_render_ecs_query_comps, "forcefieldAppliesFog", bool(false))
            );

        }while (++comp != compE);
    }
  );
}
