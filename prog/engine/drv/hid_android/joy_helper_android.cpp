// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_helper_android.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <limits>
#include <string.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>

#include <atomic>

void *HumanInput::JoyDeviceHelper::s_AttachedActivity = NULL;
jobject HumanInput::JoyDeviceHelper::s_ActivityClazz = NULL;
bool HumanInput::JoyDeviceHelper::s_Inited = false;
JNIEnv *HumanInput::JoyDeviceHelper::s_Jni = NULL;
jclass HumanInput::JoyDeviceHelper::s_JavaHelperClass = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_ListenGamepads = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_StopListeningGamepads = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_IsDeviceChanged = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_IsGamepadConnected = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_GetConnectedGamepadVendorId = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_GetConnectedGamepadProductId = NULL;
jmethodID HumanInput::JoyDeviceHelper::s_GetDisplayRotation = NULL;

static std::atomic<bool> is_input_device_changed = false;


bool HumanInput::JoyDeviceHelper::attach()
{
  android_app *app = (android_app *)win32_get_instance();
  if (!app)
    return false;

  void *activity = app->activity;

  if (!activity && !s_AttachedActivity && !s_Inited)
  {
    logwarn("AndroidJoyHelper: JNI is not inited and activities are empty");
    return false;
  }

  JavaVMAttachArgs attachArgs;
  attachArgs.version = JNI_VERSION_1_6;
  attachArgs.name = "NativeThread";
  attachArgs.group = NULL;

  // always attach to the caller's thread
  jint result = android::attach_current_thread(android::get_java_vm(activity ? activity : s_AttachedActivity), &s_Jni, &attachArgs);
  if (result != JNI_OK || s_Jni == NULL)
    return false;

  // check if already inited
  if (s_AttachedActivity == activity && s_Inited)
    return true;

  if (!activity && s_Inited)
    return true;

  s_AttachedActivity = activity;

#define JNI_RETURN(obj, ret) obj == NULL ? NULL : ret

  s_ActivityClazz = android::get_activity_class(activity);
  jstring strClassName = s_Jni->NewStringUTF("com/gaijinent/helpers/GamepadHelper");
  jclass activityClass = s_Jni->GetObjectClass(android::get_activity_class(activity));

  jmethodID getClassLoaderID =
    JNI_RETURN(activityClass, s_Jni->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;"));
  jobject classLoader = JNI_RETURN(getClassLoaderID, s_Jni->CallObjectMethod(android::get_activity_class(activity), getClassLoaderID));
  jmethodID findClass = JNI_RETURN(classLoader,
    s_Jni->GetMethodID(s_Jni->GetObjectClass(classLoader), "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"));
  jobject cls = findClass ? s_Jni->CallObjectMethod(classLoader, findClass, strClassName) : nullptr;
  s_Jni->ExceptionClear();
  s_JavaHelperClass = cls ? (jclass)s_Jni->NewGlobalRef(cls) : nullptr;

  s_ListenGamepads =
    JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "listenGamepads", "(Landroid/app/Activity;)V"));
  s_Jni->ExceptionClear();

  s_StopListeningGamepads = JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "stopListeningGamepads", "()V"));
  s_Jni->ExceptionClear();

  s_IsDeviceChanged = JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "isDeviceChanged", "()Z"));
  s_Jni->ExceptionClear();

  s_IsGamepadConnected = JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "isGamepadConnected", "()Z"));
  s_Jni->ExceptionClear();

  s_GetConnectedGamepadVendorId =
    JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "getConnectedGamepadVendorId", "()I"));
  s_Jni->ExceptionClear();

  s_GetConnectedGamepadProductId =
    JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "getConnectedGamepadProductId", "()I"));
  s_Jni->ExceptionClear();

  s_GetDisplayRotation =
    JNI_RETURN(s_JavaHelperClass, s_Jni->GetStaticMethodID(s_JavaHelperClass, "getDisplayRotation", "(Landroid/app/Activity;)I"));
  s_Jni->ExceptionClear();

  s_Jni->DeleteLocalRef(strClassName);

#undef JNI_RETURN

  s_Inited = s_JavaHelperClass && s_ListenGamepads && s_StopListeningGamepads && s_IsDeviceChanged && s_IsGamepadConnected &&
             s_GetConnectedGamepadVendorId && s_GetConnectedGamepadProductId && s_GetDisplayRotation;

  if (s_Inited)
  {
    s_Jni->CallStaticVoidMethod(s_JavaHelperClass, s_ListenGamepads, android::get_activity_class(activity));
    s_Jni->ExceptionClear();
  }
  else
    logwarn("AndroidJoyHelper: JNI init failed");

  return s_Inited;
}


void HumanInput::JoyDeviceHelper::detach()
{
  if (s_AttachedActivity)
  {
    stopListeningGamepads();
    android::get_java_vm(s_AttachedActivity)->DetachCurrentThread();
    s_AttachedActivity = NULL;
  }
  s_Inited = false;
}


void HumanInput::JoyDeviceHelper::stopListeningGamepads()
{
  if (attach())
    return s_Jni->CallStaticVoidMethod(s_JavaHelperClass, s_StopListeningGamepads);
}


bool HumanInput::JoyDeviceHelper::isDeviceChanged()
{
  if (attach())
    return s_Jni->CallStaticBooleanMethod(s_JavaHelperClass, s_IsDeviceChanged);

  return false;
}


bool HumanInput::JoyDeviceHelper::isGamepadConnected()
{
  if (attach())
    return s_Jni->CallStaticBooleanMethod(s_JavaHelperClass, s_IsGamepadConnected);

  return false;
}


int HumanInput::JoyDeviceHelper::getConnectedGamepadVendorId()
{
  if (attach())
    return s_Jni->CallStaticIntMethod(s_JavaHelperClass, s_GetConnectedGamepadVendorId);

  return GAMEPAD_VENDOR_UNKNOWN;
}

int HumanInput::JoyDeviceHelper::getConnectedGamepadProductId()
{
  if (attach())
    return s_Jni->CallStaticIntMethod(s_JavaHelperClass, s_GetConnectedGamepadProductId);

  return GAMEPAD_VENDOR_UNKNOWN;
}


int HumanInput::JoyDeviceHelper::getRotation()
{
  static int currentRotation = ROTATION_90;
  static int nextTimeToCheckRotation = 0;

  if (DAGOR_UNLIKELY(get_time_msec() >= nextTimeToCheckRotation))
  {
    if (attach())
    {
      currentRotation = s_Jni->CallStaticIntMethod(s_JavaHelperClass, s_GetDisplayRotation, s_ActivityClazz);
      nextTimeToCheckRotation = get_time_msec() + 1000;
    }
    else
    {
      currentRotation = ROTATION_90;
      nextTimeToCheckRotation = std::numeric_limits<int>::max();
    }
  }

  return currentRotation;
}


bool HumanInput::JoyDeviceHelper::acquireDeviceChanges()
{
  static bool isInputDeviceInited = false;
  if (!isInputDeviceInited)
  {
    isInputDeviceInited = true;
    is_input_device_changed = isDeviceChanged();
  }

  const bool res = is_input_device_changed;
  is_input_device_changed = false;
  return res;
}


#ifdef __cplusplus
extern "C"
{
#endif

  JNIEXPORT void JNICALL Java_com_gaijinent_helpers_GamepadListener_nativeOnChangeCurrentInputDevice(JNIEnv *jni, jobject cls)
  {
    is_input_device_changed = true;
  }

#ifdef __cplusplus
}
#endif