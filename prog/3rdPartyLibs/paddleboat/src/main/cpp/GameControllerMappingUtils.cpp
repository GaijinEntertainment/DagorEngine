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

#include "GameControllerMappingUtils.h"

#include "GameControllerManager.h"

extern "C" {
uint32_t Paddleboat_getVersion();
}

namespace paddleboat {

MappingTableSearch::MappingTableSearch()
        : mappingRoot(nullptr),
          vendorId(0),
          productId(0),
          minApi(0),
          maxApi(0),
          tableIndex(0),
          mapEntryCount(0),
          tableEntryCount(0),
          tableMaxEntryCount(GameControllerMappingInfo::MAX_CONTROLLER_TABLE_SIZE) {}

MappingTableSearch::MappingTableSearch(
        Paddleboat_Controller_Mapping_File_Controller_Entry *mapRoot, int32_t entryCount)
        : mappingRoot(mapRoot),
          vendorId(0),
          productId(0),
          minApi(0),
          maxApi(0),
          tableIndex(0),
          mapEntryCount(0),
          tableEntryCount(entryCount),
          tableMaxEntryCount(GameControllerMappingInfo::MAX_CONTROLLER_TABLE_SIZE) {}

void MappingTableSearch::initSearchParameters(const int32_t newVendorId,
                                              const int32_t newProductId,
                                              const int32_t newMinApi,
                                              const int32_t newMaxApi) {
    vendorId = newVendorId;
    productId = newProductId;
    minApi = newMinApi;
    maxApi = newMaxApi;
    tableIndex = 0;
}

bool GameControllerMappingUtils::findMatchingMapEntry(
        MappingTableSearch *searchEntry) {
    int32_t currentIndex = 0;

    // Starting out with a linear search. Updating the map table is something
    // that should only ever be done once at startup, if it actually takes an
    // appreciable time to execute when working with a big remap dictionary,
    // this is low-hanging fruit to optimize.
    const Paddleboat_Controller_Mapping_File_Controller_Entry *mapRoot =
            searchEntry->mappingRoot;
    while (currentIndex < searchEntry->tableEntryCount) {
        const Paddleboat_Controller_Mapping_File_Controller_Entry &mapEntry =
                mapRoot[currentIndex];
        if (mapEntry.vendorId > searchEntry->vendorId) {
            // Passed by the search vendorId value, so we don't already exist in
            // the table, set the current index as the insert point and bail
            searchEntry->tableIndex = currentIndex;
            return false;
        } else if (searchEntry->vendorId == mapEntry.vendorId) {
            if (mapEntry.productId > searchEntry->productId) {
                // Passed by the search productId value, so we don't already
                // exist in the table, set the current index as the insert point
                // and bail
                searchEntry->tableIndex = currentIndex;
                return false;
            } else if (searchEntry->productId == mapEntry.productId) {
                // Any overlap of the min/max API range is treated as matching
                // an existing entry
                if ((searchEntry->minApi >= mapEntry.minimumEffectiveApiLevel &&
                     searchEntry->minApi <=
                     mapEntry.maximumEffectiveApiLevel) ||
                    (searchEntry->minApi >= mapEntry.minimumEffectiveApiLevel &&
                     mapEntry.maximumEffectiveApiLevel == 0)) {
                    searchEntry->tableIndex = currentIndex;
                    return true;
                }
            }
        }
        ++currentIndex;
    }
    searchEntry->tableIndex = currentIndex;
    return false;
}

Paddleboat_ErrorCode GameControllerMappingUtils::insertMapEntry(
        const Paddleboat_Controller_Mapping_File_Controller_Entry *mappingData,
        MappingTableSearch *searchEntry,
        const IndexTableRemap *axisRemapTable,
        const IndexTableRemap *buttonRemapTable) {
    Paddleboat_ErrorCode result = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
    bool doCopy = false;

    // If min/max match exactly on the same device, just replace inline instead of inserting
    // otherwise verify there is room in the table for another entry
    Paddleboat_Controller_Mapping_File_Controller_Entry &indexEntry =
            searchEntry->mappingRoot[searchEntry->tableIndex];
    const bool sameDevice = (mappingData->productId == indexEntry.productId &&
                             mappingData->vendorId == indexEntry.vendorId);
    if (sameDevice &&
        mappingData->minimumEffectiveApiLevel == indexEntry.minimumEffectiveApiLevel &&
        mappingData->maximumEffectiveApiLevel == indexEntry.maximumEffectiveApiLevel) {
        doCopy = true;
    } else if (searchEntry->tableEntryCount < searchEntry->tableMaxEntryCount &&
               searchEntry->tableIndex < searchEntry->tableMaxEntryCount) {
        // Check for min/max API 'overlap' between the new entry and the insert point on the
        // same device, we may need to 'patch' the max API of the insert point and do the actual
        // insert one index higher.
        if (sameDevice &&
           ((mappingData->minimumEffectiveApiLevel >= indexEntry.minimumEffectiveApiLevel &&
             mappingData->minimumEffectiveApiLevel <= indexEntry.maximumEffectiveApiLevel) ||
            (mappingData->minimumEffectiveApiLevel >= indexEntry.minimumEffectiveApiLevel &&
             indexEntry.maximumEffectiveApiLevel == 0))) {
            indexEntry.maximumEffectiveApiLevel = mappingData->minimumEffectiveApiLevel - 1;
            searchEntry->tableIndex += 1;
        }

        // Empty table, or inserting at the end, no relocation of existing data
        // required, otherwise shift existing data down starting at the insert
        // index.
        if (!(searchEntry->tableEntryCount == 0 ||
              searchEntry->tableIndex == searchEntry->tableEntryCount)) {
            const size_t copySize =
                    (searchEntry->tableEntryCount - searchEntry->tableIndex) *
                    sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry);
            memmove(&searchEntry->mappingRoot[searchEntry->tableIndex + 1],
                    &searchEntry->mappingRoot[searchEntry->tableIndex],
                    copySize);
        }
        doCopy = true;
    }
    if (doCopy) {
        // Copy the new data
        Paddleboat_Controller_Mapping_File_Controller_Entry *newEntry =
                &searchEntry->mappingRoot[searchEntry->tableIndex];
        memcpy(newEntry, mappingData,
               sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry));
        // Fix up the axis and button indices using the remap tables
        const uint32_t newAxisIndex = axisRemapTable[newEntry->axisTableIndex].newIndex;
        newEntry->axisTableIndex = newAxisIndex;
        const uint32_t newButtonIndex = buttonRemapTable[newEntry->buttonTableIndex].newIndex;
        newEntry->buttonTableIndex = newButtonIndex;
        result = PADDLEBOAT_NO_ERROR;
    }
    return result;
}

