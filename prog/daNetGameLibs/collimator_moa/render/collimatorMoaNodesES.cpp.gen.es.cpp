// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaNodesES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_collimator_moa_lens_render_fg_node_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("collimator_moa_render__lens_render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void create_collimator_moa_lens_render_fg_node_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_collimator_moa_lens_render_fg_node_es_event_handler(evt
        , ECS_RW_COMP(create_collimator_moa_lens_render_fg_node_es_event_handler_comps, "collimator_moa_render__lens_render_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_collimator_moa_lens_render_fg_node_es_event_handler_es_desc
(
  "create_collimator_moa_lens_render_fg_node_es",
  "prog/daNetGameLibs/collimator_moa/render/collimatorMoaNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_collimator_moa_lens_render_fg_node_es_event_handler_all_events),
  make_span(create_collimator_moa_lens_render_fg_node_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
