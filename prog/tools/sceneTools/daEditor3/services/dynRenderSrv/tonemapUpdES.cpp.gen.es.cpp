#include "tonemapUpdES.cpp.inl"
ECS_DEF_PULL_VAR(tonemapUpd);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tonemap_update_es_event_handler_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("color_grading"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemap"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("white_balance"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemap_update_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  tonemap_update_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc tonemap_update_es_event_handler_es_desc
(
  "tonemap_update_es",
  "prog/tools/sceneTools/daEditor3/services/dynRenderSrv/tonemapUpdES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemap_update_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(tonemap_update_es_event_handler_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"color_grading,tonemap,white_balance");
