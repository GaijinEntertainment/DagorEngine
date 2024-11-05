// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <supp/dag_android_native_app_glue.h>

#include <android/native_activity.h>
#include <jni.h>

#define DEFAULT_DEVICE_ID -1
namespace HumanInput
{
class JoyDeviceHelper
{
private:
  static void *s_AttachedActivity;
  static jobject s_ActivityClazz;
  static bool s_Inited;
  static JNIEnv *s_Jni;

  static jclass s_JavaHelperClass;
  static jmethodID s_ListenGamepads;
  static jmethodID s_StopListeningGamepads;
  static jmethodID s_IsDeviceChanged;
  static jmethodID s_IsGamepadConnected;
  static jmethodID s_GetConnectedGamepadVendorId;
  static jmethodID s_GetConnectedGamepadProductId;
  static jmethodID s_GetDisplayRotation;

  static void stopListeningGamepads();

public:
  enum
  {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
  };

  static bool attach();
  static void detach();
  static bool isDeviceChanged();
  static bool isGamepadConnected();
  static int getConnectedGamepadVendorId();
  static int getConnectedGamepadProductId();
  static int getRotation();
  static bool acquireDeviceChanges();
};
} // namespace HumanInput