// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <windows.h>
#include <winGuiWrapper/wgw_input.h>

#pragma comment(lib, "user32.lib")

namespace wingw
{
bool is_key_pressed(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }


bool is_special_pressed() { return is_key_pressed(V_SHIFT) || is_key_pressed(V_CONTROL) || is_key_pressed(V_ALT); }

int get_modif()
{
  int _modif = 0;
  if (is_key_pressed(V_SHIFT))
    _modif |= M_SHIFT;
  if (is_key_pressed(V_CONTROL))
    _modif |= M_CTRL;
  if (is_key_pressed(V_ALT))
    _modif |= M_ALT;

  return _modif;
}

void get_mouse_pos(int &x, int &y)
{
  POINT pt;
  if (GetCursorPos(&pt))
  {
    x = pt.x;
    y = pt.y;
  }
}
} // namespace wingw
