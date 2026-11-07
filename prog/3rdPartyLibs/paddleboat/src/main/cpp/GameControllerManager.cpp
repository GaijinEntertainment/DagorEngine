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

#include <android/api-level.h>

#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "GameControllerInternalConstants.h"
#include "GameControllerLog.h"
#include "GameControllerMappingUtils.h"
#include "InternalControllerTable.h"
#include "Log.h"

#define ARRAY_COUNTOF(array) (sizeof(array) / sizeof(array[0]))
#define LOG_TAG "GameControllerManager"
// If defined, output information about qualifying game controller
// event data to log
//#define LOG_INPUT_EVENTS

// JNI interface functions

extern "C" {

void Java_com_google_android_games_paddleboat_GameControllerManager_onControllerConnected(
    JNIEnv *env, jobject gcmObject, jintArray deviceInfoArray,
    jfloatArray axisMinArray, jfloatArray axisMaxArray,
    jfloatArray axisFlatArray, jfloatArray axisFuzzArray) {
    paddleboat::GameControllerDeviceInfo *deviceInfo =
        paddleboat::GameControllerManager::onConnection();
    if (deviceInfo != nullptr) {
        // Copy the array contents into the DeviceInfo array equivalents
        const jsize infoArraySize = env->GetArrayLength(deviceInfoArray);
        if ((infoArraySize * sizeof(int32_t)) ==
            sizeof(paddleboat::GameControllerDeviceInfo::InfoFields)) {
            env->GetIntArrayRegion(
                deviceInfoArray, 0, infoArraySize,
                reinterpret_cast<jint *>(deviceInfo->getInfo()));
        } else {
            ALOGE(
                "deviceInfoArray/GameControllerDeviceInfo::InfoFields size "
                "mismatch");
        }
#if defined LOG_INPUT_EVENTS
        ALOGI("onControllerConnected deviceId %d",
              deviceInfo->getInfo()->mDeviceId);
#endif

        // all axis arrays are presumed to be the same size
        const jsize axisArraySize = env->GetArrayLength(axisMinArray);
        if ((axisArraySize * sizeof(float)) ==
            (sizeof(float) * paddleboat::MAX_AXIS_COUNT)) {
            env->GetFloatArrayRegion(axisMinArray, 0, axisArraySize,
                                     deviceInfo->getMinArray());
            env->GetFloatArrayRegion(axisMaxArray, 0, axisArraySize,
                                     deviceInfo->getMaxArray());
            env->GetFloatArrayRegion(axisFlatArray, 0, axisArraySize,
                                     deviceInfo->getFlatArray());
            env->GetFloatArrayRegion(axisFuzzArray, 0, axisArraySize,
                                     deviceInfo->getFuzzArray());
        } else {
            ALOGE(
                "axisArray/GameControllerDeviceInfo::axisArray size mismatch");
        }

        // Retrieve the device name string
        const jint deviceId = deviceInfo->getInfo()->mDeviceId;
        jmethodID getDeviceNameById = env->GetMethodID(
            paddleboat::GameControllerManager::getGameControllerClass(),
            "getDeviceNameById", "(I)Ljava/lang/String;");
        if (getDeviceNameById != NULL) {
            jstring deviceNameJstring = reinterpret_cast<
                jstring>(env->CallObjectMethod(
                paddleboat::GameControllerManager::getGameControllerObject(),
                getDeviceNameById, deviceId));
            const char *deviceName =
                env->GetStringUTFChars(deviceNameJstring, NULL);
            if (deviceName != nullptr) {
                deviceInfo->setName(deviceName);
            }
            env->ReleaseStringUTFChars(deviceNameJstring, deviceName);
        }
    }
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onControllerDisconnected(
    JNIEnv *env, jobject gcmObject, jint deviceId) {
    paddleboat::GameControllerManager::onDisconnection(deviceId);
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onMotionData(
    JNIEnv *env, jobject gcmObject, jint deviceId, jint motionType,
    jlong timestamp, jfloat dataX, jfloat dataY, jfloat dataZ) {
    paddleboat::GameControllerManager::onMotionData(
        deviceId, motionType, timestamp, dataX, dataY, dataZ);
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onMouseConnected(
    JNIEnv *env, jobject gcmObject, jint deviceId) {
    paddleboat::GameControllerManager::onMouseConnection(deviceId);
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onMouseDisconnected(
    JNIEnv *env, jobject gcmObject, jint deviceId) {
    paddleboat::GameControllerManager::onMouseDisconnection(deviceId);
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onKeyboardConnected(
        JNIEnv *env, jobject gcmObject, jint deviceId) {
    paddleboat::GameControllerManager::onKeyboardConnection(deviceId);
}

void Java_com_google_android_games_paddleboat_GameControllerManager_onKeyboardDisconnected(
        JNIEnv *env, jobject gcmObject, jint deviceId) {
    paddleboat::GameControllerManager::onKeyboardDisconnection(deviceId);
}

}  // extern "C"

namespace paddleboat {
constexpr const char *CLASSLOADER_CLASS = "java/lang/ClassLoader";
constexpr const char *CLASSLOADER_GETCLASSLOADER_METHOD_NAME = "getClassLoader";
constexpr const char *CLASSLOADER_GETCLASSLOADER_METHOD_SIG =
    "()Ljava/lang/ClassLoader;";
constexpr const char *CLASSLOADER_LOADCLASS_METHOD_NAME = "loadClass";
constexpr const char *CLASSLOADER_LOADCLASS_METHOD_SIG =
    "(Ljava/lang/String;)Ljava/lang/Class;";

constexpr const char *GCM_CLASSNAME =
    "com/google/android/games/paddleboat/GameControllerManager";
constexpr const char *GCM_INIT_METHOD_NAME = "<init>";
constexpr const char *GCM_INIT_METHOD_SIGNATURE =
    "(Landroid/content/Context;Z)V";
constexpr const char *GCM_ONSTOP_METHOD_NAME = "onStop";
constexpr const char *GCM_ONSTART_METHOD_NAME = "onStart";
constexpr const char *GCM_GETAPILEVEL_METHOD_NAME = "getApiLevel";
constexpr const char *GCM_GETAPILEVEL_METHOD_SIGNATURE = "()I";
constexpr const char *GCM_GETBATTERYLEVEL_METHOD_NAME = "getBatteryLevel";
constexpr const char *GCM_GETBATTERYLEVEL_METHOD_SIGNATURE = "(I)F";
constexpr const char *GCM_GETBATTERYSTATUS_METHOD_NAME = "getBatteryStatus";
constexpr const char *GCM_GETBATTERYSTATUS_METHOD_SIGNATURE = "(I)I";
constexpr const char *GCM_GETINTEGRATED_METHOD_NAME = "getIntegratedSensorFlags";
constexpr const char *GCM_GETINTEGRATED_METHOD_SIGNATURE = "()I";
constexpr const char *GCM_SETACTIVESENSOR_METHOD_NAME = "setActiveIntegratedSensors";
constexpr const char *GCM_SETACTIVESENSOR_METHOD_SIGNATURE = "(I)V";
constexpr const char *GCM_SETLIGHT_METHOD_NAME = "setLight";
constexpr const char *GCM_SETLIGHT_METHOD_SIGNATURE = "(III)V";
constexpr const char *GCM_SETNATIVEREADY_METHOD_NAME = "setNativeReady";
constexpr const char *GCM_SETREPORTMOTIONEVENTS_METHOD_NAME =
    "setReportMotionEvents";
constexpr const char *GCM_SETVIBRATION_METHOD_NAME = "setVibration";
constexpr const char *GCM_SETVIBRATION_METHOD_SIGNATURE = "(IIIII)V";

constexpr const char *VOID_METHOD_SIGNATURE = "()V";

constexpr float VIBRATION_INTENSITY_SCALE = 255.0f;

typedef struct MethodTableEntry {
    const char *methodName;
    const char *methodSignature;
    jmethodID *methodID;
} MethodTableEntry;

const JNINativeMethod GCM_NATIVE_METHODS[] = {
    {"onControllerConnected", "([I[F[F[F[F)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onControllerConnected)},
    {"onControllerDisconnected", "(I)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onControllerDisconnected)},
    {"onMotionData", "(IIJFFF)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onMotionData)},
    {"onKeyboardConnected", "(I)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onKeyboardConnected)},
    {"onKeyboardDisconnected", "(I)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onKeyboardDisconnected)},
    {"onMouseConnected", "(I)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onMouseConnected)},
    {"onMouseDisconnected", "(I)V",
     reinterpret_cast<void *>(
         Java_com_google_android_games_paddleboat_GameControllerManager_onMouseDisconnected)},
};

std::mutex GameControllerManager::sInstanceMutex;
std::unique_ptr<GameControllerManager> GameControllerManager::sInstance;

Paddleboat_ErrorCode GameControllerManager::init(JNIEnv *env,
                                                 jobject jcontext) {
    std::lock_guard<std::mutex> lock(sInstanceMutex);
    if (sInstance) {
        ALOGE("Attempted to initialize Paddleboat twice");
        return PADDLEBOAT_ERROR_ALREADY_INITIALIZED;
    }

    sInstance = std::make_unique<GameControllerManager>(env, jcontext,
                                                        ConstructorTag{});
    if (!sInstance->mInitialized) {
        ALOGE("Failed to initialize Paddleboat");
        return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
    }
    GameControllerManager *gcm = sInstance.get();
    if (gcm == nullptr) {
        ALOGE("Failed to initialize Paddleboat");
        return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
    }

    // Load our GameControllerManager class
    jclass activityClass = env->GetObjectClass(jcontext);
    jmethodID getClassLoaderID =
        env->GetMethodID(activityClass, CLASSLOADER_GETCLASSLOADER_METHOD_NAME,
                         CLASSLOADER_GETCLASSLOADER_METHOD_SIG);
    jobject classLoaderObject =
        env->CallObjectMethod(jcontext, getClassLoaderID);
    jclass classLoader = env->FindClass(CLASSLOADER_CLASS);
    jmethodID loadClassID =
        env->GetMethodID(classLoader, CLASSLOADER_LOADCLASS_METHOD_NAME,
                         CLASSLOADER_LOADCLASS_METHOD_SIG);
    jstring gcmClassName = env->NewStringUTF(GCM_CLASSNAME);
    jclass gcmClass = jclass(
        env->CallObjectMethod(classLoaderObject, loadClassID, gcmClassName));
    if (gcmClass == NULL) {
        ALOGE("Failed to find GameControllerManager class");
        return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
    }

    // Register our native methods, in case we were linked as a static library
    int rc = env->RegisterNatives(
        gcmClass, GCM_NATIVE_METHODS,
        sizeof(GCM_NATIVE_METHODS) / sizeof(JNINativeMethod));
    if (rc != JNI_OK) {
        ALOGE("Failed to register native methods. %d", rc);
        return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
    }

    jclass global_gcmClass = static_cast<jclass>(env->NewGlobalRef(gcmClass));

    // Retrieve our JNI methodIDs to the game controller manager class object
    gcm->mGameControllerClass = global_gcmClass;
    Paddleboat_ErrorCode methodResult = gcm->initMethods(env);
    if (methodResult != PADDLEBOAT_NO_ERROR) {
        return methodResult;
    }

#if _DEBUG
    jboolean printControllerInfo = JNI_TRUE;
#else
    jboolean printControllerInfo = JNI_FALSE;
#endif
    jobject gcmObject = env->NewObject(global_gcmClass, gcm->mInitMethodId,
                                       jcontext, printControllerInfo);
    if (gcmObject == NULL) {
        ALOGE("Failed to create GameControllerManager");
        return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
    }
    jobject global_gcmObject = env->NewGlobalRef(gcmObject);
    env->DeleteLocalRef(gcmObject);

    gcm->mGameControllerObject = global_gcmObject;

    return PADDLEBOAT_NO_ERROR;
}

void GameControllerManager::destroyInstance(JNIEnv *env) {
    std::lock_guard<std::mutex> lock(sInstanceMutex);
    sInstance.get()->releaseGlobals(env);
    sInstance.reset();
}

void GameControllerManager::releaseGlobals(JNIEnv *env) {
    if (mGameControllerClass != NULL) {
        env->DeleteGlobalRef(mGameControllerClass);
        mGameControllerClass = NULL;
    }
    if (mGameControllerObject != NULL) {
        env->DeleteGlobalRef(mGameControllerObject);
        mGameControllerObject = NULL;
    }
}

GameControllerManager *GameControllerManager::getInstance() {
    std::lock_guard<std::mutex> lock(sInstanceMutex);
    return sInstance.get();
}

bool GameControllerManager::isInitialized() {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        // This is a case of error.
        // We do not log anything here, so that we do not spam
        // the user when this function is called each frame.
        return false;
    }
    return gcm->mGCMClassInitialized;
}

Paddleboat_ErrorCode GameControllerManager::initMethods(JNIEnv *env) {
    const MethodTableEntry methodTable[] = {
        {GCM_INIT_METHOD_NAME, GCM_INIT_METHOD_SIGNATURE, &mInitMethodId},
        {GCM_GETAPILEVEL_METHOD_NAME, GCM_GETAPILEVEL_METHOD_SIGNATURE,
         &mGetApiLevelMethodId},
        {GCM_GETBATTERYLEVEL_METHOD_NAME, GCM_GETBATTERYLEVEL_METHOD_SIGNATURE,
         &mGetBatteryLevelMethodId},
        {GCM_GETBATTERYSTATUS_METHOD_NAME,
         GCM_GETBATTERYSTATUS_METHOD_SIGNATURE, &mGetBatteryStatusMethodId},
        {GCM_GETINTEGRATED_METHOD_NAME,
         GCM_GETINTEGRATED_METHOD_SIGNATURE, &mGetIntegratedSensorMethodId},
        {GCM_SETACTIVESENSOR_METHOD_NAME,
         GCM_SETACTIVESENSOR_METHOD_SIGNATURE, &mSetActiveIntegratedSensorsMethodId},
        {GCM_SETVIBRATION_METHOD_NAME, GCM_SETVIBRATION_METHOD_SIGNATURE,
         &mSetVibrationMethodId},
        {GCM_SETLIGHT_METHOD_NAME, GCM_SETLIGHT_METHOD_SIGNATURE,
         &mSetLightMethodId},
        {GCM_SETNATIVEREADY_METHOD_NAME, VOID_METHOD_SIGNATURE,
         &mSetNativeReadyMethodId},
        {GCM_SETREPORTMOTIONEVENTS_METHOD_NAME, VOID_METHOD_SIGNATURE,
         &mSetReportMotionEventsMethodId}};
    const size_t methodTableCount = ARRAY_COUNTOF(methodTable);

    for (size_t i = 0; i < methodTableCount; ++i) {
        const MethodTableEntry &entry = methodTable[i];
        jmethodID methodID = env->GetMethodID(
            mGameControllerClass, entry.methodName, entry.methodSignature);
        if (methodID == NULL) {
            ALOGE("Failed to find %s init method", entry.methodName);
            return PADDLEBOAT_ERROR_INIT_GCM_FAILURE;
        } else {
            *entry.methodID = methodID;
        }
    }
    return PADDLEBOAT_NO_ERROR;
}

GameControllerManager::GameControllerManager(JNIEnv *env, jobject jcontext,
                                             ConstructorTag) {
    mContext = jcontext;
    mMouseData.timestamp = 0;
    mMouseData.buttonsDown = 0;
    mMouseData.mouseScrollDeltaH = 0;
    mMouseData.mouseScrollDeltaV = 0;
    mMouseData.mouseX = 0.0f;
    mMouseData.mouseY = 0.0f;
    mInitialized = true;

    const Paddleboat_Internal_Mapping_Header *mappingHeader = GetInternalMappingHeader();
    mMappingInfo.mAxisTableEntryCount = mappingHeader->axisTableEntryCount;
    mMappingInfo.mButtonTableEntryCount = mappingHeader->buttonTableEntryCount;
    mMappingInfo.mControllerTableEntryCount = mappingHeader->controllerTableEntryCount;
    mMappingInfo.mStringTableEntryCount = mappingHeader->stringTableEntryCount;
    memcpy(mMappingInfo.mAxisTable, mappingHeader->axisTable,
           sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry) *
                   mMappingInfo.mAxisTableEntryCount);
    memcpy(mMappingInfo.mButtonTable, mappingHeader->buttonTable,
           sizeof(Paddleboat_Controller_Mapping_File_Button_Entry) *
                   mMappingInfo.mButtonTableEntryCount);
    memcpy(mMappingInfo.mControllerTable, mappingHeader->controllerTable,
           sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry) *
                   mMappingInfo.mControllerTableEntryCount);
    memcpy(mMappingInfo.mStringTable, mappingHeader->stringTable,
           sizeof(Paddleboat_Controller_Mapping_File_String_Entry) *
                   mMappingInfo.mStringTableEntryCount);

    // Our minimum supported API level, we will retrieve the actual runtime API
    // level later on calling getApiLevel
    mApiLevel = 16;
}

GameControllerManager::~GameControllerManager() { mInitialized = false; }

int32_t GameControllerManager::processInputEvent(const AInputEvent *event) {
    int32_t handledEvent = IGNORED_EVENT;
    if (event != nullptr) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);
            const int32_t eventSource = AInputEvent_getSource(event);
            const int32_t dpadSource = eventSource & AINPUT_SOURCE_DPAD;
            const int32_t gamepadSource = eventSource & AINPUT_SOURCE_GAMEPAD;
            const int32_t joystickSource = eventSource & AINPUT_SOURCE_JOYSTICK;
            const int32_t mouseSource = eventSource & AINPUT_SOURCE_MOUSE;
            if (dpadSource == AINPUT_SOURCE_DPAD ||
                gamepadSource == AINPUT_SOURCE_GAMEPAD ||
                joystickSource == AINPUT_SOURCE_JOYSTICK) {
                const int32_t eventDeviceId = AInputEvent_getDeviceId(event);
                for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                    if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                        if (gcm->mGameControllers[i].getControllerStatus() ==
                            PADDLEBOAT_CONTROLLER_ACTIVE) {
                            const GameControllerDeviceInfo &deviceInfo =
                                gcm->mGameControllers[i].getDeviceInfo();
                            if (deviceInfo.getInfo().mDeviceId ==
                                eventDeviceId) {
                                const int32_t eventType =
                                    AInputEvent_getType(event);
                                if (eventType == AINPUT_EVENT_TYPE_KEY) {
                                    handledEvent =
                                        gcm->processControllerKeyEvent(
                                            event, gcm->mGameControllers[i]);
                                } else if (eventType ==
                                           AINPUT_EVENT_TYPE_MOTION) {
                                    handledEvent =
                                        gcm->mGameControllers[i]
                                            .processMotionEvent(event);
                                }
                                break;
                            }
                        }
                    }
                }
            } else if (mouseSource == AINPUT_SOURCE_MOUSE) {
                handledEvent = gcm->processMouseEvent(event);
            }
#if defined LOG_INPUT_EVENTS
            LogInputEvent(event);
#endif
        }
    }
    return handledEvent;
}

int32_t GameControllerManager::processGameActivityKeyInputEvent(
    const void *event, const size_t eventSize) {
    int32_t handledEvent = IGNORED_EVENT;
    if (event != nullptr) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);
            const Paddleboat_GameActivityKeyEvent *keyEvent =
                reinterpret_cast<const Paddleboat_GameActivityKeyEvent *>(
                    event);
            const int32_t eventSource = keyEvent->source;
            const int32_t eventDeviceId = keyEvent->deviceId;
            const int32_t dpadSource = eventSource & AINPUT_SOURCE_DPAD;
            const int32_t gamepadSource = eventSource & AINPUT_SOURCE_GAMEPAD;
            const int32_t joystickSource = eventSource & AINPUT_SOURCE_JOYSTICK;
            if (dpadSource == AINPUT_SOURCE_DPAD ||
                gamepadSource == AINPUT_SOURCE_GAMEPAD ||
                joystickSource == AINPUT_SOURCE_JOYSTICK) {
                for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                    if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                        if (gcm->mGameControllers[i].getControllerStatus() ==
                            PADDLEBOAT_CONTROLLER_ACTIVE) {
                            const GameControllerDeviceInfo &deviceInfo =
                                gcm->mGameControllers[i].getDeviceInfo();
                            if (deviceInfo.getInfo().mDeviceId ==
                                eventDeviceId) {
                                handledEvent =
                                    gcm->processControllerGameActivityKeyEvent(
                                        keyEvent, eventSize,
                                        gcm->mGameControllers[i]);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return handledEvent;
}

int32_t GameControllerManager::processGameActivityMotionInputEvent(
    const void *event, const size_t eventSize) {
    int32_t handledEvent = IGNORED_EVENT;
    if (event != nullptr) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);
            // The fields we access are the same on original and V2 structs
            const Paddleboat_GameActivityMotionEvent *motionEvent =
                reinterpret_cast<const Paddleboat_GameActivityMotionEvent *>(
                    event);
            // Axis values are different depending on struct version
            const float *axisValues = gcm->getAxisValuesFromGameActivityMotionEvent(event,
                                                                                    eventSize, 0);
            if (axisValues != nullptr) {
                const int32_t eventSource = motionEvent->source;
                const int32_t eventDeviceId = motionEvent->deviceId;
                const int32_t dpadSource = eventSource & AINPUT_SOURCE_DPAD;
                const int32_t gamepadSource = eventSource & AINPUT_SOURCE_GAMEPAD;
                const int32_t joystickSource = eventSource & AINPUT_SOURCE_JOYSTICK;
                const int32_t mouseSource = eventSource & AINPUT_SOURCE_MOUSE;
                if (dpadSource == AINPUT_SOURCE_DPAD ||
                    gamepadSource == AINPUT_SOURCE_GAMEPAD ||
                    joystickSource == AINPUT_SOURCE_JOYSTICK) {
                    for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                        if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                            if (gcm->mGameControllers[i].getControllerStatus() ==
                                PADDLEBOAT_CONTROLLER_ACTIVE) {
                                const GameControllerDeviceInfo &deviceInfo =
                                        gcm->mGameControllers[i].getDeviceInfo();
                                if (deviceInfo.getInfo().mDeviceId ==
                                    eventDeviceId) {
                                    handledEvent =
                                            gcm->mGameControllers[i]
                                                    .processGameActivityMotionEvent(
                                                            axisValues);
                                    break;
                                }
                            }
                        }
                    }
                } else if (mouseSource == AINPUT_SOURCE_MOUSE) {
                    handledEvent = gcm->processGameActivityMouseEvent(
                            event, eventSize, eventDeviceId);
                }
            }
        }
    }
    return handledEvent;
}

int32_t GameControllerManager::processControllerKeyEvent(
    const AInputEvent *event, GameController &gameController) {
    int32_t handledEvent = HANDLED_EVENT;
    const int32_t eventKeycode = AKeyEvent_getKeyCode(event);

    mLastKeyEventKeyCode = eventKeycode;
    if (eventKeycode == AKEYCODE_BACK) {
        if (!mBackButtonConsumed) {
            handledEvent = IGNORED_EVENT;
        }
    } else {
        handledEvent = gameController.processKeyEvent(event);
    }
    return handledEvent;
}

int32_t GameControllerManager::processControllerGameActivityKeyEvent(
    const Paddleboat_GameActivityKeyEvent *event, const size_t eventSize,
    GameController &gameController) {
    int32_t handledEvent = HANDLED_EVENT;
    const int32_t eventKeyCode = event->keyCode;

    mLastKeyEventKeyCode = eventKeyCode;
    if (eventKeyCode == AKEYCODE_BACK) {
        if (!mBackButtonConsumed) {
            handledEvent = IGNORED_EVENT;
        }
    } else {
        handledEvent =
            gameController.processGameActivityKeyEvent(event, eventSize);
    }
    return handledEvent;
}

int32_t GameControllerManager::processMouseEvent(const AInputEvent *event) {
    int32_t handledEvent = IGNORED_EVENT;
    const int32_t eventDeviceId = AInputEvent_getDeviceId(event);
    const int32_t eventType = AInputEvent_getType(event);

    // Always update the virtual pointer data in the appropriate controller data
    // structures
    if (eventType == AINPUT_EVENT_TYPE_MOTION) {
        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (mGameControllers[i].getConnectionIndex() >= 0) {
                if (mGameControllers[i].getControllerStatus() ==
                    PADDLEBOAT_CONTROLLER_ACTIVE) {
                    const Paddleboat_Controller_Info &controllerInfo =
                        mGameControllers[i].getControllerInfo();
                    if (controllerInfo.deviceId == eventDeviceId) {
                        Paddleboat_Controller_Data &controllerData =
                            mGameControllers[i].getControllerData();
                        controllerData.virtualPointer.pointerX =
                            AMotionEvent_getAxisValue(event,
                                                      AMOTION_EVENT_AXIS_X, 0);
                        controllerData.virtualPointer.pointerY =
                            AMotionEvent_getAxisValue(event,
                                                      AMOTION_EVENT_AXIS_Y, 0);
                        const float axisP = AMotionEvent_getAxisValue(
                            event, AMOTION_EVENT_AXIS_PRESSURE, 0);

                        const bool hasTouchpadButton =
                            ((controllerInfo.controllerFlags &
                              PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD) != 0);
                        if (hasTouchpadButton) {
                            if (axisP > 0.0f) {
                                controllerData.buttonsDown |=
                                    PADDLEBOAT_BUTTON_TOUCHPAD;
                            } else {
                                controllerData.buttonsDown &=
                                    (~PADDLEBOAT_BUTTON_TOUCHPAD);
                            }
                        }
                        mGameControllers[i].setControllerDataDirty(true);

                        // If this controller is our 'active' virtual mouse,
                        // update the mouse data
                        if (mMouseStatus ==
                                PADDLEBOAT_MOUSE_CONTROLLER_EMULATED &&
                            mMouseControllerIndex == static_cast<int32_t>(i)) {
                            mMouseData.mouseX =
                                controllerData.virtualPointer.pointerX;
                            mMouseData.mouseY =
                                controllerData.virtualPointer.pointerY;
                            mMouseData.buttonsDown = static_cast<uint32_t>(
                                AMotionEvent_getButtonState(event));
                            mMouseData.buttonsDown |= axisP > 0.0f ? 1 : 0;
                            updateMouseDataTimestamp();
                        }
                        break;
                    }
                }
            }
        }
    }

    if (mMouseStatus == PADDLEBOAT_MOUSE_PHYSICAL) {
        for (size_t i = 0; i < MAX_MOUSE_DEVICES; ++i) {
            if (mMouseDeviceIds[i] == eventDeviceId) {
                if (eventType == AINPUT_EVENT_TYPE_MOTION) {
                    mMouseData.mouseX = AMotionEvent_getAxisValue(
                        event, AMOTION_EVENT_AXIS_X, 0);
                    mMouseData.mouseY = AMotionEvent_getAxisValue(
                        event, AMOTION_EVENT_AXIS_Y, 0);
                    const int32_t buttonState =
                        AMotionEvent_getButtonState(event);
                    mMouseData.buttonsDown = static_cast<uint32_t>(buttonState);
                    const float axisHScroll = AMotionEvent_getAxisValue(
                        event, AMOTION_EVENT_AXIS_HSCROLL, 0);
                    const float axisVScroll = AMotionEvent_getAxisValue(
                        event, AMOTION_EVENT_AXIS_VSCROLL, 0);
                    // These are treated as cumulative deltas and reset when the
                    // calling application requests the mouse data.
                    mMouseData.mouseScrollDeltaH +=
                        static_cast<int32_t>(axisHScroll);
                    mMouseData.mouseScrollDeltaV +=
                        static_cast<int32_t>(axisVScroll);
                    updateMouseDataTimestamp();
                }
                handledEvent = HANDLED_EVENT;
                break;
            }
        }
    }

    return handledEvent;
}

