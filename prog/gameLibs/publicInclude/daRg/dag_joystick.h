#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>

#include <humanInput/dag_hiJoystick.h>


namespace darg
{

struct IGuiScene;

/// @brief Helper class to handle joystick events from separate input thread
class JoystickHandler
{
private:
  JoystickHandler(const JoystickHandler &) = delete;
  JoystickHandler &operator=(const JoystickHandler &) = delete;
  JoystickHandler(JoystickHandler &&) = delete;
  JoystickHandler &operator=(JoystickHandler &&) = delete;

public:
  JoystickHandler() = default;
  ~JoystickHandler() = default;

  // May be called from any thread
  // If called from main thread, processes the queue and new joystick state immediately
  // return combination of BehaviorResult flags
  int onJoystickStateChanged(dag::ConstSpan<IGuiScene *> scenes, HumanInput::IGenJoystick *joy, int joy_ord_id);
  // This must be called from main thread only to process queued joystick state changes
  // return combination of BehaviorResult flags
  int processPendingBtnStack(dag::ConstSpan<IGuiScene *> scenes);

  // Helpers for single-scene case

  int onJoystickStateChanged(IGuiScene *scene, HumanInput::IGenJoystick *joy, int joy_ord_id)
  {
    return onJoystickStateChanged(make_span_const(&scene, 1), joy, joy_ord_id);
  }
  int processPendingBtnStack(IGuiScene *scene) { return processPendingBtnStack(make_span_const(&scene, 1)); }

private:
  HumanInput::IGenJoystick *curJoy = nullptr;
  int16_t curJoyOrdId = -1;
  bool lastProcessSkipped = false;
  dag::RelocatableFixedVector<HumanInput::ButtonBits, 8> btnStack;

  // return combination of BehaviorResult flags
  int dispatchJoystickStateChanged(dag::ConstSpan<IGuiScene *> scenes, const HumanInput::ButtonBits &buttons,
    const HumanInput::ButtonBits &buttonsPrev);
};


} // namespace darg
