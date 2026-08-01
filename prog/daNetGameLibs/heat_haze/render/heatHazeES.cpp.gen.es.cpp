// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "heatHazeES.cpp.inl"
ECS_DEF_PULL_VAR(heatHaze);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_heat_haze_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("heat_haze__manager"), ecs::ComponentTypeInfo<HeatHazeManager>()},
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [2]
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void init_heat_haze_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_heat_haze_es(evt
        , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__manager", HeatHazeManager)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__lod", int)
    , ECS_RO_COMP(init_heat_haze_es_comps, "dafg_camera_registrator__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_heat_haze_es_es_desc
(
  "init_heat_haze_es",
  "prog/daNetGameLibs/heat_haze/render/heatHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_heat_haze_es_all_events),
  make_span(init_heat_haze_es_comps+0, 2)/*rw*/,
  make_span(init_heat_haze_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsReady,
                       SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc heat_haze_view_nodes_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("heat_haze__manager"), ecs::ComponentTypeInfo<HeatHazeManager>()},
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()}
};
static void heat_haze_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<OnCameraPerViewNodeConstruction>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      heat_haze_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
            , ECS_RO_COMP(heat_haze_view_nodes_es_comps, "heat_haze__manager", HeatHazeManager)
      , ECS_RO_COMP(heat_haze_view_nodes_es_comps, "heat_haze__lod", int)
      );
    while (++comp != compE);
  } else if (evt.is<OnCameraMainViewNodeConstruction>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      heat_haze_view_nodes_es(static_cast<const OnCameraMainViewNodeConstruction&>(evt)
            , ECS_RO_COMP(heat_haze_view_nodes_es_comps, "heat_haze__manager", HeatHazeManager)
      );
    while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <%s> in heat_haze_view_nodes_es", evt.getName());}
}
static ecs::EntitySystemDesc heat_haze_view_nodes_es_es_desc
(
  "heat_haze_view_nodes_es",
  "prog/daNetGameLibs/heat_haze/render/heatHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, heat_haze_view_nodes_es_all_events),
  empty_span(),
  make_span(heat_haze_view_nodes_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraMainViewNodeConstruction,
                       OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
