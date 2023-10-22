#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>

HumanInput::IGenJoystickClassDrv *HumanInput::createXinputJoystickClassDriver(bool)
{
  return HumanInput::createNullJoystickClassDriver();
}
