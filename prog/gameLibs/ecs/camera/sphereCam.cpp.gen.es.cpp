#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t camera__lookDir_get_type();
static ecs::LTComponentList camera__lookDir_component(ECS_HASH("camera__lookDir"), camera__lookDir_get_type(), "prog/gameLibs/ecs/camera/sphereCam.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "sphereCam.cpp.inl"
ECS_DEF_PULL_VAR(sphereCam);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc sphere_cam_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("sphere_cam"), ecs::ComponentTypeInfo<SphereCamera>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 ro components at [2]
  {ECS_HASH("camera__target"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("sphere_cam__look_at"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL}
};
static void sphere_cam_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    sphere_cam_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    , ECS_RW_COMP(sphere_cam_es_comps, "sphere_cam", SphereCamera)
    , ECS_RO_COMP(sphere_cam_es_comps, "camera__target", ecs::EntityId)
    , ECS_RW_COMP(sphere_cam_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(sphere_cam_es_comps, "sphere_cam__look_at", Point3(Point3(0, 0, 0)))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sphere_cam_es_es_desc
(
  "sphere_cam_es",
  "prog/gameLibs/ecs/camera/sphereCam.cpp.inl",
  ecs::EntitySystemOps(sphere_cam_es_all),
  make_span(sphere_cam_es_comps+0, 2)/*rw*/,
  make_span(sphere_cam_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc sphere_cam_input_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("sphere_cam"), ecs::ComponentTypeInfo<SphereCamera>()}
};
static void sphere_cam_input_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventDaInputActionTriggered>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sphere_cam_input_es_event_handler(static_cast<const EventDaInputActionTriggered&>(evt)
        , ECS_RW_COMP(sphere_cam_input_es_event_handler_comps, "sphere_cam", SphereCamera)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sphere_cam_input_es_event_handler_es_desc
(
  "sphere_cam_input_es",
  "prog/gameLibs/ecs/camera/sphereCam.cpp.inl",
  ecs::EntitySystemOps(nullptr, sphere_cam_input_es_event_handler_all_events),
  make_span(sphere_cam_input_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventDaInputActionTriggered>::build(),
  0
,"render");
static constexpr ecs::component_t camera__lookDir_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
