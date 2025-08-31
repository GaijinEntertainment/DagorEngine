// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "scopeFullDeferredES.cpp.inl"
ECS_DEF_PULL_VAR(scopeFullDeferred);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc full_deferred_scope_render_features_changed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("scope__full_deferred_fg_nodes"), ecs::ComponentTypeInfo<dag::Vector<dafg::NodeHandle>>()}
};
static void full_deferred_scope_render_features_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    full_deferred_scope_render_features_changed_es_event_handler(evt
        , ECS_RW_COMP(full_deferred_scope_render_features_changed_es_event_handler_comps, "scope__full_deferred_fg_nodes", dag::Vector<dafg::NodeHandle>)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc full_deferred_scope_render_features_changed_es_event_handler_es_desc
(
  "full_deferred_scope_render_features_changed_es",
  "prog/daNetGameLibs/scope/render/scopeFullDeferredES.cpp.inl",
  ecs::EntitySystemOps(nullptr, full_deferred_scope_render_features_changed_es_event_handler_all_events),
  make_span(full_deferred_scope_render_features_changed_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsReady,
                       SetFxQuality>::build(),
  0
,"render");
