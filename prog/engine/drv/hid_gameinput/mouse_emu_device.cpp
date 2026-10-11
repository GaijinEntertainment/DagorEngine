// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mouse_emu_device.h"

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <workCycle/dag_workCycle.h>
#include <supp/dag_math.h>
#include <stdlib.h>


#define LOW_THRES   2000
#define MOUSE_SPEED 0.01f

namespace HumanInput
{

void GamepadMouseEmuDevice::update()
{
  uint64_t bw0 = raw_state_joy.buttons.getDWord0();

  // Use second joystick from composite joystick as xinput-compatible device.
  const uint64_t lbMask = JOY_XINPUT_REAL_MASK_R_TRIGGER | (uint64_t)JOY_XINPUT_REAL_MASK_R_TRIGGER << JOY_XINPUT_REAL_BTN_COUNT;
  const int dx = raw_state_joy.x;
  const int dy = raw_state_joy.y;

  if (buttonsEnabled)
  {
    if (!stg_pnt.allowEmulatedLMB)
    {
      if (lastButtons & lbMask)
        onButtonUp(0);
      lastButtons = 0;
    }
    else
    {
      // emulate LMB with right thumb click
      if ((bw0 & lbMask) && !(lastButtons & lbMask))
        onButtonDown(0);
      else if (!(bw0 & lbMask) && (lastButtons & lbMask))
        onButtonUp(0);

      lastButtons = bw0;
    }
  }

  if (cursorEnabled)
  {
    int vx = abs(dx), vy = abs(dy);
    if (vx < LOW_THRES)
      vx = 0;
    else
      vx = (dx > 0) ? dx - LOW_THRES : dx + LOW_THRES;

    if (vy < LOW_THRES)
      vy = 0;
    else
      vy = (dy > 0) ? dy - LOW_THRES : dy + LOW_THRES;

    if (vx || vy)
    {
      incX += vx * ::dagor_game_act_time * MOUSE_SPEED;
      incY -= vy * ::dagor_game_act_time * MOUSE_SPEED;

      if (fabsf(incX) > 1.0f || fabsf(incY) > 1.0f)
      {
        vx = incX > 0 ? floorf(incX) : ceilf(incX);
        vy = incY > 0 ? floorf(incY) : ceilf(incY);
        incX -= vx;
        incY -= vy;

        onMove(vx, vy);
      }
    }
  }
}

} // namespace HumanInput
