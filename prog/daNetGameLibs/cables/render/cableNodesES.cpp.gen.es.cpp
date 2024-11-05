#include "cableNodesES.cpp.inl"
ECS_DEF_PULL_VAR(cableNodes);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc cabels_node_created_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cable_trans_framegraph_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void cabels_node_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cabels_node_created_es_event_handler(evt
        , ECS_RW_COMP(cabels_node_created_es_event_handler_comps, "cable_trans_framegraph_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cabels_node_created_es_event_handler_es_desc
(
  "cabels_node_created_es",
  "prog/daNetGameLibs/cables/render/cableNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cabels_node_created_es_event_handler_all_events),
  make_span(cabels_node_created_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