const Paddleboat_Controller_Mapping_File_Controller_Entry *
        GameControllerMappingUtils::validateMapTable(
        const Paddleboat_Controller_Mapping_File_Controller_Entry *mappingRoot,
        const int32_t tableEntryCount) {
    // The map table is always assumed to be sorted by increasing vendorId. Each
    // sequence of entries with the same vendorId are sorted by increasing
    // productId. Multiple entries with the same vendorId/productId range are
    // sorted by increasing min/max API ranges.
    //    vendorId
    //   productId
    //      minApi
    int32_t currentIndex = 0;
    int32_t previousVendorId = -1;
    while (currentIndex < tableEntryCount) {
        if (mappingRoot[currentIndex].vendorId < previousVendorId) {
            // failure in vendorId order, return the offending entry
            return &mappingRoot[currentIndex];
        }

        int32_t previousProductId = mappingRoot[currentIndex].productId;
        int32_t previousMinApi =
                mappingRoot[currentIndex].minimumEffectiveApiLevel;
        int32_t previousMaxApi =
                mappingRoot[currentIndex].maximumEffectiveApiLevel;
        previousVendorId = mappingRoot[currentIndex++].vendorId;

        while (currentIndex < tableEntryCount &&
               mappingRoot[currentIndex].vendorId == previousVendorId) {
            while (currentIndex < tableEntryCount &&
                   mappingRoot[currentIndex].productId == previousProductId) {
                if (mappingRoot[currentIndex].minimumEffectiveApiLevel <
                    previousMinApi ||
                    mappingRoot[currentIndex].minimumEffectiveApiLevel <
                    previousMaxApi) {
                    // failure in API order, return the offending entry
                    return &mappingRoot[currentIndex];
                }
                previousMinApi =
                        mappingRoot[currentIndex].minimumEffectiveApiLevel;
                previousMaxApi =
                        mappingRoot[currentIndex++].maximumEffectiveApiLevel;
            }
            if (mappingRoot[currentIndex].productId < previousProductId) {
                // failure in productId order, return the offending entry
                return &mappingRoot[currentIndex];
            }
            previousProductId = mappingRoot[currentIndex++].productId;
        }
    }

    // Validation success, return nullptr (no offending entries to return)
    return nullptr;
}

