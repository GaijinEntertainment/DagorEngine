// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>

namespace HumanInput
{
struct JoyCookedCreateParams;

enum AxisType
{
  JOY_AXIS_X,
  JOY_AXIS_Y,
  JOY_AXIS_Z,
  JOY_AXIS_RX,
  JOY_AXIS_RY,
  JOY_AXIS_RZ,
  JOY_SLIDER0,
  JOY_SLIDER1,
};

class Di8JoystickDevice : public IGenJoystick
{
public:
  Di8JoystickDevice(IDirectInputDevice8 *dev, bool must_poll, bool remap_360, bool quiet, unsigned *guid);
  ~Di8JoystickDevice();

  void setDeviceName(const char *name);
  void setId(int id, int user_id)
  {
    ordId = id;
    userId = user_id;
  }
  void markAsGamepad(bool is_gamepad) { isActualGamepad = is_gamepad; }
  bool isGamepad() { return isActualGamepad; }

  void addAxis(AxisType axis_type, int di_type, const char *name);
  void addButton(const char *name);
  void addPovHat(const char *name);

  IDirectInputDevice8 *getDev() { return device; }
  bool isEqual(Di8JoystickDevice *dev);

  void acquire();
  void unacquire();
  void poll();

  bool updateState(int dt_msec, bool def);

  void addFx(IGenJoyFfEffect *fx);
  void delFx(IGenJoyFfEffect *fx);
  bool processAxes(JoyCookedCreateParams &dest, const JoyFfCreateParams &src);
  void applyRemapX360(const char *productName);
  void applyRemapHats();

  // IGenJoystick interface implementation
  virtual const char *getName() const { return name; }
  virtual const char *getDeviceID() const { return devID; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const { return false; }
  virtual unsigned short getUserId() const { return userId; }
  virtual const JoystickRawState &getRawState() const { return state; }
  virtual void setClient(IGenJoystickClient *cli);
  virtual IGenJoystickClient *getClient() const;

  virtual int getButtonCount() const;
  virtual const char *getButtonName(int idx) const;

  virtual int getAxisCount() const;
  virtual const char *getAxisName(int idx) const;

  virtual int getPovHatCount() const;
  virtual const char *getPovHatName(int idx) const;

  virtual int getButtonId(const char *name) const;
  virtual int getAxisId(const char *name) const;
  virtual int getPovHatId(const char *name) const;
  virtual void setAxisLimits(int axis_id, float min_val, float max_val);
  virtual bool getButtonState(int btn_id) const;
  virtual float getAxisPos(int axis_id) const;
  virtual int getAxisPosRaw(int axis_id) const;
  virtual int getPovHatAngle(int axis_id) const;

  virtual IJoyFfCondition *createConditionFx(int condfx_type, const JoyFfCreateParams &cp);
  virtual IJoyFfConstForce *createConstForceFx(const JoyFfCreateParams &cp, float force);
  virtual IJoyFfPeriodic *createPeriodicFx(int period_type, const JoyFfCreateParams &cp);
  virtual IJoyFfRampForce *createRampForceFx(const JoyFfCreateParams &cp, float start_f, float end_f);

  virtual void doRumble(float lowFreq, float highFreq) {}
  virtual void setLight(bool set, unsigned char r, unsigned char g, unsigned char b) {}
  virtual bool isConnected();

  int getPairedButtonForAxis(int axis_idx) const override;
  int getPairedAxisForButton(int btn_idx) const override;

  int getJointAxisForAxis(int axis_idx) const override;

protected:
  IDirectInputDevice8 *device;
  IGenJoystickClient *client;
  JoystickRawState state;
  void (*decodeData)(const DIJOYSTATE2 &is, JoystickRawState &os);

  Tab<IGenJoyFfEffect *> fxList;

  int ordId, userId;
  bool mustPoll, remapAsX360, isActualGamepad;
  bool shouldRemapAsX360() const { return remapAsX360 && isActualGamepad; }

  struct AxisData
  {
    AxisType type;
    int diType;
    float v0, dv;
    int *valStorage;
    String name;
  };
  struct PovHatData
  {
    String name;
    String axisName[2];
  };

  Tab<String> buttons;
  Tab<AxisData> axes;
  Tab<PovHatData> povHats;
  String name;
  char devID[34];

  static constexpr int MAX_BUTTONS = JoystickRawState::MAX_BUTTONS - 16;

private:
  int getVirtualPOVAxis(int hat_id, int axis_id) const;
};
} // namespace HumanInput
