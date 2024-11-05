//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace wingw
{
enum
{
  V_LBUTTON = 0x01,
  V_RBUTTON,

  V_MBUTTON = 0x04,

  V_BACK = 0x08,
  V_TAB,

  V_ENTER = 0x0D,

  V_RETURN = 0x0D,
  V_SHIFT = 0x10,
  V_CONTROL,
  V_ALT,
  V_CAPSLOCK = 0x14,

  V_ESC = 0x1B,
  V_SPACE = 0x20,

  V_PAGE_UP = 0x21,
  V_PAGE_DOWN = 0x22,
  V_END = 0x23,
  V_HOME = 0x24,

  V_LEFT = 0x25,
  V_UP,
  V_RIGHT,
  V_DOWN,

  V_INSERT = 0x2D,
  V_DELETE,

  V_NUMPAD0 = 0x60,
  V_NUMPAD1,
  V_NUMPAD2,
  V_NUMPAD3,
  V_NUMPAD4,
  V_NUMPAD5,
  V_NUMPAD6,
  V_NUMPAD7,
  V_NUMPAD8,
  V_NUMPAD9,

  V_DECIMAL = 0x6E,

  V_F1 = 0x70,
  V_F2,
  V_F3,
  V_F4,
  V_F5,
  V_F6,
  V_F7,
  V_F8,
  V_F9,
  V_F10,
  V_F11,
  V_F12,

  V_LSHIFT = 0xA0,
  V_RSHIFT,
  V_LCONTROL,
  V_RCONTROL,
  V_LATL,
  V_RATL,

  V_BRACKET_O = 0xDB,
  V_BRACKET_C = 0xDD,
};

enum
{
  M_ALT = 1,
  M_SHIFT = 2,
  M_CTRL = 4,
  M_EXTENDED = 0x80,
};


bool is_key_pressed(int vk);
bool is_special_pressed();
int get_modif();
void get_mouse_pos(int &x, int &y);
}; // namespace wingw
