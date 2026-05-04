// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "frameDownsampleNodeES.cpp.inl"
ECS_DEF_PULL_VAR(frameDownsampleNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_frame_downsample_nodes_es_comps[] ={};
static void create_frame_downsample_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_frame_downsample_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_frame_downsample_nodes_es_es_desc
(
  "create_frame_downsample_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/frameDownsampleNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_frame_downsample_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
