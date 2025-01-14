#include "footLockerES.cpp.inl"
ECS_DEF_PULL_VAR(footLocker);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc motion_matching_update_anim_tree_foot_locker_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()}
};
static void motion_matching_update_anim_tree_foot_locker_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    motion_matching_update_anim_tree_foot_locker_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(motion_matching_update_anim_tree_foot_locker_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(motion_matching_update_anim_tree_foot_locker_es_comps, "motion_matching__controller", MotionMatchingController)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc motion_matching_update_anim_tree_foot_locker_es_es_desc
(
  "motion_matching_update_anim_tree_foot_locker_es",
  "prog/daNetGameLibs/motion_matching/es/footLockerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_matching_update_anim_tree_foot_locker_es_all_events),
  make_span(motion_matching_update_anim_tree_foot_locker_es_comps+0, 1)/*rw*/,
  make_span(motion_matching_update_anim_tree_foot_locker_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"animchar_es","wait_motion_matching_job_es");
