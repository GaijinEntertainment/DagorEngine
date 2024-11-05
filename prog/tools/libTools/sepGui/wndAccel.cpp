// Copyright (C) Gaijin Games KFT.  All rights reserved.

// #define _WIN32_WINNT 0x500

#include "wndAccel.h"
#include "wndManager.h"
#include <winGuiWrapper/wgw_input.h>
#include <windows.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// manager
//-----------------------------------------------------------------------------

void WinManager::addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift)
{
  mAccels.addAccelerator(cmd_id, v_key, ctrl, alt, shift, true);
}


void WinManager::addAcceleratorUp(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift)
{
  mAccels.addAccelerator(cmd_id, v_key, ctrl, alt, shift, false);
}


void WinManager::addViewportAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift, bool allow_repeat)
{
  G_ASSERT(false);
}

void WinManager::clearAccelerators() { mAccels.clear(); }

unsigned WinManager::processImguiAccelerator()
{
  G_ASSERT(false);
  return 0;
}

unsigned WinManager::processImguiViewportAccelerator()
{
  G_ASSERT(false);
  return 0;
}

void WinManager::initCustomMouseCursors(const char *path) { G_ASSERT(false); }

void WinManager::updateImguiMouseCursor() { G_ASSERT(false); }

//-----------------------------------------------------------------------------
// accels
//-----------------------------------------------------------------------------

static WinAccel *g_win_accel = NULL;

LRESULT CALLBACK accel_hook_proc(int code, WPARAM w_param, LPARAM l_param)
{
  if ((code == HC_ACTION) && (::g_win_accel->procAccel(code, (TSgWParam)w_param, (TSgLParam)l_param)))
    return 1;

  return CallNextHookEx(0, code, w_param, l_param);
}


WinAccel::WinAccel(WinManager *manager) : mAccelArray(midmem), hhandle(0), mManager(manager)
{
  G_ASSERT(!::g_win_accel && "Should be only one instance!");

  ::g_win_accel = this;

  clear();
  hhandle = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)::accel_hook_proc, NULL, GetCurrentThreadId());
}


WinAccel::~WinAccel()
{
  UnhookWindowsHookEx((HHOOK)hhandle);
  clear();
}


//-----------------------------------------------------------------------------


long WinAccel::procAccel(int code, TSgWParam w_param, TSgLParam l_param)
{
  bool _key_down = true;
  bool _empty_process = false;

  switch (uintptr_t(l_param) >> 28)
  {
    case 0xC: _key_down = false; break;

    case 0x4: _empty_process = true;
    case 0: _key_down = true; break;

    default: return 0;
  }

  for (int i = 0; i < mAccelArray.size(); ++i)
  {
    if ((uintptr_t(w_param) == mAccelArray[i].v_key) && (wingw::is_key_pressed(VK_CONTROL) == mAccelArray[i].ctrl) &&
        (wingw::is_key_pressed(VK_MENU) == mAccelArray[i].alt) && (wingw::is_key_pressed(VK_SHIFT) == mAccelArray[i].shift) &&
        (_key_down == mAccelArray[i].down))
    {
      if (_empty_process)
        return 1;
      return mManager->getMainMenu()->click(mAccelArray[i].cmd_id); // if processed return 1;
    }
  }

  return 0;
}


//-----------------------------------------------------------------------------


void WinAccel::addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift, bool down)
{
  AccelRecord a = {cmd_id, v_key, ctrl, alt, shift, down};
  mAccelArray.push_back(a);
}


void WinAccel::clear() { clear_and_shrink(mAccelArray); }
