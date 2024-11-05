// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "acc_gyro_classdrv.h"

#include <drv/hid/dag_hiCreate.h>
#include <supp/dag_math.h>
using namespace HumanInput;

IGenJoystickClassDrv *HumanInput::createJoystickClassDriver(bool exclude_xinput, bool remap_360)
{
  AccelGyroInpClassDriver *cd = new (inimem) AccelGyroInpClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}

namespace hid_ios
{
extern double g_acc_x, g_acc_y, g_acc_z;
extern double g_base_acc_x, g_base_acc_y, g_base_acc_z;
} // namespace hid_ios

void HumanInput::setAsNeutralGsensor_iOS()
{
  using namespace hid_ios;

  g_base_acc_x = g_acc_x;
  g_base_acc_y = g_acc_y;
  g_base_acc_z = g_acc_z;

  double l2 = g_acc_x * g_acc_x + g_acc_y * g_acc_y + g_acc_z * g_acc_z;
  if (l2 > 1e-6)
  {
    l2 = sqrt(l2);
    g_base_acc_x /= l2;
    g_base_acc_y /= l2;
    g_base_acc_z /= l2;
  }
}
