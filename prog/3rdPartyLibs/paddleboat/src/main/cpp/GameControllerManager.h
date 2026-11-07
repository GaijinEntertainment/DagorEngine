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

#include <android/input.h>
#include <jni.h>

#include <mutex>

#include "GameController.h"
#include "GameControllerMappingInfo.h"
#include "ThreadUtil.h"

namespace paddleboat {

class GameControllerManager {
   private:
    // Allows construction with std::unique_ptr from a static method, but
    // disallows construction outside of the class since no one else can
    // construct a ConstructorTag
    struct ConstructorTag {};

    static constexpr int32_t MAX_MOUSE_DEVICES = 2;
    static constexpr int32_t INVALID_MOUSE_ID = -1;

    // Assuming update is getting called at 60Hz, wait one minute in between
    // checking battery status
    static constexpr int32_t BATTERY_REFRESH_WAIT = 60 * 60;

   public:
    GameControllerManager(JNIEnv *env, jobject jcontext, ConstructorTag);

    ~GameControllerManager();

    static Paddleboat_ErrorCode init(JNIEnv *env, jobject jcontext);

    static void destroyInstance(JNIEnv *env);

    static bool isInitialized();

    static bool getPhysicalKeyboardStatus();

    static Paddleboat_Integrated_Motion_Sensor_Flags getIntegratedMotionSensorFlags();

    // Get/Set whether AKEYCODE_BACK is 'eaten' or allowed to pass through to
    // the system This can be used to block the OS backing out of the game, or
    // allowing it if the game is in an appropriate state (i.e. the title
    // screen)
    static bool getBackButtonConsumed();

    static void setBackButtonConsumed(bool consumed);

    static Paddleboat_ErrorCode setControllerLight(
        const int32_t controllerIndex, const Paddleboat_LightType lightType,
        const uint32_t lightData, JNIEnv *env);

    static void setControllerStatusCallback(
        Paddleboat_ControllerStatusCallback statusCallback, void *userData);

    static Paddleboat_ErrorCode setMotionDataCallback(
        Paddleboat_MotionDataCallback motionDataCallback,
        Paddleboat_Integrated_Motion_Sensor_Flags integratedFlags, void *userData);

    static void setMouseStatusCallback(
        Paddleboat_MouseStatusCallback statusCallback, void *userData);

    static void setPhysicalKeyboardStatusCallback(
            Paddleboat_PhysicalKeyboardStatusCallback statusCallback, void *userData);

    static void onStop(JNIEnv *env);

    static void onStart(JNIEnv *env);

    static void update(JNIEnv *env);

    static Paddleboat_ErrorCode getControllerData(
        const int32_t controllerIndex,
        Paddleboat_Controller_Data *controllerData);

    static Paddleboat_ErrorCode getControllerInfo(
        const int32_t controllerIndex, Paddleboat_Controller_Info *deviceInfo);

    static Paddleboat_ErrorCode getControllerName(const int32_t controllerIndex,
                                                  const size_t bufferSize,
                                                  char *controllerName);

    static Paddleboat_ControllerStatus getControllerStatus(
        const int32_t controllerIndex);

    static Paddleboat_ErrorCode setControllerVibrationData(
        const int32_t controllerIndex,
        const Paddleboat_Vibration_Data *vibrationData, JNIEnv *env);

    static Paddleboat_ErrorCode getMouseData(Paddleboat_Mouse_Data *mouseData);

    static Paddleboat_MouseStatus getMouseStatus();

    static int32_t processInputEvent(const AInputEvent *event);

    static int32_t processGameActivityKeyInputEvent(const void *event,
                                                    const size_t eventSize);

    static int32_t processGameActivityMotionInputEvent(const void *event,
                                                       const size_t eventSize);

    static uint64_t getActiveAxisMask();

    static void addControllerRemapData(
        const Paddleboat_Remap_Addition_Mode addMode,
        const int32_t remapTableEntryCount,
        const Paddleboat_Controller_Mapping_Data *mappingData);

    static int32_t getControllerRemapTableData(
        const int32_t destRemapTableEntryCount,
        Paddleboat_Controller_Mapping_Data *mappingData);

    static Paddleboat_ErrorCode addControllerRemapDataFromFd(
            const Paddleboat_Remap_Addition_Mode addMode,
            const int fileDescriptor);

    static Paddleboat_ErrorCode addControllerRemapDataFromFileBuffer(
            const Paddleboat_Remap_Addition_Mode addMode,
            const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
            const size_t mappingFileBufferSize);

    // Called from the JNI bridge functions
    static GameControllerDeviceInfo *onConnection();

