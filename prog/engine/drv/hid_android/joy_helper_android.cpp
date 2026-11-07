// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_helper_android.h"
#include "jni_utils_android.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_mutex.h>

#include <atomic>

namespace
{
struct GamepadHelperJni
{
  void *activity = nullptr;
  JavaVM *vm = nullptr;
  jclass helperClass = nullptr;
  jmethodID listenGamepads = nullptr;
  jmethodID stopListeningGamepads = nullptr;
  jmethodID isDeviceChanged = nullptr;
  jmethodID getConnectedGamepadIds = nullptr;
  jmethodID getDisplayRotation = nullptr;
  // optional: without it a button press doesn't switch the gamepad
  jmethodID selectGamepad = nullptr;

  bool valid() const
  {
    return helperClass && listenGamepads && stopListeningGamepads && isDeviceChanged && getConnectedGamepadIds && getDisplayRotation;
  }
};

GamepadHelperJni gamepad_helper;
dag::mutex gamepad_helper_mutex;
std::atomic<bool> is_input_device_changed = false;

bool check_exception(JNIEnv *env, const char *where) { return hid_jni::check_exception(env, where, "AndroidJoyHelper"); }

jmethodID get_static_method(JNIEnv *env, jclass cls, const char *name, const char *signature)
{
  jmethodID id = env->GetStaticMethodID(cls, name, signature);
  return check_exception(env, name) ? nullptr : id;
}

// a method the GamepadHelper class may lack; its absence is no error
jmethodID get_optional_static_method(JNIEnv *env, jclass cls, const char *name, const char *signature)
{
  jmethodID id = env->GetStaticMethodID(cls, name, signature);
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    logdbg("AndroidJoyHelper: GamepadHelper.%s is not found", name);
    return nullptr;
  }
  return id;
}

void resolve_gamepad_helper(JNIEnv *env, void *activity)
{
  hid_jni::release_global_ref(env, gamepad_helper.helperClass);
  gamepad_helper = GamepadHelperJni{};
  gamepad_helper.activity = activity;
  gamepad_helper.vm = android::get_java_vm(activity);

  hid_jni::LocalFrame frame(env, 8, "AndroidJoyHelper");
  if (!frame.valid())
    return;

  jobject activityObj = android::get_activity_class(activity);
  if (!activityObj)
    return;
  jclass activityClass = env->GetObjectClass(activityObj);
  jmethodID getClassLoaderID =
    activityClass ? env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
  if (check_exception(env, "getClassLoader id") || !getClassLoaderID)
    return;
  jobject classLoader = env->CallObjectMethod(activityObj, getClassLoaderID);
  if (check_exception(env, "getClassLoader") || !classLoader)
    return;
  jclass classLoaderClass = env->GetObjectClass(classLoader);
  jmethodID loadClassID =
    classLoaderClass ? env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
  if (check_exception(env, "loadClass id") || !loadClassID)
    return;
  jstring className = env->NewStringUTF("com/gaijinent/helpers/GamepadHelper");
  if (check_exception(env, "NewStringUTF") || !className)
    return;
  jclass helperClass = (jclass)env->CallObjectMethod(classLoader, loadClassID, className);
  if (check_exception(env, "loadClass GamepadHelper") || !helperClass)
  {
    logwarn("AndroidJoyHelper: GamepadHelper class is not found");
    return;
  }

  gamepad_helper.listenGamepads = get_static_method(env, helperClass, "listenGamepads", "(Landroid/app/Activity;)V");
  gamepad_helper.stopListeningGamepads = get_static_method(env, helperClass, "stopListeningGamepads", "()V");
  gamepad_helper.isDeviceChanged = get_static_method(env, helperClass, "isDeviceChanged", "()Z");
  gamepad_helper.getConnectedGamepadIds = get_static_method(env, helperClass, "getConnectedGamepadIds", "()[I");
  gamepad_helper.getDisplayRotation = get_static_method(env, helperClass, "getDisplayRotation", "(Landroid/app/Activity;)I");
  gamepad_helper.selectGamepad = get_optional_static_method(env, helperClass, "selectGamepad", "(I)Z");
  gamepad_helper.helperClass = (jclass)env->NewGlobalRef(helperClass);
  check_exception(env, "NewGlobalRef");

  if (!gamepad_helper.valid())
  {
    hid_jni::release_global_ref(env, gamepad_helper.helperClass);
    logwarn("AndroidJoyHelper: JNI init failed");
    return;
  }

  env->CallStaticVoidMethod(gamepad_helper.helperClass, gamepad_helper.listenGamepads, activityObj);
  check_exception(env, "listenGamepads");
}

