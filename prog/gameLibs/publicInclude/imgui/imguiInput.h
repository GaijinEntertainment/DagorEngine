//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

using GlobalInputHandler = eastl::function<bool(/*key_down*/ bool, /*key_idx*/ int, /*key_modif*/ unsigned int)>;

class IPoint2;
namespace HumanInput
{
class IGenJoystick;
}

const IPoint2 &imgui_get_saved_mouse_pos();
bool imgui_handle_special_keys_down(bool ctrl, bool shift, bool alt, int btn_idx, unsigned int key_modif);
bool imgui_handle_special_keys_up(bool ctrl, bool shift, bool alt, int btn_idx, unsigned int key_modif);
void imgui_register_global_input_handler(GlobalInputHandler);
bool imgui_handle_special_controller_combinations(const HumanInput::IGenJoystick *joy);

bool imgui_in_hybrid_input_mode();
void imgui_use_hybrid_input_mode(bool);
void imgui_switch_state();
void imgui_switch_overlay();
void imgui_set_viewport_offset(int offsetX, int offsetY);
void imgui_draw_mouse_cursor(bool draw_mouse_cursor);
