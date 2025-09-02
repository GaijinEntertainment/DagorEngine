// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>

enum ImGuiKey : int;

static constexpr int EC_WHEEL_DELTA = 120; // Use the WHEEL_DELTA delta value from Windows.

// Correctponds to MK_LBUTTON, MK_RBUTTON and MK_MBUTTON.
enum
{
  EC_MK_LBUTTON = 0x1,
  EC_MK_RBUTTON = 0x2,
  EC_MK_MBUTTON = 0x10,
};

// Get the mouse cursor's current position in screen coordinates.
IPoint2 ec_get_cursor_pos();

// Set the mouse cursor's current position in screen coordinates.
void ec_set_cursor_pos(IPoint2 pos);

// Set the mouse cursor's visibility.
void ec_show_cursor(bool show);

// Set busy (waiting) cursor for the application.
// Returns with the previous busy state.
bool ec_set_busy(bool busy);

// Checks whether the busy cursor should be shown by the application.
bool ec_get_busy();

// Checks whether the specified key is currently pressed.
bool ec_is_key_down(ImGuiKey key);

// Checks whether the Alt key is currently pressed.
bool ec_is_alt_key_down();

// Checks whether the Ctrl key is currently pressed.
bool ec_is_ctrl_key_down();

// Checks whether the Shift key is currently pressed.
bool ec_is_shift_key_down();
