// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "scopeMobileDeferredES.cpp.inl"
ECS_DEF_PULL_VAR(scopeMobileDeferred);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_mobile_deferred_scope_render_pass_es_event_handler_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("scope__mobile_deferred__manager"), ecs::ComponentTypeInfo<ScopeMobileDeferredManager>()},
  {ECS_HASH("scope__mobile_deferred__begin_rp_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__lens_mask_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__depth_cut_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__lens_mobile_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void create_mobile_deferred_scope_render_pass_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_mobile_deferred_scope_render_pass_es_event_handler(evt
        , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__manager", ScopeMobileDeferredManager)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__begin_rp_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__lens_mask_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__mobile_deferred__depth_cut_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "scope__lens_mobile_node", dafg::NodeHandle)
    , ECS_RW_COMP(create_mobile_deferred_scope_render_pass_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_mobile_deferred_scope_render_pass_es_event_handler_es_desc
(
  "create_mobile_deferred_scope_render_pass_es",
  "prog/daNetGameLibs/scope/render/scopeMobileDeferredES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_mobile_deferred_scope_render_pass_es_event_handler_all_events),
  make_span(create_mobile_deferred_scope_render_pass_es_event_handler_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc mobile_deferred_scope_render_features_changed_es_event_handler_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("scope__mobile_deferred__manager"), ecs::ComponentTypeInfo<ScopeMobileDeferredManager>()},
  {ECS_HASH("scope__mobile_deferred__begin_rp_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__lens_mask_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__mobile_deferred__depth_cut_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("scope__lens_mobile_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void mobile_deferred_scope_render_features_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mobile_deferred_scope_render_features_changed_es_event_handler(evt
        , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__manager", ScopeMobileDeferredManager)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__begin_rp_node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__lens_mask_node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__mobile_deferred__depth_cut_node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "scope__lens_mobile_node", dafg::NodeHandle)
    , ECS_RW_COMP(mobile_deferred_scope_render_features_changed_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mobile_deferred_scope_render_features_changed_es_event_handler_es_desc
(
  "mobile_deferred_scope_render_features_changed_es",
  "prog/daNetGameLibs/scope/render/scopeMobileDeferredES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mobile_deferred_scope_render_features_changed_es_event_handler_all_events),
  make_span(mobile_deferred_scope_render_features_changed_es_event_handler_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
