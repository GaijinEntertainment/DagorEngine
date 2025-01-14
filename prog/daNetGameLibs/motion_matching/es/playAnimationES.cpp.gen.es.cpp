#include "playAnimationES.cpp.inl"
ECS_DEF_PULL_VAR(playAnimation);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc mm_update_root_orientation_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("mm_trajectory__linearVelocity"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("mm_trajectory__angularVelocity"), ecs::ComponentTypeInfo<Point3>()}
};
static void mm_update_root_orientation_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mm_update_root_orientation_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RO_COMP(mm_update_root_orientation_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(mm_update_root_orientation_es_comps, "mm_trajectory__linearVelocity", Point3)
    , ECS_RO_COMP(mm_update_root_orientation_es_comps, "mm_trajectory__angularVelocity", Point3)
    , ECS_RW_COMP(mm_update_root_orientation_es_comps, "motion_matching__controller", MotionMatchingController)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mm_update_root_orientation_es_es_desc
(
  "mm_update_root_orientation_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mm_update_root_orientation_es_all_events),
  make_span(mm_update_root_orientation_es_comps+0, 1)/*rw*/,
  make_span(mm_update_root_orientation_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"mm_update_goal_features_es","mm_trajectory_update");
static constexpr ecs::ComponentDesc mm_calculate_root_offset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
//start of 2 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void mm_calculate_root_offset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mm_calculate_root_offset_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RO_COMP(mm_calculate_root_offset_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(mm_calculate_root_offset_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RO_COMP_OR(mm_calculate_root_offset_es_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mm_calculate_root_offset_es_es_desc
(
  "mm_calculate_root_offset_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mm_calculate_root_offset_es_all_events),
  make_span(mm_calculate_root_offset_es_comps+0, 1)/*rw*/,
  make_span(mm_calculate_root_offset_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"animchar_es","after_guns_update_sync,wait_motion_matching_job_es");
static constexpr ecs::ComponentDesc mm_update_goal_features_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_matching__goalFeature"), ecs::ComponentTypeInfo<FrameFeatures>()},
//start of 6 ro components at [1]
  {ECS_HASH("motion_matching__goalNodesIdx"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("mm_trajectory__featurePositions"), ecs::ComponentTypeInfo<ecs::Point3List>()},
  {ECS_HASH("mm_trajectory__featureDirections"), ecs::ComponentTypeInfo<ecs::Point3List>()}
};
static void mm_update_goal_features_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mm_update_goal_features_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(mm_update_goal_features_es_comps, "motion_matching__goalFeature", FrameFeatures)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "motion_matching__goalNodesIdx", ecs::IntList)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "motion_matching__enabled", bool)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "mm_trajectory__featurePositions", ecs::Point3List)
    , ECS_RO_COMP(mm_update_goal_features_es_comps, "mm_trajectory__featureDirections", ecs::Point3List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mm_update_goal_features_es_es_desc
(
  "mm_update_goal_features_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mm_update_goal_features_es_all_events),
  make_span(mm_update_goal_features_es_comps+0, 1)/*rw*/,
  make_span(mm_update_goal_features_es_comps+1, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"motion_matching_job_es");
static constexpr ecs::ComponentDesc update_tag_changes_es_comps[] =
{
//start of 7 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__presetIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("motion_matching__presetBlendTimeLeft"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("mm_trajectory__linearVelocityViscosity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("mm_trajectory__angularVelocityViscosity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void update_tag_changes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_tag_changes_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(update_tag_changes_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RW_COMP(update_tag_changes_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(update_tag_changes_es_comps, "motion_matching__presetIdx", int)
    , ECS_RW_COMP(update_tag_changes_es_comps, "motion_matching__presetBlendTimeLeft", float)
    , ECS_RW_COMP(update_tag_changes_es_comps, "mm_trajectory__linearVelocityViscosity", float)
    , ECS_RW_COMP(update_tag_changes_es_comps, "mm_trajectory__angularVelocityViscosity", float)
    , ECS_RW_COMP(update_tag_changes_es_comps, "motion_matching__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_tag_changes_es_es_desc
(
  "update_tag_changes_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_tag_changes_es_all_events),
  make_span(update_tag_changes_es_comps+0, 7)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"motion_matching_job_es");
//static constexpr ecs::ComponentDesc motion_matching_job_es_comps[] ={};
static void motion_matching_job_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  motion_matching_job_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc motion_matching_job_es_es_desc
(
  "motion_matching_job_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_matching_job_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"before_net_phys_sync");
static constexpr ecs::ComponentDesc wait_motion_matching_job_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("main_database__perFrameLimit"), ecs::ComponentTypeInfo<int>()}
};
static void wait_motion_matching_job_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    wait_motion_matching_job_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RO_COMP(wait_motion_matching_job_es_comps, "main_database__perFrameLimit", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc wait_motion_matching_job_es_es_desc
(
  "wait_motion_matching_job_es",
  "prog/daNetGameLibs/motion_matching/es/playAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, wait_motion_matching_job_es_all_events),
  empty_span(),
  make_span(wait_motion_matching_job_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"animchar_es","after_net_phys_sync");
static constexpr ecs::ComponentDesc motion_matching_ecs_query_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("motion_matching__updateProgress"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__metricaTolerance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__presetBlendTimeLeft"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__goalFeature"), ecs::ComponentTypeInfo<FrameFeatures>()},
//start of 5 ro components at [5]
  {ECS_HASH("motion_matching__blendTimeToAnimtree"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__distanceFactor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__presetIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("animchar__visible"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc motion_matching_ecs_query_desc
(
  "motion_matching_ecs_query",
  make_span(motion_matching_ecs_query_comps+0, 5)/*rw*/,
  make_span(motion_matching_ecs_query_comps+5, 5)/*ro*/,
  empty_span(),
  empty_span()
  , 1);
template<typename Callable>
inline void motion_matching_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, motion_matching_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(motion_matching_ecs_query_comps, "animchar__visible", bool)) )
            continue;
          function(
              ECS_RW_COMP(motion_matching_ecs_query_comps, "motion_matching__controller", MotionMatchingController)
            , ECS_RW_COMP(motion_matching_ecs_query_comps, "motion_matching__updateProgress", float)
            , ECS_RW_COMP(motion_matching_ecs_query_comps, "motion_matching__metricaTolerance", float)
            , ECS_RW_COMP(motion_matching_ecs_query_comps, "motion_matching__presetBlendTimeLeft", float)
            , ECS_RO_COMP(motion_matching_ecs_query_comps, "motion_matching__blendTimeToAnimtree", float)
            , ECS_RO_COMP(motion_matching_ecs_query_comps, "motion_matching__distanceFactor", float)
            , ECS_RW_COMP(motion_matching_ecs_query_comps, "motion_matching__goalFeature", FrameFeatures)
            , ECS_RO_COMP(motion_matching_ecs_query_comps, "motion_matching__presetIdx", int)
            , ECS_RO_COMP(motion_matching_ecs_query_comps, "motion_matching__enabled", bool)
            );

        }while (++comp != compE);
      }
    , nullptr, motion_matching_ecs_query_desc.getQuant());
}
static constexpr ecs::ComponentDesc motion_matching_per_frame_limit_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("main_database__perFrameLimit"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc motion_matching_per_frame_limit_ecs_query_desc
(
  "motion_matching_per_frame_limit_ecs_query",
  empty_span(),
  make_span(motion_matching_per_frame_limit_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void motion_matching_per_frame_limit_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, motion_matching_per_frame_limit_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(motion_matching_per_frame_limit_ecs_query_comps, "main_database__perFrameLimit", int)
            );

        }while (++comp != compE);
    }
  );
}
