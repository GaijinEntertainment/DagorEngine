#include "invalidateShadowsES.cpp.inl"
ECS_DEF_PULL_VAR(invalidateShadows);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc invalidate_ri_shadows_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_texture_gen"), ecs::ComponentTypeInfo<int>()}
};
static void invalidate_ri_shadows_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    invalidate_ri_shadows_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(invalidate_ri_shadows_es_comps, "ri_texture_gen", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc invalidate_ri_shadows_es_es_desc
(
  "invalidate_ri_shadows_es",
  "prog/daNetGame/render/invalidateShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_ri_shadows_es_all_events),
  make_span(invalidate_ri_shadows_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc invalidate_shadows_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("isRendinstDestr"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void invalidate_shadows_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    invalidate_shadows_es_event_handler(evt
        , ECS_RO_COMP(invalidate_shadows_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP(invalidate_shadows_es_event_handler_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc invalidate_shadows_es_event_handler_es_desc
(
  "invalidate_shadows_es",
  "prog/daNetGame/render/invalidateShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_shadows_es_event_handler_all_events),
  empty_span(),
  make_span(invalidate_shadows_es_event_handler_comps+0, 2)/*ro*/,
  make_span(invalidate_shadows_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render",nullptr,"ri_extra_destroyed_es");
