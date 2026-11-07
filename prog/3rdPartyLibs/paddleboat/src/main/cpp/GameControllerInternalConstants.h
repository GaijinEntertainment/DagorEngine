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
inline constexpr size_t MAX_AXIS_COUNT = 48;
// Must match GameControllerManager.DEVICEINFO_ARRAY_SIZE
inline constexpr size_t DEVICEINFO_ARRAY_SIZE = 7;
inline constexpr size_t DEVICEINFO_ARRAY_BYTESIZE =
    sizeof(int32_t) * DEVICEINFO_ARRAY_SIZE;
inline constexpr size_t DEVICEINFO_MAX_NAME_LENGTH = 128;

inline constexpr int32_t IGNORED_EVENT = 0;
inline constexpr int32_t HANDLED_EVENT = 1;
}  // namespace paddleboat