const float *GameControllerManager::getAxisValuesFromGameActivityMotionEvent(
        const void *event, const size_t eventSize, const uint32_t pointerIndex) {
    if (pointerIndex < PADDLEBOAT_MAX_NUM_POINTERS_IN_MOTION_EVENT) {
        if (eventSize == sizeof(Paddleboat_GameActivityMotionEvent)) {
            const Paddleboat_GameActivityMotionEvent *motionEvent =
                    reinterpret_cast<const Paddleboat_GameActivityMotionEvent *>(event);
            if (pointerIndex < motionEvent->pointerCount) {
                return motionEvent->pointers[pointerIndex].axisValues;
            }
        }
        // assume V2
        const Paddleboat_GameActivityMotionEventV2 *motionEventV2 =
                reinterpret_cast<const Paddleboat_GameActivityMotionEventV2 *>(event);
        if (pointerIndex < motionEventV2->pointerCount) {
            return motionEventV2->pointers[pointerIndex].axisValues;
        }
    }
    return nullptr;
}

int32_t GameControllerManager::processGameActivityMouseEvent(
    const void *event, const size_t eventSize, const int32_t eventDeviceId) {
    int32_t handledEvent = IGNORED_EVENT;

    // Button state same for all versions of struct, currently
    const int32_t buttonState =
            (reinterpret_cast<const Paddleboat_GameActivityMotionEvent *>(event))->buttonState;
    // location of axis values dependent on struct version
    const float *axisValues = getAxisValuesFromGameActivityMotionEvent(event, eventSize, 0);
    if (axisValues != nullptr) {
        // Always update the virtual pointer data in the appropriate controller data
        // structures
        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (mGameControllers[i].getConnectionIndex() >= 0) {
                if (mGameControllers[i].getControllerStatus() ==
                    PADDLEBOAT_CONTROLLER_ACTIVE) {
                    const Paddleboat_Controller_Info &controllerInfo =
                        mGameControllers[i].getControllerInfo();
                    if (controllerInfo.deviceId == eventDeviceId) {
                        Paddleboat_Controller_Data &controllerData =
                            mGameControllers[i].getControllerData();
                        controllerData.virtualPointer.pointerX =
                            axisValues[AMOTION_EVENT_AXIS_X];
                        controllerData.virtualPointer.pointerY =
                            axisValues[AMOTION_EVENT_AXIS_Y];
                        const float axisP = axisValues[AMOTION_EVENT_AXIS_PRESSURE];

                        const bool hasTouchpadButton =
                            ((controllerInfo.controllerFlags &
                              PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD) != 0);
                        if (hasTouchpadButton) {
                            if (axisP > 0.0f) {
                                controllerData.buttonsDown |=
                                    PADDLEBOAT_BUTTON_TOUCHPAD;
                            } else {
                                controllerData.buttonsDown &=
                                    (~PADDLEBOAT_BUTTON_TOUCHPAD);
                            }
                        }
                        mGameControllers[i].setControllerDataDirty(true);

                        // If this controller is our 'active' virtual mouse,
                        // update the mouse data
                        if (mMouseStatus ==
                                PADDLEBOAT_MOUSE_CONTROLLER_EMULATED &&
                            mMouseControllerIndex == static_cast<int32_t>(i)) {
                            mMouseData.mouseX =
                                controllerData.virtualPointer.pointerX;
                            mMouseData.mouseY =
                                controllerData.virtualPointer.pointerY;
                            mMouseData.buttonsDown = buttonState;
                            mMouseData.buttonsDown |= axisP > 0.0f ? 1 : 0;
                            updateMouseDataTimestamp();
                        }
                        handledEvent = HANDLED_EVENT;
                        break;
                    }
                }
            }
        }

        if (mMouseStatus == PADDLEBOAT_MOUSE_PHYSICAL) {
            for (size_t i = 0; i < MAX_MOUSE_DEVICES; ++i) {
                if (mMouseDeviceIds[i] == eventDeviceId) {
                    mMouseData.mouseX = axisValues[AMOTION_EVENT_AXIS_X];
                    mMouseData.mouseY = axisValues[AMOTION_EVENT_AXIS_Y];
                    mMouseData.buttonsDown = static_cast<uint32_t>(buttonState);
                    const float axisHScroll = axisValues[AMOTION_EVENT_AXIS_HSCROLL];
                    const float axisVScroll = axisValues[AMOTION_EVENT_AXIS_VSCROLL];
                    // These are treated as cumulative deltas and reset when the
                    // calling application requests the mouse data.
                    mMouseData.mouseScrollDeltaH += static_cast<int32_t>(axisHScroll);
                    mMouseData.mouseScrollDeltaV += static_cast<int32_t>(axisVScroll);
                    updateMouseDataTimestamp();
                }
                handledEvent = HANDLED_EVENT;
                break;
            }
        }
    }

    return handledEvent;
}

