#include <ecs/core/entitySystem.h>
#include <ecs/inputEvents.h>
#include <math/dag_mathBase.h>
#include <winGuiWrapper/wgw_input.h>

ECS_TAG(tools)
ECS_NO_ORDER
static __forceinline void key_board_input_es(const ecs::UpdateStageInfoAct &stage, float input__keyboardSmoothFactor,
  float &input__deltaTime, float &input__verticalAxis, float &input__horizontalAxis, IPoint2 &input__mousePosition,
  IPoint2 &input__mouseDelta, bool &input__mouseLeftButtonPressed, bool &input__mouseRightButtonPressed,
  bool &input__mouseMiddleButtonPressed, bool &input__keyAccelerationPressed, ecs::EntityManager &manager)
{
  input__deltaTime = stage.dt;
  input__verticalAxis = approach(input__verticalAxis, (float)(wingw::is_key_pressed('W') - wingw::is_key_pressed('S')), stage.dt,
    input__keyboardSmoothFactor);
  input__horizontalAxis = approach(input__horizontalAxis, (float)(wingw::is_key_pressed('D') - wingw::is_key_pressed('A')), stage.dt,
    input__keyboardSmoothFactor);

  input__keyAccelerationPressed = wingw::is_key_pressed(wingw::V_SHIFT);
  IPoint2 newMousePos;
  wingw::get_mouse_pos(newMousePos.x, newMousePos.y);
  input__mouseDelta = newMousePos - input__mousePosition;
  input__mousePosition = newMousePos;

  bool leftButtonPressed = wingw::is_key_pressed(wingw::V_LBUTTON);
  bool rightButtonPressed = wingw::is_key_pressed(wingw::V_RBUTTON);
  bool middleButtonPressed = wingw::is_key_pressed(wingw::V_MBUTTON);

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