Paddleboat_ErrorCode GameControllerMappingUtils::validateMapFile(
            const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
            const size_t mappingFileBufferSize) {
    if (mappingFileHeader->fileIdentifier != PADDLEBOAT_MAPPING_FILE_IDENTIFIER) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }
    if (mappingFileHeader->libraryMinimumVersion > Paddleboat_getVersion()) {
        return PADDLEBOAT_INCOMPATIBLE_MAPPING_DATA;
    }

    // Must have at least one entry in each table
    if (mappingFileHeader->axisTableEntryCount == 0 ||
        mappingFileHeader->buttonTableEntryCount == 0 ||
        mappingFileHeader->controllerTableEntryCount == 0 ||
        mappingFileHeader->stringTableEntryCount == 0) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }
    // Bounds check against specified buffer size, ensure all internal data
    // ranges fit in the buffer
    if (mappingFileBufferSize < sizeof(Paddleboat_Controller_Mapping_File_Header)) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }

    const uint8_t* fileStart = reinterpret_cast<const uint8_t*>(mappingFileHeader);
    const ptrdiff_t maxSize = mappingFileBufferSize;

    const uint8_t *endAxis = fileStart + mappingFileHeader->axisTableOffset +
        (mappingFileHeader->axisTableEntryCount *
        sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry));
    if ((endAxis - fileStart) > maxSize) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }

    const uint8_t *endButton = fileStart + mappingFileHeader->buttonTableOffset +
        (mappingFileHeader->buttonTableEntryCount *
        sizeof(Paddleboat_Controller_Mapping_File_Button_Entry));
    if ((endButton - fileStart) > maxSize) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }

    const uint8_t *endString = fileStart + mappingFileHeader->stringTableOffset +
        (mappingFileHeader->stringTableEntryCount *
        sizeof(Paddleboat_Controller_Mapping_File_String_Entry));
    if ((endString - fileStart) > maxSize) {
        return PADDLEBOAT_INVALID_MAPPING_DATA;
    }
    return PADDLEBOAT_NO_ERROR;
}

Paddleboat_ErrorCode GameControllerMappingUtils::mergeStringTable(
        const Paddleboat_Controller_Mapping_File_String_Entry *newStrings,
        const uint32_t newStringCount,
        Paddleboat_Controller_Mapping_File_String_Entry *stringEntries,
        uint32_t *stringEntryCount,
        const uint32_t maxStringEntryCount,
        IndexTableRemap *remapTable) {
    Paddleboat_ErrorCode result = PADDLEBOAT_NO_ERROR;

    // Check to see if a string in the incoming table already exists in the
    // existing table or if it needs to be added
    const uint32_t existingStringCount = *stringEntryCount;
    uint32_t currentStringCount = existingStringCount;
    for (uint32_t newIndex = 0; newIndex < newStringCount; ++newIndex) {
        bool foundExistingMatch = false;
        for (uint32_t existingIndex = 0; existingIndex < existingStringCount; ++existingIndex) {
            if (strncmp(newStrings[newIndex].stringTableEntry,
                        stringEntries[existingIndex].stringTableEntry,
                        PADDLEBOAT_STRING_TABLE_ENTRY_MAX_SIZE) == 0) {
                remapTable[newIndex].newIndex = existingIndex;
                foundExistingMatch = true;
                break;
            }
        }
        if (!foundExistingMatch) {
            if (currentStringCount >= maxStringEntryCount) {
                // Return error if out of room in string table
                result = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
                break;
            }
            memcpy(&stringEntries[currentStringCount], &newStrings[newIndex],
                   sizeof(Paddleboat_Controller_Mapping_File_String_Entry));
            remapTable[newIndex].newIndex = currentStringCount;
            currentStringCount += 1;
            *stringEntryCount = currentStringCount;
        }
    }
    return result;
}

