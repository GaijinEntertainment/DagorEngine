#include "joy_acc_gyro_classdrv.h"

#include <humanInput/dag_hiCreate.h>
#include <supp/dag_math.h>
using namespace HumanInput;

IGenJoystickClassDrv *HumanInput::createJoystickClassDriver(bool exclude_xinput, bool remap_360)
{
  JoyAccelGyroInpClassDriver *cd = new (inimem) JoyAccelGyroInpClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}

namespace hid_ios
{
double acc_x = 0, acc_y = 0, acc_z = 0;
double g_acc_x = 0, g_acc_y = 0, g_acc_z = 0;
double m_acc_x = 0, m_acc_y = 0, m_acc_z = 0;
double r_rate_x = 0, r_rate_y = 0, r_rate_z = 0;
double g_base_acc_x = 0, g_base_acc_y = 0, g_base_acc_z = -1;
double g_rel_ang_x = 0, g_rel_ang_y = 0;
} // namespace hid_ios

void HumanInput::setAsNeutralGsensor_android()
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
