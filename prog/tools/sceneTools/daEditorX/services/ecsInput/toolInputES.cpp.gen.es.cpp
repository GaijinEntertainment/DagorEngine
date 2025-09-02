// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "toolInputES.cpp.inl"
ECS_DEF_PULL_VAR(toolInput);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc key_board_input_es_comps[] =
{
//start of 9 rw components at [0]
  {ECS_HASH("input__deltaTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("input__verticalAxis"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("input__horizontalAxis"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("input__mousePosition"), ecs::ComponentTypeInfo<IPoint2>()},
  {ECS_HASH("input__mouseDelta"), ecs::ComponentTypeInfo<IPoint2>()},
  {ECS_HASH("input__mouseLeftButtonPressed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("input__mouseRightButtonPressed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("input__mouseMiddleButtonPressed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("input__keyAccelerationPressed"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [9]
  {ECS_HASH("input__keyboardSmoothFactor"), ecs::ComponentTypeInfo<float>()}
};
static void key_board_input_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    key_board_input_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(key_board_input_es_comps, "input__keyboardSmoothFactor", float)
    , ECS_RW_COMP(key_board_input_es_comps, "input__deltaTime", float)
    , ECS_RW_COMP(key_board_input_es_comps, "input__verticalAxis", float)
    , ECS_RW_COMP(key_board_input_es_comps, "input__horizontalAxis", float)
    , ECS_RW_COMP(key_board_input_es_comps, "input__mousePosition", IPoint2)
    , ECS_RW_COMP(key_board_input_es_comps, "input__mouseDelta", IPoint2)
    , ECS_RW_COMP(key_board_input_es_comps, "input__mouseLeftButtonPressed", bool)
    , ECS_RW_COMP(key_board_input_es_comps, "input__mouseRightButtonPressed", bool)
    , ECS_RW_COMP(key_board_input_es_comps, "input__mouseMiddleButtonPressed", bool)
    , ECS_RW_COMP(key_board_input_es_comps, "input__keyAccelerationPressed", bool)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc key_board_input_es_es_desc
(
  "key_board_input_es",
  "prog/tools/sceneTools/daEditorX/services/ecsInput/toolInputES.cpp.inl",
  ecs::EntitySystemOps(key_board_input_es_all),
  make_span(key_board_input_es_comps+0, 9)/*rw*/,
  make_span(key_board_input_es_comps+9, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"tools",nullptr,"*");
static constexpr ecs::ComponentDesc on_mouse_wheel_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("input__mouseWheel"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("input__mouseApproachWheel"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("input__mouseWheelDelta"), ecs::ComponentTypeInfo<float>()},
//start of 2 ro components at [3]
  {ECS_HASH("input__deltaTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("input__wheelSmoothFactor"), ecs::ComponentTypeInfo<float>()}
};
static void on_mouse_wheel_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnMouseWheel>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_mouse_wheel_es(static_cast<const OnMouseWheel&>(evt)
        , ECS_RO_COMP(on_mouse_wheel_es_comps, "input__deltaTime", float)
    , ECS_RO_COMP(on_mouse_wheel_es_comps, "input__wheelSmoothFactor", float)
    , ECS_RW_COMP(on_mouse_wheel_es_comps, "input__mouseWheel", int)
    , ECS_RW_COMP(on_mouse_wheel_es_comps, "input__mouseApproachWheel", float)
    , ECS_RW_COMP(on_mouse_wheel_es_comps, "input__mouseWheelDelta", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_mouse_wheel_es_es_desc
(
  "on_mouse_wheel_es",
  "prog/tools/sceneTools/daEditorX/services/ecsInput/toolInputES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_mouse_wheel_es_all_events),
  make_span(on_mouse_wheel_es_comps+0, 3)/*rw*/,
  make_span(on_mouse_wheel_es_comps+3, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnMouseWheel>::build(),
  0
,"tools");
