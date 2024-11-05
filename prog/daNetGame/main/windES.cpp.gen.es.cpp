#include "windES.cpp.inl"
ECS_DEF_PULL_VAR(wind);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc wind_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ambient_wind"), ecs::ComponentTypeInfo<AmbientWind>()},
//start of 1 ro components at [1]
  {ECS_HASH("wind__dir"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void wind_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    wind_update_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(wind_update_es_comps, "ambient_wind", AmbientWind)
    , ECS_RO_COMP_OR(wind_update_es_comps, "wind__dir", float(0))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc wind_update_es_es_desc
(
  "wind_update_es",
  "prog/daNetGame/main/windES.cpp.inl",
  ecs::EntitySystemOps(wind_update_es_all),
  make_span(wind_update_es_comps+0, 1)/*rw*/,
  make_span(wind_update_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc wind_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ambient_wind"), ecs::ComponentTypeInfo<AmbientWind>()},
//start of 8 ro components at [1]
  {ECS_HASH("wind__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__noiseStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__noiseSpeed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__noiseScale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__noisePerpendicular"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wind__dir"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("wind__left_top_right_bottom"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("wind__flowMap"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void wind_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    wind_es_event_handler(evt
        , ECS_RW_COMP(wind_es_event_handler_comps, "ambient_wind", AmbientWind)
    , ECS_RO_COMP(wind_es_event_handler_comps, "wind__strength", float)
    , ECS_RO_COMP(wind_es_event_handler_comps, "wind__noiseStrength", float)
    , ECS_RO_COMP(wind_es_event_handler_comps, "wind__noiseSpeed", float)
    , ECS_RO_COMP(wind_es_event_handler_comps, "wind__noiseScale", float)
    , ECS_RO_COMP(wind_es_event_handler_comps, "wind__noisePerpendicular", float)
    , ECS_RO_COMP_OR(wind_es_event_handler_comps, "wind__dir", float(0))
    , ECS_RO_COMP_OR(wind_es_event_handler_comps, "wind__left_top_right_bottom", Point4(Point4(-2048, -2048, 2048, 2048)))
    , ECS_RO_COMP_OR(wind_es_event_handler_comps, "wind__flowMap", ecs::string(""))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc wind_es_event_handler_es_desc
(
  "wind_es",
  "prog/daNetGame/main/windES.cpp.inl",
  ecs::EntitySystemOps(nullptr, wind_es_event_handler_all_events),
  make_span(wind_es_event_handler_comps+0, 1)/*rw*/,
  make_span(wind_es_event_handler_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"wind__dir,wind__flowMap,wind__left_top_right_bottom,wind__noisePerpendicular,wind__noiseScale,wind__noiseSpeed,wind__noiseStrength,wind__strength","*");
static constexpr ecs::ComponentDesc destroy_wind_managed_resources_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ambient_wind"), ecs::ComponentTypeInfo<AmbientWind>()},
//start of 1 rq components at [1]
  {ECS_HASH("wind__strength"), ecs::ComponentTypeInfo<float>()}
};
static void destroy_wind_managed_resources_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destroy_wind_managed_resources_es_event_handler(evt
        , ECS_RW_COMP(destroy_wind_managed_resources_es_event_handler_comps, "ambient_wind", AmbientWind)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destroy_wind_managed_resources_es_event_handler_es_desc
(
  "destroy_wind_managed_resources_es",
  "prog/daNetGame/main/windES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_wind_managed_resources_es_event_handler_all_events),
  make_span(destroy_wind_managed_resources_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(destroy_wind_managed_resources_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
