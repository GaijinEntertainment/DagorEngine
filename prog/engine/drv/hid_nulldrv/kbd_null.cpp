#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <string.h>

using namespace HumanInput;

class NullKeyboardDrv : public IGenKeyboardClassDrv, public IGenKeyboard
{
public:
  // IGenKeyboardClassDrv interface
  virtual void enable(bool) {}
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy() {}
  virtual void refreshDeviceList() {}
  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices() {}
  virtual IGenKeyboard *getDevice(int) const { return (NullKeyboardDrv *)this; }
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual void useDefClient(IGenKeyboardClient *) {}

  // IGenKeyaboard interface
  virtual const char *getName() const { return "NullKeyboard"; }
  virtual const KeyboardRawState &getRawState() const { return raw_state_kbd; }
  virtual void setClient(IGenKeyboardClient *) {}
  virtual IGenKeyboardClient *getClient() const { return NULL; }
  virtual int getKeyCount() const { return 0; }
  virtual const char *getKeyName(int) const { return NULL; }
  virtual int convertToChar(int, bool) const { return 0; }
};

static NullKeyboardDrv drv;

IGenKeyboardClassDrv *HumanInput::createNullKeyboardClassDriver()
{
  memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));
  return &drv;
}
