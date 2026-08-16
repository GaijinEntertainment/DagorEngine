// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "screenSpaceReflectionNodeES.cpp.inl"
ECS_DEF_PULL_VAR(screenSpaceReflectionNode);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_ssr_camera_nodes_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dng_ssr_camera_nodes__config"), ecs::ComponentTypeInfo<SsrNodesConfig>()}
};
static void create_ssr_camera_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraMainViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_ssr_camera_nodes_es(static_cast<const OnCameraMainViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_ssr_camera_nodes_es_comps, "dng_ssr_camera_nodes__config", SsrNodesConfig)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_ssr_camera_nodes_es_es_desc
(
  "create_ssr_camera_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/screenSpaceReflectionNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ssr_camera_nodes_es_all_events),
  empty_span(),
  make_span(create_ssr_camera_nodes_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraMainViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_ssr_camera_view_nodes_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dng_ssr_camera_nodes__config"), ecs::ComponentTypeInfo<SsrNodesConfig>()}
};
static void create_ssr_camera_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraPerViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_ssr_camera_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_ssr_camera_view_nodes_es_comps, "dng_ssr_camera_nodes__config", SsrNodesConfig)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_ssr_camera_view_nodes_es_es_desc
(
  "create_ssr_camera_view_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/screenSpaceReflectionNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ssr_camera_view_nodes_es_all_events),
  empty_span(),
  make_span(create_ssr_camera_view_nodes_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc reset_ssr_camera_nodes_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dng_ssr_camera_nodes__config"), ecs::ComponentTypeInfo<SsrNodesConfig>()},
//start of 1 ro components at [1]
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void reset_ssr_camera_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ResetSsrNodes>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_ssr_camera_nodes_es(static_cast<const ResetSsrNodes&>(evt)
        , ECS_RW_COMP(reset_ssr_camera_nodes_es_comps, "dng_ssr_camera_nodes__config", SsrNodesConfig)
    , ECS_RO_COMP(reset_ssr_camera_nodes_es_comps, "dafg_camera_registrator__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_ssr_camera_nodes_es_es_desc
(
  "reset_ssr_camera_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/screenSpaceReflectionNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_ssr_camera_nodes_es_all_events),
  make_span(reset_ssr_camera_nodes_es_comps+0, 1)/*rw*/,
  make_span(reset_ssr_camera_nodes_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ResetSsrNodes>::build(),
  0
,"render");
