// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _HID_TVOS_GAMEPAD_EX_H
#define _HID_TVOS_GAMEPAD_EX_H
#pragma once

#import <GameController/GameController.h>


class TvosGamepadEx : public HumanInput::IGenJoystick
{
  enum
  {
    BUTTONS_NUM = HumanInput::JOY_XINPUT_REAL_BTN_COUNT,
    AXES_NUM = HumanInput::JOY_XINPUT_REAL_AXIS_COUNT
  };

public:
  TvosGamepadEx(GCExtendedGamepad *gpad);

  // general routines
  virtual const char *getName() const { return "Gamepad"; }
  virtual const char *getDeviceID() const { return "Gamepad"; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const { return false; }
  virtual unsigned short getUserId() const { return 0; }
  virtual const HumanInput::JoystickRawState &getRawState() const { return rawState; }
  virtual void setClient(HumanInput::IGenJoystickClient *cli);
  virtual HumanInput::IGenJoystickClient *getClient() const;

  // buttons and axes description
  virtual int getButtonCount() const { return BUTTONS_NUM; }
  virtual const char *getButtonName(int idx) const;
  virtual int getAxisCount() const { return AXES_NUM; }
  virtual const char *getAxisName(int idx) const;
  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int idx) const { return ""; }

  // buttons and axes name resolver
  virtual int getButtonId(const char *name) const;
  virtual int getAxisId(const char *name) const;
  virtual int getPovHatId(const char *name) const;

  // axis limits remap setup
  virtual void setAxisLimits(int axis_id, float min_val, float max_val);

  // current state information
  virtual bool getButtonState(int btn_id) const;
  virtual float getAxisPos(int axis_id) const;
  virtual int getAxisPosRaw(int axis_id) const;
  // return angle in hundredths of degree [0-36000) or -1 if not pressed
  virtual int getPovHatAngle(int hat_id) const;

  // returns device connection status
  virtual bool isConnected();

  void update();

private:
  GCExtendedGamepad *_gamepad;
  HumanInput::JoystickRawState rawState;
  HumanInput::IGenJoystickClient *client;
  struct TvosAxis
  {
    float val;
    float min;
    float max;
    TvosAxis() : val(0), min(-1), max(1) {}
  };
  TvosAxis axis[AXES_NUM];
};

#endif //_HID_TVOS_GAMEPAD_EX_H
