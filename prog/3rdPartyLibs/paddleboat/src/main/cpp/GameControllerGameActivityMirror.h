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

#include <cstdint>

namespace paddleboat {
// GameActivity needs to use its own event data structures for key and motion
// events. We do not want to have a dependency on GameActivity. Unfortunately,
// this means we need to internally mirror the relevant structures.
// Paddleboat_processGameActivityInputEvent includes a struct size parameter,
// and we know that the GameActivity structures will only ever add fields, so we
// can determine if we are being passed a later version of the struct than the
// mirrored internal version.

// The following should mirror GameActivity.h
#define PADDLEBOAT_GAME_ACTIVITY_POINTER_INFO_AXIS_COUNT 48
#define PADDLEBOAT_MAX_NUM_POINTERS_IN_MOTION_EVENT 8

typedef struct Paddleboat_GameActivityPointerInfo {
    int32_t id;
    float axisValues[PADDLEBOAT_GAME_ACTIVITY_POINTER_INFO_AXIS_COUNT];
    float rawX;
    float rawY;
} Paddleboat_GameActivityPointerInfo;

typedef struct Paddleboat_GameActivityMotionEvent {
    int32_t deviceId;
    int32_t source;
    int32_t action;
    int64_t eventTime;
    int64_t downTime;
    int32_t flags;
    int32_t metaState;
    int32_t actionButton;
    int32_t buttonState;
    int32_t classification;
    int32_t edgeFlags;
    uint32_t pointerCount;
    Paddleboat_GameActivityPointerInfo
        pointers[PADDLEBOAT_MAX_NUM_POINTERS_IN_MOTION_EVENT];
    float precisionX;
    float precisionY;
} Paddleboat_GameActivityMotionEvent;

typedef struct Paddleboat_GameActivityKeyEvent {
    int32_t deviceId;
    int32_t source;
    int32_t action;
    int64_t eventTime;
    int64_t downTime;
    int32_t flags;
    int32_t metaState;
    int32_t modifiers;
    int32_t repeatCount;
    int32_t keyCode;
} Paddleboat_GameActivityKeyEvent;

// Used in GameActivity 1.2.2 and higher
typedef struct Paddleboat_GameActivityPointerInfoV2 {
    int32_t id;
    int32_t toolType;
    float axisValues[PADDLEBOAT_GAME_ACTIVITY_POINTER_INFO_AXIS_COUNT];
    float rawX;
    float rawY;
} Paddleboat_GameActivityPointerInfoV2;

typedef struct Paddleboat_GameActivityMotionEventV2 {
    int32_t deviceId;
    int32_t source;
    int32_t action;
    int64_t eventTime;
    int64_t downTime;
    int32_t flags;
    int32_t metaState;
    int32_t actionButton;
    int32_t buttonState;
    int32_t classification;
    int32_t edgeFlags;
    uint32_t pointerCount;
    Paddleboat_GameActivityPointerInfoV2
        pointers[PADDLEBOAT_MAX_NUM_POINTERS_IN_MOTION_EVENT];
    int historySize;
    long* historicalEventTimes;
    float* historicalAxisValues;
    float precisionX;
    float precisionY;
} Paddleboat_GameActivityMotionEventV2;

}  // namespace paddleboat
