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
  const char *getName() const override { return name; }
  const char *getDeviceID() const override { return devID; }
  bool getDeviceDesc(DataBlock &out_desc) const override { return false; }
  unsigned short getUserId() const override { return userId; }
  const JoystickRawState &getRawState() const override { return state; }
  void setClient(IGenJoystickClient *cli) override;
  IGenJoystickClient *getClient() const override;

  int getButtonCount() const override;
  const char *getButtonName(int idx) const override;

  int getAxisCount() const override;
  const char *getAxisName(int idx) const override;

  int getPovHatCount() const override;
  const char *getPovHatName(int idx) const override;

  int getButtonId(const char *name) const override;
  int getAxisId(const char *name) const override;
  int getPovHatId(const char *name) const override;
  void setAxisLimits(int axis_id, float min_val, float max_val) override;
  bool getButtonState(int btn_id) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;
  int getPovHatAngle(int axis_id) const override;

  IJoyFfCondition *createConditionFx(int condfx_type, const JoyFfCreateParams &cp) override;
  IJoyFfConstForce *createConstForceFx(const JoyFfCreateParams &cp, float force) override;
  IJoyFfPeriodic *createPeriodicFx(int period_type, const JoyFfCreateParams &cp) override;
  IJoyFfRampForce *createRampForceFx(const JoyFfCreateParams &cp, float start_f, float end_f) override;

  void doRumble(float lowFreq, float highFreq) {}
  void setLight(bool set, unsigned char r, unsigned char g, unsigned char b) {}
  bool isConnected();
  bool isXinputCompatible() const override { return shouldRemapAsX360(); }

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
