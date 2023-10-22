#import <GameController/GameController.h>
#include <util/dag_globDef.h>
#include <humanInput/dag_hiXInputMappings.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiTvosMap.h>

#include "gamepad_ex.h"

#define SET_BUTTON(gpButton, X360_BUTTON)               \
  {                                                     \
    if(gamepad.gpButton.isPressed)                      \
        rawState.buttons.set(HumanInput::X360_BUTTON); \
    else                                                \
        rawState.buttons.clr(HumanInput::X360_BUTTON); \
  }

#define DO_BUTTON(gpButton, X360_BUTTON)          \
      if(gamepad.gpButton == element)             \
      SET_BUTTON(gpButton, X360_BUTTON)


TvosGamepadEx::TvosGamepadEx(GCExtendedGamepad *gpad): _gamepad(gpad), client(0)
{
  _gamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
  {
    DO_BUTTON(buttonX, JOY_XINPUT_REAL_BTN_X);
    DO_BUTTON(buttonY, JOY_XINPUT_REAL_BTN_Y);
    DO_BUTTON(buttonA, JOY_XINPUT_REAL_BTN_A);
    DO_BUTTON(buttonB, JOY_XINPUT_REAL_BTN_B);

    DO_BUTTON(leftTrigger, JOY_XINPUT_REAL_BTN_L_TRIGGER);
    DO_BUTTON(rightTrigger, JOY_XINPUT_REAL_BTN_R_TRIGGER);
    DO_BUTTON(leftShoulder, JOY_XINPUT_REAL_BTN_L_SHOULDER);
    DO_BUTTON(rightShoulder, JOY_XINPUT_REAL_BTN_R_SHOULDER);

    if(gamepad.dpad == element)
    {
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_TRIGGER].val = gamepad.dpad.xAxis.value;
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_TRIGGER].val = gamepad.dpad.yAxis.value;
      SET_BUTTON(dpad.up, JOY_XINPUT_REAL_BTN_D_UP);
      SET_BUTTON(dpad.down, JOY_XINPUT_REAL_BTN_D_DOWN);
      SET_BUTTON(dpad.left, JOY_XINPUT_REAL_BTN_D_LEFT);
      SET_BUTTON(dpad.right, JOY_XINPUT_REAL_BTN_D_RIGHT);
    }

    if(gamepad.leftThumbstick == element)
    {
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H].val = gamepad.leftThumbstick.xAxis.value;
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V].val = gamepad.leftThumbstick.yAxis.value;
      SET_BUTTON(leftThumbstick.up, JOY_XINPUT_REAL_BTN_L_THUMB_UP);
      SET_BUTTON(leftThumbstick.down, JOY_XINPUT_REAL_BTN_L_THUMB_DOWN);
      SET_BUTTON(leftThumbstick.left, JOY_XINPUT_REAL_BTN_L_THUMB_LEFT);
      SET_BUTTON(leftThumbstick.right, JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT);
    }

    if(gamepad.rightThumbstick == element)
    {
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_H].val = gamepad.rightThumbstick.xAxis.value;
      axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_V].val = gamepad.rightThumbstick.yAxis.value;
      SET_BUTTON(rightThumbstick.up, JOY_XINPUT_REAL_BTN_R_THUMB_UP);
      SET_BUTTON(rightThumbstick.down, JOY_XINPUT_REAL_BTN_R_THUMB_DOWN);
      SET_BUTTON(rightThumbstick.left, JOY_XINPUT_REAL_BTN_R_THUMB_LEFT);
      SET_BUTTON(rightThumbstick.right, JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT);
    }
  };
}

#undef SET_BUTTON
#undef DO_BUTTON

// general routines
void TvosGamepadEx::setClient ( HumanInput::IGenJoystickClient *cli )
{
  if ( cli == client )
    return;

  if ( client )
    client->detached ( this );
  client = cli;
  if ( client )
    client->attached ( this );
}

HumanInput::IGenJoystickClient *TvosGamepadEx::getClient() const
{
  return client;
}

// buttons and axes description
const char *TvosGamepadEx::getButtonName(int idx) const
{
  if (idx < 0 || idx >= BUTTONS_NUM)
    return NULL;
  return HumanInput::joyXInputButtonName[idx];
}
const char *TvosGamepadEx::getAxisName(int idx) const
{
  if (idx < 0 || idx >= AXES_NUM)
    return NULL;
  return HumanInput::joyXInputAxisName[idx];;
}
// buttons and axes name resolver
int TvosGamepadEx::getButtonId(const char *name) const
{
  for ( int i = 0; i < BUTTONS_NUM; i ++ )
    if ( strcmp ( name, HumanInput::joyXInputButtonName[i] ) == 0 )
      return i;
  return -1;
}

int TvosGamepadEx::getAxisId(const char *name) const
{
  for ( int i = 0; i < AXES_NUM; i ++ )
    if ( strcmp ( name, HumanInput::joyXInputAxisName[i] ) == 0 )
      return i;
  return -1;
}

int TvosGamepadEx::getPovHatId(const char *name) const{ return -1; }

// axis limits remap setup
void TvosGamepadEx::setAxisLimits ( int axis_id, float min_val, float max_val )
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);

  axis[axis_id].min = min_val;
  axis[axis_id].max = max_val;
}

// current state information
bool TvosGamepadEx::getButtonState ( int btn_id ) const
{
  G_ASSERT(btn_id >= 0 && btn_id < BUTTONS_NUM);
  return rawState.buttons.get(btn_id);
}
float TvosGamepadEx::getAxisPos ( int axis_id ) const
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  return (axis[axis_id].val + 1.0f) / 2.0f * (axis[axis_id].max - axis[axis_id].min) + axis[axis_id].min;
}
int TvosGamepadEx::getAxisPosRaw ( int axis_id ) const
{
  return *(int32_t *)&axis[axis_id].val; //read raw data is not good idea
}
// return angle in hundredths of degree [0-36000) or -1 if not pressed
int TvosGamepadEx::getPovHatAngle( int hat_id ) const {return -1;}

// returns device connection status
bool TvosGamepadEx::isConnected()
{
  return true;
}

void TvosGamepadEx::update()
{
  if (client)
    client->stateChanged(this, 0);

  rawState.buttonsPrev = rawState.buttons;
  rawState.buttons.clr(HumanInput::TVOS_REMOTE_BUTTON_MENU);
}
