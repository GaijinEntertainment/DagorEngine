#include "timelapseScreenerES.cpp.inl"
ECS_DEF_PULL_VAR(timelapseScreener);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc timelapse_screener_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("timelapse_screener__curTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("timelapse_screener__curWaitTimer"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("timelapse_screener__sequenceNum"), ecs::ComponentTypeInfo<int>()},
//start of 4 ro components at [3]
  {ECS_HASH("timelapse_screener__toTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("timelapse_screener__timeStep"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("timelapse_screener__waitTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("timelapse_screener__deltaPos"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL}
};
static void timelapse_screener_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    timelapse_screener_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(timelapse_screener_es_comps, "timelapse_screener__curTime", float)
    , ECS_RW_COMP(timelapse_screener_es_comps, "timelapse_screener__curWaitTimer", float)
    , ECS_RW_COMP(timelapse_screener_es_comps, "timelapse_screener__sequenceNum", int)
    , ECS_RO_COMP(timelapse_screener_es_comps, "timelapse_screener__toTime", float)
    , ECS_RO_COMP(timelapse_screener_es_comps, "timelapse_screener__timeStep", float)
    , ECS_RO_COMP(timelapse_screener_es_comps, "timelapse_screener__waitTime", float)
    , ECS_RO_COMP_OR(timelapse_screener_es_comps, "timelapse_screener__deltaPos", Point3(Point3(0.f, 0.f, 0.f)))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc timelapse_screener_es_es_desc
(
  "timelapse_screener_es",
  "prog/daNetGameLibs/timelapse_screener/render/timelapseScreenerES.cpp.inl",
  ecs::EntitySystemOps(timelapse_screener_es_all),
  make_span(timelapse_screener_es_comps+0, 3)/*rw*/,
  make_span(timelapse_screener_es_comps+3, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render");
static constexpr ecs::ComponentDesc query_camera_pos_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 ro components at [1]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc query_camera_pos_ecs_query_desc
(
  "query_camera_pos_ecs_query",
  make_span(query_camera_pos_ecs_query_comps+0, 1)/*rw*/,
  make_span(query_camera_pos_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_camera_pos_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_camera_pos_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(query_camera_pos_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RW_COMP(query_camera_pos_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
      }
  );
}
