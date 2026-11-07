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

#include "GameControllerInternalConstants.h"

namespace paddleboat {

class GameControllerDeviceInfo {
   public:
    // These are copied directly from the int[] mGameControllerDeviceInfoArray,
    // the layout should match the field definitions in
    // Paddleboat_Controller_Info.java
    struct InfoFields {
        int32_t mDeviceId;
        int32_t mVendorId;
        int32_t mProductId;
        int32_t mAxisBitsLow;
        int32_t mAxisBitsHigh;
        int32_t mControllerNumber;
        int32_t mControllerFlags;
    };

    GameControllerDeviceInfo();

    void setName(const char *name);

    const char *getName() const { return mName; }

    const InfoFields &getInfo() const { return mInfo; }

    const float *getMinArray() const { return mAxisMinArray; }

    const float *getMaxArray() const { return mAxisMaxArray; }

    const float *getFlatArray() const { return mAxisFlatArray; }

    const float *getFuzzArray() const { return mAxisFuzzArray; }

    InfoFields *getInfo() { return &mInfo; }

    float *getMinArray() { return mAxisMinArray; }

    float *getMaxArray() { return mAxisMaxArray; }

    float *getFlatArray() { return mAxisFlatArray; }

    float *getFuzzArray() { return mAxisFuzzArray; }

   private:
    char mName[paddleboat::DEVICEINFO_MAX_NAME_LENGTH];
    InfoFields mInfo;
    float mAxisMinArray[paddleboat::MAX_AXIS_COUNT];
    float mAxisMaxArray[paddleboat::MAX_AXIS_COUNT];
    float mAxisFlatArray[paddleboat::MAX_AXIS_COUNT];
    float mAxisFuzzArray[paddleboat::MAX_AXIS_COUNT];
};
}  // namespace paddleboat
