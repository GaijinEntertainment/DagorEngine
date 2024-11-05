// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include "input/touchInput.h"

namespace bind_dascript
{
__forceinline void press_button(dainput::action_handle_t a) { touch_input.pressButton(a); }

__forceinline void release_button(dainput::action_handle_t a) { touch_input.releaseButton(a); }

__forceinline bool is_button_pressed(dainput::action_handle_t a) { return touch_input.isButtonPressed(a); }

__forceinline void set_stick_value(dainput::action_handle_t a, const Point2 &val) { touch_input.setStickValue(a, val); }

__forceinline Point2 get_stick_value(dainput::action_handle_t a) { return touch_input.getStickValue(a); }

__forceinline void set_axis_value(dainput::action_handle_t a, const float &val) { touch_input.setAxisValue(a, val); }

__forceinline float get_axis_value(dainput::action_handle_t a) { return touch_input.getAxisValue(a); }
} // namespace bind_dascript