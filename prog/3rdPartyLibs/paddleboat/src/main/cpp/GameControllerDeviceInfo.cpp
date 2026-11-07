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
#include "GameControllerDeviceInfo.h"

#include <cstring>

namespace paddleboat {
GameControllerDeviceInfo::GameControllerDeviceInfo() {
    mName[0] = '\0';
    mInfo.mDeviceId = -1;
    mInfo.mProductId = -1;
    mInfo.mVendorId = -1;
    mInfo.mAxisBitsLow = 0;
    mInfo.mAxisBitsHigh = 0;
    mInfo.mControllerNumber = -1;
    mInfo.mControllerFlags = 0;

    for (size_t i = 0; i < paddleboat::MAX_AXIS_COUNT; ++i) {
        mAxisMinArray[i] = 0.0f;
        mAxisMaxArray[i] = 0.0f;
        mAxisFlatArray[i] = 0.0f;
        mAxisFuzzArray[i] = 0.0f;
    }
}

void GameControllerDeviceInfo::setName(const char *name) {
    strncpy(mName, name, DEVICEINFO_MAX_NAME_LENGTH);
}
}  // namespace paddleboat