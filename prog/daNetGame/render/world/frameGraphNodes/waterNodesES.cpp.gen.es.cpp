// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterNodesES.cpp.inl"
ECS_DEF_PULL_VAR(waterNodes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_water_nodes_es_comps[] ={};
static void create_water_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_water_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_water_nodes_es_es_desc
(
  "create_water_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_water_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc recreate_water_refraction_stub_es_comps[] ={};
static void recreate_water_refraction_stub_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  recreate_water_refraction_stub_es(evt
        );
}
static ecs::EntitySystemDesc recreate_water_refraction_stub_es_es_desc
(
  "recreate_water_refraction_stub_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, recreate_water_refraction_stub_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc close_water_refraction_stub_es_comps[] ={};
static void close_water_refraction_stub_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  close_water_refraction_stub_es(evt
        );
}
static ecs::EntitySystemDesc close_water_refraction_stub_es_es_desc
(
  "close_water_refraction_stub_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, close_water_refraction_stub_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
