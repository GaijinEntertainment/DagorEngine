// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dynamicLightingNodesES.cpp.inl"
ECS_DEF_PULL_VAR(dynamicLightingNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dynamic_lighting_on_appear_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("dynamic_lighting_is_rtsm"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("prepare_dynamic_lighting_texture_per_camera"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_generate_tiles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void dynamic_lighting_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_lighting_on_appear_es(evt
        , ECS_RW_COMP(dynamic_lighting_on_appear_es_comps, "dynamic_lighting_is_rtsm", bool)
    , ECS_RW_COMP(dynamic_lighting_on_appear_es_comps, "prepare_dynamic_lighting_texture_per_camera", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_appear_es_comps, "dynamic_lighting_generate_tiles_node", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_appear_es_comps, "dynamic_lighting_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_lighting_on_appear_es_es_desc
(
  "dynamic_lighting_on_appear_es",
  "prog/daNetGame/render/world/frameGraphNodes/dynamicLightingNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_lighting_on_appear_es_all_events),
  make_span(dynamic_lighting_on_appear_es_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dynamic_lighting_on_change_render_features_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("prepare_dynamic_lighting_texture_per_camera"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_generate_tiles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 ro components at [3]
  {ECS_HASH("dynamic_lighting_is_rtsm"), ecs::ComponentTypeInfo<bool>()}
};
static void dynamic_lighting_on_change_render_features_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ChangeRenderFeatures>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_lighting_on_change_render_features_es(static_cast<const ChangeRenderFeatures&>(evt)
        , ECS_RO_COMP(dynamic_lighting_on_change_render_features_es_comps, "dynamic_lighting_is_rtsm", bool)
    , ECS_RW_COMP(dynamic_lighting_on_change_render_features_es_comps, "prepare_dynamic_lighting_texture_per_camera", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_change_render_features_es_comps, "dynamic_lighting_generate_tiles_node", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_change_render_features_es_comps, "dynamic_lighting_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_lighting_on_change_render_features_es_es_desc
(
  "dynamic_lighting_on_change_render_features_es",
  "prog/daNetGame/render/world/frameGraphNodes/dynamicLightingNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_lighting_on_change_render_features_es_all_events),
  make_span(dynamic_lighting_on_change_render_features_es_comps+0, 3)/*rw*/,
  make_span(dynamic_lighting_on_change_render_features_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dynamic_lighting_on_set_resolution_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("prepare_dynamic_lighting_texture_per_camera"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_generate_tiles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 ro components at [3]
  {ECS_HASH("dynamic_lighting_is_rtsm"), ecs::ComponentTypeInfo<bool>()}
};
static void dynamic_lighting_on_set_resolution_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_lighting_on_set_resolution_es(static_cast<const SetResolutionEvent&>(evt)
        , ECS_RO_COMP(dynamic_lighting_on_set_resolution_es_comps, "dynamic_lighting_is_rtsm", bool)
    , ECS_RW_COMP(dynamic_lighting_on_set_resolution_es_comps, "prepare_dynamic_lighting_texture_per_camera", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_set_resolution_es_comps, "dynamic_lighting_generate_tiles_node", dafg::NodeHandle)
    , ECS_RW_COMP(dynamic_lighting_on_set_resolution_es_comps, "dynamic_lighting_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_lighting_on_set_resolution_es_es_desc
(
  "dynamic_lighting_on_set_resolution_es",
  "prog/daNetGame/render/world/frameGraphNodes/dynamicLightingNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_lighting_on_set_resolution_es_all_events),
  make_span(dynamic_lighting_on_set_resolution_es_comps+0, 3)/*rw*/,
  make_span(dynamic_lighting_on_set_resolution_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_dynamic_lighting_state_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("dynamic_lighting_is_rtsm"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("prepare_dynamic_lighting_texture_per_camera"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_generate_tiles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("dynamic_lighting_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc set_dynamic_lighting_state_ecs_query_desc
(
  "set_dynamic_lighting_state_ecs_query",
  make_span(set_dynamic_lighting_state_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_dynamic_lighting_state_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, set_dynamic_lighting_state_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_dynamic_lighting_state_ecs_query_comps, "dynamic_lighting_is_rtsm", bool)
            , ECS_RW_COMP(set_dynamic_lighting_state_ecs_query_comps, "prepare_dynamic_lighting_texture_per_camera", dafg::NodeHandle)
            , ECS_RW_COMP(set_dynamic_lighting_state_ecs_query_comps, "dynamic_lighting_generate_tiles_node", dafg::NodeHandle)
            , ECS_RW_COMP(set_dynamic_lighting_state_ecs_query_comps, "dynamic_lighting_node", dafg::NodeHandle)
            );

        }
    }
  );
}
