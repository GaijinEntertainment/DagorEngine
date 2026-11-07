/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <memory>

#include "paddleboat.h"
#include "GameControllerMappingFile.h"

namespace paddleboat {

class GameControllerMappingInfo {
public:
    // Internal new table size limits
    static constexpr int32_t MAX_CONTROLLER_TABLE_SIZE = 256;
    static constexpr int32_t MAX_BUTTON_TABLE_SIZE = 32;
    static constexpr int32_t MAX_AXIS_TABLE_SIZE = 32;
    static constexpr int32_t MAX_STRING_TABLE_SIZE = 128;

    GameControllerMappingInfo() {
        memset(mAxisTable, 0, sizeof(mAxisTable));
        memset(mButtonTable, 0, sizeof(mButtonTable));
        memset(mControllerTable, 0, sizeof(mControllerTable));
        memset(mStringTable, 0, sizeof(mStringTable));
    }

    uint32_t mAxisTableEntryCount = 0;
    uint32_t mButtonTableEntryCount = 0;
    uint32_t mControllerTableEntryCount = 0;
    uint32_t mStringTableEntryCount = 0;
    Paddleboat_Controller_Mapping_File_Axis_Entry mAxisTable[MAX_AXIS_TABLE_SIZE];
    Paddleboat_Controller_Mapping_File_Button_Entry mButtonTable[MAX_BUTTON_TABLE_SIZE];
    Paddleboat_Controller_Mapping_File_Controller_Entry mControllerTable[MAX_CONTROLLER_TABLE_SIZE];
    Paddleboat_Controller_Mapping_File_String_Entry mStringTable[MAX_STRING_TABLE_SIZE];
};

}
