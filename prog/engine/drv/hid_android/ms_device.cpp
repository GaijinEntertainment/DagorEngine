// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiGlobals.h>
#include "ms_device.h"
#include <generic/dag_tab.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <stdlib.h>
#include "../hid_commonCode/touch_api_impl.inc.cpp"

using namespace HumanInput;
static Tab<unsigned short> ext_btn_touches;

TapMouseDevice::TapMouseDevice()
{
  client = NULL;
  memset(&state, 0, sizeof(state));
  lbmPressedT0 = lbmReleasedT0 = lbmMotion = lbmMotionDx = lbmMotionDy = 0;
  add_wnd_proc_component(this);

  raw_state_pnt.mouse.x = state.mouse.x = 0;
  raw_state_pnt.mouse.y = state.mouse.y = 0;
}
TapMouseDevice::~TapMouseDevice()
{
  setClient(NULL);
  del_wnd_proc_component(this);
  clear_and_shrink(ext_btn_touches);
}


void TapMouseDevice::setClient(IGenPointingClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}

void TapMouseDevice::update()
{
  /*
    if (!stg_pnt.touchScreen)
      return;

    int t = get_time_msec();
    if (lbmPressedT0 && t > lbmPressedT0 + stg_pnt.tapAndHoldThresMs && lbmMotion < stg_pnt.tapStillCircleRad)
    {
      state.mouse.buttons &= ~0x100000;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      if (client)
        client->gmcMouseButtonUp(this, 20);

      state.mouse.buttons |= 0x80000;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      if (client)
        client->gmcMouseButtonDown(this, 19);
      lbmPressedT0 = 0;
    }
    else if (lbmPressedT0 && t > lbmPressedT0 + stg_pnt.tapAndHoldThresMs)
      lbmPressedT0 = 0;
    else if (lbmReleasedT0 && t > lbmReleasedT0 + stg_pnt.tapReleaseDelayMs)
    {
      state.mouse.buttons &= ~1;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      if (client)
        client->gmcMouseButtonUp(this, 0);
      lbmReleasedT0 = 0;
    }
  */
}

namespace native_android
{
extern int32_t window_width;
extern int32_t window_height;
extern int32_t buffer_width;
extern int32_t buffer_height;
} // namespace native_android

static Point2 convert_to_screen_coordinate(const int x, const int y)
{
  const auto convert = [](float income, float buffer, float window) {
    const bool sameSize = buffer == window;
    float ret = income;
    if (!sameSize)
      ret = income * buffer / window;

    // clamp to valid range
    return max(0.0f, min(ret, buffer));
  };
  Point2 p;
  p.x = convert((float)x, native_android::buffer_width, native_android::window_width);
  p.y = convert((float)y, native_android::buffer_height, native_android::window_height);

  return p;
}


IWndProcComponent::RetCode TapMouseDevice::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
{
  if (msg == GPCM_MouseBtnPress)
  {
    raw_state_pnt.mouse.deltaX = state.mouse.deltaX = 0;
    raw_state_pnt.mouse.deltaY = state.mouse.deltaY = 0;
    if (!stg_pnt.touchScreen)
    {
      state.mouse.buttons |= 1;
      raw_state_pnt.mouse.buttons |= 1;
      raw_state_pnt.mouse.x = state.mouse.x = wParam;
      raw_state_pnt.mouse.y = state.mouse.y = lParam;
    }
    else
    {
      lbmPressedT0 = get_time_msec();
      lbmMotion = lbmMotionDx = lbmMotionDy = 0;
      if (lbmReleasedT0)
      {
        state.mouse.buttons &= ~1;
        raw_state_pnt.mouse.buttons = state.mouse.buttons;
        if (client)
          client->gmcMouseButtonUp(this, 0);
        lbmReleasedT0 = 0;
      }

      state.mouse.buttons |= 0x100000;
      raw_state_pnt.mouse.x = state.mouse.x = wParam;
      raw_state_pnt.mouse.y = state.mouse.y = lParam;
      if (client)
      {
        client->gmcMouseMove(this, 0, 0);
        raw_state_pnt.mouse.buttons = state.mouse.buttons;
        client->gmcMouseButtonDown(this, 20);
      }
      return PROCEED_OTHER_COMPONENTS;
    }
    if (client)
      client->gmcMouseButtonDown(this, 0);
  }
  else if (msg == GPCM_MouseMove || msg == GPCM_MouseBtnRelease)
  {
    int dx = wParam - state.mouse.x;
    int dy = lParam - state.mouse.y;
    raw_state_pnt.mouse.deltaX += dx;
    raw_state_pnt.mouse.deltaY += dy;
    lbmMotionDx += dx;
    lbmMotionDy += dy;
    lbmMotion = max(lbmMotion, max(abs(lbmMotionDx), abs(lbmMotionDy)));
    state.mouse.deltaX = raw_state_pnt.mouse.deltaX;
    state.mouse.deltaY = raw_state_pnt.mouse.deltaY;

    raw_state_pnt.mouse.x = state.mouse.x = wParam;
    raw_state_pnt.mouse.y = state.mouse.y = lParam;
    if (msg == GPCM_MouseMove)
    {
      raw_state_pnt.mouse.buttons = state.mouse.buttons;

      if (client)
        client->gmcMouseMove(this, dx, dy);
      return PROCEED_OTHER_COMPONENTS;
    }

    if (!stg_pnt.touchScreen)
    {
      state.mouse.buttons &= ~1;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      client->gmcMouseButtonUp(this, 0);
    }
    else if (state.mouse.buttons & 0x100000)
    {
      state.mouse.buttons &= ~0x100000;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      if (client)
        client->gmcMouseButtonUp(this, 20);

      if (lbmMotion < stg_pnt.tapStillCircleRad)
      {
        state.mouse.buttons |= 1;
        raw_state_pnt.mouse.buttons = state.mouse.buttons;
        if (client)
          client->gmcMouseButtonDown(this, 0);

        lbmPressedT0 = 0;
        lbmReleasedT0 = get_time_msec();
      }
    }
    else if (state.mouse.buttons & 0x80000)
    {
      state.mouse.buttons &= ~0x80000;
      raw_state_pnt.mouse.buttons = state.mouse.buttons;
      if (client)
        client->gmcMouseButtonUp(this, 19);
    }
  }
  else if (msg == GPCM_TouchBegan || GPCM_TouchMoved || GPCM_TouchEnded)
  {
    const Point2 p = convert_to_screen_coordinate(lParam & 0xffff, lParam >> 16);

    if (msg == GPCM_TouchBegan)
      hid::gpcm_TouchBegan(state, this, client, p.x, p.y, wParam, PointingRawState::Touch::TS_touchScreen);
    else if (msg == GPCM_TouchMoved)
      hid::gpcm_TouchMoved(state, this, client, p.x, p.y, wParam);
    else if (msg == GPCM_TouchEnded)
      hid::gpcm_TouchEnded(state, this, client, p.x, p.y, wParam);
  }

  return PROCEED_OTHER_COMPONENTS;
}
