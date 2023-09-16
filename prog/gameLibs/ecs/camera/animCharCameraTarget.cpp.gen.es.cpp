#include "animCharCameraTarget.cpp.inl"
ECS_DEF_PULL_VAR(animCharCameraTarget);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_cam_target_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camera__look_at"), ecs::ComponentTypeInfo<DPoint3>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_camera_target__nodeIndex"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>()},
//start of 2 no components at [4]
  {ECS_HASH("animchar_camera_target__node_offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_cam_target_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(animchar_cam_target_es_comps, "animchar__updatable", bool)) )
      continue;
    animchar_cam_target_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(animchar_cam_target_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_cam_target_es_comps, "animchar_camera_target__nodeIndex", int)
    , ECS_RW_COMP(animchar_cam_target_es_comps, "camera__look_at", DPoint3)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_cam_target_es_es_desc
(
  "animchar_cam_target_es",
  "prog/gameLibs/ecs/camera/animCharCameraTarget.cpp.inl",
  ecs::EntitySystemOps(animchar_cam_target_es_all),
  make_span(animchar_cam_target_es_comps+0, 1)/*rw*/,
  make_span(animchar_cam_target_es_comps+1, 3)/*ro*/,
  empty_span(),
  make_span(animchar_cam_target_es_comps+4, 2)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"after_animchar_update_sync");
static constexpr ecs::ComponentDesc animchar_cam_target_with_offset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camera__look_at"), ecs::ComponentTypeInfo<DPoint3>()},
//start of 4 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_camera_target__nodeIndex"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("animchar_camera_target__node_offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [5]
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_cam_target_with_offset_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(animchar_cam_target_with_offset_es_comps, "animchar__updatable", bool)) )
      continue;
    animchar_cam_target_with_offset_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(animchar_cam_target_with_offset_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_cam_target_with_offset_es_comps, "animchar_camera_target__nodeIndex", int)
    , ECS_RO_COMP(animchar_cam_target_with_offset_es_comps, "animchar_camera_target__node_offset", Point3)
    , ECS_RW_COMP(animchar_cam_target_with_offset_es_comps, "camera__look_at", DPoint3)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_cam_target_with_offset_es_es_desc
(
  "animchar_cam_target_with_offset_es",
  "prog/gameLibs/ecs/camera/animCharCameraTarget.cpp.inl",
  ecs::EntitySystemOps(animchar_cam_target_with_offset_es_all),
  make_span(animchar_cam_target_with_offset_es_comps+0, 1)/*rw*/,
  make_span(animchar_cam_target_with_offset_es_comps+1, 4)/*ro*/,
  empty_span(),
  make_span(animchar_cam_target_with_offset_es_comps+5, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"after_animchar_update_sync");
static constexpr ecs::ComponentDesc animchar_cam_target_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_camera_target__nodeIndex"), ecs::ComponentTypeInfo<int>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_camera_target__node_name"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void animchar_cam_target_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_cam_target_init_es_event_handler(evt
        , ECS_RO_COMP(animchar_cam_target_init_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP_PTR(animchar_cam_target_init_es_event_handler_comps, "animchar_camera_target__node_name", ecs::string)
    , ECS_RW_COMP(animchar_cam_target_init_es_event_handler_comps, "animchar_camera_target__nodeIndex", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_cam_target_init_es_event_handler_es_desc
(
  "animchar_cam_target_init_es",
  "prog/gameLibs/ecs/camera/animCharCameraTarget.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_cam_target_init_es_event_handler_all_events),
  make_span(animchar_cam_target_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_cam_target_init_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
