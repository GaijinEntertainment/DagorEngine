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

#pragma once

#include <android/input.h>

#include "GameControllerDeviceInfo.h"
#include "GameControllerGameActivityMirror.h"
#include "GameControllerMappingFile.h"
#include "paddleboat.h"

namespace paddleboat {

// set if axisMultiplier/axisAdjustment should be applied (raw device axis isn't
// -1.0 to 1.0)
inline constexpr uint32_t GAMECONTROLLER_AXIS_FLAG_APPLY_ADJUSTMENTS =
    (1U << 0);
// set if trigger is being faked as an analog axis (device just has a on/off
// button flag)
inline constexpr uint32_t GAMECONTROLLER_AXIS_FLAG_DIGITAL_TRIGGER = (1U << 1);

class GameController {
   public:
    enum GameControllerAxis {
        GAMECONTROLLER_AXIS_LSTICK_X = 0,
        GAMECONTROLLER_AXIS_LSTICK_Y,
        GAMECONTROLLER_AXIS_RSTICK_X,
        GAMECONTROLLER_AXIS_RSTICK_Y,
        GAMECONTROLLER_AXIS_L1,
        GAMECONTROLLER_AXIS_L2,
        GAMECONTROLLER_AXIS_R1,
        GAMECONTROLLER_AXIS_R2,
        GAMECONTROLLER_AXIS_HAT_X,
        GAMECONTROLLER_AXIS_HAT_Y,
        GAMECONTROLLER_AXIS_COUNT
    };

    struct GameControllerAxisInfo {
        // Index into the device axis array, -1 is unmapped
        int32_t axisIndex = -1;
        // See GAMECONTROLLER_AXIS_FLAG constants
        uint32_t axisFlags = 0;
        // Button mask flag, if backed/shadowed by digital button when axis >
        // 0.0
        uint32_t axisButtonMask = 0;
        // Button mask flag, if backed/shadowed by digital button when axis <
        // 0.0
        uint32_t axisButtonNegativeMask = 0;
        // Multiplier to normalize to a 0.0 center -> 1.0 edge
        float axisMultiplier = 1.0f;
        // Adjustment to bring center to 0.0 if necessary
        float axisAdjust = 0.0f;
        // Invert the axis data
        bool axisInvert = false;

        void resetInfo() {
            axisIndex = -1;
            axisFlags = 0;
            axisButtonMask = 0;
            axisButtonNegativeMask = 0;
            axisMultiplier = 1.0f;
            axisAdjust = 0.0f;
            axisInvert = false;
        }
    };

    GameController();

    void setupController(const Paddleboat_Controller_Mapping_File_Controller_Entry
        *controllerEntry, const Paddleboat_Controller_Mapping_File_Axis_Entry *axisEntry,
        const Paddleboat_Controller_Mapping_File_Button_Entry *buttonEntry);

    void initializeDefaultAxisMapping();

    int32_t processGameActivityKeyEvent(
        const Paddleboat_GameActivityKeyEvent *event, const size_t eventSize);

    int32_t processGameActivityMotionEvent(const float *axisValues);

    int32_t processKeyEvent(const AInputEvent *event);

    int32_t processMotionEvent(const AInputEvent *event);

    Paddleboat_ControllerStatus getControllerStatus() const {
        return mControllerStatus;
    }

    void setControllerStatus(
        const Paddleboat_ControllerStatus controllerStatus) {
        mControllerStatus = controllerStatus;
    }

    int32_t getConnectionIndex() const { return mConnectionIndex; }

    void setConnectionIndex(const int32_t connectionIndex) {
        mConnectionIndex = connectionIndex;
    }

    uint64_t getControllerAxisMask() const { return mControllerAxisMask; }

    Paddleboat_Controller_Data &getControllerData() { return mControllerData; }

    const Paddleboat_Controller_Data &getControllerData() const {
        return mControllerData;
    }

    Paddleboat_Controller_Info &getControllerInfo() { return mControllerInfo; }

    const Paddleboat_Controller_Info &getControllerInfo() const {
        return mControllerInfo;
    }

    GameControllerDeviceInfo &getDeviceInfo() { return mDeviceInfo; }

    const GameControllerDeviceInfo &getDeviceInfo() const {
        return mDeviceInfo;
    }

    GameControllerAxisInfo *getAxisInfo() { return mAxisInfo; }

    const GameControllerAxisInfo *getAxisInfo() const { return mAxisInfo; }

    bool getControllerDataDirty() const { return mControllerDataDirty; }

    void setControllerDataDirty(const bool dirty);

    void resetControllerData();

   private:
    int32_t processKeyEventInternal(const int32_t eventKeyCode,
                                    const int32_t eventKeyAction);

    int32_t processMotionEventInternal(const float *axisArray,
                                       const AInputEvent *event);

    void setupAxis(const GameControllerAxis gcAxis,
                   const int32_t preferredNativeAxisId,
                   const int32_t secondaryNativeAxisId,
                   const int32_t buttonMask, const int32_t buttonNegativeMask,
                   const bool axisInvert);

    void adjustAxisConstants();

    uint64_t mControllerAxisMask = 0;
    Paddleboat_ControllerStatus mControllerStatus =
        PADDLEBOAT_CONTROLLER_INACTIVE;
    int32_t mConnectionIndex = -1;
    uint32_t mAxisInversionBitmask = 0;
    Paddleboat_Controller_Data mControllerData;
    Paddleboat_Controller_Info mControllerInfo;
    int32_t mButtonKeycodes[PADDLEBOAT_BUTTON_COUNT];
    GameControllerAxisInfo mAxisInfo[GAMECONTROLLER_AXIS_COUNT];
    GameControllerDeviceInfo mDeviceInfo;
    // Controller data has been updated since the last time it was read
    bool mControllerDataDirty;
};
}  // namespace paddleboat
