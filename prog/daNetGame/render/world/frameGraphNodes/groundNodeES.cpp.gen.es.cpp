// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "groundNodeES.cpp.inl"
ECS_DEF_PULL_VAR(groundNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_ground_nodes_es_comps[] ={};
static void create_ground_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_ground_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_ground_nodes_es_es_desc
(
  "create_ground_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/groundNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ground_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc create_ground_triangle_debug_es_comps[] ={};
static void create_ground_triangle_debug_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<CreateTriangleDebugNodes>());
  create_ground_triangle_debug_es(static_cast<const CreateTriangleDebugNodes&>(evt)
        );
}
static ecs::EntitySystemDesc create_ground_triangle_debug_es_es_desc
(
  "create_ground_triangle_debug_es",
  "prog/daNetGame/render/world/frameGraphNodes/groundNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ground_triangle_debug_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CreateTriangleDebugNodes>::build(),
  0
,"dev,render");
