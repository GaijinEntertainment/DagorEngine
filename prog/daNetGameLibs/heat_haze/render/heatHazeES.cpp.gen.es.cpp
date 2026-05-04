// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "heatHazeES.cpp.inl"
ECS_DEF_PULL_VAR(heatHaze);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_heat_haze_es_comps[] =
{
//start of 6 rw components at [0]
  {ECS_HASH("heat_haze__manager"), ecs::ComponentTypeInfo<HeatHazeManager>()},
  {ECS_HASH("heat_haze__render_clear_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__render_before_particles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__render_particles_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__apply_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()}
};
static void init_heat_haze_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_heat_haze_es(evt
        , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__manager", HeatHazeManager)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__render_clear_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__render_before_particles_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__render_particles_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__apply_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_heat_haze_es_comps, "heat_haze__lod", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_heat_haze_es_es_desc
(
  "init_heat_haze_es",
  "prog/daNetGameLibs/heat_haze/render/heatHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_heat_haze_es_all_events),
  make_span(init_heat_haze_es_comps+0, 6)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsReady,
                       SetResolutionEvent>::build(),
  0
,"render");
