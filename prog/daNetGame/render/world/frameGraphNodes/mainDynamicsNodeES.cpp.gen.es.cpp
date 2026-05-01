// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "mainDynamicsNodeES.cpp.inl"
ECS_DEF_PULL_VAR(mainDynamicsNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_opaque_dynamics_nodes_es_comps[] ={};
static void create_opaque_dynamics_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_opaque_dynamics_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_opaque_dynamics_nodes_es_es_desc
(
  "create_opaque_dynamics_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/mainDynamicsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_opaque_dynamics_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc create_opaque_dynamics_triangle_debug_es_comps[] ={};
static void create_opaque_dynamics_triangle_debug_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<CreateTriangleDebugNodes>());
  create_opaque_dynamics_triangle_debug_es(static_cast<const CreateTriangleDebugNodes&>(evt)
        );
}
static ecs::EntitySystemDesc create_opaque_dynamics_triangle_debug_es_es_desc
(
  "create_opaque_dynamics_triangle_debug_es",
  "prog/daNetGame/render/world/frameGraphNodes/mainDynamicsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_opaque_dynamics_triangle_debug_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CreateTriangleDebugNodes>::build(),
  0
,"dev,render");
