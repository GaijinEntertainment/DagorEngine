// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _HID_TVOS_REMOTE_CONTROL_H
#define _HID_TVOS_REMOTE_CONTROL_H

#pragma once
#import <GameController/GameController.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <UIKit/UIPress.h>

#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <humanInput/dag_hiTvosMap.h>
#include <humanInput/dag_hiXInputMappings.h>

#include <debug/dag_debug.h>

using namespace HumanInput;


@protocol TvosInputDeviceUpdate

- (void)update;
- (HumanInput::IGenJoystick *)getJoystick;

@end

@interface TvosInputDevices : NSObject <TvosInputDeviceUpdate>

- (id)init;
- (void)controllerDidDisconnect:(NSNotification *)notification;
- (void)controllerDidConnect:(NSNotification *)notification;
- (void)reconnectDevices;
- (void)update;
- (int)count;
- (NSObject<TvosInputDeviceUpdate> *)getItem:(int)idx;
- (void)setClient:(HumanInput::IGenJoystickClient *)cli;
- (HumanInput::IGenJoystick *)getJoystick;
- (bool)isChanged;
@property(nonatomic, retain) NSMutableArray *controllers;
@end

@interface TvosJoystickController : NSObject <TvosInputDeviceUpdate>

- (id)initWith:(GCExtendedGamepad *)extendedGamepad;
- (void)dealloc;
- (void)update;
@end

@interface TvosRemoteController : UIGestureRecognizer <TvosInputDeviceUpdate>

- (id)initWith:(GCController *)controller andMenuButtonReceiver:(TvosInputDevices *)menuButtonDetector;
- (void)setViewController:(GCEventViewController *)gvc;
- (void)update;
@end


class TvosRemoteControl : public HumanInput::IGenJoystick
{
public:
  TvosRemoteControl()
  {
    for (int i = 0; i < AXES_NUM; ++i)
    {
      minAxis[i] = -1;
      maxAxis[i] = 1;
    }
  };

  float clamp_axis(int Axis_id, float value) const
  {
    float v = 2. * (value - minAxis[Axis_id]) / (maxAxis[Axis_id] - minAxis[Axis_id]) - 1;
    return (v < -1) ? -1 : (v > 1) ? 1 : v;
  }

  virtual const char *getName() const { return "Siri remote"; }

  virtual const char *getDeviceID() const { return "Apple_TV_REMOTE_CONTROLLER"; }

  virtual bool getDeviceDesc(DataBlock &out_desc) const { return false; };
  virtual unsigned short getUserId() const { return 0; }
  virtual const HumanInput::JoystickRawState &getRawState() const { return rawState; }

  virtual void setClient(HumanInput::IGenJoystickClient *cli)
  {
    debug("TvosRemoteControl::setClient(%p)", cli);
    client = cli;
  }

  virtual HumanInput::IGenJoystickClient *getClient() const { return client; }

  // buttons and axes description
  virtual int getButtonCount() const { return TVOS_REMOTE_BUTTONS_END; }

  virtual const char *getButtonName(int idx) const
  {
    if (idx >= TVOS_REMOTE_BUTTONS_BEGIN && idx < TVOS_REMOTE_BUTTONS_END)
      return tvosRemoteButtonName[idx - TVOS_REMOTE_BUTTONS_BEGIN];
    return "Undefined button";
  }

  virtual int getAxisCount() const { return AXES_NUM; }

