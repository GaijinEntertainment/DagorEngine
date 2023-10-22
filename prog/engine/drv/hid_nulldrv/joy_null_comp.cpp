#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiComposite.h>

HumanInput::CompositeJoystickClassDriver *global_cls_composite_drv_joy = NULL;

HumanInput::CompositeJoystickClassDriver *HumanInput::CompositeJoystickClassDriver::create() { return NULL; }
void HumanInput::CompositeJoystickClassDriver::addClassDrv(IGenJoystickClassDrv *, bool) {}
bool HumanInput::CompositeJoystickClassDriver::isAxisDigital(int) const { return false; }
int HumanInput::CompositeJoystickClassDriver::getRealDeviceCount(bool) const { return 0; }
const char *HumanInput::CompositeJoystickClassDriver::getRealDeviceName(int) const { return NULL; }
const char *HumanInput::CompositeJoystickClassDriver::getRealDeviceID(int) const { return NULL; }
bool HumanInput::CompositeJoystickClassDriver::getRealDeviceDesc(int, DataBlock &) const { return false; }
int HumanInput::CompositeJoystickClassDriver::remapAbsButtonId(int, int &) const { return -1; }
int HumanInput::CompositeJoystickClassDriver::remapRelButtonId(int, int) const { return -1; }
int HumanInput::CompositeJoystickClassDriver::remapAbsAxisId(int, int &) const { return -1; }
int HumanInput::CompositeJoystickClassDriver::remapRelAxisId(int, int) const { return -1; }
int HumanInput::CompositeJoystickClassDriver::remapAbsPovHatId(int, int &) const { return -1; }
int HumanInput::CompositeJoystickClassDriver::remapRelPovHatId(int, int) const { return -1; }
void HumanInput::CompositeJoystickClassDriver::setLocClient(ICompositeLocalization *, bool) {}
const char *HumanInput::CompositeJoystickClassDriver::getNativeAxisName(int) const { return NULL; }
const char *HumanInput::CompositeJoystickClassDriver::getNativeButtonName(int) const { return NULL; }
void HumanInput::CompositeJoystickClassDriver::setRumbleTargetRealDevIdx(int) {}
float HumanInput::CompositeJoystickClassDriver::getStickDeadZoneScale(int, bool) const { return 0; }
void HumanInput::CompositeJoystickClassDriver::setStickDeadZoneScale(int, bool, float) {}
float HumanInput::CompositeJoystickClassDriver::getStickDeadZoneAbs(int, bool, HumanInput::IGenJoystick *) const { return 0; }
void HumanInput::enableJoyFx(bool) {}
