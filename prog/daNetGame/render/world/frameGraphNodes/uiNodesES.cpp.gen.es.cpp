// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "uiNodesES.cpp.inl"
ECS_DEF_PULL_VAR(uiNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_ui_nodes_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("before_ui_begin"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("before_ui_end"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("ui_render"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("ui_blend"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void create_ui_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_ui_nodes_es(evt
        , ECS_RW_COMP(create_ui_nodes_es_comps, "before_ui_begin", dafg::NodeHandle)
    , ECS_RW_COMP(create_ui_nodes_es_comps, "before_ui_end", dafg::NodeHandle)
    , ECS_RW_COMP(create_ui_nodes_es_comps, "ui_render", dafg::NodeHandle)
    , ECS_RW_COMP(create_ui_nodes_es_comps, "ui_blend", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_ui_nodes_es_es_desc
(
  "create_ui_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/uiNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ui_nodes_es_all_events),
  make_span(create_ui_nodes_es_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction,
                       SetAntialiasing>::build(),
  0
,"render");
