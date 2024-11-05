//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace HumanInput
{
// common HID classes
class IGenHidClassDriver;


// keyboard data
struct KeyboardSettings;
struct KeyboardRawState;

// keyboard driver classes
class IGenKeyboard;
class IGenKeyboardClient;
class IGenKeyboardClassDrv;


// mouse data
struct PointingSettings;
struct PointingRawState;

// mouse driver classes
class IGenPointing;
class IGenPointingClient;
class IGenPointingClassDrv;
class CompositeJoystickClassDriver;


// joystick data
struct JoystickSettings;
struct JoystickRawState;

// joystick driver classes
class IGenJoystick;
class IGenJoystickClient;
class IGenJoystickClassDrv;

// joystick force feedback effects
struct JoyFfCreateParams;
class IGenJoyFfEffect;
class IJoyFfCondition;
class IJoyFfConstForce;
class IJoyFfPeriodic;
class IJoyFfRampForce;

// VR input device
struct VrInput;
}; // namespace HumanInput
