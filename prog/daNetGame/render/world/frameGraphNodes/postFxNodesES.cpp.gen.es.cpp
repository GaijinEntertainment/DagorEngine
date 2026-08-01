// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "postFxNodesES.cpp.inl"
ECS_DEF_PULL_VAR(postFxNodes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_postfx_nodes_es_comps[] ={};
static void create_postfx_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
if (evt.is<OnCameraNodeConstruction>()) {
    create_postfx_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
            );
} else if (evt.is<OnCameraNodeWithSlotsConstruction>()) {
    create_postfx_nodes_es(static_cast<const OnCameraNodeWithSlotsConstruction&>(evt)
            );
  } else {G_ASSERTF(0, "Unexpected event type <%s> in create_postfx_nodes_es", evt.getName());}
}
static ecs::EntitySystemDesc create_postfx_nodes_es_es_desc
(
  "create_postfx_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/postFxNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_postfx_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeWithSlotsConstruction,
                       OnCameraNodeConstruction>::build(),
  0
,"render");
