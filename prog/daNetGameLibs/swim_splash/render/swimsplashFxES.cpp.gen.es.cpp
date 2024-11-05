#include "swimsplashFxES.cpp.inl"
ECS_DEF_PULL_VAR(swimsplashFx);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc swim_spash_fx_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("swimsplash"), ecs::ComponentTypeInfo<SwimSplash>()}
};
static void swim_spash_fx_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventSwimSplash>());
  swim_spash_fx_es_event_handler(static_cast<const EventSwimSplash&>(evt)
        );
}
static ecs::EntitySystemDesc swim_spash_fx_es_event_handler_es_desc
(
  "swim_spash_fx_es",
  "prog/daNetGameLibs/swim_splash/render/swimsplashFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, swim_spash_fx_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(swim_spash_fx_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventSwimSplash>::build(),
  0
);
static constexpr ecs::ComponentDesc swimsplash_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("swimsplash"), ecs::ComponentTypeInfo<SwimSplash>()},
//start of 8 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("swimsplash__maxDistanceFromCameraToUpdate"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("swimsplash__scaleMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("swimsplash__minTimeBetweenSplashes"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("swimsplash__maxRenderingSpeed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("animchar__visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void swimsplash_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP_OR(swimsplash_es_comps, "animchar__visible", bool( true))) )
      continue;
    swimsplash_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
          , ECS_RO_COMP(swimsplash_es_comps, "eid", ecs::EntityId)
      , ECS_RW_COMP(swimsplash_es_comps, "swimsplash", SwimSplash)
      , ECS_RO_COMP(swimsplash_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
      , ECS_RO_COMP(swimsplash_es_comps, "transform", TMatrix)
      , ECS_RO_COMP(swimsplash_es_comps, "swimsplash__maxDistanceFromCameraToUpdate", float)
      , ECS_RO_COMP(swimsplash_es_comps, "swimsplash__scaleMul", float)
      , ECS_RO_COMP(swimsplash_es_comps, "swimsplash__minTimeBetweenSplashes", float)
      , ECS_RO_COMP(swimsplash_es_comps, "swimsplash__maxRenderingSpeed", float)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc swimsplash_es_es_desc
(
  "swimsplash_es",
  "prog/daNetGameLibs/swim_splash/render/swimsplashFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, swimsplash_es_all_events),
  make_span(swimsplash_es_comps+0, 1)/*rw*/,
  make_span(swimsplash_es_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,nullptr,"start_async_phys_sim_es");
