/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GameControllerLog.h"

#include <android/keycodes.h>

#include "GameControllerLogStrings.h"
#include "Log.h"

#define LOG_TAG "GameControllerManager"
// Filter input event logging to qualifying 'gamecontroller' event sources
#define LOG_FILTER_PADDLEBOAT_SOURCES

#define ELEMENTS_OF(x) (sizeof(x) / sizeof(x[0]))

namespace paddleboat {
const char *LogGetInputSourceString(const int32_t eventSource) {
    const char *inputSourceString = "AINPUT_SOURCE_UNKNOWN";

    switch (eventSource) {
        case AINPUT_SOURCE_KEYBOARD:
            inputSourceString = "AINPUT_SOURCE_KEYBOARD";
            break;
        case AINPUT_SOURCE_DPAD:
            inputSourceString = "AINPUT_SOURCE_DPAD";
            break;
        case AINPUT_SOURCE_GAMEPAD:
            inputSourceString = "AINPUT_SOURCE_GAMEPAD";
            break;
        case AINPUT_SOURCE_TOUCHSCREEN:
            inputSourceString = "AINPUT_SOURCE_TOUCHSCREEN";
            break;
        case AINPUT_SOURCE_MOUSE:
            inputSourceString = "AINPUT_SOURCE_MOUSE";
            break;
        case AINPUT_SOURCE_STYLUS:
            inputSourceString = "AINPUT_SOURCE_STYLUS";
            break;
        case AINPUT_SOURCE_BLUETOOTH_STYLUS:
            inputSourceString = "AINPUT_SOURCE_BLUETOOTH_STYLUS";
            break;
        case AINPUT_SOURCE_MOUSE_RELATIVE:
            inputSourceString = "AINPUT_SOURCE_MOUSE_RELATIVE";
            break;
        case AINPUT_SOURCE_TOUCHPAD:
            inputSourceString = "AINPUT_SOURCE_TOUCHPAD";
            break;
        case AINPUT_SOURCE_TOUCH_NAVIGATION:
            inputSourceString = "AINPUT_SOURCE_TOUCH_NAVIGATION";
            break;
        case AINPUT_SOURCE_JOYSTICK:
            inputSourceString = "AINPUT_SOURCE_JOYSTICK";
            break;
        case AINPUT_SOURCE_ROTARY_ENCODER:
            inputSourceString = "AINPUT_SOURCE_ROTARY_ENCODER";
            break;
        default:
            break;
    }
    return inputSourceString;
}

void LogInputEvent(const AInputEvent *event) {
    const int32_t eventSource = AInputEvent_getSource(event);
#if defined LOG_FILTER_PADDLEBOAT_SOURCES
    if (!(eventSource == AINPUT_SOURCE_DPAD ||
          eventSource == AINPUT_SOURCE_GAMEPAD ||
          eventSource == AINPUT_SOURCE_JOYSTICK)) {
        return;
    }
#endif
    const int32_t eventDeviceId = AInputEvent_getDeviceId(event);
    const int32_t eventType = AInputEvent_getType(event);
    const char *inputSourceString = LogGetInputSourceString(eventSource);

    if (eventType == AINPUT_EVENT_TYPE_KEY) {
        const int32_t eventAction = AKeyEvent_getAction(event);
        const int32_t eventFlags = AKeyEvent_getFlags(event);
        const int32_t eventKeycode = AKeyEvent_getKeyCode(event);
        const char *actionString =
            (eventAction < ELEMENTS_OF(AKEY_ACTION_STRINGS) && eventAction >= 0)
                ? AKEY_ACTION_STRINGS[eventAction]
                : "AKEY_ACTION out of range";
        const char *keycodeString =
            (eventKeycode < ELEMENTS_OF(AKEYCODE_STRINGS) && eventKeycode >= 0)
                ? AKEYCODE_STRINGS[eventKeycode]
                : "AKEYCODE out of range";
        ALOGI(
            "LogInputEvent\nAINPUT_EVENT_TYPE_KEY deviceId %d source %s\n%s %s "
            "%08x",
            eventDeviceId, inputSourceString, actionString, keycodeString,
            eventFlags);
    } else if (eventType == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t eventAction =
            AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        const int32_t eventFlags = AMotionEvent_getFlags(event);
        const char *actionString =
            (eventAction < ELEMENTS_OF(AMOTION_ACTION_STRINGS) &&
             eventAction >= 0)
                ? AMOTION_ACTION_STRINGS[eventAction]
                : "AMOTION_ACTION out of range";
        ALOGI(
            "LogInputEvent\nAINPUT_EVENT_TYPE_MOTION deviceId %d source %s\n%s "
            "%08x",
            eventDeviceId, inputSourceString, actionString, eventFlags);
    }
}
}  // namespace paddleboat