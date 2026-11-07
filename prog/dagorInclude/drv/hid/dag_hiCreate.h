//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiDecl.h>
class DataBlock;

#define _TARGET_HAS_IME (_TARGET_C1 | _TARGET_C2 | _TARGET_ANDROID | _TARGET_IOS | _TARGET_PC | _TARGET_XBOX | _TARGET_C3)

namespace HumanInput
{
IGenKeyboardClassDrv *createWinKeyboardClassDriver();
IGenPointingClassDrv *createWinMouseClassDriver();

//! exclude_dspads: DirectInput skips every pad of dsPadIds.h by its ids, whether or not the raw HID driver of
//! createDsPadJoystickClassDriver() opened it; evdev skips it only when the game can read and write its hidraw
//! node (the udev rules), else the pad stays an evdev joystick; the other backends ignore the flag
IGenJoystickClassDrv *createJoystickClassDriver(bool exclude_xinput = false, bool remap_360 = false, bool exclude_dspads = false);
IGenJoystickClassDrv *createXinputJoystickClassDriver(bool should_mix_input = false);
//! DualSense, DualSense Edge and DualShock 4 over raw HID (Windows hid.dll, Linux hidraw); device id "ds4"
IGenJoystickClassDrv *createDsPadJoystickClassDriver();
//! Windows and Linux: true while Steam Input remaps a pad of the driver above into its virtual Xbox pad (on Windows
//! also hiding it from the game); the driver updates it on each device scan, and it is false without the driver
bool isDsPadRemappedBySteamInput();
IGenJoystickClassDrv *createGameInputJoystickClassDriver(bool should_mix_input = false);
IGenJoystickClassDrv *createGameInputFlightStickClassDriver();
CompositeJoystickClassDriver *createGameInputCompositeJoystickClassDriver(bool should_mix_input = false, bool add_flight_stick = true);
IGenKeyboardClassDrv *createGameInputKeyboardClassDriver();
IGenPointingClassDrv *createGameInputPointingClassDriver(bool emu_mouse = false, bool hw_mouse = false);

CompositeJoystickClassDriver *createPS4CompositeJoystickClassDriver();
IGenJoystickClassDrv *createPS4JoystickClassDriver();
IGenPointingClassDrv *createPS4PointingClassDriver();
IGenKeyboardClassDrv *createPS4KeyboardClassDriver();

//! tvOS: create default joystick siricontroller or nimbus gamepad
IGenJoystickClassDrv *createTvosJoystickClassDriver();

IGenPointingClassDrv *createMouseEmuClassDriver();

IGenKeyboardClassDrv *createNullKeyboardClassDriver();
IGenPointingClassDrv *createNullMouseClassDriver();
IGenJoystickClassDrv *createNullJoystickClassDriver();

#if _TARGET_IOS
//! iOS: setup (calibrate) neutral joystick position (gravity sensor)
void setAsNeutralGsensor_iOS();
//! iOS: returns the GCController (as void *) of the gamepad the joystick driver reads (device 0), nullptr when no
//! gamepad is connected or without the driver; the driver changes it on a device list refresh or a disconnect and
//! does not keep it retained for the caller. Main thread only
void *getGamepadController_iOS();
//! iOS: shows/hides screen keyboard (raw mode: typed chars are sent as GPCM_Char/DKEY_BACK)
void showScreenKeyboard(bool show);
//! iOS: parameters of the native edit field for showScreenKeyboardIME_iOS()
struct ScreenKeyboardParamsIOS
{
  //! UITextField.textContentType: tells AutoFill and QuickType what the field holds
  enum ContentType
  {
    CONTENT_NONE,     //!< no content type, no AutoFill suggestions
    CONTENT_PASSWORD, //!< UITextContentTypePassword: offers saved passwords from the keychain;
                      //!< it does not hide the text, set secure = true together with it
    CONTENT_EMAIL,    //!< UITextContentTypeEmailAddress: offers the user's e-mail addresses
  };

  const char *text = "";          //!< initial text, UTF-8; NULL or invalid UTF-8 - empty text
  const char *hint = "";          //!< placeholder, UTF-8; NULL, empty or invalid UTF-8 - no placeholder
  int keyboardType = 0;           //!< UIKeyboardType, 0 - UIKeyboardTypeDefault
  int returnKeyType = 0;          //!< UIReturnKeyType, 0 - UIReturnKeyDefault
  int autocapitalizationType = 0; //!< UITextAutocapitalizationType, 0 - UITextAutocapitalizationTypeNone
  ContentType contentType = CONTENT_NONE;
  bool secure = false; //!< hides the typed text (UITextField.secureTextEntry)
  int cursorPos = -1;  //!< in unicode codepoints, <0 - end of text
  int maxChars = 0;    //!< in unicode codepoints, <=0 - no limit
};
//! iOS: show IME keyboard; returns false when it can't be shown or another IME session is open (close it with
//! showScreenKeyboard(false) first), on_finish_cb is not called then; a keyboard open in raw mode
//! (showScreenKeyboard(true)) switches to this IME session;
//! otherwise on_finish_cb is called once, with IME_STATUS_CLOSED, when the edit field closes (Done or Return key,
//! tap outside the field, the field loses focus, or showScreenKeyboard(false)); it receives the text in UTF-8
//! and the cursor position in unicode codepoints
bool showScreenKeyboardIME_iOS(const ScreenKeyboardParamsIOS &params, void (*on_finish_cb)(const char *str, int cursor, int status));
#elif _TARGET_ANDROID
//! setup (calibrate) neutral joystick position (gravity sensor)
void setAsNeutralGsensor_android();
//! shows/hides screen keyboard
void showScreenKeyboard(bool show);
//! returns visibility of screen keyboard (works for IME too); 0=hidden, 1=IME, 2=KBD
int getScreenKeyboardStatus_android();
//! getGamepadDeviceId_android() result when no gamepad is connected
static constexpr int GAMEPAD_DEVICE_ID_NONE = -1;
//! returns the InputDevice id of the current gamepad, whose input and layout the joystick driver uses, or
//! GAMEPAD_DEVICE_ID_NONE; current is the first gamepad at start, then the one whose button was pressed (the id changes
//! on the press, the layout a frame or two later), an added gamepad or a device that became one, and the first
//! remaining one when the current is removed or stops being a gamepad; these device changes reach the value on the next
//! device list refresh
int getGamepadDeviceId_android();
#endif
#if _TARGET_HAS_IME
enum
{
  IME_STATUS_ERROR = -1,
  IME_STATUS_CANCELLED = 0,
  IME_STATUS_CLOSED = 1,
  IME_STATUS_UPDATED = 2,
};
typedef void (*OnUpdateIME)(void *userdata, const char *str, int cursor, int status);
typedef bool (*OnShowIME)(const DataBlock &init_params, OnUpdateIME on_update_cb, void *userdata);
//! returns true when showScreenKeyboard_IME() is supported by system
bool isImeAvailable();
//! shows IME (screen keyboard);
//! init_params is open-spec data container (see engine/workCycle/ps4/orbisIME.cpp for details)
//! supplied callback is called when input is finished;
//! return status: 1=OK, 0=cancelled, -1=aborted
bool showScreenKeyboard_IME(const DataBlock &init_params, OnUpdateIME on_update_cb, void *userdata);
#endif
} // namespace HumanInput
