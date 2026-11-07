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

#include "GameController.h"

#include <android/input.h>
#include <math.h>

#include <chrono>
#include <cstdlib>
#include <memory>

#include "GameControllerInternalConstants.h"

namespace paddleboat {
struct ControllerButtonMap {
    int32_t buttonKeycodes[PADDLEBOAT_BUTTON_COUNT];
};

const ControllerButtonMap defaultButtonMap = {{
    AKEYCODE_DPAD_UP,        // PADDLEBOAT_BUTTON_DPAD_UP
    AKEYCODE_DPAD_LEFT,      // PADDLEBOAT_BUTTON_DPAD_LEFT
    AKEYCODE_DPAD_DOWN,      // PADDLEBOAT_BUTTON_DPAD_DOWN
    AKEYCODE_DPAD_RIGHT,     // PADDLEBOAT_BUTTON_DPAD_RIGHT
    AKEYCODE_BUTTON_A,       // PADDLEBOAT_BUTTON_A
    AKEYCODE_BUTTON_B,       // PADDLEBOAT_BUTTON_B
    AKEYCODE_BUTTON_X,       // PADDLEBOAT_BUTTON_X
    AKEYCODE_BUTTON_Y,       // PADDLEBOAT_BUTTON_Y
    AKEYCODE_BUTTON_L1,      // PADDLEBOAT_BUTTON_L1
    AKEYCODE_BUTTON_L2,      // PADDLEBOAT_BUTTON_L2
    AKEYCODE_BUTTON_THUMBL,  // PADDLEBOAT_BUTTON_L3
    AKEYCODE_BUTTON_R1,      // PADDLEBOAT_BUTTON_R1
    AKEYCODE_BUTTON_R2,      // PADDLEBOAT_BUTTON_R2
    AKEYCODE_BUTTON_THUMBR,  // PADDLEBOAT_BUTTON_R3
    AKEYCODE_BUTTON_SELECT,  // PADDLEBOAT_BUTTON_SELECT
    AKEYCODE_BUTTON_START,   // PADDLEBOAT_BUTTON_START
    AKEYCODE_BUTTON_MODE,    // PADDLEBOAT_BUTTON_SYSTEM
    0,                       // PADDLEBOAT_BUTTON_TOUCHPAD
    0,                       // PADDLEBOAT_BUTTON_AUX1
    0,                       // PADDLEBOAT_BUTTON_AUX2
    0,                       // PADDLEBOAT_BUTTON_AUX3
    0                        // PADDLEBOAT_BUTTON_AUX4
}};

// Axis must be at least this value to trigger a mapped button press
constexpr float AXIS_BUTTON_THRESHOLD = 0.1f;

void resetData(Paddleboat_Controller_Data &pbData) {
    pbData.timestamp = 0;
    pbData.buttonsDown = 0;
    pbData.leftStick.stickX = 0.0f;
    pbData.leftStick.stickY = 0.0f;
    pbData.rightStick.stickX = 0.0f;
    pbData.rightStick.stickY = 0.0f;
    pbData.triggerL1 = 0.0f;
    pbData.triggerL2 = 0.0f;
    pbData.triggerR1 = 0.0f;
    pbData.triggerR2 = 0.0f;
    pbData.virtualPointer.pointerX = 0.0f;
    pbData.virtualPointer.pointerY = 0.0f;
}

void resetInfo(Paddleboat_Controller_Info &pbInfo) {
    pbInfo.controllerNumber = -1;
    pbInfo.controllerFlags = 0;
    pbInfo.vendorId = 0;
    pbInfo.productId = 0;
    pbInfo.deviceId = -1;
    pbInfo.leftStickPrecision.stickFlatX = 0.0f;
    pbInfo.leftStickPrecision.stickFlatY = 0.0f;
    pbInfo.leftStickPrecision.stickFuzzX = 0.0f;
    pbInfo.leftStickPrecision.stickFuzzY = 0.0f;
    pbInfo.rightStickPrecision.stickFlatX = 0.0f;
    pbInfo.rightStickPrecision.stickFlatY = 0.0f;
    pbInfo.rightStickPrecision.stickFuzzX = 0.0f;
    pbInfo.rightStickPrecision.stickFuzzY = 0.0f;
}

GameController::GameController()
    : mConnectionIndex(-1),
      mControllerData(),
      mControllerInfo(),
      mAxisInfo(),
      mDeviceInfo(),
      mControllerDataDirty(true) {
    memset(mButtonKeycodes, 0, sizeof(mButtonKeycodes));
    resetControllerData();
}

void GameController::resetControllerData() {
    resetData(mControllerData);
    resetInfo(mControllerInfo);
}

void GameController::setupController(
        const Paddleboat_Controller_Mapping_File_Controller_Entry *controllerEntry,
        const Paddleboat_Controller_Mapping_File_Axis_Entry *axisEntry,
        const Paddleboat_Controller_Mapping_File_Button_Entry *buttonEntry) {

    const GameControllerDeviceInfo::InfoFields &infoFields =
            *(mDeviceInfo.getInfo());
    uint64_t axisLow = static_cast<uint64_t>(infoFields.mAxisBitsLow);
    uint64_t axisHigh = static_cast<uint64_t>(infoFields.mAxisBitsHigh);
    mControllerAxisMask = axisLow | (axisHigh << 32ULL);
    mControllerInfo.controllerFlags = infoFields.mControllerFlags;
    mControllerInfo.controllerNumber = infoFields.mControllerNumber;
    mControllerInfo.deviceId = infoFields.mDeviceId;
    mControllerInfo.productId = infoFields.mProductId;
    mControllerInfo.vendorId = infoFields.mVendorId;

    if (controllerEntry != nullptr) {
        mControllerInfo.controllerFlags |= controllerEntry->flags;
        mAxisInversionBitmask = axisEntry->axisInversionBitmask;
        for (int32_t i = 0; i < PADDLEBOAT_BUTTON_COUNT; ++i) {
            if (buttonEntry->buttonMapping[i] != PADDLEBOAT_BUTTON_IGNORED) {
                mButtonKeycodes[i] = buttonEntry->buttonMapping[i];
            }
        }

        for (int32_t i = 0; i < PADDLEBOAT_MAPPING_AXIS_COUNT; ++i) {
            if (axisEntry->axisMapping[i] != PADDLEBOAT_AXIS_IGNORED) {
                bool axisInvert = ((mAxisInversionBitmask & (1 << i)) != 0);
                const GameControllerAxis gcAxis =
                        static_cast<GameControllerAxis>(i);
                const int32_t nativeAxisId = axisEntry->axisMapping[i];
                const int32_t positiveButton =
                        (axisEntry->axisPositiveButtonMapping[i] ==
                         PADDLEBOAT_AXIS_BUTTON_IGNORED)
                        ? 0
                        : (1 << axisEntry->axisPositiveButtonMapping[i]);
                const int32_t negativeButton =
                        (axisEntry->axisNegativeButtonMapping[i] ==
                         PADDLEBOAT_AXIS_BUTTON_IGNORED)
                        ? 0
                        : (1 << axisEntry->axisNegativeButtonMapping[i]);
                setupAxis(gcAxis, nativeAxisId, nativeAxisId, positiveButton,
                          negativeButton, axisInvert);
            }
        }
        adjustAxisConstants();

    } else {
        // Fallback defaults if there wasn't mapping data provided
        initializeDefaultAxisMapping();
        memcpy(mButtonKeycodes, &defaultButtonMap, sizeof(ControllerButtonMap));
        mControllerInfo.controllerFlags |=
                PADDLEBOAT_CONTROLLER_FLAG_GENERIC_PROFILE;
    }
}

void GameController::initializeDefaultAxisMapping() {
    setupAxis(GAMECONTROLLER_AXIS_LSTICK_X, AMOTION_EVENT_AXIS_X,
              AMOTION_EVENT_AXIS_X, 0, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_LSTICK_Y, AMOTION_EVENT_AXIS_Y,
              AMOTION_EVENT_AXIS_Y, 0, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_RSTICK_X, AMOTION_EVENT_AXIS_Z,
              AMOTION_EVENT_AXIS_RX, 0, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_RSTICK_Y, AMOTION_EVENT_AXIS_RZ,
              AMOTION_EVENT_AXIS_RY, 0, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_L2, AMOTION_EVENT_AXIS_LTRIGGER,
              AMOTION_EVENT_AXIS_BRAKE, PADDLEBOAT_BUTTON_L2, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_R2, AMOTION_EVENT_AXIS_RTRIGGER,
              AMOTION_EVENT_AXIS_GAS, PADDLEBOAT_BUTTON_R2, 0, false);
    setupAxis(GAMECONTROLLER_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_X,
              AMOTION_EVENT_AXIS_HAT_X, PADDLEBOAT_BUTTON_DPAD_RIGHT,
              PADDLEBOAT_BUTTON_DPAD_LEFT, false);
    setupAxis(GAMECONTROLLER_AXIS_HAT_Y, AMOTION_EVENT_AXIS_HAT_Y,
              AMOTION_EVENT_AXIS_HAT_Y, PADDLEBOAT_BUTTON_DPAD_DOWN,
              PADDLEBOAT_BUTTON_DPAD_UP, false);
    adjustAxisConstants();
}

void GameController::adjustAxisConstants() {
    if (mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X].axisIndex >= 0) {
        const bool stickAxisAdjust =
            ((mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X].axisFlags &
              GAMECONTROLLER_AXIS_FLAG_APPLY_ADJUSTMENTS) != 0);