Paddleboat_ErrorCode GameControllerMappingUtils::mergeAxisTable(
        const Paddleboat_Controller_Mapping_File_Axis_Entry *newAxis,
        const uint32_t newAxisCount,
        Paddleboat_Controller_Mapping_File_Axis_Entry *axisEntries,
        uint32_t *axisEntryCount,
        const uint32_t maxAxisEntryCount,
        const IndexTableRemap *stringRemapTable,
        IndexTableRemap *axisRemapTable) {
    Paddleboat_ErrorCode result = PADDLEBOAT_NO_ERROR;

    // Check if an axis table already exists in the existing table or if it needs to be added.
    // Matching is done by name, so we have to use the string table via the remapped string index
    const uint32_t existingAxisCount = *axisEntryCount;
    uint32_t currentAxisCount = existingAxisCount;

    for (uint32_t newIndex = 0; newIndex < newAxisCount; ++newIndex) {
        bool foundExistingMatch = false;
        const uint32_t newAxisStringTableIndex =
                stringRemapTable[newAxis[newIndex].axisNameStringTableIndex].newIndex;
        for (uint32_t existingIndex = 0; existingIndex < existingAxisCount; ++existingIndex) {
            if (axisEntries[existingIndex].axisNameStringTableIndex == newAxisStringTableIndex) {
                axisRemapTable[newIndex].newIndex = existingIndex;
                foundExistingMatch = true;
                break;
            }
        }
        if (!foundExistingMatch) {
            if (currentAxisCount >= maxAxisEntryCount) {
                // Return error if out of room in axis table
                result = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
                break;
            }
            memcpy(&axisEntries[currentAxisCount], &newAxis[newIndex],
                   sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry));
            axisEntries[currentAxisCount].axisNameStringTableIndex = newAxisStringTableIndex;
            axisRemapTable[newIndex].newIndex = currentAxisCount;
            currentAxisCount += 1;
            *axisEntryCount = currentAxisCount;
        }
    }
    return result;
}

Paddleboat_ErrorCode GameControllerMappingUtils::mergeButtonTable(
        const Paddleboat_Controller_Mapping_File_Button_Entry *newButton,
        const uint32_t newButtonCount,
        Paddleboat_Controller_Mapping_File_Button_Entry *buttonEntries,
        uint32_t *buttonEntryCount,
        const uint32_t maxButtonEntryCount,
        const IndexTableRemap *stringRemapTable,
        IndexTableRemap *buttonRemapTable) {
    Paddleboat_ErrorCode result = PADDLEBOAT_NO_ERROR;

    // Check if a button table already exists in the existing table or if it needs to be added.
    // Matching is done by name, so we have to use the string table via the remapped string index
    const uint32_t existingButtonCount = *buttonEntryCount;
    uint32_t currentButtonCount = existingButtonCount;

    for (uint32_t newIndex = 0; newIndex < newButtonCount; ++newIndex) {
        bool foundExistingMatch = false;
        const uint32_t newButtonStringTableIndex =
                stringRemapTable[newButton[newIndex].buttonNameStringTableIndex].newIndex;
        for (uint32_t existingIndex = 0; existingIndex < existingButtonCount; ++existingIndex) {
            if (buttonEntries[existingIndex].buttonNameStringTableIndex ==
                    newButtonStringTableIndex) {
                buttonRemapTable[newIndex].newIndex = existingIndex;
                foundExistingMatch = true;
                break;
            }
        }
        if (!foundExistingMatch) {
            if (currentButtonCount >= maxButtonEntryCount) {
                // Return error if out of room in axis table
                result = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
                break;
            }
            memcpy(&buttonEntries[currentButtonCount], &newButton[newIndex],
                   sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry));
            buttonEntries[currentButtonCount].buttonNameStringTableIndex =
                    newButtonStringTableIndex;
            buttonRemapTable[newIndex].newIndex = currentButtonCount;
            currentButtonCount += 1;
            *buttonEntryCount = currentButtonCount;
        }
    }
    return result;
}