uint64_t GameControllerManager::getActiveAxisMask() {
    uint64_t returnMask = 0;
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        returnMask = gcm->mActiveAxisMask;
    }
    return returnMask;
}

void GameControllerManager::update(JNIEnv *env) {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        return;
    }
    if (!gcm->mGCMClassInitialized && gcm->mGameControllerObject != NULL) {
        gcm->mApiLevel = env->CallIntMethod(gcm->mGameControllerObject,
                                            gcm->mGetApiLevelMethodId);
        gcm->mIntegratedSensorFlags = static_cast<uint32_t>(
                env->CallIntMethod(gcm->mGameControllerObject,
                gcm->mGetIntegratedSensorMethodId));

        // Tell the GCM class we are ready to receive information about
        // controllers
        gcm->mGCMClassInitialized = true;
        // Early return because the first time through ::update is from Paddleboat_init
        // and the user hasn't had a chance to register callbacks yet
        return;
    }
    if (gcm->mSetNativeReadyMethodId != NULL) {
        env->CallVoidMethod(gcm->mGameControllerObject,
                            gcm->mSetNativeReadyMethodId);
        gcm->mSetNativeReadyMethodId = NULL;
    }

    if (gcm->mMotionDataCallback != nullptr &&
        gcm->mMotionEventReporting == false) {
        // If a motion data callback is registered, tell the managed side to
        // start reporting motion event data
        env->CallVoidMethod(gcm->mGameControllerObject,
                            gcm->mSetReportMotionEventsMethodId);
        gcm->mMotionEventReporting = true;
    }

    if (gcm->mActiveSensorFlagsDirty) {
        // Pass the integrated flags to the managed side, so it can determine whether
        // to enable or disable listening to an integrated sensor
        env->CallVoidMethod(
                gcm->mGameControllerObject, gcm->mSetActiveIntegratedSensorsMethodId,
                static_cast<jint>(gcm->mMotionDataCallbackFlags));
        gcm->mActiveSensorFlagsDirty = false;
    }


    std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);

    // Process pending connections/disconnections
    if (gcm->mStatusCallback != nullptr) {
        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                if (gcm->mGameControllers[i].getControllerStatus() ==
                    PADDLEBOAT_CONTROLLER_JUST_CONNECTED) {
                    // Look for a mapping table entry for this controller
                    // device, if one does not exist, nullptr is passed and
                    // GameController will fallback to default axis and button
                    // mapping.
                    const Paddleboat_Controller_Mapping_File_Controller_Entry *mapData =
                        gcm->getMapForController(gcm->mGameControllers[i]);
#if defined LOG_INPUT_EVENTS
                    if (mapData != nullptr) {
                        ALOGI("Found controller map for vId/pId: %x %x",
                              gcm->mGameControllers[i]
                                  .getDeviceInfo()
                                  .getInfo()
                                  ->mVendorId,
                              gcm->mGameControllers[i]
                                  .getDeviceInfo()
                                  .getInfo()
                                  ->mProductId);
                    } else {
                        ALOGI(
                            "No controller map found, using defaults for "
                            "vId/pId: %x %x",
                            gcm->mGameControllers[i]
                                .getDeviceInfo()
                                .getInfo()
                                ->mVendorId,
                            gcm->mGameControllers[i]
                                .getDeviceInfo()
                                .getInfo()
                                ->mProductId);
                    }
#endif
                    const uint32_t axisIndex = mapData != nullptr ? mapData->axisTableIndex : 0;
                    const uint32_t buttonIndex = mapData != nullptr ? mapData->buttonTableIndex : 0;
                    gcm->mGameControllers[i].setupController(mapData,
                        &gcm->mMappingInfo.mAxisTable[axisIndex],
                        &gcm->mMappingInfo.mButtonTable[buttonIndex]);
                    // Update the active axis mask to include any new axis used
                    // by the new controller
                    gcm->mActiveAxisMask |=
                        gcm->mGameControllers[i].getControllerAxisMask();
                    gcm->mGameControllers[i].setControllerStatus(
                        PADDLEBOAT_CONTROLLER_ACTIVE);
                    if (gcm->mStatusCallback != nullptr) {
#if defined LOG_INPUT_EVENTS
                        ALOGI(
                            "statusCallback "
                            "PADDLEBOAT_CONTROLLER_JUST_CONNECTED on %d",
                            static_cast<int>(i));
#endif
                        gcm->mStatusCallback(
                            i, PADDLEBOAT_CONTROLLER_JUST_CONNECTED,
                            gcm->mStatusCallbackUserData);
                    }
                    gcm->rescanVirtualMouseControllers();
                } else if (gcm->mGameControllers[i].getControllerStatus() ==
                           PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED) {
                    gcm->mGameControllers[i].setControllerStatus(
                        PADDLEBOAT_CONTROLLER_INACTIVE);
                    // free the controller for reuse
                    gcm->mGameControllers[i].setConnectionIndex(-1);
                    if (gcm->mStatusCallback != nullptr) {
#if defined LOG_INPUT_EVENTS
                        ALOGI(
                            "statusCallback "
                            "PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED on %d",
                            static_cast<int>(i));
#endif
                        gcm->mStatusCallback(
                            i, PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED,
                            gcm->mStatusCallbackUserData);
                    }
                    gcm->rescanVirtualMouseControllers();
                }
            }
        }
    }
    gcm->updateBattery(env);
}

