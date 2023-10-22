#include <humanInput/dag_hiGlobals.h>
#include "ms_device.h"
#include <generic/dag_tab.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
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

void TapMouseDevice::setClipRect(int l, int t, int r, int b)
{
  if (clipX0 == l && clipY0 == t && clipX1 == r && clipY1 == b)
    return;
  clipX0 = l, clipY0 = t, clipX1 = r, clipY1 = b;
  state.mouse.x = (l + r) / 2;
  state.mouse.y = (t + b) / 2;
}

void TapMouseDevice::setPosition(int x, int y)
{
  state.mouse.x = clamp(x, clipX0, clipX1);
  state.mouse.y = clamp(y, clipY0, clipY1);
}

void TapMouseDevice::update()
{
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
  else if (msg == GPCM_TouchBegan)
    hid::gpcm_TouchBegan(state, this, client, lParam & 0xffff, lParam >> 16, wParam, PointingRawState::Touch::TS_touchScreen);
  else if (msg == GPCM_TouchMoved)
    hid::gpcm_TouchMoved(state, this, client, lParam & 0xffff, lParam >> 16, wParam);
  else if (msg == GPCM_TouchEnded)
    hid::gpcm_TouchEnded(state, this, client, lParam & 0xffff, lParam >> 16, wParam);

  return PROCEED_OTHER_COMPONENTS;
}
