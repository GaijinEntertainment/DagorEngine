// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "heatHazeES.cpp.inl"
ECS_DEF_PULL_VAR(heatHaze);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_heat_haze_es_event_handler_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("heat_haze__manager"), ecs::ComponentTypeInfo<HeatHazeManager>()},
  {ECS_HASH("heat_haze__render_clear_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__render_particles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__apply_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()}
};
static void init_heat_haze_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_heat_haze_es_event_handler(evt
        , ECS_RW_COMP(init_heat_haze_es_event_handler_comps, "heat_haze__manager", HeatHazeManager)
    , ECS_RW_COMP(init_heat_haze_es_event_handler_comps, "heat_haze__render_clear_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_event_handler_comps, "heat_haze__render_particles_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_event_handler_comps, "heat_haze__apply_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_event_handler_comps, "heat_haze__lod", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_heat_haze_es_event_handler_es_desc
(
  "init_heat_haze_es",
  "prog/daNetGameLibs/heat_haze/render/heatHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_heat_haze_es_event_handler_all_events),
  make_span(init_heat_haze_es_event_handler_comps+0, 5)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc heat_haze_render_features_changed_es_event_handler_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("heat_haze__manager"), ecs::ComponentTypeInfo<HeatHazeManager>()},
  {ECS_HASH("heat_haze__render_clear_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__render_particles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__apply_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()}
};
static void heat_haze_render_features_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    heat_haze_render_features_changed_es_event_handler(evt
        , ECS_RW_COMP(heat_haze_render_features_changed_es_event_handler_comps, "heat_haze__manager", HeatHazeManager)
    , ECS_RW_COMP(heat_haze_render_features_changed_es_event_handler_comps, "heat_haze__render_clear_node", dafg::NodeHandle)
    , ECS_RW_COMP(heat_haze_render_features_changed_es_event_handler_comps, "heat_haze__render_particles_node", dafg::NodeHandle)
    , ECS_RW_COMP(heat_haze_render_features_changed_es_event_handler_comps, "heat_haze__apply_node", dafg::NodeHandle)
    , ECS_RW_COMP(heat_haze_render_features_changed_es_event_handler_comps, "heat_haze__lod", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc heat_haze_render_features_changed_es_event_handler_es_desc
(
  "heat_haze_render_features_changed_es",
  "prog/daNetGameLibs/heat_haze/render/heatHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heat_haze_render_features_changed_es_event_handler_all_events),
  make_span(heat_haze_render_features_changed_es_event_handler_comps+0, 5)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