Paddleboat_ErrorCode GameControllerManager::getControllerData(
    const int32_t controllerIndex, Paddleboat_Controller_Data *controllerData) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;
    if (controllerData != nullptr) {
        if (controllerIndex >= 0 &&
            controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
            GameControllerManager *gcm = getInstance();
            if (gcm) {
                if (gcm->mGameControllers[controllerIndex]
                        .getConnectionIndex() == controllerIndex) {
                    if (gcm->mGameControllers[controllerIndex]
                            .getControllerDataDirty()) {
                        gcm->mGameControllers[controllerIndex]
                            .setControllerDataDirty(false);
                    }
                    memcpy(controllerData,
                           &gcm->mGameControllers[controllerIndex]
                                .getControllerData(),
                           sizeof(Paddleboat_Controller_Data));
                } else {
                    errorCode = PADDLEBOAT_ERROR_NO_CONTROLLER;
                }
            } else {
                errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_INVALID_CONTROLLER_INDEX;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_PARAMETER;
    }
    return errorCode;
}

Paddleboat_ErrorCode GameControllerManager::getControllerInfo(
    const int32_t controllerIndex, Paddleboat_Controller_Info *controllerInfo) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;
    if (controllerInfo != nullptr) {
        if (controllerIndex >= 0 &&
            controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
            GameControllerManager *gcm = getInstance();
            if (gcm) {
                if (gcm->mGameControllers[controllerIndex]
                        .getConnectionIndex() == controllerIndex) {
                    memcpy(controllerInfo,
                           &gcm->mGameControllers[controllerIndex]
                                .getControllerInfo(),
                           sizeof(Paddleboat_Controller_Info));
                } else {
                    errorCode = PADDLEBOAT_ERROR_NO_CONTROLLER;
                }
            } else {
                errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_INVALID_CONTROLLER_INDEX;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_PARAMETER;
    }
    return errorCode;
}

Paddleboat_ErrorCode GameControllerManager::getControllerName(
    const int32_t controllerIndex, const size_t bufferSize,
    char *controllerName) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;
    if (controllerName != nullptr) {
        if (controllerIndex >= 0 &&
            controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
            GameControllerManager *gcm = getInstance();
            if (gcm) {
                if (gcm->mGameControllers[controllerIndex]
                        .getConnectionIndex() == controllerIndex) {
                    const GameControllerDeviceInfo &deviceInfo =
                        gcm->mGameControllers[controllerIndex].getDeviceInfo();
                    strncpy(controllerName, deviceInfo.getName(), bufferSize);
                    // Manually zero-terminate if the string was too long to fit
                    const size_t nameLength = strlen(deviceInfo.getName());
                    if (nameLength >= bufferSize) {
                        controllerName[bufferSize - 1] = '\0';
                    }
                } else {
                    errorCode = PADDLEBOAT_ERROR_NO_CONTROLLER;
                }
            } else {
                errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_INVALID_CONTROLLER_INDEX;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_PARAMETER;
    }
    return errorCode;
}

Paddleboat_ControllerStatus GameControllerManager::getControllerStatus(
    const int32_t controllerIndex) {
    Paddleboat_ControllerStatus controllerStatus =
        PADDLEBOAT_CONTROLLER_INACTIVE;
    if (controllerIndex >= 0 && controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            controllerStatus =
                gcm->mGameControllers[controllerIndex].getControllerStatus();
        }
    }
    return controllerStatus;
}

bool GameControllerManager::isLightTypeSupported(
    const Paddleboat_Controller_Info &controllerInfo,
    const Paddleboat_LightType lightType) {
    bool isSupported = false;

    if (mGameControllerObject != NULL && mSetLightMethodId != NULL) {
        if (lightType == PADDLEBOAT_LIGHT_RGB) {
            if ((controllerInfo.controllerFlags &
                 PADDLEBOAT_CONTROLLER_FLAG_LIGHT_RGB) != 0) {
                isSupported = true;
            }
        } else if (lightType == PADDLEBOAT_LIGHT_PLAYER_NUMBER) {
            if ((controllerInfo.controllerFlags &
                 PADDLEBOAT_CONTROLLER_FLAG_LIGHT_PLAYER) != 0) {
                isSupported = true;
            }
        }
    }
    return isSupported;
}

Paddleboat_ErrorCode GameControllerManager::setControllerLight(
    const int32_t controllerIndex, const Paddleboat_LightType lightType,
    const uint32_t lightData, JNIEnv *env) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;

    if (controllerIndex >= 0 && controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            if (gcm->mGameControllers[controllerIndex].getConnectionIndex() ==
                controllerIndex) {
                const Paddleboat_Controller_Info &controllerInfo =
                    gcm->mGameControllers[controllerIndex].getControllerInfo();
                if (gcm->isLightTypeSupported(controllerInfo, lightType)) {
                    const jint jLightType = static_cast<jint>(lightType);
                    const jint jLightData = static_cast<jint>(lightData);
                    env->CallVoidMethod(
                        gcm->mGameControllerObject, gcm->mSetLightMethodId,
                        controllerInfo.deviceId, jLightType, jLightData);
                } else {
                    errorCode = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
                }
            } else {
                errorCode = PADDLEBOAT_ERROR_NO_CONTROLLER;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_CONTROLLER_INDEX;
    }
    return errorCode;
}

Paddleboat_ErrorCode GameControllerManager::setControllerVibrationData(
    const int32_t controllerIndex,
    const Paddleboat_Vibration_Data *vibrationData, JNIEnv *env) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;

    if (vibrationData != nullptr) {
        if (controllerIndex >= 0 &&
            controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
            GameControllerManager *gcm = getInstance();
            if (gcm) {
                if (gcm->mGameControllers[controllerIndex]
                        .getConnectionIndex() == controllerIndex) {
                    const Paddleboat_Controller_Info &controllerInfo =
                        gcm->mGameControllers[controllerIndex]
                            .getControllerInfo();
                    if ((controllerInfo.controllerFlags &
                         PADDLEBOAT_CONTROLLER_FLAG_VIBRATION) != 0) {
                        if (gcm->mGameControllerObject != NULL &&
                            gcm->mSetVibrationMethodId != NULL) {
                            const jint intensityLeft =
                                static_cast<jint>(vibrationData->intensityLeft *
                                                  VIBRATION_INTENSITY_SCALE);
                            const jint intensityRight = static_cast<jint>(
                                vibrationData->intensityRight *
                                VIBRATION_INTENSITY_SCALE);
                            const jint durationLeft =
                                static_cast<jint>(vibrationData->durationLeft);
                            const jint durationRight =
                                static_cast<jint>(vibrationData->durationRight);
                            env->CallVoidMethod(gcm->mGameControllerObject,
                                                gcm->mSetVibrationMethodId,
                                                controllerInfo.deviceId,
                                                intensityLeft, durationLeft,
                                                intensityRight, durationRight);
                        }
                    } else {
                        errorCode = PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
                    }
                } else {
                    errorCode = PADDLEBOAT_ERROR_NO_CONTROLLER;
                }
            } else {
                errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_INVALID_CONTROLLER_INDEX;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_PARAMETER;
    }
    return errorCode;
}

GameControllerDeviceInfo *GameControllerManager::onConnection() {
    GameControllerDeviceInfo *deviceInfo = nullptr;
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);
        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (gcm->mGameControllers[i].getConnectionIndex() < 0) {
                gcm->mGameControllers[i].setConnectionIndex(i);
                gcm->mGameControllers[i].resetControllerData();
                deviceInfo = &gcm->mGameControllers[i].getDeviceInfo();
                gcm->mGameControllers[i].setControllerStatus(
                    PADDLEBOAT_CONTROLLER_JUST_CONNECTED);
#if defined LOG_INPUT_EVENTS
                ALOGI(
                    "Setting PADDLEBOAT_CONTROLLER_JUST_CONNECTED on index %d",
                    static_cast<int>(i));
#endif
                break;
            }
        }
    }
    return deviceInfo;
}