Paddleboat_ErrorCode GameControllerMappingUtils::mergeControllerRemapData(
        const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
        const size_t mappingFileBufferSize,
        GameControllerMappingInfo &mappingInfo) {
    Paddleboat_ErrorCode mergeResult = PADDLEBOAT_NO_ERROR;
    // The incoming file has its own internal indexes for its axis, button and
    // string table entries. These indices will need to be remapped after this data is merged
    // into the existing internal arrays. Allocate an array of remap structures large
    // enough for all required indices and section it by index category.
    const size_t totalIndexCount = mappingFileHeader->axisTableEntryCount +
                                   mappingFileHeader->buttonTableEntryCount +
                                   mappingFileHeader->stringTableEntryCount;
    std::unique_ptr<IndexTableRemap[]> remapTable =
            std::make_unique<IndexTableRemap[]>(totalIndexCount);
    IndexTableRemap *axisIndexTableRemap = remapTable.get();
    IndexTableRemap *buttonIndexTableRemap = axisIndexTableRemap +
                                             mappingFileHeader->axisTableEntryCount;
    IndexTableRemap *stringIndexTableRemap = buttonIndexTableRemap +
                                             mappingFileHeader->buttonTableEntryCount;

    // Merge the string, axis, and button tables from the file into the internal tables
    const uint8_t *fileStart = reinterpret_cast<const uint8_t*>(mappingFileHeader);
    // Merge string table first since axis and button depend on it
    const Paddleboat_Controller_Mapping_File_String_Entry *fileStringTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_String_Entry *>(
                    fileStart + mappingFileHeader->stringTableOffset);
    mergeResult = GameControllerMappingUtils::mergeStringTable(
            fileStringTable, mappingFileHeader->stringTableEntryCount,
            mappingInfo.mStringTable, &mappingInfo.mStringTableEntryCount,
            GameControllerMappingInfo::MAX_STRING_TABLE_SIZE, stringIndexTableRemap);
    if (mergeResult != PADDLEBOAT_NO_ERROR) {
        return mergeResult;
    }
    // Axis table
    const Paddleboat_Controller_Mapping_File_Axis_Entry *fileAxisTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_Axis_Entry *>(
                    fileStart + mappingFileHeader->axisTableOffset);
    mergeResult = GameControllerMappingUtils::mergeAxisTable(
            fileAxisTable, mappingFileHeader->axisTableEntryCount,
            mappingInfo.mAxisTable, &mappingInfo.mAxisTableEntryCount,
            GameControllerMappingInfo::MAX_AXIS_TABLE_SIZE, stringIndexTableRemap,
            axisIndexTableRemap);
    if (mergeResult != PADDLEBOAT_NO_ERROR) {
        return mergeResult;
    }
    // Button table
    const Paddleboat_Controller_Mapping_File_Button_Entry *fileButtonTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_Button_Entry *>(
                    fileStart + mappingFileHeader->buttonTableOffset);
    mergeResult = GameControllerMappingUtils::mergeButtonTable(
            fileButtonTable, mappingFileHeader->buttonTableEntryCount,
            mappingInfo.mButtonTable, &mappingInfo.mButtonTableEntryCount,
            GameControllerMappingInfo::MAX_BUTTON_TABLE_SIZE, stringIndexTableRemap,
            buttonIndexTableRemap);
    if (mergeResult != PADDLEBOAT_NO_ERROR) {
        return mergeResult;
    }

    // Loop through the controller list in the file and merge them into the internal
    // table.
    for (uint32_t i = 0; i < mappingFileHeader->controllerTableEntryCount; ++i) {
        const Paddleboat_Controller_Mapping_File_Controller_Entry *fileControllerTable =
                reinterpret_cast<const Paddleboat_Controller_Mapping_File_Controller_Entry *>(
                        fileStart + mappingFileHeader->controllerTableOffset);
        MappingTableSearch mapSearch(mappingInfo.mControllerTable,
                                     mappingInfo.mControllerTableEntryCount);
        mapSearch.initSearchParameters(fileControllerTable[i].vendorId,
                                       fileControllerTable[i].productId,
                                       fileControllerTable[i].minimumEffectiveApiLevel,
                                       fileControllerTable[i].maximumEffectiveApiLevel);
        GameControllerMappingUtils::findMatchingMapEntry(&mapSearch);
        mergeResult = GameControllerMappingUtils::insertMapEntry(&fileControllerTable[i],
                                                                 &mapSearch,
                                                                 axisIndexTableRemap,
                                                                 buttonIndexTableRemap);
        if (mergeResult != PADDLEBOAT_NO_ERROR) {
            break;
        }
        mappingInfo.mControllerTableEntryCount += 1;
    }
    return mergeResult;
}

