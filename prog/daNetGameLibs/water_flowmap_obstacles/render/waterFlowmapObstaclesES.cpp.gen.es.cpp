#include "waterFlowmapObstaclesES.cpp.inl"
ECS_DEF_PULL_VAR(waterFlowmapObstacles);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc attempt_to_enable_water_flowmap_obstacles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void attempt_to_enable_water_flowmap_obstacles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    attempt_to_enable_water_flowmap_obstacles_es(evt
        , ECS_RW_COMP(attempt_to_enable_water_flowmap_obstacles_es_comps, "water", FFTWater)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attempt_to_enable_water_flowmap_obstacles_es_es_desc
(
  "attempt_to_enable_water_flowmap_obstacles_es",
  "prog/daNetGameLibs/water_flowmap_obstacles/render/waterFlowmapObstaclesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attempt_to_enable_water_flowmap_obstacles_es_all_events),
  make_span(attempt_to_enable_water_flowmap_obstacles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_water_flowmap_obstacles_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void disable_water_flowmap_obstacles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  disable_water_flowmap_obstacles_es(evt
        );
}
static ecs::EntitySystemDesc disable_water_flowmap_obstacles_es_es_desc
(
  "disable_water_flowmap_obstacles_es",
  "prog/daNetGameLibs/water_flowmap_obstacles/render/waterFlowmapObstaclesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_water_flowmap_obstacles_es_all_events),
  empty_span(),
  empty_span(),
  make_span(disable_water_flowmap_obstacles_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_water_flowmap_obstacles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void update_water_flowmap_obstacles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_water_flowmap_obstacles_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(update_water_flowmap_obstacles_es_comps, "water", FFTWater)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_water_flowmap_obstacles_es_es_desc
(
  "update_water_flowmap_obstacles_es",
  "prog/daNetGameLibs/water_flowmap_obstacles/render/waterFlowmapObstaclesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_water_flowmap_obstacles_es_all_events),
  make_span(update_water_flowmap_obstacles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
