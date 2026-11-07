// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_debug.h>
#include <jni.h>

namespace hid_jni
{

inline void *current_activity()
{
  android_app *state = (android_app *)win32_get_instance();
  return state ? state->activity : nullptr;
}

inline JNIEnv *get_env(JavaVM *vm)
{
  if (!vm)
    return nullptr;

  JNIEnv *env = nullptr;
  JavaVMAttachArgs args;
  args.version = JNI_VERSION_1_6;
  args.name = "NativeThread";
  args.group = nullptr;

  return android::attach_current_thread(vm, &env, &args) == JNI_OK ? env : nullptr;
}

inline JNIEnv *get_env(void *activity) { return get_env(activity ? android::get_java_vm(activity) : nullptr); }

inline bool check_exception(JNIEnv *env, const char *where, const char *log_tag = "jni")
{
  if (!env || !env->ExceptionCheck())
    return false;
  env->ExceptionDescribe();
  env->ExceptionClear();
  logwarn("[%s] JNI exception cleared after %s", log_tag, where ? where : "?");
  return true;
}

// RAII local-reference frame. Our threads are attached to the JVM manually and
// never return to Java, so their local reference tables are never popped by
// ART; without explicit frames every GetObjectClass/FindClass/CallObjectMethod
// leaks a slot until the 512-entry table overflows and aborts the process.
class LocalFrame
{
  JNIEnv *env;

public:
  LocalFrame(JNIEnv *e, jint capacity, const char *log_tag = "jni") : env(e)
  {
    if (env && env->PushLocalFrame(capacity) < 0)
    {
      check_exception(env, "PushLocalFrame", log_tag);
      env = nullptr;
    }
  }
  ~LocalFrame()
  {
    if (env)
      env->PopLocalFrame(nullptr);
  }
  bool valid() const { return env != nullptr; }

  LocalFrame(const LocalFrame &) = delete;
  LocalFrame &operator=(const LocalFrame &) = delete;
};

template <typename T>
inline void release_global_ref(JNIEnv *env, T &ref)
{
  if (env && ref)
    env->DeleteGlobalRef(ref);
  ref = nullptr;
}

} // namespace hid_jni
