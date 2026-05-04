// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "asyncAnimcharRenderingStartNodeES.cpp.inl"
ECS_DEF_PULL_VAR(asyncAnimcharRenderingStartNode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_async_animchar_rendering_start_nodes_es_comps[] ={};
static void create_async_animchar_rendering_start_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_async_animchar_rendering_start_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_async_animchar_rendering_start_nodes_es_es_desc
(
  "create_async_animchar_rendering_start_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/asyncAnimcharRenderingStartNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_async_animchar_rendering_start_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
