// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "hideAnimcharNodesEcsNodeES.cpp.inl"
ECS_DEF_PULL_VAR(hideAnimcharNodesEcsNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_hide_animchar_node_es_comps[] ={};
static void create_hide_animchar_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_hide_animchar_node_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_hide_animchar_node_es_es_desc
(
  "create_hide_animchar_node_es",
  "prog/daNetGame/render/world/frameGraphNodes/hideAnimcharNodesEcsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_hide_animchar_node_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
