#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <math/dag_mathBase.h>
#include <string.h>

using namespace HumanInput;

class NullMouseDrv : public IGenPointingClassDrv, public IGenPointing
{
  bool relative = false;
  int clipX0 = 0, clipY0 = 0, clipX1 = 128, clipY1 = 128;

public:
  // IGenPointingClassDrv interface
  virtual void enable(bool) {}
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy() {}
  virtual void refreshDeviceList() {}
  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices() {}
  IGenPointing *getDevice(int) const override { return (NullMouseDrv *)this; }
  void useDefClient(IGenPointingClient *) override {}

  // IGenPointing interface
  virtual const char *getName() const { return "NullMouse"; }
  const PointingRawState &getRawState() const override { return raw_state_pnt; }
  void setClient(IGenPointingClient *) override {}
  IGenPointingClient *getClient() const override { return NULL; }
  virtual int getBtnCount() const { return 0; }
  virtual const char *getBtnName(int) const { return NULL; }
  virtual void setRelativeMovementMode(bool enable) { relative = enable; }
  virtual bool getRelativeMovementMode() { return relative; }

  virtual void setClipRect(int l, int t, int r, int b)
  {
    if (clipX0 == l && clipY0 == t && clipX1 == r && clipY1 == b)
      return;
    clipX0 = l, clipY0 = t, clipX1 = r, clipY1 = b;
    raw_state_pnt.mouse.x = (l + r) / 2;
    raw_state_pnt.mouse.y = (t + b) / 2;
  }
  virtual void setPosition(int x, int y)
  {
    raw_state_pnt.mouse.x = clamp(x, clipX0, clipX1);
    raw_state_pnt.mouse.y = clamp(y, clipY0, clipY1);
  }

  virtual void setMouseCapture(void *) {}
  virtual void releaseMouseCapture() {}
  virtual bool isPointerOverWindow() { return true; }
};

static NullMouseDrv drv;

IGenPointingClassDrv *HumanInput::createNullMouseClassDriver()
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));
  return &drv;
}
