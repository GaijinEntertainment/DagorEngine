#include "movingZoneHudInfoES.cpp.inl"
ECS_DEF_PULL_VAR(movingZoneHudInfo);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc moving_zone_hud_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("moving_zone_hud__zoneStartTimer"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("moving_zone_hud__zoneFinishTimer"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("moving_zone_hud__zoneProgress"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("moving_zone_hud__playerZonePosition"), ecs::ComponentTypeInfo<int>()},
//start of 6 ro components at [4]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("moving_zone__startEndTime"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("moving_zone__sourcePos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("moving_zone__targetPos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("moving_zone__sourceRadius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("moving_zone__targetRadius"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [10]
  {ECS_HASH("moving_zone_hud"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void moving_zone_hud_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    moving_zone_hud_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(moving_zone_hud_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(moving_zone_hud_es_comps, "moving_zone__startEndTime", Point2)
    , ECS_RO_COMP(moving_zone_hud_es_comps, "moving_zone__sourcePos", Point3)
    , ECS_RO_COMP(moving_zone_hud_es_comps, "moving_zone__targetPos", Point3)
    , ECS_RO_COMP(moving_zone_hud_es_comps, "moving_zone__sourceRadius", float)
    , ECS_RO_COMP(moving_zone_hud_es_comps, "moving_zone__targetRadius", float)
    , ECS_RW_COMP(moving_zone_hud_es_comps, "moving_zone_hud__zoneStartTimer", int)
    , ECS_RW_COMP(moving_zone_hud_es_comps, "moving_zone_hud__zoneFinishTimer", int)
    , ECS_RW_COMP(moving_zone_hud_es_comps, "moving_zone_hud__zoneProgress", int)
    , ECS_RW_COMP(moving_zone_hud_es_comps, "moving_zone_hud__playerZonePosition", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc moving_zone_hud_es_es_desc
(
  "moving_zone_hud_es",
  "prog/daNetGameLibs/moving_zone_hud/render/movingZoneHudInfoES.cpp.inl",
  ecs::EntitySystemOps(moving_zone_hud_es_all),
  make_span(moving_zone_hud_es_comps+0, 4)/*rw*/,
  make_span(moving_zone_hud_es_comps+4, 6)/*ro*/,
  make_span(moving_zone_hud_es_comps+10, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"moving_zone");