Paddleboat_ErrorCode GameControllerMappingUtils::overwriteControllerRemapData(
        const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
        const size_t mappingFileBufferSize,
        GameControllerMappingInfo &mappingInfo) {

    // Bounds check of internal buffers
    if (mappingFileHeader->stringTableEntryCount >
        GameControllerMappingInfo::MAX_STRING_TABLE_SIZE ||
        mappingFileHeader->buttonTableEntryCount >
        GameControllerMappingInfo::MAX_BUTTON_TABLE_SIZE ||
        mappingFileHeader->axisTableEntryCount >
        GameControllerMappingInfo::MAX_AXIS_TABLE_SIZE ||
        mappingFileHeader->controllerTableEntryCount >
        GameControllerMappingInfo::MAX_CONTROLLER_TABLE_SIZE) {
        return PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    const uint8_t *fileStart = reinterpret_cast<const uint8_t*>(mappingFileHeader);

    const Paddleboat_Controller_Mapping_File_String_Entry *fileStringTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_String_Entry *>(
                    fileStart + mappingFileHeader->stringTableOffset);
    memcpy(mappingInfo.mStringTable, fileStringTable,
           mappingFileHeader->stringTableEntryCount *
           sizeof(Paddleboat_Controller_Mapping_File_String_Entry));
    mappingInfo.mStringTableEntryCount = mappingFileHeader->stringTableEntryCount;

    const Paddleboat_Controller_Mapping_File_Axis_Entry *fileAxisTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_Axis_Entry *>(
                    fileStart + mappingFileHeader->axisTableOffset);
    memcpy(mappingInfo.mAxisTable, fileAxisTable,
           mappingFileHeader->axisTableEntryCount *
           sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry));
    mappingInfo.mAxisTableEntryCount = mappingFileHeader->axisTableEntryCount;

    const Paddleboat_Controller_Mapping_File_Button_Entry *fileButtonTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_Button_Entry *>(
                    fileStart + mappingFileHeader->buttonTableOffset);
    memcpy(mappingInfo.mButtonTable, fileButtonTable,
           mappingFileHeader->buttonTableEntryCount *
           sizeof(Paddleboat_Controller_Mapping_File_Button_Entry));
    mappingInfo.mButtonTableEntryCount = mappingFileHeader->buttonTableEntryCount;

    const Paddleboat_Controller_Mapping_File_Controller_Entry *fileControllerTable =
            reinterpret_cast<const Paddleboat_Controller_Mapping_File_Controller_Entry *>(
                    fileStart + mappingFileHeader->controllerTableOffset);
    memcpy(mappingInfo.mControllerTable, fileControllerTable,
           mappingFileHeader->controllerTableEntryCount *
           sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry));
    mappingInfo.mControllerTableEntryCount = mappingFileHeader->controllerTableEntryCount;

    return PADDLEBOAT_NO_ERROR;
}

}  // namespace paddleboat