void GameControllerManager::onDisconnection(const int32_t deviceId) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        std::lock_guard<std::mutex> lock(gcm->mUpdateMutex);
        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                const GameControllerDeviceInfo &deviceInfo =
                    gcm->mGameControllers[i].getDeviceInfo();
                if (deviceInfo.getInfo().mDeviceId == deviceId) {
                    gcm->mGameControllers[i].setControllerStatus(
                        PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED);
#if defined LOG_INPUT_EVENTS
                    ALOGI(
                        "Setting PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED on "
                        "index %d",
                        static_cast<int>(i));
#endif
                }
            }
        }
    }
}

Paddleboat_ErrorCode GameControllerManager::getMouseData(
    Paddleboat_Mouse_Data *mouseData) {
    Paddleboat_ErrorCode errorCode = PADDLEBOAT_NO_ERROR;
    if (mouseData != nullptr) {
        GameControllerManager *gcm = getInstance();
        if (gcm) {
            if (gcm->mMouseStatus != PADDLEBOAT_MOUSE_NONE) {
                memcpy(mouseData, &gcm->mMouseData,
                       sizeof(Paddleboat_Mouse_Data));
                // We reset the scroll wheel(s) values after each read
                gcm->mMouseData.mouseScrollDeltaH = 0;
                gcm->mMouseData.mouseScrollDeltaV = 0;
            } else {
                errorCode = PADDLEBOAT_ERROR_NO_MOUSE;
            }
        } else {
            errorCode = PADDLEBOAT_ERROR_NOT_INITIALIZED;
        }
    } else {
        errorCode = PADDLEBOAT_ERROR_INVALID_PARAMETER;
    }
    return errorCode;
}

