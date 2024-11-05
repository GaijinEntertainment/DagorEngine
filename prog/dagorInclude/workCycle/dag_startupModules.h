//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//! schedules video initialization in fullscreen or windowed mode
void dagor_init_video(const char *wc_name, int ncmd, void *hicon, const char *title, const char *game_name = "", int game_version = 0);

//! schedules keyboard driver initialization (used window message mode)
void dagor_init_keyboard_win();

//! schedules mouse driver initialization (used window message mode)
void dagor_init_mouse_win();

//! schedules joystick driver initialization
void dagor_init_joystick();
void dagor_init_joystick_xinput();
void dagor_init_joystick_steam();

//! schedules null device drivers initialization
void dagor_init_keyboard_null();
void dagor_init_mouse_null();
void dagor_init_joystick_null();

//! performs video initialization (creates app window)
//! and schedules common initialization
void dagor_common_startup();
