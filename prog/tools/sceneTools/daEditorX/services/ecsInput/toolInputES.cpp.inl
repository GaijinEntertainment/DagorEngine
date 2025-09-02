// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>
#include <ecs/inputEvents.h>
#include <EditorCore/ec_input.h>
#include <math/dag_mathBase.h>

#include <imgui/imgui.h>

ECS_TAG(tools)
ECS_NO_ORDER
static __forceinline void key_board_input_es(const ecs::UpdateStageInfoAct &stage, float input__keyboardSmoothFactor,
  float &input__deltaTime, float &input__verticalAxis, float &input__horizontalAxis, IPoint2 &input__mousePosition,
  IPoint2 &input__mouseDelta, bool &input__mouseLeftButtonPressed, bool &input__mouseRightButtonPressed,
  bool &input__mouseMiddleButtonPressed, bool &input__keyAccelerationPressed, ecs::EntityManager &manager)
{
  input__deltaTime = stage.dt;
  input__verticalAxis = approach(input__verticalAxis, (float)(ec_is_key_down(ImGuiKey_W) - ec_is_key_down(ImGuiKey_S)), stage.dt,
    input__keyboardSmoothFactor);
  input__horizontalAxis = approach(input__horizontalAxis, (float)(ec_is_key_down(ImGuiKey_D) - ec_is_key_down(ImGuiKey_A)), stage.dt,
    input__keyboardSmoothFactor);

  input__keyAccelerationPressed = ec_is_shift_key_down();
  IPoint2 newMousePos = ec_get_cursor_pos();
  input__mouseDelta = newMousePos - input__mousePosition;
  input__mousePosition = newMousePos;

  bool leftButtonPressed = ec_is_key_down(ImGuiKey_MouseLeft);
  bool rightButtonPressed = ec_is_key_down(ImGuiKey_MouseRight);
  bool middleButtonPressed = ec_is_key_down(ImGuiKey_MouseMiddle);

  bool anyButtonDown = false;
  bool anyButtonUp = false;

  if (leftButtonPressed != input__mouseLeftButtonPressed)
  {
    input__mouseLeftButtonPressed = leftButtonPressed;
    if (leftButtonPressed)
    {
      manager.broadcastEvent(OnMouseDownLeft(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseDown(MouseButton::LeftButton, newMousePos.x, newMousePos.y));
    }
    else
    {
      manager.broadcastEvent(OnMouseUpLeft(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseUp(MouseButton::LeftButton, newMousePos.x, newMousePos.y));
    }
  }
  if (rightButtonPressed != input__mouseRightButtonPressed)
  {
    input__mouseRightButtonPressed = rightButtonPressed;
    if (rightButtonPressed)
    {
      manager.broadcastEvent(OnMouseDownRight(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseDown(MouseButton::RightButton, newMousePos.x, newMousePos.y));
    }
    else
    {
      manager.broadcastEvent(OnMouseUpRight(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseUp(MouseButton::RightButton, newMousePos.x, newMousePos.y));
    }
  }
  if (middleButtonPressed != input__mouseMiddleButtonPressed)
  {
    input__mouseMiddleButtonPressed = middleButtonPressed;
    if (middleButtonPressed)
    {
      manager.broadcastEvent(OnMouseDownMiddle(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseDown(MouseButton::MiddleButton, newMousePos.x, newMousePos.y));
    }
    else
    {
      manager.broadcastEvent(OnMouseUpMiddle(newMousePos.x, newMousePos.y));
      manager.broadcastEvent(OnMouseUp(MouseButton::MiddleButton, newMousePos.x, newMousePos.y));
    }
  }
}

// actually no mouse wheel handling
ECS_TAG(tools)
static __forceinline void on_mouse_wheel_es(const OnMouseWheel &event, float input__deltaTime, float input__wheelSmoothFactor,
  int &input__mouseWheel, float &input__mouseApproachWheel, float &input__mouseWheelDelta)
{
  input__mouseWheel += event.delta;
  float newApproachWheel = approach(input__mouseApproachWheel, (float)input__mouseWheel, input__deltaTime, input__wheelSmoothFactor);
  input__mouseWheelDelta = newApproachWheel - input__mouseApproachWheel;
  input__mouseApproachWheel = newApproachWheel;
}
