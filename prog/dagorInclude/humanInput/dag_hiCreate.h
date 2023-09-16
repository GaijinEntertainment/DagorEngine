//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>
class DataBlock;

#define _TARGET_HAS_IME (_TARGET_C1 | _TARGET_C2 | _TARGET_ANDROID | _TARGET_IOS | _TARGET_PC | _TARGET_XBOX | _TARGET_C3)

namespace HumanInput
{
IGenKeyboardClassDrv *createWinKeyboardClassDriver();
IGenPointingClassDrv *createWinMouseClassDriver();

IGenJoystickClassDrv *createJoystickClassDriver(bool exclude_xinput = false, bool remap_360 = false);
IGenJoystickClassDrv *createXinputJoystickClassDriver(bool should_mix_input = false);
IGenJoystickClassDrv *createSteamJoystickClassDriver(const char *absolute_path_to_controller_config);

CompositeJoystickClassDriver *createPS4CompositeJoystickClassDriver();
IGenJoystickClassDrv *createPS4JoystickClassDriver();
IGenPointingClassDrv *createPS4PointingClassDriver();
IGenKeyboardClassDrv *createPS4KeyboardClassDriver();

//! tvOS: create default joystick siricontroller or nimbus gamepad
IGenJoystickClassDrv *createTvosJoystickClassDriver();

IGenPointingClassDrv *createMouseEmuClassDriver();
IGenKeyboardClassDrv *createKeyboardEmuClassDriver();

IGenKeyboardClassDrv *createNullKeyboardClassDriver();
IGenPointingClassDrv *createNullMouseClassDriver();
IGenJoystickClassDrv *createNullJoystickClassDriver();

#if _TARGET_IOS
//! iOS: setup (calibrate) neutral joystick position (gravity sensor)
void setAsNeutralGsensor_iOS();
//! iOS: shows/hides screen keyboard
void showScreenKeyboard(bool show);
//! iOS: show IME keyboard
void showScreenKeyboardIME_iOS(const char *text, const char *hint, unsigned int ime_flag, unsigned int edit_flag, bool isSecure,
  void(on_finish_cb)(const char *str));
#elif _TARGET_ANDROID
//! setup (calibrate) neutral joystick position (gravity sensor)
void setAsNeutralGsensor_android();
//! shows/hides screen keyboard
void showScreenKeyboard(bool show);
//! returns visibility of screen keyboard (works for IME too); 0=hidden, 1=IME, 2=KBD
int getScreenKeyboardStatus_android();
#endif
#if _TARGET_HAS_IME
typedef void (*OnFinishIME)(void *userdata, const char *str, int status);
typedef bool (*OnShowIME)(const DataBlock &init_params, OnFinishIME on_finish_cb, void *userdata);
//! returns true when showScreenKeyboard_IME() is supported by system
bool isImeAvailable();
//! shows IME (screen keyboard);
//! init_params is open-spec data container (see engine2/workCycle/ps4/orbisIME.cpp for details)
//! supplied callback is called when input is finished;
//! return status: 1=OK, 0=cancelled, -1=aborted
bool showScreenKeyboard_IME(const DataBlock &init_params, OnFinishIME on_finish_cb, void *userdata);
#endif
} // namespace HumanInput
