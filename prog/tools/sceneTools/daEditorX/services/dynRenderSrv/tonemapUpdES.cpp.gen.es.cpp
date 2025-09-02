// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "tonemapUpdES.cpp.inl"
ECS_DEF_PULL_VAR(tonemapUpd);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ed_tonemap_update_es_event_handler_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("color_grading"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemap"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("white_balance"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ed_tonemap_update_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  ed_tonemap_update_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc ed_tonemap_update_es_event_handler_es_desc
(
  "ed_tonemap_update_es",
  "prog/tools/sceneTools/daEditorX/services/dynRenderSrv/tonemapUpdES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ed_tonemap_update_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(ed_tonemap_update_es_event_handler_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"color_grading,tonemap,white_balance");