JNIEnv *bind_gamepad_helper()
{
  void *activity = hid_jni::current_activity();
  JNIEnv *env = hid_jni::get_env(activity);
  if (!env)
    return nullptr;

  if (!gamepad_helper.valid() || gamepad_helper.activity != activity)
    resolve_gamepad_helper(env, activity);

  return gamepad_helper.valid() ? env : nullptr;
}
} // namespace


void HumanInput::JoyDeviceHelper::detach()
{
  dag::lock_guard lock(gamepad_helper_mutex);
  JNIEnv *env = hid_jni::get_env(gamepad_helper.vm);
  if (!env)
    return;
  if (gamepad_helper.valid())
  {
    env->CallStaticVoidMethod(gamepad_helper.helperClass, gamepad_helper.stopListeningGamepads);
    check_exception(env, "stopListeningGamepads");
  }
  hid_jni::release_global_ref(env, gamepad_helper.helperClass);
  gamepad_helper = GamepadHelperJni{};
}


bool HumanInput::JoyDeviceHelper::isDeviceChanged()
{
  dag::lock_guard lock(gamepad_helper_mutex);
  JNIEnv *env = bind_gamepad_helper();
  if (!env)
    return false;

  const bool res = env->CallStaticBooleanMethod(gamepad_helper.helperClass, gamepad_helper.isDeviceChanged);
  return check_exception(env, "isDeviceChanged") ? false : res;
}


bool HumanInput::JoyDeviceHelper::getConnectedGamepadIds(int &device_id, int &vendor_id, int &product_id)
{
  dag::lock_guard lock(gamepad_helper_mutex);
  JNIEnv *env = bind_gamepad_helper();
  if (!env)
    return false;

  hid_jni::LocalFrame frame(env, 1, "AndroidJoyHelper");
  if (!frame.valid())
    return false;

  jintArray ids = (jintArray)env->CallStaticObjectMethod(gamepad_helper.helperClass, gamepad_helper.getConnectedGamepadIds);
  if (check_exception(env, "getConnectedGamepadIds") || !ids || env->GetArrayLength(ids) != 3)
    return false;

  jint values[3];
  env->GetIntArrayRegion(ids, 0, 3, values);
  if (check_exception(env, "GetIntArrayRegion"))
    return false;

  device_id = values[0];
  vendor_id = values[1];
  product_id = values[2];
  return true;
}


bool HumanInput::JoyDeviceHelper::selectGamepad(int device_id)
{
  dag::lock_guard lock(gamepad_helper_mutex);
  JNIEnv *env = bind_gamepad_helper();
  if (!env || !gamepad_helper.selectGamepad)
    return false;

  const bool res = env->CallStaticBooleanMethod(gamepad_helper.helperClass, gamepad_helper.selectGamepad, (jint)device_id);
  return check_exception(env, "selectGamepad") ? false : res;
}


int HumanInput::JoyDeviceHelper::getRotation()
{
  static std::atomic<int> currentRotation = ROTATION_90;
  static std::atomic<int> nextTimeToCheckRotation = 0;

  if (DAGOR_UNLIKELY(get_time_msec() >= nextTimeToCheckRotation.load(std::memory_order_relaxed)))
  {
    dag::lock_guard lock(gamepad_helper_mutex);
    nextTimeToCheckRotation.store(get_time_msec() + 1000, std::memory_order_relaxed);
    if (JNIEnv *env = bind_gamepad_helper())
    {
      const int rotation = env->CallStaticIntMethod(gamepad_helper.helperClass, gamepad_helper.getDisplayRotation,
        android::get_activity_class(gamepad_helper.activity));
      if (!check_exception(env, "getDisplayRotation") && rotation >= 0)
        currentRotation.store(rotation, std::memory_order_relaxed);
    }
  }

  return currentRotation.load(std::memory_order_relaxed);
}


bool HumanInput::JoyDeviceHelper::isDeviceChangePending()
{
  static std::atomic<bool> isInputDeviceInited = false;
  if (!isInputDeviceInited.exchange(true) && isDeviceChanged())
    is_input_device_changed = true;
  return is_input_device_changed.load();
}


void HumanInput::JoyDeviceHelper::clearDeviceChanges() { is_input_device_changed = false; }


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
