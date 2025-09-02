#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_get_type();
static ecs::LTComponentList animchar_component(ECS_HASH("animchar"), animchar_get_type(), "prog/gameLibs/ecs/camera/nodeCam.cpp.inl", "node_cam_target_changed_es_event_handler", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "nodeCam.cpp.inl"
ECS_DEF_PULL_VAR(nodeCam);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc node_cam_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("node_cam"), ecs::ComponentTypeInfo<NodeCamera>()},
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("node_cam__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("node_cam__angles"), ecs::ComponentTypeInfo<Point3>()},
//start of 1 ro components at [5]
  {ECS_HASH("camera__target"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void node_cam_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    node_cam_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(node_cam_es_comps, "node_cam", NodeCamera)
    , ECS_RW_COMP(node_cam_es_comps, "fov", float)
    , ECS_RW_COMP(node_cam_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(node_cam_es_comps, "node_cam__offset", Point3)
    , ECS_RW_COMP(node_cam_es_comps, "node_cam__angles", Point3)
    , ECS_RO_COMP(node_cam_es_comps, "camera__target", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc node_cam_es_es_desc
(
  "node_cam_es",
  "prog/gameLibs/ecs/camera/nodeCam.cpp.inl",
  ecs::EntitySystemOps(node_cam_es_all),
  make_span(node_cam_es_comps+0, 5)/*rw*/,
  make_span(node_cam_es_comps+5, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc node_cam_target_changed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("node_cam"), ecs::ComponentTypeInfo<NodeCamera>()},
//start of 2 ro components at [1]
  {ECS_HASH("node_cam__node"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("camera__target"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void node_cam_target_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    node_cam_target_changed_es_event_handler(evt
        , ECS_RW_COMP(node_cam_target_changed_es_event_handler_comps, "node_cam", NodeCamera)
    , ECS_RO_COMP(node_cam_target_changed_es_event_handler_comps, "node_cam__node", ecs::string)
    , ECS_RO_COMP(node_cam_target_changed_es_event_handler_comps, "camera__target", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc node_cam_target_changed_es_event_handler_es_desc
(
  "node_cam_target_changed_es",
  "prog/gameLibs/ecs/camera/nodeCam.cpp.inl",
  ecs::EntitySystemOps(nullptr, node_cam_target_changed_es_event_handler_all_events),
  make_span(node_cam_target_changed_es_event_handler_comps+0, 1)/*rw*/,
  make_span(node_cam_target_changed_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","camera__target");
static constexpr ecs::ComponentDesc node_cam_input_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("node_cam"), ecs::ComponentTypeInfo<NodeCamera>()},
  {ECS_HASH("node_cam__angles"), ecs::ComponentTypeInfo<Point3>()},
//start of 2 ro components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("node_cam__locked"), ecs::ComponentTypeInfo<bool>()}
};
static void node_cam_input_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventDaInputActionTriggered>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RO_COMP(node_cam_input_es_event_handler_comps, "node_cam__locked", bool)) )
      continue;
    node_cam_input_es_event_handler(static_cast<const EventDaInputActionTriggered&>(evt)
          , ECS_RW_COMP(node_cam_input_es_event_handler_comps, "node_cam", NodeCamera)
      , ECS_RO_COMP(node_cam_input_es_event_handler_comps, "transform", TMatrix)
      , ECS_RW_COMP(node_cam_input_es_event_handler_comps, "node_cam__angles", Point3)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc node_cam_input_es_event_handler_es_desc
(
  "node_cam_input_es",
  "prog/gameLibs/ecs/camera/nodeCam.cpp.inl",
  ecs::EntitySystemOps(nullptr, node_cam_input_es_event_handler_all_events),
  make_span(node_cam_input_es_event_handler_comps+0, 2)/*rw*/,
  make_span(node_cam_input_es_event_handler_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventDaInputActionTriggered>::build(),
  0
,"input");
static constexpr ecs::component_t animchar_get_type(){return ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>::type; }
