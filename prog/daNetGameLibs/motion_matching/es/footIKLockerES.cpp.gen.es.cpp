#include "footIKLockerES.cpp.inl"
ECS_DEF_PULL_VAR(footIKLocker);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc mm_foot_ik_locker_invalidate_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("foot_ik_locker_inited"), ecs::ComponentTypeInfo<bool>()}
};
static void mm_foot_ik_locker_invalidate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<InvalidateAnimationDataBase>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mm_foot_ik_locker_invalidate_es(static_cast<const InvalidateAnimationDataBase&>(evt)
        , ECS_RW_COMP(mm_foot_ik_locker_invalidate_es_comps, "foot_ik_locker_inited", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mm_foot_ik_locker_invalidate_es_es_desc
(
  "mm_foot_ik_locker_invalidate_es",
  "prog/daNetGameLibs/motion_matching/es/footIKLockerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mm_foot_ik_locker_invalidate_es_all_events),
  make_span(mm_foot_ik_locker_invalidate_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateAnimationDataBase>::build(),
  0
,"render",nullptr,"init_animations_es");
static constexpr ecs::ComponentDesc motion_matching_apply_foot_ik_locker_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__dataBaseEid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void motion_matching_apply_foot_ik_locker_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    motion_matching_apply_foot_ik_locker_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(motion_matching_apply_foot_ik_locker_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RO_COMP(motion_matching_apply_foot_ik_locker_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(motion_matching_apply_foot_ik_locker_es_comps, "motion_matching__dataBaseEid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc motion_matching_apply_foot_ik_locker_es_es_desc
(
  "motion_matching_apply_foot_ik_locker_es",
  "prog/daNetGameLibs/motion_matching/es/footIKLockerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_matching_apply_foot_ik_locker_es_all_events),
  make_span(motion_matching_apply_foot_ik_locker_es_comps+0, 1)/*rw*/,
  make_span(motion_matching_apply_foot_ik_locker_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"animchar_es","wait_motion_matching_job_es");
static constexpr ecs::ComponentDesc motion_matching_update_anim_tree_foot_locker_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("motion_matching__animationBlendTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()}
};
static void motion_matching_update_anim_tree_foot_locker_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    motion_matching_update_anim_tree_foot_locker_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(motion_matching_update_anim_tree_foot_locker_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(motion_matching_update_anim_tree_foot_locker_es_comps, "motion_matching__animationBlendTime", float)
    , ECS_RO_COMP(motion_matching_update_anim_tree_foot_locker_es_comps, "motion_matching__controller", MotionMatchingController)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc motion_matching_update_anim_tree_foot_locker_es_es_desc
(
  "motion_matching_update_anim_tree_foot_locker_es",
  "prog/daNetGameLibs/motion_matching/es/footIKLockerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_matching_update_anim_tree_foot_locker_es_all_events),
  make_span(motion_matching_update_anim_tree_foot_locker_es_comps+0, 1)/*rw*/,
  make_span(motion_matching_update_anim_tree_foot_locker_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"animchar_es","wait_motion_matching_job_es");
static constexpr ecs::ComponentDesc foot_ik_locker_nodes_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("foot_ik_locker_nodes"), ecs::ComponentTypeInfo<FootIKLockerNodes>()},
  {ECS_HASH("foot_ik_locker_inited"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("foot_ik_locker_enabled"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [3]
  {ECS_HASH("foot_ik_locker_nodeNames"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static ecs::CompileTimeQueryDesc foot_ik_locker_nodes_ecs_query_desc
(
  "foot_ik_locker_nodes_ecs_query",
  make_span(foot_ik_locker_nodes_ecs_query_comps+0, 3)/*rw*/,
  make_span(foot_ik_locker_nodes_ecs_query_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void foot_ik_locker_nodes_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, foot_ik_locker_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(foot_ik_locker_nodes_ecs_query_comps, "foot_ik_locker_nodes", FootIKLockerNodes)
            , ECS_RO_COMP(foot_ik_locker_nodes_ecs_query_comps, "foot_ik_locker_nodeNames", ecs::Array)
            , ECS_RW_COMP(foot_ik_locker_nodes_ecs_query_comps, "foot_ik_locker_inited", bool)
            , ECS_RW_COMP(foot_ik_locker_nodes_ecs_query_comps, "foot_ik_locker_enabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc foot_ik_locker_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("foot_ik_locker_nodes"), ecs::ComponentTypeInfo<FootIKLockerNodes>()},
  {ECS_HASH("foot_ik_locker_blendTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foot_ik_locker_maxReachScale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foot_ik_locker_enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("foot_ik_locker_inited"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("foot_ik_locker_unlockByAnimRadius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foot_ik_locker_unlockWhenUnreachableRadius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("foot_ik_locker_toeNodeHeight"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc foot_ik_locker_ecs_query_desc
(
  "foot_ik_locker_ecs_query",
  empty_span(),
  make_span(foot_ik_locker_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void foot_ik_locker_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, foot_ik_locker_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_nodes", FootIKLockerNodes)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_blendTime", float)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_maxReachScale", float)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_enabled", bool)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_inited", bool)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_unlockByAnimRadius", float)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_unlockWhenUnreachableRadius", float)
            , ECS_RO_COMP(foot_ik_locker_ecs_query_comps, "foot_ik_locker_toeNodeHeight", float)
            );

        }
    }
  );
}
