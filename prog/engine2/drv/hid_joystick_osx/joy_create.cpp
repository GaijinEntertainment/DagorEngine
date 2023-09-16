#include "joy_classdrv.h"
#include <humanInput/dag_hiCreate.h>
#include <debug/dag_debug.h>
using namespace HumanInput;

IGenJoystickClassDrv *HumanInput::createJoystickClassDriver(bool, bool)
{
  HidJoystickClassDriver *cd = new (inimem) HidJoystickClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}
