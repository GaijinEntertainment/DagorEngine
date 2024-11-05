// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <render/genericLUT/commonLUT.h>
#include <drv/3d/dag_resetDevice.h>

static int driver_reset_counter = 0;
static void reset_counter(bool) { driver_reset_counter++; }
REGISTER_D3D_AFTER_RESET_FUNC(reset_counter);

CommonLUT::CommonLUT() = default;

bool CommonLUT::isInited() const { return static_cast<bool>(lutBuilder.getLUT()); }

void CommonLUT::requestUpdate() { frameReady = false; }

bool CommonLUT::render()
{
  if (driver_reset_counter > driverResetCounter)
  {
    frameReady = false;
    driverResetCounter = driver_reset_counter;
  }
  if (frameReady)
    return true;
  TIME_D3D_PROFILE(build_lut)
  if (!lutBuilder.perform())
    return false;
  lutBuilder.getLUT().setVar();
  frameReady = true;
  return true;
}
