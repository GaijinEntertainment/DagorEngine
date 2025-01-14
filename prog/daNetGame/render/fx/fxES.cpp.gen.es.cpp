#include "fxES.cpp.inl"
ECS_DEF_PULL_VAR(fx);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_gravity_zone_buffer_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dafx_gravity_zone_buffer_gpu"), ecs::ComponentTypeInfo<UniqueBufHolder>()}
};
static void create_gravity_zone_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    acesfx::create_gravity_zone_buffer_es(evt
        , ECS_RW_COMP(create_gravity_zone_buffer_es_comps, "dafx_gravity_zone_buffer_gpu", UniqueBufHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_gravity_zone_buffer_es_es_desc
(
  "create_gravity_zone_buffer_es",
  "prog/daNetGame/render/fx/fxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_gravity_zone_buffer_es_all_events),
  make_span(create_gravity_zone_buffer_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc start_effect_pos_norm_es_comps[] ={};
static void start_effect_pos_norm_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<acesfx::StartEffectPosNormEvent>());
  start_effect_pos_norm_es(static_cast<const acesfx::StartEffectPosNormEvent&>(evt)
        );
}
static ecs::EntitySystemDesc start_effect_pos_norm_es_es_desc
(
  "start_effect_pos_norm_es",
  "prog/daNetGame/render/fx/fxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, start_effect_pos_norm_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<acesfx::StartEffectPosNormEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_gravity_zone_buffer_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dafx_gravity_zone_buffer_gpu"), ecs::ComponentTypeInfo<UniqueBufHolder>()}
};
static ecs::CompileTimeQueryDesc update_gravity_zone_buffer_ecs_query_desc
(
  "acesfx::update_gravity_zone_buffer_ecs_query",
  make_span(update_gravity_zone_buffer_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void acesfx::update_gravity_zone_buffer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_gravity_zone_buffer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_gravity_zone_buffer_ecs_query_comps, "dafx_gravity_zone_buffer_gpu", UniqueBufHolder)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc effect_quality_reset_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("global_fx__target"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc effect_quality_reset_ecs_query_desc
(
  "acesfx::effect_quality_reset_ecs_query",
  empty_span(),
  make_span(effect_quality_reset_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult acesfx::effect_quality_reset_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, effect_quality_reset_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(effect_quality_reset_ecs_query_comps, "global_fx__target", int)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc thermal_vision_on_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("thermal_vision__on"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc thermal_vision_on_ecs_query_desc
(
  "acesfx::thermal_vision_on_ecs_query",
  empty_span(),
  make_span(thermal_vision_on_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void acesfx::thermal_vision_on_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, thermal_vision_on_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(thermal_vision_on_ecs_query_comps, "thermal_vision__on", bool)
            );

        }
    }
  );
}