        mControllerInfo.leftStickPrecision.stickFlatX =
            mDeviceInfo.getFlatArray()[mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X]
                                           .axisIndex];
        mControllerInfo.leftStickPrecision.stickFlatY =
            mDeviceInfo.getFlatArray()[mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_Y]
                                           .axisIndex];
        mControllerInfo.leftStickPrecision.stickFuzzX =
            mDeviceInfo.getFuzzArray()[mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X]
                                           .axisIndex];
        mControllerInfo.leftStickPrecision.stickFuzzY =
            mDeviceInfo.getFuzzArray()[mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_Y]
                                           .axisIndex];
        if (stickAxisAdjust) {
            // We are adjusting the raw axis values, so we also adjust the
            // 'flat' and 'fuzz' values for the sticks
            mControllerInfo.leftStickPrecision.stickFlatX *=
                mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X].axisMultiplier;
            mControllerInfo.leftStickPrecision.stickFlatY *=
                mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_Y].axisMultiplier;
            mControllerInfo.leftStickPrecision.stickFuzzX *=
                mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_X].axisMultiplier;
            mControllerInfo.leftStickPrecision.stickFuzzY *=
                mAxisInfo[GAMECONTROLLER_AXIS_LSTICK_Y].axisMultiplier;
        }
    }

    if (mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X].axisIndex >= 0) {
        const bool stickAxisAdjust =
            ((mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X].axisFlags &
              GAMECONTROLLER_AXIS_FLAG_APPLY_ADJUSTMENTS) != 0);

        mControllerInfo.rightStickPrecision.stickFlatX =
            mDeviceInfo.getFlatArray()[mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X]
                                           .axisIndex];
        mControllerInfo.rightStickPrecision.stickFlatY =
            mDeviceInfo.getFlatArray()[mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_Y]
                                           .axisIndex];
        mControllerInfo.rightStickPrecision.stickFuzzX =
            mDeviceInfo.getFuzzArray()[mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X]
                                           .axisIndex];
        mControllerInfo.rightStickPrecision.stickFuzzY =
            mDeviceInfo.getFuzzArray()[mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_Y]
                                           .axisIndex];
        if (stickAxisAdjust) {
            // We are adjusting the raw axis values, so we also adjust the
            // 'flat' and 'fuzz' values for the sticks
            mControllerInfo.rightStickPrecision.stickFlatX *=
                fabs(mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X].axisMultiplier);
            mControllerInfo.rightStickPrecision.stickFlatY *=
                fabs(mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_Y].axisMultiplier);
            mControllerInfo.rightStickPrecision.stickFuzzX *=
                fabs(mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_X].axisMultiplier);
            mControllerInfo.rightStickPrecision.stickFuzzY *=
                fabs(mAxisInfo[GAMECONTROLLER_AXIS_RSTICK_Y].axisMultiplier);
        }
    }
}

