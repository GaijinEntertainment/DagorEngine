// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "fxES.cpp.inl"
ECS_DEF_PULL_VAR(fx);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_gravity_zone_buffer_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dafx_gravity_zone_buffer_gpu"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("dafx_gravity_zone_buffer_gpu_staging"), ecs::ComponentTypeInfo<UniqueBuf>()}
};
static void create_gravity_zone_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    acesfx::create_gravity_zone_buffer_es(evt
        , ECS_RW_COMP(create_gravity_zone_buffer_es_comps, "dafx_gravity_zone_buffer_gpu", UniqueBufHolder)
    , ECS_RW_COMP(create_gravity_zone_buffer_es_comps, "dafx_gravity_zone_buffer_gpu_staging", UniqueBuf)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_gravity_zone_buffer_es_es_desc
(
  "create_gravity_zone_buffer_es",
  "prog/daNetGame/render/fx/fxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_gravity_zone_buffer_es_all_events),
  make_span(create_gravity_zone_buffer_es_comps+0, 2)/*rw*/,
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
static constexpr ecs::ComponentDesc init_fx_discard_es_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("global_fx__discard__lowres"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__highres"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__distortion"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__volfogInjection"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__atest"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__waterProj"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__thermal"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__underwater"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__fom"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("global_fx__discard__bvh"), ecs::ComponentTypeInfo<float>()}
};
static void init_fx_discard_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_fx_discard_es(evt
        , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__lowres", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__highres", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__distortion", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__volfogInjection", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__atest", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__waterProj", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__thermal", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__underwater", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__fom", float)
    , ECS_RO_COMP(init_fx_discard_es_comps, "global_fx__discard__bvh", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_fx_discard_es_es_desc
(
  "init_fx_discard_es",
  "prog/daNetGame/render/fx/fxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_fx_discard_es_all_events),
  empty_span(),
  make_span(init_fx_discard_es_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc update_gravity_zone_buffer_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dafx_gravity_zone_buffer_gpu"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("dafx_gravity_zone_buffer_gpu_staging"), ecs::ComponentTypeInfo<UniqueBuf>()}
};
static ecs::CompileTimeQueryDesc update_gravity_zone_buffer_ecs_query_desc
(
  "acesfx::update_gravity_zone_buffer_ecs_query",
  make_span(update_gravity_zone_buffer_ecs_query_comps+0, 2)/*rw*/,
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
            , ECS_RW_COMP(update_gravity_zone_buffer_ecs_query_comps, "dafx_gravity_zone_buffer_gpu_staging", UniqueBuf)
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
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(effect_quality_reset_ecs_query_comps, "global_fx__target", int)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
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
