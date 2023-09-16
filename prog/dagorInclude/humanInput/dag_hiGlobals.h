//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>
#include <humanInput/dag_hiKeybData.h>
#include <humanInput/dag_hiPointingData.h>
#include <humanInput/dag_hiJoyData.h>

#define MAX_WM_DEVICECHANGE_COUNT 50


namespace HumanInput
{
// global settings for human input devices
// (affects all devices of one class)
extern KeyboardSettings stg_kbd;
extern PointingSettings stg_pnt;
extern JoystickSettings stg_joy;

// global raw state for human input devices
// (can be filled by several devices of one class simultaneously)
extern KeyboardRawState raw_state_kbd;
extern PointingRawState raw_state_pnt;
extern JoystickRawState raw_state_joy;
} // namespace HumanInput
