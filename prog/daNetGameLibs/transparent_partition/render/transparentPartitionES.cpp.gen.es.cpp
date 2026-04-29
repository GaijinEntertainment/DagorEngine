// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "transparentPartitionES.cpp.inl"
ECS_DEF_PULL_VAR(transparentPartition);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_early_particles_node_es_event_handler_comps[] =
{
//start of 6 rw components at [0]
  {ECS_HASH("early_rendinst_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("particles_partition_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("early_particles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("early_particles_lowres_prepare_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("early_particles_lowres_render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("are_early_particle_nodes_created"), ecs::ComponentTypeInfo<bool>()}
};
static void init_early_particles_node_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_early_particles_node_es_event_handler(evt
        , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "early_rendinst_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "particles_partition_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "early_particles_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "early_particles_lowres_prepare_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "early_particles_lowres_render_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_early_particles_node_es_event_handler_comps, "are_early_particle_nodes_created", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_early_particles_node_es_event_handler_es_desc
(
  "init_early_particles_node_es",
  "prog/daNetGameLibs/transparent_partition/render/transparentPartitionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_early_particles_node_es_event_handler_all_events),
  make_span(init_early_particles_node_es_event_handler_comps+0, 6)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel,
                       SetFxQuality,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