  virtual const char *getAxisName(int idx) const
  {
    if (idx < 0 || idx >= AXES_NUM)
      return NULL;
    return HumanInput::joyXInputAxisName[idx];
    ;
  }

  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int idx) const { return "Undefined PovHat"; }

  // buttons and axes name resolver
  virtual int getButtonId(const char *name) const
  {
    for (int i = 0; i < TVOS_REMOTE_BUTTONS_COUNT; ++i)
    {
      if (strcmp(tvosRemoteButtonName[i], name) == 0)
        return i + TVOS_REMOTE_BUTTONS_BEGIN;
    }
    return -1;
  }

  virtual int getAxisId(const char *name) const
  {
    for (int i = 0; i < AXES_NUM; i++)
      if (strcmp(name, HumanInput::joyXInputAxisName[i]) == 0)
        return i;
    return -1;
  }
  virtual int getPovHatId(const char *name) const { return -1; }

  // axis limits remap setup
  virtual void setAxisLimits(int axis_id, float min_val, float max_val)
  {
    if (axis_id > 0 && axis_id < AXES_NUM)
    {
      minAxis[axis_id] = min_val;
      maxAxis[axis_id] = max_val;
    }
  };

  // current state information
  virtual bool getButtonState(int btn_id) const { return btn_id >= 0 ? rawState.buttons.get(btn_id) : false; }
  virtual float getAxisPos(int axis_id) const
  {
    if (axis_id == JOY_XINPUT_REAL_AXIS_L_THUMB_H)
    {
      return clamp_axis(axis_id, rawState.sensorX);
      // if(getButtonState(TVOS_REMOTE_ROTATION_LEFT))
      //   return 1;
      // else if(getButtonState(TVOS_REMOTE_ROTATION_RIGHT))
      //   return -1;
    }
    else if (axis_id == JOY_XINPUT_REAL_AXIS_L_THUMB_V)
    {
      return clamp_axis(axis_id, rawState.sensorY);
      // if(getButtonState(TVOS_REMOTE_MOVEMENT_FORWARD))
      //   return 1;
      // else if(getButtonState(TVOS_REMOTE_MOVEMENT_BACK))
      //   return -1;
    }
    /*for camera rotation*/
    if (axis_id == JOY_XINPUT_REAL_AXIS_R_THUMB_H)
    {
      return clamp_axis(axis_id, -rawState.sensorX);
      // if(getButtonState(TVOS_REMOTE_ROTATION_LEFT))
      //   return -1;
      // else if(getButtonState(TVOS_REMOTE_ROTATION_RIGHT))
      //   return 1;
    }
    return 0;
  }

  virtual int getAxisPosRaw(int axis_id) const { return 0; }
  // return angle in hundredths of degree [0-36000) or -1 if not pressed

  virtual int getPovHatAngle(int hat_id) const { return -1; }

  // returns device connection status
  virtual bool isConnected() { return true; }

  void update()
  {
    if (client)
      client->stateChanged(this, 0);
    rawState.buttonsPrev = rawState.buttons;

    rawState.buttons.clr(TVOS_REMOTE_MOVE_UP);
    rawState.buttons.clr(TVOS_REMOTE_MOVE_DOWN);
    rawState.buttons.clr(TVOS_REMOTE_MOVE_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_MOVE_RIGHT);

    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_UP);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_DOWN);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_UP_DOWN);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_DOWN_UP);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_LEFT_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_SWIPE_RIGHT_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_BUTTON_MENU);
    clearTaps();
  }

  void clearTaps()
  {
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_TOP_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_TOP_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_TOP_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_BOTTOM_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_BUTTOM_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_BOTTOM_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_CENTER_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_TAP_CENTER_RIGHT);
  }
  void clearClicks()
  {
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_TOP_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_TOP_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_TOP_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_CENTER_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_CENTER_RIGHT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_BOTTOM_LEFT);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_BUTTOM_CENTER);
    rawState.buttons.clr(TVOS_REMOTE_DPAD_CLICK_BOTTOM_RIGHT);
  }

private:
  enum
  {
    BUTTONS_NUM = TVOS_REMOTE_BUTTONS_END,
    AXES_NUM = HumanInput::JOY_XINPUT_REAL_AXIS_COUNT
  };

  HumanInput::IGenJoystickClient *client;
  HumanInput::JoystickRawState rawState;
  float minAxis[AXES_NUM];
  float maxAxis[AXES_NUM];
};

#endif //_HID_TVOS_REMOTE_CONTROL_H
