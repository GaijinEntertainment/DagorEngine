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

#include "GameControllerManager.h"
#include "paddleboat.h"

using namespace paddleboat;

extern "C" {

#define PADDLEBOAT_VERSION_CONCAT_NX(PREFIX, MAJOR, MINOR, BUGFIX) \
    PREFIX##_##MAJOR##_##MINOR##_##BUGFIX
#define PADDLEBOAT_VERSION_CONCAT(PREFIX, MAJOR, MINOR, BUGFIX) \
    PADDLEBOAT_VERSION_CONCAT_NX(PREFIX, MAJOR, MINOR, BUGFIX)
#define PADDLEBOAT_VERSION_SYMBOL                                           \
    PADDLEBOAT_VERSION_CONCAT(PADDLEBOAT_version, PADDLEBOAT_MAJOR_VERSION, \
                              PADDLEBOAT_MINOR_VERSION,                     \
                              PADDLEBOAT_BUGFIX_VERSION)

void PADDLEBOAT_VERSION_SYMBOL();

// Private, used internally by mapping file check
uint32_t Paddleboat_getVersion() {
    return PADDLEBOAT_PACKED_VERSION;
}

Paddleboat_ErrorCode Paddleboat_init(JNIEnv *env, jobject jcontext) {
    PADDLEBOAT_VERSION_SYMBOL();
    Paddleboat_ErrorCode errorCode = GameControllerManager::init(env, jcontext);
    if (errorCode == PADDLEBOAT_NO_ERROR) {
        GameControllerManager::update(env);
    }
    return errorCode;
}

bool Paddleboat_isInitialized() {
    return GameControllerManager::isInitialized();
}

void Paddleboat_destroy(JNIEnv *env) {
    GameControllerManager::destroyInstance(env);
}

void Paddleboat_onStop(JNIEnv *env) { GameControllerManager::onStop(env); }

void Paddleboat_onStart(JNIEnv *env) { GameControllerManager::onStart(env); }

int32_t Paddleboat_processInputEvent(const AInputEvent *event) {
    return GameControllerManager::processInputEvent(event);
}

int32_t Paddleboat_processGameActivityKeyInputEvent(const void *event,
                                                    const size_t eventSize) {
    return GameControllerManager::processGameActivityKeyInputEvent(event,
                                                                   eventSize);
}

int32_t Paddleboat_processGameActivityMotionInputEvent(const void *event,
                                                       const size_t eventSize) {
    return GameControllerManager::processGameActivityMotionInputEvent(
        event, eventSize);
}

uint64_t Paddleboat_getActiveAxisMask() {
    return GameControllerManager::getActiveAxisMask();
}

bool Paddleboat_getBackButtonConsumed() {
    return GameControllerManager::getBackButtonConsumed();
}

Paddleboat_Integrated_Motion_Sensor_Flags Paddleboat_getIntegratedMotionSensorFlags() {
    return GameControllerManager::getIntegratedMotionSensorFlags();
}

void Paddleboat_setBackButtonConsumed(bool consumeBackButton) {
    GameControllerManager::setBackButtonConsumed(consumeBackButton);
}

void Paddleboat_setControllerStatusCallback(
    Paddleboat_ControllerStatusCallback statusCallback, void *userData) {
    GameControllerManager::setControllerStatusCallback(statusCallback,
                                                       userData);
}

void Paddleboat_setMotionDataCallback(
    Paddleboat_MotionDataCallback motionDataCallback, void *userData) {
    GameControllerManager::setMotionDataCallback(motionDataCallback,
                                                 PADDLEBOAT_INTEGRATED_SENSOR_NONE, userData);
}

Paddleboat_ErrorCode Paddleboat_setMotionDataCallbackWithIntegratedFlags(
        Paddleboat_MotionDataCallback motionDataCallback,
        Paddleboat_Integrated_Motion_Sensor_Flags integratedSensorFlags,
        void *userData) {
    return GameControllerManager::setMotionDataCallback(motionDataCallback,
                                                        integratedSensorFlags, userData);
}

void Paddleboat_setMouseStatusCallback(
    Paddleboat_MouseStatusCallback statusCallback, void *userData) {
    GameControllerManager::setMouseStatusCallback(statusCallback, userData);
}

void Paddleboat_setPhysicalKeyboardStatusCallback(
        Paddleboat_PhysicalKeyboardStatusCallback statusCallback,
        void *userData) {
    GameControllerManager::setPhysicalKeyboardStatusCallback(statusCallback, userData);
}

Paddleboat_ErrorCode Paddleboat_getControllerData(
    const int32_t controllerIndex, Paddleboat_Controller_Data *controllerData) {
    return GameControllerManager::getControllerData(controllerIndex,
                                                    controllerData);
}

Paddleboat_ErrorCode Paddleboat_getControllerInfo(
    const int32_t controllerIndex, Paddleboat_Controller_Info *controllerInfo) {
    return GameControllerManager::getControllerInfo(controllerIndex,
                                                    controllerInfo);
}

Paddleboat_ErrorCode Paddleboat_getControllerName(const int32_t controllerIndex,
                                                  const size_t bufferSize,
                                                  char *controllerName) {
    return GameControllerManager::getControllerName(controllerIndex, bufferSize,
                                                    controllerName);
}

Paddleboat_ControllerStatus Paddleboat_getControllerStatus(
    const int32_t controllerIndex) {
    return GameControllerManager::getControllerStatus(controllerIndex);
}

Paddleboat_ErrorCode Paddleboat_setControllerLight(
    const int32_t controllerIndex, const Paddleboat_LightType lightType,
    const uint32_t lightData, JNIEnv *env) {
    return GameControllerManager::setControllerLight(controllerIndex, lightType,
                                                     lightData, env);
}

Paddleboat_ErrorCode Paddleboat_setControllerVibrationData(
    const int32_t controllerIndex,
    const Paddleboat_Vibration_Data *vibrationData, JNIEnv *env) {
    return GameControllerManager::setControllerVibrationData(
        controllerIndex, vibrationData, env);
}

Paddleboat_ErrorCode Paddleboat_getMouseData(Paddleboat_Mouse_Data *mouseData) {
    return GameControllerManager::getMouseData(mouseData);
}

Paddleboat_MouseStatus Paddleboat_getMouseStatus() {
    return GameControllerManager::getMouseStatus();
}

bool Paddleboat_getPhysicalKeyboardStatus() {
    return GameControllerManager::getPhysicalKeyboardStatus();
}

void Paddleboat_addControllerRemapData(
    const Paddleboat_Remap_Addition_Mode addMode,
    const int32_t remapTableEntryCount,
    const Paddleboat_Controller_Mapping_Data *mappingData) {
    GameControllerManager::addControllerRemapData(addMode, remapTableEntryCount,
                                                  mappingData);
}

Paddleboat_ErrorCode Paddleboat_addControllerRemapDataFromFd(
        const Paddleboat_Remap_Addition_Mode addMode,
        const int mappingFileDescriptor) {
    return GameControllerManager::addControllerRemapDataFromFd(addMode, mappingFileDescriptor);
}

Paddleboat_ErrorCode Paddleboat_addControllerRemapDataFromFileBuffer(
        const Paddleboat_Remap_Addition_Mode addMode,
        const void *mappingFileBuffer,
        const size_t mappingFileBufferSize) {
    return GameControllerManager::addControllerRemapDataFromFileBuffer(addMode,
        reinterpret_cast<const Paddleboat_Controller_Mapping_File_Header *>(mappingFileBuffer),
        mappingFileBufferSize);
}

int32_t Paddleboat_getControllerRemapTableData(
    const int32_t destRemapTableEntryCount,
    Paddleboat_Controller_Mapping_Data *mappingData) {
    return GameControllerManager::getControllerRemapTableData(
        destRemapTableEntryCount, mappingData);
}

void Paddleboat_update(JNIEnv *env) { GameControllerManager::update(env); }

int32_t Paddleboat_getLastKeycode() {
    return GameControllerManager::getLastKeycode();
}

void PADDLEBOAT_VERSION_SYMBOL() {
    // Intentionally empty: this function is used to ensure that the proper
    // version of the library is linked against the proper headers.
    // In case of mismatch, a linker error will be triggered because of an
    // undefined symbol, as the name of the function depends on the version.
}
}  // extern "C" {