Paddleboat_MouseStatus GameControllerManager::getMouseStatus() {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        return gcm->mMouseStatus;
    }
    return PADDLEBOAT_MOUSE_NONE;
}

void GameControllerManager::onMotionData(const int32_t deviceId,
                                         const int32_t motionType,
                                         const uint64_t timestamp,
                                         const float dataX, const float dataY,
                                         const float dataZ) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        if (gcm->mMotionDataCallback != nullptr) {
            Paddleboat_Motion_Data motionData;
            motionData.motionType =
                    static_cast<Paddleboat_Motion_Type>(motionType);
            motionData.timestamp = timestamp;
            motionData.motionX = dataX;
            motionData.motionY = dataY;
            motionData.motionZ = dataZ;

            if (deviceId == PADDLEBOAT_INTEGRATED_SENSOR_INDEX) {
                gcm->mMotionDataCallback(PADDLEBOAT_INTEGRATED_SENSOR_INDEX, &motionData,
                                         gcm->mMotionDataCallbackUserData);
            } else {
                for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                    if (gcm->mGameControllers[i].getConnectionIndex() >= 0) {
                        const GameControllerDeviceInfo &deviceInfo =
                                gcm->mGameControllers[i].getDeviceInfo();
                        if (deviceInfo.getInfo().mDeviceId == deviceId) {
                            gcm->mMotionDataCallback(
                                    i, &motionData, gcm->mMotionDataCallbackUserData);
                            return;
                        }
                    }
                }
            }
        }
    }
}

