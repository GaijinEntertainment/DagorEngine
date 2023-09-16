#pragma once

#include <humanInput/dag_hiDecl.h>
#include <math/dag_math2d.h>
#include <squirrel.h>


class SqModules;

namespace gamelib
{

namespace input
{
void initialize();
void finalize();

void bind_script(SqModules *module_mgr);
void on_keyboard_event(HSQUIRRELVM vm, HumanInput::IGenKeyboard *kbd, int btn_idx, bool pressed, bool repeat, wchar_t wc,
  bool processed_by_ui);
void on_mouse_button_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, int btn_idx, bool pressed, const Point2 &pos,
  bool processed_by_ui);
void on_mouse_wheel_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, int delta, const Point2 &pos, bool processed_by_ui);
void on_mouse_move_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, const Point2 &pos, bool processed_by_ui);
void on_joystick_state_changed(HSQUIRRELVM vm, HumanInput::IGenJoystick *joy, int joy_ord_id);
void process_pending_joystick_btn_stack(HSQUIRRELVM vm);
} // namespace input

} // namespace gamelib
