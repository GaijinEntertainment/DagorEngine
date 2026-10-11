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

//! exclude_sony_pads: DirectInput skips every pad of dsPadIds.h by its ids, whether or not the raw HID driver of
//! createSonyPadJoystickClassDriver() opened it; evdev skips it only when the game can read and write its hidraw
//! node (the udev rules), else the pad stays an evdev joystick; the other backends ignore the flag
IGenJoystickClassDrv *createJoystickClassDriver(bool exclude_xinput = false, bool remap_360 = false, bool exclude_sony_pads = false);
IGenJoystickClassDrv *createXinputJoystickClassDriver(bool should_mix_input = false);
//! DualSense, DualSense Edge and DualShock 4 over raw HID (Windows hid.dll, Linux hidraw); device id "ds4"
IGenJoystickClassDrv *createSonyPadJoystickClassDriver();
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
//! iOS: shows/hides screen keyboard
void showScreenKeyboard(bool show);
//! iOS: show IME keyboard
void showScreenKeyboardIME_iOS(const char *text, const char *hint, unsigned int ime_flag, unsigned int edit_flag, int cursor_pos,
  bool is_secure, int max_chars, void(on_finish_cb)(const char *str, int cursor));
#elif _TARGET_ANDROID
//! setup (calibrate) neutral joystick position (gravity sensor)
void setAsNeutralGsensor_android();
//! shows/hides screen keyboard
void showScreenKeyboard(bool show);
//! returns visibility of screen keyboard (works for IME too); 0=hidden, 1=IME, 2=KBD
int getScreenKeyboardStatus_android();
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