bool GameControllerManager::getPhysicalKeyboardStatus() {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        return gcm->mPhysicalKeyboardConnected;
    }
    return false;
}

void GameControllerManager::onKeyboardConnection(const int32_t /*deviceId*/) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mPhysicalKeyboardConnected = true;
        if (gcm->mKeyboardCallback != nullptr) {
            gcm->mKeyboardCallback(true, gcm->mKeyboardCallbackUserData);
        }
    }
}

void GameControllerManager::onKeyboardDisconnection(const int32_t /*deviceId*/) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mPhysicalKeyboardConnected = false;
        if (gcm->mKeyboardCallback != nullptr) {
            gcm->mKeyboardCallback(false, gcm->mKeyboardCallbackUserData);
        }
    }
}

void GameControllerManager::onMouseConnection(const int32_t deviceId) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        for (size_t i = 0; i < MAX_MOUSE_DEVICES; ++i) {
            if (gcm->mMouseDeviceIds[i] == INVALID_MOUSE_ID) {
                gcm->mMouseDeviceIds[i] = deviceId;
                break;
            }
        }
        if (gcm->mMouseStatus != PADDLEBOAT_MOUSE_PHYSICAL) {
            gcm->mMouseStatus = PADDLEBOAT_MOUSE_PHYSICAL;
            if (gcm->mMouseCallback != nullptr) {
                gcm->mMouseCallback(gcm->mMouseStatus,
                                    gcm->mMouseCallbackUserData);
            }
        }
    }
}

void GameControllerManager::onMouseDisconnection(const int32_t deviceId) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        int mouseDeviceCount = 0;
        for (size_t i = 0; i < MAX_MOUSE_DEVICES; ++i) {
            if (gcm->mMouseDeviceIds[i] == deviceId) {
                gcm->mMouseDeviceIds[i] = INVALID_MOUSE_ID;
            } else if (gcm->mMouseDeviceIds[i] != INVALID_MOUSE_ID) {
                ++mouseDeviceCount;
            }
        }

        // If no other physical mice are connected, see if we downgrade to
        // a controller virtual mouse or no mouse at all
        if (mouseDeviceCount == 0) {
            gcm->mMouseStatus = (gcm->mMouseControllerIndex == INVALID_MOUSE_ID)
                                    ? PADDLEBOAT_MOUSE_NONE
                                    : PADDLEBOAT_MOUSE_CONTROLLER_EMULATED;
            if (gcm->mMouseCallback != nullptr) {
                gcm->mMouseCallback(gcm->mMouseStatus,
                                    gcm->mMouseCallbackUserData);
            }
        }
    }
}

// Make sure the 'virtual' mouse is the first active controller index
// which supports a virtual pointer. If no mouse is currently active,
// upgrade to a virtual mouse and send a mouse status callback.
// If only a virtual mouse was active, and it vanished, set no mouse and
// send a mouse status callback.
void GameControllerManager::rescanVirtualMouseControllers() {
    mMouseControllerIndex = INVALID_MOUSE_ID;
    for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
        if (mGameControllers[i].getControllerStatus() ==
            PADDLEBOAT_CONTROLLER_ACTIVE) {
            if ((mGameControllers[i].getControllerInfo().controllerFlags &
                 PADDLEBOAT_CONTROLLER_FLAG_VIRTUAL_MOUSE) != 0) {
                mMouseControllerIndex = i;
                if (mMouseStatus == PADDLEBOAT_MOUSE_NONE) {
                    mMouseStatus = PADDLEBOAT_MOUSE_CONTROLLER_EMULATED;
                    if (mMouseCallback != nullptr) {
                        mMouseCallback(mMouseStatus, mMouseCallbackUserData);
                    }
                }
                break;
            }
        }
    }

    // If no virtual mouse exists, downgrade to no mouse and send a mouse status
    // callback.
    if (mMouseControllerIndex == INVALID_MOUSE_ID &&
        mMouseStatus == PADDLEBOAT_MOUSE_CONTROLLER_EMULATED) {
        mMouseStatus = PADDLEBOAT_MOUSE_NONE;
        if (mMouseCallback != nullptr) {
            mMouseCallback(mMouseStatus, mMouseCallbackUserData);
        }
    }
}

Paddleboat_ErrorCode GameControllerManager::setMotionDataCallback(
    Paddleboat_MotionDataCallback motionDataCallback,
    Paddleboat_Integrated_Motion_Sensor_Flags integratedFlags, void *userData) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        if ((integratedFlags & gcm->mIntegratedSensorFlags) != integratedFlags) {
            // Return an error if requesting an unavailable integrated sensor
            return PADDLEBOAT_ERROR_FEATURE_NOT_SUPPORTED;
        }
        gcm->mMotionDataCallback = motionDataCallback;
        gcm->mMotionDataCallbackUserData = userData;
        gcm->mMotionDataCallbackFlags = integratedFlags;
        // Mark to call the managed side with the updated flags on the next update
        gcm->mActiveSensorFlagsDirty = true;
    }
    return PADDLEBOAT_NO_ERROR;
}

void GameControllerManager::setMouseStatusCallback(
    Paddleboat_MouseStatusCallback statusCallback, void *userData) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mMouseCallback = statusCallback;
        gcm->mMouseCallbackUserData = userData;
    }
}

void GameControllerManager::setPhysicalKeyboardStatusCallback(
        Paddleboat_PhysicalKeyboardStatusCallback statusCallback, void *userData) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mKeyboardCallback = statusCallback;
        gcm->mKeyboardCallbackUserData = userData;
    }
}

