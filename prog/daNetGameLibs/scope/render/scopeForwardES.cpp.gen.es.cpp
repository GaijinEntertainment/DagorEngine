#include "scopeForwardES.cpp.inl"
ECS_DEF_PULL_VAR(scopeForward);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_forward_scope_rendering_es_event_handler_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("forward__setup_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__prepass_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__lens_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__vrs_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__lens_hole_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__lens_mobile_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void init_forward_scope_rendering_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_forward_scope_rendering_es_event_handler(evt
        , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "forward__setup_aim_rendering_data_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__forward__prepass_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__forward__node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__forward__lens_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__forward__vrs_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__forward__lens_hole_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "scope__lens_mobile_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_forward_scope_rendering_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_forward_scope_rendering_es_event_handler_es_desc
(
  "init_forward_scope_rendering_es",
  "prog/daNetGameLibs/scope/render/scopeForwardES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_forward_scope_rendering_es_event_handler_all_events),
  make_span(init_forward_scope_rendering_es_event_handler_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc forward_scope_render_features_changed_es_event_handler_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("forward__setup_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__prepass_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__lens_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__vrs_mask_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__forward__lens_hole_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("scope__lens_mobile_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("setup_scope_aim_rendering_data_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void forward_scope_render_features_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    forward_scope_render_features_changed_es_event_handler(evt
        , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "forward__setup_aim_rendering_data_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__forward__prepass_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__forward__node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__forward__lens_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__forward__vrs_mask_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__forward__lens_hole_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "scope__lens_mobile_node", dabfg::NodeHandle)
    , ECS_RW_COMP(forward_scope_render_features_changed_es_event_handler_comps, "setup_scope_aim_rendering_data_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc forward_scope_render_features_changed_es_event_handler_es_desc
(
  "forward_scope_render_features_changed_es",
  "prog/daNetGameLibs/scope/render/scopeForwardES.cpp.inl",
  ecs::EntitySystemOps(nullptr, forward_scope_render_features_changed_es_event_handler_all_events),
  make_span(forward_scope_render_features_changed_es_event_handler_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
