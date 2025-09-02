#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t camera_animator__angle_get_type();
static ecs::LTComponentList camera_animator__angle_component(ECS_HASH("camera_animator__angle"), camera_animator__angle_get_type(), "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl", "", 0);
static constexpr ecs::component_t camera_animator__origo_get_type();
static ecs::LTComponentList camera_animator__origo_component(ECS_HASH("camera_animator__origo"), camera_animator__origo_get_type(), "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl", "", 0);
static constexpr ecs::component_t camera_animator__speed_get_type();
static ecs::LTComponentList camera_animator__speed_component(ECS_HASH("camera_animator__speed"), camera_animator__speed_get_type(), "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl", "", 0);
static constexpr ecs::component_t camera_path_animator__distance_get_type();
static ecs::LTComponentList camera_path_animator__distance_component(ECS_HASH("camera_path_animator__distance"), camera_path_animator__distance_get_type(), "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl", "", 0);
static constexpr ecs::component_t camera_path_animator__transforms_get_type();
static ecs::LTComponentList camera_path_animator__transforms_component(ECS_HASH("camera_path_animator__transforms"), camera_path_animator__transforms_get_type(), "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animateCameraConsoleES.cpp.inl"
ECS_DEF_PULL_VAR(animateCameraConsole);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc camera_animator_update_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("camera_animator__angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera_animator__prev_transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("camera_animator__origo"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera_animator__speed"), ecs::ComponentTypeInfo<float>()}
};
static void camera_animator_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    camera_animator_update_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(camera_animator_update_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(camera_animator_update_es_comps, "camera_animator__origo", Point3)
    , ECS_RO_COMP(camera_animator_update_es_comps, "camera_animator__speed", float)
    , ECS_RW_COMP(camera_animator_update_es_comps, "camera_animator__angle", float)
    , ECS_RW_COMP(camera_animator_update_es_comps, "camera_animator__prev_transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camera_animator_update_es_es_desc
(
  "camera_animator_update_es",
  "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl",
  ecs::EntitySystemOps(camera_animator_update_es_all),
  make_span(camera_animator_update_es_comps+0, 2)/*rw*/,
  make_span(camera_animator_update_es_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc camera_path_animator_update_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("camera_path_animator__progress"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera_animator__prev_transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 4 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("camera_path_animator__transforms"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("camera_animator__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera_path_animator__distance"), ecs::ComponentTypeInfo<float>()}
};
static void camera_path_animator_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    camera_path_animator_update_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(camera_path_animator_update_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(camera_path_animator_update_es_comps, "camera_path_animator__transforms", ecs::Array)
    , ECS_RO_COMP(camera_path_animator_update_es_comps, "camera_animator__speed", float)
    , ECS_RO_COMP(camera_path_animator_update_es_comps, "camera_path_animator__distance", float)
    , ECS_RW_COMP(camera_path_animator_update_es_comps, "camera_path_animator__progress", float)
    , ECS_RW_COMP(camera_path_animator_update_es_comps, "camera_animator__prev_transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camera_path_animator_update_es_es_desc
(
  "camera_path_animator_update_es",
  "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl",
  ecs::EntitySystemOps(camera_path_animator_update_es_all),
  make_span(camera_path_animator_update_es_comps+0, 2)/*rw*/,
  make_span(camera_path_animator_update_es_comps+2, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc camera_animator_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("camera_animator__prev_transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void camera_animator_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    camera_animator_es_event_handler(evt
        , ECS_RW_COMP(camera_animator_es_event_handler_comps, "camera_animator__prev_transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camera_animator_es_event_handler_es_desc
(
  "camera_animator_es",
  "prog/daNetGame/camera/animateCameraConsoleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, camera_animator_es_event_handler_all_events),
  make_span(camera_animator_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::component_t camera_animator__angle_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t camera_animator__origo_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t camera_animator__speed_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t camera_path_animator__distance_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t camera_path_animator__transforms_get_type(){return ecs::ComponentTypeInfo<ecs::Array>::type; }