Paddleboat_Integrated_Motion_Sensor_Flags GameControllerManager::getIntegratedMotionSensorFlags() {
    Paddleboat_Integrated_Motion_Sensor_Flags flags = PADDLEBOAT_INTEGRATED_SENSOR_NONE;
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        flags = static_cast<Paddleboat_Integrated_Motion_Sensor_Flags>(gcm->mIntegratedSensorFlags);
    }
    return flags;
}

jclass GameControllerManager::getGameControllerClass() {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        return NULL;
    }
    return gcm->mGameControllerClass;
}

jobject GameControllerManager::getGameControllerObject() {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        return NULL;
    }
    return gcm->mGameControllerObject;
}

bool GameControllerManager::getBackButtonConsumed() {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        return gcm->mBackButtonConsumed;
    }
    return false;
}

void GameControllerManager::setBackButtonConsumed(bool consumed) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mBackButtonConsumed = consumed;
    }
}

void GameControllerManager::setControllerStatusCallback(
    Paddleboat_ControllerStatusCallback statusCallback, void *userData) {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        gcm->mStatusCallback = statusCallback;
        gcm->mStatusCallbackUserData = userData;
    }
}

// device debug helper function
int32_t GameControllerManager::getLastKeycode() {
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        return gcm->mLastKeyEventKeyCode;
    }
    return 0;
}

void GameControllerManager::onStop(JNIEnv *env) {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        return;
    }
    if (gcm->mGameControllerObject != NULL) {
        jmethodID onPauseID =
            env->GetMethodID(gcm->mGameControllerClass, GCM_ONSTOP_METHOD_NAME,
                             VOID_METHOD_SIGNATURE);
        if (onPauseID != NULL) {
            env->CallVoidMethod(gcm->mGameControllerObject, onPauseID);
        }
    }
}

void GameControllerManager::onStart(JNIEnv *env) {
    GameControllerManager *gcm = getInstance();
    if (!gcm) {
        return;
    }
    if (gcm->mGameControllerObject != NULL) {
        jmethodID onResumeID =
            env->GetMethodID(gcm->mGameControllerClass, GCM_ONSTART_METHOD_NAME,
                             VOID_METHOD_SIGNATURE);
        if (onResumeID != NULL) {
            env->CallVoidMethod(gcm->mGameControllerObject, onResumeID);
        }
    }
}

void GameControllerManager::updateBattery(JNIEnv *env) {
    if (mBatteryWait <= 0) {
        mBatteryWait = BATTERY_REFRESH_WAIT;

        for (size_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
            if (mGameControllers[i].getControllerStatus() ==
                PADDLEBOAT_CONTROLLER_ACTIVE) {
                const Paddleboat_Controller_Info &controllerInfo =
                    mGameControllers[i].getControllerInfo();
                if ((controllerInfo.controllerFlags &
                     PADDLEBOAT_CONTROLLER_FLAG_BATTERY) != 0) {
                    Paddleboat_Controller_Data &controllerData =
                        mGameControllers[i].getControllerData();
                    const jint deviceId = controllerInfo.deviceId;
                    jfloat batteryLevel = env->CallFloatMethod(
                        mGameControllerObject, mGetBatteryLevelMethodId,
                        deviceId);
                    jint batteryStatus =
                        env->CallIntMethod(mGameControllerObject,
                                           mGetBatteryStatusMethodId, deviceId);
                    controllerData.battery.batteryLevel = batteryLevel;
                    // Java 'enum' starts at 1, not 0.
                    controllerData.battery.batteryStatus =
                        static_cast<Paddleboat_BatteryStatus>(batteryStatus -
                                                              1);
                }
            }
        }
    } else {
        --mBatteryWait;
    }
}

void GameControllerManager::updateMouseDataTimestamp() {
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    mMouseData.timestamp = static_cast<uint64_t>(timestamp);
}

Paddleboat_ErrorCode GameControllerManager::addControllerRemapDataFromFd(
   const Paddleboat_Remap_Addition_Mode addMode, const int fileDescriptor) {
    Paddleboat_ErrorCode result = PADDLEBOAT_ERROR_NOT_INITIALIZED;
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        struct stat fileStat;
        // assume file i/o failed unless we reach addControllerRemapDataFromFileBuffer, which
        // returns its own error code result
        result = PADDLEBOAT_ERROR_FILE_IO;
        if (fstat(fileDescriptor, &fileStat) == 0) {
            const size_t fileSize = static_cast<size_t>(fileStat.st_size);
            void *fileBuffer = malloc(fileStat.st_size);
            if (fileBuffer != nullptr) {
                if (read(fileDescriptor, fileBuffer, fileSize) == static_cast<ssize_t>(fileSize)) {
                    result = GameControllerManager::addControllerRemapDataFromFileBuffer(addMode,
                        reinterpret_cast<const Paddleboat_Controller_Mapping_File_Header *>
                        (fileBuffer), fileSize);
                }
                free(fileBuffer);
            }
        }
    }
    return result;
}

Paddleboat_ErrorCode GameControllerManager::addControllerRemapDataFromFileBuffer(
    const Paddleboat_Remap_Addition_Mode addMode,
    const Paddleboat_Controller_Mapping_File_Header *mappingFileHeader,
    const size_t mappingFileBufferSize) {
    Paddleboat_ErrorCode result = PADDLEBOAT_ERROR_NOT_INITIALIZED;
    GameControllerManager *gcm = getInstance();
    if (gcm) {
        result = GameControllerMappingUtils::validateMapFile(mappingFileHeader,
                                                             mappingFileBufferSize);
        if (result == PADDLEBOAT_NO_ERROR) {
            switch (addMode) {
                case PADDLEBOAT_REMAP_ADD_MODE_DEFAULT:
                    result = GameControllerMappingUtils::mergeControllerRemapData(
                            mappingFileHeader, mappingFileBufferSize, gcm->mMappingInfo);
                    break;
                case PADDLEBOAT_REMAP_ADD_MODE_REPLACE_ALL:
                    result = GameControllerMappingUtils::overwriteControllerRemapData(
                            mappingFileHeader, mappingFileBufferSize, gcm->mMappingInfo);
                    break;
            }
        }
    }
    return result;
}

void GameControllerManager::addControllerRemapData(
    const Paddleboat_Remap_Addition_Mode /*addMode*/,
    const int32_t /*remapTableEntryCount*/,
    const Paddleboat_Controller_Mapping_Data */*mappingData*/) {
    /*
     * Paddleboat 1.2 deprecated this function in favor of addControllerRemapDataFromFileBuffer
     * and addControllerRemapDataFromFd. The new functions use a more efficient system with
     * additional features.
     */
}

int32_t GameControllerManager::getControllerRemapTableData(
    const int32_t /*destRemapTableEntryCount*/,
    Paddleboat_Controller_Mapping_Data */*mappingData*/) {
    return 0;
}

const Paddleboat_Controller_Mapping_File_Controller_Entry *
GameControllerManager::getMapForController(const GameController &gameController) {
    const Paddleboat_Controller_Mapping_File_Controller_Entry *returnEntry = nullptr;
    const GameControllerDeviceInfo &deviceInfo = gameController.getDeviceInfo();
    MappingTableSearch mapSearch(&mMappingInfo.mControllerTable[0],
                                 mMappingInfo.mControllerTableEntryCount);
    mapSearch.initSearchParameters(deviceInfo.getInfo().mVendorId,
                                   deviceInfo.getInfo().mProductId, mApiLevel,
                                   mApiLevel);
    bool success = GameControllerMappingUtils::findMatchingMapEntry(&mapSearch);
    if (success) {
        returnEntry = &mapSearch.mappingRoot[mapSearch.tableIndex];
    }
    return returnEntry;
}

}  // namespace paddleboat
