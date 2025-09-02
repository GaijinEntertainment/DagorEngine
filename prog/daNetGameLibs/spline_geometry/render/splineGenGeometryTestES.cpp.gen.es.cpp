// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "splineGenGeometryTestES.cpp.inl"
ECS_DEF_PULL_VAR(splineGenGeometryTest);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc spline_gen_test_make_points_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("spline_gen_geometry__points"), ecs::ComponentTypeInfo<ecs::List<Point3>>()},
  {ECS_HASH("spline_gen_geometry__request_active"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [3]
  {ECS_HASH("splineGenTest"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void spline_gen_test_make_points_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    spline_gen_test_make_points_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(spline_gen_test_make_points_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(spline_gen_test_make_points_es_comps, "spline_gen_geometry__points", ecs::List<Point3>)
    , ECS_RW_COMP(spline_gen_test_make_points_es_comps, "spline_gen_geometry__request_active", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_test_make_points_es_es_desc
(
  "spline_gen_test_make_points_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryTestES.cpp.inl",
  ecs::EntitySystemOps(spline_gen_test_make_points_es_all),
  make_span(spline_gen_test_make_points_es_comps+0, 2)/*rw*/,
  make_span(spline_gen_test_make_points_es_comps+2, 1)/*ro*/,
  make_span(spline_gen_test_make_points_es_comps+3, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"dev,render",nullptr,"*");