    static void onDisconnection(const int32_t deviceId);

    static void onKeyboardConnection(const int32_t deviceId);

    static void onKeyboardDisconnection(const int32_t deviceId);

    static void onMotionData(const int32_t deviceId, const int32_t motionType,
                             const uint64_t timestamp, const float dataX,
                             const float dataY, const float dataZ);

    static void onMouseConnection(const int32_t deviceId);

    static void onMouseDisconnection(const int32_t deviceId);

    static jclass getGameControllerClass();

    static jobject getGameControllerObject();

    // device debug helper function
    static int32_t getLastKeycode();

   private:
    static GameControllerManager *getInstance();

    const float *getAxisValuesFromGameActivityMotionEvent(const void *event,
                                                          const size_t eventSize,
                                                          const uint32_t pointerIndex);

    Paddleboat_ErrorCode initMethods(JNIEnv *env);

    bool isLightTypeSupported(const Paddleboat_Controller_Info &controllerInfo,
                              const Paddleboat_LightType lightType);

    int32_t processControllerKeyEvent(const AInputEvent *event,
                                      GameController &gameController);

    int32_t processControllerGameActivityKeyEvent(
        const Paddleboat_GameActivityKeyEvent *event, const size_t eventSize,
        GameController &gameController);

    int32_t processMouseEvent(const AInputEvent *event);

    int32_t processGameActivityMouseEvent(const void *event,
                                          const size_t eventSize,
                                          const int32_t eventDeviceId);

    void rescanVirtualMouseControllers();

    void updateBattery(JNIEnv *env);

    void updateMouseDataTimestamp();

    void releaseGlobals(JNIEnv *env);

    const Paddleboat_Controller_Mapping_File_Controller_Entry *getMapForController(
        const GameController &gameController);

    bool mInitialized = false;
    bool mGCMClassInitialized = false;
    bool mActiveSensorFlagsDirty = false;
    bool mBackButtonConsumed = true;
    bool mMotionEventReporting = false;
    bool mPhysicalKeyboardConnected = false;

    int32_t mApiLevel = 16;
    int32_t mBatteryWait = BATTERY_REFRESH_WAIT;
    uint32_t mIntegratedSensorFlags = 0;
    jobject mContext = NULL;
    jclass mGameControllerClass = NULL;
    jobject mGameControllerObject = NULL;
    jmethodID mInitMethodId = NULL;
    jmethodID mGetApiLevelMethodId = NULL;
    jmethodID mGetBatteryLevelMethodId = NULL;
    jmethodID mGetBatteryStatusMethodId = NULL;
    jmethodID mGetIntegratedSensorMethodId = NULL;
    jmethodID mSetActiveIntegratedSensorsMethodId = NULL;
    jmethodID mSetLightMethodId = NULL;
    jmethodID mSetNativeReadyMethodId = NULL;
    jmethodID mSetReportMotionEventsMethodId = NULL;
    jmethodID mSetVibrationMethodId = NULL;

    uint64_t mActiveAxisMask = 0;

    GameControllerMappingInfo mMappingInfo;

    Paddleboat_MotionDataCallback mMotionDataCallback = nullptr;
    void *mMotionDataCallbackUserData = nullptr;
    Paddleboat_Integrated_Motion_Sensor_Flags mMotionDataCallbackFlags =
            PADDLEBOAT_INTEGRATED_SENSOR_NONE;

    GameController mGameControllers[PADDLEBOAT_MAX_CONTROLLERS];
    Paddleboat_ControllerStatusCallback mStatusCallback = nullptr;
    void *mStatusCallbackUserData = nullptr;
    // device debug helper
    int32_t mLastKeyEventKeyCode = 0;

    Paddleboat_PhysicalKeyboardStatusCallback mKeyboardCallback = nullptr;
    void *mKeyboardCallbackUserData = nullptr;

    Paddleboat_MouseStatus mMouseStatus = PADDLEBOAT_MOUSE_NONE;
    int32_t mMouseDeviceIds[MAX_MOUSE_DEVICES] = {INVALID_MOUSE_ID,
                                                  INVALID_MOUSE_ID};
    int32_t mMouseControllerIndex = INVALID_MOUSE_ID;
    Paddleboat_Mouse_Data mMouseData;
    Paddleboat_MouseStatusCallback mMouseCallback = nullptr;
    void *mMouseCallbackUserData = nullptr;

    std::mutex mUpdateMutex;
    static std::mutex sInstanceMutex;
    static std::unique_ptr<GameControllerManager> sInstance
        GUARDED_BY(sInstanceMutex);
};
}  // namespace paddleboat
