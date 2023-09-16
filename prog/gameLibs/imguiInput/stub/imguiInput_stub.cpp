#include "imgui/imguiInput.h"
#include <math/integer/dag_IPoint2.h>

const IPoint2 &imgui_get_saved_mouse_pos() { return ZERO<IPoint2>(); }
bool imgui_handle_special_keys_down(bool, bool, bool, int, unsigned int) { return false; }
bool imgui_handle_special_keys_up(bool, bool, bool, int, unsigned int) { return false; }
void imgui_register_global_input_handler(GlobalInputHandler) {}
bool imgui_handle_special_controller_combinations(const HumanInput::IGenJoystick *) { return false; }

bool imgui_in_hybrid_input_mode() { return false; }
void imgui_use_hybrid_input_mode(bool) {}