void GameController::setupAxis(const GameControllerAxis gcAxis,
                               const int32_t preferredNativeAxisId,
                               const int32_t secondaryNativeAxisId,
                               const int32_t buttonMask,
                               const int32_t buttonNegativeMask,
                               const bool axisInvert) {
    mAxisInfo[gcAxis].resetInfo();
    mAxisInfo[gcAxis].axisButtonMask = buttonMask;
    mAxisInfo[gcAxis].axisButtonNegativeMask = buttonNegativeMask;
    mAxisInfo[gcAxis].axisInvert = axisInvert;

    // Do we have a mapping for the preferred native axis?
    if (preferredNativeAxisId < MAX_AXIS_COUNT &&
        secondaryNativeAxisId < MAX_AXIS_COUNT) {
        const int32_t preferredMask = (1U << preferredNativeAxisId);
        const int32_t secondaryMask = (1U << secondaryNativeAxisId);
        if ((mControllerAxisMask & preferredMask) != 0) {
            // preferred axis is mapped
            mAxisInfo[gcAxis].axisIndex = preferredNativeAxisId;
        } else if ((preferredNativeAxisId != secondaryNativeAxisId) &&
                   ((mControllerAxisMask & secondaryMask) != 0)) {
            // secondary axis is mapped
            mAxisInfo[gcAxis].axisIndex = secondaryNativeAxisId;
        } else if (buttonMask) {
            // There wasn't a matching axis,
            // but we will fake this axis using a digital button mapping
            mAxisInfo[gcAxis].axisFlags |=
                GAMECONTROLLER_AXIS_FLAG_DIGITAL_TRIGGER;
        }

        // If we found a native axis, check its ranges and see if we need to set
        // up adjustment values if the ranges aren't -1.0 to 1.0 for a stick or
        // 0.0 to 1.0 for a trigger
        if (mAxisInfo[gcAxis].axisIndex >= 0) {
            bool isStickAxis = (!(gcAxis >= GAMECONTROLLER_AXIS_L1 &&
                                  gcAxis <= GAMECONTROLLER_AXIS_R2));
            const float minAdjust = isStickAxis ? 1.0f : 0.0f;
            const float rawMin =
                mDeviceInfo.getMinArray()[mAxisInfo[gcAxis].axisIndex];
            const float rawMax =
                mDeviceInfo.getMaxArray()[mAxisInfo[gcAxis].axisIndex];
            const float diffMin = fabsf(rawMin + minAdjust);
            const float diffMax = fabsf(1.0f - rawMax);
            if (!(diffMin <= FLT_MIN && diffMax <= FLT_MIN)) {
                mAxisInfo[gcAxis].axisFlags |=
                    GAMECONTROLLER_AXIS_FLAG_APPLY_ADJUSTMENTS;
                if (isStickAxis) {
                    // normalize min and max axis to 1.0
                    mAxisInfo[gcAxis].axisMultiplier = 1.0f / rawMax;
                    // Make sure center of stick is 0.0
                    const float rawCenter = ((rawMax - rawMin) * 0.5f) + rawMin;
                    if (rawCenter >= FLT_MIN) {
                        mAxisInfo[gcAxis].axisAdjust =
                            -(rawCenter * mAxisInfo[gcAxis].axisMultiplier);
                    }
                } else {
                    // This case is hit on PS5 API <= 30 having weird trigger
                    // axis mappings of: L2=RX R2=RY with a -1.0 to 1.0 range
                    mAxisInfo[gcAxis].axisMultiplier = 1.0f / (rawMax - rawMin);
                    mAxisInfo[gcAxis].axisAdjust =
                        (-rawMin) * mAxisInfo[gcAxis].axisMultiplier;
                }
            }
        }
    }
}

