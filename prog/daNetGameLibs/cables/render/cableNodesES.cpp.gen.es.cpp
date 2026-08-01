// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cableNodesES.cpp.inl"
ECS_DEF_PULL_VAR(cableNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc cables_view_nodes_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("cables_nodes_registrator"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void cables_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraPerViewNodeConstruction>());
  cables_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc cables_view_nodes_es_es_desc
(
  "cables_view_nodes_es",
  "prog/daNetGameLibs/cables/render/cableNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cables_view_nodes_es_all_events),
  empty_span(),
  empty_span(),
  make_span(cables_view_nodes_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
