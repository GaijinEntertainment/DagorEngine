#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>

HumanInput::IGenJoystickClassDrv *HumanInput::createJoystickClassDriver(bool, bool)
{
  return HumanInput::createNullJoystickClassDriver();
}