int32_t GameController::processGameActivityKeyEvent(
    const Paddleboat_GameActivityKeyEvent *event, const size_t eventSize) {
    return processKeyEventInternal(event->keyCode, event->action);
}

int32_t GameController::processGameActivityMotionEvent(const float *axisValues) {
    return processMotionEventInternal(axisValues, nullptr);
}

int32_t GameController::processKeyEvent(const AInputEvent *event) {
    const int32_t eventKeyCode = AKeyEvent_getKeyCode(event);
    const int32_t eventKeyAction = AKeyEvent_getAction(event);
    return processKeyEventInternal(eventKeyCode, eventKeyAction);
}

int32_t GameController::processKeyEventInternal(const int32_t eventKeyCode,
                                                const int32_t eventKeyAction) {
    int32_t handledEvent = IGNORED_EVENT;
    int32_t buttonMask = 0;
    const bool bDown = (eventKeyAction == AKEY_EVENT_ACTION_DOWN);

    for (uint32_t i = 0; i < PADDLEBOAT_BUTTON_COUNT; ++i) {
        if (eventKeyCode == mButtonKeycodes[i]) {
            int32_t newButtonFlag = (1 << i);
            buttonMask |= newButtonFlag;

            // If we have no analog axis mapped for a trigger, allow the
            // button events to set the analog trigger data
            switch (newButtonFlag) {
                case PADDLEBOAT_BUTTON_L1:
                    if (mAxisInfo[GAMECONTROLLER_AXIS_L1].axisIndex == -1) {
                        mControllerData.triggerL1 = bDown ? 1.0f : 0.0f;
                    }
                    break;
                case PADDLEBOAT_BUTTON_L2:
                    if (mAxisInfo[GAMECONTROLLER_AXIS_L2].axisIndex == -1) {
                        mControllerData.triggerL2 = bDown ? 1.0f : 0.0f;
                    }
                    break;
                case PADDLEBOAT_BUTTON_R1:
                    if (mAxisInfo[GAMECONTROLLER_AXIS_R1].axisIndex == -1) {
                        mControllerData.triggerR1 = bDown ? 1.0f : 0.0f;
                    }
                    break;
                case PADDLEBOAT_BUTTON_R2:
                    if (mAxisInfo[GAMECONTROLLER_AXIS_R2].axisIndex == -1) {
                        mControllerData.triggerR2 = bDown ? 1.0f : 0.0f;
                    }
                    break;
                default:
                    break;
            }

            break;
        }
    }

    if (buttonMask != 0) {
        if (bDown) {
            mControllerData.buttonsDown |= buttonMask;
            setControllerDataDirty(true);
        } else {
            mControllerData.buttonsDown &= (~buttonMask);
            setControllerDataDirty(true);
        }
        handledEvent = HANDLED_EVENT;
    }
    return handledEvent;
}

