#include "depthAboveRenderTransparentES.cpp.inl"
ECS_DEF_PULL_VAR(depthAboveRenderTransparent);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc request_depth_above_render_transparent_on_appear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("request_depth_above_render_transparent"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void request_depth_above_render_transparent_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  request_depth_above_render_transparent_on_appear_es(evt
        );
}
static ecs::EntitySystemDesc request_depth_above_render_transparent_on_appear_es_es_desc
(
  "request_depth_above_render_transparent_on_appear_es",
  "prog/daNetGame/render/depthAboveRenderTransparentES.cpp.inl",
  ecs::EntitySystemOps(nullptr, request_depth_above_render_transparent_on_appear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(request_depth_above_render_transparent_on_appear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc request_depth_above_render_transparent_on_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("request_depth_above_render_transparent"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void request_depth_above_render_transparent_on_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  request_depth_above_render_transparent_on_disappear_es(evt
        );
}
static ecs::EntitySystemDesc request_depth_above_render_transparent_on_disappear_es_es_desc
(
  "request_depth_above_render_transparent_on_disappear_es",
  "prog/daNetGame/render/depthAboveRenderTransparentES.cpp.inl",
  ecs::EntitySystemOps(nullptr, request_depth_above_render_transparent_on_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(request_depth_above_render_transparent_on_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
