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
#include "paddleboat.h"

namespace paddleboat {

/**
 * @brief A structure that describes the header of a PaddleboatMappingTool
 * configuration file. The ::Paddleboat_addControllerRemapDataFromFileBuffer
 * function accepts this file data as an input to modify the internal
 * controller mapping database.
 * See ::Paddleboat_addControllerRemapDataFromFileBuffer
 * for links to documentation on controller remapping configuration.
 */
typedef struct  __attribute__((packed)) Paddleboat_Controller_Mapping_File_Header {
    /** @brief Identifier value for the file, expected to be equal
     * to the `PADDLEBOAT_MAPPING_FILE_IDENTIFIER` constant.
     */
    uint32_t fileIdentifier;
    /** @brief Semantic version number of the earliest version
     * of the Paddleboat library that is compatible with this file.
     * (i.e. 0x010200 for 1.2.0)
     */
    uint32_t libraryMinimumVersion;
    /** @brief The number of axis remapping table entries present in the file. The
     * axis remapping table is a sequential array of
     * `Paddleboat_Controller_Mapping_File_Axis_Entry` structures starting
     * at `axisTableOffset` bytes from the beginning of this header.
     */
    uint32_t axisTableEntryCount;
    /** @brief The number of button remapping table entries present in the file. The
     * button remapping table is a sequential array of
     * `Paddleboat_Controller_Mapping_File_Button_Entry` structures starting
     * at `buttonTableOffset` bytes from the beginning of this header.
     */
    uint32_t buttonTableEntryCount;
    /** @brief The number of controller mapping definition entries present
     * in the file. The controller mapping definition table is a sequential array of
     * `Paddleboat_Controller_Mapping_File_Controller_Entry` structures starting
     * at `controllerTableOffset` bytes from the beginning of this header.
     */
    uint32_t controllerTableEntryCount;
    /** @brief The number of string table entries present in the file.
     * The string table is a sequential array of
     * `Paddleboat_Controller_Mapping_File_String_Entry` structures starting
     * at `stringTableOffset` bytes from the beginning of this header.
     */
    uint32_t stringTableEntryCount;
    /** @brief The offset in bytes from the start of the header to the first
     * `Paddleboat_Controller_Mapping_File_Axis_Entry` structure.
     */
    uint64_t axisTableOffset;
    /** @brief The offset in bytes from the start of the header to the first
     * `Paddleboat_Controller_Mapping_File_Button_Entry` structure.
     */
    uint64_t buttonTableOffset;
    /** @brief The offset in bytes from the start of the header to the first
     * `Paddleboat_Controller_Mapping_File_Controller_Entry` structure.
     */
    uint64_t controllerTableOffset;
    /** @brief The offset in bytes from the start of the header to the first
     * `Paddleboat_Controller_Mapping_File_String_Entry` structure.
     */
    uint64_t stringTableOffset;
} Paddleboat_Controller_Mapping_File_Header;

/**
* @brief A structure that describes the axis mapping for a controller.
* Axis mapping is used to translate AMOTION_EVENT_AXIS values to
* PADDLEBOAT_AXIS values. The axis mapping table is also used to
* specify button activation in response to axis inputs, and
* optional axis value inversion on a per-axis basis.
*/
typedef struct  __attribute__((packed)) Paddleboat_Controller_Mapping_File_Axis_Entry {
    /** @brief Index into the file's string table for the entry
     * containing the string with the name of this axis configuration.
     * The name is used for identification and conflict resolution
     * when updating the controller database with new or replacement entries.
     */
    uint32_t axisNameStringTableIndex;
    /** @brief AMOTION_EVENT_AXIS value for
     * the corresponding Paddleboat control axis, or PADDLEBOAT_AXIS_IGNORED if
     * unsupported.
     */
    uint16_t axisMapping[PADDLEBOAT_MAPPING_AXIS_COUNT];
    /** @brief Button to set on
     * positive axis value, PADDLEBOAT_AXIS_BUTTON_IGNORED if none.
     */
    uint8_t axisPositiveButtonMapping[PADDLEBOAT_MAPPING_AXIS_COUNT];
    /** @brief Button to set on
     * negative axis value, PADDLEBOAT_AXIS_BUTTON_IGNORED if none.
     */
    uint8_t axisNegativeButtonMapping[PADDLEBOAT_MAPPING_AXIS_COUNT];
    /** @brief Bitmask value, if a bit corresponding to
     * an axis index (i.e. bit 1 for `PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_Y`
     * is set, then the incoming axis input will be inverted prior to
     * reporting. This is to support workarounds for controllers where
     * the axis values are reversed so pushing 'down' reads as 'up'.
     */
    uint32_t axisInversionBitmask;
} Paddleboat_Controller_Mapping_File_Axis_Entry;

/**
* @brief A structure that describes the button mapping for a controller.
* Button mapping is used to translate AKEYCODE values to
* PADDLEBOAT_BUTTON values.
*/
typedef struct  __attribute__((packed)) Paddleboat_Controller_Mapping_File_Button_Entry {
    /** @brief Index into the file's string table for the entry
     * containing the string with the name of this button configuration.
     * The name is used for identification and conflict resolution
     * when updating the controller database with new or replacement entries.
     */
    uint32_t buttonNameStringTableIndex;
    /** @brief AKEYCODE_ value corresponding
     * with the corresponding Paddleboat button.
     * PADDLEBOAT_BUTTON_IGNORED if unsupported.
     */
    uint16_t buttonMapping[PADDLEBOAT_BUTTON_COUNT];
} Paddleboat_Controller_Mapping_File_Button_Entry;

/**
* @brief A structure that defines controller mapping and
* configuration data for a specific controller device.
*/
typedef struct  __attribute__((packed)) Paddleboat_Controller_Mapping_File_Controller_Entry {
    /** @brief Minimum API level required for this entry */
    int16_t minimumEffectiveApiLevel;
    /** @brief Maximum API level required for this entry, 0 = no max */
    int16_t maximumEffectiveApiLevel;
    /** @brief VendorID of the controller device for this entry */
    int32_t vendorId;
    /** @brief ProductID of the controller device for this entry */
    int32_t productId;
    /** @brief Flag bits, will be ORed with
     * `Paddleboat_Controller_Info.controllerFlags` at setup
     */
    uint32_t flags;
    /** @brief index into the array of
     * `Paddleboat_Controller_Mapping_File_Axis_Entry` structs of the
     * axis mapping table used by this controller
     */
    uint32_t axisTableIndex;
    /** @brief index into the array of
     * `Paddleboat_Controller_Mapping_File_Button_Entry` structs of the
     * button mapping table used by this controller
     */
    uint32_t buttonTableIndex;
    /** @brief Index into the file's string table for the entry
     * containing the string with optional allowlist filtering
     * information. If no filter is specified, the string value will be "None".
     * If an allowlist filter is specified, this controller mapping
     * will only be added to the database if the runtime filter criteria
     * are met. See ::Paddleboat_addControllerRemapDataFromFileBuffer
     * for links to documentation on controller remapping configuration.
     */
    uint32_t deviceAllowlistStringTableIndex;
    /** @brief Index into the file's string table for the entry
     * containing the string with optional denylist filtering
     * information. If no filter is specified, the string value will be "None".
     * If an denylist filter is specified, this controller mapping
     * will not be added to the database if the runtime filter criteria
     * are met. See ::Paddleboat_addControllerRemapDataFromFileBuffer
     * for links to documentation on controller remapping configuration.
     */
    uint32_t deviceDenylistStringTableIndex;
} Paddleboat_Controller_Mapping_File_Controller_Entry;

/**
* @brief A structure that defines a string table entry
* in the PaddleboatMappingTool file.
*/
typedef struct  __attribute__((packed)) Paddleboat_Controller_Mapping_File_String_Entry {
    /** @brief An ASCII string, expected to be 0 terminated.
     * Must fit, including termination in
     * `PADDLEBOAT_STRING_TABLE_ENTRY_MAX_SIZE`
     */
    char stringTableEntry[PADDLEBOAT_STRING_TABLE_ENTRY_MAX_SIZE];
} Paddleboat_Controller_Mapping_File_String_Entry;

} // namespace paddleboat
