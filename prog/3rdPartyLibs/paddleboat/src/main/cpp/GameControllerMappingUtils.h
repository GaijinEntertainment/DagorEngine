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

#ifndef PADDLEBOAT_H
#include "paddleboat.h"
#endif
#include "GameControllerMappingFile.h"
#include "GameControllerMappingInfo.h"

namespace paddleboat {

class IndexTableRemap {
public:
    IndexTableRemap() : newIndex(-1) {}
    IndexTableRemap(int32_t i) : newIndex(i) {}
    int32_t newIndex;
};

class MappingTableSearch {
public:
    MappingTableSearch();

    MappingTableSearch(Paddleboat_Controller_Mapping_File_Controller_Entry *mapRoot,
                       int32_t entryCount);

    void initSearchParameters(const int32_t newVendorId,
                              const int32_t newProductId,
                              const int32_t newMinApi, const int32_t newMaxApi);

    Paddleboat_Controller_Mapping_File_Controller_Entry *mappingRoot;
    int32_t vendorId;
    int32_t productId;
    int32_t minApi;
    int32_t maxApi;
    int32_t tableIndex;
    int32_t mapEntryCount;
    int32_t tableEntryCount;
    int32_t tableMaxEntryCount;
};

class GameControllerMappingUtils {
public:
    static bool findMatchingMapEntry(MappingTableSearch *searchEntry);

    static Paddleboat_ErrorCode insertMapEntry(
            const Paddleboat_Controller_Mapping_File_Controller_Entry *mappingData,
            MappingTableSearch *searchEntry,
            const IndexTableRemap *axisRemapTable,
            const IndexTableRemap *buttonRemapTable);

    static const Paddleboat_Controller_Mapping_File_Controller_Entry *validateMapTable(
            const Paddleboat_Controller_Mapping_File_Controller_Entry *mappingRoot,
            const int32_t tableEntryCount);

    static Paddleboat_ErrorCode validateMapFile(
            const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
            const size_t mappingFileBufferSize);

    static Paddleboat_ErrorCode mergeStringTable(
            const Paddleboat_Controller_Mapping_File_String_Entry *newStrings,
            const uint32_t newStringCount,
            Paddleboat_Controller_Mapping_File_String_Entry *stringEntries,
            uint32_t *stringEntryCount,
            const uint32_t maxStringEntryCount,
            IndexTableRemap *remapTable);

    static Paddleboat_ErrorCode mergeAxisTable(
            const Paddleboat_Controller_Mapping_File_Axis_Entry *newAxis,
            const uint32_t newAxisCount,
            Paddleboat_Controller_Mapping_File_Axis_Entry *axisEntries,
            uint32_t *axisEntryCount,
            const uint32_t maxAxisEntryCount,
            const IndexTableRemap *stringRemapTable,
            IndexTableRemap *axisRemapTable);

    static Paddleboat_ErrorCode mergeButtonTable(
            const Paddleboat_Controller_Mapping_File_Button_Entry *newButton,
            const uint32_t newButtonCount,
            Paddleboat_Controller_Mapping_File_Button_Entry *buttonEntries,
            uint32_t *buttonEntryCount,
            const uint32_t maxButtonEntryCount,
            const IndexTableRemap *stringRemapTable,
            IndexTableRemap *buttonRemapTable);

    static Paddleboat_ErrorCode mergeControllerRemapData(
            const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
            const size_t mappingFileBufferSize,
            GameControllerMappingInfo &mappingInfo);

    static Paddleboat_ErrorCode overwriteControllerRemapData(
            const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
            const size_t mappingFileBufferSize,
            GameControllerMappingInfo &mappingInfo);
};

}  // namespace paddleboat
