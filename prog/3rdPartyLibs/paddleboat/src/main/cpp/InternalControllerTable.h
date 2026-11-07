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

#include "paddleboat.h"
#include "GameControllerMappingFile.h"

namespace paddleboat {
// Interim until PaddleboatMappingTool is finished
typedef struct Paddleboat_Internal_Mapping_Header {
    uint32_t axisTableEntryCount;
    uint32_t buttonTableEntryCount;
    uint32_t controllerTableEntryCount;
    uint32_t stringTableEntryCount;
    const Paddleboat_Controller_Mapping_File_Axis_Entry *axisTable;
    const Paddleboat_Controller_Mapping_File_Button_Entry *buttonTable;
    const Paddleboat_Controller_Mapping_File_Controller_Entry *controllerTable;
    const Paddleboat_Controller_Mapping_File_String_Entry *stringTable;
} Paddleboat_Internal_Mapping_Header;

const Paddleboat_Internal_Mapping_Header *GetInternalMappingHeader();

}  // namespace paddleboat