int32_t GameController::processMotionEvent(const AInputEvent *event) {
    return processMotionEventInternal(nullptr, event);
}

int32_t GameController::processMotionEventInternal(const float *axisArray,
                                                   const AInputEvent *event) {
    int32_t handledEvent = IGNORED_EVENT;

    for (uint32_t axis = GAMECONTROLLER_AXIS_LSTICK_X;
         axis < GAMECONTROLLER_AXIS_COUNT; ++axis) {
        if (mAxisInfo[axis].axisIndex >= 0 &&
            mAxisInfo[axis].axisIndex <
                PADDLEBOAT_GAME_ACTIVITY_POINTER_INFO_AXIS_COUNT) {
            float axisValue = 0.0f;
            if (axisArray != nullptr) {
                axisValue = axisArray[mAxisInfo[axis].axisIndex];
            } else if (event != nullptr) {
                axisValue = AMotionEvent_getAxisValue(
                    event, mAxisInfo[axis].axisIndex, 0);
            }
            if ((mAxisInfo[axis].axisFlags &
                 GAMECONTROLLER_AXIS_FLAG_APPLY_ADJUSTMENTS) != 0) {
                axisValue = ((axisValue * mAxisInfo[axis].axisMultiplier) +
                             mAxisInfo[axis].axisAdjust);
            }
            if (mAxisInfo[axis].axisInvert) {
                axisValue = -axisValue;
            }

            // We take advantage of the GameControllerAxis matching the axis
            // order in the Paddleboat_Controller_Data struct to use the current
            // axis as an index into the axis entries in the
            // Paddleboat_Controller_Data struct
            if (axis < GAMECONTROLLER_AXIS_HAT_X) {
                float *axisData = &mControllerData.leftStick.stickX;
                axisData[axis] = axisValue;
            }

            // If this axis has a button associated with it, set/clear the flags
            // as appropriate
            if (mAxisInfo[axis].axisButtonMask != 0 ||
                mAxisInfo[axis].axisButtonNegativeMask) {
                if (axisValue > -AXIS_BUTTON_THRESHOLD &&
                    axisValue < AXIS_BUTTON_THRESHOLD) {
                    const uint32_t buttonMask =
                        mAxisInfo[axis].axisButtonMask |
                        mAxisInfo[axis].axisButtonNegativeMask;
                    mControllerData.buttonsDown &= (~buttonMask);
                } else if (axisValue > AXIS_BUTTON_THRESHOLD) {
                    mControllerData.buttonsDown |=
                        mAxisInfo[axis].axisButtonMask;
                } else if (axisValue < -AXIS_BUTTON_THRESHOLD) {
                    mControllerData.buttonsDown |=
                        mAxisInfo[axis].axisButtonNegativeMask;
                }
            }

            setControllerDataDirty(true);
            handledEvent = HANDLED_EVENT;
        }
    }
    return handledEvent;
}

void GameController::setControllerDataDirty(const bool dirty) {
    mControllerDataDirty = dirty;
    if (dirty) {
        // update the timestamp any time we mark dirty
        const auto timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        mControllerData.timestamp = static_cast<uint64_t>(timestamp);
    }
}

}  // namespace paddleboat