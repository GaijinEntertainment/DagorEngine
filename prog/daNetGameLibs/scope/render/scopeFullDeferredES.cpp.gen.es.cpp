#include "scopeFullDeferredES.cpp.inl"
ECS_DEF_PULL_VAR(scopeFullDeferred);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_full_deferred_scope_rendering_es_event_handler_comps[] =
{
//start of 12 rw components at [0]
  {ECS_HASH("scope__full_deferred__opaque_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__prepass_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__lens_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__vrs_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__cut_depth_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__crosshair_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__render_lens_frame_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__render_lens_optics_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("aim_dof_prepare_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("aim_dof_restore_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void init_full_deferred_scope_rendering_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_full_deferred_scope_rendering_es_event_handler(evt
        , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__opaque_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__prepass_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__lens_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__vrs_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__cut_depth_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__crosshair_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__render_lens_frame_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "scope__full_deferred__render_lens_optics_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "aim_dof_prepare_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "aim_dof_restore_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_full_deferred_scope_rendering_es_event_handler_comps, "setup_aim_rendering_data_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_full_deferred_scope_rendering_es_event_handler_es_desc
(
  "init_full_deferred_scope_rendering_es",
  "prog/daNetGameLibs/scope/render/scopeFullDeferredES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_full_deferred_scope_rendering_es_event_handler_all_events),
  make_span(init_full_deferred_scope_rendering_es_event_handler_comps+0, 12)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc full_deferred_scope_render_features_changed_es_event_handler_comps[] =
{
//start of 12 rw components at [0]
  {ECS_HASH("scope__full_deferred__opaque_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__prepass_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__lens_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__vrs_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__cut_depth_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__render_lens_frame_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__render_lens_optics_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__full_deferred__crosshair_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("aim_dof_prepare_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("aim_dof_restore_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void full_deferred_scope_render_features_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    full_deferred_scope_render_features_changed_es_event_handler(evt
        , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__opaque_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__prepass_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__lens_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__vrs_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__cut_depth_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__render_lens_frame_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__render_lens_optics_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred__crosshair_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "aim_dof_prepare_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "aim_dof_restore_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dabfg::NodeHandle)
    , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "setup_aim_rendering_data_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc full_deferred_scope_render_features_changed_es_event_handler_es_desc
(
  "full_deferred_scope_render_features_changed_es",
  "prog/daNetGameLibs/scope/render/scopeFullDeferredES.cpp.inl",
  ecs::EntitySystemOps(nullptr, full_deferred_scope_render_features_changed_es_event_handler_all_events),
  make_span(full_deferred_scope_render_features_changed_es_event_handler_comps+0, 12)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
