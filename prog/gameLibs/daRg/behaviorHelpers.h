#pragma once

#include <daRg/dag_guiScene.h>
#include <sqrat.h>

namespace darg
{
class Element;
struct EventDataRect;

void call_click_handler(IGuiScene *scene, Element *elem, InputDevice dev_id, bool dbl, int mouse_btn, short mx, short my);
bool call_mouse_move_handler(IGuiScene *scene, Element *elem, short mx, short my);
bool call_mouse_wheel_handler(IGuiScene *scene, Element *elem, int delta, short mx, short my);
void call_hotkey_handler(IGuiScene *scene, Element *elem, Sqrat::Function &handler, bool from_joystick);
void calc_elem_rect_for_event_handler(EventDataRect &rect, Element *elem);

int active_state_flags_for_device(InputDevice device);
} // namespace darg
